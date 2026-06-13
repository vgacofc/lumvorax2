"""
ARCHypothesisTester - Test et validation d'hypothèses de transformation
Génère, teste et score les hypothèses pour résoudre les puzzles ARC
"""

import numpy as np
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
from .arc_rule_extractor import ARCRuleExtractor, TransformationHypothesis, TrainingPair


@dataclass
class HypothesisTestResult:
    """Résultat du test d'une hypothèse"""
    hypothesis: TransformationHypothesis
    validation_score: float  # 0.0 à 1.0 sur training pairs
    predicted_output: Optional[np.ndarray]  # Prédiction sur test input
    test_confidence: float  # Confiance dans la prédiction
    details: Dict  # Détails du test
    
    def __repr__(self):
        return f"Test({self.hypothesis.type.value}, val={self.validation_score:.2f}, conf={self.test_confidence:.2f})"


class ARCHypothesisTester:
    """
    Testeur d'hypothèses ARC
    
    Workflow:
    1. Recevoir training pairs + test input
    2. Générer hypothèses via ARCRuleExtractor
    3. Valider chaque hypothèse sur training pairs
    4. Scorer et ranker les hypothèses
    5. Appliquer la meilleure au test input
    6. Retourner prédiction + confiance
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.extractor = ARCRuleExtractor(verbose=verbose)
        self.test_results: List[HypothesisTestResult] = []
        
    def add_training_pair(self, input_grid: np.ndarray, output_grid: np.ndarray, pair_id: int = 0):
        """Ajoute une paire d'entraînement"""
        self.extractor.add_training_pair(input_grid, output_grid, pair_id)
    
    def test_all_hypotheses(self, test_input: np.ndarray) -> List[HypothesisTestResult]:
        """
        Teste toutes les hypothèses générées
        
        Args:
            test_input: Grille de test à transformer
            
        Returns:
            Liste de résultats triés par score de validation
        """
        if self.verbose:
            print(f"\n[HypothesisTester] 🧪 Test de toutes les hypothèses sur test input {test_input.shape}")
        
        # Extraire les règles depuis training pairs
        hypotheses = self.extractor.extract_rules()
        
        if not hypotheses:
            if self.verbose:
                print("[HypothesisTester] ⚠️ Aucune hypothèse générée")
            return []
        
        # Tester chaque hypothèse
        self.test_results = []
        for hyp in hypotheses:
            result = self._test_single_hypothesis(hyp, test_input)
            self.test_results.append(result)
        
        # Trier par score de validation décroissant
        self.test_results.sort(key=lambda r: r.validation_score, reverse=True)
        
        if self.verbose:
            print(f"\n[HypothesisTester] 📊 {len(self.test_results)} hypothèses testées:")
            for i, result in enumerate(self.test_results[:5], 1):
                print(f"  {i}. {result}")
        
        return self.test_results
    
    def _test_single_hypothesis(self, hypothesis: TransformationHypothesis, test_input: np.ndarray) -> HypothesisTestResult:
        """Teste une seule hypothèse"""
        # Valider sur training pairs
        validation_score = self.extractor.validate_hypothesis(hypothesis)
        
        # Appliquer au test input
        try:
            predicted_output = self.extractor.apply_hypothesis(hypothesis, test_input)
            success = True
        except Exception as e:
            if self.verbose:
                print(f"[HypothesisTester] ⚠️ Erreur application {hypothesis.type.value}: {e}")
            predicted_output = None
            success = False
        
        # Calculer confiance de test
        test_confidence = self._calculate_test_confidence(
            hypothesis, 
            test_input, 
            predicted_output, 
            validation_score
        )
        
        return HypothesisTestResult(
            hypothesis=hypothesis,
            validation_score=validation_score,
            predicted_output=predicted_output,
            test_confidence=test_confidence,
            details={
                "success": success,
                "test_input_shape": test_input.shape,
                "predicted_shape": predicted_output.shape if predicted_output is not None else None
            }
        )
    
    def _calculate_test_confidence(
        self, 
        hypothesis: TransformationHypothesis,
        test_input: np.ndarray,
        predicted_output: Optional[np.ndarray],
        validation_score: float
    ) -> float:
        """
        Calcule la confiance dans la prédiction de test
        
        Facteurs:
        - Score de validation sur training pairs (poids: 0.6)
        - Confiance initiale de l'hypothèse (poids: 0.2)
        - Cohérence de la prédiction (poids: 0.2)
        """
        if predicted_output is None:
            return 0.0
        
        # Facteur 1: Validation score (0.6)
        factor1 = validation_score * 0.6
        
        # Facteur 2: Confiance hypothèse (0.2)
        factor2 = hypothesis.confidence * 0.2
        
        # Facteur 3: Cohérence prédiction (0.2)
        coherence = self._check_prediction_coherence(test_input, predicted_output)
        factor3 = coherence * 0.2
        
        return factor1 + factor2 + factor3
    
    def _check_prediction_coherence(self, test_input: np.ndarray, predicted_output: np.ndarray) -> float:
        """
        Vérifie la cohérence de la prédiction
        
        Critères:
        - Taille raisonnable (pas trop grande/petite)
        - Couleurs valides (0-9)
        - Pas de valeurs aberrantes
        """
        score = 1.0
        
        # Vérifier taille
        size_ratio = (predicted_output.shape[0] * predicted_output.shape[1]) / (test_input.shape[0] * test_input.shape[1])
        if size_ratio > 100 or size_ratio < 0.01:
            score *= 0.5  # Taille suspecte
        
        # Vérifier couleurs valides (0-9 pour ARC)
        if np.any(predicted_output < 0) or np.any(predicted_output > 9):
            score *= 0.3  # Couleurs invalides
        
        # Vérifier pas de NaN ou Inf
        if np.any(np.isnan(predicted_output)) or np.any(np.isinf(predicted_output)):
            score = 0.0  # Prédiction invalide
        
        return score
    
    def get_best_prediction(self) -> Optional[HypothesisTestResult]:
        """Retourne la meilleure prédiction (score validation max)"""
        if not self.test_results:
            return None
        return self.test_results[0]
    
    def get_high_confidence_predictions(self, min_validation: float = 0.8) -> List[HypothesisTestResult]:
        """Retourne les prédictions avec validation >= min_validation"""
        return [r for r in self.test_results if r.validation_score >= min_validation]
    
    def get_consensus_prediction(self, top_n: int = 3) -> Optional[np.ndarray]:
        """
        Obtient une prédiction par consensus des top N hypothèses
        
        Si plusieurs hypothèses donnent la même prédiction, c'est un bon signe
        """
        if not self.test_results or len(self.test_results) < top_n:
            return None
        
        # Collecter les top N prédictions
        predictions = []
        for result in self.test_results[:top_n]:
            if result.predicted_output is not None:
                predictions.append(result.predicted_output)
        
        if not predictions:
            return None
        
        # Vérifier si plusieurs sont identiques
        for i, pred1 in enumerate(predictions):
            count = 1
            for pred2 in predictions[i+1:]:
                if np.array_equal(pred1, pred2):
                    count += 1
            
            # Si majorité (>50%), retourner cette prédiction
            if count > len(predictions) / 2:
                if self.verbose:
                    print(f"[HypothesisTester] ✅ Consensus trouvé: {count}/{len(predictions)} hypothèses identiques")
                return pred1
        
        # Pas de consensus, retourner la meilleure
        return predictions[0]
    
    def get_statistics(self) -> Dict:
        """Retourne des statistiques sur les tests"""
        if not self.test_results:
            return {
                "total_hypotheses": 0,
                "successful_predictions": 0,
                "best_validation_score": 0.0,
                "avg_validation_score": 0.0,
                "high_confidence_count": 0
            }
        
        successful = len([r for r in self.test_results if r.predicted_output is not None])
        high_conf = len([r for r in self.test_results if r.validation_score >= 0.8])
        avg_val = np.mean([r.validation_score for r in self.test_results])
        
        return {
            "total_hypotheses": len(self.test_results),
            "successful_predictions": successful,
            "best_validation_score": self.test_results[0].validation_score,
            "avg_validation_score": float(avg_val),
            "high_confidence_count": high_conf,
            "transformation_types": list(set(r.hypothesis.type.value for r in self.test_results))
        }
    
    def explain_best_prediction(self) -> str:
        """Génère une explication de la meilleure prédiction"""
        best = self.get_best_prediction()
        if not best:
            return "Aucune prédiction disponible"
        
        explanation = f"""
🎯 MEILLEURE PRÉDICTION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Transformation: {best.hypothesis.type.value}
Description: {best.hypothesis.description}

Scores:
  • Validation (training pairs): {best.validation_score:.1%}
  • Confiance hypothèse: {best.hypothesis.confidence:.1%}
  • Confiance test: {best.test_confidence:.1%}

Paramètres: {best.hypothesis.parameters}

Détails:
  • Input shape: {best.details['test_input_shape']}
  • Output shape: {best.details['predicted_shape']}
  • Succès application: {best.details['success']}

Interprétation:
"""
        
        if best.validation_score >= 0.9:
            explanation += "  ✅ Très haute confiance - Cette transformation fonctionne parfaitement sur tous les exemples\n"
        elif best.validation_score >= 0.7:
            explanation += "  ⚠️ Confiance moyenne - Cette transformation fonctionne sur la plupart des exemples\n"
        else:
            explanation += "  ❌ Faible confiance - Cette transformation ne fonctionne que partiellement\n"
        
        return explanation


