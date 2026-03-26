# RAPPORT REPLIT EXECUTION V6 FINAL

## 1) Intégrité manifest V6
- Manifest: `results/manifest_forensic_v6.json`
- Verify exit code: 0
```
artifact_count=11
missing=0
mismatches=0
```

## 2) Capture hardware réelle
- report_file: `results/ADC_REAL_HARDWARE_CAPTURE_V6.json`
- goal: `real_hardware_capture_on_replit`
- timestamp_utc: `2026-03-02T16:39:50Z`
- source=/dev/hwrng size=0 read_elapsed_s=None bytes_per_sec=None error=[Errno 2] No such file or directory: '/dev/hwrng'
- source=/dev/random size=32768 read_elapsed_s=0.00010571000166237354 bytes_per_sec=309980129.4550869 error=None
- source=/dev/urandom size=32768 read_elapsed_s=8.174999675247818e-05 bytes_per_sec=400831820.20440465 error=None
- jitter samples=50000 elapsed_s=0.006668349997198675 ops_per_sec=7498106.731201067 nqubit_per_sec_proxy=7498106.731201067 mean_ns=133.3212 stddev_ns=1385.9077853993606

## 3) Métriques système/hardware complètes
- cpu_count=8
- platform=Linux-6.14.11-x86_64-with-glibc2.40
- loadavg={'load_1m': 8.9169921875, 'load_5m': 3.4013671875, 'load_15m': 2.10400390625}
- disk_root={'total_bytes': 79144566784, 'used_bytes': 39432732672, 'free_bytes': 36357648384}
- cpu_stat={'cpu_total_ticks': 28417158, 'cpu_idle_ticks': 23900340}
- mem_total=65849792 kB mem_available=19872988 kB mem_free=4382608 kB

## 4) Stress telemetry
```
# V6 STRESS TELEMETRY
workers=64
duration_s=4
cpu_pressure_estimated_pct=100.00
memory_peak_pct=69.92
target_pct=99.00
status=TARGET_REACHED

```

## 5) Conclusion
- GO technique V6 si verify=0 et au moins une source hardware capturée sans erreur.
- ADC physique certifiée laboratoire: NON (ce run est environnement Replit).
