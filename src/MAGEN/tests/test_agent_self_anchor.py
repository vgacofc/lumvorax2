"""
Tests Unitaires - Agent Self-Anchor Module
==========================================

**Objectif**: Valider identification avatar via corrélation causale motion↔action

**Tests**:
1. Identification avatar en < 10 steps
2. Confidence > 0.8
3. Robustesse multi-entités
4. Gestion entités statiques
5. Reset complet

**Session**: 73 - Jour 8 - Phase 1
**Date**: 2026-06-15
**Version**: 1.0.0
"""

import pytest
import sys
from pathlib import Path

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.agent_self_anchor import AgentSelfAnchor, MotionSignature, AnchorEvent


class TestAgentSelfAnchor:
    """Tests du module AgentSelfAnchor"""
    
    def test_initialization(self):
        """Test initialisation module"""
        anchor = AgentSelfAnchor()
        
        assert anchor.agent_id is None
        assert anchor.confidence == 0.0
        assert anchor.step_count == 0
        assert len(anchor.motion_signatures) == 0
        assert len(anchor.action_history) == 0
        assert len(anchor.trajectory_history) == 0
    
    def test_single_entity_identification(self):
        """Test identification avatar unique (cas simple)"""
        anchor = AgentSelfAnchor()
        
        # Simuler 5 steps avec 1 entité mobile
        for step in range(5):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            
            anchor.observe_motion(prev_entities, curr_entities, f'ACTION{step}')
        
        # Vérifier identification
        avatar_id = anchor.identify_avatar()
        assert avatar_id == 1
        assert anchor.confidence >= 0.8
        assert anchor.identification_step is not None
        assert anchor.identification_step <= 5
    
    def test_multi_entity_identification(self):
        """Test identification avatar parmi plusieurs entités"""
        anchor = AgentSelfAnchor()
        
        # Simuler 5 steps avec 3 entités (1 mobile, 2 statiques)
        for step in range(5):
            prev_entities = [
                {'id': 1, 'x': step, 'y': 0, 'type': 'player'},      # Mobile
                {'id': 2, 'x': 5, 'y': 5, 'type': 'obstacle'},       # Statique
                {'id': 3, 'x': 10, 'y': 10, 'type': 'goal'}          # Statique
            ]
            curr_entities = [
                {'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'},  # Bouge
                {'id': 2, 'x': 5, 'y': 5, 'type': 'obstacle'},       # Ne bouge pas
                {'id': 3, 'x': 10, 'y': 10, 'type': 'goal'}          # Ne bouge pas
            ]
            
            anchor.observe_motion(prev_entities, curr_entities, f'ACTION{step}')
        
        # Vérifier identification correcte
        avatar_id = anchor.identify_avatar()
        assert avatar_id == 1  # Seule entité mobile
        assert anchor.confidence >= 0.8
    
    def test_identification_speed(self):
        """Test identification en < 10 steps"""
        anchor = AgentSelfAnchor()
        
        # Simuler mouvements clairs
        for step in range(10):
            prev_entities = [{'id': 1, 'x': step, 'y': step, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': step + 1, 'type': 'player'}]
            
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
            
            # Vérifier si identifié
            if anchor.agent_id is not None:
                break
        
        # Doit être identifié en < 10 steps
        assert anchor.agent_id == 1
        assert anchor.identification_step is not None
        assert anchor.identification_step < 10
    
    def test_get_avatar_position(self):
        """Test récupération position avatar"""
        anchor = AgentSelfAnchor()
        
        # Identifier avatar
        for step in range(5):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        # Récupérer position
        current_entities = [{'id': 1, 'x': 10, 'y': 5, 'type': 'player'}]
        position = anchor.get_avatar_position(current_entities)
        
        assert position == (10, 5)
    
    def test_get_avatar_position_not_identified(self):
        """Test position avatar non identifié"""
        anchor = AgentSelfAnchor()
        
        current_entities = [{'id': 1, 'x': 10, 'y': 5, 'type': 'player'}]
        position = anchor.get_avatar_position(current_entities)
        
        assert position is None
    
    def test_trajectory_history(self):
        """Test historique trajectoire"""
        anchor = AgentSelfAnchor()
        
        # Simuler mouvements
        positions = [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0)]
        for i, pos in enumerate(positions[:-1]):
            prev_entities = [{'id': 1, 'x': pos[0], 'y': pos[1], 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': positions[i+1][0], 'y': positions[i+1][1], 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        # Vérifier trajectoire (contient positions après mouvement)
        trajectory = anchor.get_trajectory()
        assert len(trajectory) > 0
        # La trajectoire contient les positions 1,2,3,4 (pas 0 car c'est avant premier mouvement)
        assert trajectory[-1] == positions[-2]  # Dernière position enregistrée = avant-dernière simulée
    
    def test_statistics(self):
        """Test statistiques identification"""
        anchor = AgentSelfAnchor()
        
        # Simuler quelques steps
        for step in range(3):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        stats = anchor.get_statistics()
        
        assert 'agent_id' in stats
        assert 'confidence' in stats
        assert 'current_step' in stats
        assert stats['current_step'] == 3
        assert 'entities_tracked' in stats
        assert 'identified' in stats
    
    def test_reset(self):
        """Test reset complet"""
        anchor = AgentSelfAnchor()
        
        # Identifier avatar
        for step in range(5):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        assert anchor.agent_id is not None
        
        # Reset
        anchor.reset()
        
        # Vérifier état initial
        assert anchor.agent_id is None
        assert anchor.confidence == 0.0
        assert anchor.step_count == 0
        assert len(anchor.motion_signatures) == 0
        assert len(anchor.action_history) == 0
        assert len(anchor.trajectory_history) == 0
    
    def test_forensic_logging(self):
        """Test logging forensique"""
        anchor = AgentSelfAnchor()
        
        # Simuler observations
        for step in range(3):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        # Vérifier événements loggés
        assert len(anchor.events) > 0
        
        # Vérifier types événements
        event_types = [e.event_type for e in anchor.events]
        assert 'anchor_initialized' in event_types
        assert 'motion_observed' in event_types
    
    def test_confidence_threshold(self):
        """Test seuil confidence"""
        anchor = AgentSelfAnchor()
        anchor.confidence_threshold = 0.9  # Seuil élevé
        
        # Simuler peu d'observations
        for step in range(2):
            prev_entities = [{'id': 1, 'x': step, 'y': 0, 'type': 'player'}]
            curr_entities = [{'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'}]
            anchor.observe_motion(prev_entities, curr_entities, 'ACTION1')
        
        # Ne devrait pas identifier (confidence insuffisante)
        avatar_id = anchor.identify_avatar()
        assert avatar_id is None or anchor.confidence < 0.9
    
    def test_multiple_moving_entities(self):
        """Test plusieurs entités mobiles (cas complexe)"""
        anchor = AgentSelfAnchor()
        
        # Simuler 2 entités mobiles (avatar bouge plus souvent)
        for step in range(6):
            prev_entities = [
                {'id': 1, 'x': step, 'y': 0, 'type': 'player'},      # Bouge toujours
                {'id': 2, 'x': step // 2, 'y': 5, 'type': 'enemy'}   # Bouge 1 fois sur 2
            ]
            curr_entities = [
                {'id': 1, 'x': step + 1, 'y': 0, 'type': 'player'},
                {'id': 2, 'x': (step + 1) // 2, 'y': 5, 'type': 'enemy'}
            ]
            
            anchor.observe_motion(prev_entities, curr_entities, f'ACTION{step}')
        
        # Avatar devrait être entité 1 (bouge plus souvent)
        avatar_id = anchor.identify_avatar()
        assert avatar_id == 1
        assert anchor.confidence >= 0.8


if __name__ == '__main__':
    pytest.main([__file__, '-v'])

# Made with Bob
