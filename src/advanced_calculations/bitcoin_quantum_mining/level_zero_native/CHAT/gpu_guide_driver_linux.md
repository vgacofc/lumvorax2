drm/i915 Intel GFX Driver
The drm/i915 driver supports all (with the exception of some very early models) integrated GFX chipsets with both Intel display and rendering blocks. This excludes a set of SoC platforms with an SGX rendering unit, those have basic support through the gma500 drm driver.

Core Driver Infrastructure
This section covers core driver infrastructure used by both the display and the GEM parts of the driver.

Runtime Power Management
The i915 driver supports dynamic enabling and disabling of entire hardware blocks at runtime. This is especially important on the display side where software is supposed to control many power gates manually on recent hardware, since on the GT side a lot of the power management is done by the hardware. But even there some manual control at the device level is required.

Since i915 supports a diverse set of platforms with a unified codebase and hardware engineers just love to shuffle functionality around between power domains there’s a sizeable amount of indirection required. This file provides generic functions to the driver for grabbing and releasing references for abstract power domains. It then maps those to the actual power wells present for a given platform.

intel_wakeref_t intel_runtime_pm_get_raw(struct intel_runtime_pm *rpm)
grab a raw runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

Description

This is the unlocked version of intel_display_power_is_enabled() and should only be used from error capture and recovery code where deadlocks are possible. This function grabs a device-level runtime pm reference (mostly used for asynchronous PM management from display code) and ensures that it is powered up. Raw references are not considered during wakelock assert checks.

Any runtime pm reference obtained by this function must have a symmetric call to intel_runtime_pm_put_raw() to release the reference again.

Return

the wakeref cookie to pass to intel_runtime_pm_put_raw(), evaluates as True if the wakeref was acquired, or False otherwise.

intel_wakeref_t intel_runtime_pm_get(struct intel_runtime_pm *rpm)
grab a runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

Description

This function grabs a device-level runtime pm reference (mostly used for GEM code to ensure the GTT or GT is on) and ensures that it is powered up.

Any runtime pm reference obtained by this function must have a symmetric call to intel_runtime_pm_put() to release the reference again.

Return

the wakeref cookie to pass to intel_runtime_pm_put()

intel_wakeref_t __intel_runtime_pm_get_if_active(struct intel_runtime_pm *rpm, bool ignore_usecount)
grab a runtime pm reference if device is active

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

bool ignore_usecount
get a ref even if dev->power.usage_count is 0

Description

This function grabs a device-level runtime pm reference if the device is already active and ensures that it is powered up. It is illegal to try and access the HW should intel_runtime_pm_get_if_active() report failure.

If ignore_usecount is true, a reference will be acquired even if there is no user requiring the device to be powered up (dev->power.usage_count == 0). If the function returns false in this case then it’s guaranteed that the device’s runtime suspend hook has been called already or that it will be called (and hence it’s also guaranteed that the device’s runtime resume hook will be called eventually).

Any runtime pm reference obtained by this function must have a symmetric call to intel_runtime_pm_put() to release the reference again.

Return

the wakeref cookie to pass to intel_runtime_pm_put(), evaluates as True if the wakeref was acquired, or False otherwise.

intel_wakeref_t intel_runtime_pm_get_noresume(struct intel_runtime_pm *rpm)
grab a runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

Description

This function grabs a device-level runtime pm reference.

It will _not_ resume the device but instead only get an extra wakeref. Therefore it is only valid to call this functions from contexts where the device is known to be active and with another wakeref previously hold.

Any runtime pm reference obtained by this function must have a symmetric call to intel_runtime_pm_put() to release the reference again.

Return

the wakeref cookie to pass to intel_runtime_pm_put()

void intel_runtime_pm_put_raw(struct intel_runtime_pm *rpm, intel_wakeref_t wref)
release a raw runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

intel_wakeref_t wref
wakeref acquired for the reference that is being released

Description

This function drops the device-level runtime pm reference obtained by intel_runtime_pm_get_raw() and might power down the corresponding hardware block right away if this is the last reference.

void intel_runtime_pm_put_unchecked(struct intel_runtime_pm *rpm)
release an unchecked runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

Description

This function drops the device-level runtime pm reference obtained by intel_runtime_pm_get() and might power down the corresponding hardware block right away if this is the last reference.

This function exists only for historical reasons and should be avoided in new code, as the correctness of its use cannot be checked. Always use intel_runtime_pm_put() instead.

void intel_runtime_pm_put(struct intel_runtime_pm *rpm, intel_wakeref_t wref)
release a runtime pm reference

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

intel_wakeref_t wref
wakeref acquired for the reference that is being released

Description

This function drops the device-level runtime pm reference obtained by intel_runtime_pm_get() and might power down the corresponding hardware block right away if this is the last reference.

void intel_runtime_pm_enable(struct intel_runtime_pm *rpm)
enable runtime pm

Parameters

struct intel_runtime_pm *rpm
the intel_runtime_pm structure

Description

This function enables runtime pm at the end of the driver load sequence.

Note that this function does currently not enable runtime pm for the subordinate display power domains. That is done by intel_power_domains_enable().

void intel_uncore_forcewake_get(struct intel_uncore *uncore, enum forcewake_domains fw_domains)
grab forcewake domain references

Parameters

struct intel_uncore *uncore
the intel_uncore structure

enum forcewake_domains fw_domains
forcewake domains to get reference on

Description

This function can be used get GT’s forcewake domain references. Normal register access will handle the forcewake domains automatically. However if some sequence requires the GT to not power down a particular forcewake domains this function should be called at the beginning of the sequence. And subsequently the reference should be dropped by symmetric call to intel_unforce_forcewake_put(). Usually caller wants all the domains to be kept awake so the fw_domains would be then FORCEWAKE_ALL.

void intel_uncore_forcewake_user_get(struct intel_uncore *uncore)
claim forcewake on behalf of userspace

Parameters

struct intel_uncore *uncore
the intel_uncore structure

Description

This function is a wrapper around intel_uncore_forcewake_get() to acquire the GT powerwell and in the process disable our debugging for the duration of userspace’s bypass.

void intel_uncore_forcewake_user_put(struct intel_uncore *uncore)
release forcewake on behalf of userspace

Parameters

struct intel_uncore *uncore
the intel_uncore structure

Description

This function complements intel_uncore_forcewake_user_get() and releases the GT powerwell taken on behalf of the userspace bypass.

void intel_uncore_forcewake_get__locked(struct intel_uncore *uncore, enum forcewake_domains fw_domains)
grab forcewake domain references

Parameters

struct intel_uncore *uncore
the intel_uncore structure

enum forcewake_domains fw_domains
forcewake domains to get reference on

Description

See intel_uncore_forcewake_get(). This variant places the onus on the caller to explicitly handle the dev_priv->uncore.lock spinlock.

void intel_uncore_forcewake_put(struct intel_uncore *uncore, enum forcewake_domains fw_domains)
release a forcewake domain reference

Parameters

struct intel_uncore *uncore
the intel_uncore structure

enum forcewake_domains fw_domains
forcewake domains to put references

Description

This function drops the device-level forcewakes for specified domains obtained by intel_uncore_forcewake_get().

void intel_uncore_forcewake_flush(struct intel_uncore *uncore, enum forcewake_domains fw_domains)
flush the delayed release

Parameters

struct intel_uncore *uncore
the intel_uncore structure

enum forcewake_domains fw_domains
forcewake domains to flush

void intel_uncore_forcewake_put__locked(struct intel_uncore *uncore, enum forcewake_domains fw_domains)
release forcewake domain references

Parameters

struct intel_uncore *uncore
the intel_uncore structure

enum forcewake_domains fw_domains
forcewake domains to put references

Description

See intel_uncore_forcewake_put(). This variant places the onus on the caller to explicitly handle the dev_priv->uncore.lock spinlock.

int __intel_wait_for_register_fw(struct intel_uncore *uncore, i915_reg_t reg, u32 mask, u32 value, unsigned int fast_timeout_us, unsigned int slow_timeout_ms, u32 *out_value)
wait until register matches expected state

Parameters

struct intel_uncore *uncore
the struct intel_uncore

i915_reg_t reg
the register to read

u32 mask
mask to apply to register value

u32 value
expected value

unsigned int fast_timeout_us
fast timeout in microsecond for atomic/tight wait

unsigned int slow_timeout_ms
slow timeout in millisecond

u32 *out_value
optional placeholder to hold registry value

Description

This routine waits until the target register reg contains the expected value after applying the mask, i.e. it waits until

(intel_uncore_read_fw(uncore, reg) & mask) == value
Otherwise, the wait will timeout after slow_timeout_ms milliseconds. For atomic context slow_timeout_ms must be zero and fast_timeout_us must be not larger than 20,0000 microseconds.

Note that this routine assumes the caller holds forcewake asserted, it is not suitable for very long waits. See intel_wait_for_register() if you wish to wait without holding forcewake for the duration (i.e. you expect the wait to be slow).

Return

0 if the register matches the desired condition, or -ETIMEDOUT.

int __intel_wait_for_register(struct intel_uncore *uncore, i915_reg_t reg, u32 mask, u32 value, unsigned int fast_timeout_us, unsigned int slow_timeout_ms, u32 *out_value)
wait until register matches expected state

Parameters

struct intel_uncore *uncore
the struct intel_uncore

i915_reg_t reg
the register to read

u32 mask
mask to apply to register value

u32 value
expected value

unsigned int fast_timeout_us
fast timeout in microsecond for atomic/tight wait

unsigned int slow_timeout_ms
slow timeout in millisecond

u32 *out_value
optional placeholder to hold registry value

Description

This routine waits until the target register reg contains the expected value after applying the mask, i.e. it waits until

(intel_uncore_read(uncore, reg) & mask) == value
Otherwise, the wait will timeout after timeout_ms milliseconds.

Return

0 if the register matches the desired condition, or -ETIMEDOUT.

enum forcewake_domains intel_uncore_forcewake_for_reg(struct intel_uncore *uncore, i915_reg_t reg, unsigned int op)
which forcewake domains are needed to access a register

Parameters

struct intel_uncore *uncore
pointer to struct intel_uncore

i915_reg_t reg
register in question

unsigned int op
operation bitmask of FW_REG_READ and/or FW_REG_WRITE

Description

Returns a set of forcewake domains required to be taken with for example intel_uncore_forcewake_get for the specified register to be accessible in the specified mode (read, write or read/write) with raw mmio accessors.

NOTE

On Gen6 and Gen7 write forcewake domain (FORCEWAKE_RENDER) requires the callers to do FIFO management on their own or risk losing writes.

Interrupt Handling
These functions provide the basic support for enabling and disabling the interrupt handling support. There’s a lot more functionality in i915_irq.c and related files, but that will be described in separate chapters.

void intel_irq_init(struct drm_i915_private *dev_priv)
initializes irq support

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function initializes all the irq support including work items, timers and all the vtables. It does not setup the interrupt itself though.

void intel_runtime_pm_disable_interrupts(struct drm_i915_private *dev_priv)
runtime interrupt disabling

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function is used to disable interrupts at runtime, both in the runtime pm and the system suspend/resume code.

void intel_runtime_pm_enable_interrupts(struct drm_i915_private *dev_priv)
runtime interrupt enabling

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function is used to enable interrupts at runtime, both in the runtime pm and the system suspend/resume code.

Intel GVT-g Guest Support(vGPU)
Intel GVT-g is a graphics virtualization technology which shares the GPU among multiple virtual machines on a time-sharing basis. Each virtual machine is presented a virtual GPU (vGPU), which has equivalent features as the underlying physical GPU (pGPU), so i915 driver can run seamlessly in a virtual machine. This file provides vGPU specific optimizations when running in a virtual machine, to reduce the complexity of vGPU emulation and to improve the overall performance.

A primary function introduced here is so-called “address space ballooning” technique. Intel GVT-g partitions global graphics memory among multiple VMs, so each VM can directly access a portion of the memory without hypervisor’s intervention, e.g. filling textures or queuing commands. However with the partitioning an unmodified i915 driver would assume a smaller graphics memory starting from address ZERO, then requires vGPU emulation module to translate the graphics address between ‘guest view’ and ‘host view’, for all registers and command opcodes which contain a graphics memory address. To reduce the complexity, Intel GVT-g introduces “address space ballooning”, by telling the exact partitioning knowledge to each guest i915 driver, which then reserves and prevents non-allocated portions from allocation. Thus vGPU emulation module only needs to scan and validate graphics addresses without complexity of address translation.

void intel_vgpu_detect(struct drm_i915_private *dev_priv)
detect virtual GPU

Parameters

struct drm_i915_private *dev_priv
i915 device private

Description

This function is called at the initialization stage, to detect whether running on a vGPU.

void intel_vgt_deballoon(struct i915_ggtt *ggtt)
deballoon reserved graphics address trunks

Parameters

struct i915_ggtt *ggtt
the global GGTT from which we reserved earlier

Description

This function is called to deallocate the ballooned-out graphic memory, when driver is unloaded or when ballooning fails.

int intel_vgt_balloon(struct i915_ggtt *ggtt)
balloon out reserved graphics address trunks

Parameters

struct i915_ggtt *ggtt
the global GGTT from which to reserve

Description

This function is called at the initialization stage, to balloon out the graphic address space allocated to other vGPUs, by marking these spaces as reserved. The ballooning related knowledge(starting address and size of the mappable/unmappable graphic memory) is described in the vgt_if structure in a reserved mmio range.

To give an example, the drawing below depicts one typical scenario after ballooning. Here the vGPU1 has 2 pieces of graphic address spaces ballooned out each for the mappable and the non-mappable part. From the vGPU1 point of view, the total size is the same as the physical one, with the start address of its graphic space being zero. Yet there are some portions ballooned out( the shadow part, which are marked as reserved by drm allocator). From the host point of view, the graphic address space is partitioned by multiple vGPUs in different VMs.

                       vGPU1 view         Host view
            0 ------> +-----------+     +-----------+
              ^       |###########|     |   vGPU3   |
              |       |###########|     +-----------+
              |       |###########|     |   vGPU2   |
              |       +-----------+     +-----------+
       mappable GM    | available | ==> |   vGPU1   |
              |       +-----------+     +-----------+
              |       |###########|     |           |
              v       |###########|     |   Host    |
              +=======+===========+     +===========+
              ^       |###########|     |   vGPU3   |
              |       |###########|     +-----------+
              |       |###########|     |   vGPU2   |
              |       +-----------+     +-----------+
     unmappable GM    | available | ==> |   vGPU1   |
              |       +-----------+     +-----------+
              |       |###########|     |           |
              |       |###########|     |   Host    |
              v       |###########|     |           |
total GM size ------> +-----------+     +-----------+
Return

zero on success, non-zero if configuration invalid or ballooning failed

Intel GVT-g Host Support(vGPU device model)
Intel GVT-g is a graphics virtualization technology which shares the GPU among multiple virtual machines on a time-sharing basis. Each virtual machine is presented a virtual GPU (vGPU), which has equivalent features as the underlying physical GPU (pGPU), so i915 driver can run seamlessly in a virtual machine.

To virtualize GPU resources GVT-g driver depends on hypervisor technology e.g KVM/VFIO/mdev, Xen, etc. to provide resource access trapping capability and be virtualized within GVT-g device module. More architectural design doc is available on https://github.com/intel/gvt-linux/wiki.

int intel_gvt_init(struct drm_i915_private *dev_priv)
initialize GVT components

Parameters

struct drm_i915_private *dev_priv
drm i915 private data

Description

This function is called at the initialization stage to create a GVT device.

Return

Zero on success, negative error code if failed.

void intel_gvt_driver_remove(struct drm_i915_private *dev_priv)
cleanup GVT components when i915 driver is unbinding

Parameters

struct drm_i915_private *dev_priv
drm i915 private *

Description

This function is called at the i915 driver unloading stage, to shutdown GVT components and release the related resources.

void intel_gvt_resume(struct drm_i915_private *dev_priv)
GVT resume routine wapper

Parameters

struct drm_i915_private *dev_priv
drm i915 private *

Description

This function is called at the i915 driver resume stage to restore required HW status for GVT so that vGPU can continue running after resumed.

Workarounds
Hardware workarounds are register programming documented to be executed in the driver that fall outside of the normal programming sequences for a platform. There are some basic categories of workarounds, depending on how/when they are applied:

Context workarounds: workarounds that touch registers that are saved/restored to/from the HW context image. The list is emitted (via Load Register Immediate commands) once when initializing the device and saved in the default context. That default context is then used on every context creation to have a “primed golden context”, i.e. a context image that already contains the changes needed to all the registers.

Context workarounds should be implemented in the *_ctx_workarounds_init() variants respective to the targeted platforms.

Engine workarounds: the list of these WAs is applied whenever the specific engine is reset. It’s also possible that a set of engine classes share a common power domain and they are reset together. This happens on some platforms with render and compute engines. In this case (at least) one of them need to keeep the workaround programming: the approach taken in the driver is to tie those workarounds to the first compute/render engine that is registered. When executing with GuC submission, engine resets are outside of kernel driver control, hence the list of registers involved in written once, on engine initialization, and then passed to GuC, that saves/restores their values before/after the reset takes place. See drivers/gpu/drm/i915/gt/uc/intel_guc_ads.c for reference.

Workarounds for registers specific to RCS and CCS should be implemented in rcs_engine_wa_init() and ccs_engine_wa_init(), respectively; those for registers belonging to BCS, VCS or VECS should be implemented in xcs_engine_wa_init(). Workarounds for registers not belonging to a specific engine’s MMIO range but that are part of of the common RCS/CCS reset domain should be implemented in general_render_compute_wa_init(). The settings about the CCS load balancing should be added in ccs_engine_wa_mode().

GT workarounds: the list of these WAs is applied whenever these registers revert to their default values: on GPU reset, suspend/resume [1], etc.

GT workarounds should be implemented in the *_gt_workarounds_init() variants respective to the targeted platforms.

Register whitelist: some workarounds need to be implemented in userspace, but need to touch privileged registers. The whitelist in the kernel instructs the hardware to allow the access to happen. From the kernel side, this is just a special case of a MMIO workaround (as we write the list of these to/be-whitelisted registers to some special HW registers).

Register whitelisting should be done in the *_whitelist_build() variants respective to the targeted platforms.

Workaround batchbuffers: buffers that get executed automatically by the hardware on every HW context restore. These buffers are created and programmed in the default context so the hardware always go through those programming sequences when switching contexts. The support for workaround batchbuffers is enabled these hardware mechanisms:

INDIRECT_CTX: A batchbuffer and an offset are provided in the default context, pointing the hardware to jump to that location when that offset is reached in the context restore. Workaround batchbuffer in the driver currently uses this mechanism for all platforms.

BB_PER_CTX_PTR: A batchbuffer is provided in the default context, pointing the hardware to a buffer to continue executing after the engine registers are restored in a context restore sequence. This is currently not used in the driver.

Other: There are WAs that, due to their nature, cannot be applied from a central place. Those are peppered around the rest of the code, as needed. Workarounds related to the display IP are the main example.

[1]
Technically, some registers are powercontext saved & restored, so they survive a suspend/resume. In practice, writing them again is not too costly and simplifies things, so it’s the approach taken in the driver.

Display Hardware Handling
This section covers everything related to the display hardware including the mode setting infrastructure, plane, sprite and cursor handling and display, output probing and related topics.

Mode Setting Infrastructure
The i915 driver is thus far the only DRM driver which doesn’t use the common DRM helper code to implement mode setting sequences. Thus it has its own tailor-made infrastructure for executing a display configuration change.

Frontbuffer Tracking
Many features require us to track changes to the currently active frontbuffer, especially rendering targeted at the frontbuffer.

To be able to do so we track frontbuffers using a bitmask for all possible frontbuffer slots through intel_frontbuffer_track(). The functions in this file are then called when the contents of the frontbuffer are invalidated, when frontbuffer rendering has stopped again to flush out all the changes and when the frontbuffer is exchanged with a flip. Subsystems interested in frontbuffer changes (e.g. PSR, FBC, DRRS) should directly put their callbacks into the relevant places and filter for the frontbuffer slots that they are interested int.

On a high level there are two types of powersaving features. The first one work like a special cache (FBC and PSR) and are interested when they should stop caching and when to restart caching. This is done by placing callbacks into the invalidate and the flush functions: At invalidate the caching must be stopped and at flush time it can be restarted. And maybe they need to know when the frontbuffer changes (e.g. when the hw doesn’t initiate an invalidate and flush on its own) which can be achieved with placing callbacks into the flip functions.

The other type of display power saving feature only cares about busyness (e.g. DRRS). In that case all three (invalidate, flush and flip) indicate busyness. There is no direct way to detect idleness. Instead an idle timer work delayed work should be started from the flush and flip functions and cancelled as soon as busyness is detected.

bool intel_frontbuffer_invalidate(struct intel_frontbuffer *front, enum fb_op_origin origin)
invalidate frontbuffer object

Parameters

struct intel_frontbuffer *front
GEM object to invalidate

enum fb_op_origin origin
which operation caused the invalidation

Description

This function gets called every time rendering on the given object starts and frontbuffer caching (fbc, low refresh rate for DRRS, panel self refresh) must be invalidated. For ORIGIN_CS any subsequent invalidation will be delayed until the rendering completes or a flip on this frontbuffer plane is scheduled.

void intel_frontbuffer_flush(struct intel_frontbuffer *front, enum fb_op_origin origin)
flush frontbuffer object

Parameters

struct intel_frontbuffer *front
GEM object to flush

enum fb_op_origin origin
which operation caused the flush

Description

This function gets called every time rendering on the given object has completed and frontbuffer caching can be started again.

void frontbuffer_flush(struct drm_i915_private *i915, unsigned int frontbuffer_bits, enum fb_op_origin origin)
flush frontbuffer

Parameters

struct drm_i915_private *i915
i915 device

unsigned int frontbuffer_bits
frontbuffer plane tracking bits

enum fb_op_origin origin
which operation caused the flush

Description

This function gets called every time rendering on the given planes has completed and frontbuffer caching can be started again. Flushes will get delayed if they’re blocked by some outstanding asynchronous rendering.

Can be called without any locks held.

void intel_frontbuffer_flip_prepare(struct drm_i915_private *i915, unsigned frontbuffer_bits)
prepare asynchronous frontbuffer flip

Parameters

struct drm_i915_private *i915
i915 device

unsigned frontbuffer_bits
frontbuffer plane tracking bits

Description

This function gets called after scheduling a flip on obj. The actual frontbuffer flushing will be delayed until completion is signalled with intel_frontbuffer_flip_complete. If an invalidate happens in between this flush will be cancelled.

Can be called without any locks held.

void intel_frontbuffer_flip_complete(struct drm_i915_private *i915, unsigned frontbuffer_bits)
complete asynchronous frontbuffer flip

Parameters

struct drm_i915_private *i915
i915 device

unsigned frontbuffer_bits
frontbuffer plane tracking bits

Description

This function gets called after the flip has been latched and will complete on the next vblank. It will execute the flush if it hasn’t been cancelled yet.

Can be called without any locks held.

void intel_frontbuffer_flip(struct drm_i915_private *i915, unsigned frontbuffer_bits)
synchronous frontbuffer flip

Parameters

struct drm_i915_private *i915
i915 device

unsigned frontbuffer_bits
frontbuffer plane tracking bits

Description

This function gets called after scheduling a flip on obj. This is for synchronous plane updates which will happen on the next vblank and which will not get delayed by pending gpu rendering.

Can be called without any locks held.

void intel_frontbuffer_queue_flush(struct intel_frontbuffer *front)
queue flushing frontbuffer object

Parameters

struct intel_frontbuffer *front
GEM object to flush

Description

This function is targeted for our dirty callback for queueing flush when dma fence is signales

void intel_frontbuffer_track(struct intel_frontbuffer *old, struct intel_frontbuffer *new, unsigned int frontbuffer_bits)
update frontbuffer tracking

Parameters

struct intel_frontbuffer *old
current buffer for the frontbuffer slots

struct intel_frontbuffer *new
new buffer for the frontbuffer slots

unsigned int frontbuffer_bits
bitmask of frontbuffer slots

Description

This updates the frontbuffer tracking bits frontbuffer_bits by clearing them from old and setting them in new. Both old and new can be NULL.

Display FIFO Underrun Reporting
The i915 driver checks for display fifo underruns using the interrupt signals provided by the hardware. This is enabled by default and fairly useful to debug display issues, especially watermark settings.

If an underrun is detected this is logged into dmesg. To avoid flooding logs and occupying the cpu underrun interrupts are disabled after the first occurrence until the next modeset on a given pipe.

Note that underrun detection on gmch platforms is a bit more ugly since there is no interrupt (despite that the signalling bit is in the PIPESTAT pipe interrupt register). Also on some other platforms underrun interrupts are shared, which means that if we detect an underrun we need to disable underrun reporting on all pipes.

The code also supports underrun detection on the PCH transcoder.

bool intel_set_cpu_fifo_underrun_reporting(struct drm_i915_private *dev_priv, enum pipe pipe, bool enable)
set cpu fifo underrrun reporting state

Parameters

struct drm_i915_private *dev_priv
i915 device instance

enum pipe pipe
(CPU) pipe to set state for

bool enable
whether underruns should be reported or not

Description

This function sets the fifo underrun state for pipe. It is used in the modeset code to avoid false positives since on many platforms underruns are expected when disabling or enabling the pipe.

Notice that on some platforms disabling underrun reports for one pipe disables for all due to shared interrupts. Actual reporting is still per-pipe though.

Returns the previous state of underrun reporting.

bool intel_set_pch_fifo_underrun_reporting(struct drm_i915_private *dev_priv, enum pipe pch_transcoder, bool enable)
set PCH fifo underrun reporting state

Parameters

struct drm_i915_private *dev_priv
i915 device instance

enum pipe pch_transcoder
the PCH transcoder (same as pipe on IVB and older)

bool enable
whether underruns should be reported or not

Description

This function makes us disable or enable PCH fifo underruns for a specific PCH transcoder. Notice that on some PCHs (e.g. CPT/PPT), disabling FIFO underrun reporting for one transcoder may also disable all the other PCH error interruts for the other transcoders, due to the fact that there’s just one interrupt mask/enable bit for all the transcoders.

Returns the previous state of underrun reporting.

void intel_cpu_fifo_underrun_irq_handler(struct drm_i915_private *dev_priv, enum pipe pipe)
handle CPU fifo underrun interrupt

Parameters

struct drm_i915_private *dev_priv
i915 device instance

enum pipe pipe
(CPU) pipe to set state for

Description

This handles a CPU fifo underrun interrupt, generating an underrun warning into dmesg if underrun reporting is enabled and then disables the underrun interrupt to avoid an irq storm.

void intel_pch_fifo_underrun_irq_handler(struct drm_i915_private *dev_priv, enum pipe pch_transcoder)
handle PCH fifo underrun interrupt

Parameters

struct drm_i915_private *dev_priv
i915 device instance

enum pipe pch_transcoder
the PCH transcoder (same as pipe on IVB and older)

Description

This handles a PCH fifo underrun interrupt, generating an underrun warning into dmesg if underrun reporting is enabled and then disables the underrun interrupt to avoid an irq storm.

void intel_check_cpu_fifo_underruns(struct drm_i915_private *dev_priv)
check for CPU fifo underruns immediately

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

Check for CPU fifo underruns immediately. Useful on IVB/HSW where the shared error interrupt may have been disabled, and so CPU fifo underruns won’t necessarily raise an interrupt, and on GMCH platforms where underruns never raise an interrupt.

void intel_check_pch_fifo_underruns(struct drm_i915_private *dev_priv)
check for PCH fifo underruns immediately

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

Check for PCH fifo underruns immediately. Useful on CPT/PPT where the shared error interrupt may have been disabled, and so PCH fifo underruns won’t necessarily raise an interrupt.

