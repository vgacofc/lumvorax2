# Analyse bas niveau exhaustive — run_id 20260303_190540

## Ce qui a été ajouté (instrumentation bas niveau supplémentaire)

Instrumentation **nouvellement incluse** dans le benchmark strict, en plus de la latence et des débits:

1. **Compteurs process enfant (kernel rusage delta)**
   - `child_minflt`, `child_majflt` (page faults mineurs/majeurs)
   - `child_inblock`, `child_oublock` (I/O bloc)
   - `child_nvcsw`, `child_nivcsw` (context switches volontaires/non volontaires)
   - `child_nsignals`, `child_nswap`, `child_msgsnd`, `child_msgrcv`

2. **Compteurs hôte pendant chaque workload**
   - delta jiffies CPU total/busy (`host_cpu_jiffies_total_delta`, `host_cpu_jiffies_busy_delta`)
   - delta vmstat (`pgfault`, `pgmajfault`, `pswpin`, `pswpout`, scans/steals kswapd/direct)

3. **Métriques système/cgroup/pression**
   - `kernel_release`, `machine`
   - `self_vm_peak_kb`, `self_vm_hwm_kb`, `self_threads`, `self_voluntary_ctxt_switches`, `self_nonvoluntary_ctxt_switches`
   - `cgroup_memory_max`, `cgroup_memory_current`, `cgroup_cpu_max`
   - `pressure_cpu`, `pressure_memory`, `pressure_io` (si disponibles)

## Exécution réalisée

Commande utilisée:

```bash
bash src/advanced_calculations/quantum_simulator_v5_competitor_cpu/run_on_replit_v5_competitors.sh /workspace/Lumvorax 5 60 200 36 0 0 "Qulacs" 1 24 8
```

- strict unifié activé,
- 1 concurrent (`Qulacs`) + notre simulateur,
- protocole auto-maximisé,
- shots montés à 2048 pour cette validation.

## Résultat strict

- `strict_unified_workloads_total = 10`
- `strict_unified_workloads_ok = 10`
- `strict_unified_workload_success_rate = 1.0`

=> Aucun échec runtime sur la phase stricte.

## Réponse directe à la demande « tout inclure »

Ce run inclut maintenant une instrumentation bas niveau beaucoup plus large côté process + noyau + cgroup.

### Ce qui n'est toujours pas capturé (limites environnement/tooling)

Pour être totalement transparent, ces métriques demandent des privilèges/outils non garantis dans cet environnement:
- compteurs micro-architecturaux CPU (cache misses, branch misses, cycles/instructions `perf_event_open`),
- télémétrie RAPL énergie/puissance,
- traces eBPF/kprobe et flamegraphs noyau,
- PMU par cœur avec isolation cpuset stricte,
- NUMA détaillé par page et migration live.

Donc: **on a intégré un gros niveau bas système réellement accessible ici**, mais “absolument tout” au sens forensique hardware complet nécessite un runner privilégié dédié.

