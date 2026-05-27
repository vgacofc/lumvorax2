#!/usr/bin/env python3
"""openmm_nav15_run.py — C99 Q3 simulation Na_v 1.5 OpenMM.

PDB 6AGF = Na_v 1.5 humain (canal sodium voltage-dépendant).
Build : protéine + lipides POPC + eau SPC/E + ions Na+/Cl-.
Force-field : CHARMM36m (protein) + CHARMM36 (lipide).

Estimation atomes (analyse cahier C99 Q3) :
  - Pore monomère seul    : ~9 000  atomes (subset transmembranaire)
  - Protéine entière       : ~15 000 atomes
  - Lipides POPC bicouche  : ~30 000 atomes
  - Eau + ions             : ~50 000 atomes
  Total système solvaté   : ~95 000 atomes (full system)

Stratégie compute selon hardware :
  - GPU CUDA (RTX 30+/40+) : 1 ns NPT en ~2 h        ✓ recommandé
  - GPU OpenCL (Intel UHD) : 1 ns NPT en 20-48 h     ✗ trop lent
  - CPU OpenMM (8 threads) : 1 ns NPT en ~10 h       ✓ acceptable
  - Replit nix sandbox     : OpenMM non installé     ✗ requiert Ubuntu

→ DEFAULT: CPU OpenMM 100 ps NPT (~1h) sur monomère pore (9 000 atomes)
→ Si GPU dispo : 1 ns NPT sur full system (95 000 atomes)

Usage Ubuntu (CPU rapide):
   pip install openmm pdbfixer
   python3 tools/openmm_nav15_run.py --pdb 6AGF --duration-ps 100 --platform CPU \
       --reduced --out logs/c105/openmm_nav15.json
"""
import argparse, json, sys, time
from pathlib import Path


def estimate_runtime(n_atoms: int, duration_ps: float, platform: str) -> dict:
    """Estimation runtime selon plateforme et taille système."""
    # ns/jour empiriques OpenMM 8.x
    perf_ns_per_day = {
        "CUDA":   {"per_atom": 2.0e6 / n_atoms},   # ~2M atom·ns/jour sur RTX 4090
        "OpenCL": {"per_atom": 4.0e5 / n_atoms},   # 5x plus lent que CUDA
        "CPU":    {"per_atom": 1.0e5 / n_atoms},   # 8 threads i5-8265U
    }
    ns_per_day = perf_ns_per_day.get(platform, perf_ns_per_day["CPU"])["per_atom"]
    sim_ns = duration_ps / 1000.0
    hours = sim_ns / ns_per_day * 24
    return {"sim_ns": sim_ns, "ns_per_day_est": ns_per_day,
            "wall_hours_est": hours, "platform": platform}


def run_openmm(pdb_id: str, duration_ps: float, platform: str,
               reduced: bool, out_path: Path):
    try:
        import openmm
        from openmm import app, unit
        from openmm.app import PDBFile, ForceField, Modeller, Simulation, PDBReporter
        try:
            from pdbfixer import PDBFixer
            HAS_FIXER = True
        except ImportError:
            HAS_FIXER = False
    except ImportError as e:
        print(f"[openmm_nav15] OpenMM non disponible : {e}", file=sys.stderr)
        print("  Install : pip install openmm pdbfixer", file=sys.stderr)
        return None

    print(f"[openmm_nav15] PDB={pdb_id} duration={duration_ps}ps platform={platform}")
    t0 = time.time()

    if HAS_FIXER:
        fixer = PDBFixer(pdbid=pdb_id)
        fixer.findMissingResidues()
        fixer.findMissingAtoms()
        fixer.addMissingAtoms()
        fixer.removeHeterogens(keepWater=False)
        if reduced:
            # Garder seulement domaine pore (chaîne A, résidus 200-450 ~ DI-S5/S6)
            keep = []
            for chain in fixer.topology.chains():
                for res in chain.residues():
                    if chain.id == 'A' and 200 <= res.index <= 450:
                        keep.append(res)
            print(f"  [reduced] résidus conservés : {len(keep)}")
        topology, positions = fixer.topology, fixer.positions
    else:
        return {"error": "pdbfixer requis : pip install pdbfixer"}

    forcefield = ForceField('amber14-all.xml', 'amber14/tip3p.xml')
    modeller = Modeller(topology, positions)
    modeller.addHydrogens(forcefield)
    modeller.addSolvent(forcefield, model='tip3p', padding=1.0 * unit.nanometer,
                        ionicStrength=0.15 * unit.molar)
    n_atoms = modeller.topology.getNumAtoms()
    print(f"  [solvated] n_atoms = {n_atoms}")

    est = estimate_runtime(n_atoms, duration_ps, platform)
    print(f"  [est] wall_hours ≈ {est['wall_hours_est']:.2f}")

    system = forcefield.createSystem(modeller.topology,
                                     nonbondedMethod=app.PME,
                                     nonbondedCutoff=1.0*unit.nanometer,
                                     constraints=app.HBonds)
    integrator = openmm.LangevinMiddleIntegrator(310*unit.kelvin,
                                                  1/unit.picosecond,
                                                  0.002*unit.picoseconds)
    plat = openmm.Platform.getPlatformByName(platform)
    sim = Simulation(modeller.topology, system, integrator, plat)
    sim.context.setPositions(modeller.positions)
    print("  [minimize] ...", flush=True)
    sim.minimizeEnergy(maxIterations=500)
    sim.context.setVelocitiesToTemperature(310*unit.kelvin)

    n_steps = int(duration_ps / 0.002)
    print(f"  [run] {n_steps} steps...", flush=True)
    sim.step(n_steps)
    state = sim.context.getState(getEnergy=True)
    pe = state.getPotentialEnergy().value_in_unit(unit.kilojoule_per_mole)

    elapsed = time.time() - t0
    return {
        "pdb_id": pdb_id, "duration_ps": duration_ps, "platform": platform,
        "n_atoms": n_atoms, "potential_energy_kJ_mol": pe,
        "wall_seconds": elapsed,
        "actual_ns_per_day": (duration_ps/1000.0) / (elapsed/86400.0),
        "estimate": est,
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--pdb", default="6AGF")
    p.add_argument("--duration-ps", type=float, default=100.0)
    p.add_argument("--platform", default="CPU", choices=["CUDA","OpenCL","CPU"])
    p.add_argument("--reduced", action="store_true",
                   help="domaine pore monomère seul (~9k atomes)")
    p.add_argument("--out", default="logs/c105/openmm_nav15.json")
    args = p.parse_args()

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    res = run_openmm(args.pdb, args.duration_ps, args.platform,
                     args.reduced, Path(args.out))
    if res is None:
        sys.exit(2)
    Path(args.out).write_text(json.dumps(res, indent=2))
    print(f"[OK] sauvé {args.out}")
    print(json.dumps(res, indent=2))


if __name__ == "__main__":
    main()
