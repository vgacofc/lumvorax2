#!/usr/bin/env python3
"""MAGEN V40.3 Agent pour Kaggle ARC-AGI-3"""
import sys
import os
import random

# Import MAGEN (simplifié pour Kaggle)
class MAGENAgentSimple:
    """Agent MAGEN V40.3 simplifié pour Kaggle"""
    
    def __init__(self):
        self.step_count = 0
        self.max_steps = 200
    
    def is_done(self, frames, latest_frame) -> bool:
        """Return True when agent wants to stop"""
        self.step_count = len(frames)
        return self.step_count >= self.max_steps
    
    def choose_action(self, frames, latest_frame):
        """Choose next action"""
        # Pour l'instant, action aléatoire
        # TODO: Intégrer vraie logique MAGEN V40.3
        from arc_agi import GameAction
        
        actions = [
            GameAction.UP,
            GameAction.DOWN,
            GameAction.LEFT,
            GameAction.RIGHT,
            GameAction.TOGGLE_SELECTED,
            GameAction.SUBMIT
        ]
        
        # Stratégie simple: soumettre après quelques steps
        if self.step_count > 50 and random.random() < 0.1:
            return GameAction.SUBMIT
        
        return random.choice(actions)

# Export pour Kaggle
Agent = MAGENAgentSimple
