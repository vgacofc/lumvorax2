from __future__ import annotations
import cmath, math

def run_arpes(series: list[float], window: str='hann') -> dict:
    vals=[x for x in series if math.isfinite(x)]
    n=len(vals)
    if n < 16:
        return {"status":"FAIL","reason":"insufficient_points"}
    if window == 'hann':
        w=[0.5 - 0.5*math.cos(2*math.pi*i/(n-1)) for i in range(n)]
    else:
        w=[1.0]*n
    x=[vals[i]*w[i] for i in range(n)]
    mu=sum(x)/n
    x=[v-mu for v in x]
    amps=[]
    for k in range(1,n//2):
        s=0j
        for t,val in enumerate(x):
            s += val * cmath.exp(-2j*math.pi*k*t/n)
        amps.append((k,abs(s)/n))
    kmax,amax=max(amps,key=lambda p:p[1])
    spec_weight=sum(a for _,a in amps)
    return {
        "status":"PASS",
        "peak_mode":kmax,
        "peak_amplitude":amax,
        "spectral_weight":spec_weight,
        "spectral_sharpness":amax/(spec_weight+1e-15)
    }
