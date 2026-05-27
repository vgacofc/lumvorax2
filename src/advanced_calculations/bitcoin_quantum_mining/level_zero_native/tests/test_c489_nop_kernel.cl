// Kernel Gen9 ULTRA-MINIMAL - Ne fait RIEN
// Objectif: Valider que le pipeline GPGPU fonctionne
// Ce kernel se termine immédiatement sans aucune opération

__kernel void nop_kernel(__global uint* output)
{
    // NOP - Ne fait absolument rien
    // Le kernel se termine immédiatement
}

; Made with Bob
