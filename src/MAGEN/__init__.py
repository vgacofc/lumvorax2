"""
MAGEN - Mémoire Artificielle GEN
Système de mémoire artificielle pour ARC-AGI-3

Modules:
- core: Système mémoire multi-échelle
- perception: Traitement grilles ARC
- agent: Agent exploration/résolution
- forensic: Logs forensiques LumVorax
"""

__version__ = "1.0.0"
__author__ = "LumVorax Team"

from .core.magen_memory import MAGENMemory, Experience, Pattern, MAGENMetrics
from .perception.arc_perception import ARCPerception, GridFeatures
from .agent.arc_agent import ARCAgent
from .forensic.lumvorax_logger import LumVoraxLogger, ForensicEvent

__all__ = [
    'MAGENMemory',
    'Experience',
    'Pattern',
    'MAGENMetrics',
    'ARCPerception',
    'GridFeatures',
    'ARCAgent',
    'LumVoraxLogger',
    'ForensicEvent'
]

# Made with Bob
