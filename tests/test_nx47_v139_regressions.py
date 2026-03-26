from pathlib import Path

from nx47_vesu_kernel_v139 import V139Config


def test_v139_defaults_keep_kaggle_safe_coverage_profile():
    cfg = V139Config()
    assert cfg.max_train_volumes == 24
    assert cfg.max_val_volumes == 8
    assert cfg.preflight_train_pct == 5.0
    assert cfg.preflight_test_pct == 5.0


def test_v139_has_stall_alert_guardrail_config():
    cfg = V139Config()
    assert cfg.stage_stall_alert_s > 0.0


def test_v139_exec_complete_logged_before_logs_dump():
    src = Path('nx47_vesu_kernel_v139.py').read_text(encoding='utf-8')
    i_exec = src.find("self.log('EXEC_COMPLETE'")
    i_dump = src.find('self.logs_path.write_text')
    assert i_exec != -1 and i_dump != -1 and i_exec < i_dump


def test_v139_progress_payload_has_eta_and_stall_alert():
    src = Path('nx47_vesu_kernel_v139.py').read_text(encoding='utf-8')
    assert 'eta_s=' in src
    assert "self.log('STALL_ALERT'" in src


def test_v139_training_outputs_include_weight_delta_and_ablation_blocks():
    src = Path('nx47_vesu_kernel_v139.py').read_text(encoding='utf-8')
    assert 'weight_delta_l2' in src
    assert "'ablation_check': ablation" in src
    assert "'stability_probe': stability" in src