Plane Configuration
This section covers plane configuration and composition with the primary plane, sprites, cursors and overlays. This includes the infrastructure to do atomic vsync’ed updates of all this state and also tightly coupled topics like watermark setup and computation, framebuffer compression and panel self refresh.

Atomic Plane Helpers
The functions here are used by the atomic plane helper functions to implement legacy plane updates (i.e., drm_plane->update_plane() and drm_plane->disable_plane()). This allows plane updates to use the atomic state infrastructure and perform plane updates as separate prepare/check/commit/cleanup steps.

struct drm_plane_state *intel_plane_duplicate_state(struct drm_plane *plane)
duplicate plane state

Parameters

struct drm_plane *plane
drm plane

Description

Allocates and returns a copy of the plane state (both common and Intel-specific) for the specified plane.

Return

The newly allocated plane state, or NULL on failure.

void intel_plane_destroy_state(struct drm_plane *plane, struct drm_plane_state *state)
destroy plane state

Parameters

struct drm_plane *plane
drm plane

struct drm_plane_state *state
state object to destroy

Description

Destroys the plane state (both common and Intel-specific) for the specified plane.

int intel_prepare_plane_fb(struct drm_plane *_plane, struct drm_plane_state *_new_plane_state)
Prepare fb for usage on plane

Parameters

struct drm_plane *_plane
drm plane to prepare for

struct drm_plane_state *_new_plane_state
the plane state being prepared

Description

Prepares a framebuffer for usage on a display plane. Generally this involves pinning the underlying object and updating the frontbuffer tracking bits. Some older platforms need special physical address handling for cursor planes.

Returns 0 on success, negative error code on failure.

void intel_cleanup_plane_fb(struct drm_plane *plane, struct drm_plane_state *_old_plane_state)
Cleans up an fb after plane use

Parameters

struct drm_plane *plane
drm plane to clean up for

struct drm_plane_state *_old_plane_state
the state from the previous modeset

Description

Cleans up a framebuffer that has just been removed from a plane.

Asynchronous Page Flip
Asynchronous page flip is the implementation for the DRM_MODE_PAGE_FLIP_ASYNC flag. Currently async flip is only supported via the drmModePageFlip IOCTL. Correspondingly, support is currently added for primary plane only.

Async flip can only change the plane surface address, so anything else changing is rejected from the intel_async_flip_check_hw() function. Once this check is cleared, flip done interrupt is enabled using the intel_crtc_enable_flip_done() function.

As soon as the surface address register is written, flip done interrupt is generated and the requested events are sent to the usersapce in the interrupt handler itself. The timestamp and sequence sent during the flip done event correspond to the last vblank and have no relation to the actual time when the flip done event was sent.

Output Probing
This section covers output probing and related infrastructure like the hotplug interrupt storm detection and mitigation code. Note that the i915 driver still uses most of the common DRM helper code for output probing, so those sections fully apply.

Hotplug
Simply put, hotplug occurs when a display is connected to or disconnected from the system. However, there may be adapters and docking stations and Display Port short pulses and MST devices involved, complicating matters.

Hotplug in i915 is handled in many different levels of abstraction.

The platform dependent interrupt handling code in i915_irq.c enables, disables, and does preliminary handling of the interrupts. The interrupt handlers gather the hotplug detect (HPD) information from relevant registers into a platform independent mask of hotplug pins that have fired.

The platform independent interrupt handler intel_hpd_irq_handler() in intel_hotplug.c does hotplug irq storm detection and mitigation, and passes further processing to appropriate bottom halves (Display Port specific and regular hotplug).

The Display Port work function i915_digport_work_func() calls into intel_dp_hpd_pulse() via hooks, which handles DP short pulses and DP MST long pulses, with failures and non-MST long pulses triggering regular hotplug processing on the connector.

The regular hotplug work function i915_hotplug_work_func() calls connector detect hooks, and, if connector status changes, triggers sending of hotplug uevent to userspace via drm_kms_helper_hotplug_event().

Finally, the userspace is responsible for triggering a modeset upon receiving the hotplug uevent, disabling or enabling the crtc as needed.

The hotplug interrupt storm detection and mitigation code keeps track of the number of interrupts per hotplug pin per a period of time, and if the number of interrupts exceeds a certain threshold, the interrupt is disabled for a while before being re-enabled. The intention is to mitigate issues raising from broken hardware triggering massive amounts of interrupts and grinding the system to a halt.

Current implementation expects that hotplug interrupt storm will not be seen when display port sink is connected, hence on platforms whose DP callback is handled by i915_digport_work_func reenabling of hpd is not performed (it was never expected to be disabled in the first place ;) ) this is specific to DP sinks handled by this routine and any other display such as HDMI or DVI enabled on the same port will have proper logic since it will use i915_hotplug_work_func where this logic is handled.

enum hpd_pin intel_hpd_pin_default(struct drm_i915_private *dev_priv, enum port port)
return default pin associated with certain port.

Parameters

struct drm_i915_private *dev_priv
private driver data pointer

enum port port
the hpd port to get associated pin

Description

It is only valid and used by digital port encoder.

Return pin that is associatade with port.

bool intel_hpd_irq_storm_detect(struct drm_i915_private *dev_priv, enum hpd_pin pin, bool long_hpd)
gather stats and detect HPD IRQ storm on a pin

Parameters

struct drm_i915_private *dev_priv
private driver data pointer

enum hpd_pin pin
the pin to gather stats on

bool long_hpd
whether the HPD IRQ was long or short

Description

Gather stats about HPD IRQs from the specified pin, and detect IRQ storms. Only the pin specific stats and state are changed, the caller is responsible for further action.

The number of IRQs that are allowed within HPD_STORM_DETECT_PERIOD is stored in dev_priv->display.hotplug.hpd_storm_threshold which defaults to HPD_STORM_DEFAULT_THRESHOLD. Long IRQs count as +10 to this threshold, and short IRQs count as +1. If this threshold is exceeded, it’s considered an IRQ storm and the IRQ state is set to HPD_MARK_DISABLED.

By default, most systems will only count long IRQs towards dev_priv->display.hotplug.hpd_storm_threshold. However, some older systems also suffer from short IRQ storms and must also track these. Because short IRQ storms are naturally caused by sideband interactions with DP MST devices, short IRQ detection is only enabled for systems without DP MST support. Systems which are new enough to support DP MST are far less likely to suffer from IRQ storms at all, so this is fine.

The HPD threshold can be controlled through i915_hpd_storm_ctl in debugfs, and should only be adjusted for automated hotplug testing.

Return true if an IRQ storm was detected on pin.

void intel_hpd_trigger_irq(struct intel_digital_port *dig_port)
trigger an hpd irq event for a port

Parameters

struct intel_digital_port *dig_port
digital port

Description

Trigger an HPD interrupt event for the given port, emulating a short pulse generated by the sink, and schedule the dig port work to handle it.

void intel_hpd_irq_handler(struct drm_i915_private *dev_priv, u32 pin_mask, u32 long_mask)
main hotplug irq handler

Parameters

struct drm_i915_private *dev_priv
drm_i915_private

u32 pin_mask
a mask of hpd pins that have triggered the irq

u32 long_mask
a mask of hpd pins that may be long hpd pulses

Description

This is the main hotplug irq handler for all platforms. The platform specific irq handlers call the platform specific hotplug irq handlers, which read and decode the appropriate registers into bitmasks about hpd pins that have triggered (pin_mask), and which of those pins may be long pulses (long_mask). The long_mask is ignored if the port corresponding to the pin is not a digital port.

Here, we do hotplug irq storm detection and mitigation, and pass further processing to appropriate bottom halves.

void intel_hpd_init(struct drm_i915_private *dev_priv)
initializes and enables hpd support

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function enables the hotplug support. It requires that interrupts have already been enabled with intel_irq_init_hw(). From this point on hotplug and poll request can run concurrently to other code, so locking rules must be obeyed.

This is a separate step from interrupt enabling to simplify the locking rules in the driver load and resume code.

Also see: intel_hpd_poll_enable() and intel_hpd_poll_disable().

void intel_hpd_poll_enable(struct drm_i915_private *dev_priv)
enable polling for connectors with hpd

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function enables polling for all connectors which support HPD. Under certain conditions HPD may not be functional. On most Intel GPUs, this happens when we enter runtime suspend. On Valleyview and Cherryview systems, this also happens when we shut off all of the powerwells.

Since this function can get called in contexts where we’re already holding dev->mode_config.mutex, we do the actual hotplug enabling in a seperate worker.

Also see: intel_hpd_init() and intel_hpd_poll_disable().

void intel_hpd_poll_disable(struct drm_i915_private *dev_priv)
disable polling for connectors with hpd

Parameters

struct drm_i915_private *dev_priv
i915 device instance

Description

This function disables polling for all connectors which support HPD. Under certain conditions HPD may not be functional. On most Intel GPUs, this happens when we enter runtime suspend. On Valleyview and Cherryview systems, this also happens when we shut off all of the powerwells.

Since this function can get called in contexts where we’re already holding dev->mode_config.mutex, we do the actual hotplug enabling in a seperate worker.

Also used during driver init to initialize connector->polled appropriately for all connectors.

Also see: intel_hpd_init() and intel_hpd_poll_enable().

High Definition Audio
The graphics and audio drivers together support High Definition Audio over HDMI and Display Port. The audio programming sequences are divided into audio codec and controller enable and disable sequences. The graphics driver handles the audio codec sequences, while the audio driver handles the audio controller sequences.

The disable sequences must be performed before disabling the transcoder or port. The enable sequences may only be performed after enabling the transcoder and port, and after completed link training. Therefore the audio enable/disable sequences are part of the modeset sequence.

The codec and controller sequences could be done either parallel or serial, but generally the ELDV/PD change in the codec sequence indicates to the audio driver that the controller sequence should start. Indeed, most of the co-operation between the graphics and audio drivers is handled via audio related registers. (The notable exception is the power management, not covered here.)

The struct i915_audio_component is used to interact between the graphics and audio drivers. The struct i915_audio_component_ops ops in it is defined in graphics driver and called in audio driver. The struct i915_audio_component_audio_ops audio_ops is called from i915 driver.

void intel_audio_codec_enable(struct intel_encoder *encoder, const struct intel_crtc_state *crtc_state, const struct drm_connector_state *conn_state)
Enable the audio codec for HD audio

Parameters

struct intel_encoder *encoder
encoder on which to enable audio

const struct intel_crtc_state *crtc_state
pointer to the current crtc state.

const struct drm_connector_state *conn_state
pointer to the current connector state.

Description

The enable sequences may only be performed after enabling the transcoder and port, and after completed link training.

void intel_audio_codec_disable(struct intel_encoder *encoder, const struct intel_crtc_state *old_crtc_state, const struct drm_connector_state *old_conn_state)
Disable the audio codec for HD audio

Parameters

struct intel_encoder *encoder
encoder on which to disable audio

const struct intel_crtc_state *old_crtc_state
pointer to the old crtc state.

const struct drm_connector_state *old_conn_state
pointer to the old connector state.

Description

The disable sequences must be performed before disabling the transcoder or port.

void intel_audio_hooks_init(struct drm_i915_private *i915)
Set up chip specific audio hooks

Parameters

struct drm_i915_private *i915
device private

void i915_audio_component_init(struct drm_i915_private *i915)
initialize and register the audio component

Parameters

struct drm_i915_private *i915
i915 device instance

Description

This will register with the component framework a child component which will bind dynamically to the snd_hda_intel driver’s corresponding master component when the latter is registered. During binding the child initializes an instance of struct i915_audio_component which it receives from the master. The master can then start to use the interface defined by this struct. Each side can break the binding at any point by deregistering its own component after which each side’s component unbind callback is called.

We ignore any error during registration and continue with reduced functionality (i.e. without HDMI audio).

void i915_audio_component_cleanup(struct drm_i915_private *i915)
deregister the audio component

Parameters

struct drm_i915_private *i915
i915 device instance

Description

Deregisters the audio component, breaking any existing binding to the corresponding snd_hda_intel driver’s master component.

void intel_audio_init(struct drm_i915_private *i915)
Initialize the audio driver either using component framework or using lpe audio bridge

Parameters

struct drm_i915_private *i915
the i915 drm device private data

void intel_audio_deinit(struct drm_i915_private *i915)
deinitialize the audio driver

Parameters

struct drm_i915_private *i915
the i915 drm device private data

struct i915_audio_component
Used for direct communication between i915 and hda drivers

Definition:

struct i915_audio_component {
    struct drm_audio_component      base;
    int aud_sample_rate[MAX_PORTS];
};
Members

base
the drm_audio_component base class

aud_sample_rate
the array of audio sample rate per port

Intel HDMI LPE Audio Support
Motivation: Atom platforms (e.g. valleyview and cherryTrail) integrates a DMA-based interface as an alternative to the traditional HDaudio path. While this mode is unrelated to the LPE aka SST audio engine, the documentation refers to this mode as LPE so we keep this notation for the sake of consistency.

The interface is handled by a separate standalone driver maintained in the ALSA subsystem for simplicity. To minimize the interaction between the two subsystems, a bridge is setup between the hdmi-lpe-audio and i915: 1. Create a platform device to share MMIO/IRQ resources 2. Make the platform device child of i915 device for runtime PM. 3. Create IRQ chip to forward the LPE audio irqs. the hdmi-lpe-audio driver probes the lpe audio device and creates a new sound card

Threats: Due to the restriction in Linux platform device model, user need manually uninstall the hdmi-lpe-audio driver before uninstalling i915 module, otherwise we might run into use-after-free issues after i915 removes the platform device: even though hdmi-lpe-audio driver is released, the modules is still in “installed” status.

Implementation: The MMIO/REG platform resources are created according to the registers specification. When forwarding LPE audio irqs, the flow control handler selection depends on the platform, for example on valleyview handle_simple_irq is enough.

void intel_lpe_audio_irq_handler(struct drm_i915_private *dev_priv)
forwards the LPE audio irq

Parameters

struct drm_i915_private *dev_priv
the i915 drm device private data

Description

the LPE Audio irq is forwarded to the irq handler registered by LPE audio driver.

int intel_lpe_audio_init(struct drm_i915_private *dev_priv)
detect and setup the bridge between HDMI LPE Audio driver and i915

Parameters

struct drm_i915_private *dev_priv
the i915 drm device private data

Return

0 if successful. non-zero if detection or llocation/initialization fails

void intel_lpe_audio_teardown(struct drm_i915_private *dev_priv)
destroy the bridge between HDMI LPE audio driver and i915

Parameters

struct drm_i915_private *dev_priv
the i915 drm device private data

Description

release all the resources for LPE audio <-> i915 bridge.

void intel_lpe_audio_notify(struct drm_i915_private *dev_priv, enum transcoder cpu_transcoder, enum port port, const void *eld, int ls_clock, bool dp_output)
notify lpe audio event audio driver and i915

Parameters

struct drm_i915_private *dev_priv
the i915 drm device private data

enum transcoder cpu_transcoder
CPU transcoder

enum port port
port

const void *eld
ELD data

int ls_clock
Link symbol clock in kHz

bool dp_output
Driving a DP output?

Description

Notify lpe audio driver of eld change.

Panel Self Refresh PSR (PSR/SRD)
Since Haswell Display controller supports Panel Self-Refresh on display panels witch have a remote frame buffer (RFB) implemented according to PSR spec in eDP1.3. PSR feature allows the display to go to lower standby states when system is idle but display is on as it eliminates display refresh request to DDR memory completely as long as the frame buffer for that display is unchanged.

Panel Self Refresh must be supported by both Hardware (source) and Panel (sink).

PSR saves power by caching the framebuffer in the panel RFB, which allows us to power down the link and memory controller. For DSI panels the same idea is called “manual mode”.

The implementation uses the hardware-based PSR support which automatically enters/exits self-refresh mode. The hardware takes care of sending the required DP aux message and could even retrain the link (that part isn’t enabled yet though). The hardware also keeps track of any frontbuffer changes to know when to exit self-refresh mode again. Unfortunately that part doesn’t work too well, hence why the i915 PSR support uses the software frontbuffer tracking to make sure it doesn’t miss a screen update. For this integration intel_psr_invalidate() and intel_psr_flush() get called by the frontbuffer tracking code. Note that because of locking issues the self-refresh re-enable code is done from a work queue, which must be correctly synchronized/cancelled when shutting down the pipe.”

DC3CO (DC3 clock off)

On top of PSR2, GEN12 adds a intermediate power savings state that turns clock off automatically during PSR2 idle state. The smaller overhead of DC3co entry/exit vs. the overhead of PSR2 deep sleep entry/exit allows the HW to enter a low-power state even when page flipping periodically (for instance a 30fps video playback scenario).

Every time a flips occurs PSR2 will get out of deep sleep state(if it was), so DC3CO is enabled and tgl_dc3co_disable_work is schedule to run after 6 frames, if no other flip occurs and the function above is executed, DC3CO is disabled and PSR2 is configured to enter deep sleep, resetting again in case of another flip. Front buffer modifications do not trigger DC3CO activation on purpose as it would bring a lot of complexity and most of the moderns systems will only use page flips.

void intel_psr_disable(struct intel_dp *intel_dp, const struct intel_crtc_state *old_crtc_state)
Disable PSR

Parameters

struct intel_dp *intel_dp
Intel DP

const struct intel_crtc_state *old_crtc_state
old CRTC state

Description

This function needs to be called before disabling pipe.

void intel_psr_pause(struct intel_dp *intel_dp)
Pause PSR

Parameters

struct intel_dp *intel_dp
Intel DP

Description

This function need to be called after enabling psr.

void intel_psr_resume(struct intel_dp *intel_dp)
Resume PSR

Parameters

struct intel_dp *intel_dp
Intel DP

Description

This function need to be called after pausing psr.

void intel_psr_wait_for_idle_locked(const struct intel_crtc_state *new_crtc_state)
wait for PSR be ready for a pipe update

Parameters

const struct intel_crtc_state *new_crtc_state
new CRTC state

Description

This function is expected to be called from pipe_update_start() where it is not expected to race with PSR enable or disable.

void intel_psr_invalidate(struct intel_display *display, unsigned frontbuffer_bits, enum fb_op_origin origin)
Invalidate PSR

Parameters

struct intel_display *display
display device

unsigned frontbuffer_bits
frontbuffer plane tracking bits

enum fb_op_origin origin
which operation caused the invalidate

Description

Since the hardware frontbuffer tracking has gaps we need to integrate with the software frontbuffer tracking. This function gets called every time frontbuffer rendering starts and a buffer gets dirtied. PSR must be disabled if the frontbuffer mask contains a buffer relevant to PSR.

Dirty frontbuffers relevant to PSR are tracked in busy_frontbuffer_bits.”

void intel_psr_flush(struct intel_display *display, unsigned frontbuffer_bits, enum fb_op_origin origin)
Flush PSR

Parameters

struct intel_display *display
display device

unsigned frontbuffer_bits
frontbuffer plane tracking bits

enum fb_op_origin origin
which operation caused the flush

Description

Since the hardware frontbuffer tracking has gaps we need to integrate with the software frontbuffer tracking. This function gets called every time frontbuffer rendering has completed and flushed out to memory. PSR can be enabled again if no other frontbuffer relevant to PSR is dirty.

Dirty frontbuffers relevant to PSR are tracked in busy_frontbuffer_bits.

void intel_psr_init(struct intel_dp *intel_dp)
Init basic PSR work and mutex.

Parameters

struct intel_dp *intel_dp
Intel DP

Description

This function is called after the initializing connector. (the initializing of connector treats the handling of connector capabilities) And it initializes basic PSR stuff for each DP Encoder.

void intel_psr_lock(const struct intel_crtc_state *crtc_state)
grab PSR lock

Parameters

const struct intel_crtc_state *crtc_state
the crtc state

Description

This is initially meant to be used by around CRTC update, when vblank sensitive registers are updated and we need grab the lock before it to avoid vblank evasion.

void intel_psr_unlock(const struct intel_crtc_state *crtc_state)
release PSR lock

Parameters

const struct intel_crtc_state *crtc_state
the crtc state

Description

Release the PSR lock that was held during pipe update.

Frame Buffer Compression (FBC)
FBC tries to save memory bandwidth (and so power consumption) by compressing the amount of memory used by the display. It is total transparent to user space and completely handled in the kernel.

The benefits of FBC are mostly visible with solid backgrounds and variation-less patterns. It comes from keeping the memory footprint small and having fewer memory pages opened and accessed for refreshing the display.

i915 is responsible to reserve stolen memory for FBC and configure its offset on proper registers. The hardware takes care of all compress/decompress. However there are many known cases where we have to forcibly disable it to allow proper screen updates.

void intel_fbc_disable(struct intel_crtc *crtc)
disable FBC if it’s associated with crtc

Parameters

struct intel_crtc *crtc
the CRTC

Description

This function disables FBC if it’s associated with the provided CRTC.

void intel_fbc_handle_fifo_underrun_irq(struct intel_display *display)
disable FBC when we get a FIFO underrun

Parameters

struct intel_display *display
display

Description

Without FBC, most underruns are harmless and don’t really cause too many problems, except for an annoying message on dmesg. With FBC, underruns can become black screens or even worse, especially when paired with bad watermarks. So in order for us to be on the safe side, completely disable FBC in case we ever detect a FIFO underrun on any pipe. An underrun on any pipe already suggests that watermarks may be bad, so try to be as safe as possible.

This function is called from the IRQ handler.

void intel_fbc_init(struct intel_display *display)
Initialize FBC

Parameters

struct intel_display *display
display

Description

This function might be called during PM init process.

void intel_fbc_sanitize(struct intel_display *display)
Sanitize FBC

Parameters

struct intel_display *display
display

Description

Make sure FBC is initially disabled since we have no idea eg. into which parts of stolen it might be scribbling into.

Display Refresh Rate Switching (DRRS)
Display Refresh Rate Switching (DRRS) is a power conservation feature which enables swtching between low and high refresh rates, dynamically, based on the usage scenario. This feature is applicable for internal panels.

Indication that the panel supports DRRS is given by the panel EDID, which would list multiple refresh rates for one resolution.

DRRS is of 2 types - static and seamless. Static DRRS involves changing refresh rate (RR) by doing a full modeset (may appear as a blink on screen) and is used in dock-undock scenario. Seamless DRRS involves changing RR without any visual effect to the user and can be used during normal system usage. This is done by programming certain registers.

Support for static/seamless DRRS may be indicated in the VBT based on inputs from the panel spec.

DRRS saves power by switching to low RR based on usage scenarios.

The implementation is based on frontbuffer tracking implementation. When there is a disturbance on the screen triggered by user activity or a periodic system activity, DRRS is disabled (RR is changed to high RR). When there is no movement on screen, after a timeout of 1 second, a switch to low RR is made.

For integration with frontbuffer tracking code, intel_drrs_invalidate() and intel_drrs_flush() are called.

DRRS can be further extended to support other internal panels and also the scenario of video playback wherein RR is set based on the rate requested by userspace.

void intel_drrs_activate(const struct intel_crtc_state *crtc_state)
activate DRRS

Parameters

const struct intel_crtc_state *crtc_state
the crtc state

Description

Activates DRRS on the crtc.

void intel_drrs_deactivate(const struct intel_crtc_state *old_crtc_state)
deactivate DRRS

Parameters

const struct intel_crtc_state *old_crtc_state
the old crtc state

Description

Deactivates DRRS on the crtc.

void intel_drrs_invalidate(struct drm_i915_private *dev_priv, unsigned int frontbuffer_bits)
Disable Idleness DRRS

Parameters

struct drm_i915_private *dev_priv
i915 device

unsigned int frontbuffer_bits
frontbuffer plane tracking bits

Description

This function gets called everytime rendering on the given planes start. Hence DRRS needs to be Upclocked, i.e. (LOW_RR -> HIGH_RR).

Dirty frontbuffers relevant to DRRS are tracked in busy_frontbuffer_bits.

void intel_drrs_flush(struct drm_i915_private *dev_priv, unsigned int frontbuffer_bits)
Restart Idleness DRRS

Parameters

struct drm_i915_private *dev_priv
i915 device

unsigned int frontbuffer_bits
frontbuffer plane tracking bits

Description

This function gets called every time rendering on the given planes has completed or flip on a crtc is completed. So DRRS should be upclocked (LOW_RR -> HIGH_RR). And also Idleness detection should be started again, if no other planes are dirty.

Dirty frontbuffers relevant to DRRS are tracked in busy_frontbuffer_bits.

void intel_drrs_crtc_init(struct intel_crtc *crtc)
Init DRRS for CRTC

Parameters

struct intel_crtc *crtc
crtc

Description

This function is called only once at driver load to initialize basic DRRS stuff.

DPIO
VLV, CHV and BXT have slightly peculiar display PHYs for driving DP/HDMI ports. DPIO is the name given to such a display PHY. These PHYs don’t follow the standard programming model using direct MMIO registers, and instead their registers must be accessed trough IOSF sideband. VLV has one such PHY for driving ports B and C, and CHV adds another PHY for driving port D. Each PHY responds to specific IOSF-SB port.

Each display PHY is made up of one or two channels. Each channel houses a common lane part which contains the PLL and other common logic. CH0 common lane also contains the IOSF-SB logic for the Common Register Interface (CRI) ie. the DPIO registers. CRI clock must be running when any DPIO registers are accessed.

In addition to having their own registers, the PHYs are also controlled through some dedicated signals from the display controller. These include PLL reference clock enable, PLL enable, and CRI clock selection, for example.

Eeach channel also has two splines (also called data lanes), and each spline is made up of one Physical Access Coding Sub-Layer (PCS) block and two TX lanes. So each channel has two PCS blocks and four TX lanes. The TX lanes are used as DP lanes or TMDS data/clock pairs depending on the output type.

Additionally the PHY also contains an AUX lane with AUX blocks for each channel. This is used for DP AUX communication, but this fact isn’t really relevant for the driver since AUX is controlled from the display controller side. No DPIO registers need to be accessed during AUX communication,

Generally on VLV/CHV the common lane corresponds to the pipe and the spline (PCS/TX) corresponds to the port.

For dual channel PHY (VLV/CHV):

pipe A == CMN/PLL/REF CH0

pipe B == CMN/PLL/REF CH1

port B == PCS/TX CH0

port C == PCS/TX CH1

This is especially important when we cross the streams ie. drive port B with pipe B, or port C with pipe A.

For single channel PHY (CHV):

pipe C == CMN/PLL/REF CH0

port D == PCS/TX CH0

On BXT the entire PHY channel corresponds to the port. That means the PLL is also now associated with the port rather than the pipe, and so the clock needs to be routed to the appropriate transcoder. Port A PLL is directly connected to transcoder EDP and port B/C PLLs can be routed to any transcoder A/B/C.

Note: DDI0 is digital port B, DD1 is digital port C, and DDI2 is digital port D (CHV) or port A (BXT).

Dual channel PHY (VLV/CHV/BXT)
---------------------------------
|      CH0      |      CH1      |
|  CMN/PLL/REF  |  CMN/PLL/REF  |
|---------------|---------------| Display PHY
| PCS01 | PCS23 | PCS01 | PCS23 |
|-------|-------|-------|-------|
|TX0|TX1|TX2|TX3|TX0|TX1|TX2|TX3|
---------------------------------
|     DDI0      |     DDI1      | DP/HDMI ports
---------------------------------

Single channel PHY (CHV/BXT)
-----------------
|      CH0      |
|  CMN/PLL/REF  |
|---------------| Display PHY
| PCS01 | PCS23 |
|-------|-------|
|TX0|TX1|TX2|TX3|
-----------------
|     DDI2      | DP/HDMI port
-----------------
DMC Firmware Support
From gen9 onwards we have newly added DMC (Display microcontroller) in display engine to save and restore the state of display engine when it enter into low-power state and comes back to normal.

void intel_dmc_load_program(struct drm_i915_private *i915)
write the firmware from memory to register.

