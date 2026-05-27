# RAPPORT REPLIT EXECUTION V6 FINAL

## 1) Intégrité manifest V6
- Manifest: `results/manifest_forensic_v6.json`
- Verify exit code: 0
```
artifact_count=9
missing=0
mismatches=0
```

## 2) Capture hardware réelle
- report_file: `results/ADC_REAL_HARDWARE_CAPTURE_V6.json`
- goal: `real_hardware_capture_on_replit`
- timestamp_utc: `2026-03-01T14:34:42Z`
- source=/dev/hwrng size=0 read_elapsed_s=None bytes_per_sec=None error=[Errno 2] No such file or directory: '/dev/hwrng'
- source=/dev/random size=32768 read_elapsed_s=0.00023873400004958967 bytes_per_sec=137257365.91014877 error=None
- source=/dev/urandom size=32768 read_elapsed_s=0.000245641999754298 bytes_per_sec=133397383.31708747 error=None
- jitter samples=50000 elapsed_s=0.021633696000208147 ops_per_sec=2311209.3282404877 nqubit_per_sec_proxy=2311209.3282404877 mean_ns=432.74296 stddev_ns=8503.185822561472

## 3) Métriques système/hardware complètes
- cpu_count=3
- platform=Linux-6.12.47-x86_64-with-glibc2.39
- loadavg={'load_1m': 10.525390625, 'load_5m': 2.4697265625, 'load_15m': 0.80908203125}
- disk_root={'total_bytes': 67049664512, 'used_bytes': 31251054592, 'free_bytes': 32345862144}
- cpu_stat={'cpu_total_ticks': 728778, 'cpu_idle_ticks': 715018}
- mem_total=18801884 kB mem_available=18180216 kB mem_free=13826512 kB

## 4) Stress telemetry
```
# V6 STRESS TELEMETRY
workers=64
duration_s=4
cpu_pressure_estimated_pct=100.00
memory_peak_pct=3.91
target_pct=99.00
status=TARGET_REACHED

```

## 5) Conclusion
- GO technique V6 si verify=0 et au moins une source hardware capturée sans erreur.
- ADC physique certifiée laboratoire: NON (ce run est environnement Catlin/Kaggle/Replit).
