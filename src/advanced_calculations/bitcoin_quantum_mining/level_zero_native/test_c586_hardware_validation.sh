#!/bin/bash
# Test C586: Validation Hardware Complète GPU Intel Gen9.5
# Objectif: Vérifier TOUTES les hypothèses hardware avant implémentation

echo "=== TEST C586: VALIDATION HARDWARE COMPLÈTE ==="
echo ""
date
echo ""

LOG_FILE="logs/hardware_validation_$(date +%Y%m%d_%H%M%S).log"
mkdir -p logs

{
    echo "=== VALIDATION HARDWARE GPU INTEL GEN9.5 ==="
    echo ""
    
    # 1. Architecture GPU exacte
    echo "### 1. ARCHITECTURE GPU EXACTE ###"
    echo ""
    
    echo "--- lspci GPU ---"
    lspci -vnn | grep -A 20 VGA
    echo ""
    
    echo "--- GPU Device ID ---"
    lspci -n | grep "00:02.0"
    echo ""
    
    # 2. i915 Capabilities
    echo "### 2. I915 CAPABILITIES ###"
    echo ""
    
    if [ -f /sys/kernel/debug/dri/0/i915_capabilities ]; then
        echo "--- i915_capabilities ---"
        echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_capabilities
        echo ""
    else
        echo "❌ /sys/kernel/debug/dri/0/i915_capabilities not accessible"
        echo ""
    fi
    
    # 3. Engine Info
    echo "### 3. ENGINE INFO ###"
    echo ""
    
    if [ -f /sys/kernel/debug/dri/0/i915_engine_info ]; then
        echo "--- i915_engine_info ---"
        echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_engine_info
        echo ""
    else
        echo "❌ /sys/kernel/debug/dri/0/i915_engine_info not accessible"
        echo ""
    fi
    
    # 4. PPGTT Info
    echo "### 4. PPGTT INFO ###"
    echo ""
    
    if [ -f /sys/kernel/debug/dri/0/i915_gem_gtt ]; then
        echo "--- i915_gem_gtt (first 200 lines) ---"
        echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_gem_gtt | head -200
        echo ""
    else
        echo "❌ /sys/kernel/debug/dri/0/i915_gem_gtt not accessible"
        echo ""
    fi
    
    # 5. GT Frequency
    echo "### 5. GT FREQUENCY ###"
    echo ""
    
    echo "--- Current frequency ---"
    cat /sys/class/drm/card0/gt_cur_freq_mhz 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- Max frequency ---"
    cat /sys/class/drm/card0/gt_max_freq_mhz 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- Min frequency ---"
    cat /sys/class/drm/card0/gt_min_freq_mhz 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- Boost frequency ---"
    cat /sys/class/drm/card0/gt_boost_freq_mhz 2>/dev/null || echo "Not available"
    echo ""
    
    # 6. RC6 Power States
    echo "### 6. RC6 POWER STATES ###"
    echo ""
    
    echo "--- RC6 residency ---"
    cat /sys/class/drm/card0/rc6_residency_ms 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- RC6p residency ---"
    cat /sys/class/drm/card0/rc6p_residency_ms 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- RC6pp residency ---"
    cat /sys/class/drm/card0/rc6pp_residency_ms 2>/dev/null || echo "Not available"
    echo ""
    
    # 7. i915 Parameters
    echo "### 7. I915 PARAMETERS ###"
    echo ""
    
    echo "--- i915 module parameters ---"
    ls -la /sys/module/i915/parameters/ 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- enable_guc ---"
    cat /sys/module/i915/parameters/enable_guc 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- enable_ppgtt ---"
    cat /sys/module/i915/parameters/enable_ppgtt 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- enable_execlists ---"
    cat /sys/module/i915/parameters/enable_execlists 2>/dev/null || echo "Not available"
    echo ""
    
    echo "--- enable_hangcheck ---"
    cat /sys/module/i915/parameters/enable_hangcheck 2>/dev/null || echo "Not available"
    echo ""
    
    # 8. GEM Objects (pendant OpenCL)
    echo "### 8. GEM OBJECTS (SNAPSHOT) ###"
    echo ""
    
    if [ -f /sys/kernel/debug/dri/0/i915_gem_objects ]; then
        echo "--- i915_gem_objects (first 100 lines) ---"
        echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_gem_objects | head -100
        echo ""
    else
        echo "❌ /sys/kernel/debug/dri/0/i915_gem_objects not accessible"
        echo ""
    fi
    
    # 9. Kernel Boot Parameters
    echo "### 9. KERNEL BOOT PARAMETERS ###"
    echo ""
    
    echo "--- i915 in cmdline ---"
    cat /proc/cmdline | grep -o 'i915[^ ]*'
    echo ""
    
    # 10. DRM Info
    echo "### 10. DRM INFO ###"
    echo ""
    
    echo "--- DRM devices ---"
    ls -la /dev/dri/
    echo ""
    
    echo "--- DRM card0 ---"
    ls -la /sys/class/drm/card0/
    echo ""
    
    # 11. GPU Memory Info
    echo "### 11. GPU MEMORY INFO ###"
    echo ""
    
    if [ -f /sys/kernel/debug/dri/0/i915_gem_stolen ]; then
        echo "--- Stolen memory ---"
        echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_gem_stolen
        echo ""
    fi
    
    # 12. Firmware Info
    echo "### 12. FIRMWARE INFO ###"
    echo ""
    
    echo "--- GuC firmware ---"
    ls -la /lib/firmware/i915/*guc* 2>/dev/null || echo "Not found"
    echo ""
    
    echo "--- HuC firmware ---"
    ls -la /lib/firmware/i915/*huc* 2>/dev/null || echo "Not found"
    echo ""
    
    echo "--- DMC firmware ---"
    ls -la /lib/firmware/i915/*dmc* 2>/dev/null || echo "Not found"
    echo ""
    
    echo ""
    echo "=== VALIDATION COMPLÉTÉE ==="
    echo ""
    
} 2>&1 | tee "$LOG_FILE"

echo ""
echo "✅ Log sauvegardé: $LOG_FILE"
echo ""
echo "=== PROCHAINE ÉTAPE: Lancer OpenCL avec intel_gpu_top ==="
echo ""
