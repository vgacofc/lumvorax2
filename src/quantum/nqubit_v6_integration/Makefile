PYTHON ?= python

.PHONY: collect_metrics acquire build_manifest verify_manifest test_unit test_integration test_stress test_all report_v6 compare_line_by_line run_a_to_z

collect_metrics:
	$(PYTHON) tools/collect_system_metrics_v6.py

acquire:
	$(PYTHON) tools/acquire_real_hardware_v6.py --output-dir results --bytes 32768 --jitter-samples 50000

build_manifest:
	$(PYTHON) tools/build_manifest_v6.py --input-dir results --output results/manifest_forensic_v6.json --exclude manifest_forensic_v6.json manifest_forensic_v6.json.sig

verify_manifest:
	$(PYTHON) tools/verify_manifest_v6.py --manifest results/manifest_forensic_v6.json

test_unit:
	$(PYTHON) tests/test_manifest_tools_v6.py
	$(PYTHON) tests/test_hardware_capture_v6.py
	$(PYTHON) tests/test_system_metrics_v6.py
	$(PYTHON) tests/test_kernel_v6.py

test_integration: collect_metrics acquire build_manifest verify_manifest

test_stress:
	$(PYTHON) tools/stress_harness_v6.py --seconds 4 --workers 64 --target 99 --output results/stress_telemetry_v6.log

test_all: test_unit test_integration test_stress

report_v6: collect_metrics acquire test_stress build_manifest verify_manifest
	$(PYTHON) tools/generate_v6_post_run_report.py --manifest results/manifest_forensic_v6.json --adc-report results/ADC_REAL_HARDWARE_CAPTURE_V6.json --stress-log results/stress_telemetry_v6.log --system-metrics results/system_metrics_v6.json --output RAPPORT_REPLIT_EXECUTION_V6_FINAL.md


compare_line_by_line:
	$(PYTHON) tools/compare_line_by_line_with_source_v6.py

run_a_to_z:
	$(PYTHON) run_a_to_z_v6_integration.py