Parameters

struct drm_i915_private *i915
i915 drm device.

Description

DMC firmware is read from a .bin file and kept in internal memory one time. Everytime display comes back from low power state this function is called to copy the firmware from internal memory to registers.

void intel_dmc_disable_program(struct drm_i915_private *i915)
disable the firmware

Parameters

struct drm_i915_private *i915
i915 drm device

Description

Disable all event handlers in the firmware, making sure the firmware is inactive after the display is uninitialized.

void intel_dmc_init(struct drm_i915_private *i915)
initialize the firmware loading.

Parameters

struct drm_i915_private *i915
i915 drm device.

Description

This function is called at the time of loading the display driver to read firmware from a .bin file and copied into a internal memory.

void intel_dmc_suspend(struct drm_i915_private *i915)
prepare DMC firmware before system suspend

Parameters

struct drm_i915_private *i915
i915 drm device

Description

Prepare the DMC firmware before entering system suspend. This includes flushing pending work items and releasing any resources acquired during init.

void intel_dmc_resume(struct drm_i915_private *i915)
init DMC firmware during system resume

Parameters

struct drm_i915_private *i915
i915 drm device

Description

Reinitialize the DMC firmware during system resume, reacquiring any resources released in intel_dmc_suspend().

void intel_dmc_fini(struct drm_i915_private *i915)
unload the DMC firmware.

Parameters

struct drm_i915_private *i915
i915 drm device.

Description

Firmmware unloading includes freeing the internal memory and reset the firmware loading status.

DMC wakelock support
Wake lock is the mechanism to cause display engine to exit DC states to allow programming to registers that are powered down in those states. Previous projects exited DC states automatically when detecting programming. Now software controls the exit by programming the wake lock. This improves system performance and system interactions and better fits the flip queue style of programming. Wake lock is only required when DC5, DC6, or DC6v have been enabled in DC_STATE_EN and the wake lock mode of operation has been enabled.

The wakelock mechanism in DMC allows the display engine to exit DC states explicitly before programming registers that may be powered down. In earlier hardware, this was done automatically and implicitly when the display engine accessed a register. With the wakelock implementation, the driver asserts a wakelock in DMC, which forces it to exit the DC state until the wakelock is deasserted.

The mechanism can be enabled and disabled by writing to the DMC_WAKELOCK_CFG register. There are also 13 control registers that can be used to hold and release different wakelocks. In the current implementation, we only need one wakelock, so only DMC_WAKELOCK1_CTL is used. The other definitions are here for potential future use.

Video BIOS Table (VBT)
The Video BIOS Table, or VBT, provides platform and board specific configuration information to the driver that is not discoverable or available through other means. The configuration is mostly related to display hardware. The VBT is available via the ACPI OpRegion or, on older systems, in the PCI ROM.

The VBT consists of a VBT Header (defined as struct vbt_header), a BDB Header (struct bdb_header), and a number of BIOS Data Blocks (BDB) that contain the actual configuration information. The VBT Header, and thus the VBT, begins with “$VBT” signature. The VBT Header contains the offset of the BDB Header. The data blocks are concatenated after the BDB Header. The data blocks have a 1-byte Block ID, 2-byte Block Size, and Block Size bytes of data. (Block 53, the MIPI Sequence Block is an exception.)

The driver parses the VBT during load. The relevant information is stored in driver private data for ease of use, and the actual VBT is not read after that.

bool intel_bios_is_valid_vbt(struct intel_display *display, const void *buf, size_t size)
does the given buffer contain a valid VBT

Parameters

struct intel_display *display
display device

const void *buf
pointer to a buffer to validate

size_t size
size of the buffer

Description

Returns true on valid VBT.

void intel_bios_init(struct intel_display *display)
find VBT and initialize settings from the BIOS

Parameters

struct intel_display *display
display device instance

Description

Parse and initialize settings from the Video BIOS Tables (VBT). If the VBT was not found in ACPI OpRegion, try to find it in PCI ROM first. Also initialize some defaults if the VBT is not present at all.

void intel_bios_driver_remove(struct intel_display *display)
Free any resources allocated by intel_bios_init()

Parameters

struct intel_display *display
display device instance

bool intel_bios_is_tv_present(struct intel_display *display)
is integrated TV present in VBT

Parameters

struct intel_display *display
display device instance

Description

Return true if TV is present. If no child devices were parsed from VBT, assume TV is present.

bool intel_bios_is_lvds_present(struct intel_display *display, u8 *i2c_pin)
is LVDS present in VBT

Parameters

struct intel_display *display
display device instance

u8 *i2c_pin
i2c pin for LVDS if present

Description

Return true if LVDS is present. If no child devices were parsed from VBT, assume LVDS is present.

bool intel_bios_is_port_present(struct intel_display *display, enum port port)
is the specified digital port present

Parameters

struct intel_display *display
display device instance

enum port port
port to check

Description

Return true if the device in port is present.

bool intel_bios_is_dsi_present(struct intel_display *display, enum port *port)
is DSI present in VBT

Parameters

struct intel_display *display
display device instance

enum port *port
port for DSI if present

Description

Return true if DSI is present, and return the port in port.

struct vbt_header
VBT Header structure

Definition:

struct vbt_header {
    u8 signature[20];
    u16 version;
    u16 header_size;
    u16 vbt_size;
    u8 vbt_checksum;
    u8 reserved0;
    u32 bdb_offset;
    u32 aim_offset[4];
};
Members

signature
VBT signature, always starts with “$VBT”

version
Version of this structure

header_size
Size of this structure

vbt_size
Size of VBT (VBT Header, BDB Header and data blocks)

vbt_checksum
Checksum

reserved0
Reserved

bdb_offset
Offset of struct bdb_header from beginning of VBT

aim_offset
Offsets of add-in data blocks from beginning of VBT

struct bdb_header
BDB Header structure

Definition:

struct bdb_header {
    u8 signature[16];
    u16 version;
    u16 header_size;
    u16 bdb_size;
};
Members

signature
BDB signature “BIOS_DATA_BLOCK”

version
Version of the data block definitions

header_size
Size of this structure

bdb_size
Size of BDB (BDB Header and data blocks)

Display clocks
The display engine uses several different clocks to do its work. There are two main clocks involved that aren’t directly related to the actual pixel clock or any symbol/bit clock of the actual output port. These are the core display clock (CDCLK) and RAWCLK.

CDCLK clocks most of the display pipe logic, and thus its frequency must be high enough to support the rate at which pixels are flowing through the pipes. Downscaling must also be accounted as that increases the effective pixel rate.

On several platforms the CDCLK frequency can be changed dynamically to minimize power consumption for a given display configuration. Typically changes to the CDCLK frequency require all the display pipes to be shut down while the frequency is being changed.

On SKL+ the DMC will toggle the CDCLK off/on during DC5/6 entry/exit. DMC will not change the active CDCLK frequency however, so that part will still be performed by the driver directly.

There are multiple components involved in the generation of the CDCLK frequency:

We have the CDCLK PLL, which generates an output clock based on a reference clock and a ratio parameter.

The CD2X Divider, which divides the output of the PLL based on a divisor selected from a set of pre-defined choices.

The CD2X Squasher, which further divides the output based on a waveform represented as a sequence of bits where each zero “squashes out” a clock cycle.

And, finally, a fixed divider that divides the output frequency by 2.

As such, the resulting CDCLK frequency can be calculated with the following formula:

cdclk = vco / cd2x_div / (sq_len / sq_div) / 2

, where vco is the frequency generated by the PLL; cd2x_div represents the CD2X Divider; sq_len and sq_div are the bit length and the number of high bits for the CD2X Squasher waveform, respectively; and 2 represents the fixed divider.

Note that some older platforms do not contain the CD2X Divider and/or CD2X Squasher, in which case we can ignore their respective factors in the formula above.

Several methods exist to change the CDCLK frequency, which ones are supported depends on the platform:

Full PLL disable + re-enable with new VCO frequency. Pipes must be inactive.

CD2X divider update. Single pipe can be active as the divider update can be synchronized with the pipe’s start of vblank.

Crawl the PLL smoothly to the new VCO frequency. Pipes can be active.

Squash waveform update. Pipes can be active.

Crawl and squash can also be done back to back. Pipes can be active.

RAWCLK is a fixed frequency clock, often used by various auxiliary blocks such as AUX CH or backlight PWM. Hence the only thing we really need to know about RAWCLK is its frequency so that various dividers can be programmed correctly.

void intel_cdclk_init_hw(struct drm_i915_private *i915)
Initialize CDCLK hardware

Parameters

struct drm_i915_private *i915
i915 device

Description

Initialize CDCLK. This consists mainly of initializing dev_priv->display.cdclk.hw and sanitizing the state of the hardware if needed. This is generally done only during the display core initialization sequence, after which the DMC will take care of turning CDCLK off/on as needed.

void intel_cdclk_uninit_hw(struct drm_i915_private *i915)
Uninitialize CDCLK hardware

Parameters

struct drm_i915_private *i915
i915 device

Description

Uninitialize CDCLK. This is done only during the display core uninitialization sequence.

bool intel_cdclk_clock_changed(const struct intel_cdclk_config *a, const struct intel_cdclk_config *b)
Check whether the clock changed

Parameters

const struct intel_cdclk_config *a
first CDCLK configuration

const struct intel_cdclk_config *b
second CDCLK configuration

Return

True if CDCLK changed in a way that requires re-programming and False otherwise.

bool intel_cdclk_can_cd2x_update(struct drm_i915_private *dev_priv, const struct intel_cdclk_config *a, const struct intel_cdclk_config *b)
Determine if changing between the two CDCLK configurations requires only a cd2x divider update

Parameters

struct drm_i915_private *dev_priv
i915 device

const struct intel_cdclk_config *a
first CDCLK configuration

const struct intel_cdclk_config *b
second CDCLK configuration

Return

True if changing between the two CDCLK configurations can be done with just a cd2x divider update, false if not.

bool intel_cdclk_changed(const struct intel_cdclk_config *a, const struct intel_cdclk_config *b)
Determine if two CDCLK configurations are different

Parameters

const struct intel_cdclk_config *a
first CDCLK configuration

const struct intel_cdclk_config *b
second CDCLK configuration

Return

True if the CDCLK configurations don’t match, false if they do.

void intel_set_cdclk_pre_plane_update(struct intel_atomic_state *state)
Push the CDCLK state to the hardware

Parameters

struct intel_atomic_state *state
intel atomic state

Description

Program the hardware before updating the HW plane state based on the new CDCLK state, if necessary.

void intel_set_cdclk_post_plane_update(struct intel_atomic_state *state)
Push the CDCLK state to the hardware

Parameters

struct intel_atomic_state *state
intel atomic state

Description

Program the hardware after updating the HW plane state based on the new CDCLK state, if necessary.

void intel_update_max_cdclk(struct drm_i915_private *dev_priv)
Determine the maximum support CDCLK frequency

Parameters

struct drm_i915_private *dev_priv
i915 device

Description

Determine the maximum CDCLK frequency the platform supports, and also derive the maximum dot clock frequency the maximum CDCLK frequency allows.

void intel_update_cdclk(struct drm_i915_private *dev_priv)
Determine the current CDCLK frequency

Parameters

struct drm_i915_private *dev_priv
i915 device

Description

Determine the current CDCLK frequency.

u32 intel_read_rawclk(struct drm_i915_private *dev_priv)
Determine the current RAWCLK frequency

Parameters

struct drm_i915_private *dev_priv
i915 device

Description

Determine the current RAWCLK frequency. RAWCLK is a fixed frequency clock so this needs to done only once.

void intel_init_cdclk_hooks(struct drm_i915_private *dev_priv)
Initialize CDCLK related modesetting hooks

Parameters

struct drm_i915_private *dev_priv
i915 device

Display PLLs
Display PLLs used for driving outputs vary by platform. While some have per-pipe or per-encoder dedicated PLLs, others allow the use of any PLL from a pool. In the latter scenario, it is possible that multiple pipes share a PLL if their configurations match.

This file provides an abstraction over display PLLs. The function intel_shared_dpll_init() initializes the PLLs for the given platform. The users of a PLL are tracked and that tracking is integrated with the atomic modset interface. During an atomic operation, required PLLs can be reserved for a given CRTC and encoder configuration by calling intel_reserve_shared_dplls() and previously reserved PLLs can be released with intel_release_shared_dplls(). Changes to the users are first staged in the atomic state, and then made effective by calling intel_shared_dpll_swap_state() during the atomic commit phase.

struct intel_shared_dpll *intel_get_shared_dpll_by_id(struct drm_i915_private *i915, enum intel_dpll_id id)
get a DPLL given its id

Parameters

struct drm_i915_private *i915
i915 device instance

enum intel_dpll_id id
pll id

Return

A pointer to the DPLL with id

void intel_enable_shared_dpll(const struct intel_crtc_state *crtc_state)
enable a CRTC’s shared DPLL

Parameters

const struct intel_crtc_state *crtc_state
CRTC, and its state, which has a shared DPLL

Description

Enable the shared DPLL used by crtc.

void intel_disable_shared_dpll(const struct intel_crtc_state *crtc_state)
disable a CRTC’s shared DPLL

Parameters

const struct intel_crtc_state *crtc_state
CRTC, and its state, which has a shared DPLL

Description

Disable the shared DPLL used by crtc.

void intel_reference_shared_dpll_crtc(const struct intel_crtc *crtc, const struct intel_shared_dpll *pll, struct intel_shared_dpll_state *shared_dpll_state)
Get a DPLL reference for a CRTC

Parameters

const struct intel_crtc *crtc
CRTC on which behalf the reference is taken

const struct intel_shared_dpll *pll
DPLL for which the reference is taken

struct intel_shared_dpll_state *shared_dpll_state
the DPLL atomic state in which the reference is tracked

Description

Take a reference for pll tracking the use of it by crtc.

void intel_unreference_shared_dpll_crtc(const struct intel_crtc *crtc, const struct intel_shared_dpll *pll, struct intel_shared_dpll_state *shared_dpll_state)
Drop a DPLL reference for a CRTC

Parameters

const struct intel_crtc *crtc
CRTC on which behalf the reference is dropped

const struct intel_shared_dpll *pll
DPLL for which the reference is dropped

struct intel_shared_dpll_state *shared_dpll_state
the DPLL atomic state in which the reference is tracked

Description

Drop a reference for pll tracking the end of use of it by crtc.

void intel_shared_dpll_swap_state(struct intel_atomic_state *state)
make atomic DPLL configuration effective

Parameters

struct intel_atomic_state *state
atomic state

Description

This is the dpll version of drm_atomic_helper_swap_state() since the helper does not handle driver-specific global state.

For consistency with atomic helpers this function does a complete swap, i.e. it also puts the current state into state, even though there is no need for that at this moment.

void icl_set_active_port_dpll(struct intel_crtc_state *crtc_state, enum icl_port_dpll_id port_dpll_id)
select the active port DPLL for a given CRTC

Parameters

struct intel_crtc_state *crtc_state
state for the CRTC to select the DPLL for

enum icl_port_dpll_id port_dpll_id
the active port_dpll_id to select

Description

Select the given port_dpll_id instance from the DPLLs reserved for the CRTC.

void intel_shared_dpll_init(struct drm_i915_private *i915)
Initialize shared DPLLs

Parameters

struct drm_i915_private *i915
i915 device

Description

Initialize shared DPLLs for i915.

int intel_compute_shared_dplls(struct intel_atomic_state *state, struct intel_crtc *crtc, struct intel_encoder *encoder)
compute DPLL state CRTC and encoder combination

Parameters

struct intel_atomic_state *state
atomic state

struct intel_crtc *crtc
CRTC to compute DPLLs for

struct intel_encoder *encoder
encoder

Description

This function computes the DPLL state for the given CRTC and encoder.

The new configuration in the atomic commit state is made effective by calling intel_shared_dpll_swap_state().

Return

0 on success, negative error code on falure.

int intel_reserve_shared_dplls(struct intel_atomic_state *state, struct intel_crtc *crtc, struct intel_encoder *encoder)
reserve DPLLs for CRTC and encoder combination

Parameters

struct intel_atomic_state *state
atomic state

struct intel_crtc *crtc
CRTC to reserve DPLLs for

struct intel_encoder *encoder
encoder

Description

This function reserves all required DPLLs for the given CRTC and encoder combination in the current atomic commit state and the new crtc atomic state.

The new configuration in the atomic commit state is made effective by calling intel_shared_dpll_swap_state().

The reserved DPLLs should be released by calling intel_release_shared_dplls().

Return

0 if all required DPLLs were successfully reserved, negative error code otherwise.

void intel_release_shared_dplls(struct intel_atomic_state *state, struct intel_crtc *crtc)
end use of DPLLs by CRTC in atomic state

Parameters

struct intel_atomic_state *state
atomic state

struct intel_crtc *crtc
crtc from which the DPLLs are to be released

Description

This function releases all DPLLs reserved by intel_reserve_shared_dplls() from the current atomic commit state and the old crtc atomic state.

The new configuration in the atomic commit state is made effective by calling intel_shared_dpll_swap_state().

void intel_update_active_dpll(struct intel_atomic_state *state, struct intel_crtc *crtc, struct intel_encoder *encoder)
update the active DPLL for a CRTC/encoder

Parameters

struct intel_atomic_state *state
atomic state

struct intel_crtc *crtc
the CRTC for which to update the active DPLL

struct intel_encoder *encoder
encoder determining the type of port DPLL

Description

Update the active DPLL for the given crtc/encoder in crtc’s atomic state, from the port DPLLs reserved previously by intel_reserve_shared_dplls(). The DPLL selected will be based on the current mode of the encoder’s port.

int intel_dpll_get_freq(struct drm_i915_private *i915, const struct intel_shared_dpll *pll, const struct intel_dpll_hw_state *dpll_hw_state)
calculate the DPLL’s output frequency

Parameters

struct drm_i915_private *i915
i915 device

const struct intel_shared_dpll *pll
DPLL for which to calculate the output frequency

const struct intel_dpll_hw_state *dpll_hw_state
DPLL state from which to calculate the output frequency

Description

Return the output frequency corresponding to pll’s passed in dpll_hw_state.

bool intel_dpll_get_hw_state(struct drm_i915_private *i915, struct intel_shared_dpll *pll, struct intel_dpll_hw_state *dpll_hw_state)
readout the DPLL’s hardware state

Parameters

struct drm_i915_private *i915
i915 device

struct intel_shared_dpll *pll
DPLL for which to calculate the output frequency

struct intel_dpll_hw_state *dpll_hw_state
DPLL’s hardware state

Description

Read out pll’s hardware state into dpll_hw_state.

void intel_dpll_dump_hw_state(struct drm_i915_private *i915, struct drm_printer *p, const struct intel_dpll_hw_state *dpll_hw_state)
dump hw_state

Parameters

struct drm_i915_private *i915
i915 drm device

struct drm_printer *p
where to print the state to

const struct intel_dpll_hw_state *dpll_hw_state
hw state to be dumped

Description

Dumo out the relevant values in dpll_hw_state.

bool intel_dpll_compare_hw_state(struct drm_i915_private *i915, const struct intel_dpll_hw_state *a, const struct intel_dpll_hw_state *b)
compare the two states

Parameters

struct drm_i915_private *i915
i915 drm device

const struct intel_dpll_hw_state *a
first DPLL hw state

const struct intel_dpll_hw_state *b
second DPLL hw state

Description

Compare DPLL hw states a and b.

Return

true if the states are equal, false if the differ

enum intel_dpll_id
possible DPLL ids

Constants

DPLL_ID_PRIVATE
non-shared dpll in use

DPLL_ID_PCH_PLL_A
DPLL A in ILK, SNB and IVB

DPLL_ID_PCH_PLL_B
DPLL B in ILK, SNB and IVB

DPLL_ID_WRPLL1
HSW and BDW WRPLL1

DPLL_ID_WRPLL2
HSW and BDW WRPLL2

DPLL_ID_SPLL
HSW and BDW SPLL

DPLL_ID_LCPLL_810
HSW and BDW 0.81 GHz LCPLL

DPLL_ID_LCPLL_1350
HSW and BDW 1.35 GHz LCPLL

DPLL_ID_LCPLL_2700
HSW and BDW 2.7 GHz LCPLL

DPLL_ID_SKL_DPLL0
SKL and later DPLL0

DPLL_ID_SKL_DPLL1
SKL and later DPLL1

DPLL_ID_SKL_DPLL2
SKL and later DPLL2

DPLL_ID_SKL_DPLL3
SKL and later DPLL3

DPLL_ID_ICL_DPLL0
ICL/TGL combo PHY DPLL0

DPLL_ID_ICL_DPLL1
ICL/TGL combo PHY DPLL1

DPLL_ID_EHL_DPLL4
EHL combo PHY DPLL4

DPLL_ID_ICL_TBTPLL
ICL/TGL TBT PLL

DPLL_ID_ICL_MGPLL1
ICL MG PLL 1 port 1 (C),
TGL TC PLL 1 port 1 (TC1)

DPLL_ID_ICL_MGPLL2
ICL MG PLL 1 port 2 (D)
TGL TC PLL 1 port 2 (TC2)

DPLL_ID_ICL_MGPLL3
ICL MG PLL 1 port 3 (E)
TGL TC PLL 1 port 3 (TC3)

DPLL_ID_ICL_MGPLL4
ICL MG PLL 1 port 4 (F)
TGL TC PLL 1 port 4 (TC4)

DPLL_ID_TGL_MGPLL5
TGL TC PLL port 5 (TC5)

DPLL_ID_TGL_MGPLL6
TGL TC PLL port 6 (TC6)

DPLL_ID_DG1_DPLL0
DG1 combo PHY DPLL0

DPLL_ID_DG1_DPLL1
DG1 combo PHY DPLL1

DPLL_ID_DG1_DPLL2
DG1 combo PHY DPLL2

DPLL_ID_DG1_DPLL3
DG1 combo PHY DPLL3

Description

Enumeration of possible IDs for a DPLL. Real shared dpll ids must be >= 0.

struct intel_shared_dpll_state
hold the DPLL atomic state

Definition:

struct intel_shared_dpll_state {
    u8 pipe_mask;
    struct intel_dpll_hw_state hw_state;
};
Members

pipe_mask
mask of pipes using this DPLL, active or not

hw_state
hardware configuration for the DPLL stored in struct intel_dpll_hw_state.

Description

This structure holds an atomic state for the DPLL, that can represent either its current state (in struct intel_shared_dpll) or a desired future state which would be applied by an atomic mode set (stored in a struct intel_atomic_state).

See also intel_reserve_shared_dplls() and intel_release_shared_dplls().

struct dpll_info
display PLL platform specific info

Definition:

struct dpll_info {
    const char *name;
    const struct intel_shared_dpll_funcs *funcs;
    enum intel_dpll_id id;
    enum intel_display_power_domain power_domain;
    bool always_on;
    bool is_alt_port_dpll;
};
Members

name
DPLL name; used for logging

funcs
platform specific hooks

id
unique indentifier for this DPLL

power_domain
extra power domain required by the DPLL

always_on
Inform the state checker that the DPLL is kept enabled even if not in use by any CRTC.

is_alt_port_dpll
Inform the state checker that the DPLL can be used as a fallback (for TC->TBT fallback).

struct intel_shared_dpll
display PLL with tracked state and users

Definition:

struct intel_shared_dpll {
    struct intel_shared_dpll_state state;
    u8 index;
    u8 active_mask;
    bool on;
    const struct dpll_info *info;
    intel_wakeref_t wakeref;
};
Members

state
Store the state for the pll, including its hw state and CRTCs using it.

index
index for atomic state

active_mask
mask of active pipes (i.e. DPMS on) using this DPLL

on
is the PLL actually active? Disabled during modeset

info
platform specific info

wakeref
In some platforms a device-level runtime pm reference may need to be grabbed to disable DC states while this DPLL is enabled

Display State Buffer
A DSB (Display State Buffer) is a queue of MMIO instructions in the memory which can be offloaded to DSB HW in Display Controller. DSB HW is a DMA engine that can be programmed to download the DSB from memory. It allows driver to batch submit display HW programming. This helps to reduce loading time and CPU activity, thereby making the context switch faster. DSB Support added from Gen12 Intel graphics based platform.

DSB’s can access only the pipe, plane, and transcoder Data Island Packet registers.

DSB HW can support only register writes (both indexed and direct MMIO writes). There are no registers reads possible with DSB HW engine.

void intel_dsb_reg_write(struct intel_dsb *dsb, i915_reg_t reg, u32 val)
Emit register wriite to the DSB context

Parameters

struct intel_dsb *dsb
DSB context

i915_reg_t reg
register address.

u32 val
value.

Description

This function is used for writing register-value pair in command buffer of DSB.

void intel_dsb_commit(struct intel_dsb *dsb, bool wait_for_vblank)
Trigger workload execution of DSB.

Parameters

struct intel_dsb *dsb
DSB context

bool wait_for_vblank
wait for vblank before executing

Description

This function is used to do actual write to hardware using DSB.

struct intel_dsb *intel_dsb_prepare(struct intel_atomic_state *state, struct intel_crtc *crtc, enum intel_dsb_id dsb_id, unsigned int max_cmds)
Allocate, pin and map the DSB command buffer.

Parameters

struct intel_atomic_state *state
the atomic state

struct intel_crtc *crtc
the CRTC

enum intel_dsb_id dsb_id
the DSB engine to use

unsigned int max_cmds
number of commands we need to fit into command buffer

Description

This function prepare the command buffer which is used to store dsb instructions with data.

Return

DSB context, NULL on failure

void intel_dsb_cleanup(struct intel_dsb *dsb)
To cleanup DSB context.

Parameters

struct intel_dsb *dsb
DSB context

Description

This function cleanup the DSB context by unpinning and releasing the VMA object associated with it.

GT Programming
Multicast/Replicated (MCR) Registers
Some GT registers are designed as “multicast” or “replicated” registers: multiple instances of the same register share a single MMIO offset. MCR registers are generally used when the hardware needs to potentially track independent values of a register per hardware unit (e.g., per-subslice, per-L3bank, etc.). The specific types of replication that exist vary per-platform.

MMIO accesses to MCR registers are controlled according to the settings programmed in the platform’s MCR_SELECTOR register(s). MMIO writes to MCR registers can be done in either a (i.e., a single write updates all instances of the register to the same value) or unicast (a write updates only one specific instance). Reads of MCR registers always operate in a unicast manner regardless of how the multicast/unicast bit is set in MCR_SELECTOR. Selection of a specific MCR instance for unicast operations is referred to as “steering.”

If MCR register operations are steered toward a hardware unit that is fused off or currently powered down due to power gating, the MMIO operation is “terminated” by the hardware. Terminated read operations will return a value of zero and terminated unicast write operations will be silently ignored.

void intel_gt_mcr_lock(struct intel_gt *gt, unsigned long *flags)
Acquire MCR steering lock

Parameters

struct intel_gt *gt
GT structure

unsigned long *flags
storage to save IRQ flags to

Description

Performs locking to protect the steering for the duration of an MCR operation. On MTL and beyond, a hardware lock will also be taken to serialize access not only for the driver, but also for external hardware and firmware agents.

Context

Takes gt->mcr_lock. uncore->lock should not be held when this function is called, although it may be acquired after this function call.

void intel_gt_mcr_unlock(struct intel_gt *gt, unsigned long flags)
Release MCR steering lock

Parameters

struct intel_gt *gt
GT structure

unsigned long flags
IRQ flags to restore

Description

Releases the lock acquired by intel_gt_mcr_lock().

Context

Releases gt->mcr_lock

void intel_gt_mcr_lock_sanitize(struct intel_gt *gt)
Sanitize MCR steering lock

Parameters

struct intel_gt *gt
GT structure

Description

This will be used to sanitize the initial status of the hardware lock during driver load and resume since there won’t be any concurrent access from other agents at those times, but it’s possible that boot firmware may have left the lock in a bad state.