if __name__ == "__main__":
    # Test simple
    print("🧪 Test ARCHypothesisTester")
    
    tester = ARCHypothesisTester(verbose=True)
    
    # Ajouter training pairs (rotation 90°)
    inp1 = np.array([[1, 2], [3, 4]])
    out1 = np.rot90(inp1, k=1)
    tester.add_training_pair(inp1, out1, pair_id=1)
    
    inp2 = np.array([[5, 6], [7, 8]])
    out2 = np.rot90(inp2, k=1)
    tester.add_training_pair(inp2, out2, pair_id=2)
    
    # Test input
    test_input = np.array([[9, 10], [11, 12]])
    
    # Tester toutes les hypothèses
    results = tester.test_all_hypotheses(test_input)
    
    # Meilleure prédiction
    best = tester.get_best_prediction()
    if best:
        print(f"\n✅ Meilleure prédiction:")
        print(f"   Transformation: {best.hypothesis.type.value}")
        print(f"   Validation: {best.validation_score:.1%}")
        print(f"   Confiance: {best.test_confidence:.1%}")
        print(f"\n   Input:\n{test_input}")
        print(f"\n   Predicted:\n{best.predicted_output}")
        
        # Explication
        print(tester.explain_best_prediction())
    
    # Statistiques
    stats = tester.get_statistics()
    print(f"\n📊 Statistiques:")
    for key, value in stats.items():
        print(f"   {key}: {value}")

# Made with Bob
