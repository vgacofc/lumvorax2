"""
MAGEN DSL Program - Représentation de programmes de transformation ARC
"""

import numpy as np
from typing import List, Callable, Any, Optional, Dict
from dataclasses import dataclass, field
import json


@dataclass
class Operation:
    """Représente une opération primitive avec ses arguments"""
    name: str
    func: Callable
    args: tuple = field(default_factory=tuple)
    kwargs: dict = field(default_factory=dict)
    
    def execute(self, grid: np.ndarray) -> np.ndarray:
        """Exécute l'opération sur une grille"""
        return self.func(grid, *self.args, **self.kwargs)
    
    def __repr__(self) -> str:
        args_str = ', '.join(str(a) for a in self.args)
        kwargs_str = ', '.join(f'{k}={v}' for k, v in self.kwargs.items())
        params = ', '.join(filter(None, [args_str, kwargs_str]))
        return f"{self.name}({params})" if params else self.name


@dataclass
class Program:
    """
    Représente un programme de transformation ARC
    
    Un programme est une séquence d'opérations primitives qui transforment
    une grille d'entrée en grille de sortie.
    
    Exemple:
        program = Program([
            Operation('crop', crop, args=(BoundingBox(0,0,2,2),)),
            Operation('rotate90', rotate90),
            Operation('scale', scale, args=(2,))
        ])
        
        result = program.execute(input_grid)
    
    """
    operations: List[Operation] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def execute(self, grid: np.ndarray) -> np.ndarray:
        """
        Exécute le programme sur une grille
        
        Args:
            grid: Grille d'entrée
            
        Returns:
            Grille transformée
            
        Raises:
            Exception: Si une opération échoue
        """
        result = grid.copy()
        
        for i, op in enumerate(self.operations):
            try:
                result = op.execute(result)
            except Exception as e:
                raise RuntimeError(
                    f"Program execution failed at operation {i} ({op}): {e}"
                ) from e
        
        return result
    
    def add_operation(self, name: str, func: Callable, 
                     args: tuple = (), kwargs: dict = None) -> 'Program':
        """
        Ajoute une opération au programme
        
        Args:
            name: Nom de l'opération
            func: Fonction à exécuter
            args: Arguments positionnels
            kwargs: Arguments nommés
            
        Returns:
            Self pour chaînage
        """
        if kwargs is None:
            kwargs = {}
        
        self.operations.append(Operation(name, func, args, kwargs))
        return self
    
    def copy(self) -> 'Program':
        """Crée une copie du programme"""
        return Program(
            operations=self.operations.copy(),
            metadata=self.metadata.copy()
        )
    
    def extend(self, other: 'Program') -> 'Program':
        """
        Étend le programme avec les opérations d'un autre programme
        
        Args:
            other: Programme à ajouter
            
        Returns:
            Nouveau programme combiné
        """
        new_program = self.copy()
        new_program.operations.extend(other.operations)
        return new_program
    
    def length(self) -> int:
        """Retourne le nombre d'opérations"""
        return len(self.operations)
    
    def complexity(self) -> int:
        """
        Calcule la complexité du programme (MDL approximatif)
        
        Returns:
            Score de complexité (plus bas = plus simple)
        """
        # Complexité basique: nombre d'opérations + nombre de paramètres
        complexity = len(self.operations)
        
        for op in self.operations:
            complexity += len(op.args) + len(op.kwargs)
        
        return complexity
    
    def to_string(self) -> str:
        """
        Convertit le programme en représentation textuelle
        
        Returns:
            Chaîne lisible du programme
        """
        if not self.operations:
            return "Program(empty)"
        
        ops_str = '\n  '.join(str(op) for op in self.operations)
        return f"Program(\n  {ops_str}\n)"
    
    def to_dict(self) -> dict:
        """
        Convertit le programme en dictionnaire sérialisable
        
        Note: Les fonctions ne sont pas sérialisées, seulement les noms
        """
        return {
            'operations': [
                {
                    'name': op.name,
                    'args': [str(a) for a in op.args],
                    'kwargs': {k: str(v) for k, v in op.kwargs.items()}
                }
                for op in self.operations
            ],
            'metadata': self.metadata,
            'complexity': self.complexity()
        }
    
    def __repr__(self) -> str:
        return self.to_string()
    
    def __len__(self) -> int:
        return len(self.operations)
    
    def __eq__(self, other: Any) -> bool:
        """Égalité basée sur la séquence d'opérations"""
        if not isinstance(other, Program):
            return False
        
        if len(self.operations) != len(other.operations):
            return False
        
        for op1, op2 in zip(self.operations, other.operations):
            if op1.name != op2.name:
                return False
            if op1.args != op2.args:
                return False
            if op1.kwargs != op2.kwargs:
                return False
        
        return True
    
    def __hash__(self) -> int:
        """Hash basé sur la séquence d'opérations"""
        return hash(tuple(
            (op.name, op.args, tuple(sorted(op.kwargs.items())))
            for op in self.operations
        ))


