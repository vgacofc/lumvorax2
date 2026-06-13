"""
ARCRuleExtractor - Extraction de règles depuis training pairs
Analyse les paires input→output pour identifier la transformation
"""

import numpy as np
from typing import List, Dict, Tuple, Optional, Set
from dataclasses import dataclass
from enum import Enum


class TransformationType(Enum):
    """Types de transformations ARC détectables"""
    ROTATION_90 = "rotation_90"
    ROTATION_180 = "rotation_180"
    ROTATION_270 = "rotation_270"
    FLIP_HORIZONTAL = "flip_horizontal"
    FLIP_VERTICAL = "flip_vertical"
    COPY = "copy"
    SCALE_UP = "scale_up"
    SCALE_DOWN = "scale_down"
    COLOR_MAP = "color_map"
    PATTERN_REPEAT = "pattern_repeat"
    FILL_REGION = "fill_region"
    EXTRACT_SHAPE = "extract_shape"
    OVERLAY = "overlay"
    GRID_SPLIT = "grid_split"
    SYMMETRY_COMPLETE = "symmetry_complete"
    UNKNOWN = "unknown"


@dataclass
class TransformationHypothesis:
    """Hypothèse de transformation"""
    type: TransformationType
    confidence: float  # 0.0 à 1.0
    parameters: Dict  # Paramètres spécifiques à la transformation
    description: str
    
    def __repr__(self):
        return f"{self.type.value} (conf={self.confidence:.2f}): {self.description}"


@dataclass
class TrainingPair:
    """Paire input→output d'entraînement"""
    input_grid: np.ndarray
    output_grid: np.ndarray
    pair_id: int


