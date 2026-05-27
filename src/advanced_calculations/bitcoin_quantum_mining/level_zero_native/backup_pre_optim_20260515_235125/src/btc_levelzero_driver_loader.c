/*
 * LumVorax Bitcoin Mining — Level Zero Driver Loader Manual (C198 Phase 10A)
 * 
 * OBJECTIF : Charger manuellement libze_intel_gpu.so via dlopen() pour contourner
 *            le problème de découverte du loader système (zeInit() = 0 Drivers)
 * 
 * CONTEXTE : Le loader Level Zero système (/usr/lib/x86_64-linux-gnu/libze_loader.so.1.17.42)
 *            ne découvre pas notre driver custom libze_intel_gpu.so malgré :
 *            - Installation dans /usr/lib/x86_64-linux-gnu/
 *            - Manifeste /etc/ze_loader/intel.conf créé
 *            - Variables ZE_LOADER_DRIVERS_PATH testées
 * 
 * SOLUTION : Charger le driver manuellement et appeler zeInit() via fonction exportée
 * 
 * TRAÇABILITÉ : Forensique bit-level sur TOUTES opérations dlopen/dlsym
 */

#include "btc_levelzero_driver_loader.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

/* Pointeurs fonctions Level Zero chargées dynamiquement */
static void* g_driver_handle = NULL;
static ze_result_t (*g_zeInit)(ze_init_flags_t) = NULL;
static ze_result_t (*g_zeDriverGet)(uint32_t*, ze_driver_handle_t*) = NULL;
static ze_result_t (*g_zeDeviceGet)(ze_driver_handle_t, uint32_t*, ze_device_handle_t*) = NULL;
static ze_result_t (*g_zeDeviceGetProperties)(ze_device_handle_t, ze_device_properties_t*) = NULL;
static ze_result_t (*g_zeDeviceGetMemoryProperties)(ze_device_handle_t, uint32_t*, ze_device_memory_properties_t*) = NULL;
static ze_result_t (*g_zeContextCreate)(ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*) = NULL;
static ze_result_t (*g_zeModuleCreate)(ze_context_handle_t, ze_device_handle_t, const ze_module_desc_t*, ze_module_handle_t*, ze_module_build_log_handle_t*) = NULL;
static ze_result_t (*g_zeKernelCreate)(ze_module_handle_t, const ze_kernel_desc_t*, ze_kernel_handle_t*) = NULL;
static ze_result_t (*g_zeCommandListCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_list_desc_t*, ze_command_list_handle_t*) = NULL;
static ze_result_t (*g_zeCommandQueueCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t*, ze_command_queue_handle_t*) = NULL;
static ze_result_t (*g_zeMemAllocDevice)(ze_context_handle_t, const ze_device_mem_alloc_desc_t*, size_t, size_t, ze_device_handle_t, void**) = NULL;
static ze_result_t (*g_zeMemAllocShared)(ze_context_handle_t, const ze_device_mem_alloc_desc_t*, const ze_host_mem_alloc_desc_t*, size_t, size_t, ze_device_handle_t, void**) = NULL;  /* C198 Phase 12G */
static ze_result_t (*g_zeMemFree)(ze_context_handle_t, void*) = NULL;
static ze_result_t (*g_zeCommandListAppendMemoryCopy)(ze_command_list_handle_t, void*, const void*, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*g_zeCommandListAppendLaunchKernel)(ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t*, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*g_zeCommandListClose)(ze_command_list_handle_t) = NULL;
static ze_result_t (*g_zeCommandQueueExecuteCommandLists)(ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_fence_handle_t) = NULL;
static ze_result_t (*g_zeCommandQueueSynchronize)(ze_command_queue_handle_t, uint64_t) = NULL;
static ze_result_t (*g_zeKernelSetArgumentValue)(ze_kernel_handle_t, uint32_t, size_t, const void*) = NULL;
static ze_result_t (*g_zeKernelSetGroupSize)(ze_kernel_handle_t, uint32_t, uint32_t, uint32_t) = NULL;
static ze_result_t (*g_zeContextDestroy)(ze_context_handle_t) = NULL;  /* C198 Bug Fix */
static ze_result_t (*g_zeCommandQueueDestroy)(ze_command_queue_handle_t) = NULL;  /* C198 Bug Fix */
static ze_result_t (*g_zeCommandListDestroy)(ze_command_list_handle_t) = NULL;  /* C198 Phase 12D Bug Fix */
static ze_result_t (*g_zeCommandListReset)(ze_command_list_handle_t) = NULL;  /* C198 Bug Fix #9 */
static ze_result_t (*g_zeModuleBuildLogGetString)(ze_module_build_log_handle_t, size_t*, char*) = NULL;  /* C198 Bug Fix #16 */
static ze_result_t (*g_zeModuleBuildLogDestroy)(ze_module_build_log_handle_t) = NULL;  /* C198 Bug Fix #17 */
static ze_result_t (*g_zeKernelDestroy)(ze_kernel_handle_t) = NULL;  /* C198 Bug Fix #25 */
static ze_result_t (*g_zeModuleDestroy)(ze_module_handle_t) = NULL;  /* C198 Bug Fix #26 */