u32 intel_gt_mcr_read(struct intel_gt *gt, i915_mcr_reg_t reg, int group, int instance)
read a specific instance of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the MCR register to read

int group
the MCR group

int instance
the MCR instance

Context

Takes and releases gt->mcr_lock

Description

Returns the value read from an MCR register after steering toward a specific group/instance.

void intel_gt_mcr_unicast_write(struct intel_gt *gt, i915_mcr_reg_t reg, u32 value, int group, int instance)
write a specific instance of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the MCR register to write

u32 value
value to write

int group
the MCR group

int instance
the MCR instance

Description

Write an MCR register in unicast mode after steering toward a specific group/instance.

Context

Calls a function that takes and releases gt->mcr_lock

void intel_gt_mcr_multicast_write(struct intel_gt *gt, i915_mcr_reg_t reg, u32 value)
write a value to all instances of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the MCR register to write

u32 value
value to write

Description

Write an MCR register in multicast mode to update all instances.

Context

Takes and releases gt->mcr_lock

void intel_gt_mcr_multicast_write_fw(struct intel_gt *gt, i915_mcr_reg_t reg, u32 value)
write a value to all instances of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the MCR register to write

u32 value
value to write

Description

Write an MCR register in multicast mode to update all instances. This function assumes the caller is already holding any necessary forcewake domains; use intel_gt_mcr_multicast_write() in cases where forcewake should be obtained automatically.

Context

The caller must hold gt->mcr_lock.

u32 intel_gt_mcr_multicast_rmw(struct intel_gt *gt, i915_mcr_reg_t reg, u32 clear, u32 set)
Performs a multicast RMW operations

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the MCR register to read and write

u32 clear
bits to clear during RMW

u32 set
bits to set during RMW

Description

Performs a read-modify-write on an MCR register in a multicast manner. This operation only makes sense on MCR registers where all instances are expected to have the same value. The read will target any non-terminated instance and the write will be applied to all instances.

This function assumes the caller is already holding any necessary forcewake domains; use intel_gt_mcr_multicast_rmw() in cases where forcewake should be obtained automatically.

Returns the old (unmodified) value read.

Context

Calls functions that take and release gt->mcr_lock

void intel_gt_mcr_get_nonterminated_steering(struct intel_gt *gt, i915_mcr_reg_t reg, u8 *group, u8 *instance)
find group/instance values that will steer a register to a non-terminated instance

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
register for which the steering is required

u8 *group
return variable for group steering

u8 *instance
return variable for instance steering

Description

This function returns a group/instance pair that is guaranteed to work for read steering of the given register. Note that a value will be returned even if the register is not replicated and therefore does not actually require steering.

u32 intel_gt_mcr_read_any_fw(struct intel_gt *gt, i915_mcr_reg_t reg)
reads one instance of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
register to read

Description

Reads a GT MCR register. The read will be steered to a non-terminated instance (i.e., one that isn’t fused off or powered down by power gating). This function assumes the caller is already holding any necessary forcewake domains; use intel_gt_mcr_read_any() in cases where forcewake should be obtained automatically.

Returns the value from a non-terminated instance of reg.

Context

The caller must hold gt->mcr_lock.

u32 intel_gt_mcr_read_any(struct intel_gt *gt, i915_mcr_reg_t reg)
reads one instance of an MCR register

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
register to read

Description

Reads a GT MCR register. The read will be steered to a non-terminated instance (i.e., one that isn’t fused off or powered down by power gating).

Returns the value from a non-terminated instance of reg.

Context

Calls a function that takes and releases gt->mcr_lock.

void intel_gt_mcr_get_ss_steering(struct intel_gt *gt, unsigned int dss, unsigned int *group, unsigned int *instance)
returns the group/instance steering for a SS

Parameters

struct intel_gt *gt
GT structure

unsigned int dss
DSS ID to obtain steering for

unsigned int *group
pointer to storage for steering group ID

unsigned int *instance
pointer to storage for steering instance ID

Description

Returns the steering IDs (via the group and instance parameters) that correspond to a specific subslice/DSS ID.

int intel_gt_mcr_wait_for_reg(struct intel_gt *gt, i915_mcr_reg_t reg, u32 mask, u32 value, unsigned int fast_timeout_us, unsigned int slow_timeout_ms)
wait until MCR register matches expected state

Parameters

struct intel_gt *gt
GT structure

i915_mcr_reg_t reg
the register to read

u32 mask
mask to apply to register value

u32 value
value to wait for

unsigned int fast_timeout_us
fast timeout in microsecond for atomic/tight wait

unsigned int slow_timeout_ms
slow timeout in millisecond

Description

This routine waits until the target register reg contains the expected value after applying the mask, i.e. it waits until

(intel_gt_mcr_read_any_fw(gt, reg) & mask) == value
Otherwise, the wait will timeout after slow_timeout_ms milliseconds. For atomic context slow_timeout_ms must be zero and fast_timeout_us must be not larger than 20,0000 microseconds.

This function is basically an MCR-friendly version of __intel_wait_for_register_fw(). Generally this function will only be used on GAM registers which are a bit special --- although they’re MCR registers, reads (e.g., waiting for status updates) are always directed to the primary instance.

Note that this routine assumes the caller holds forcewake asserted, it is not suitable for very long waits.

Context

Calls a function that takes and releases gt->mcr_lock

Return

0 if the register matches the desired condition, or -ETIMEDOUT.

Memory Management and Command Submission
This sections covers all things related to the GEM implementation in the i915 driver.

Intel GPU Basics
An Intel GPU has multiple engines. There are several engine types:

Render Command Streamer (RCS). An engine for rendering 3D and performing compute.

Blitting Command Streamer (BCS). An engine for performing blitting and/or copying operations.

Video Command Streamer. An engine used for video encoding and decoding. Also sometimes called ‘BSD’ in hardware documentation.

Video Enhancement Command Streamer (VECS). An engine for video enhancement. Also sometimes called ‘VEBOX’ in hardware documentation.

Compute Command Streamer (CCS). An engine that has access to the media and GPGPU pipelines, but not the 3D pipeline.

Graphics Security Controller (GSCCS). A dedicated engine for internal communication with GSC controller on security related tasks like High-bandwidth Digital Content Protection (HDCP), Protected Xe Path (PXP), and HuC firmware authentication.

The Intel GPU family is a family of integrated GPU’s using Unified Memory Access. For having the GPU “do work”, user space will feed the GPU batch buffers via one of the ioctls DRM_IOCTL_I915_GEM_EXECBUFFER2 or DRM_IOCTL_I915_GEM_EXECBUFFER2_WR. Most such batchbuffers will instruct the GPU to perform work (for example rendering) and that work needs memory from which to read and memory to which to write. All memory is encapsulated within GEM buffer objects (usually created with the ioctl DRM_IOCTL_I915_GEM_CREATE). An ioctl providing a batchbuffer for the GPU to create will also list all GEM buffer objects that the batchbuffer reads and/or writes. For implementation details of memory management see GEM BO Management Implementation Details.

The i915 driver allows user space to create a context via the ioctl DRM_IOCTL_I915_GEM_CONTEXT_CREATE which is identified by a 32-bit integer. Such a context should be viewed by user-space as -loosely- analogous to the idea of a CPU process of an operating system. The i915 driver guarantees that commands issued to a fixed context are to be executed so that writes of a previously issued command are seen by reads of following commands. Actions issued between different contexts (even if from the same file descriptor) are NOT given that guarantee and the only way to synchronize across contexts (even from the same file descriptor) is through the use of fences. At least as far back as Gen4, also have that a context carries with it a GPU HW context; the HW context is essentially (most of at least) the state of a GPU. In addition to the ordering guarantees, the kernel will restore GPU state via HW context when commands are issued to a context, this saves user space the need to restore (most of at least) the GPU state at the start of each batchbuffer. The non-deprecated ioctls to submit batchbuffer work can pass that ID (in the lower bits of drm_i915_gem_execbuffer2::rsvd1) to identify what context to use with the command.

The GPU has its own memory management and address space. The kernel driver maintains the memory translation table for the GPU. For older GPUs (i.e. those before Gen8), there is a single global such translation table, a global Graphics Translation Table (GTT). For newer generation GPUs each context has its own translation table, called Per-Process Graphics Translation Table (PPGTT). Of important note, is that although PPGTT is named per-process it is actually per context. When user space submits a batchbuffer, the kernel walks the list of GEM buffer objects used by the batchbuffer and guarantees that not only is the memory of each such GEM buffer object resident but it is also present in the (PP)GTT. If the GEM buffer object is not yet placed in the (PP)GTT, then it is given an address. Two consequences of this are: the kernel needs to edit the batchbuffer submitted to write the correct value of the GPU address when a GEM BO is assigned a GPU address and the kernel might evict a different GEM BO from the (PP)GTT to make address room for another GEM BO. Consequently, the ioctls submitting a batchbuffer for execution also include a list of all locations within buffers that refer to GPU-addresses so that the kernel can edit the buffer correctly. This process is dubbed relocation.

Locking Guidelines
Note
This is a description of how the locking should be after refactoring is done. Does not necessarily reflect what the locking looks like while WIP.

All locking rules and interface contracts with cross-driver interfaces (dma-buf, dma_fence) need to be followed.

No struct_mutex anywhere in the code

dma_resv will be the outermost lock (when needed) and ww_acquire_ctx is to be hoisted at highest level and passed down within i915_gem_ctx in the call chain

While holding lru/memory manager (buddy, drm_mm, whatever) locks system memory allocations are not allowed

Enforce this by priming lockdep (with fs_reclaim). If we allocate memory while holding these looks we get a rehash of the shrinker vs. struct_mutex saga, and that would be real bad.

Do not nest different lru/memory manager locks within each other. Take them in turn to update memory allocations, relying on the object’s dma_resv ww_mutex to serialize against other operations.

The suggestion for lru/memory managers locks is that they are small enough to be spinlocks.

All features need to come with exhaustive kernel selftests and/or IGT tests when appropriate

All LMEM uAPI paths need to be fully restartable (_interruptible() for all locks/waits/sleeps)

Error handling validation through signal injection. Still the best strategy we have for validating GEM uAPI corner cases. Must be excessively used in the IGT, and we need to check that we really have full path coverage of all error cases.

-EDEADLK handling with ww_mutex

GEM BO Management Implementation Details
A VMA represents a GEM BO that is bound into an address space. Therefore, a VMA’s presence cannot be guaranteed before binding, or after unbinding the object into/from the address space.

To make things as simple as possible (ie. no refcounting), a VMA’s lifetime will always be <= an objects lifetime. So object refcounting should cover us.

Buffer Object Eviction
This section documents the interface functions for evicting buffer objects to make space available in the virtual gpu address spaces. Note that this is mostly orthogonal to shrinking buffer objects caches, which has the goal to make main memory (shared with the gpu through the unified memory architecture) available.

int i915_gem_evict_something(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww, u64 min_size, u64 alignment, unsigned long color, u64 start, u64 end, unsigned flags)
Evict vmas to make room for binding a new one

Parameters

struct i915_address_space *vm
address space to evict from

struct i915_gem_ww_ctx *ww
An optional struct i915_gem_ww_ctx.

u64 min_size
size of the desired free space

u64 alignment
alignment constraint of the desired free space

unsigned long color
color for the desired space

u64 start
start (inclusive) of the range from which to evict objects

u64 end
end (exclusive) of the range from which to evict objects

unsigned flags
additional flags to control the eviction algorithm

Description

This function will try to evict vmas until a free space satisfying the requirements is found. Callers must check first whether any such hole exists already before calling this function.

This function is used by the object/vma binding code.

Since this function is only used to free up virtual address space it only ignores pinned vmas, and not object where the backing storage itself is pinned. Hence obj->pages_pin_count does not protect against eviction.

To clarify: This is for freeing up virtual address space, not for freeing memory in e.g. the shrinker.

int i915_gem_evict_for_node(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww, struct drm_mm_node *target, unsigned int flags)
Evict vmas to make room for binding a new one

Parameters

struct i915_address_space *vm
address space to evict from

struct i915_gem_ww_ctx *ww
An optional struct i915_gem_ww_ctx.

struct drm_mm_node *target
range (and color) to evict for

unsigned int flags
additional flags to control the eviction algorithm

Description

This function will try to evict vmas that overlap the target node.

To clarify: This is for freeing up virtual address space, not for freeing memory in e.g. the shrinker.

int i915_gem_evict_vm(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww, struct drm_i915_gem_object **busy_bo)
Evict all idle vmas from a vm

Parameters

struct i915_address_space *vm
Address space to cleanse

struct i915_gem_ww_ctx *ww
An optional struct i915_gem_ww_ctx. If not NULL, i915_gem_evict_vm will be able to evict vma’s locked by the ww as well.

struct drm_i915_gem_object **busy_bo
Optional pointer to struct drm_i915_gem_object. If not NULL, then in the event i915_gem_evict_vm() is unable to trylock an object for eviction, then busy_bo will point to it. -EBUSY is also returned. The caller must drop the vm->mutex, before trying again to acquire the contended lock. The caller also owns a reference to the object.

Description

This function evicts all vmas from a vm.

This is used by the execbuf code as a last-ditch effort to defragment the address space.

To clarify: This is for freeing up virtual address space, not for freeing memory in e.g. the shrinker.

Buffer Object Memory Shrinking
This section documents the interface function for shrinking memory usage of buffer object caches. Shrinking is used to make main memory available. Note that this is mostly orthogonal to evicting buffer objects, which has the goal to make space in gpu virtual address spaces.

unsigned long i915_gem_shrink(struct i915_gem_ww_ctx *ww, struct drm_i915_private *i915, unsigned long target, unsigned long *nr_scanned, unsigned int shrink)
Shrink buffer object caches

Parameters

struct i915_gem_ww_ctx *ww
i915 gem ww acquire ctx, or NULL

struct drm_i915_private *i915
i915 device

unsigned long target
amount of memory to make available, in pages

unsigned long *nr_scanned
optional output for number of pages scanned (incremental)

unsigned int shrink
control flags for selecting cache types

Description

This function is the main interface to the shrinker. It will try to release up to target pages of main memory backing storage from buffer objects. Selection of the specific caches can be done with flags. This is e.g. useful when purgeable objects should be removed from caches preferentially.

Note that it’s not guaranteed that released amount is actually available as free system memory - the pages might still be in-used to due to other reasons (like cpu mmaps) or the mm core has reused them before we could grab them. Therefore code that needs to explicitly shrink buffer objects caches (e.g. to avoid deadlocks in memory reclaim) must fall back to i915_gem_shrink_all().

Also note that any kind of pinning (both per-vma address space pins and backing storage pins at the buffer object level) result in the shrinker code having to skip the object.

Return

The number of pages of backing storage actually released.

unsigned long i915_gem_shrink_all(struct drm_i915_private *i915)
Shrink buffer object caches completely

Parameters

struct drm_i915_private *i915
i915 device

Description

This is a simple wraper around i915_gem_shrink() to aggressively shrink all caches completely. It also first waits for and retires all outstanding requests to also be able to release backing storage for active objects.

This should only be used in code to intentionally quiescent the gpu or as a last-ditch effort when memory seems to have run out.

Return

The number of pages of backing storage actually released.

void i915_gem_object_make_unshrinkable(struct drm_i915_gem_object *obj)
Hide the object from the shrinker. By default all object types that support shrinking(see IS_SHRINKABLE), will also make the object visible to the shrinker after allocating the system memory pages.

Parameters

struct drm_i915_gem_object *obj
The GEM object.

Description

This is typically used for special kernel internal objects that can’t be easily processed by the shrinker, like if they are perma-pinned.

void __i915_gem_object_make_shrinkable(struct drm_i915_gem_object *obj)
Move the object to the tail of the shrinkable list. Objects on this list might be swapped out. Used with WILLNEED objects.

Parameters

struct drm_i915_gem_object *obj
The GEM object.

Description

DO NOT USE. This is intended to be called on very special objects that don’t yet have mm.pages, but are guaranteed to have potentially reclaimable pages underneath.

void __i915_gem_object_make_purgeable(struct drm_i915_gem_object *obj)
Move the object to the tail of the purgeable list. Objects on this list might be swapped out. Used with DONTNEED objects.

Parameters

struct drm_i915_gem_object *obj
The GEM object.

Description

DO NOT USE. This is intended to be called on very special objects that don’t yet have mm.pages, but are guaranteed to have potentially reclaimable pages underneath.

void i915_gem_object_make_shrinkable(struct drm_i915_gem_object *obj)
Move the object to the tail of the shrinkable list. Objects on this list might be swapped out. Used with WILLNEED objects.

Parameters

struct drm_i915_gem_object *obj
The GEM object.

Description

MUST only be called on objects which have backing pages.

MUST be balanced with previous call to i915_gem_object_make_unshrinkable().

void i915_gem_object_make_purgeable(struct drm_i915_gem_object *obj)
Move the object to the tail of the purgeable list. Used with DONTNEED objects. Unlike with shrinkable objects, the shrinker will attempt to discard the backing pages, instead of trying to swap them out.

Parameters

struct drm_i915_gem_object *obj
The GEM object.

Description

MUST only be called on objects which have backing pages.

MUST be balanced with previous call to i915_gem_object_make_unshrinkable().

Batchbuffer Parsing
Motivation: Certain OpenGL features (e.g. transform feedback, performance monitoring) require userspace code to submit batches containing commands such as MI_LOAD_REGISTER_IMM to access various registers. Unfortunately, some generations of the hardware will noop these commands in “unsecure” batches (which includes all userspace batches submitted via i915) even though the commands may be safe and represent the intended programming model of the device.

The software command parser is similar in operation to the command parsing done in hardware for unsecure batches. However, the software parser allows some operations that would be noop’d by hardware, if the parser determines the operation is safe, and submits the batch as “secure” to prevent hardware parsing.

Threats: At a high level, the hardware (and software) checks attempt to prevent granting userspace undue privileges. There are three categories of privilege.

First, commands which are explicitly defined as privileged or which should only be used by the kernel driver. The parser rejects such commands

Second, commands which access registers. To support correct/enhanced userspace functionality, particularly certain OpenGL extensions, the parser provides a whitelist of registers which userspace may safely access

Third, commands which access privileged memory (i.e. GGTT, HWS page, etc). The parser always rejects such commands.

The majority of the problematic commands fall in the MI_* range, with only a few specific commands on each engine (e.g. PIPE_CONTROL and MI_FLUSH_DW).

Implementation: Each engine maintains tables of commands and registers which the parser uses in scanning batch buffers submitted to that engine.

Since the set of commands that the parser must check for is significantly smaller than the number of commands supported, the parser tables contain only those commands required by the parser. This generally works because command opcode ranges have standard command length encodings. So for commands that the parser does not need to check, it can easily skip them. This is implemented via a per-engine length decoding vfunc.

Unfortunately, there are a number of commands that do not follow the standard length encoding for their opcode range, primarily amongst the MI_* commands. To handle this, the parser provides a way to define explicit “skip” entries in the per-engine command tables.

Other command table entries map fairly directly to high level categories mentioned above: rejected, register whitelist. The parser implements a number of checks, including the privileged memory checks, via a general bitmasking mechanism.

int intel_engine_init_cmd_parser(struct intel_engine_cs *engine)
set cmd parser related fields for an engine

Parameters

struct intel_engine_cs *engine
the engine to initialize

Description

Optionally initializes fields related to batch buffer command parsing in the struct intel_engine_cs based on whether the platform requires software command parsing.

void intel_engine_cleanup_cmd_parser(struct intel_engine_cs *engine)
clean up cmd parser related fields

Parameters

struct intel_engine_cs *engine
the engine to clean up

Description

Releases any resources related to command parsing that may have been initialized for the specified engine.

int intel_engine_cmd_parser(struct intel_engine_cs *engine, struct i915_vma *batch, unsigned long batch_offset, unsigned long batch_length, struct i915_vma *shadow, bool trampoline)
parse a batch buffer for privilege violations

Parameters

struct intel_engine_cs *engine
the engine on which the batch is to execute

struct i915_vma *batch
the batch buffer in question

unsigned long batch_offset
byte offset in the batch at which execution starts

unsigned long batch_length
length of the commands in batch_obj

struct i915_vma *shadow
validated copy of the batch buffer in question

bool trampoline
true if we need to trampoline into privileged execution

Description

Parses the specified batch buffer looking for privilege violations as described in the overview.

Return

non-zero if the parser finds violations or otherwise fails; -EACCES if the batch appears legal but should use hardware parsing

int i915_cmd_parser_get_version(struct drm_i915_private *dev_priv)
get the cmd parser version number

Parameters

struct drm_i915_private *dev_priv
i915 device private

Description

The cmd parser maintains a simple increasing integer version number suitable for passing to userspace clients to determine what operations are permitted.

Return

the current version number of the cmd parser

User Batchbuffer Execution
struct i915_gem_engines
A set of engines

Definition:

struct i915_gem_engines {
    union {
        struct list_head link;
        struct rcu_head rcu;
    };
    struct i915_sw_fence fence;
    struct i915_gem_context *ctx;
    unsigned int num_engines;
    struct intel_context *engines[];
};
Members

{unnamed_union}
anonymous

link
Link in i915_gem_context::stale::engines

rcu
RCU to use when freeing

fence
Fence used for delayed destruction of engines

ctx
i915_gem_context backpointer

num_engines
Number of engines in this set

engines
Array of engines

struct i915_gem_engines_iter
Iterator for an i915_gem_engines set

Definition:

struct i915_gem_engines_iter {
    unsigned int idx;
    const struct i915_gem_engines *engines;
};
Members

idx
Index into i915_gem_engines::engines

engines
Engine set being iterated

enum i915_gem_engine_type
Describes the type of an i915_gem_proto_engine

Constants

I915_GEM_ENGINE_TYPE_INVALID
An invalid engine

I915_GEM_ENGINE_TYPE_PHYSICAL
A single physical engine

I915_GEM_ENGINE_TYPE_BALANCED
A load-balanced engine set

I915_GEM_ENGINE_TYPE_PARALLEL
A parallel engine set

struct i915_gem_proto_engine
prototype engine

Definition:

struct i915_gem_proto_engine {
    enum i915_gem_engine_type type;
    struct intel_engine_cs *engine;
    unsigned int num_siblings;
    unsigned int width;
    struct intel_engine_cs **siblings;
    struct intel_sseu sseu;
};
Members

type
Type of this engine

engine
Engine, for physical

num_siblings
Number of balanced or parallel siblings

width
Width of each sibling

siblings
Balanced siblings or num_siblings * width for parallel

sseu
Client-set SSEU parameters

Description

This struct describes an engine that a context may contain. Engines have four types:

I915_GEM_ENGINE_TYPE_INVALID: Invalid engines can be created but they show up as a NULL in i915_gem_engines::engines[i] and any attempt to use them by the user results in -EINVAL. They are also useful during proto-context construction because the client may create invalid engines and then set them up later as virtual engines.

I915_GEM_ENGINE_TYPE_PHYSICAL: A single physical engine, described by i915_gem_proto_engine::engine.

I915_GEM_ENGINE_TYPE_BALANCED: A load-balanced engine set, described i915_gem_proto_engine::num_siblings and i915_gem_proto_engine::siblings.

I915_GEM_ENGINE_TYPE_PARALLEL: A parallel submission engine set, described i915_gem_proto_engine::width, i915_gem_proto_engine::num_siblings, and i915_gem_proto_engine::siblings.

struct i915_gem_proto_context
prototype context

Definition:

struct i915_gem_proto_context {
    struct drm_i915_file_private *fpriv;
    struct i915_address_space *vm;
    unsigned long user_flags;
    struct i915_sched_attr sched;
    int num_user_engines;
    struct i915_gem_proto_engine *user_engines;
    struct intel_sseu legacy_rcs_sseu;
    bool single_timeline;
    bool uses_protected_content;
    intel_wakeref_t pxp_wakeref;
};
Members

fpriv
Client which creates the context

vm
See i915_gem_context.vm

user_flags
See i915_gem_context.user_flags

sched
See i915_gem_context.sched

num_user_engines
Number of user-specified engines or -1

user_engines
User-specified engines

legacy_rcs_sseu
Client-set SSEU parameters for the legacy RCS

single_timeline
See See i915_gem_context.syncobj

uses_protected_content
See i915_gem_context.uses_protected_content

pxp_wakeref
See i915_gem_context.pxp_wakeref

Description

The struct i915_gem_proto_context represents the creation parameters for a struct i915_gem_context. This is used to gather parameters provided either through creation flags or via SET_CONTEXT_PARAM so that, when we create the final i915_gem_context, those parameters can be immutable.

The context uAPI allows for two methods of setting context parameters: SET_CONTEXT_PARAM and CONTEXT_CREATE_EXT_SETPARAM. The former is allowed to be called at any time while the later happens as part of GEM_CONTEXT_CREATE. When these were initially added, Currently, everything settable via one is settable via the other. While some params are fairly simple and setting them on a live context is harmless such the context priority, others are far trickier such as the VM or the set of engines. To avoid some truly nasty race conditions, we don’t allow setting the VM or the set of engines on live contexts.

The way we dealt with this without breaking older userspace that sets the VM or engine set via SET_CONTEXT_PARAM is to delay the creation of the actual context until after the client is done configuring it with SET_CONTEXT_PARAM. From the perspective of the client, it has the same u32 context ID the whole time. From the perspective of i915, however, it’s an i915_gem_proto_context right up until the point where we attempt to do something which the proto-context can’t handle at which point the real context gets created.

This is accomplished via a little xarray dance. When GEM_CONTEXT_CREATE is called, we create a proto-context, reserve a slot in context_xa but leave it NULL, the proto-context in the corresponding slot in proto_context_xa. Then, whenever we go to look up a context, we first check context_xa. If it’s there, we return the i915_gem_context and we’re done. If it’s not, we look in proto_context_xa and, if we find it there, we create the actual context and kill the proto-context.

At the time we made this change (April, 2021), we did a fairly complete audit of existing userspace to ensure this wouldn’t break anything:

Mesa/i965 didn’t use the engines or VM APIs at all

Mesa/ANV used the engines API but via CONTEXT_CREATE_EXT_SETPARAM and didn’t use the VM API.

Mesa/iris didn’t use the engines or VM APIs at all

The open-source compute-runtime didn’t yet use the engines API but did use the VM API via SET_CONTEXT_PARAM. However, CONTEXT_SETPARAM was always the second ioctl on that context, immediately following GEM_CONTEXT_CREATE.

The media driver sets engines and bonding/balancing via SET_CONTEXT_PARAM. However, CONTEXT_SETPARAM to set the VM was always the second ioctl on that context, immediately following GEM_CONTEXT_CREATE and setting engines immediately followed that.

In order for this dance to work properly, any modification to an i915_gem_proto_context that is exposed to the client via drm_i915_file_private::proto_context_xa must be guarded by drm_i915_file_private::proto_context_lock. The exception is when a proto-context has not yet been exposed such as when handling CONTEXT_CREATE_SET_PARAM during GEM_CONTEXT_CREATE.

struct i915_gem_context
client state

Definition:

