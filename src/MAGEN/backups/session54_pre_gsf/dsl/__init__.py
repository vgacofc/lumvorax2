"""
MAGEN DSL Module - Domain Specific Language for ARC-AGI Transformations
"""

from .primitives import *
from .program import Program
from .composer import Composer

__all__ = [
    'Program',
    'Composer',
    # Primitives exported from primitives.py
]

# Made with Bob
