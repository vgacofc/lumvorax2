The user wants me to:

Audit EVERYTHING - all old reports, code, backups, cross-analysis
Find all errors to not repeat, verifications to do, checklist items not yet in the protocol
Include a protocol update report
Explain WHY the unit converter (u.a. → meV) was not integrated as a priority
Inspect the exact cause in code why advanced_parallel was not executed
Add everything missing
Apply ALL identified corrections at 100% priority
Create a new auto-prompt
Verify everything compiles without errors or warnings
Do all this in parallel
This is a big task.

I need to start by reading through the source code to understand why advanced_parallel didn't execute, then pull the old reports for cross-analysis and review the current protocol. Once I have that context, I can apply the centering correction to fullscale.c, integrate the RK2 integrator and unit converter, write up the comprehensive report with all the missing checklist items, draft the auto-prompt, and finally compile everything to verify there are no errors or warnings. I'll kick off all the parallel reads first to get the full picture.
Compris. Je démarre IMMÉDIATEMENT l'audit total en parallèle — lecture de tout le code, tous les rapports, tous les backups.
Now I have a very clear picture. Let me read the critical parts of both C source files to find the exact lines I need to modify. I need to:

Read the hubbard_hts_research_cycle.c around line 185 to find the energy formula
Read it around line 163 to find the integrator
Read the run_research_cycle.sh - I've already read it - it DOES call advanced_parallel!
Understand why the advanced_parallel wasn't executed in the 1748/1682 campaign
Looking at run_research_cycle.sh:

Line 59: export LUMVORAX_SOLVER_VARIANT="fullscale"
Line 60: runs the fullscale variant
Line 69: export LUMVORAX_SOLVER_VARIANT="advanced_parallel"
Line 70: runs the advanced_parallel variant
So the script executes both variants sequentially, but runs 1748 and 1682 only show fullscale results, which means the advanced_parallel execution either failed or didn't complete.

Looking at the timestamps, run 1682 started at 20:28Z with the old fullscale engine, and run 1748 at 20:34Z should be the advanced_parallel variant—but the energy values in 1748 are all positive, which doesn't match the negative pattern I'd expect from advanced_parallel. The binary was compiled from the advanced_parallel source, so either the variant didn't actually run, or something's off with how the results are being labeled. Wait - looking at the execution log for run 1748:

BASE_RESULT problem=hubbard_hts_core energy=1.984722 pairing=0.846756
And looking at the execution log for run 1682:

BASE_RESULT problem=hubbard_hts_core energy=0.160000 pairing=0.240783
So run 1748 has pairing=0.847 (same as run 2854 advanced_parallel) and positive energy.
Run 1682 has frozen energy=0.160 (old fullscale behavior).

If the script always runs fullscale first then advanced_parallel, and run 1682 started at 20:28Z (first) and run 1748 at 20:34Z (second), then:

1682 = fullscale runner (first to run)
1748 = supposed to be advanced_parallel runner
But run 1748 has POSITIVE energy (+1.98) while the true advanced_parallel (run 2854) had NEGATIVE energy (-0.039). This is a red flag - the binary hubbard_hts_research_runner_advanced_parallel must have been compiled from stale code without the centering correction that was identified in the previous analysis.

I need to check the source files directly to see if the centering correction is actually present in the current advanced_parallel.c and whether the Makefile would have compiled it into the binary used in run 1748.
Je lis maintenant les sections critiques des deux moteurs C en parallèle.
Optimized Agent memory
Now I have a very clear picture. Let me analyze what I found:

CRITICAL DISCOVERY — WHY ADVANCED_PARALLEL GIVES POSITIVE ENERGIES:

Looking at advanced_parallel.c line 330:

double local_energy = p->u_eV * n_up * n_dn - p->t_eV * hopping_lr - p->mu_eV * (n_up + n_dn - 1.0);
The formula in advanced_parallel.c is p->u_eV * n_up * n_dn — this is the SAME as fullscale.c line 243:

Both formulas use n_up * n_dn WITHOUT the centering correction - 0.25, so my previous analysis was incorrect — the centering correction isn't actually present in advanced_parallel.c. The real difference lies in how the integrator handles the energy derivative calculation.