struct i915_gem_context {
    struct drm_i915_private *i915;
    struct drm_i915_file_private *file_priv;
    struct i915_gem_engines __rcu *engines;
    struct mutex engines_mutex;
    struct drm_syncobj *syncobj;
    struct i915_address_space *vm;
    struct pid *pid;
    struct list_head link;
    struct i915_drm_client *client;
    struct list_head client_link;
    struct kref ref;
    struct work_struct release_work;
    struct rcu_head rcu;
    unsigned long user_flags;
#define UCONTEXT_NO_ERROR_CAPTURE       1;
#define UCONTEXT_BANNABLE               2;
#define UCONTEXT_RECOVERABLE            3;
#define UCONTEXT_PERSISTENCE            4;
#define UCONTEXT_LOW_LATENCY            5;
    unsigned long flags;
#define CONTEXT_CLOSED                  0;
#define CONTEXT_USER_ENGINES            1;
    bool uses_protected_content;
    intel_wakeref_t pxp_wakeref;
    struct mutex mutex;
    struct i915_sched_attr sched;
    atomic_t guilty_count;
    atomic_t active_count;
    unsigned long hang_timestamp[2];
#define CONTEXT_FAST_HANG_JIFFIES (120 * HZ) ;
    u8 remap_slice;
    struct radix_tree_root handles_vma;
    struct mutex lut_mutex;
    char name[TASK_COMM_LEN + 8];
    struct {
        spinlock_t lock;
        struct list_head engines;
    } stale;
};
Members

i915
i915 device backpointer

file_priv
owning file descriptor

engines
User defined engines for this context

Various uAPI offer the ability to lookup up an index from this array to select an engine operate on.

Multiple logically distinct instances of the same engine may be defined in the array, as well as composite virtual engines.

Execbuf uses the I915_EXEC_RING_MASK as an index into this array to select which HW context + engine to execute on. For the default array, the user_ring_map[] is used to translate the legacy uABI onto the approprate index (e.g. both I915_EXEC_DEFAULT and I915_EXEC_RENDER select the same context, and I915_EXEC_BSD is weird). For a use defined array, execbuf uses I915_EXEC_RING_MASK as a plain index.

User defined by I915_CONTEXT_PARAM_ENGINE (when the CONTEXT_USER_ENGINES flag is set).

engines_mutex
guards writes to engines

syncobj
Shared timeline syncobj

When the SHARED_TIMELINE flag is set on context creation, we emulate a single timeline across all engines using this syncobj. For every execbuffer2 call, this syncobj is used as both an in- and out-fence. Unlike the real intel_timeline, this doesn’t provide perfect atomic in-order guarantees if the client races with itself by calling execbuffer2 twice concurrently. However, if userspace races with itself, that’s not likely to yield well- defined results anyway so we choose to not care.

vm
unique address space (GTT)

In full-ppgtt mode, each context has its own address space ensuring complete seperation of one client from all others.

In other modes, this is a NULL pointer with the expectation that the caller uses the shared global GTT.

pid
process id of creator

Note that who created the context may not be the principle user, as the context may be shared across a local socket. However, that should only affect the default context, all contexts created explicitly by the client are expected to be isolated.

link
place with drm_i915_private.context_list

client
struct i915_drm_client

client_link
for linking onto i915_drm_client.ctx_list

ref
reference count

A reference to a context is held by both the client who created it and on each request submitted to the hardware using the request (to ensure the hardware has access to the state until it has finished all pending writes). See i915_gem_context_get() and i915_gem_context_put() for access.

release_work
Work item for deferred cleanup, since i915_gem_context_put() tends to be called from hardirq context.

FIXME: The only real reason for this is i915_gem_engines.fence, all other callers are from process context and need at most some mild shuffling to pull the i915_gem_context_put() call out of a spinlock.

rcu
rcu_head for deferred freeing.

user_flags
small set of booleans controlled by the user

flags
small set of booleans

uses_protected_content
context uses PXP-encrypted objects.

This flag can only be set at ctx creation time and it’s immutable for the lifetime of the context. See I915_CONTEXT_PARAM_PROTECTED_CONTENT in uapi/drm/i915_drm.h for more info on setting restrictions and expected behaviour of marked contexts.

pxp_wakeref
wakeref to keep the device awake when PXP is in use

PXP sessions are invalidated when the device is suspended, which in turns invalidates all contexts and objects using it. To keep the flow simple, we keep the device awake when contexts using PXP objects are in use. It is expected that the userspace application only uses PXP when the display is on, so taking a wakeref here shouldn’t worsen our power metrics.

mutex
guards everything that isn’t engines or handles_vma

sched
scheduler parameters

guilty_count
How many times this context has caused a GPU hang.

active_count
How many times this context was active during a GPU hang, but did not cause it.

hang_timestamp
The last time(s) this context caused a GPU hang

remap_slice
Bitmask of cache lines that need remapping

handles_vma
rbtree to look up our context specific obj/vma for the user handle. (user handles are per fd, but the binding is per vm, which may be one per context or shared with the global GTT)

lut_mutex
Locks handles_vma

name
arbitrary name, used for user debug

A name is constructed for the context from the creator’s process name, pid and user handle in order to uniquely identify the context in messages.

stale
tracks stale engines to be destroyed

Description

The struct i915_gem_context represents the combined view of the driver and logical hardware state for a particular client.

Userspace submits commands to be executed on the GPU as an instruction stream within a GEM object we call a batchbuffer. This instructions may refer to other GEM objects containing auxiliary state such as kernels, samplers, render targets and even secondary batchbuffers. Userspace does not know where in the GPU memory these objects reside and so before the batchbuffer is passed to the GPU for execution, those addresses in the batchbuffer and auxiliary objects are updated. This is known as relocation, or patching. To try and avoid having to relocate each object on the next execution, userspace is told the location of those objects in this pass, but this remains just a hint as the kernel may choose a new location for any object in the future.

At the level of talking to the hardware, submitting a batchbuffer for the GPU to execute is to add content to a buffer from which the HW command streamer is reading.

Add a command to load the HW context. For Logical Ring Contexts, i.e. Execlists, this command is not placed on the same buffer as the remaining items.

Add a command to invalidate caches to the buffer.

Add a batchbuffer start command to the buffer; the start command is essentially a token together with the GPU address of the batchbuffer to be executed.

Add a pipeline flush to the buffer.

Add a memory write command to the buffer to record when the GPU is done executing the batchbuffer. The memory write writes the global sequence number of the request, i915_request::global_seqno; the i915 driver uses the current value in the register to determine if the GPU has completed the batchbuffer.

Add a user interrupt command to the buffer. This command instructs the GPU to issue an interrupt when the command, pipeline flush and memory write are completed.

Inform the hardware of the additional commands added to the buffer (by updating the tail pointer).

Processing an execbuf ioctl is conceptually split up into a few phases.

Validation - Ensure all the pointers, handles and flags are valid.

Reservation - Assign GPU address space for every object

Relocation - Update any addresses to point to the final locations

Serialisation - Order the request with respect to its dependencies

Construction - Construct a request to execute the batchbuffer

Submission (at some point in the future execution)

Reserving resources for the execbuf is the most complicated phase. We neither want to have to migrate the object in the address space, nor do we want to have to update any relocations pointing to this object. Ideally, we want to leave the object where it is and for all the existing relocations to match. If the object is given a new address, or if userspace thinks the object is elsewhere, we have to parse all the relocation entries and update the addresses. Userspace can set the I915_EXEC_NORELOC flag to hint that all the target addresses in all of its objects match the value in the relocation entries and that they all match the presumed offsets given by the list of execbuffer objects. Using this knowledge, we know that if we haven’t moved any buffers, all the relocation entries are valid and we can skip the update. (If userspace is wrong, the likely outcome is an impromptu GPU hang.) The requirement for using I915_EXEC_NO_RELOC are:

The addresses written in the objects must match the corresponding reloc.presumed_offset which in turn must match the corresponding execobject.offset.

Any render targets written to in the batch must be flagged with EXEC_OBJECT_WRITE.

To avoid stalling, execobject.offset should match the current address of that object within the active context.

The reservation is done is multiple phases. First we try and keep any object already bound in its current location - so as long as meets the constraints imposed by the new execbuffer. Any object left unbound after the first pass is then fitted into any available idle space. If an object does not fit, all objects are removed from the reservation and the process rerun after sorting the objects into a priority order (more difficult to fit objects are tried first). Failing that, the entire VM is cleared and we try to fit the execbuf once last time before concluding that it simply will not fit.

A small complication to all of this is that we allow userspace not only to specify an alignment and a size for the object in the address space, but we also allow userspace to specify the exact offset. This objects are simpler to place (the location is known a priori) all we have to do is make sure the space is available.

Once all the objects are in place, patching up the buried pointers to point to the final locations is a fairly simple job of walking over the relocation entry arrays, looking up the right address and rewriting the value into the object. Simple! ... The relocation entries are stored in user memory and so to access them we have to copy them into a local buffer. That copy has to avoid taking any pagefaults as they may lead back to a GEM object requiring the struct_mutex (i.e. recursive deadlock). So once again we split the relocation into multiple passes. First we try to do everything within an atomic context (avoid the pagefaults) which requires that we never wait. If we detect that we may wait, or if we need to fault, then we have to fallback to a slower path. The slowpath has to drop the mutex. (Can you hear alarm bells yet?) Dropping the mutex means that we lose all the state we have built up so far for the execbuf and we must reset any global data. However, we do leave the objects pinned in their final locations - which is a potential issue for concurrent execbufs. Once we have left the mutex, we can allocate and copy all the relocation entries into a large array at our leisure, reacquire the mutex, reclaim all the objects and other state and then proceed to update any incorrect addresses with the objects.

As we process the relocation entries, we maintain a record of whether the object is being written to. Using NORELOC, we expect userspace to provide this information instead. We also check whether we can skip the relocation by comparing the expected value inside the relocation entry with the target’s final address. If they differ, we have to map the current object and rewrite the 4 or 8 byte pointer within.

Serialising an execbuf is quite simple according to the rules of the GEM ABI. Execution within each context is ordered by the order of submission. Writes to any GEM object are in order of submission and are exclusive. Reads from a GEM object are unordered with respect to other reads, but ordered by writes. A write submitted after a read cannot occur before the read, and similarly any read submitted after a write cannot occur before the write. Writes are ordered between engines such that only one write occurs at any time (completing any reads beforehand) - using semaphores where available and CPU serialisation otherwise. Other GEM access obey the same rules, any write (either via mmaps using set-domain, or via pwrite) must flush all GPU reads before starting, and any read (either using set-domain or pread) must flush all GPU writes before starting. (Note we only employ a barrier before, we currently rely on userspace not concurrently starting a new execution whilst reading or writing to an object. This may be an advantage or not depending on how much you trust userspace not to shoot themselves in the foot.) Serialisation may just result in the request being inserted into a DAG awaiting its turn, but most simple is to wait on the CPU until all dependencies are resolved.

After all of that, is just a matter of closing the request and handing it to the hardware (well, leaving it in a queue to be executed). However, we also offer the ability for batchbuffers to be run with elevated privileges so that they access otherwise hidden registers. (Used to adjust L3 cache etc.) Before any batch is given extra privileges we first must check that it contains no nefarious instructions, we check that each instruction is from our whitelist and all registers are also from an allowed list. We first copy the user’s batchbuffer to a shadow (so that the user doesn’t have access to it, either by the CPU or GPU as we scan it) and then parse each instruction. If everything is ok, we set a flag telling the hardware to run the batchbuffer in trusted mode, otherwise the ioctl is rejected.

Scheduling
struct i915_sched_engine
scheduler engine

Definition:

struct i915_sched_engine {
    struct kref ref;
    spinlock_t lock;
    struct list_head requests;
    struct list_head hold;
    struct tasklet_struct tasklet;
    struct i915_priolist default_priolist;
    int queue_priority_hint;
    struct rb_root_cached queue;
    bool no_priolist;
    void *private_data;
    void (*destroy)(struct kref *kref);
    bool (*disabled)(struct i915_sched_engine *sched_engine);
    void (*kick_backend)(const struct i915_request *rq, int prio);
    void (*bump_inflight_request_prio)(struct i915_request *rq, int prio);
    void (*retire_inflight_request_prio)(struct i915_request *rq);
    void (*schedule)(struct i915_request *request, const struct i915_sched_attr *attr);
};
Members

ref
reference count of schedule engine object

lock
protects requests in priority lists, requests, hold and tasklet while running

requests
list of requests inflight on this schedule engine

hold
list of ready requests, but on hold

tasklet
softirq tasklet for submission

default_priolist
priority list for I915_PRIORITY_NORMAL

queue_priority_hint
Highest pending priority.

When we add requests into the queue, or adjust the priority of executing requests, we compute the maximum priority of those pending requests. We can then use this value to determine if we need to preempt the executing requests to service the queue. However, since the we may have recorded the priority of an inflight request we wanted to preempt but since completed, at the time of dequeuing the priority hint may no longer may match the highest available request priority.

queue
queue of requests, in priority lists

no_priolist
priority lists disabled

private_data
private data of the submission backend

destroy
destroy schedule engine / cleanup in backend

disabled
check if backend has disabled submission

kick_backend
kick backend after a request’s priority has changed

bump_inflight_request_prio
update priority of an inflight request

retire_inflight_request_prio
indicate request is retired to priority tracking

schedule
adjust priority of request

Call when the priority on a request has changed and it and its dependencies may need rescheduling. Note the request itself may not be ready to run!

Description

A schedule engine represents a submission queue with different priority bands. It contains all the common state (relative to the backend) to queue, track, and submit a request.

This object at the moment is quite i915 specific but will transition into a container for the drm_gpu_scheduler plus a few other variables once the i915 is integrated with the DRM scheduler.

Logical Rings, Logical Ring Contexts and Execlists
Motivation: GEN8 brings an expansion of the HW contexts: “Logical Ring Contexts”. These expanded contexts enable a number of new abilities, especially “Execlists” (also implemented in this file).

One of the main differences with the legacy HW contexts is that logical ring contexts incorporate many more things to the context’s state, like PDPs or ringbuffer control registers:

The reason why PDPs are included in the context is straightforward: as PPGTTs (per-process GTTs) are actually per-context, having the PDPs contained there mean you don’t need to do a ppgtt->switch_mm yourself, instead, the GPU will do it for you on the context switch.

But, what about the ringbuffer control registers (head, tail, etc..)? shouldn’t we just need a set of those per engine command streamer? This is where the name “Logical Rings” starts to make sense: by virtualizing the rings, the engine cs shifts to a new “ring buffer” with every context switch. When you want to submit a workload to the GPU you: A) choose your context, B) find its appropriate virtualized ring, C) write commands to it and then, finally, D) tell the GPU to switch to that context.

Instead of the legacy MI_SET_CONTEXT, the way you tell the GPU to switch to a contexts is via a context execution list, ergo “Execlists”.

LRC implementation: Regarding the creation of contexts, we have:

One global default context.

One local default context for each opened fd.

One local extra context for each context create ioctl call.

Now that ringbuffers belong per-context (and not per-engine, like before) and that contexts are uniquely tied to a given engine (and not reusable, like before) we need:

One ringbuffer per-engine inside each context.

One backing object per-engine inside each context.

The global default context starts its life with these new objects fully allocated and populated. The local default context for each opened fd is more complex, because we don’t know at creation time which engine is going to use them. To handle this, we have implemented a deferred creation of LR contexts:

The local context starts its life as a hollow or blank holder, that only gets populated for a given engine once we receive an execbuffer. If later on we receive another execbuffer ioctl for the same context but a different engine, we allocate/populate a new ringbuffer and context backing object and so on.

Finally, regarding local contexts created using the ioctl call: as they are only allowed with the render ring, we can allocate & populate them right away (no need to defer anything, at least for now).

Execlists implementation: Execlists are the new method by which, on gen8+ hardware, workloads are submitted for execution (as opposed to the legacy, ringbuffer-based, method). This method works as follows:

When a request is committed, its commands (the BB start and any leading or trailing commands, like the seqno breadcrumbs) are placed in the ringbuffer for the appropriate context. The tail pointer in the hardware context is not updated at this time, but instead, kept by the driver in the ringbuffer structure. A structure representing this request is added to a request queue for the appropriate engine: this structure contains a copy of the context’s tail after the request was written to the ring buffer and a pointer to the context itself.

If the engine’s request queue was empty before the request was added, the queue is processed immediately. Otherwise the queue will be processed during a context switch interrupt. In any case, elements on the queue will get sent (in pairs) to the GPU’s ExecLists Submit Port (ELSP, for short) with a globally unique 20-bits submission ID.

When execution of a request completes, the GPU updates the context status buffer with a context complete event and generates a context switch interrupt. During the interrupt handling, the driver examines the events in the buffer: for each context complete event, if the announced ID matches that on the head of the request queue, then that request is retired and removed from the queue.

After processing, if any requests were retired and the queue is not empty then a new execution list can be submitted. The two requests at the front of the queue are next to be submitted but since a context may not occur twice in an execution list, if subsequent requests have the same ID as the first then the two requests must be combined. This is done simply by discarding requests at the head of the queue until either only one requests is left (in which case we use a NULL second context) or the first two requests have unique IDs.

By always executing the first two requests in the queue the driver ensures that the GPU is kept as busy as possible. In the case where a single context completes but a second context is still executing, the request for this second context will be at the head of the queue when we remove the first one. This request will then be resubmitted along with a new request for a different context, which will cause the hardware to continue executing the second request and queue the new request (the GPU detects the condition of a context getting preempted with the same context and optimizes the context switch flow by not doing preemption, but just sampling the new tail pointer).

Global GTT views
Background and previous state

Historically objects could exists (be bound) in global GTT space only as singular instances with a view representing all of the object’s backing pages in a linear fashion. This view will be called a normal view.

To support multiple views of the same object, where the number of mapped pages is not equal to the backing store, or where the layout of the pages is not linear, concept of a GGTT view was added.

One example of an alternative view is a stereo display driven by a single image. In this case we would have a framebuffer looking like this (2x2 pages):

12 34

Above would represent a normal GGTT view as normally mapped for GPU or CPU rendering. In contrast, fed to the display engine would be an alternative view which could look something like this:

1212 3434

In this example both the size and layout of pages in the alternative view is different from the normal view.

Implementation and usage

GGTT views are implemented using VMAs and are distinguished via enum i915_gtt_view_type and struct i915_gtt_view.

A new flavour of core GEM functions which work with GGTT bound objects were added with the _ggtt_ infix, and sometimes with _view postfix to avoid renaming in large amounts of code. They take the struct i915_gtt_view parameter encapsulating all metadata required to implement a view.

As a helper for callers which are only interested in the normal view, globally const i915_gtt_view_normal singleton instance exists. All old core GEM API functions, the ones not taking the view parameter, are operating on, or with the normal GGTT view.

Code wanting to add or use a new GGTT view needs to:

Add a new enum with a suitable name.

Extend the metadata in the i915_gtt_view structure if required.

Add support to i915_get_vma_pages().

New views are required to build a scatter-gather table from within the i915_get_vma_pages function. This table is stored in the vma.gtt_view and exists for the lifetime of an VMA.

Core API is designed to have copy semantics which means that passed in struct i915_gtt_view does not need to be persistent (left around after calling the core API functions).

int i915_gem_gtt_reserve(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww, struct drm_mm_node *node, u64 size, u64 offset, unsigned long color, unsigned int flags)
reserve a node in an address_space (GTT)

Parameters

struct i915_address_space *vm
the struct i915_address_space

struct i915_gem_ww_ctx *ww
An optional struct i915_gem_ww_ctx.

struct drm_mm_node *node
the struct drm_mm_node (typically i915_vma.mode)

u64 size
how much space to allocate inside the GTT, must be #I915_GTT_PAGE_SIZE aligned

u64 offset
where to insert inside the GTT, must be #I915_GTT_MIN_ALIGNMENT aligned, and the node (offset + size) must fit within the address space

unsigned long color
color to apply to node, if this node is not from a VMA, color must be #I915_COLOR_UNEVICTABLE

unsigned int flags
control search and eviction behaviour

Description

i915_gem_gtt_reserve() tries to insert the node at the exact offset inside the address space (using size and color). If the node does not fit, it tries to evict any overlapping nodes from the GTT, including any neighbouring nodes if the colors do not match (to ensure guard pages between differing domains). See i915_gem_evict_for_node() for the gory details on the eviction algorithm. #PIN_NONBLOCK may used to prevent waiting on evicting active overlapping objects, and any overlapping node that is pinned or marked as unevictable will also result in failure.

Return

0 on success, -ENOSPC if no suitable hole is found, -EINTR if asked to wait for eviction and interrupted.

int i915_gem_gtt_insert(struct i915_address_space *vm, struct i915_gem_ww_ctx *ww, struct drm_mm_node *node, u64 size, u64 alignment, unsigned long color, u64 start, u64 end, unsigned int flags)
insert a node into an address_space (GTT)

Parameters

struct i915_address_space *vm
the struct i915_address_space

struct i915_gem_ww_ctx *ww
An optional struct i915_gem_ww_ctx.

struct drm_mm_node *node
the struct drm_mm_node (typically i915_vma.node)

u64 size
how much space to allocate inside the GTT, must be #I915_GTT_PAGE_SIZE aligned

u64 alignment
required alignment of starting offset, may be 0 but if specified, this must be a power-of-two and at least #I915_GTT_MIN_ALIGNMENT

unsigned long color
color to apply to node

u64 start
start of any range restriction inside GTT (0 for all), must be #I915_GTT_PAGE_SIZE aligned

u64 end
end of any range restriction inside GTT (U64_MAX for all), must be #I915_GTT_PAGE_SIZE aligned if not U64_MAX

unsigned int flags
control search and eviction behaviour

Description

i915_gem_gtt_insert() first searches for an available hole into which is can insert the node. The hole address is aligned to alignment and its size must then fit entirely within the [start, end] bounds. The nodes on either side of the hole must match color, or else a guard page will be inserted between the two nodes (or the node evicted). If no suitable hole is found, first a victim is randomly selected and tested for eviction, otherwise then the LRU list of objects within the GTT is scanned to find the first set of replacement nodes to create the hole. Those old overlapping nodes are evicted from the GTT (and so must be rebound before any future use). Any node that is currently pinned cannot be evicted (see i915_vma_pin()). Similar if the node’s VMA is currently active and #PIN_NONBLOCK is specified, that node is also skipped when searching for an eviction candidate. See i915_gem_evict_something() for the gory details on the eviction algorithm.

Return

0 on success, -ENOSPC if no suitable hole is found, -EINTR if asked to wait for eviction and interrupted.

GTT Fences and Swizzling
void i915_vma_revoke_fence(struct i915_vma *vma)
force-remove fence for a VMA

Parameters

struct i915_vma *vma
vma to map linearly (not through a fence reg)

Description

This function force-removes any fence from the given object, which is useful if the kernel wants to do untiled GTT access.

int i915_vma_pin_fence(struct i915_vma *vma)
set up fencing for a vma

Parameters

struct i915_vma *vma
vma to map through a fence reg

Description

When mapping objects through the GTT, userspace wants to be able to write to them without having to worry about swizzling if the object is tiled. This function walks the fence regs looking for a free one for obj, stealing one if it can’t find any.

It then sets up the reg based on the object’s properties: address, pitch and tiling format.

For an untiled surface, this removes any existing fence.

Return

0 on success, negative error code on failure.

struct i915_fence_reg *i915_reserve_fence(struct i915_ggtt *ggtt)
Reserve a fence for vGPU

Parameters

struct i915_ggtt *ggtt
Global GTT

Description

This function walks the fence regs looking for a free one and remove it from the fence_list. It is used to reserve fence for vGPU to use.

void i915_unreserve_fence(struct i915_fence_reg *fence)
Reclaim a reserved fence

Parameters

struct i915_fence_reg *fence
the fence reg

Description

This function add a reserved fence register from vGPU to the fence_list.

void intel_ggtt_restore_fences(struct i915_ggtt *ggtt)
restore fence state

Parameters

struct i915_ggtt *ggtt
Global GTT

Description

Restore the hw fence state to match the software tracking again, to be called after a gpu reset and on resume. Note that on runtime suspend we only cancel the fences, to be reacquired by the user later.

void detect_bit_6_swizzle(struct i915_ggtt *ggtt)
detect bit 6 swizzling pattern

Parameters

struct i915_ggtt *ggtt
Global GGTT

Description

Detects bit 6 swizzling of address lookup between IGD access and CPU access through main memory.

void i915_gem_object_do_bit_17_swizzle(struct drm_i915_gem_object *obj, struct sg_table *pages)
fixup bit 17 swizzling

Parameters

struct drm_i915_gem_object *obj
i915 GEM buffer object

struct sg_table *pages
the scattergather list of physical pages

Description

This function fixes up the swizzling in case any page frame number for this object has changed in bit 17 since that state has been saved with i915_gem_object_save_bit_17_swizzle().

This is called when pinning backing storage again, since the kernel is free to move unpinned backing storage around (either by directly moving pages or by swapping them out and back in again).

void i915_gem_object_save_bit_17_swizzle(struct drm_i915_gem_object *obj, struct sg_table *pages)
save bit 17 swizzling

Parameters

struct drm_i915_gem_object *obj
i915 GEM buffer object

struct sg_table *pages
the scattergather list of physical pages

Description

This function saves the bit 17 of each page frame number so that swizzling can be fixed up later on with i915_gem_object_do_bit_17_swizzle(). This must be called before the backing storage can be unpinned.

Global GTT Fence Handling
Important to avoid confusions: “fences” in the i915 driver are not execution fences used to track command completion but hardware detiler objects which wrap a given range of the global GTT. Each platform has only a fairly limited set of these objects.

Fences are used to detile GTT memory mappings. They’re also connected to the hardware frontbuffer render tracking and hence interact with frontbuffer compression. Furthermore on older platforms fences are required for tiled objects used by the display engine. They can also be used by the render engine - they’re required for blitter commands and are optional for render commands. But on gen4+ both display (with the exception of fbc) and rendering have their own tiling state bits and don’t need fences.

Also note that fences only support X and Y tiling and hence can’t be used for the fancier new tiling formats like W, Ys and Yf.

Finally note that because fences are such a restricted resource they’re dynamically associated with objects. Furthermore fence state is committed to the hardware lazily to avoid unnecessary stalls on gen2/3. Therefore code must explicitly call i915_gem_object_get_fence() to synchronize fencing status for cpu access. Also note that some code wants an unfenced view, for those cases the fence can be removed forcefully with i915_gem_object_put_fence().

Internally these functions will synchronize with userspace access by removing CPU ptes into GTT mmaps (not the GTT ptes themselves) as needed.

Hardware Tiling and Swizzling Details
The idea behind tiling is to increase cache hit rates by rearranging pixel data so that a group of pixel accesses are in the same cacheline. Performance improvement from doing this on the back/depth buffer are on the order of 30%.

Intel architectures make this somewhat more complicated, though, by adjustments made to addressing of data when the memory is in interleaved mode (matched pairs of DIMMS) to improve memory bandwidth. For interleaved memory, the CPU sends every sequential 64 bytes to an alternate memory channel so it can get the bandwidth from both.

The GPU also rearranges its accesses for increased bandwidth to interleaved memory, and it matches what the CPU does for non-tiled. However, when tiled it does it a little differently, since one walks addresses not just in the X direction but also Y. So, along with alternating channels when bit 6 of the address flips, it also alternates when other bits flip -- Bits 9 (every 512 bytes, an X tile scanline) and 10 (every two X tile scanlines) are common to both the 915 and 965-class hardware.

The CPU also sometimes XORs in higher bits as well, to improve bandwidth doing strided access like we do so frequently in graphics. This is called “Channel XOR Randomization” in the MCH documentation. The result is that the CPU is XORing in either bit 11 or bit 17 to bit 6 of its address decode.

All of this bit 6 XORing has an effect on our memory management, as we need to make sure that the 3d driver can correctly address object contents.

If we don’t have interleaved memory, all tiling is safe and no swizzling is required.

When bit 17 is XORed in, we simply refuse to tile at all. Bit 17 is not just a page offset, so as we page an object out and back in, individual pages in it will have different bit 17 addresses, resulting in each 64 bytes being swapped with its neighbor!

Otherwise, if interleaved, we have to tell the 3d driver what the address swizzling it needs to do is, since it’s writing with the CPU to the pages (bit 6 and potentially bit 11 XORed in), and the GPU is reading from the pages (bit 6, 9, and 10 XORed in), resulting in a cumulative bit swizzling required by the CPU of XORing in bit 6, 9, 10, and potentially 11, in order to match what the GPU expects.

