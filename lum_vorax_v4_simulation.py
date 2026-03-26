import numpy as np
import csv
import hashlib
from datetime import datetime

class KerrSpacetimeSimulatorV4:
    """
    Simulateur de Relativité Générale pour la métrique de Kerr (Trous Noirs en rotation).
    Version V4 - Audit Expert et Stabilité Extrémale.
    """
    def __init__(self, a=0.9999, M=1.0):
        self.a = a  # Paramètre de rotation (0 <= a < 1)
        self.M = M  # Masse du trou noir
        self.timestamp = datetime.now().isoformat()

    def get_metric(self, r, theta):
        """Calcule les composantes de la métrique de Kerr en coordonnées Boyer-Lindquist."""
        Sigma = r**2 + (self.a * np.cos(theta))**2
        Delta = r**2 - 2 * self.M * r + self.a**2
        
        # Composantes g_tt, g_t_phi, g_rr, g_theta_theta, g_phi_phi
        g_tt = -(1 - (2 * self.M * r) / Sigma)
        g_t_phi = -(2 * self.M * r * self.a * np.sin(theta)**2) / Sigma
        g_rr = Sigma / Delta
        g_theta_theta = Sigma
        g_phi_phi = (r**2 + self.a**2 + (2 * self.M * r * self.a**2 * np.sin(theta)**2) / Sigma) * np.sin(theta)**2
        
        return g_tt, g_t_phi, g_rr, g_theta_theta, g_phi_phi

    def run_scenario(self, scenario_name, initial_state):
        print(f"🚀 Exécution Scénario: {scenario_name}")
        r, theta, phi, t = initial_state
        results = []
        
        for step in range(100):
            g_tt, g_t_phi, g_rr, g_theta_theta, g_phi_phi = self.get_metric(r, theta)
            
            # Simulation simplifiée de l'évolution
            r -= 0.001 * (1.0 / (g_rr + 1e-15)) 
            
            # Capture de la preuve bit-exact (hexadécimal)
            r_hex = float(r).hex()
            
            log_entry = {
                "step": step,
                "r": r,
                "r_hex": r_hex,
                "g_tt": g_tt,
                "timestamp": datetime.now().isoformat()
            }
            results.append(log_entry)
            
        filename_csv = f"LOG_RUN_{scenario_name}_{self.timestamp.replace(':', '-')}.csv"
        
        # Écriture manuelle sans pandas pour éviter les dépendances manquantes
        keys = results[0].keys()
        with open(filename_csv, 'w', newline='') as output_file:
            dict_writer = csv.DictWriter(output_file, fieldnames=keys)
            dict_writer.writeheader()
            dict_writer.writerows(results)
        
        # Calcul du checksum
        with open(filename_csv, 'rb') as f:
            checksum = hashlib.sha256(f.read()).hexdigest()
            
        print(f"✅ Scénario {scenario_name} terminé. Checksum: {checksum}")
        return filename_csv, checksum

if __name__ == "__main__":
    # Test 1: EXTREMAL_STABILITY (a -> 1)
    sim_extremal = KerrSpacetimeSimulatorV4(a=0.99999)
    sim_extremal.run_scenario("EXTREMAL_STABILITY", (2.5, 1.57, 0, 0))
    
    # Test 2: COUNTER_ROTATION_DRAG
    sim_drag = KerrSpacetimeSimulatorV4(a=0.9)
    sim_drag.run_scenario("COUNTER_ROTATION_DRAG", (3.0, 1.57, 0.5, 0))
    
    # Test 3: PENROSE_CANDIDATE (Ergosphère)
    sim_penrose = KerrSpacetimeSimulatorV4(a=0.95)
    sim_penrose.run_scenario("PENROSE_CANDIDATE", (1.5, 1.57, 0, 0))