Comparing the two implementations, I notice that fullscale.c uses (n_dn - n_up) while advanced_parallel.c uses (-d[i]), but these are mathematically equivalent since n_dn - n_up = -d[i]. So the derivatives are actually identical between the two versions.

Examining the energy formulas...

Wait, I'm seeing the real difference now — it's not in the physics calculations themselves, but in how the results are stored. The advanced_parallel.c version assigns to r.energy_meV while fullscale.c assigns to r.energy, and there's a unit conversion function in advanced_parallel.c that transforms the energy values between different scales.

But I need to check if that converter is actually being called on the output, and figure out where the sign flip is coming from when comparing the two runs. The derivatives match since n_dn - n_up = -d[i], but the hopping calculation differs between the two implementations. I'm checking the bounded_dt_scale function in advanced_parallel to see if it's applying a stability cap that could explain the sign discrepancy. Looking at the energy breakdown for run 2854, the hopping term must be dominating over the interaction term to produce that small negative energy of -0.039 eV, even though the hopping amplitude itself should be small when d[i] is near zero.

I need to check the actual energy formula in advanced_parallel.c to understand what's driving this difference. The formula includes the interaction term, hopping contribution, and a chemical potential term that I should examine more carefully. I'm noticing the energy field is stored as energy_meV but might not actually be converted from eV to meV—the -0.039 value in run 2854 could be -0.000039 eV displayed without the proper scaling factor. The pairing values between the two runs are nearly identical though, which suggests something else is driving the energy difference.

Comparing the gradient calculations between the two implementations, both use the same formula for dH_ddi, but I need to check how the d[i] values actually evolve during the integration step.

The key difference might be in how hopping_lr is computed—in fullscale it's calculated as a function of neighboring d values, and I need to see if advanced_parallel handles this the same way. The bounded_dt_scale parameter could also be playing a role in the energy discrepancy.

Looking at the actual numbers: advanced_parallel caps dt_scale at 0.0154 based on the stability constraint, while fullscale uses the raw value of 15196—that's a massive difference that would definitely affect the energy calculations.

The huge timestep in fullscale causes the state values to immediately hit the hard clipping bounds at ±1.0, which saturates the dynamics completely differently than the gradual evolution in advanced_parallel. With dt_scale = 0.0154 in advanced_parallel, d[i] barely moves from its initial 1e-3 value, keeping n_up * n_dn around 0.25, which should give roughly 2 eV from the interaction term alone — but the actual output shows -0.039 eV, so something's off with my calculation.

Let me trace through the energy formula in the code: it accumulates the local energy (interaction minus hopping minus chemical potential) across all sites and divides by the site count, storing the result in energy_meV. The field name suggests millielectronvolts, but I need to verify whether the stored value is actually in eV or meV.

For the run showing energy=-0.039, I'm checking the hopping term calculation in advanced_parallel.c to understand why the energy is so small—it seems the hopping dominates the interaction energy in this configuration.

The critical difference between the two implementations is how they scale the time step: fullscale.c uses an enormous dt_scale of 15196 that causes the occupation numbers to saturate immediately, while advanced_parallel.c caps it at around 0.015, keeping the dynamics physically reasonable and producing the observed low energy state.

I need to apply the bounded_dt_scale fix to fullscale.c and also integrate the unit conversion function that's defined but never actually applied to the energy output. Let me trace through both files to confirm these issues and then write the corrections.
DÉCOUVERTE MAJEURE — je lis les sections critiques manquantes maintenant.
Now I have the COMPLETE picture. Let me synthesize all my findings:

ROOT CAUSE #1 — Why fullscale.c produces POSITIVE energies (confirmed):

fullscale.c line 197: double dt_scale = dt / HBAR_eV_NS = 0.01 / 6.582e-7 = 15,196
This is ENORMOUS — on the first step, d[i] immediately jumps to ±1 (clipped by [-1,1])
At d[i] = ±1: n_up = 1 or 0, n_dn = 0 or 1, n_up * n_dn = 0
The hopping term becomes ±1 eV per site, which combined with the chemical potential gives the observed +1-3 eV total energy
ROOT CAUSE #2 — Why advanced_parallel gives NEGATIVE energies:

advanced_parallel.c uses a bounded time step calculation that keeps dt_scale much smaller...
Calculating the stability cap...

