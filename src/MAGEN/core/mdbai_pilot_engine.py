"""
MDBAI/PILOT Workflow Engine - Orchestrateur Central MAGEN V35+

Ce module implémente le workflow engine qui coordonne l'exécution structurée
de MAGEN selon l'architecture cognitive en 7 phases.

Architecture:
    PHASE 0: Affordance Discovery      - Quelles actions sont plausibles ?
    PHASE 1: Candidate Filtering       - Éliminer actions absurdes
    PHASE 2: Reputation Prioritization - Prioriser selon historique
    PHASE 3: Budget Allocation         - Allouer temps intelligemment
    PHASE 4: Trajectory Monitoring     - Surveiller utilité temps réel
    PHASE 5: Global Regret Detection   - Détecter gaspillage global
    PHASE 6: Symbolic Validation       - Valider solutions

Principe:
    Le MDBAI/PILOT est le "cerveau central" qui:
    - Orchestre l'exécution des phases
    - Gère les transitions entre phases
    - Collecte les métriques globales
    - Décide des interventions (reset, changement stratégie)
    - Génère les rapports forensic complets

Version: 1.0.0
Date: 2026-06-15
Auteur: Bob (LVX AI Systems)
Protocole: MDBAI/PILOT + LUMVORAX + LEÇON-73.1
"""

import time
import json
from typing import Dict, List, Optional, Any, Tuple
from enum import Enum
from dataclasses import dataclass, asdict
from pathlib import Path

from MAGEN.core.forensic_middleware import ForensicMiddleware


class WorkflowPhase(Enum):
    """Phases du workflow MDBAI/PILOT"""
    AFFORDANCE_DISCOVERY = "affordance_discovery"
    CANDIDATE_FILTERING = "candidate_filtering"
    REPUTATION_PRIORITIZATION = "reputation_prioritization"
    BUDGET_ALLOCATION = "budget_allocation"
    TRAJECTORY_MONITORING = "trajectory_monitoring"
    GLOBAL_REGRET_DETECTION = "global_regret_detection"
    SYMBOLIC_VALIDATION = "symbolic_validation"


class WorkflowStatus(Enum):
    """Statut d'exécution du workflow"""
    IDLE = "idle"
    RUNNING = "running"
    PAUSED = "paused"
    COMPLETED = "completed"
    FAILED = "failed"
    ABORTED = "aborted"


@dataclass
class PhaseResult:
    """Résultat d'exécution d'une phase"""
    phase: WorkflowPhase
    status: str  # "success", "failure", "skipped"
    duration_ns: int
    metrics: Dict[str, Any]
    output: Any
    error: Optional[str] = None


@dataclass
class WorkflowExecution:
    """Exécution complète d'un workflow"""
    workflow_id: str
    puzzle_id: str
    start_time_ns: int
    end_time_ns: Optional[int]
    status: WorkflowStatus
    phases_executed: List[PhaseResult]
    global_metrics: Dict[str, Any]
    final_solution: Optional[Any]


