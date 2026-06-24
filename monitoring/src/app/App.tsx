import {
  AreaChart, Area, BarChart, Bar, LineChart, Line,
  XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, RadarChart, PolarGrid,
  PolarAngleAxis, Radar, Legend,
} from "recharts";
import { MetricCard } from "./components/MetricCard";
import { ExecutionPanel } from "./components/ExecutionPanel";

const tpsEvolution = [
  { cycle: "C199.21", lumvorax: 485, solana: 400 },
  { cycle: "C199.22", lumvorax: 298, solana: 400 },
  { cycle: "C199.23", lumvorax: 355, solana: 400 },
];

const throughputEvolution = [
  { cycle: "C199.21", throughput: 0.485 },
  { cycle: "C199.22", throughput: 329 },
  { cycle: "C199.23", throughput: 391 },
];

const latencyEvolution = [
  { cycle: "C199.21", latence: 3.0 },
  { cycle: "C199.22", latence: 4.78 },
  { cycle: "C199.23", latence: 2.41 },
];

const overheadEvolution = [
  { cycle: "C199.21", overhead: 0 },
  { cycle: "C199.22", overhead: 38 },
  { cycle: "C199.23", overhead: 27 },
];

const simdRadar = [
  { metric: "SSE4.2", value: 100 },
  { metric: "AES-NI", value: 100 },
  { metric: "AVX2", value: 0 },
  { metric: "TPS Gain", value: 63 },
  { metric: "Latency Red.", value: 97 },
  { metric: "Cache Opt.", value: 75 },
];

const tableData = [
  { metric: "PoH Ticks/s", c21: "485K", c22: "298K", c23: "355K", delta: "+19.1%", ok: true },
  { metric: "vs Solana 400K", c21: "+21.2%", c22: "-25.4%", c23: "-11.2%", delta: "Amélioration", ok: true },
  { metric: "Throughput Vérif.", c21: "485K", c22: "329M", c23: "391M", delta: "+18.8%", ok: true },
  { metric: "Latence Vérif.", c21: "3ms", c22: "4.78ms", c23: "2.41ms", delta: "-49.6%", ok: true },
  { metric: "Overhead Forensic", c21: "0.0%", c22: "~38%", c23: "~27%", delta: "-11%", ok: true },
];

const CustomTooltip = ({ active, payload, label }: any) => {
  if (active && payload && payload.length) {
    return (
      <div style={{ background: "#040c14", border: "1px solid #00d4ff33", padding: "8px 12px", fontFamily: "JetBrains Mono, monospace", fontSize: "0.7rem" }}>
        <div style={{ color: "#4a7a9b", marginBottom: 4 }}>{label}</div>
        {payload.map((p: any, i: number) => (
          <div key={i} style={{ color: p.color }}>{p.name}: {p.value}</div>
        ))}
      </div>
    );
  }
  return null;
};

