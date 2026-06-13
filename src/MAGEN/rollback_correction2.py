#!/usr/bin/env python3
"""
Script de ROLLBACK de CORRECTION #2 (Boundary Validation)
Restaure les fichiers DSL à leur état V5 (avant safe_operations)
"""

import os
import shutil

# Supprimer safe_operations.py
safe_ops_file = "dsl/safe_operations.py"
if os.path.exists(safe_ops_file):
    os.remove(safe_ops_file)
    print(f"✅ Supprimé: {safe_ops_file}")

# Restaurer structural_detector.py
with open("dsl/structural_detector.py", "r") as f:
    content = f.read()

# Supprimer l'import
content = content.replace("from .safe_operations import safe_get_cell, validate_bounds\n", "")

# Restaurer detect_L_shapes
content = content.replace(
    """        # Utilisation de safe_get_cell pour éviter IndexError
        if not validate_bounds(grid, y, x):
            continue
        
        cell = safe_get_cell(grid, y, x, default=0)""",
    """        if y < 0 or y >= h or x < 0 or x >= w:
            continue
        
        cell = grid[y, x]"""
)

# Restaurer detect_corners
content = content.replace(
    """        # Utilisation de safe_get_cell pour éviter IndexError
        if not validate_bounds(grid, y, x):
            continue
        
        cell = safe_get_cell(grid, y, x, default=0)""",
    """        if y < 0 or y >= h or x < 0 or x >= w:
            continue
        
        cell = grid[y, x]"""
)

# Restaurer detect_endpoints
content = content.replace(
    """        # Utilisation de safe_get_cell pour éviter IndexError
        if not validate_bounds(grid, y, x):
            continue
        
        cell = safe_get_cell(grid, y, x, default=0)""",
    """        if y < 0 or y >= h or x < 0 or x >= w:
            continue
        
        cell = grid[y, x]"""
)

with open("dsl/structural_detector.py", "w") as f:
    f.write(content)
print("✅ Restauré: dsl/structural_detector.py")

# Restaurer primitives_advanced.py
with open("dsl/primitives_advanced.py", "r") as f:
    content = f.read()

# Supprimer l'import
content = content.replace("from .safe_operations import safe_get_cell, safe_set_cell, validate_bounds\n", "")

# Restaurer detect_and_mark_L_shapes
content = content.replace(
    """            # Utilisation de safe_get_cell pour éviter IndexError
            if not validate_bounds(grid, y, x):
                continue
            
            cell = safe_get_cell(grid, y, x, default=0)""",
    """            if y < 0 or y >= h or x < 0 or x >= w:
                continue
            
            cell = grid[y, x]"""
)

# Restaurer sample_grid_non_integer
content = content.replace(
    """    # Utilisation de safe_get_cell pour éviter IndexError
    if not validate_bounds(grid, y0, x0):
        return 0
    
    return safe_get_cell(grid, y0, x0, default=0)""",
    """    if y0 < 0 or y0 >= grid.shape[0] or x0 < 0 or x0 >= grid.shape[1]:
        return 0
    
    return grid[y0, x0]"""
)

# Restaurer downsample_by_majority
content = content.replace(
    """            # Utilisation de safe_get_cell pour éviter IndexError
            if validate_bounds(grid, y, x):
                values.append(safe_get_cell(grid, y, x, default=0))""",
    """            if 0 <= y < h and 0 <= x < w:
                values.append(grid[y, x])"""
)

with open("dsl/primitives_advanced.py", "w") as f:
    f.write(content)
print("✅ Restauré: dsl/primitives_advanced.py")

# Restaurer shape_corner_marker.py
with open("dsl/shape_corner_marker.py", "r") as f:
    content = f.read()

# Supprimer l'import
content = content.replace("from .safe_operations import safe_set_cell, validate_bounds\n", "")

# Restaurer mark_shape_corners
content = content.replace(
    """            # Utilisation de safe_set_cell pour éviter IndexError
            if validate_bounds(result, y, x):
                safe_set_cell(result, y, x, marker_color)""",
    """            if 0 <= y < result.shape[0] and 0 <= x < result.shape[1]:
                result[y, x] = marker_color"""
)

with open("dsl/shape_corner_marker.py", "w") as f:
    f.write(content)
print("✅ Restauré: dsl/shape_corner_marker.py")

print("\n🎉 ROLLBACK CORRECTION #2 TERMINÉ")
print("Tous les fichiers DSL restaurés à leur état V5")

# Made with Bob
