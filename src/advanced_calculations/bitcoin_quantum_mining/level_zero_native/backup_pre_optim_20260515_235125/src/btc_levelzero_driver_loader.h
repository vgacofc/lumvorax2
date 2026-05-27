/*
 * LumVorax Bitcoin Mining — Level Zero Driver Loader Manual (C198 Phase 10A)
 * 
 * Header pour chargement manuel du driver Level Zero via dlopen()
 */

#ifndef BTC_LEVELZERO_DRIVER_LOADER_H
#define BTC_LEVELZERO_DRIVER_LOADER_H

#include <level_zero/ze_api.h>

/*
 * Charge manuellement le driver Level Zero Intel
 * Retourne 0 si succès, -1 si échec
 */
int btc_l0_load_driver_manual(void);

/*
 * Décharge le driver Level Zero
 */
void btc_l0_unload_driver_manual(void);

/*
 * Wrappers pour appeler les fonctions Level Zero chargées dynamiquement
 * Ces fonctions remplacent les appels directs à zeXXX()
 */

ze_result_t btc_l0_zeInit(ze_init_flags_t flags);
ze_result_t btc_l0_zeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers);
ze_result_t btc_l0_zeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount, ze_device_handle_t* phDevices);
ze_result_t btc_l0_zeDeviceGetProperties(ze_device_handle_t hDevice, ze_device_properties_t* pDeviceProperties);
ze_result_t btc_l0_zeDeviceGetMemoryProperties(ze_device_handle_t hDevice, uint32_t* pCount, ze_device_memory_properties_t* pMemProperties);
ze_result_t btc_l0_zeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc, ze_context_handle_t* phContext);
ze_result_t btc_l0_zeModuleCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_module_desc_t* desc, ze_module_handle_t* phModule, ze_module_build_log_handle_t* phBuildLog);  /* C198 Phase 12E */
ze_result_t btc_l0_zeKernelCreate(ze_module_handle_t hModule, const ze_kernel_desc_t* desc, ze_kernel_handle_t* phKernel);  /* C198 Phase 12E */
ze_result_t btc_l0_zeCommandListCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList);
ze_result_t btc_l0_zeCommandQueueCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* desc, ze_command_queue_handle_t* phCommandQueue);
ze_result_t btc_l0_zeMemAllocDevice(ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr);
ze_result_t btc_l0_zeMemAllocShared(ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr);  /* C198 Phase 12G : Shared memory UMA */
ze_result_t btc_l0_zeMemFree(ze_context_handle_t hContext, void* ptr);
ze_result_t btc_l0_zeCommandListAppendMemoryCopy(ze_command_list_handle_t hCommandList, void* dstptr, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents);
ze_result_t btc_l0_zeCommandListAppendLaunchKernel(ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents);
ze_result_t btc_l0_zeCommandListClose(ze_command_list_handle_t hCommandList);
ze_result_t btc_l0_zeCommandQueueExecuteCommandLists(ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence);
ze_result_t btc_l0_zeCommandQueueSynchronize(ze_command_queue_handle_t hCommandQueue, uint64_t timeout);
ze_result_t btc_l0_zeKernelSetArgumentValue(ze_kernel_handle_t hKernel, uint32_t argIndex, size_t argSize, const void* pArgValue);
ze_result_t btc_l0_zeKernelSetGroupSize(ze_kernel_handle_t hKernel, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ);
/* C198 Phase 12F Bug Fix : Pointeurs nullifiés après destroy */
ze_result_t btc_l0_zeContextDestroy(ze_context_handle_t *phContext);
ze_result_t btc_l0_zeCommandQueueDestroy(ze_command_queue_handle_t *phCommandQueue);
ze_result_t btc_l0_zeCommandListDestroy(ze_command_list_handle_t *phCommandList);
ze_result_t btc_l0_zeModuleDestroy(ze_module_handle_t *phModule);
ze_result_t btc_l0_zeKernelDestroy(ze_kernel_handle_t *phKernel);
ze_result_t btc_l0_zeCommandListReset(ze_command_list_handle_t hCommandList);  /* C198 Bug Fix #9 */
ze_result_t btc_l0_zeModuleBuildLogGetString(ze_module_build_log_handle_t hModuleBuildLog, size_t* pSize, char* pBuildLog);  /* C198 Bug Fix #16 */
ze_result_t btc_l0_zeModuleBuildLogDestroy(ze_module_build_log_handle_t hModuleBuildLog);  /* C198 Bug Fix #17 */

#endif /* BTC_LEVELZERO_DRIVER_LOADER_H */

// Made with Bob