export default function App() {
  return (
    <div className="min-h-screen" style={{ background: "#020408", fontFamily: "Rajdhani, sans-serif", color: "#e2f0ff" }}>
      {/* Ambient background grid */}
      <div className="fixed inset-0 pointer-events-none" style={{
        backgroundImage: "linear-gradient(rgba(0,212,255,0.03) 1px, transparent 1px), linear-gradient(90deg, rgba(0,212,255,0.03) 1px, transparent 1px)",
        backgroundSize: "40px 40px",
      }} />

      <div className="relative z-10 max-w-screen-2xl mx-auto px-6 py-8">
        {/* Header */}
        <div className="flex items-start justify-between mb-8">
          <div>
            <div className="flex items-center gap-3 mb-1">
              <div className="w-8 h-px" style={{ background: "linear-gradient(90deg, transparent, #00d4ff)" }} />
              <span style={{ fontFamily: "JetBrains Mono, monospace", color: "#4a7a9b", fontSize: "0.65rem", letterSpacing: "0.2em" }}>
                RAPPORT PHASE 3 — SIMD OPTIMIZER DÉTECTION RUNTIME
              </span>
            </div>
            <h1 style={{ fontFamily: "Orbitron, monospace", fontSize: "2rem", fontWeight: 900, letterSpacing: "0.1em", color: "#e2f0ff", textShadow: "0 0 30px #00d4ff60", lineHeight: 1.2 }}>
              LUM<span style={{ color: "#00d4ff" }}>VORAX</span>
            </h1>
            <div style={{ fontFamily: "Rajdhani, sans-serif", color: "#4a7a9b", fontSize: "0.85rem", letterSpacing: "0.15em", marginTop: "2px" }}>
              BLOCKCHAIN PERFORMANCE DASHBOARD — CYCLE C199.23
            </div>
          </div>
          <div className="text-right">
            <div className="flex items-center justify-end gap-2 mb-1">
              <div className="w-2 h-2 rounded-full animate-pulse" style={{ background: "#00ff88", boxShadow: "0 0 8px #00ff88" }} />
              <span style={{ fontFamily: "JetBrains Mono, monospace", color: "#00ff88", fontSize: "0.65rem", letterSpacing: "0.15em" }}>SYSTÈME OPÉRATIONNEL</span>
            </div>
            <div style={{ fontFamily: "JetBrains Mono, monospace", color: "#2a4a5b", fontSize: "0.6rem" }}>
              Intel Core i5-8265U @ 1.60GHz | 4C/8T
            </div>
            <div style={{ fontFamily: "JetBrains Mono, monospace", color: "#2a4a5b", fontSize: "0.6rem" }}>
              SSE4.2 ✓ | AES-NI ✓ | AVX2 ✗
            </div>
          </div>
        </div>

        {/* ── BOUTON EXÉCUTION — PLEINE LARGEUR EN HAUT ── */}
        <div className="rounded-sm border flex items-center justify-center mb-6" style={{ background: "#020810", borderColor: "#00d4ff15", minHeight: 560 }}>
          <ExecutionPanel />
        </div>

        {/* KPI Cards */}
        <div className="grid grid-cols-2 gap-4 mb-6" style={{ gridTemplateColumns: "repeat(4, 1fr)" }}>
          <MetricCard
            label="TPS Génération PoH"
            value="355,211"
            unit="ticks/s"
            delta="+19.1%"
            deltaPositive={true}
            glowColor="#00d4ff"
            subValue="C199.23 — SSE4.2 Vectorisé"
          />
          <MetricCard
            label="Throughput Vérification"
            value="391M"
            unit="verif/s"
            delta="+18.8%"
            deltaPositive={true}
            glowColor="#00ff88"
            subValue="3,919 batches traités"
          />
          <MetricCard
            label="Latence Vérification"
            value="2.41"
            unit="ms"
            delta="-49.6%"
            deltaPositive={true}
            glowColor="#ffd700"
            subValue="4.78ms → 2.41ms"
          />
          <MetricCard
            label="Erreurs Détectées"
            value="0"
            unit="/ 3.9M ticks"
            glowColor="#00ff88"
            subValue="Intégrité forensique ✓"
          />
        </div>

        {/* Charts Row 1 */}
        <div className="grid gap-4 mb-4" style={{ gridTemplateColumns: "1fr 1fr 1fr" }}>
          {/* TPS vs Solana */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#00d4ff15" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#00d4ff", boxShadow: "0 0 8px #00d4ff" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                POH TICKS/S — ÉVOLUTION 3 CYCLES
              </span>
            </div>
            <ResponsiveContainer width="100%" height={180}>
              <BarChart data={tpsEvolution} barGap={4}>
                <CartesianGrid strokeDasharray="2 4" stroke="#0a1e2e" vertical={false} />
                <XAxis dataKey="cycle" tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} unit="K" />
                <Tooltip content={<CustomTooltip />} />
                <Bar dataKey="lumvorax" name="LumVorax" fill="#00d4ff" radius={[2, 2, 0, 0]} style={{ filter: "drop-shadow(0 0 6px #00d4ff80)" }} />
                <Bar dataKey="solana" name="Solana" fill="#7c3aed44" stroke="#7c3aed" strokeWidth={1} radius={[2, 2, 0, 0]} />
              </BarChart>
            </ResponsiveContainer>
            <div className="flex gap-4 mt-2">
              <div className="flex items-center gap-1.5"><div className="w-2 h-2" style={{ background: "#00d4ff" }} /><span style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#4a7a9b" }}>LumVorax</span></div>
              <div className="flex items-center gap-1.5"><div className="w-2 h-2" style={{ background: "#7c3aed" }} /><span style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#4a7a9b" }}>Solana 400K ref</span></div>
            </div>
          </div>

          {/* Latency */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#ffd70015" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#ffd700", boxShadow: "0 0 8px #ffd700" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                LATENCE VÉRIFICATION (ms)
              </span>
            </div>
            <ResponsiveContainer width="100%" height={180}>
              <AreaChart data={latencyEvolution}>
                <defs>
                  <linearGradient id="latGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#ffd700" stopOpacity={0.3} />
                    <stop offset="95%" stopColor="#ffd700" stopOpacity={0} />
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="2 4" stroke="#0a1e2e" vertical={false} />
                <XAxis dataKey="cycle" tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} unit="ms" />
                <Tooltip content={<CustomTooltip />} />
                <Area type="monotone" dataKey="latence" name="Latence" stroke="#ffd700" fill="url(#latGrad)" strokeWidth={2} dot={{ fill: "#ffd700", r: 4, strokeWidth: 0 }} />
              </AreaChart>
            </ResponsiveContainer>
          </div>

          {/* SIMD Radar */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#00ff8815" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#00ff88", boxShadow: "0 0 8px #00ff88" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                PROFIL SIMD & OPTIMISATION
              </span>
            </div>
            <ResponsiveContainer width="100%" height={180}>
              <RadarChart data={simdRadar}>
                <PolarGrid stroke="#0a1e2e" />
                <PolarAngleAxis dataKey="metric" tick={{ fill: "#4a7a9b", fontSize: 9, fontFamily: "JetBrains Mono, monospace" }} />
                <Radar name="C199.23" dataKey="value" stroke="#00ff88" fill="#00ff88" fillOpacity={0.15} strokeWidth={2} />
              </RadarChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Charts Row 2 + Table */}
        <div className="grid gap-4 mb-6" style={{ gridTemplateColumns: "1fr 1fr 1fr" }}>
          {/* Throughput */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#00ff8815" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#00ff88", boxShadow: "0 0 8px #00ff88" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                THROUGHPUT VÉRIFICATION (M verif/s)
              </span>
            </div>
            <ResponsiveContainer width="100%" height={150}>
              <AreaChart data={throughputEvolution}>
                <defs>
                  <linearGradient id="tpGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#00ff88" stopOpacity={0.4} />
                    <stop offset="95%" stopColor="#00ff88" stopOpacity={0} />
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="2 4" stroke="#0a1e2e" vertical={false} />
                <XAxis dataKey="cycle" tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} />
                <Tooltip content={<CustomTooltip />} />
                <Area type="monotone" dataKey="throughput" name="M verif/s" stroke="#00ff88" fill="url(#tpGrad)" strokeWidth={2} dot={{ fill: "#00ff88", r: 4, strokeWidth: 0 }} />
              </AreaChart>
            </ResponsiveContainer>
          </div>

          {/* Overhead */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#ff6b3515" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#ff6b35", boxShadow: "0 0 8px #ff6b35" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                OVERHEAD FORENSIC (%) — RÉDUCTION
              </span>
            </div>
            <ResponsiveContainer width="100%" height={150}>
              <LineChart data={overheadEvolution}>
                <CartesianGrid strokeDasharray="2 4" stroke="#0a1e2e" vertical={false} />
                <XAxis dataKey="cycle" tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fill: "#4a7a9b", fontSize: 10, fontFamily: "JetBrains Mono, monospace" }} axisLine={false} tickLine={false} unit="%" />
                <Tooltip content={<CustomTooltip />} />
                <Line type="monotone" dataKey="overhead" name="Overhead %" stroke="#ff6b35" strokeWidth={2} dot={{ fill: "#ff6b35", r: 5, strokeWidth: 0 }} style={{ filter: "drop-shadow(0 0 4px #ff6b3580)" }} />
              </LineChart>
            </ResponsiveContainer>
          </div>

          {/* Comparison Table */}
          <div className="rounded-sm border p-4" style={{ background: "#040c14", borderColor: "#00d4ff15" }}>
            <div className="flex items-center gap-2 mb-4">
              <div className="w-1 h-4" style={{ background: "#00d4ff", boxShadow: "0 0 8px #00d4ff" }} />
              <span style={{ fontFamily: "Rajdhani, sans-serif", fontWeight: 700, fontSize: "0.8rem", letterSpacing: "0.12em", color: "#a0c8e8" }}>
                TABLEAU COMPARATIF 3 CYCLES
              </span>
            </div>
            <table className="w-full" style={{ borderCollapse: "collapse" }}>
              <thead>
                <tr>
                  {["MÉTRIQUE", "C199.21", "C199.22", "C199.23", "Δ"].map(h => (
                    <th key={h} className="pb-2 text-left" style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.55rem", color: "#2a4a5b", letterSpacing: "0.12em", borderBottom: "1px solid #0a1e2e", paddingRight: "8px" }}>
                      {h}
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {tableData.map((row, i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #071020" }}>
                    <td className="py-1.5 pr-2" style={{ fontFamily: "Rajdhani, sans-serif", fontSize: "0.7rem", color: "#a0c8e8", fontWeight: 600 }}>{row.metric}</td>
                    <td className="py-1.5 pr-2" style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#4a7a9b" }}>{row.c21}</td>
                    <td className="py-1.5 pr-2" style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#4a7a9b" }}>{row.c22}</td>
                    <td className="py-1.5 pr-2" style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#00d4ff", textShadow: "0 0 8px #00d4ff60" }}>{row.c23}</td>
                    <td className="py-1.5" style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#00ff88" }}>{row.delta}</td>
                  </tr>
                ))}
              </tbody>
            </table>
            <div className="mt-3 pt-3 border-t space-y-1.5" style={{ borderColor: "#071020" }}>
              {[
                { label: "AVX2 256-bit", available: false },
                { label: "SSE4.2 128-bit", available: true },
                { label: "AES-NI Crypto HW", available: true },
              ].map(item => (
                <div key={item.label} className="flex items-center justify-between">
                  <span style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#4a7a9b" }}>{item.label}</span>
                  <div className="flex items-center gap-2">
                    <div className="w-12 h-1 rounded-full overflow-hidden" style={{ background: "#0a1628" }}>
                      <div className="h-full rounded-full" style={{ width: item.available ? "100%" : "0%", background: item.available ? "#00ff88" : "#ff3366", boxShadow: item.available ? "0 0 6px #00ff88" : "none" }} />
                    </div>
                    <span style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.55rem", color: item.available ? "#00ff88" : "#ff3366" }}>
                      {item.available ? "OK" : "N/A"}
                    </span>
                  </div>
                </div>
              ))}
              <div className="mt-2 p-1.5 rounded-sm" style={{ background: "#040810", border: "1px solid #7c3aed22" }}>
                <div style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.55rem", color: "#7c3aed", lineHeight: 1.6 }}>
                  AVX2 estimé: +30-35% → 387–402K ticks/s ✓
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Footer */}
        <div className="mt-6 pt-4 border-t flex items-center justify-between" style={{ borderColor: "#00d4ff10" }}>
          <div style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.6rem", color: "#1a3a4a", letterSpacing: "0.1em" }}>
            LUMVORAX BLOCKCHAIN — PHASE 3 SIMD OPTIMIZER — SESSION: 10.04s — TOTAL TICKS: 3,920,180 — BATCHES: 3,919 — ERRORS: 0
          </div>
          <div className="flex items-center gap-4">
            {["✓ MONOTONIE TEMPORELLE", "✓ COHÉRENCE INTER-LOGS", "✓ GRANULARITÉ NS"].map(v => (
              <span key={v} style={{ fontFamily: "JetBrains Mono, monospace", fontSize: "0.55rem", color: "#00ff8860", letterSpacing: "0.1em" }}>{v}</span>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