Object Tiling IOCTLs
u32 i915_gem_fence_size(struct drm_i915_private *i915, u32 size, unsigned int tiling, unsigned int stride)
required global GTT size for a fence

Parameters

struct drm_i915_private *i915
i915 device

u32 size
object size

unsigned int tiling
tiling mode

unsigned int stride
tiling stride

Description

Return the required global GTT size for a fence (view of a tiled object), taking into account potential fence register mapping.

u32 i915_gem_fence_alignment(struct drm_i915_private *i915, u32 size, unsigned int tiling, unsigned int stride)
required global GTT alignment for a fence

Parameters

struct drm_i915_private *i915
i915 device

u32 size
object size

unsigned int tiling
tiling mode

unsigned int stride
tiling stride

Description

Return the required global GTT alignment for a fence (a view of a tiled object), taking into account potential fence register mapping.

int i915_gem_set_tiling_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
IOCTL handler to set tiling mode

Parameters

struct drm_device *dev
DRM device

void *data
data pointer for the ioctl

struct drm_file *file
DRM file for the ioctl call

Description

Sets the tiling mode of an object, returning the required swizzling of bit 6 of addresses in the object.

Called by the user via ioctl.

Return

Zero on success, negative errno on failure.

int i915_gem_get_tiling_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
IOCTL handler to get tiling mode

Parameters

struct drm_device *dev
DRM device

void *data
data pointer for the ioctl

struct drm_file *file
DRM file for the ioctl call

Description

Returns the current tiling mode and required bit 6 swizzling for the object.

Called by the user via ioctl.

Return

Zero on success, negative errno on failure.

i915_gem_set_tiling_ioctl() and i915_gem_get_tiling_ioctl() is the userspace interface to declare fence register requirements.

In principle GEM doesn’t care at all about the internal data layout of an object, and hence it also doesn’t care about tiling or swizzling. There’s two exceptions:

For X and Y tiling the hardware provides detilers for CPU access, so called fences. Since there’s only a limited amount of them the kernel must manage these, and therefore userspace must tell the kernel the object tiling if it wants to use fences for detiling.

On gen3 and gen4 platforms have a swizzling pattern for tiled objects which depends upon the physical page frame number. When swapping such objects the page frame number might change and the kernel must be able to fix this up and hence now the tiling. Note that on a subset of platforms with asymmetric memory channel population the swizzling pattern changes in an unknown way, and for those the kernel simply forbids swapping completely.

Since neither of this applies for new tiling layouts on modern platforms like W, Ys and Yf tiling GEM only allows object tiling to be set to X or Y tiled. Anything else can be handled in userspace entirely without the kernel’s invovlement.

Protected Objects
PXP (Protected Xe Path) is a feature available in Gen12 and newer platforms. It allows execution and flip to display of protected (i.e. encrypted) objects. The SW support is enabled via the CONFIG_DRM_I915_PXP kconfig.

Objects can opt-in to PXP encryption at creation time via the I915_GEM_CREATE_EXT_PROTECTED_CONTENT create_ext flag. For objects to be correctly protected they must be used in conjunction with a context created with the I915_CONTEXT_PARAM_PROTECTED_CONTENT flag. See the documentation of those two uapi flags for details and restrictions.

Protected objects are tied to a pxp session; currently we only support one session, which i915 manages and whose index is available in the uapi (I915_PROTECTED_CONTENT_DEFAULT_SESSION) for use in instructions targeting protected objects. The session is invalidated by the HW when certain events occur (e.g. suspend/resume). When this happens, all the objects that were used with the session are marked as invalid and all contexts marked as using protected content are banned. Any further attempt at using them in an execbuf call is rejected, while flips are converted to black frames.

Some of the PXP setup operations are performed by the Management Engine, which is handled by the mei driver; communication between i915 and mei is performed via the mei_pxp component module.

struct intel_pxp
pxp state

Definition:

struct intel_pxp {
    struct intel_gt *ctrl_gt;
    bool platform_cfg_is_bad;
    u32 kcr_base;
    struct gsccs_session_resources {
        u64 host_session_handle;
        struct intel_context *ce;
        struct i915_vma *pkt_vma;
        void *pkt_vaddr;
        struct i915_vma *bb_vma;
        void *bb_vaddr;
    } gsccs_res;
    struct i915_pxp_component *pxp_component;
    struct device_link *dev_link;
    bool pxp_component_added;
    struct intel_context *ce;
    struct mutex arb_mutex;
    bool arb_is_valid;
    u32 key_instance;
    struct mutex tee_mutex;
    struct {
        struct drm_i915_gem_object *obj;
        void *vaddr;
    } stream_cmd;
    bool hw_state_invalidated;
    bool irq_enabled;
    struct completion termination;
    struct work_struct session_work;
    u32 session_events;
#define PXP_TERMINATION_REQUEST  BIT(0);
#define PXP_TERMINATION_COMPLETE BIT(1);
#define PXP_INVAL_REQUIRED       BIT(2);
#define PXP_EVENT_TYPE_IRQ       BIT(3);
};
Members

ctrl_gt
poiner to the tile that owns the controls for PXP subsystem assets that the VDBOX, the KCR engine (and GSC CS depending on the platform)

platform_cfg_is_bad
used to track if any prior arb session creation resulted in a failure that was caused by a platform configuration issue, meaning that failure will not get resolved without a change to the platform (not kernel) such as BIOS configuration, firwmware update, etc. This bool gets reflected when GET_PARAM:I915_PARAM_PXP_STATUS is called.

kcr_base
base mmio offset for the KCR engine which is different on legacy platforms vs newer platforms where the KCR is inside the media-tile.

gsccs_res
resources for request submission for platforms that have a GSC engine.

pxp_component
i915_pxp_component struct of the bound mei_pxp module. Only set and cleared inside component bind/unbind functions, which are protected by tee_mutex.

dev_link
Enforce module relationship for power management ordering.

pxp_component_added
track if the pxp component has been added. Set and cleared in tee init and fini functions respectively.

ce
kernel-owned context used for PXP operations

arb_mutex
protects arb session start

arb_is_valid
tracks arb session status. After a teardown, the arb session can still be in play on the HW even if the keys are gone, so we can’t rely on the HW state of the session to know if it’s valid and need to track the status in SW.

key_instance
tracks which key instance we’re on, so we can use it to determine if an object was created using the current key or a previous one.

tee_mutex
protects the tee channel binding and messaging.

stream_cmd
LMEM obj used to send stream PXP commands to the GSC

hw_state_invalidated
if the HW perceives an attack on the integrity of the encryption it will invalidate the keys and expect SW to re-initialize the session. We keep track of this state to make sure we only re-start the arb session when required.

irq_enabled
tracks the status of the kcr irqs

termination
tracks the status of a pending termination. Only re-initialized under gt->irq_lock and completed in session_work.

session_work
worker that manages session events.

session_events
pending session events, protected with gt->irq_lock.

Microcontrollers
Starting from gen9, three microcontrollers are available on the HW: the graphics microcontroller (GuC), the HEVC/H.265 microcontroller (HuC) and the display microcontroller (DMC). The driver is responsible for loading the firmwares on the microcontrollers; the GuC and HuC firmwares are transferred to WOPCM using the DMA engine, while the DMC firmware is written through MMIO.

WOPCM
WOPCM Layout
The layout of the WOPCM will be fixed after writing to GuC WOPCM size and offset registers whose values are calculated and determined by HuC/GuC firmware size and set of hardware requirements/restrictions as shown below:

  +=========> +====================+ <== WOPCM Top
  ^           |  HW contexts RSVD  |
  |     +===> +====================+ <== GuC WOPCM Top
  |     ^     |                    |
  |     |     |                    |
  |     |     |                    |
  |    GuC    |                    |
  |   WOPCM   |                    |
  |    Size   +--------------------+
WOPCM   |     |    GuC FW RSVD     |
  |     |     +--------------------+
  |     |     |   GuC Stack RSVD   |
  |     |     +------------------- +
  |     v     |   GuC WOPCM RSVD   |
  |     +===> +====================+ <== GuC WOPCM base
  |           |     WOPCM RSVD     |
  |           +------------------- + <== HuC Firmware Top
  v           |      HuC FW        |
  +=========> +====================+ <== WOPCM Base
GuC accessible WOPCM starts at GuC WOPCM base and ends at GuC WOPCM top. The top part of the WOPCM is reserved for hardware contexts (e.g. RC6 context).

GuC
The GuC is a microcontroller inside the GT HW, introduced in gen9. The GuC is designed to offload some of the functionality usually performed by the host driver; currently the main operations it can take care of are:

Authentication of the HuC, which is required to fully enable HuC usage.

Low latency graphics context scheduling (a.k.a. GuC submission).

GT Power management.

The enable_guc module parameter can be used to select which of those operations to enable within GuC. Note that not all the operations are supported on all gen9+ platforms.

Enabling the GuC is not mandatory and therefore the firmware is only loaded if at least one of the operations is selected. However, not loading the GuC might result in the loss of some features that do require the GuC (currently just the HuC, but more are expected to land in the future).

struct intel_guc
Top level structure of GuC.

Definition:

struct intel_guc {
    struct intel_uc_fw fw;
    struct intel_guc_log log;
    struct intel_guc_ct ct;
    struct intel_guc_slpc slpc;
    struct intel_guc_state_capture *capture;
    struct dentry *dbgfs_node;
    struct i915_sched_engine *sched_engine;
    struct i915_request *stalled_request;
    enum {
        STALL_NONE,
        STALL_REGISTER_CONTEXT,
        STALL_MOVE_LRC_TAIL,
        STALL_ADD_REQUEST,
    } submission_stall_reason;
    spinlock_t irq_lock;
    unsigned int msg_enabled_mask;
    atomic_t outstanding_submission_g2h;
    struct xarray tlb_lookup;
    u32 serial_slot;
    u32 next_seqno;
    struct {
        bool enabled;
        void (*reset)(struct intel_guc *guc);
        void (*enable)(struct intel_guc *guc);
        void (*disable)(struct intel_guc *guc);
    } interrupts;
    struct {
        spinlock_t lock;
        struct ida guc_ids;
        int num_guc_ids;
        unsigned long *guc_ids_bitmap;
        struct list_head guc_id_list;
        unsigned int guc_ids_in_use;
        struct list_head destroyed_contexts;
        struct work_struct destroyed_worker;
        struct work_struct reset_fail_worker;
        intel_engine_mask_t reset_fail_mask;
        unsigned int sched_disable_delay_ms;
        unsigned int sched_disable_gucid_threshold;
    } submission_state;
    bool submission_supported;
    bool submission_selected;
    bool submission_initialized;
    struct intel_uc_fw_ver submission_version;
    bool rc_supported;
    bool rc_selected;
    struct i915_vma *ads_vma;
    struct iosys_map ads_map;
    u32 ads_regset_size;
    u32 ads_regset_count[I915_NUM_ENGINES];
    struct guc_mmio_reg *ads_regset;
    u32 ads_golden_ctxt_size;
    u32 ads_waklv_size;
    u32 ads_capture_size;
    struct i915_vma *lrc_desc_pool_v69;
    void *lrc_desc_pool_vaddr_v69;
    struct xarray context_lookup;
    u32 params[GUC_CTL_MAX_DWORDS];
    struct {
        u32 base;
        unsigned int count;
        enum forcewake_domains fw_domains;
    } send_regs;
    i915_reg_t notify_reg;
    u32 mmio_msg;
    struct mutex send_mutex;
    struct {
        spinlock_t lock;
        u64 gt_stamp;
        unsigned long ping_delay;
        struct delayed_work work;
        u32 shift;
        unsigned long last_stat_jiffies;
    } timestamp;
    struct work_struct dead_guc_worker;
    unsigned long last_dead_guc_jiffies;
#ifdef CONFIG_DRM_I915_SELFTEST;
    int number_guc_id_stolen;
    u32 fast_response_selftest;
#endif;
};
Members

fw
the GuC firmware

log
sub-structure containing GuC log related data and objects

ct
the command transport communication channel

slpc
sub-structure containing SLPC related data and objects

capture
the error-state-capture module’s data and objects

dbgfs_node
debugfs node

sched_engine
Global engine used to submit requests to GuC

stalled_request
if GuC can’t process a request for any reason, we save it until GuC restarts processing. No other request can be submitted until the stalled request is processed.

submission_stall_reason
reason why submission is stalled

irq_lock
protects GuC irq state

msg_enabled_mask
mask of events that are processed when receiving an INTEL_GUC_ACTION_DEFAULT G2H message.

outstanding_submission_g2h
number of outstanding GuC to Host responses related to GuC submission, used to determine if the GT is idle

tlb_lookup
xarray to store all pending TLB invalidation requests

serial_slot
id to the initial waiter created in tlb_lookup, which is used only when failed to allocate new waiter.

next_seqno
the next id (sequence number) to allocate.

interrupts
pointers to GuC interrupt-managing functions.

submission_state
sub-structure for submission state protected by single lock

submission_state.lock
protects everything in submission_state, ce->guc_id.id, and ce->guc_id.ref when transitioning in and out of zero

submission_state.guc_ids
used to allocate new guc_ids, single-lrc

submission_state.num_guc_ids
Number of guc_ids, selftest feature to be able to reduce this number while testing.

submission_state.guc_ids_bitmap
used to allocate new guc_ids, multi-lrc

submission_state.guc_id_list
list of intel_context with valid guc_ids but no refs

submission_state.guc_ids_in_use
Number single-lrc guc_ids in use

submission_state.destroyed_contexts
list of contexts waiting to be destroyed (deregistered with the GuC)

submission_state.destroyed_worker
worker to deregister contexts, need as we need to take a GT PM reference and can’t from destroy function as it might be in an atomic context (no sleeping)

submission_state.reset_fail_worker
worker to trigger a GT reset after an engine reset fails

submission_state.reset_fail_mask
mask of engines that failed to reset

submission_state.sched_disable_delay_ms
schedule disable delay, in ms, for contexts

submission_state.sched_disable_gucid_threshold
threshold of min remaining available guc_ids before we start bypassing the schedule disable delay

submission_supported
tracks whether we support GuC submission on the current platform

submission_selected
tracks whether the user enabled GuC submission

submission_initialized
tracks whether GuC submission has been initialised

submission_version
Submission API version of the currently loaded firmware

rc_supported
tracks whether we support GuC rc on the current platform

rc_selected
tracks whether the user enabled GuC rc

ads_vma
object allocated to hold the GuC ADS

ads_map
contents of the GuC ADS

ads_regset_size
size of the save/restore regsets in the ADS

ads_regset_count
number of save/restore registers in the ADS for each engine

ads_regset
save/restore regsets in the ADS

ads_golden_ctxt_size
size of the golden contexts in the ADS

ads_waklv_size
size of workaround KLVs

ads_capture_size
size of register lists in the ADS used for error capture

lrc_desc_pool_v69
object allocated to hold the GuC LRC descriptor pool

lrc_desc_pool_vaddr_v69
contents of the GuC LRC descriptor pool

context_lookup
used to resolve intel_context from guc_id, if a context is present in this structure it is registered with the GuC

params
Control params for fw initialization

send_regs
GuC’s FW specific registers used for sending MMIO H2G

notify_reg
register used to send interrupts to the GuC FW

mmio_msg
notification bitmask that the GuC writes in one of its registers when the CT channel is disabled, to be processed when the channel is back up.

send_mutex
used to serialize the intel_guc_send actions

timestamp
GT timestamp object that stores a copy of the timestamp and adjusts it for overflow using a worker.

timestamp.lock
Lock protecting the below fields and the engine stats.

timestamp.gt_stamp
64-bit extended value of the GT timestamp.

timestamp.ping_delay
Period for polling the GT timestamp for overflow.

timestamp.work
Periodic work to adjust GT timestamp, engine and context usage for overflows.

timestamp.shift
Right shift value for the gpm timestamp

timestamp.last_stat_jiffies
jiffies at last actual stats collection time. We use this timestamp to ensure we don’t oversample the stats because runtime power management events can trigger stats collection at much higher rates than required.

dead_guc_worker
Asynchronous worker thread for forcing a GuC reset. Specifically used when the G2H handler wants to issue a reset. Resets require flushing the G2H queue. So, the G2H processing itself must not trigger a reset directly. Instead, go via this worker.

last_dead_guc_jiffies
timestamp of previous ‘dead guc’ occurrance used to prevent a fundamentally broken system from continuously reloading the GuC.

number_guc_id_stolen
The number of guc_ids that have been stolen

fast_response_selftest
Backdoor to CT handler for fast response selftest

Description

It handles firmware loading and manages client pool. intel_guc owns an i915_sched_engine for submission.

u32 intel_guc_ggtt_offset(struct intel_guc *guc, struct i915_vma *vma)
Get and validate the GGTT offset of vma

Parameters

struct intel_guc *guc
intel_guc structure.

struct i915_vma *vma
i915 graphics virtual memory area.

Description

GuC does not allow any gfx GGTT address that falls into range [0, ggtt.pin_bias), which is reserved for Boot ROM, SRAM and WOPCM. Currently, in order to exclude [0, ggtt.pin_bias) address space from GGTT, all gfx objects used by GuC are allocated with intel_guc_allocate_vma() and pinned with PIN_OFFSET_BIAS along with the value of ggtt.pin_bias.

Return

GGTT offset of the vma.

GuC Firmware Layout
The GuC/HuC firmware layout looks like this:

+======================================================================+
|  Firmware blob                                                       |
+===============+===============+============+============+============+
|  CSS header   |     uCode     |  RSA key   |  modulus   |  exponent  |
+===============+===============+============+============+============+
 <-header size->                 <---header size continued ----------->
 <--- size ----------------------------------------------------------->
                                 <-key size->
                                              <-mod size->
                                                           <-exp size->
The firmware may or may not have modulus key and exponent data. The header, uCode and RSA signature are must-have components that will be used by driver. Length of each components, which is all in dwords, can be found in header. In the case that modulus and exponent are not present in fw, a.k.a truncated image, the length value still appears in header.

Driver will do some basic fw size validation based on the following rules:

Header, uCode and RSA are must-have components.

All firmware components, if they present, are in the sequence illustrated in the layout table above.

Length info of each component can be found in header, in dwords.

Modulus and exponent key are not required by driver. They may not appear in fw. So driver will load a truncated firmware in this case.

Starting from DG2, the HuC is loaded by the GSC instead of i915. The GSC firmware performs all the required integrity checks, we just need to check the version. Note that the header for GSC-managed blobs is different from the CSS used for dma-loaded firmwares.

GuC Memory Management
GuC can’t allocate any memory for its own usage, so all the allocations must be handled by the host driver. GuC accesses the memory via the GGTT, with the exception of the top and bottom parts of the 4GB address space, which are instead re-mapped by the GuC HW to memory location of the FW itself (WOPCM) or other parts of the HW. The driver must take care not to place objects that the GuC is going to access in these reserved ranges. The layout of the GuC address space is shown below:

   +===========> +====================+ <== FFFF_FFFF
   ^             |      Reserved      |
   |             +====================+ <== GUC_GGTT_TOP
   |             |                    |
   |             |        DRAM        |
  GuC            |                    |
Address    +===> +====================+ <== GuC ggtt_pin_bias
 Space     ^     |                    |
   |       |     |                    |
   |      GuC    |        GuC         |
   |     WOPCM   |       WOPCM        |
   |      Size   |                    |
   |       |     |                    |
   v       v     |                    |
   +=======+===> +====================+ <== 0000_0000
The lower part of GuC Address Space [0, ggtt_pin_bias) is mapped to GuC WOPCM while upper part of GuC Address Space [ggtt_pin_bias, GUC_GGTT_TOP) is mapped to DRAM. The value of the GuC ggtt_pin_bias is the GuC WOPCM size.

struct i915_vma *intel_guc_allocate_vma(struct intel_guc *guc, u32 size)
Allocate a GGTT VMA for GuC usage

Parameters

struct intel_guc *guc
the guc

u32 size
size of area to allocate (both virtual space and memory)

Description

This is a wrapper to create an object for use with the GuC. In order to use it inside the GuC, an object needs to be pinned lifetime, so we allocate both some backing storage and a range inside the Global GTT. We must pin it in the GGTT somewhere other than than [0, GUC ggtt_pin_bias) because that range is reserved inside GuC.

Return

A i915_vma if successful, otherwise an ERR_PTR.

GuC-specific firmware loader
int intel_guc_fw_upload(struct intel_guc *guc)
load GuC uCode to device

Parameters

struct intel_guc *guc
intel_guc structure

Description

Called from intel_uc_init_hw() during driver load, resume from sleep and after a GPU reset.

The firmware image should have already been fetched into memory, so only check that fetch succeeded, and then transfer the image to the h/w.

Return

non-zero code on error

GuC-based command submission
The Scratch registers: There are 16 MMIO-based registers start from 0xC180. The kernel driver writes a value to the action register (SOFT_SCRATCH_0) along with any data. It then triggers an interrupt on the GuC via another register write (0xC4C8). Firmware writes a success/fail code back to the action register after processes the request. The kernel driver polls waiting for this update and then proceeds.

Command Transport buffers (CTBs): Covered in detail in other sections but CTBs (Host to GuC - H2G, GuC to Host - G2H) are a message interface between the i915 and GuC.

Context registration: Before a context can be submitted it must be registered with the GuC via a H2G. A unique guc_id is associated with each context. The context is either registered at request creation time (normal operation) or at submission time (abnormal operation, e.g. after a reset).

Context submission: The i915 updates the LRC tail value in memory. The i915 must enable the scheduling of the context within the GuC for the GuC to actually consider it. Therefore, the first time a disabled context is submitted we use a schedule enable H2G, while follow up submissions are done via the context submit H2G, which informs the GuC that a previously enabled context has new work available.

Context unpin: To unpin a context a H2G is used to disable scheduling. When the corresponding G2H returns indicating the scheduling disable operation has completed it is safe to unpin the context. While a disable is in flight it isn’t safe to resubmit the context so a fence is used to stall all future requests of that context until the G2H is returned. Because this interaction with the GuC takes a non-zero amount of time we delay the disabling of scheduling after the pin count goes to zero by a configurable period of time (see SCHED_DISABLE_DELAY_MS). The thought is this gives the user a window of time to resubmit something on the context before doing this costly operation. This delay is only done if the context isn’t closed and the guc_id usage is less than a threshold (see NUM_SCHED_DISABLE_GUC_IDS_THRESHOLD).

Context deregistration: Before a context can be destroyed or if we steal its guc_id we must deregister the context with the GuC via H2G. If stealing the guc_id it isn’t safe to submit anything to this guc_id until the deregister completes so a fence is used to stall all requests associated with this guc_id until the corresponding G2H returns indicating the guc_id has been deregistered.

submission_state.guc_ids: Unique number associated with private GuC context data passed in during context registration / submission / deregistration. 64k available. Simple ida is used for allocation.

Stealing guc_ids: If no guc_ids are available they can be stolen from another context at request creation time if that context is unpinned. If a guc_id can’t be found we punt this problem to the user as we believe this is near impossible to hit during normal use cases.

Locking: In the GuC submission code we have 3 basic spin locks which protect everything. Details about each below.

sched_engine->lock This is the submission lock for all contexts that share an i915 schedule engine (sched_engine), thus only one of the contexts which share a sched_engine can be submitting at a time. Currently only one sched_engine is used for all of GuC submission but that could change in the future.

guc->submission_state.lock Global lock for GuC submission state. Protects guc_ids and destroyed contexts list.

ce->guc_state.lock Protects everything under ce->guc_state. Ensures that a context is in the correct state before issuing a H2G. e.g. We don’t issue a schedule disable on a disabled context (bad idea), we don’t issue a schedule enable when a schedule disable is in flight, etc... Also protects list of inflight requests on the context and the priority management state. Lock is individual to each context.

Lock ordering rules: sched_engine->lock -> ce->guc_state.lock guc->submission_state.lock -> ce->guc_state.lock

Reset races: When a full GT reset is triggered it is assumed that some G2H responses to H2Gs can be lost as the GuC is also reset. Losing these G2H can prove to be fatal as we do certain operations upon receiving a G2H (e.g. destroy contexts, release guc_ids, etc...). When this occurs we can scrub the context state and cleanup appropriately, however this is quite racey. To avoid races, the reset code must disable submission before scrubbing for the missing G2H, while the submission code must check for submission being disabled and skip sending H2Gs and updating context states when it is. Both sides must also make sure to hold the relevant locks.

GuC ABI
HXG Message

All messages exchanged with GuC are defined using 32 bit dwords. First dword is treated as a message header. Remaining dwords are optional.

Bits

Description

0

31

ORIGIN - originator of the message
GUC_HXG_ORIGIN_HOST = 0

GUC_HXG_ORIGIN_GUC = 1

30:28

TYPE - message type
GUC_HXG_TYPE_REQUEST = 0

GUC_HXG_TYPE_EVENT = 1

GUC_HXG_TYPE_FAST_REQUEST = 2

GUC_HXG_TYPE_NO_RESPONSE_BUSY = 3

GUC_HXG_TYPE_NO_RESPONSE_RETRY = 5

GUC_HXG_TYPE_RESPONSE_FAILURE = 6

GUC_HXG_TYPE_RESPONSE_SUCCESS = 7

27:0

AUX - auxiliary data (depends on TYPE)

1

31:0

PAYLOAD - optional payload (depends on TYPE)

...

n

31:0

HXG Request

The HXG Request message should be used to initiate synchronous activity for which confirmation or return data is expected.

The recipient of this message shall use HXG Response, HXG Failure or HXG Retry message as a definite reply, and may use HXG Busy message as a intermediate reply.

Format of DATA0 and all DATAn fields depends on the ACTION code.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_REQUEST

27:16

DATA0 - request data (depends on ACTION)

15:0

ACTION - requested action code

1

31:0

DATAn - optional data (depends on ACTION)

...

n

31:0

HXG Fast Request

The HXG Request message should be used to initiate asynchronous activity for which confirmation or return data is not expected.

If confirmation is required then HXG Request shall be used instead.

The recipient of this message may only use HXG Failure message if it was unable to accept this request (like invalid data).

Format of HXG Fast Request message is same as HXG Request except TYPE.

Bits

Description

0

31

ORIGIN - see HXG Message

30:28

TYPE = GUC_HXG_TYPE_FAST_REQUEST

27:16

DATA0 - see HXG Request

15:0

ACTION - see HXG Request

...

DATAn - see HXG Request

HXG Event

The HXG Event message should be used to initiate asynchronous activity that does not involves immediate confirmation nor data.

Format of DATA0 and all DATAn fields depends on the ACTION code.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_EVENT

27:16

DATA0 - event data (depends on ACTION)

15:0

ACTION - event action code

1

31:0

DATAn - optional event data (depends on ACTION)

...

n

31:0

HXG Busy

The HXG Busy message may be used to acknowledge reception of the HXG Request message if the recipient expects that it processing will be longer than default timeout.

The COUNTER field may be used as a progress indicator.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_NO_RESPONSE_BUSY

27:0

COUNTER - progress indicator

HXG Retry

The HXG Retry message should be used by recipient to indicate that the HXG Request message was dropped and it should be resent again.

The REASON field may be used to provide additional information.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_NO_RESPONSE_RETRY

27:0

REASON - reason for retry
GUC_HXG_RETRY_REASON_UNSPECIFIED = 0

HXG Failure

The HXG Failure message shall be used as a reply to the HXG Request message that could not be processed due to an error.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_RESPONSE_FAILURE

27:16

HINT - additional error hint

15:0

ERROR - error/result code

HXG Response

The HXG Response message shall be used as a reply to the HXG Request message that was successfully processed without an error.

Bits

Description

0

31

ORIGIN

30:28

TYPE = GUC_HXG_TYPE_RESPONSE_SUCCESS

27:0

