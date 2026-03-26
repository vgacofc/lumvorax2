#!/usr/bin/env python3
import argparse
import json
import subprocess
from pathlib import Path


def run(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return p.returncode, p.stdout.strip(), p.stderr.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--adc-report', default='results/ADC_REAL_HARDWARE_CAPTURE_V6.json')
    ap.add_argument('--stress-log', default='results/stress_telemetry_v6.log')
    ap.add_argument('--system-metrics', default='results/system_metrics_v6.json')
    ap.add_argument('--output', default='RAPPORT_REPLIT_EXECUTION_V6_FINAL.md')
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[1]
    verify = root / 'tools' / 'verify_manifest_v6.py'
    rc, out, _ = run(['python', str(verify), '--manifest', args.manifest])

    adc = {}
    adc_path = Path(args.adc_report)
    if adc_path.exists():
        adc = json.loads(adc_path.read_text(encoding='utf-8'))

    sysm = {}
    sysm_path = Path(args.system_metrics)
    if sysm_path.exists():
        sysm = json.loads(sysm_path.read_text(encoding='utf-8'))

    stress_txt = Path(args.stress_log).read_text(encoding='utf-8') if Path(args.stress_log).exists() else 'N/A'

    lines = [
        '# RAPPORT REPLIT EXECUTION V6 FINAL',
        '',
        '## 1) Intégrité manifest V6',
        f'- Manifest: `{args.manifest}`',
        f'- Verify exit code: {rc}',
        '```',
        out if out else '(no output)',
        '```',
        '',
        '## 2) Capture hardware réelle',
        f"- report_file: `{args.adc_report}`",
        f"- goal: `{adc.get('goal', 'n/a')}`",
        f"- timestamp_utc: `{adc.get('timestamp_utc', 'n/a')}`",
    ]
    for src in adc.get('sources', []):
        lines.append(
            f"- source={src.get('source')} size={src.get('size')} read_elapsed_s={src.get('read_elapsed_s')} bytes_per_sec={src.get('bytes_per_sec')} error={src.get('error')}"
        )
    if adc.get('jitter'):
        j = adc['jitter']
        lines.append(
            f"- jitter samples={j.get('samples')} elapsed_s={j.get('elapsed_s')} ops_per_sec={j.get('ops_per_sec')} nqubit_per_sec_proxy={j.get('nqubit_per_sec_proxy')} mean_ns={j.get('mean_ns')} stddev_ns={j.get('stddev_ns')}"
        )

    lines += ['', '## 3) Métriques système/hardware complètes']
    if sysm:
        lines.append(f"- cpu_count={sysm.get('system', {}).get('cpu_count')}")
        lines.append(f"- platform={sysm.get('system', {}).get('platform')}")
        lines.append(f"- loadavg={sysm.get('loadavg')}")
        lines.append(f"- disk_root={sysm.get('disk_root')}")
        lines.append(f"- cpu_stat={sysm.get('cpu_stat')}")
        mem = sysm.get('memory', {})
        lines.append(f"- mem_total={mem.get('MemTotal')} mem_available={mem.get('MemAvailable')} mem_free={mem.get('MemFree')}")
    else:
        lines.append('- N/A (fichier system_metrics_v6.json absent)')

    lines += [
        '',
        '## 4) Stress telemetry',
        '```',
        stress_txt,
        '```',
        '',
        '## 5) Conclusion',
        '- GO technique V6 si verify=0 et au moins une source hardware capturée sans erreur.',
        '- ADC physique certifiée laboratoire: NON (ce run est environnement Replit).',
    ]

    Path(args.output).write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'report_written={args.output}')


if __name__ == '__main__':
    main()
