import { ReactNode } from "react";

interface MetricCardProps {
  label: string;
  value: string;
  unit?: string;
  delta?: string;
  deltaPositive?: boolean;
  icon?: ReactNode;
  glowColor?: string;
  subValue?: string;
}

export function MetricCard({ label, value, unit, delta, deltaPositive = true, icon, glowColor = "#00d4ff", subValue }: MetricCardProps) {
  return (
    <div
      className="relative overflow-hidden rounded-sm border p-4"
      style={{
        background: "linear-gradient(135deg, #040c14 0%, #071828 100%)",
        borderColor: `${glowColor}22`,
        boxShadow: `0 0 20px ${glowColor}15, inset 0 0 40px ${glowColor}05`,
      }}
    >
      <div className="absolute top-0 left-0 right-0 h-px" style={{ background: `linear-gradient(90deg, transparent, ${glowColor}80, transparent)` }} />
      <div className="flex items-start justify-between mb-2">
        <span className="text-xs tracking-widest uppercase" style={{ color: "#4a7a9b", fontFamily: "JetBrains Mono, monospace" }}>
          {label}
        </span>
        {icon && <span style={{ color: glowColor }}>{icon}</span>}
      </div>
      <div className="flex items-end gap-2">
        <span className="leading-none" style={{ fontFamily: "Orbitron, monospace", color: glowColor, fontSize: "1.75rem", fontWeight: 700, textShadow: `0 0 20px ${glowColor}` }}>
          {value}
        </span>
        {unit && <span className="mb-1 text-xs" style={{ color: "#4a7a9b", fontFamily: "JetBrains Mono, monospace" }}>{unit}</span>}
      </div>
      {subValue && <div className="mt-1 text-xs" style={{ color: "#a0c8e8", fontFamily: "JetBrains Mono, monospace" }}>{subValue}</div>}
      {delta && (
        <div className="mt-2 flex items-center gap-1">
          <span className="text-xs" style={{ color: deltaPositive ? "#00ff88" : "#ff3366", fontFamily: "JetBrains Mono, monospace" }}>
            {deltaPositive ? "▲" : "▼"} {delta}
          </span>
          <span className="text-xs" style={{ color: "#4a7a9b", fontFamily: "JetBrains Mono, monospace" }}>vs C199.22</span>
        </div>
      )}
    </div>
  );
}
