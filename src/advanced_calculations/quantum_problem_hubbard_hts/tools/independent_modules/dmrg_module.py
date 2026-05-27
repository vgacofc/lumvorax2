from __future__ import annotations
import math
from statistics import fmean

def run_dmrg(series: list[float], max_sweeps: int, power_tol: float) -> dict:
    vals=[x for x in series if math.isfinite(x)]
    if len(vals) < 6:
        return {"status":"FAIL","reason":"insufficient_points"}
    v=[x-fmean(vals) for x in vals]
    n=len(v)
    # Build tridiagonal effective Hamiltonian from local gradients
    diag=[abs(v[i]) + abs(v[i-1] if i>0 else v[i]) for i in range(n)]
    off=[abs(v[i+1]-v[i]) for i in range(n-1)]
    x=[1.0/math.sqrt(n)]*n
    prev=0.0
    sweeps=max(4,max_sweeps)
    for _ in range(sweeps):
        y=[0.0]*n
        for i in range(n):
            y[i]+=diag[i]*x[i]
            if i>0: y[i]+=off[i-1]*x[i-1]
            if i<n-1: y[i]+=off[i]*x[i+1]
        norm=math.sqrt(sum(t*t for t in y)) or 1e-15
        x=[t/norm for t in y]
        lam=sum(x[i]*((diag[i]*x[i]) + (off[i-1]*x[i-1] if i>0 else 0.0) + (off[i]*x[i+1] if i<n-1 else 0.0)) for i in range(n))
        if abs(lam-prev) < power_tol:
            break
        prev=lam
    ent_fullscale=-sum((abs(t)+1e-15)*math.log(abs(t)+1e-15) for t in x)
    return {
        "status":"PASS",
        "effective_ground_energy":lam,
        "entanglement_index":ent_fullscale,
        "sweeps":sweeps
    }
