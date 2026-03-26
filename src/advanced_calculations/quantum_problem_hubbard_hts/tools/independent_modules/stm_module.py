from __future__ import annotations
import math

def run_stm(series: list[float], rolling_window: int=5) -> dict:
    vals=[x for x in series if math.isfinite(x)]
    if len(vals) < max(rolling_window,4):
        return {"status":"FAIL","reason":"insufficient_points"}
    rw=max(rolling_window,2)
    smooth=[]
    for i in range(len(vals)):
        a=max(0,i-rw+1)
        win=vals[a:i+1]
        smooth.append(sum(win)/len(win))
    residual=[vals[i]-smooth[i] for i in range(len(vals))]
    roughness=math.sqrt(sum(r*r for r in residual)/len(residual))
    contrast=max(vals)-min(vals)
    ldos_index=sum(abs(v) for v in smooth)/len(smooth)
    return {
        "status":"PASS",
        "ldos_index":ldos_index,
        "surface_roughness":roughness,
        "contrast":contrast
    }