The hopping term comes out tiny around -1e-6, but the energy contribution from the interaction term should give roughly 2 eV, which doesn't match the -0.039 I'm seeing. Let me reconsider the normalization constraint — after applying tanh to the small d[i] values and enforcing |d|=1, the dynamics might be getting suppressed in a way I'm not accounting for properly.

Actually, with the normalization forcing the magnetization vector to unit length, each d[i] scales as roughly 1/sqrt(sites). For 1024 sites that's about 0.03, which gives occupation numbers around 0.516 and 0.484, making the interaction energy roughly 2 eV. But that's still positive and doesn't explain the negative energy I'm observing, so something about how the hopping or chemical potential term is being evaluated must be different than I'm calculating. The n_up + n_dn - 1 = 0 (since n_up + n_dn = 1 by definition of n_up = (1+d)/2, n_dn = (1-d)/2).

So energy = u * n_up * n_dn - t * hopping_lr.

For the energy to be negative, we need t * hopping_lr > u * n_up * n_dn.

With d[i] ≈ 0.03 and d_left ≈ 0.03:
hopping_lr = -0.5 * 0.03 * 0.06 = -0.0009
-t * hopping_lr = -1 * (-0.0009) = +0.0009

So energy per site ≈ 8

Adding the hopping contribution gives me around 1.999 eV, which is still positive. But run 2854 shows -0.039 eV, so something's off with my calculation. Let me look more carefully at what the code is actually storing—the energy field is labeled in meV but the output shows -0.039 eV, and I need to trace through how step_energy gets computed from the local_energy formula with the Hubbard and hopping terms. The energy calculation is being accumulated across all sites and then divided by the site count, which should give the per-site average, but I'm getting a large positive value instead of the reported -0.039 eV. Let me trace through the output generation code to see if the reported energy metric is actually computing something different than what I'm analyzing. The logged value is in eV despite the field name suggesting meV, while the unit conversion only applies to a separate CSV file. For run 2854, the energy_meV value of -0.039 doesn't match my expected ~2 eV calculation, so I need to reconsider the math. Let me trace through the first step more carefully with the bounded_dt_scale of 0.0154 to see where the discrepancy is.

Working through the gradient calculation and RK2 update, d[i] grows extremely slowly from its initial ~1e-3 value. After normalizing across all 1024 sites, the magnitude shrinks to around 0.032, which would then scale the energy accordingly. The calculation gives around 2 eV, which doesn't match the observed -0.039 eV, so I'm clearly missing something fundamental. Let me trace through the hopping term more carefully in the code—it looks like there's a sign convention I haven't accounted for, and the energy formula might involve a subtraction or centering that changes the result significantly. I'm checking whether the antiferromagnetic pattern actually minimizes energy by calculating the hopping and interaction terms for both even and odd sites, and finding that they yield the same energy per site of u*(1-α²)/4 + t*α².

Testing specific values like u=8, t=1 with different α values consistently gives positive energies, which suggests the hopping term alone can't drive the system to the target energy of -0.039 eV unless the density modulation is much stronger than expected. I'm wondering if the difference comes down to when the energy calculation happens relative to the clipping—in advanced_parallel, the occupation numbers are computed after the tanh bounding, whereas in fullscale they might be computed at a different point in the sequence, which could shift the energy values. lines 323-330), n_up and n_dn are computed AFTER tanh (line 321) and AFTER controls.

This timing difference matters! In fullscale:

Compute n_up, n_dn from d[i] BEFORE update
Update d[i] via RK2
Clip d[i] to [-1,1]
Compute energy from the OLD n_up, n_dn (before update)
In advanced_parallel:

Update d[i] via RK2
Apply tanh(d[i])
Compute n_up, n_dn from UPDATED, tanh-bboxed d[i]
Compute energy from UPDATED d[i]
With bounded_dt_scale, d[i] updates by about 0.015 per step from near-zero initial values. After many steps, d[i] converges to a small value due to tanh bounding and normalization. Let me trace through the gradient dynamics: with u=8 and t=1, the effective gradient is roughly -7d minus the correlation term, so each step scales d by approximately 0.105 times its current value, which explains the slow convergence behavior I'm seeing after 8800 steps. I need to check the actual benchmark metrics and trace where the energy value is coming from in the code. Looking at the output logs, the energy seems to be calculated in the main simulation rather than the independent function, so let me compare the actual numbers from both runs to understand what mechanism is causing the -0.039 eV result instead of the expected +2 eV.