class ARCRuleExtractor:
    """
    Extracteur de règles ARC depuis training pairs
    
    Analyse les paires input→output pour identifier:
    - Transformations géométriques (rotation, flip, scale)
    - Transformations de couleurs (mapping, fill)
    - Patterns et répétitions
    - Relations spatiales
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.training_pairs: List[TrainingPair] = []
        self.hypotheses: List[TransformationHypothesis] = []
        
    def add_training_pair(self, input_grid: np.ndarray, output_grid: np.ndarray, pair_id: int = 0):
        """Ajoute une paire d'entraînement"""
        pair = TrainingPair(
            input_grid=input_grid.copy(),
            output_grid=output_grid.copy(),
            pair_id=pair_id
        )
        self.training_pairs.append(pair)
        if self.verbose:
            print(f"[RuleExtractor] Training pair {pair_id} ajoutée: {input_grid.shape} → {output_grid.shape}")
    
    def extract_rules(self) -> List[TransformationHypothesis]:
        """
        Extrait les règles depuis tous les training pairs
        Retourne une liste d'hypothèses triées par confiance
        """
        if not self.training_pairs:
            if self.verbose:
                print("[RuleExtractor] ⚠️ Aucun training pair disponible")
            return []
        
        self.hypotheses = []
        
        # Tester chaque type de transformation
        self._test_geometric_transforms()
        self._test_color_transforms()
        self._test_pattern_transforms()
        self._test_spatial_transforms()
        
        # Trier par confiance décroissante
        self.hypotheses.sort(key=lambda h: h.confidence, reverse=True)
        
        if self.verbose:
            print(f"\n[RuleExtractor] 🎯 {len(self.hypotheses)} hypothèses générées:")
            for i, hyp in enumerate(self.hypotheses[:5], 1):
                print(f"  {i}. {hyp}")
        
        return self.hypotheses
    
    def _test_geometric_transforms(self):
        """Teste les transformations géométriques"""
        for pair in self.training_pairs:
            inp, out = pair.input_grid, pair.output_grid
            
            # Test rotation 90°
            if np.array_equal(out, np.rot90(inp, k=1)):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.ROTATION_90,
                    confidence=1.0,
                    parameters={"k": 1},
                    description="Rotation 90° sens horaire"
                ))
            
            # Test rotation 180°
            if np.array_equal(out, np.rot90(inp, k=2)):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.ROTATION_180,
                    confidence=1.0,
                    parameters={"k": 2},
                    description="Rotation 180°"
                ))
            
            # Test rotation 270°
            if np.array_equal(out, np.rot90(inp, k=3)):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.ROTATION_270,
                    confidence=1.0,
                    parameters={"k": 3},
                    description="Rotation 270° sens horaire"
                ))
            
            # Test flip horizontal
            if np.array_equal(out, np.fliplr(inp)):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.FLIP_HORIZONTAL,
                    confidence=1.0,
                    parameters={},
                    description="Flip horizontal (miroir vertical)"
                ))
            
            # Test flip vertical
            if np.array_equal(out, np.flipud(inp)):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.FLIP_VERTICAL,
                    confidence=1.0,
                    parameters={},
                    description="Flip vertical (miroir horizontal)"
                ))
            
            # Test copie identique
            if np.array_equal(out, inp):
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.COPY,
                    confidence=1.0,
                    parameters={},
                    description="Copie identique (pas de transformation)"
                ))
    
    def _test_color_transforms(self):
        """Teste les transformations de couleurs"""
        for pair in self.training_pairs:
            inp, out = pair.input_grid, pair.output_grid
            
            # Vérifier si c'est un mapping de couleurs
            if inp.shape == out.shape:
                color_map = self._detect_color_mapping(inp, out)
                if color_map:
                    confidence = len(color_map) / (len(np.unique(inp)) + 1)
                    self.hypotheses.append(TransformationHypothesis(
                        type=TransformationType.COLOR_MAP,
                        confidence=min(confidence, 1.0),
                        parameters={"mapping": color_map},
                        description=f"Mapping de couleurs: {color_map}"
                    ))
    
    def _detect_color_mapping(self, inp: np.ndarray, out: np.ndarray) -> Optional[Dict[int, int]]:
        """Détecte un mapping de couleurs input→output"""
        if inp.shape != out.shape:
            return None
        
        color_map = {}
        for i in range(inp.shape[0]):
            for j in range(inp.shape[1]):
                in_color = inp[i, j]
                out_color = out[i, j]
                
                if in_color in color_map:
                    if color_map[in_color] != out_color:
                        return None  # Mapping inconsistant
                else:
                    color_map[in_color] = out_color
        
        # Vérifier que ce n'est pas une copie identique
        if all(k == v for k, v in color_map.items()):
            return None
        
        return color_map
    
    def _test_pattern_transforms(self):
        """Teste les transformations de patterns"""
        for pair in self.training_pairs:
            inp, out = pair.input_grid, pair.output_grid
            
            # Test répétition de pattern
            if out.shape[0] > inp.shape[0] or out.shape[1] > inp.shape[1]:
                if self._is_pattern_repeat(inp, out):
                    repeat_h = out.shape[0] // inp.shape[0]
                    repeat_v = out.shape[1] // inp.shape[1]
                    self.hypotheses.append(TransformationHypothesis(
                        type=TransformationType.PATTERN_REPEAT,
                        confidence=0.9,
                        parameters={"repeat_h": repeat_h, "repeat_v": repeat_v},
                        description=f"Répétition pattern {repeat_h}x{repeat_v}"
                    ))
    
    def _is_pattern_repeat(self, inp: np.ndarray, out: np.ndarray) -> bool:
        """Vérifie si output est une répétition de input"""
        h_in, w_in = inp.shape
        h_out, w_out = out.shape
        
        if h_out % h_in != 0 or w_out % w_in != 0:
            return False
        
        for i in range(0, h_out, h_in):
            for j in range(0, w_out, w_in):
                tile = out[i:i+h_in, j:j+w_in]
                if not np.array_equal(tile, inp):
                    return False
        
        return True
    
    def _test_spatial_transforms(self):
        """Teste les transformations spatiales complexes"""
        for pair in self.training_pairs:
            inp, out = pair.input_grid, pair.output_grid
            
            # Test extraction de forme
            if out.shape[0] < inp.shape[0] or out.shape[1] < inp.shape[1]:
                self.hypotheses.append(TransformationHypothesis(
                    type=TransformationType.EXTRACT_SHAPE,
                    confidence=0.7,
                    parameters={"size_ratio": (out.shape[0]/inp.shape[0], out.shape[1]/inp.shape[1])},
                    description=f"Extraction forme {inp.shape} → {out.shape}"
                ))
            
            # Test scale up
            if out.shape[0] > inp.shape[0] and out.shape[1] > inp.shape[1]:
                scale_h = out.shape[0] / inp.shape[0]
                scale_w = out.shape[1] / inp.shape[1]
                if abs(scale_h - scale_w) < 0.1:  # Scale uniforme
                    self.hypotheses.append(TransformationHypothesis(
                        type=TransformationType.SCALE_UP,
                        confidence=0.8,
                        parameters={"scale": scale_h},
                        description=f"Scale up x{scale_h:.1f}"
                    ))
    
    def get_best_hypothesis(self) -> Optional[TransformationHypothesis]:
        """Retourne la meilleure hypothèse (confiance maximale)"""
        if not self.hypotheses:
            return None
        return self.hypotheses[0]
    
    def get_consensus_hypotheses(self, min_confidence: float = 0.8) -> List[TransformationHypothesis]:
        """Retourne les hypothèses avec confiance >= min_confidence"""
        return [h for h in self.hypotheses if h.confidence >= min_confidence]
    
    def apply_hypothesis(self, hypothesis: TransformationHypothesis, test_input: np.ndarray) -> np.ndarray:
        """
        Applique une hypothèse à un test input
        Retourne le output prédit
        """
        if hypothesis.type == TransformationType.ROTATION_90:
            return np.rot90(test_input, k=1)
        elif hypothesis.type == TransformationType.ROTATION_180:
            return np.rot90(test_input, k=2)
        elif hypothesis.type == TransformationType.ROTATION_270:
            return np.rot90(test_input, k=3)
        elif hypothesis.type == TransformationType.FLIP_HORIZONTAL:
            return np.fliplr(test_input)
        elif hypothesis.type == TransformationType.FLIP_VERTICAL:
            return np.flipud(test_input)
        elif hypothesis.type == TransformationType.COPY:
            return test_input.copy()
        elif hypothesis.type == TransformationType.COLOR_MAP:
            mapping = hypothesis.parameters["mapping"]
            result = test_input.copy()
            for old_color, new_color in mapping.items():
                result[test_input == old_color] = new_color
            return result
        elif hypothesis.type == TransformationType.PATTERN_REPEAT:
            repeat_h = hypothesis.parameters["repeat_h"]
            repeat_v = hypothesis.parameters["repeat_v"]
            return np.tile(test_input, (repeat_h, repeat_v))
        else:
            # Transformation non implémentée, retourner input
            return test_input.copy()
    
    def validate_hypothesis(self, hypothesis: TransformationHypothesis) -> float:
        """
        Valide une hypothèse sur tous les training pairs
        Retourne le score de validation (0.0 à 1.0)
        """
        if not self.training_pairs:
            return 0.0
        
        correct = 0
        for pair in self.training_pairs:
            predicted = self.apply_hypothesis(hypothesis, pair.input_grid)
            if np.array_equal(predicted, pair.output_grid):
                correct += 1
        
        return correct / len(self.training_pairs)
    
    def get_statistics(self) -> Dict:
        """Retourne des statistiques sur l'extraction"""
        return {
            "training_pairs": len(self.training_pairs),
            "hypotheses_generated": len(self.hypotheses),
            "best_confidence": self.hypotheses[0].confidence if self.hypotheses else 0.0,
            "high_confidence_count": len([h for h in self.hypotheses if h.confidence >= 0.8]),
            "transformation_types": list(set(h.type.value for h in self.hypotheses))
        }