DATA0 - data (depends on ACTION from HXG Request)

1

31:0

DATAn - data (depends on ACTION from HXG Request)

...

n

31:0

GuC MMIO based communication

The MMIO based communication between Host and GuC relies on special hardware registers which format could be defined by the software (so called scratch registers).

Each MMIO based message, both Host to GuC (H2G) and GuC to Host (G2H) messages, which maximum length depends on number of available scratch registers, is directly written into those scratch registers.

For Gen9+, there are 16 software scratch registers 0xC180-0xC1B8, but no H2G command takes more than 4 parameters and the GuC firmware itself uses an 4-element array to store the H2G message.

For Gen11+, there are additional 4 registers 0x190240-0x19024C, which are, regardless on lower count, preferred over legacy ones.

The MMIO based communication is mainly used during driver initialization phase to setup the CTB based communication that will be used afterwards.

MMIO HXG Message

Format of the MMIO messages follows definitions of HXG Message.

Bits

Description

0

31:0

[Embedded HXG Message]

...

n

31:0

CT Buffer

Circular buffer used to send CTB Message

CTB Descriptor

Bits

Description

0

31:0

HEAD - offset (in dwords) to the last dword that was read from the CT Buffer. It can only be updated by the receiver.

1

31:0

TAIL - offset (in dwords) to the last dword that was written to the CT Buffer. It can only be updated by the sender.

2

31:0

STATUS - status of the CTB

GUC_CTB_STATUS_NO_ERROR = 0 (normal operation)

GUC_CTB_STATUS_OVERFLOW = 1 (head/tail too large)

GUC_CTB_STATUS_UNDERFLOW = 2 (truncated message)

GUC_CTB_STATUS_MISMATCH = 4 (head/tail modified)

GUC_CTB_STATUS_UNUSED = 8 (CTB is not in use)

...

RESERVED = MBZ

15

31:0

RESERVED = MBZ

CTB Message

Bits

Description

0

31:16

FENCE - message identifier

15:12

FORMAT - format of the CTB message
GUC_CTB_FORMAT_HXG = 0 - see CTB HXG Message

11:8

RESERVED

7:0

NUM_DWORDS - length of the CTB message (w/o header)

1

31:0

optional (depends on FORMAT)

...

n

31:0

CTB HXG Message

Bits

Description

0

31:16

FENCE

15:12

FORMAT = GUC_CTB_FORMAT_HXG

11:8

RESERVED = MBZ

7:0

NUM_DWORDS = length (in dwords) of the embedded HXG message

1

31:0

[Embedded HXG Message]

...

n

31:0

CTB based communication

The CTB (command transport buffer) communication between Host and GuC is based on u32 data stream written to the shared buffer. One buffer can be used to transmit data only in one direction (one-directional channel).

Current status of the each buffer is stored in the buffer descriptor. Buffer descriptor holds tail and head fields that represents active data stream. The tail field is updated by the data producer (sender), and head field is updated by the data consumer (receiver):

+------------+
| DESCRIPTOR |          +=================+============+========+
+============+          |                 | MESSAGE(s) |        |
| address    |--------->+=================+============+========+
+------------+
| head       |          ^-----head--------^
+------------+
| tail       |          ^---------tail-----------------^
+------------+
| size       |          ^---------------size--------------------^
+------------+
Each message in data stream starts with the single u32 treated as a header, followed by optional set of u32 data that makes message specific payload:

+------------+---------+---------+---------+
|         MESSAGE                          |
+------------+---------+---------+---------+
|   msg[0]   |   [1]   |   ...   |  [n-1]  |
+------------+---------+---------+---------+
|   MESSAGE  |       MESSAGE PAYLOAD       |
+   HEADER   +---------+---------+---------+
|            |    0    |   ...   |    n    |
+======+=====+=========+=========+=========+
| 31:16| code|         |         |         |
+------+-----+         |         |         |
|  15:5|flags|         |         |         |
+------+-----+         |         |         |
|   4:0|  len|         |         |         |
+------+-----+---------+---------+---------+

             ^-------------len-------------^
The message header consists of:

len, indicates length of the message payload (in u32)

code, indicates message code

flags, holds various bits to control message handling

HOST2GUC_SELF_CFG

This message is used by Host KMD to setup of the GuC Self Config KLVs.

This message must be sent as MMIO HXG Message.

Bits

Description

0

31

ORIGIN = GUC_HXG_ORIGIN_HOST

30:28

TYPE = GUC_HXG_TYPE_REQUEST

27:16

DATA0 = MBZ

15:0

ACTION = GUC_ACTION_HOST2GUC_SELF_CFG = 0x0508

1

31:16

KLV_KEY - KLV key, see GuC Self Config KLVs

15:0

KLV_LEN - KLV length

32 bit KLV = 1

64 bit KLV = 2

2

31:0

VALUE32 - Bits 31-0 of the KLV value

3

31:0

VALUE64 - Bits 63-32 of the KLV value (KLV_LEN = 2)

Bits

Description

0

31

ORIGIN = GUC_HXG_ORIGIN_GUC

30:28

TYPE = GUC_HXG_TYPE_RESPONSE_SUCCESS

27:0

DATA0 = NUM - 1 if KLV was parsed, 0 if not recognized

HOST2GUC_CONTROL_CTB

This H2G action allows Vf Host to enable or disable H2G and G2H CT Buffer.

This message must be sent as MMIO HXG Message.

Bits

Description

0

31

ORIGIN = GUC_HXG_ORIGIN_HOST

30:28

TYPE = GUC_HXG_TYPE_REQUEST

27:16

DATA0 = MBZ

15:0

ACTION = GUC_ACTION_HOST2GUC_CONTROL_CTB = 0x4509

1

31:0

CONTROL - control CTB based communication

GUC_CTB_CONTROL_DISABLE = 0

GUC_CTB_CONTROL_ENABLE = 1

Bits

Description

0

31

ORIGIN = GUC_HXG_ORIGIN_GUC

30:28

TYPE = GUC_HXG_TYPE_RESPONSE_SUCCESS

27:0

DATA0 = MBZ

GuC KLV

Bits

Description

0

31:16

KEY - KLV key identifier
GuC Self Config KLVs

15:0

LEN - length of VALUE (in 32bit dwords)

1

31:0

VALUE - actual value of the KLV (format depends on KEY)

...

n

31:0

GuC Self Config KLVs

GuC KLV keys available for use with HOST2GUC_SELF_CFG.

GUC_KLV_SELF_CFG_H2G_CTB_ADDR0x0902
Refers to 64 bit Global Gfx address of H2G CT Buffer. Should be above WOPCM address but below APIC base address for native mode.

GUC_KLV_SELF_CFG_H2G_CTB_DESCRIPTOR_ADDR0x0903
Refers to 64 bit Global Gfx address of H2G CTB Descriptor. Should be above WOPCM address but below APIC base address for native mode.

GUC_KLV_SELF_CFG_H2G_CTB_SIZE0x0904
Refers to size of H2G CT Buffer in bytes. Should be a multiple of 4K.

GUC_KLV_SELF_CFG_G2H_CTB_ADDR0x0905
Refers to 64 bit Global Gfx address of G2H CT Buffer. Should be above WOPCM address but below APIC base address for native mode.

GUC_KLV_SELF_CFG_G2H_CTB_DESCRIPTOR_ADDR0x0906
Refers to 64 bit Global Gfx address of G2H CTB Descriptor. Should be above WOPCM address but below APIC base address for native mode.

GUC_KLV_SELF_CFG_G2H_CTB_SIZE0x0907
Refers to size of G2H CT Buffer in bytes. Should be a multiple of 4K.

HuC
The HuC is a dedicated microcontroller for usage in media HEVC (High Efficiency Video Coding) operations. Userspace can directly use the firmware capabilities by adding HuC specific commands to batch buffers.

The kernel driver is only responsible for loading the HuC firmware and triggering its security authentication. This is done differently depending on the platform:

older platforms (from Gen9 to most Gen12s): the load is performed via DMA and the authentication via GuC

DG2: load and authentication are both performed via GSC.

MTL and newer platforms: the load is performed via DMA (same as with not-DG2 older platforms), while the authentication is done in 2-steps, a first auth for clear-media workloads via GuC and a second one for all workloads via GSC.

On platforms where the GuC does the authentication, to correctly do so the HuC binary must be loaded before the GuC one. Loading the HuC is optional; however, not using the HuC might negatively impact power usage and/or performance of media workloads, depending on the use-cases. HuC must be reloaded on events that cause the WOPCM to lose its contents (S3/S4, FLR); on older platforms the HuC must also be reloaded on GuC/GT reset, while on newer ones it will survive that.

See https://github.com/intel/media-driver for the latest details on HuC functionality.

int intel_huc_auth(struct intel_huc *huc, enum intel_huc_authentication_type type)
Authenticate HuC uCode

Parameters

struct intel_huc *huc
intel_huc structure

enum intel_huc_authentication_type type
authentication type (via GuC or via GSC)

Description

Called after HuC and GuC firmware loading during intel_uc_init_hw().

This function invokes the GuC action to authenticate the HuC firmware, passing the offset of the RSA signature to intel_guc_auth_huc(). It then waits for up to 50ms for firmware verification ACK.

HuC Memory Management
Similarly to the GuC, the HuC can’t do any memory allocations on its own, with the difference being that the allocations for HuC usage are handled by the userspace driver instead of the kernel one. The HuC accesses the memory via the PPGTT belonging to the context loaded on the VCS executing the HuC-specific commands.

HuC Firmware Layout
The HuC FW layout is the same as the GuC one, see GuC Firmware Layout

DMC
See DMC Firmware Support

Tracing
This sections covers all things related to the tracepoints implemented in the i915 driver.

i915_ppgtt_create and i915_ppgtt_release
With full ppgtt enabled each process using drm will allocate at least one translation table. With these traces it is possible to keep track of the allocation and of the lifetime of the tables; this can be used during testing/debug to verify that we are not leaking ppgtts. These traces identify the ppgtt through the vm pointer, which is also printed by the i915_vma_bind and i915_vma_unbind tracepoints.

i915_context_create and i915_context_free
These tracepoints are used to track creation and deletion of contexts. If full ppgtt is enabled, they also print the address of the vm assigned to the context.

Perf
Overview
Gen graphics supports a large number of performance counters that can help driver and application developers understand and optimize their use of the GPU.

This i915 perf interface enables userspace to configure and open a file descriptor representing a stream of GPU metrics which can then be read() as a stream of sample records.

The interface is particularly suited to exposing buffered metrics that are captured by DMA from the GPU, unsynchronized with and unrelated to the CPU.

Streams representing a single context are accessible to applications with a corresponding drm file descriptor, such that OpenGL can use the interface without special privileges. Access to system-wide metrics requires root privileges by default, unless changed via the dev.i915.perf_event_paranoid sysctl option.

Comparison with Core Perf
The interface was initially inspired by the core Perf infrastructure but some notable differences are:

i915 perf file descriptors represent a “stream” instead of an “event”; where a perf event primarily corresponds to a single 64bit value, while a stream might sample sets of tightly-coupled counters, depending on the configuration. For example the Gen OA unit isn’t designed to support orthogonal configurations of individual counters; it’s configured for a set of related counters. Samples for an i915 perf stream capturing OA metrics will include a set of counter values packed in a compact HW specific format. The OA unit supports a number of different packing formats which can be selected by the user opening the stream. Perf has support for grouping events, but each event in the group is configured, validated and authenticated individually with separate system calls.

i915 perf stream configurations are provided as an array of u64 (key,value) pairs, instead of a fixed struct with multiple miscellaneous config members, interleaved with event-type specific members.

i915 perf doesn’t support exposing metrics via an mmap’d circular buffer. The supported metrics are being written to memory by the GPU unsynchronized with the CPU, using HW specific packing formats for counter sets. Sometimes the constraints on HW configuration require reports to be filtered before it would be acceptable to expose them to unprivileged applications - to hide the metrics of other processes/contexts. For these use cases a read() based interface is a good fit, and provides an opportunity to filter data as it gets copied from the GPU mapped buffers to userspace buffers.

Issues hit with first prototype based on Core Perf
The first prototype of this driver was based on the core perf infrastructure, and while we did make that mostly work, with some changes to perf, we found we were breaking or working around too many assumptions baked into perf’s currently cpu centric design.

In the end we didn’t see a clear benefit to making perf’s implementation and interface more complex by changing design assumptions while we knew we still wouldn’t be able to use any existing perf based userspace tools.

Also considering the Gen specific nature of the Observability hardware and how userspace will sometimes need to combine i915 perf OA metrics with side-band OA data captured via MI_REPORT_PERF_COUNT commands; we’re expecting the interface to be used by a platform specific userspace such as OpenGL or tools. This is to say; we aren’t inherently missing out on having a standard vendor/architecture agnostic interface by not using perf.

For posterity, in case we might re-visit trying to adapt core perf to be better suited to exposing i915 metrics these were the main pain points we hit:

The perf based OA PMU driver broke some significant design assumptions:

Existing perf pmus are used for profiling work on a cpu and we were introducing the idea of _IS_DEVICE pmus with different security implications, the need to fake cpu-related data (such as user/kernel registers) to fit with perf’s current design, and adding _DEVICE records as a way to forward device-specific status records.

The OA unit writes reports of counters into a circular buffer, without involvement from the CPU, making our PMU driver the first of a kind.

Given the way we were periodically forward data from the GPU-mapped, OA buffer to perf’s buffer, those bursts of sample writes looked to perf like we were sampling too fast and so we had to subvert its throttling checks.

Perf supports groups of counters and allows those to be read via transactions internally but transactions currently seem designed to be explicitly initiated from the cpu (say in response to a userspace read()) and while we could pull a report out of the OA buffer we can’t trigger a report from the cpu on demand.

Related to being report based; the OA counters are configured in HW as a set while perf generally expects counter configurations to be orthogonal. Although counters can be associated with a group leader as they are opened, there’s no clear precedent for being able to provide group-wide configuration attributes (for example we want to let userspace choose the OA unit report format used to capture all counters in a set, or specify a GPU context to filter metrics on). We avoided using perf’s grouping feature and forwarded OA reports to userspace via perf’s ‘raw’ sample field. This suited our userspace well considering how coupled the counters are when dealing with normalizing. It would be inconvenient to split counters up into separate events, only to require userspace to recombine them. For Mesa it’s also convenient to be forwarded raw, periodic reports for combining with the side-band raw reports it captures using MI_REPORT_PERF_COUNT commands.

As a side note on perf’s grouping feature; there was also some concern that using PERF_FORMAT_GROUP as a way to pack together counter values would quite drastically inflate our sample sizes, which would likely lower the effective sampling resolutions we could use when the available memory bandwidth is limited.

With the OA unit’s report formats, counters are packed together as 32 or 40bit values, with the largest report size being 256 bytes.

PERF_FORMAT_GROUP values are 64bit, but there doesn’t appear to be a documented ordering to the values, implying PERF_FORMAT_ID must also be used to add a 64bit ID before each value; giving 16 bytes per counter.

Related to counter orthogonality; we can’t time share the OA unit, while event scheduling is a central design idea within perf for allowing userspace to open + enable more events than can be configured in HW at any one time. The OA unit is not designed to allow re-configuration while in use. We can’t reconfigure the OA unit without losing internal OA unit state which we can’t access explicitly to save and restore. Reconfiguring the OA unit is also relatively slow, involving ~100 register writes. From userspace Mesa also depends on a stable OA configuration when emitting MI_REPORT_PERF_COUNT commands and importantly the OA unit can’t be disabled while there are outstanding MI_RPC commands lest we hang the command streamer.

The contents of sample records aren’t extensible by device drivers (i.e. the sample_type bits). As an example; Sourab Gupta had been looking to attach GPU timestamps to our OA samples. We were shoehorning OA reports into sample records by using the ‘raw’ field, but it’s tricky to pack more than one thing into this field because events/core.c currently only lets a pmu give a single raw data pointer plus len which will be copied into the ring buffer. To include more than the OA report we’d have to copy the report into an intermediate larger buffer. I’d been considering allowing a vector of data+len values to be specified for copying the raw data, but it felt like a kludge to being using the raw field for this purpose.

It felt like our perf based PMU was making some technical compromises just for the sake of using perf:

perf_event_open() requires events to either relate to a pid or a specific cpu core, while our device pmu related to neither. Events opened with a pid will be automatically enabled/disabled according to the scheduling of that process - so not appropriate for us. When an event is related to a cpu id, perf ensures pmu methods will be invoked via an inter process interrupt on that core. To avoid invasive changes our userspace opened OA perf events for a specific cpu. This was workable but it meant the majority of the OA driver ran in atomic context, including all OA report forwarding, which wasn’t really necessary in our case and seems to make our locking requirements somewhat complex as we handled the interaction with the rest of the i915 driver.

i915 Driver Entry Points
This section covers the entrypoints exported outside of i915_perf.c to integrate with drm/i915 and to handle the DRM_I915_PERF_OPEN ioctl.

int i915_perf_init(struct drm_i915_private *i915)
initialize i915-perf state on module bind

Parameters

struct drm_i915_private *i915
i915 device instance

Description

Initializes i915-perf state without exposing anything to userspace.

Note

i915-perf initialization is split into an ‘init’ and ‘register’ phase with the i915_perf_register() exposing state to userspace.

void i915_perf_fini(struct drm_i915_private *i915)
Counter part to i915_perf_init()

Parameters

struct drm_i915_private *i915
i915 device instance

void i915_perf_register(struct drm_i915_private *i915)
exposes i915-perf to userspace

Parameters

struct drm_i915_private *i915
i915 device instance

Description

In particular OA metric sets are advertised under a sysfs metrics/ directory allowing userspace to enumerate valid IDs that can be used to open an i915-perf stream.

void i915_perf_unregister(struct drm_i915_private *i915)
hide i915-perf from userspace

Parameters

struct drm_i915_private *i915
i915 device instance

Description

i915-perf state cleanup is split up into an ‘unregister’ and ‘deinit’ phase where the interface is first hidden from userspace by i915_perf_unregister() before cleaning up remaining state in i915_perf_fini().

int i915_perf_open_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
DRM ioctl() for userspace to open a stream FD

Parameters

struct drm_device *dev
drm device

void *data
ioctl data copied from userspace (unvalidated)

struct drm_file *file
drm file

Description

Validates the stream open parameters given by userspace including flags and an array of u64 key, value pair properties.

Very little is assumed up front about the nature of the stream being opened (for instance we don’t assume it’s for periodic OA unit metrics). An i915-perf stream is expected to be a suitable interface for other forms of buffered data written by the GPU besides periodic OA metrics.

Note we copy the properties from userspace outside of the i915 perf mutex to avoid an awkward lockdep with mmap_lock.

Most of the implementation details are handled by i915_perf_open_ioctl_locked() after taking the gt->perf.lock mutex for serializing with any non-file-operation driver hooks.

Return

A newly opened i915 Perf stream file descriptor or negative error code on failure.

int i915_perf_release(struct inode *inode, struct file *file)
handles userspace close() of a stream file

Parameters

struct inode *inode
anonymous inode associated with file

struct file *file
An i915 perf stream file

Description

Cleans up any resources associated with an open i915 perf stream file.

NB: close() can’t really fail from the userspace point of view.

Return

zero on success or a negative error code.

int i915_perf_add_config_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
DRM ioctl() for userspace to add a new OA config

Parameters

struct drm_device *dev
drm device

void *data
ioctl data (pointer to struct drm_i915_perf_oa_config) copied from userspace (unvalidated)

struct drm_file *file
drm file

Description

Validates the submitted OA register to be saved into a new OA config that can then be used for programming the OA unit and its NOA network.

Return

A new allocated config number to be used with the perf open ioctl or a negative error code on failure.

int i915_perf_remove_config_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
DRM ioctl() for userspace to remove an OA config

Parameters

struct drm_device *dev
drm device

void *data
ioctl data (pointer to u64 integer) copied from userspace

struct drm_file *file
drm file

Description

Configs can be removed while being used, the will stop appearing in sysfs and their content will be freed when the stream using the config is closed.

Return

0 on success or a negative error code on failure.

i915 Perf Stream
This section covers the stream-semantics-agnostic structures and functions for representing an i915 perf stream FD and associated file operations.

struct i915_perf_stream
state for a single open stream FD

Definition:

struct i915_perf_stream {
    struct i915_perf *perf;
    struct intel_uncore *uncore;
    struct intel_engine_cs *engine;
    struct mutex lock;
    u32 sample_flags;
    int sample_size;
    struct i915_gem_context *ctx;
    bool enabled;
    bool hold_preemption;
    const struct i915_perf_stream_ops *ops;
    struct i915_oa_config *oa_config;
    struct llist_head oa_config_bos;
    struct intel_context *pinned_ctx;
    u32 specific_ctx_id;
    u32 specific_ctx_id_mask;
    struct hrtimer poll_check_timer;
    wait_queue_head_t poll_wq;
    bool pollin;
    bool periodic;
    int period_exponent;
    struct {
        const struct i915_oa_format *format;
        struct i915_vma *vma;
        u8 *vaddr;
        u32 last_ctx_id;
        spinlock_t ptr_lock;
        u32 head;
        u32 tail;
    } oa_buffer;
    struct i915_vma *noa_wait;
    u64 poll_oa_period;
};
Members

perf
i915_perf backpointer

uncore
mmio access path

engine
Engine associated with this performance stream.

lock
Lock associated with operations on stream

sample_flags
Flags representing the DRM_I915_PERF_PROP_SAMPLE_* properties given when opening a stream, representing the contents of a single sample as read() by userspace.

sample_size
Considering the configured contents of a sample combined with the required header size, this is the total size of a single sample record.

ctx
NULL if measuring system-wide across all contexts or a specific context that is being monitored.

enabled
Whether the stream is currently enabled, considering whether the stream was opened in a disabled state and based on I915_PERF_IOCTL_ENABLE and I915_PERF_IOCTL_DISABLE calls.

hold_preemption
Whether preemption is put on hold for command submissions done on the ctx. This is useful for some drivers that cannot easily post process the OA buffer context to subtract delta of performance counters not associated with ctx.

ops
The callbacks providing the implementation of this specific type of configured stream.

oa_config
The OA configuration used by the stream.

oa_config_bos
A list of struct i915_oa_config_bo allocated lazily each time oa_config changes.

pinned_ctx
The OA context specific information.

specific_ctx_id
The id of the specific context.

specific_ctx_id_mask
The mask used to masking specific_ctx_id bits.

poll_check_timer
High resolution timer that will periodically check for data in the circular OA buffer for notifying userspace (e.g. during a read() or poll()).

poll_wq
The wait queue that hrtimer callback wakes when it sees data ready to read in the circular OA buffer.

pollin
Whether there is data available to read.

periodic
Whether periodic sampling is currently enabled.

period_exponent
The OA unit sampling frequency is derived from this.

oa_buffer
State of the OA buffer.

oa_buffer.ptr_lock
Locks reads and writes to all head/tail state

Consider: the head and tail pointer state needs to be read consistently from a hrtimer callback (atomic context) and read() fop (user context) with tail pointer updates happening in atomic context and head updates in user context and the (unlikely) possibility of read() errors needing to reset all head/tail state.

Note: Contention/performance aren’t currently a significant concern here considering the relatively low frequency of hrtimer callbacks (5ms period) and that reads typically only happen in response to a hrtimer event and likely complete before the next callback.

Note: This lock is not held while reading and copying data to userspace so the value of head observed in htrimer callbacks won’t represent any partial consumption of data.

oa_buffer.head
Although we can always read back the head pointer register, we prefer to avoid trusting the HW state, just to avoid any risk that some hardware condition could * somehow bump the head pointer unpredictably and cause us to forward the wrong OA buffer data to userspace.

oa_buffer.tail
The last verified tail that can be read by userspace.

noa_wait
A batch buffer doing a wait on the GPU for the NOA logic to be reprogrammed.

poll_oa_period
The period in nanoseconds at which the OA buffer should be checked for available data.

struct i915_perf_stream_ops
the OPs to support a specific stream type

Definition:

struct i915_perf_stream_ops {
    void (*enable)(struct i915_perf_stream *stream);
    void (*disable)(struct i915_perf_stream *stream);
    void (*poll_wait)(struct i915_perf_stream *stream,struct file *file, poll_table *wait);
    int (*wait_unlocked)(struct i915_perf_stream *stream);
    int (*read)(struct i915_perf_stream *stream,char __user *buf,size_t count, size_t *offset);
    void (*destroy)(struct i915_perf_stream *stream);
};
Members

enable
Enables the collection of HW samples, either in response to I915_PERF_IOCTL_ENABLE or implicitly called when stream is opened without I915_PERF_FLAG_DISABLED.

disable
Disables the collection of HW samples, either in response to I915_PERF_IOCTL_DISABLE or implicitly called before destroying the stream.

poll_wait
Call poll_wait, passing a wait queue that will be woken once there is something ready to read() for the stream

wait_unlocked
For handling a blocking read, wait until there is something to ready to read() for the stream. E.g. wait on the same wait queue that would be passed to poll_wait().

read
Copy buffered metrics as records to userspace buf: the userspace, destination buffer count: the number of bytes to copy, requested by userspace offset: zero at the start of the read, updated as the read proceeds, it represents how many bytes have been copied so far and the buffer offset for copying the next record.

Copy as many buffered i915 perf samples and records for this stream to userspace as will fit in the given buffer.

Only write complete records; returning -ENOSPC if there isn’t room for a complete record.

Return any error condition that results in a short read such as -ENOSPC or -EFAULT, even though these may be squashed before returning to userspace.

destroy
Cleanup any stream specific resources.

The stream will always be disabled before this is called.

int read_properties_unlocked(struct i915_perf *perf, u64 __user *uprops, u32 n_props, struct perf_open_properties *props)
validate + copy userspace stream open properties

Parameters

struct i915_perf *perf
i915 perf instance

u64 __user *uprops
The array of u64 key value pairs given by userspace

u32 n_props
The number of key value pairs expected in uprops

struct perf_open_properties *props
The stream configuration built up while validating properties

Description

Note this function only validates properties in isolation it doesn’t validate that the combination of properties makes sense or that all properties necessary for a particular kind of stream have been set.

Note that there currently aren’t any ordering requirements for properties so we shouldn’t validate or assume anything about ordering here. This doesn’t rule out defining new properties with ordering requirements in the future.

int i915_perf_open_ioctl_locked(struct i915_perf *perf, struct drm_i915_perf_open_param *param, struct perf_open_properties *props, struct drm_file *file)
DRM ioctl() for userspace to open a stream FD

Parameters

struct i915_perf *perf
i915 perf instance