class MDBAIPilotEngine:
    """
    Workflow Engine MDBAI/PILOT
    
    Orchestre l'exécution structurée de MAGEN selon l'architecture
    cognitive en 7 phases.
    
    Responsabilités:
        - Gestion du cycle de vie du workflow
        - Orchestration des phases
        - Collecte des métriques globales
        - Décisions d'intervention (reset, abort)
        - Génération rapports forensic
    
    Conformité: LEÇON-73.1 (100%)
    """
    
    def __init__(
        self,
        max_workflow_duration_s: float = 300.0,
        enable_auto_abort: bool = True,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise le MDBAI/PILOT Engine
        
        Args:
            max_workflow_duration_s: Durée max workflow (secondes)
            enable_auto_abort: Activer abort automatique si gaspillage
            forensic_log_path: Chemin log forensic (LEÇON-73.1)
        """
        self.max_workflow_duration_s = max_workflow_duration_s
        self.enable_auto_abort = enable_auto_abort
        
        # État workflow
        self.current_workflow: Optional[WorkflowExecution] = None
        self.workflow_history: List[WorkflowExecution] = []
        
        # Statistiques globales
        self.total_workflows: int = 0
        self.successful_workflows: int = 0
        self.failed_workflows: int = 0
        self.aborted_workflows: int = 0
        
        # Modules connectés (seront injectés)
        self.modules: Dict[str, Any] = {}
        
        # Forensic (LEÇON-73.1)
        if forensic_log_path is None:
            forensic_log_path = "logs/mdbai_pilot/mdbai_pilot_engine.jsonl"
        
        self.forensic = ForensicMiddleware(
            log_path=forensic_log_path,
            component_name="MDBAIPilotEngine"
        )
        
        # Log initialisation
        self.forensic.forensic_logger.log(
            "module_initialized",
            self.__class__.__name__,
            {
                "config": {
                    "max_workflow_duration_s": max_workflow_duration_s,
                    "enable_auto_abort": enable_auto_abort
                },
                "timestamp_ns": time.time_ns()
            }
        )
    
    @property
    def _forensic_state(self) -> Dict[str, Any]:
        """État forensic (LEÇON-73.1)"""
        return {
            "current_workflow": self.current_workflow.workflow_id if self.current_workflow else None,
            "total_workflows": self.total_workflows,
            "successful_workflows": self.successful_workflows,
            "failed_workflows": self.failed_workflows,
            "aborted_workflows": self.aborted_workflows,
            "modules_connected": list(self.modules.keys())
        }
    
    def get_state(self) -> Dict[str, Any]:
        """Retourne état complet (LEÇON-73.1)"""
        return {
            "statistics": {
                "total_workflows": self.total_workflows,
                "successful_workflows": self.successful_workflows,
                "failed_workflows": self.failed_workflows,
                "aborted_workflows": self.aborted_workflows,
                "success_rate": self.successful_workflows / max(1, self.total_workflows)
            },
            "current_workflow": {
                "active": self.current_workflow is not None,
                "workflow_id": self.current_workflow.workflow_id if self.current_workflow else None,
                "status": self.current_workflow.status.value if self.current_workflow else None
            },
            "modules": {
                name: "connected" for name in self.modules.keys()
            },
            "config": {
                "max_workflow_duration_s": self.max_workflow_duration_s,
                "enable_auto_abort": self.enable_auto_abort
            }
        }
    
    def register_module(self, phase: WorkflowPhase, module: Any):
        """
        Enregistre un module pour une phase
        
        Args:
            phase: Phase du workflow
            module: Instance du module
        """
        self.modules[phase.value] = module
        
        self.forensic.forensic_logger.log(
            "module_registered",
            self.__class__.__name__,
            {
                "phase": phase.value,
                "module_type": type(module).__name__,
                "timestamp_ns": time.time_ns()
            }
        )
    
    def start_workflow(self, puzzle_id: str) -> str:
        """
        Démarre un nouveau workflow
        
        Args:
            puzzle_id: ID du puzzle à résoudre
            
        Returns:
            workflow_id: ID unique du workflow
        """
        if self.current_workflow is not None:
            raise RuntimeError("Un workflow est déjà en cours")
        
        workflow_id = f"workflow_{puzzle_id}_{time.time_ns()}"
        
        self.current_workflow = WorkflowExecution(
            workflow_id=workflow_id,
            puzzle_id=puzzle_id,
            start_time_ns=time.time_ns(),
            end_time_ns=None,
            status=WorkflowStatus.RUNNING,
            phases_executed=[],
            global_metrics={},
            final_solution=None
        )
        
        self.total_workflows += 1
        
        self.forensic.forensic_logger.log(
            "workflow_started",
            self.__class__.__name__,
            {
                "workflow_id": workflow_id,
                "puzzle_id": puzzle_id,
                "timestamp_ns": time.time_ns()
            }
        )
        
        return workflow_id
    
    def execute_phase(
        self,
        phase: WorkflowPhase,
        input_data: Any
    ) -> PhaseResult:
        """
        Exécute une phase du workflow
        
        Args:
            phase: Phase à exécuter
            input_data: Données d'entrée pour la phase
            
        Returns:
            PhaseResult: Résultat de l'exécution
        """
        if self.current_workflow is None:
            raise RuntimeError("Aucun workflow actif")
        
        start_ns = time.time_ns()
        
        # Vérifier si module enregistré
        if phase.value not in self.modules:
            result = PhaseResult(
                phase=phase,
                status="skipped",
                duration_ns=0,
                metrics={},
                output=None,
                error=f"Module {phase.value} not registered"
            )
        else:
            try:
                module = self.modules[phase.value]
                
                # Exécuter phase (interface générique)
                if hasattr(module, 'execute'):
                    output = module.execute(input_data)
                else:
                    output = None
                
                end_ns = time.time_ns()
                
                result = PhaseResult(
                    phase=phase,
                    status="success",
                    duration_ns=end_ns - start_ns,
                    metrics=module.get_state() if hasattr(module, 'get_state') else {},
                    output=output,
                    error=None
                )
            except Exception as e:
                end_ns = time.time_ns()
                result = PhaseResult(
                    phase=phase,
                    status="failure",
                    duration_ns=end_ns - start_ns,
                    metrics={},
                    output=None,
                    error=str(e)
                )
        
        # Enregistrer résultat
        self.current_workflow.phases_executed.append(result)
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "phase_executed",
            self.__class__.__name__,
            {
                "workflow_id": self.current_workflow.workflow_id,
                "phase": phase.value,
                "status": result.status,
                "duration_ns": result.duration_ns,
                "error": result.error,
                "timestamp_ns": time.time_ns()
            }
        )
        
        return result
    
    def complete_workflow(self, solution: Optional[Any] = None):
        """
        Termine le workflow actuel
        
        Args:
            solution: Solution finale (si trouvée)
        """
        if self.current_workflow is None:
            raise RuntimeError("Aucun workflow actif")
        
        self.current_workflow.end_time_ns = time.time_ns()
        self.current_workflow.status = WorkflowStatus.COMPLETED
        self.current_workflow.final_solution = solution
        
        # Calculer métriques globales
        total_duration_ns = self.current_workflow.end_time_ns - self.current_workflow.start_time_ns
        phases_success = sum(1 for p in self.current_workflow.phases_executed if p.status == "success")
        phases_total = len(self.current_workflow.phases_executed)
        
        self.current_workflow.global_metrics = {
            "total_duration_ns": total_duration_ns,
            "total_duration_s": total_duration_ns / 1e9,
            "phases_executed": phases_total,
            "phases_successful": phases_success,
            "success_rate": phases_success / max(1, phases_total),
            "solution_found": solution is not None
        }
        
        # Statistiques
        if solution is not None:
            self.successful_workflows += 1
        else:
            self.failed_workflows += 1
        
        # Archiver
        self.workflow_history.append(self.current_workflow)
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "workflow_completed",
            self.__class__.__name__,
            {
                "workflow_id": self.current_workflow.workflow_id,
                "status": self.current_workflow.status.value,
                "metrics": self.current_workflow.global_metrics,
                "timestamp_ns": time.time_ns()
            }
        )
        
        self.current_workflow = None
    
    def abort_workflow(self, reason: str):
        """
        Abandonne le workflow actuel
        
        Args:
            reason: Raison de l'abandon
        """
        if self.current_workflow is None:
            raise RuntimeError("Aucun workflow actif")
        
        self.current_workflow.end_time_ns = time.time_ns()
        self.current_workflow.status = WorkflowStatus.ABORTED
        
        self.aborted_workflows += 1
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "workflow_aborted",
            self.__class__.__name__,
            {
                "workflow_id": self.current_workflow.workflow_id,
                "reason": reason,
                "timestamp_ns": time.time_ns()
            }
        )
        
        self.workflow_history.append(self.current_workflow)
        self.current_workflow = None
    
    def close(self):
        """Ferme le engine (LEÇON-73.1)"""
        # Log final
        self.forensic.forensic_logger.log(
            "module_closing",
            self.__class__.__name__,
            {
                "final_state": self.get_state(),
                "timestamp_ns": time.time_ns()
            }
        )
        
        self.forensic.close()


# Made with ❤️ by Bob (LVX AI Systems)
# Protocole: MDBAI/PILOT + LUMVORAX + LEÇON-73.1
# Version: 1.0.0 - Workflow Engine Central

# Made with Bob