class ProgramLibrary:
    """
    Bibliothèque de programmes réutilisables
    
    Permet de stocker et récupérer des programmes par nom ou par pattern
    """
    
    def __init__(self):
        self.programs: Dict[str, Program] = {}
        self.tags: Dict[str, List[str]] = {}  # tag -> program_names
    
    def add(self, name: str, program: Program, tags: List[str] = None):
        """
        Ajoute un programme à la bibliothèque
        
        Args:
            name: Nom unique du programme
            program: Programme à stocker
            tags: Tags pour catégorisation
        """
        self.programs[name] = program
        
        if tags:
            for tag in tags:
                if tag not in self.tags:
                    self.tags[tag] = []
                self.tags[tag].append(name)
    
    def get(self, name: str) -> Optional[Program]:
        """Récupère un programme par nom"""
        return self.programs.get(name)
    
    def get_by_tag(self, tag: str) -> List[Program]:
        """Récupère tous les programmes avec un tag donné"""
        if tag not in self.tags:
            return []
        
        return [self.programs[name] for name in self.tags[tag] 
                if name in self.programs]
    
    def search_similar(self, program: Program, max_results: int = 5) -> List[tuple]:
        """
        Recherche programmes similaires
        
        Args:
            program: Programme de référence
            max_results: Nombre maximum de résultats
            
        Returns:
            Liste de (nom, programme, score_similarité)
        """
        results = []
        
        for name, stored_program in self.programs.items():
            # Similarité basique: opérations communes
            ops1 = set(op.name for op in program.operations)
            ops2 = set(op.name for op in stored_program.operations)
            
            if not ops1 or not ops2:
                similarity = 0.0
            else:
                similarity = len(ops1 & ops2) / len(ops1 | ops2)
            
            results.append((name, stored_program, similarity))
        
        # Trier par similarité décroissante
        results.sort(key=lambda x: x[2], reverse=True)
        
        return results[:max_results]
    
    def size(self) -> int:
        """Retourne le nombre de programmes stockés"""
        return len(self.programs)
    
    def clear(self):
        """Vide la bibliothèque"""
        self.programs.clear()
        self.tags.clear()


# ============================================================================
# FONCTIONS UTILITAIRES
# ============================================================================

def validate_program(program: Program, test_grid: np.ndarray) -> bool:
    """
    Valide qu'un programme peut s'exécuter sans erreur
    
    Args:
        program: Programme à valider
        test_grid: Grille de test
        
    Returns:
        True si le programme s'exécute sans erreur
    """
    try:
        program.execute(test_grid)
        return True
    except Exception:
        return False


def compare_programs(prog1: Program, prog2: Program, 
                    test_grids: List[np.ndarray]) -> Dict[str, Any]:
    """
    Compare deux programmes sur des grilles de test
    
    Args:
        prog1: Premier programme
        prog2: Second programme
        test_grids: Grilles de test
        
    Returns:
        Dict avec statistiques de comparaison
    """
    results = {
        'identical_outputs': 0,
        'different_outputs': 0,
        'prog1_errors': 0,
        'prog2_errors': 0,
        'both_errors': 0
    }
    
    for grid in test_grids:
        try:
            out1 = prog1.execute(grid)
            error1 = False
        except Exception:
            out1 = None
            error1 = True
            results['prog1_errors'] += 1
        
        try:
            out2 = prog2.execute(grid)
            error2 = False
        except Exception:
            out2 = None
            error2 = True
            results['prog2_errors'] += 1
        
        if error1 and error2:
            results['both_errors'] += 1
        elif not error1 and not error2:
            if np.array_equal(out1, out2):
                results['identical_outputs'] += 1
            else:
                results['different_outputs'] += 1
    
    return results


def simplify_program(program: Program) -> Program:
    """
    Simplifie un programme en éliminant les opérations redondantes
    
    Args:
        program: Programme à simplifier
        
    Returns:
        Programme simplifié
    """
    if len(program.operations) <= 1:
        return program.copy()
    
    simplified = Program(metadata=program.metadata.copy())
    
    # Règles de simplification basiques
    i = 0
    while i < len(program.operations):
        op = program.operations[i]
        
        # Règle 1: rotate90 + rotate90 + rotate90 + rotate90 = identité
        if (op.name == 'rotate90' and 
            i + 3 < len(program.operations) and
            all(program.operations[i+j].name == 'rotate90' for j in range(4))):
            i += 4  # Skip les 4 rotations
            continue
        
        # Règle 2: rotate90 + rotate90 = rotate180
        if (op.name == 'rotate90' and 
            i + 1 < len(program.operations) and
            program.operations[i+1].name == 'rotate90'):
            # Remplacer par rotate180
            from .primitives import rotate180
            simplified.add_operation('rotate180', rotate180)
            i += 2
            continue
        
        # Règle 3: mirror_h + mirror_h = identité
        if (op.name in ['mirror_horizontal', 'mirror_vertical'] and
            i + 1 < len(program.operations) and
            program.operations[i+1].name == op.name):
            i += 2  # Skip les deux miroirs
            continue
        
        # Pas de simplification, garder l'opération
        simplified.operations.append(op)
        i += 1
    
    return simplified


__all__ = [
    'Operation',
    'Program',
    'ProgramLibrary',
    'validate_program',
    'compare_programs',
    'simplify_program'
]

# Made with Bob