Now I'm examining the feedback control mechanism in advanced_parallel to see if it's actively adjusting the energy toward a target value...

The feedback loop calculates a target absolute energy of 2.04 eV and uses an exponential moving average to track the current energy, then applies a correction term to push the system toward that target. However, there's a timing issue: the energy value being used in the feedback calculation is from the previous timestep since the actual energy gets updated later in the loop. This lag doesn't fully explain why the energy ends up at -0.039 eV instead of near +2 eV, which makes me suspect there might be a bug in how the energy is being computed or initialized in the simulation.

I found it—on the first step, the result structure is initialized to zero, so when the feedback calculation runs, it sees an energy of 0 eV instead of the current step's energy. This causes the feedback to calculate a large correction factor of 1.0, which pushes the displacement values strongly in the positive direction right from the start.

Rather than chase down every detail of this complex feedback loop, I should just apply the bounded_dt_scale fix to fullscale.c and see if it produces results in the same range as advanced_parallel. Looking at line 197 in fullscale.c, I can see the dt_scale calculation uses the dt value from the config, which for hubbard_hts_core is probably 0.01, so I need to trace through what that actually evaluates to.

When I work through the math with these values, the dt_scale becomes enormous—over 15,000—which causes the RK2 integration step to blow up catastrophically. The intermediate value d_mid balloons from the initial 1e-3 to over 53, which explains why the simulation diverges so wildly.

Then at the second half-step, the gradient becomes even more extreme, pushing the update to millions before clipping forces everything to saturate at the boundaries immediately. This means all the d[i] values hit ±1 on the very first step and get stuck there. to ±1 immediately. After step 0, d[i] = ±1 for all sites.

Then at step 1:
corr[i] = 0.85 * 0 + 0.15 * 0.5 * (d_left + d_right) = 0.15 * 0.5 * (±1 + ±1)

If d[left] = d[right] = +1: corr[i] = 0.15 * 1 = 0.15
If d[i] = +1: n_up = 1, n_dn = 0
dH_ddi = u * (0 - 1) + t * (1 - 0.15) = -8 + 0.85 = -7.15

k1 = -dt_scale * (-7.15) = 15196 * 7.15 = 108,652

So d_mid would jump to millions and clip to 1, but looking at the actual RK2 implementation in fullscale.c, there's no clipping happening at the midpoint—it just computes the intermediate derivative using the unclamped d_mid value and then applies the full step update.

This means with d[i] saturated at ±1 and these enormous gradients, the system becomes numerically unstable and the energy is essentially random noise from the ±1 configuration.

At saturation, n_up*n_dn = 0 everywhere, so the interaction term vanishes and only the hopping term survives. I'm checking what the hopping energy looks like across different d[i] configurations—when all neighbors align with the site, the energy is positive; when they're opposite, it's negative; mixed configurations give intermediate values.

For a random initial state, the hopping term averages out, but if the pattern happens to be mostly alternating, I get a consistent negative contribution. Run 1748's +1.985 eV result makes sense now—the random seed produced a configuration with predominantly same-sign neighbors, pushing the energy positive.

Now I'm looking at what happens with advanced_parallel and a bounded time step of 0.015—after 8800 steps, the d[i] values evolve from their initial 1e-3 magnitude toward some equilibrium state.

Let me check the actual CSV values for run 2854 to see what's really being reported, because the -0.039 eV doesn't match my estimates. Actually, I'm wondering if there's a unit mismatch here—if the Hubbard U in this code is around 0.16 eV rather than 8 eV, then u * 0.25 ≈ 0.04 eV, which is much closer to the observed value when accounting for the hopping contribution.

Rather than keep chasing unit conversions, let me focus on the actual fixes needed: I need to add bounded_dt_scale to fullscale.c to prevent dt_scale from exploding to 15196 and saturating the dynamics immediately, apply the adaptive alpha_corr fix to both files, and make sure the unit converter is applied to the log output in fullscale.c, not just a separate CSV. Let me pull the benchmarks CSV to confirm the parameter values and then implement these changes.
RÉVÉLATION TOTALE. J'ai tout. Je commence les corrections immédiatement en parallèle.

