from __future__ import annotations
import math, random
from statistics import fmean

def run_qmc(series: list[float], burn_in_fraction: float, iterations_multiplier: int, seed: int) -> dict:
    clean=[x for x in series if math.isfinite(x)]
    if len(clean) < 8:
        return {"status":"FAIL","reason":"insufficient_points"}
    rng=random.Random(seed)
    mu=fmean(clean)
    sigma=math.sqrt(fmean([(x-mu)**2 for x in clean])) or 1e-12
    norm=[(x-mu)/sigma for x in clean]
    n_iter=max(len(norm)*max(iterations_multiplier,1), len(norm))
    burn=max(int(n_iter*burn_in_fraction),1)
    idx=rng.randrange(len(norm))
    accepted=0
    samples=[]
    for it in range(n_iter):
        cand=(idx + rng.choice([-1,1])) % len(norm)
        dE=norm[cand]**2 - norm[idx]**2
        if dE <= 0 or rng.random() < math.exp(-dE):
            idx=cand
            accepted+=1
        if it >= burn:
            samples.append(clean[idx])
    if not samples:
        return {"status":"FAIL","reason":"no_samples"}
    m=fmean(samples)
    v=fmean([(x-m)**2 for x in samples])
    return {
        "status":"PASS",
        "samples":len(samples),
        "acceptance_rate":accepted/n_iter,
        "mean":m,
        "variance":v,
        "std":math.sqrt(v)
    }