struct drm_i915_perf_open_param *param
The open parameters passed to ‘DRM_I915_PERF_OPEN`

struct perf_open_properties *props
individually validated u64 property value pairs

struct drm_file *file
drm file

Description

See i915_perf_ioctl_open() for interface details.

Implements further stream config validation and stream initialization on behalf of i915_perf_open_ioctl() with the gt->perf.lock mutex taken to serialize with any non-file-operation driver hooks.

In the case where userspace is interested in OA unit metrics then further config validation and stream initialization details will be handled by i915_oa_stream_init(). The code here should only validate config state that will be relevant to all stream types / backends.

Note

at this point the props have only been validated in isolation and it’s still necessary to validate that the combination of properties makes sense.

Return

zero on success or a negative error code.

void i915_perf_destroy_locked(struct i915_perf_stream *stream)
destroy an i915 perf stream

Parameters

struct i915_perf_stream *stream
An i915 perf stream

Description

Frees all resources associated with the given i915 perf stream, disabling any associated data capture in the process.

Note

The gt->perf.lock mutex has been taken to serialize with any non-file-operation driver hooks.

ssize_t i915_perf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
handles read() FOP for i915 perf stream FDs

Parameters

struct file *file
An i915 perf stream file

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

loff_t *ppos
(inout) file seek position (unused)

Description

The entry point for handling a read() on a stream file descriptor from userspace. Most of the work is left to the i915_perf_read_locked() and i915_perf_stream_ops->read but to save having stream implementations (of which we might have multiple later) we handle blocking read here.

We can also consistently treat trying to read from a disabled stream as an IO error so implementations can assume the stream is enabled while reading.

Return

The number of bytes copied or a negative error code on failure.

long i915_perf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
support ioctl() usage with i915 perf stream FDs

Parameters

struct file *file
An i915 perf stream file

unsigned int cmd
the ioctl request

unsigned long arg
the ioctl data

Description

Implementation deferred to i915_perf_ioctl_locked().

Return

zero on success or a negative error code. Returns -EINVAL for an unknown ioctl request.

void i915_perf_enable_locked(struct i915_perf_stream *stream)
handle I915_PERF_IOCTL_ENABLE ioctl

Parameters

struct i915_perf_stream *stream
A disabled i915 perf stream

Description

[Re]enables the associated capture of data for this stream.

If a stream was previously enabled then there’s currently no intention to provide userspace any guarantee about the preservation of previously buffered data.

void i915_perf_disable_locked(struct i915_perf_stream *stream)
handle I915_PERF_IOCTL_DISABLE ioctl

Parameters

struct i915_perf_stream *stream
An enabled i915 perf stream

Description

Disables the associated capture of data for this stream.

The intention is that disabling an re-enabling a stream will ideally be cheaper than destroying and re-opening a stream with the same configuration, though there are no formal guarantees about what state or buffered data must be retained between disabling and re-enabling a stream.

Note

while a stream is disabled it’s considered an error for userspace to attempt to read from the stream (-EIO).

__poll_t i915_perf_poll(struct file *file, poll_table *wait)
call poll_wait() with a suitable wait queue for stream

Parameters

struct file *file
An i915 perf stream file

poll_table *wait
poll() state table

Description

For handling userspace polling on an i915 perf stream, this ensures poll_wait() gets called with a wait queue that will be woken for new stream data.

Note

Implementation deferred to i915_perf_poll_locked()

Return

any poll events that are ready without sleeping

__poll_t i915_perf_poll_locked(struct i915_perf_stream *stream, struct file *file, poll_table *wait)
poll_wait() with a suitable wait queue for stream

Parameters

struct i915_perf_stream *stream
An i915 perf stream

struct file *file
An i915 perf stream file

poll_table *wait
poll() state table

Description

For handling userspace polling on an i915 perf stream, this calls through to i915_perf_stream_ops->poll_wait to call poll_wait() with a wait queue that will be woken for new stream data.

Return

any poll events that are ready without sleeping

i915 Perf Observation Architecture Stream
struct i915_oa_ops
Gen specific implementation of an OA unit stream

Definition:

struct i915_oa_ops {
    bool (*is_valid_b_counter_reg)(struct i915_perf *perf, u32 addr);
    bool (*is_valid_mux_reg)(struct i915_perf *perf, u32 addr);
    bool (*is_valid_flex_reg)(struct i915_perf *perf, u32 addr);
    int (*enable_metric_set)(struct i915_perf_stream *stream, struct i915_active *active);
    void (*disable_metric_set)(struct i915_perf_stream *stream);
    void (*oa_enable)(struct i915_perf_stream *stream);
    void (*oa_disable)(struct i915_perf_stream *stream);
    int (*read)(struct i915_perf_stream *stream,char __user *buf,size_t count, size_t *offset);
    u32 (*oa_hw_tail_read)(struct i915_perf_stream *stream);
};
Members

is_valid_b_counter_reg
Validates register’s address for programming boolean counters for a particular platform.

is_valid_mux_reg
Validates register’s address for programming mux for a particular platform.

is_valid_flex_reg
Validates register’s address for programming flex EU filtering for a particular platform.

enable_metric_set
Selects and applies any MUX configuration to set up the Boolean and Custom (B/C) counters that are part of the counter reports being sampled. May apply system constraints such as disabling EU clock gating as required.

disable_metric_set
Remove system constraints associated with using the OA unit.

oa_enable
Enable periodic sampling

oa_disable
Disable periodic sampling

read
Copy data from the circular OA buffer into a given userspace buffer.

oa_hw_tail_read
read the OA tail pointer register

In particular this enables us to share all the fiddly code for handling the OA unit tail pointer race that affects multiple generations.

int i915_oa_stream_init(struct i915_perf_stream *stream, struct drm_i915_perf_open_param *param, struct perf_open_properties *props)
validate combined props for OA stream and init

Parameters

struct i915_perf_stream *stream
An i915 perf stream

struct drm_i915_perf_open_param *param
The open parameters passed to DRM_I915_PERF_OPEN

struct perf_open_properties *props
The property state that configures stream (individually validated)

Description

While read_properties_unlocked() validates properties in isolation it doesn’t ensure that the combination necessarily makes sense.

At this point it has been determined that userspace wants a stream of OA metrics, but still we need to further validate the combined properties are OK.

If the configuration makes sense then we can allocate memory for a circular OA buffer and apply the requested metric set configuration.

Return

zero on success or a negative error code.

int i915_oa_read(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset)
just calls through to i915_oa_ops->read

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

Description

Updates offset according to the number of bytes successfully copied into the userspace buffer.

Return

zero on success or a negative error code

void i915_oa_stream_enable(struct i915_perf_stream *stream)
handle I915_PERF_IOCTL_ENABLE for OA stream

Parameters

struct i915_perf_stream *stream
An i915 perf stream opened for OA metrics

Description

[Re]enables hardware periodic sampling according to the period configured when opening the stream. This also starts a hrtimer that will periodically check for data in the circular OA buffer for notifying userspace (e.g. during a read() or poll()).

void i915_oa_stream_disable(struct i915_perf_stream *stream)
handle I915_PERF_IOCTL_DISABLE for OA stream

Parameters

struct i915_perf_stream *stream
An i915 perf stream opened for OA metrics

Description

Stops the OA unit from periodically writing counter reports into the circular OA buffer. This also stops the hrtimer that periodically checks for data in the circular OA buffer, for notifying userspace.

int i915_oa_wait_unlocked(struct i915_perf_stream *stream)
handles blocking IO until OA data available

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

Description

Called when userspace tries to read() from a blocking stream FD opened for OA metrics. It waits until the hrtimer callback finds a non-empty OA buffer and wakes us.

Note

it’s acceptable to have this return with some false positives since any subsequent read handling will return -EAGAIN if there isn’t really data ready for userspace yet.

Return

zero on success or a negative error code

void i915_oa_poll_wait(struct i915_perf_stream *stream, struct file *file, poll_table *wait)
call poll_wait() for an OA stream poll()

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

struct file *file
An i915 perf stream file

poll_table *wait
poll() state table

Description

For handling userspace polling on an i915 perf stream opened for OA metrics, this starts a poll_wait with the wait queue that our hrtimer callback wakes when it sees data ready to read in the circular OA buffer.

Other i915 Perf Internals
This section simply includes all other currently documented i915 perf internals, in no particular order, but may include some more minor utilities or platform specific details than found in the more high-level sections.

struct perf_open_properties
for validated properties given to open a stream

Definition:

struct perf_open_properties {
    u32 sample_flags;
    u64 single_context:1;
    u64 hold_preemption:1;
    u64 ctx_handle;
    int metrics_set;
    int oa_format;
    bool oa_periodic;
    int oa_period_exponent;
    struct intel_engine_cs *engine;
    bool has_sseu;
    struct intel_sseu sseu;
    u64 poll_oa_period;
};
Members

sample_flags
DRM_I915_PERF_PROP_SAMPLE_* properties are tracked as flags

single_context
Whether a single or all gpu contexts should be monitored

hold_preemption
Whether the preemption is disabled for the filtered context

ctx_handle
A gem ctx handle for use with single_context

metrics_set
An ID for an OA unit metric set advertised via sysfs

oa_format
An OA unit HW report format

oa_periodic
Whether to enable periodic OA unit sampling

oa_period_exponent
The OA unit sampling period is derived from this

engine
The engine (typically rcs0) being monitored by the OA unit

has_sseu
Whether sseu was specified by userspace

sseu
internal SSEU configuration computed either from the userspace specified configuration in the opening parameters or a default value (see get_default_sseu_config())

poll_oa_period
The period in nanoseconds at which the CPU will check for OA data availability

Description

As read_properties_unlocked() enumerates and validates the properties given to open a stream of metrics the configuration is built up in the structure which starts out zero initialized.

bool oa_buffer_check_unlocked(struct i915_perf_stream *stream)
check for data and update tail ptr state

Parameters

struct i915_perf_stream *stream
i915 stream instance

Description

This is either called via fops (for blocking reads in user ctx) or the poll check hrtimer (atomic ctx) to check the OA buffer tail pointer and check if there is data available for userspace to read.

This function is central to providing a workaround for the OA unit tail pointer having a race with respect to what data is visible to the CPU. It is responsible for reading tail pointers from the hardware and giving the pointers time to ‘age’ before they are made available for reading. (See description of OA_TAIL_MARGIN_NSEC above for further details.)

Besides returning true when there is data available to read() this function also updates the tail in the oa_buffer object.

Note

It’s safe to read OA config state here unlocked, assuming that this is only called while the stream is enabled, while the global OA configuration can’t be modified.

Return

true if the OA buffer contains data, else false

int append_oa_status(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset, enum drm_i915_perf_record_type type)
Appends a status record to a userspace read() buffer.

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

enum drm_i915_perf_record_type type
The kind of status to report to userspace

Description

Writes a status record (such as DRM_I915_PERF_RECORD_OA_REPORT_LOST) into the userspace read() buffer.

The buf offset will only be updated on success.

Return

0 on success, negative error code on failure.

int append_oa_sample(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset, const u8 *report)
Copies single OA report into userspace read() buffer.

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

const u8 *report
A single OA report to (optionally) include as part of the sample

Description

The contents of a sample are configured through DRM_I915_PERF_PROP_SAMPLE_* properties when opening a stream, tracked as stream->sample_flags. This function copies the requested components of a single sample to the given read() buf.

The buf offset will only be updated on success.

Return

0 on success, negative error code on failure.

int gen8_append_oa_reports(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset)
Copies all buffered OA reports into userspace read() buffer.

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

Description

Notably any error condition resulting in a short read (-ENOSPC or -EFAULT) will be returned even though one or more records may have been successfully copied. In this case it’s up to the caller to decide if the error should be squashed before returning to userspace.

Note

reports are consumed from the head, and appended to the tail, so the tail chases the head?... If you think that’s mad and back-to-front you’re not alone, but this follows the Gen PRM naming convention.

Return

0 on success, negative error code on failure.

int gen8_oa_read(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset)
copy status records then buffered OA reports

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

Description

Checks OA unit status registers and if necessary appends corresponding status records for userspace (such as for a buffer full condition) and then initiate appending any buffered OA reports.

Updates offset according to the number of bytes successfully copied into the userspace buffer.

NB: some data may be successfully copied to the userspace buffer even if an error is returned, and this is reflected in the updated offset.

Return

zero on success or a negative error code

int gen7_append_oa_reports(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset)
Copies all buffered OA reports into userspace read() buffer.

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

Description

Notably any error condition resulting in a short read (-ENOSPC or -EFAULT) will be returned even though one or more records may have been successfully copied. In this case it’s up to the caller to decide if the error should be squashed before returning to userspace.

Note

reports are consumed from the head, and appended to the tail, so the tail chases the head?... If you think that’s mad and back-to-front you’re not alone, but this follows the Gen PRM naming convention.

Return

0 on success, negative error code on failure.

int gen7_oa_read(struct i915_perf_stream *stream, char __user *buf, size_t count, size_t *offset)
copy status records then buffered OA reports

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

char __user *buf
destination buffer given by userspace

size_t count
the number of bytes userspace wants to read

size_t *offset
(inout): the current position for writing into buf

Description

Checks Gen 7 specific OA unit status registers and if necessary appends corresponding status records for userspace (such as for a buffer full condition) and then initiate appending any buffered OA reports.

Updates offset according to the number of bytes successfully copied into the userspace buffer.

Return

zero on success or a negative error code

int oa_get_render_ctx_id(struct i915_perf_stream *stream)
determine and hold ctx hw id

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

Description

Determine the render context hw id, and ensure it remains fixed for the lifetime of the stream. This ensures that we don’t have to worry about updating the context ID in OACONTROL on the fly.

Return

zero on success or a negative error code

void oa_put_render_ctx_id(struct i915_perf_stream *stream)
counterpart to oa_get_render_ctx_id releases hold

Parameters

struct i915_perf_stream *stream
An i915-perf stream opened for OA metrics

Description

In case anything needed doing to ensure the context HW ID would remain valid for the lifetime of the stream, then that can be undone here.

long i915_perf_ioctl_locked(struct i915_perf_stream *stream, unsigned int cmd, unsigned long arg)
support ioctl() usage with i915 perf stream FDs

Parameters

struct i915_perf_stream *stream
An i915 perf stream

unsigned int cmd
the ioctl request

unsigned long arg
the ioctl data

Return

zero on success or a negative error code. Returns -EINVAL for an unknown ioctl request.

int i915_perf_ioctl_version(struct drm_i915_private *i915)
Version of the i915-perf subsystem

Parameters

struct drm_i915_private *i915
The i915 device

Description

This version number is used by userspace to detect available features.

Style
The drm/i915 driver codebase has some style rules in addition to (and, in some cases, deviating from) the kernel coding style.

Register macro definition style
The style guide for i915_reg.h.

Follow the style described here for new macros, and while changing existing macros. Do not mass change existing definitions just to update the style.

File Layout
Keep helper macros near the top. For example, _PIPE() and friends.

Prefix macros that generally should not be used outside of this file with underscore ‘_’. For example, _PIPE() and friends, single instances of registers that are defined solely for the use by function-like macros.

Avoid using the underscore prefixed macros outside of this file. There are exceptions, but keep them to a minimum.

There are two basic types of register definitions: Single registers and register groups. Register groups are registers which have two or more instances, for example one per pipe, port, transcoder, etc. Register groups should be defined using function-like macros.

For single registers, define the register offset first, followed by register contents.

For register groups, define the register instance offsets first, prefixed with underscore, followed by a function-like macro choosing the right instance based on the parameter, followed by register contents.

Define the register contents (i.e. bit and bit field macros) from most significant to least significant bit. Indent the register content macros using two extra spaces between #define and the macro name.

Define bit fields using REG_GENMASK(h, l). Define bit field contents using REG_FIELD_PREP(mask, value). This will define the values already shifted in place, so they can be directly OR’d together. For convenience, function-like macros may be used to define bit fields, but do note that the macros may be needed to read as well as write the register contents.

Define bits using REG_BIT(N). Do not add _BIT suffix to the name.

Group the register and its contents together without blank lines, separate from other registers and their contents with one blank line.

Indent macro values from macro names using TABs. Align values vertically. Use braces in macro values as needed to avoid unintended precedence after macro substitution. Use spaces in macro values according to kernel coding style. Use lower case in hexadecimal values.

Naming
Try to name registers according to the specs. If the register name changes in the specs from platform to another, stick to the original name.

Try to re-use existing register macro definitions. Only add new macros for new register offsets, or when the register contents have changed enough to warrant a full redefinition.

When a register macro changes for a new platform, prefix the new macro using the platform acronym or generation. For example, SKL_ or GEN8_. The prefix signifies the start platform/generation using the register.

When a bit (field) macro changes or gets added for a new platform, while retaining the existing register macro, add a platform acronym or generation suffix to the name. For example, _SKL or _GEN8.

Examples
(Note that the values in the example are indented using spaces instead of TABs to avoid misalignment in generated documentation. Use TABs in the definitions.):

#define _FOO_A                      0xf000
#define _FOO_B                      0xf001
#define FOO(pipe)                   _MMIO_PIPE(pipe, _FOO_A, _FOO_B)
#define   FOO_ENABLE                REG_BIT(31)
#define   FOO_MODE_MASK             REG_GENMASK(19, 16)
#define   FOO_MODE_BAR              REG_FIELD_PREP(FOO_MODE_MASK, 0)
#define   FOO_MODE_BAZ              REG_FIELD_PREP(FOO_MODE_MASK, 1)
#define   FOO_MODE_QUX_SNB          REG_FIELD_PREP(FOO_MODE_MASK, 2)

#define BAR                         _MMIO(0xb000)
#define GEN8_BAR                    _MMIO(0xb888)
i915 DRM client usage stats implementation
The drm/i915 driver implements the DRM client usage stats specification as documented in DRM client usage stats.

Example of the output showing the implemented key value pairs and entirety of the currently possible format options:

pos:    0
flags:  0100002
mnt_id: 21
drm-driver: i915
drm-pdev:   0000:00:02.0
drm-client-id:      7
drm-engine-render:  9288864723 ns
drm-engine-copy:    2035071108 ns
drm-engine-video:   0 ns
drm-engine-capacity-video:   2
drm-engine-video-enhance:   0 ns
Possible drm-engine- key names are: render, copy, video and video-enhance.

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

I915 GuC Submission/DRM Scheduler Section
Upstream plan
For upstream the overall plan for landing GuC submission and integrating the i915 with the DRM scheduler is:

Merge basic GuC submission
Basic submission support for all gen11+ platforms

Not enabled by default on any current platforms but can be enabled via modparam enable_guc

Lots of rework will need to be done to integrate with DRM scheduler so no need to nit pick everything in the code, it just should be functional, no major coding style / layering errors, and not regress execlists

Update IGTs / selftests as needed to work with GuC submission

Enable CI on supported platforms for a baseline

Rework / get CI heathly for GuC submission in place as needed

Merge new parallel submission uAPI
Bonding uAPI completely incompatible with GuC submission, plus it has severe design issues in general, which is why we want to retire it no matter what

New uAPI adds I915_CONTEXT_ENGINES_EXT_PARALLEL context setup step which configures a slot with N contexts

After I915_CONTEXT_ENGINES_EXT_PARALLEL a user can submit N batches to a slot in a single execbuf IOCTL and the batches run on the GPU in paralllel

Initially only for GuC submission but execlists can be supported if needed

Convert the i915 to use the DRM scheduler
GuC submission backend fully integrated with DRM scheduler
All request queues removed from backend (e.g. all backpressure handled in DRM scheduler)

Resets / cancels hook in DRM scheduler

Watchdog hooks into DRM scheduler

Lots of complexity of the GuC backend can be pulled out once integrated with DRM scheduler (e.g. state machine gets simpler, locking gets simpler, etc...)

Execlists backend will minimum required to hook in the DRM scheduler
Legacy interface

Features like timeslicing / preemption / virtual engines would be difficult to integrate with the DRM scheduler and these features are not required for GuC submission as the GuC does these things for us

ROI low on fully integrating into DRM scheduler

Fully integrating would add lots of complexity to DRM scheduler

Port i915 priority inheritance / boosting feature in DRM scheduler
Used for i915 page flip, may be useful to other DRM drivers as well

Will be an optional feature in the DRM scheduler

Remove in-order completion assumptions from DRM scheduler
Even when using the DRM scheduler the backends will handle preemption, timeslicing, etc... so it is possible for jobs to finish out of order

Pull out i915 priority levels and use DRM priority levels

Optimize DRM scheduler as needed

TODOs for GuC submission upstream
Need an update to GuC firmware / i915 to enable error state capture

Open source tool to decode GuC logs

Public GuC spec

New uAPI for basic GuC submission
No major changes are required to the uAPI for basic GuC submission. The only change is a new scheduler attribute: I915_SCHEDULER_CAP_STATIC_PRIORITY_MAP. This attribute indicates the 2k i915 user priority levels are statically mapped into 3 levels as follows:

-1k to -1 Low priority

0 Medium priority

1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority band is reserved with the kernel. This aligns with the DRM scheduler priority levels too.

Spec references:
https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt

https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority

https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t

New parallel submission uAPI
The existing bonding uAPI is completely broken with GuC submission because whether a submission is a single context submit or parallel submit isn’t known until execbuf time activated via the I915_SUBMIT_FENCE. To submit multiple contexts in parallel with the GuC the context must be explicitly registered with N contexts and all N contexts must be submitted in a single command to the GuC. The GuC interfaces do not support dynamically changing between N contexts as the bonding uAPI does. Hence the need for a new parallel submission interface. Also the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore I915_SUBMIT_FENCE is by design a future fence, so not really something we should continue to support.

The new parallel submission uAPI consists of 3 parts:

Export engines logical mapping

A ‘set_parallel’ extension to configure contexts for parallel submission

Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Export engines logical mapping
Certain use cases require BBs to be placed on engine instances in logical order (e.g. split-frame on gen11+). The logical mapping of engine instances can change based on fusing. Rather than making UMDs be aware of fusing, simply expose the logical mapping with the existing query engine info IOCTL. Also the GuC submission interface currently only supports submitting multiple contexts to engines in logical order which is a new requirement compared to execlists. Lastly, all current platforms have at most 2 engine instances and the logical order is the same as uAPI order. This will change on platforms with more than 2 engine instances.

A single bit will be added to drm_i915_engine_info.flags indicating that the logical instance has been returned and a new field, drm_i915_engine_info.logical_instance, returns the logical instance.

A ‘set_parallel’ extension to configure contexts for parallel submission
The ‘set_parallel’ extension configures a slot for parallel submission of N BBs. It is a setup step that must be called before using any of the contexts. See I915_CONTEXT_ENGINES_EXT_LOAD_BALANCE or I915_CONTEXT_ENGINES_EXT_BOND for similar existing examples. Once a slot is configured for parallel submission the execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only supports GuC submission. Execlists supports can be added later if needed.

Add I915_CONTEXT_ENGINES_EXT_PARALLEL_SUBMIT and drm_i915_context_engines_parallel_submit to the uAPI to implement this extension.

struct i915_context_engines_parallel_submit
Configure engine for parallel submission.

Definition:

struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
Members

base
base user extension.

engine_index
slot for parallel engine

width
number of contexts per parallel engine or in other words the number of batches in each submission

num_siblings
number of siblings per context or in other words the number of possible placements for each submission

mbz16
reserved for future use; must be zero

flags
all undefined flags must be zero, currently not defined flags

mbz64
reserved for future use; must be zero

engines
2-d array of engine instances to configure parallel engine

length = width (i) * num_siblings (j) index = j + i * num_siblings

Description

Setup a slot in the context engine map to allow multiple BBs to be submitted in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU in parallel. Multiple hardware contexts are created internally in the i915 to run these BBs. Once a slot is configured for N BBs only N BBs can be submitted in each execbuf IOCTL and this is implicit behavior e.g. The user doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how many BBs there are based on the slot’s configuration. The N BBs are the last N buffer objects or first N if I915_EXEC_BATCH_FIRST is set.

The default placement behavior is to create implicit bonds between each context if each context maps to more than 1 physical engine (e.g. context is a virtual engine). Also we only allow contexts of same engine class and these contexts must be in logically contiguous order. Examples of the placement behavior are described below. Lastly, the default is to not allow BBs to be preempted mid-batch. Rather insert coordinated preemption points on all hardware contexts between each set of BBs. Flags could be added in the future to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if the placement configuration isn’t supported on the platform / submission interface. Returns -ENODEV if extension isn’t supported on the platform / submission interface.

Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL
Contexts that have been configured with the ‘set_parallel’ extension can only submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects in the drm_i915_gem_exec_object2 list or the first N if I915_EXEC_BATCH_FIRST is set. The number of BBs is implicit based on the slot submitted and how it has been configured by ‘set_parallel’ or other extensions. No uAPI changes are required to the execbuf2 IOCTL.

©The kernel development community. | Powered by Sphinx 5.3.0 & Alabaster 0.7.16 | Page source

ISA Drivers
The following text is adapted from the commit message of the initial commit of the ISA bus driver authored by Rene Herman.

During the recent “isa drivers using platform devices” discussion it was pointed out that (ALSA) ISA drivers ran into the problem of not having the option to fail driver load (device registration rather) upon not finding their hardware due to a probe() error not being passed up through the driver model. In the course of that, I suggested a separate ISA bus might be best; Russell King agreed and suggested this bus could use the .match() method for the actual device discovery.

The attached does this. For this old non (generically) discoverable ISA hardware only the driver itself can do discovery so as a difference with the platform_bus, this isa_bus also distributes match() up to the driver.

As another difference: these devices only exist in the driver model due to the driver creating them because it might want to drive them, meaning that all device creation has been made internal as well.

The usage model this provides is nice, and has been acked from the ALSA side by Takashi Iwai and Jaroslav Kysela. The ALSA driver module_init’s now (for oldisa-only drivers) become:

static int __init alsa_card_foo_init(void)
{
        return isa_register_driver(&snd_foo_isa_driver, SNDRV_CARDS);
}

static void __exit alsa_card_foo_exit(void)
{
        isa_unregister_driver(&snd_foo_isa_driver);
}
Quite like the other bus models therefore. This removes a lot of duplicated init code from the ALSA ISA drivers.

The passed in isa_driver struct is the regular driver struct embedding a struct device_driver, the normal probe/remove/shutdown/suspend/resume callbacks, and as indicated that .match callback.

The “SNDRV_CARDS” you see being passed in is a “unsigned int ndev” parameter, indicating how many devices to create and call our methods with.

The platform_driver callbacks are called with a platform_device param; the isa_driver callbacks are being called with a struct device *dev, unsigned int id pair directly -- with the device creation completely internal to the bus it’s much cleaner to not leak isa_dev’s by passing them in at all. The id is the only thing we ever want other then the struct device anyways, and it makes for nicer code in the callbacks as well.

With this additional .match() callback ISA drivers have all options. If ALSA would want to keep the old non-load behaviour, it could stick all of the old .probe in .match, which would only keep them registered after everything was found to be present and accounted for. If it wanted the behaviour of always loading as it inadvertently did for a bit after the changeover to platform devices, it could just not provide a .match() and do everything in .probe() as before.

If it, as Takashi Iwai already suggested earlier as a way of following the model from saner buses more closely, wants to load when a later bind could conceivably succeed, it could use .match() for the prerequisites (such as checking the user wants the card enabled and that port/irq/dma values have been passed in) and .probe() for everything else. This is the nicest model.

To the code...

This exports only two functions; isa_{,un}register_driver().

isa_register_driver() register’s the struct device_driver, and then loops over the passed in ndev creating devices and registering them. This causes the bus match method to be called for them, which is:

int isa_bus_match(struct device *dev, struct device_driver *driver)
{
        struct isa_driver *isa_driver = to_isa_driver(driver);

        if (dev->platform_data == isa_driver) {
                if (!isa_driver->match ||
                        isa_driver->match(dev, to_isa_dev(dev)->id))
                        return 1;
                dev->platform_data = NULL;
        }
        return 0;
}
The first thing this does is check if this device is in fact one of this driver’s devices by seeing if the device’s platform_data pointer is set to this driver. Platform devices compare strings, but we don’t need to do that with everything being internal, so isa_register_driver() abuses dev->platform_data as a isa_driver pointer which we can then check here. I believe platform_data is available for this, but if rather not, moving the isa_driver pointer to the private struct isa_dev is ofcourse fine as well.

Then, if the driver did not provide a .match, it matches. If it did, the driver match() method is called to determine a match.

If it did not match, dev->platform_data is reset to indicate this to isa_register_driver which can then unregister the device again.

If during all this, there’s any error, or no devices matched at all everything is backed out again and the error, or -ENODEV, is returned.

isa_unregister_driver() just unregisters the matched devices and the driver itself.

module_isa_driver is a helper macro for ISA drivers which do not do anything special in module init/exit. This eliminates a lot of boilerplate code. Each module may only use this macro once, and calling it replaces module_init and module_exit.

max_num_isa_dev is a macro to determine the maximum possible number of ISA devices which may be registered in the I/O port address space given the address extent of the ISA devices.