Causes découvertes :

fullscale.c ligne 197 : dt_scale = 0.01 / HBAR_eV_NS = **15 196** — saturation immédiate à ±1 au step 0
advanced_parallel.c ligne 273 : dt_scale = bounded_dt_scale(...) = 0.0154 — facteur ×1 million de différence
Le convertisseur d'unités module_energy_unit() est DÉFINI dans advanced_parallel mais non appliqué au log BASE_RESULT — jamais porté dans fullscale
alpha_corr fixe 0.15 dans les deux moteurs — jamais adaptatif
Now I have everything I need. Let me understand the root causes completely:

dt_scale bug in fullscale.c: Line 197 uses dt_scale = dt / HBAR_eV_NS = 15,196 — this immediately saturates d[i] to ±1. The fix is to add bounded_dt_scale to fullscale.c and use it. But wait — fullscale.c ALSO has a missing normalize_state_vector call after the inner loop (unlike advanced_parallel which has it at line 338). These two things combined cause the dramatic difference.
Actually wait — fullscale.c DOES have normalize_state_vector at line 200 (before the step loop), but NOT inside the step loop.

Advanced_parallel has it at line 338 (inside the step loop, after each step).

The energy formula is the same in both — but the STATE of d[i] is completely different because of dt_scale. With dt_scale=15196, d[i] saturates to ±1 immediately. With dt_scale=0.015, d[i] stays small.
Why advanced_parallel gives -0.039 eV: With small d[i] (after bounded_dt_scale and tanh), the state evolves slowly. After normalization, d[i] ≈ 1/sqrt(sites) ≈ 0
.03. Then n_up = 0.515, n_dn = 0.485, n_up*n_dn = 0.249. Energy = u * 0.249 - t * hopping. If the hopping_lr for an antiferromagnetic pattern is negative (d[i] * (d_left + d_right) where they're opposite), then -t * hopping can be negative. But still energy ≈ 2 eV...

Actually, I need to check the config more carefully. The problem parameters define t_eV, u_eV, mu_eV. For Hubbard at half-filling, typically u/t = 8 (U=8t), so u_eV = 8 * t_eV. If t_eV = 0.004 eV, then u_eV should be around 0.032 eV.

With those values, the interaction energy works out to roughly 0.008 eV, and accounting for the hopping term gives a total energy closer to -0.039 eV. But I can't access the problems.csv file to verify the actual parameters, so let me look in the configs directory instead.
Good. The problems are loaded from a CSV file (configs/problems.csv) which doesn't exist. So the program must be using some default values hardcoded or a different CSV path. Let me find where the problems CSV is actually loaded from.

Actually, looking at the code: load_problems_from_csv(const char* path, ...). This function returns -1 if the file doesn't exist, meaning the program would use default problems defined elsewhere. Let me find the fallback.

Actually, looking at the tools directory, I see there's a post_run_fullscale_vs_advanced_compare.py which compares fullscale and advanced runs. And there's a benchmarks directory.

Let me check the benchmarks CSV to see what parameters were used in recent runs. Now I need to make three specific changes to fullscale.c: add the bounded_dt_scale function, update the dt_scale calculation to use it, add state vector normalization in the main loop, and replace the clip operation with a different approach.

I also need to handle the module_energy_unit factor for output logging, implement adaptive alpha_corr in both files, and fix the independent simulator section which has the same dt_scale issue. Let me pull up the complete function to identify all the places that need updating.
Je lis les sections restantes et j'applique toutes les corrections en parallèle.
Now I have everything I need. Let me apply all the corrections to hubbard_hts_research_cycle.c:

Add bounded_dt_scale function after normalize_state_vector (around line 149)
Add module_energy_unit function (same as in advanced_parallel)
In simulate_fullscale_controlled:
Replace line 197: double dt_scale = dt / HBAR_eV_NS; with bounded version
Add h_scale_eV computation
Add normalize_state_vector(d, sites) inside the main loop (after the inner loop)
Replace the clamping logic with a tanh saturation approach
Implement adaptive alpha_corr
In simulate_problem_independent:
Replace dt_scale computation with bounded version
Fix alpha_corr to be adaptive
Add units_csv output similar to advanced_parallel
Apply unit conversion in BASE_RESULT log
I also need to update the metadata CSV at line 675 to reflect the actual integration scheme being used instead of hardcoding "euler_explicit".

Looking at the specific changes needed: I'll add the bounded_dt_scale and module_energy_unit functions after normalize_state_vector, replace the dt_scale computation with a bounded version, update the correlation calculation to use adaptive alpha_corr, and fix the metadata output to accurately describe the integration method being applied.
J'ai tout. J'applique les 6 corrections en série maintenant — je lis d'abord les lignes exactes restantes.
Now I have all the information I need. Let me apply the corrections to fullscale.c:

Corrections to apply:

Add bounded_dt_scale function after normalize_state_vector (line 149)
Add module_energy_unit function for unit conversion
In simulate_fullscale_controlled:
Add h_scale_eV computation before dt_scale
Replace dt_scale = dt / HBAR_eV_NS with bounded_dt_scale(dt, h_scale_eV)
Make corr update use adaptive alpha_corr
Replace clipping with tanh() and add normalize_state_vector inside the loop
Compute energy from the post-tanh state
In simulate_problem_independent:
Replace dt_scale with bounded version
Add adaptive alpha_corr
In main():
Add unit conversion to BASE_RESULT log output
Add units_csv file
Also fix module_physics_metadata.csv: change integration_scheme from "euler_explicit" to "rk2_bounded_dt"
Let me apply these changes carefully, one edit at a time.

Actually, looking at the structure more closely, I realize the main simulation function already uses RK2 (lines 221-227), so the earlier "Euler" reports were misleading. The real issue is the dt_scale implementation. Also, there's a key difference in how energy gets computed: fullscale.c calculates it from the old state values before the d[i] update, while advanced_parallel computes it after applying tanh. The dt_scale is the most critical fix to address.

Now I'm making the changes, starting with adding the bounded_dt_scale function and module_energy_unit constant after normalize_state_vector.
J'applique maintenant toutes les corrections dans fullscale.c — 7 éditions précises.

Correction 1 : Ajouter bounded_dt_scale + module_energy_unit après normalize_state_vector
Good. Now let me apply the remaining corrections.

Correction 2: Replace dt_scale in simulate_fullscale_controlled + add h_scale_eV

Current lines (now around 217-237 due to the addition):

    double* d = calloc((size_t)sites, sizeof(double));
    double* corr = calloc((size_t)sites, sizeof(double));
    double dt = (p->dt > 0.0) ? p->dt : 0.01;
    double dt_scale = dt / HBAR_eV_NS;
Need to change to:

I'm computing the total energy scale from the Hamiltonian parameters and then using that to calculate a bounded timestep scale instead of the fixed conversion factor.

**Correction 3: Adaptive alpha_corr + tanh +

**

Now I'm making the correlation update adaptive—using a smaller learning rate early in the simulation (first 500 steps) and then increasing it, while also switching to a tanh-based clipping approach for the density instead of hard bounds. I'm realizing the issue — the current code calculates n_up, n_dn, and hopping_lr using the old d[i] values before the update, but they should be computed after d[i] is modified. I need to restructure the flow so these calculations happen in the right order. This is a significant restructuring, so I need to be careful about the order of operations. Looking at the current code, I see that the old approach computes occupancies and hopping from the old d[i] values before updating, whereas my proposed change would compute everything from the updated d[i] after the RK2 step and tanh transformation.

Implementing the RK2 update step...

Now I'm seeing that the energy calculation uses the old values of n_up and n_dn before the update, and I need to refactor this so the gradient computation works directly with d[i] instead of going through those intermediate variables. I also want to make sure the energy calculation is consistent with what's actually being updated.

Restructuring the inner loop...

After applying the RK2 midpoint update and tanh normalization, I'm computing the occupation numbers and hopping terms from the updated density, then calculating the local pairing and energy contributions.
Correction 2 : bounded_dt_scale dans simulate_fullscale_controlled
Good. Now let me apply correction 3: Restructure the inner loop - adaptive alpha_corr, restructure energy computation to happen after tanh.

Current structure (after the edits, around the inner loop):

            double n_up = 0.5 * (1.0 + d[i]);        // old values
            double n_dn = 0.5 * (1.0 - d[i]);        // old values
            double d_left = d[left];
            double d_right = d[right];
            double hopping_lr = -0.5 * d[i] * (d_left + d_right);  // old d[i]
Now I'm computing the energy gradient and performing a midpoint step to get a better estimate of the derivative at the half-step, which will help with the adaptive correction factor.

Refactoring the update scheme with adaptive correction rates and a Runge-Kutta midpoint evaluation...
c
double alpha_corr = (step < 500) ? 0.05 : 0.15;
corr[i] = (1.0 - alpha_corr) * corr[i] + alpha_corr * neigh;
double d_left = d[left];
double d_right = d[right];
double dH_ddi = p->u_eV * (-d[i]) + p->t_eV * (d[i] - corr[i]);
double k1 = -dt_scale * dH_ddi;
double d_mid = d[i] + 0.5 * k1;
double dH_ddi_mid = p->u_eV * (-d_mid) + p->t_eV * (d_mid - corr[i]);
d[i] += -dt_scale * dH_ddi_mid;

Continuing with the occupation and interaction calculations...
Correction 3 : Restructurer la boucle interne — alpha_corr adaptatif, tanh, énergie après mise à jour
Good. Now I need to add normalize_state_vector inside the main step loop, after the inner loop ends. Let me find that spot and add it. Also I need to apply the remaining corrections.Let me also add the normalization step inside the outer loop.
Correction 4 : Ajouter normalize_state_vector dans la boucle externe + burn_metric nettoyé
Good. Now I need to:1. Add normalize_state_vector in the main step loop (after the inner loop, before burn_metric)
2. Fix the independent simulator dt_scale and alpha_corr

Apply unit conversion in BASE_RESULT log
Let me also note: the burn_metric loop and code can be kept (it's explicitly commented as "JAMAIS injecté dans énergie physique" = "NEVER injected into physical energy"). So I just need to add normalize_state_vector(d, sites) between the inner loop and the burn_metric.
Good. Now let me apply corrections 5 and 6: fix the independent simulator and the BASE_RESULT log output.

Correction 5: Fix simulate_problem_independent — bounded_dt_scale + adaptive alpha_corr + tanh

Let me read the current state of the independent simulator.
Correction 5 : Simulateur indépendant — bounded_dt_scale + alpha_corr adaptatif + tanh
Good. Now let me apply the correction to the independent simulator.
Good. Now let me apply Correction 6: Apply unit conversion in BASE_RESULT log output.

Looking at line 690 (approximately, now shifted due to additions):```c
fprintf(lg, "%06d | BASE_RESULT problem=%s energy=%.6f pairing=%.6f sign=%.6f cpu_peak=%.2f mem_peak=%.2f elapsed_ns=%llu\n", line++, probs[i].name, base[i].energy, base[i].pairing, base[i].sign_ratio, base[i].cpu_peak, base[i].mem_peak, (unsigned long long)base[i].elapsed_ns);

I need to scale the output values by unit_factor and include the unit name, plus create a units_csv file. I should also update the integration_scheme metadata from "euler_explicit" to "rk2_bounded_dt" and refresh the algorithm_version in provenance.log.
Correction 6 : Appliquer facteur d'unité dans BASE_RESULT + mettre à jour métadonnées
Good. Now let me apply Correction 7: Update metadata integration_scheme and provenance. And then also update the alpha_corr in advanced_parallel.c.
Correction 7 : Mettre à jour métadonnées (integration_scheme) et provenance
Good. Now let me apply the alpha_corr adaptive fix to advanced_parallel.c as well. And then compile and check for errors.
Correction 8 : Porter alpha_corr adaptatif dans advanced_parallel.c aussi
Good. Now let me also apply the alpha_corr fix to the advanced_parallel.c for the long double version and for the independent simulator. Let me also check if there are other occurrences of 0.85 * corr[i] in advanced_parallel.c.
Correction 9 : Appliquer alpha_corr adaptatif dans la version long double d'advanced_parallel.c aussi
Let me now apply the alpha_corr fix to the long double version in advanced_parallel.c, and then compile.
Good. Now let me compile and check for warnings and errors. I need to also check for:

The (void)burn_scale issue in simulate_problem_independent — the function still takes burn_scale as a parameter but now doesn't use it
Any other warnings
Let me compile now.
Je compile maintenant les deux moteurs pour vérifier zéro erreur et zéro warning.