/* Chemins possibles des bibliothèques */
static const char* GMMLIB_PATHS[] = {
    "/home/lvx/gmmlib_build/lib/libigdgmm.so.11",
    "/home/lvx/gmmlib_local_21/lib/libigdgmm.so.11",
    "/usr/lib/x86_64-linux-gnu/libigdgmm.so.11",
    NULL
};

static const char* DRIVER_PATHS[] = {
    "/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so",
    "/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so.1",
    "/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so.1.3.24595",
    "/home/lvx/compute-runtime_test/build/bin/libze_intel_gpu.so",
    NULL
};

/* Handle pour libigdgmm (dépendance du driver) */
static void* g_gmmlib_handle = NULL;

/*
 * Charge manuellement le driver Level Zero via dlopen()
 * Retourne 0 si succès, -1 si échec
 */
int btc_l0_load_driver_manual(void) {
    printf("\n[C198-DRIVER-LOADER] Chargement manuel driver Level Zero...\n");
    
    /* Étape 1 : Charger libigdgmm.so.11 (dépendance du driver) */
    printf("[C198-DRIVER-LOADER] Étape 1/2 : Chargement libigdgmm.so.11...\n");
    
    for (int i = 0; GMMLIB_PATHS[i] != NULL; i++) {
        printf("[C198-DRIVER-LOADER]   Tentative : %s\n", GMMLIB_PATHS[i]);
        
        g_gmmlib_handle = dlopen(GMMLIB_PATHS[i], RTLD_NOW | RTLD_GLOBAL);
        if (g_gmmlib_handle != NULL) {
            printf("[C198-DRIVER-LOADER]   ✅ libigdgmm.so.11 chargé : %s\n", GMMLIB_PATHS[i]);
            break;
        } else {
            printf("[C198-DRIVER-LOADER]   ❌ Échec : %s\n", dlerror());
        }
    }
    
    if (g_gmmlib_handle == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : libigdgmm.so.11 introuvable\n");
        return -1;
    }
    
    /* Étape 2 : Charger libze_intel_gpu.so (driver Level Zero) */
    printf("[C198-DRIVER-LOADER] Étape 2/2 : Chargement libze_intel_gpu.so...\n");
    
    for (int i = 0; DRIVER_PATHS[i] != NULL; i++) {
        printf("[C198-DRIVER-LOADER]   Tentative : %s\n", DRIVER_PATHS[i]);
        
        g_driver_handle = dlopen(DRIVER_PATHS[i], RTLD_NOW | RTLD_GLOBAL);
        if (g_driver_handle != NULL) {
            printf("[C198-DRIVER-LOADER]   ✅ Driver chargé : %s\n", DRIVER_PATHS[i]);
            
            /* Log forensique - TODO: intégrer avec système logging */
            printf("[C198-DRIVER-LOADER] Event: DRIVER_LOAD handle=%p index=%d\n",
                   g_driver_handle, i);
            
            break;
        } else {
            printf("[C198-DRIVER-LOADER]   ❌ Échec : %s\n", dlerror());
        }
    }
    
    if (g_driver_handle == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : Aucun driver Level Zero trouvé\n");
        return -1;
    }
    
    /* Charge les symboles Level Zero */
    printf("[C198-DRIVER-LOADER] Chargement symboles Level Zero...\n");
    
    #define LOAD_SYMBOL(name) \
        g_##name = dlsym(g_driver_handle, #name); \
        if (g_##name == NULL) { \
            fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : Symbole " #name " introuvable\n"); \
            dlclose(g_driver_handle); \
            g_driver_handle = NULL; \
            return -1; \
        } \
        printf("[C198-DRIVER-LOADER]   ✅ " #name " @ %p\n", (void*)g_##name);
    
    LOAD_SYMBOL(zeInit);
    LOAD_SYMBOL(zeDriverGet);
    LOAD_SYMBOL(zeDeviceGet);
    LOAD_SYMBOL(zeDeviceGetProperties);
    LOAD_SYMBOL(zeDeviceGetMemoryProperties);
    LOAD_SYMBOL(zeContextCreate);
    LOAD_SYMBOL(zeModuleCreate);
    LOAD_SYMBOL(zeKernelCreate);
    LOAD_SYMBOL(zeCommandListCreate);
    LOAD_SYMBOL(zeCommandQueueCreate);
    LOAD_SYMBOL(zeMemAllocDevice);
    LOAD_SYMBOL(zeMemAllocShared);  /* C198 Phase 12G : Shared memory UMA */
    LOAD_SYMBOL(zeMemFree);
    LOAD_SYMBOL(zeCommandListAppendMemoryCopy);
    LOAD_SYMBOL(zeCommandListAppendLaunchKernel);
    LOAD_SYMBOL(zeCommandListClose);
    LOAD_SYMBOL(zeCommandQueueExecuteCommandLists);
    LOAD_SYMBOL(zeCommandQueueSynchronize);
    LOAD_SYMBOL(zeKernelSetArgumentValue);
    LOAD_SYMBOL(zeKernelSetGroupSize);
    LOAD_SYMBOL(zeContextDestroy);  /* C198 Bug Fix */
    LOAD_SYMBOL(zeCommandQueueDestroy);  /* C198 Bug Fix */
    LOAD_SYMBOL(zeCommandListDestroy);  /* C198 Phase 12D Bug Fix */
    LOAD_SYMBOL(zeCommandListReset);  /* C198 Bug Fix #9 */
    LOAD_SYMBOL(zeModuleBuildLogGetString);  /* C198 Bug Fix #16 */
    LOAD_SYMBOL(zeModuleBuildLogDestroy);  /* C198 Bug Fix #17 */
    LOAD_SYMBOL(zeKernelDestroy);  /* C198 Bug Fix #25 */
    LOAD_SYMBOL(zeModuleDestroy);  /* C198 Bug Fix #26 */
    
    #undef LOAD_SYMBOL
    
    printf("[C198-DRIVER-LOADER] ✅ Tous les symboles chargés avec succès\n");
    return 0;
}

/*
 * Décharge le driver Level Zero
 */
void btc_l0_unload_driver_manual(void) {
    if (g_driver_handle != NULL) {
        printf("[C198-DRIVER-LOADER] Déchargement driver Level Zero...\n");
        
        /* Log forensique - TODO: intégrer avec système logging */
        printf("[C198-DRIVER-LOADER] Event: DRIVER_UNLOAD handle=%p\n",
               g_driver_handle);
        
        dlclose(g_driver_handle);
        g_driver_handle = NULL;
    }
    
    if (g_gmmlib_handle != NULL) {
        printf("[C198-DRIVER-LOADER] Déchargement libigdgmm.so.11...\n");
        dlclose(g_gmmlib_handle);
        g_gmmlib_handle = NULL;
        
        /* Réinitialise tous les pointeurs */
        g_zeInit = NULL;
        g_zeDriverGet = NULL;
        g_zeDeviceGet = NULL;
        g_zeDeviceGetProperties = NULL;
        g_zeDeviceGetMemoryProperties = NULL;
        g_zeContextCreate = NULL;
        g_zeModuleCreate = NULL;
        g_zeKernelCreate = NULL;
        g_zeCommandListCreate = NULL;
        g_zeCommandQueueCreate = NULL;
        g_zeMemAllocDevice = NULL;
        g_zeMemFree = NULL;
        g_zeCommandListAppendMemoryCopy = NULL;
        g_zeCommandListAppendLaunchKernel = NULL;
        g_zeCommandListClose = NULL;
        g_zeCommandQueueExecuteCommandLists = NULL;
        g_zeCommandQueueSynchronize = NULL;
        g_zeKernelSetArgumentValue = NULL;
        g_zeKernelSetGroupSize = NULL;
        
        printf("[C198-DRIVER-LOADER] ✅ Driver déchargé\n");
    }
}

/*
 * Wrappers pour appeler les fonctions Level Zero chargées dynamiquement
 */

ze_result_t btc_l0_zeInit(ze_init_flags_t flags) {
    if (g_zeInit == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeInit non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeInit(flags);
}

ze_result_t btc_l0_zeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers) {
    if (g_zeDriverGet == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeDriverGet non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeDriverGet(pCount, phDrivers);
}

ze_result_t btc_l0_zeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount, ze_device_handle_t* phDevices) {
    if (g_zeDeviceGet == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeDeviceGet non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeDeviceGet(hDriver, pCount, phDevices);
}

ze_result_t btc_l0_zeDeviceGetProperties(ze_device_handle_t hDevice, ze_device_properties_t* pDeviceProperties) {
    if (g_zeDeviceGetProperties == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeDeviceGetProperties non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeDeviceGetProperties(hDevice, pDeviceProperties);
}

ze_result_t btc_l0_zeDeviceGetMemoryProperties(ze_device_handle_t hDevice, uint32_t* pCount, ze_device_memory_properties_t* pMemProperties) {
    if (g_zeDeviceGetMemoryProperties == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeDeviceGetMemoryProperties non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeDeviceGetMemoryProperties(hDevice, pCount, pMemProperties);
}

ze_result_t btc_l0_zeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc, ze_context_handle_t* phContext) {
    if (g_zeContextCreate == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeContextCreate non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeContextCreate(hDriver, desc, phContext);
}

ze_result_t btc_l0_zeModuleCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_module_desc_t* desc, ze_module_handle_t* phModule, ze_module_build_log_handle_t* phBuildLog) {
    if (g_zeModuleCreate == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeModuleCreate non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeModuleCreate(hContext, hDevice, desc, phModule, phBuildLog);
}

ze_result_t btc_l0_zeKernelCreate(ze_module_handle_t hModule, const ze_kernel_desc_t* desc, ze_kernel_handle_t* phKernel) {
    if (g_zeKernelCreate == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeKernelCreate non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeKernelCreate(hModule, desc, phKernel);
}

ze_result_t btc_l0_zeCommandListCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList) {
    if (g_zeCommandListCreate == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListCreate non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandListCreate(hContext, hDevice, desc, phCommandList);
}

ze_result_t btc_l0_zeCommandQueueCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* desc, ze_command_queue_handle_t* phCommandQueue) {
    if (g_zeCommandQueueCreate == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandQueueCreate non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandQueueCreate(hContext, hDevice, desc, phCommandQueue);
}

ze_result_t btc_l0_zeMemAllocDevice(ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr) {
    if (g_zeMemAllocDevice == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeMemAllocDevice non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeMemAllocDevice(hContext, device_desc, size, alignment, hDevice, pptr);
}

/* C198 Phase 12G : Wrapper pour allocation mémoire partagée UMA */
ze_result_t btc_l0_zeMemAllocShared(ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr) {
    if (g_zeMemAllocShared == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeMemAllocShared non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeMemAllocShared(hContext, device_desc, host_desc, size, alignment, hDevice, pptr);
}

ze_result_t btc_l0_zeMemFree(ze_context_handle_t hContext, void* ptr) {
    if (g_zeMemFree == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeMemFree non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeMemFree(hContext, ptr);
}

ze_result_t btc_l0_zeCommandListAppendMemoryCopy(ze_command_list_handle_t hCommandList, void* dstptr, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents) {
    if (g_zeCommandListAppendMemoryCopy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListAppendMemoryCopy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandListAppendMemoryCopy(hCommandList, dstptr, srcptr, size, hSignalEvent, numWaitEvents, phWaitEvents);
}

ze_result_t btc_l0_zeCommandListAppendLaunchKernel(ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents) {
    if (g_zeCommandListAppendLaunchKernel == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListAppendLaunchKernel non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandListAppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents);
}

ze_result_t btc_l0_zeCommandListClose(ze_command_list_handle_t hCommandList) {
    if (g_zeCommandListClose == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListClose non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandListClose(hCommandList);
}

ze_result_t btc_l0_zeCommandQueueExecuteCommandLists(ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence) {
    if (g_zeCommandQueueExecuteCommandLists == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandQueueExecuteCommandLists non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists, phCommandLists, hFence);
}

ze_result_t btc_l0_zeCommandQueueSynchronize(ze_command_queue_handle_t hCommandQueue, uint64_t timeout) {
    if (g_zeCommandQueueSynchronize == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandQueueSynchronize non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandQueueSynchronize(hCommandQueue, timeout);
}

ze_result_t btc_l0_zeKernelSetArgumentValue(ze_kernel_handle_t hKernel, uint32_t argIndex, size_t argSize, const void* pArgValue) {
    if (g_zeKernelSetArgumentValue == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeKernelSetArgumentValue non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeKernelSetArgumentValue(hKernel, argIndex, argSize, pArgValue);
}

ze_result_t btc_l0_zeKernelSetGroupSize(ze_kernel_handle_t hKernel, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) {
    if (g_zeKernelSetGroupSize == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeKernelSetGroupSize non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeKernelSetGroupSize(hKernel, groupSizeX, groupSizeY, groupSizeZ);
}

/* C198 Bug Fix : Wrappers destroy manquants */
/* C198 Phase 12F Bug Fix : Nullifier pointeurs après destroy */
ze_result_t btc_l0_zeContextDestroy(ze_context_handle_t *phContext) {
    if (g_zeContextDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeContextDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    if (phContext == NULL || *phContext == NULL) {
        return ZE_RESULT_SUCCESS;  /* Déjà détruit */
    }
    ze_result_t result = g_zeContextDestroy(*phContext);
    if (result == ZE_RESULT_SUCCESS) {
        *phContext = NULL;  /* Nullifier pour éviter use-after-free */
    }
    return result;
}

ze_result_t btc_l0_zeCommandQueueDestroy(ze_command_queue_handle_t *phCommandQueue) {
    if (g_zeCommandQueueDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandQueueDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    if (phCommandQueue == NULL || *phCommandQueue == NULL) {
        return ZE_RESULT_SUCCESS;
    }
    ze_result_t result = g_zeCommandQueueDestroy(*phCommandQueue);
    if (result == ZE_RESULT_SUCCESS) {
        *phCommandQueue = NULL;
    }
    return result;
}

ze_result_t btc_l0_zeCommandListReset(ze_command_list_handle_t hCommandList) {
    if (g_zeCommandListReset == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListReset non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeCommandListReset(hCommandList);
}

ze_result_t btc_l0_zeModuleBuildLogGetString(ze_module_build_log_handle_t hModuleBuildLog, size_t* pSize, char* pBuildLog) {
    if (g_zeModuleBuildLogGetString == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeModuleBuildLogGetString non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeModuleBuildLogGetString(hModuleBuildLog, pSize, pBuildLog);
}

ze_result_t btc_l0_zeModuleBuildLogDestroy(ze_module_build_log_handle_t hModuleBuildLog) {
    if (g_zeModuleBuildLogDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeModuleBuildLogDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    return g_zeModuleBuildLogDestroy(hModuleBuildLog);
}

ze_result_t btc_l0_zeKernelDestroy(ze_kernel_handle_t *phKernel) {
    if (g_zeKernelDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeKernelDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    if (phKernel == NULL || *phKernel == NULL) {
        return ZE_RESULT_SUCCESS;
    }
    ze_result_t result = g_zeKernelDestroy(*phKernel);
    if (result == ZE_RESULT_SUCCESS) {
        *phKernel = NULL;
    }
    return result;
}

ze_result_t btc_l0_zeModuleDestroy(ze_module_handle_t *phModule) {
    if (g_zeModuleDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeModuleDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    if (phModule == NULL || *phModule == NULL) {
        return ZE_RESULT_SUCCESS;
    }
    ze_result_t result = g_zeModuleDestroy(*phModule);
    if (result == ZE_RESULT_SUCCESS) {
        *phModule = NULL;
    }
    return result;
}

ze_result_t btc_l0_zeCommandListDestroy(ze_command_list_handle_t *phCommandList) {
    if (g_zeCommandListDestroy == NULL) {
        fprintf(stderr, "[C198-DRIVER-LOADER] ERREUR : zeCommandListDestroy non chargé\n");
        return ZE_RESULT_ERROR_UNINITIALIZED;
    }
    if (phCommandList == NULL || *phCommandList == NULL) {
        return ZE_RESULT_SUCCESS;
    }
    ze_result_t result = g_zeCommandListDestroy(*phCommandList);
    if (result == ZE_RESULT_SUCCESS) {
        *phCommandList = NULL;
    }
    return result;
}

// Made with Bob