if __name__ == "__main__":
    # Test simple
    print("🧪 Test ARCRuleExtractor")
    
    extractor = ARCRuleExtractor(verbose=True)
    
    # Test 1: Rotation 90°
    inp1 = np.array([[1, 2], [3, 4]])
    out1 = np.rot90(inp1, k=1)
    extractor.add_training_pair(inp1, out1, pair_id=1)
    
    # Test 2: Même transformation
    inp2 = np.array([[5, 6], [7, 8]])
    out2 = np.rot90(inp2, k=1)
    extractor.add_training_pair(inp2, out2, pair_id=2)
    
    # Extraire règles
    hypotheses = extractor.extract_rules()
    
    print(f"\n✅ {len(hypotheses)} hypothèses trouvées")
    best = extractor.get_best_hypothesis()
    if best:
        print(f"🎯 Meilleure: {best}")
        
        # Valider
        score = extractor.validate_hypothesis(best)
        print(f"📊 Score validation: {score:.1%}")
        
        # Appliquer à nouveau test
        test_input = np.array([[9, 10], [11, 12]])
        predicted = extractor.apply_hypothesis(best, test_input)
        print(f"\n🔮 Prédiction sur nouveau test:")
        print(f"Input:\n{test_input}")
        print(f"Predicted:\n{predicted}")

# Made with Bob
