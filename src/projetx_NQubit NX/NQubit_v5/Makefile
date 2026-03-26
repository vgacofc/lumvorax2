PYTHON ?= python

.PHONY: test_unit test_integration test_stress test_all report_v5

test_unit:
	$(PYTHON) tests/test_manifest_tools.py

test_integration:
	$(PYTHON) tests/test_manifest_integration.py

test_stress:
	$(PYTHON) tools/stress_harness_v5.py --seconds 3 --workers 64 --target-cpu-ram 99 --output results/stress_telemetry_v5.log

test_all: test_unit test_integration test_stress

report_v5:
	$(PYTHON) tools/generate_v5_post_run_report.py --manifest results/manifest_forensic_v5.json --output RAPPORT_REPLIT_EXECUTION_V5_FINAL.md
