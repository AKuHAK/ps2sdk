/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * EE Kernel prototypes
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stddef.h>
#include <stdarg.h>
#include <sifdma.h>

#define DI DIntr
#define EI EIntr

// Workaround for EE kernel bug: call this immediately before returning from any interrupt handler.
#define ExitHandler() asm volatile("sync\nei\n")

// note: 'sync' is the same as 'sync.l'
#define EE_SYNC()  __asm__ volatile("sync")
#define EE_SYNCL() __asm__ volatile("sync.l")
#define EE_SYNCP() __asm__ volatile("sync.p")

#define UNCACHED_SEG(x) \
    ((void *)(((u32)(x)) | 0x20000000))

#define IS_UNCACHED_SEG(x) \
    (((u32)(x)) & 0x20000000)

#define UCAB_SEG(x) \
    ((void *)(((u32)(x)) | 0x30000000))

#define PUSHDATA(t, x, v, l) \
    *(t *)(x) = (v);         \
    (l)       = sizeof(t)

#define POPDATA(t, x, v, l) \
    (v) = *(t *)(x);        \
    (l) = sizeof(t)

#define ALIGNED(x) __attribute__((aligned((x))))

// GP functions
extern void *ChangeGP(void *gp);
extern void SetGP(void *gp);
extern void *GetGP(void);

extern void *_gp;
#define SetModuleGP() ChangeGP(&_gp)

/** Special thread ID for referring to the running thread.
    Unlike the IOP kernel, this is only supported by ReferThreadStatus() and ChangeThreadPriority().
    It can also be used by the iWakeupThread() _syscall_.
    But because the libkernel patch may call WakeupThread() to avoid the defect within iWakeupThread() that prevents the running thread from being woken up,
    THS_SELF should not be used with iWakeupThread(). */
#define TH_SELF 0

/** Limits */
#define MAX_THREADS    256 // A few will be used for the kernel patches. Thread 0 is always the idle thread.
#define MAX_SEMAPHORES 256 // A few will be used for the kernel patches.
#define MAX_PRIORITY   128
#define MAX_HANDLERS   128
#define MAX_ALARMS     64

/** Modes for FlushCache */
#define WRITEBACK_DCACHE  0
#define INVALIDATE_DCACHE 1
#define INVALIDATE_ICACHE 2
#define INVALIDATE_CACHE  3 // Invalidate both data & instruction caches.

/** EE Interrupt Controller (INTC) interrupt numbers */
enum {
    INTC_GS,
    INTC_SBUS,
    INTC_VBLANK_S,
    INTC_VBLANK_E,
    INTC_VIF0,
    INTC_VIF1,
    INTC_VU0,
    INTC_VU1,
    INTC_IPU,
    INTC_TIM0,
    INTC_TIM1,
    INTC_TIM2,
    // INTC_TIM3,       // Reserved by the EE kernel for alarms (do not use)
    INTC_SFIFO = 13, // Error encountered during SFIFO transfer
    INTC_VU0WD       // VU0 WatchDog; ForceBreak is sent to VU0 if left in RUN state for extended periods of time.
};

// For backward-compatibility
#define kINTC_GS           INTC_GS
#define kINTC_SBUS         INTC_SBUS
#define kINTC_VBLANK_START INTC_VBLANK_S
#define kINTC_VBLANK_END   INTC_VBLANK_E
#define kINTC_VIF0         INTC_VIF0
#define kINTC_VIF1         INTC_VIF1
#define kINTC_VU0          INTC_VU0
#define kINTC_VU1          INTC_VU1
#define kINTC_IPU          INTC_IPU
#define kINTC_TIMER0       INTC_TIM0
#define kINTC_TIMER1       INTC_TIM1

/** EE Direct Memory Access Controller (DMAC) interrupt numbers */
enum {
    DMAC_VIF0,
    DMAC_VIF1,
    DMAC_GIF,
    DMAC_FROM_IPU,
    DMAC_TO_IPU,
    DMAC_SIF0,
    DMAC_SIF1,
    DMAC_SIF2,
    DMAC_FROM_SPR,
    DMAC_TO_SPR,

    DMAC_CIS = 13, // Channel interrupt
    DMAC_MEIS,     // MemFIFO empty interrupt
    DMAC_BEIS,     // Bus error interrupt
};

/** ResetEE argument bits */
#define INIT_DMAC 0x01
#define INIT_VU1  0x02
#define INIT_VIF1 0x04
#define INIT_GIF  0x08
#define INIT_VU0  0x10
#define INIT_VIF0 0x20
#define INIT_IPU  0x40

static inline void nopdelay(void)
{
    int i = 0xfffff;

    do {
        __asm__("nop\nnop\nnop\nnop\nnop\n");
    } while (i-- != -1);
}

static inline int ee_get_opmode(void)
{
    u32 status;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        ".set\tpop\n\t"
        : "=r"(status));

    return ((status >> 3) & 3);
}

static inline int ee_set_opmode(u32 opmode)
{
    u32 status, mask;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "li\t%1, 0xffffffe7\n\t"
        "and\t%0, %1\n\t"
        "or\t%0, %2\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status), "=r"(mask)
        : "r"(opmode));

    return ((status >> 3) & 3);
}

static inline int ee_kmode_enter()
{
    u32 status, mask;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "li\t%1, 0xffffffe7\n\t"
        "and\t%0, %1\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status), "=r"(mask));

    return status;
}

static inline int ee_kmode_exit()
{
    int status;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "ori\t%0, 0x10\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status));

    return status;
}

typedef struct t_ee_sema
{
    int count,
        max_count,
        init_count,
        wait_threads;
    u32 attr,
        option;
} ee_sema_t;

typedef struct t_ee_thread
{
    int status;           // 0x00
    void *func;           // 0x04
    void *stack;          // 0x08
    int stack_size;       // 0x0C
    void *gp_reg;         // 0x10
    int initial_priority; // 0x14
    int current_priority; // 0x18
    u32 attr;             // 0x1C
    u32 option;           // 0x20 Do not use - officially documented to not work.

} ee_thread_t;

/** Thread status */
#define THS_RUN         0x01
#define THS_READY       0x02
#define THS_WAIT        0x04
#define THS_SUSPEND     0x08
#define THS_WAITSUSPEND 0x0c
#define THS_DORMANT     0x10

/** Thread WAIT Status */
#define TSW_NONE  0 // Thread is not in WAIT state
#define TSW_SLEEP 1
#define TSW_SEMA  2

// sizeof() == 0x30
typedef struct t_ee_thread_status
{
    int status;           // 0x00
    void *func;           // 0x04
    void *stack;          // 0x08
    int stack_size;       // 0x0C
    void *gp_reg;         // 0x10
    int initial_priority; // 0x14
    int current_priority; // 0x18
    u32 attr;             // 0x1C
    u32 option;           // 0x20
    u32 waitType;         // 0x24
    u32 waitId;           // 0x28
    u32 wakeupCount;      // 0x2C
} ee_thread_status_t;

/** CpuConfig options */
enum CPU_CONFIG {
    CPU_CONFIG_ENABLE_DIE = 0, // Enable Dual Issue
    CPU_CONFIG_ENABLE_ICE,     // Enable Instruction Cache
    CPU_CONFIG_ENABLE_DCE,     // Enable Data Cache
    CPU_CONFIG_DISBLE_DIE,     // Disable Dual Issue
    CPU_CONFIG_DISBLE_ICE,     // Disable Instruction Cache
    CPU_CONFIG_DISBLE_DCE      // Disable Data Cache
};

/** EnableCache & DisableCache options (multiple options may be specified) */
#define CPU_DATA_CACHE        1
#define CPU_INSTRUCTION_CACHE 2

/** Cop0 Register for (i)GetCop0 and (i)SetCop0 */
enum {
    COP0_INDEX,
    COP0_RANDOM,
    COP0_ENTRYLO0,
    COP0_ENTRYLO1,
    COP0_CONTEXT,
    COP0_PAGEMASK,
    COP0_WIRED,
    // 7 reserved
    COP0_BADVADDR = 8,
    COP0_COUNT,
    COP0_ENTRYHI,
    COP0_COMPARE,
    COP0_STATUS,
    COP0_CAUSE,
    COP0_EPC,
    COP0_PRID,
    COP0_CONFIG,
    // 17-22 reserved
    COP0_BADPADDR = 23,
    COP0_DEBUG,
    COP0_PERF,
    // 26-27 reserved
    COP0_TAGLO = 28,
    COP0_TAGHI,
    COP0_ERROREPC,
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization/deinitialization routines.  */
extern void _InitSys(void); // Run by crt0

extern void TerminateLibrary(void); // Run by crt0

/* Thread update functions */
extern int InitThread(void); // Run by _InitSys

extern s32 iWakeupThread(s32 thread_id);
extern s32 iRotateThreadReadyQueue(s32 priority);
extern s32 iSuspendThread(s32 thread_id);

/* TLB update functions */
extern void InitTLBFunctions(void); // Run by _InitSys

extern void InitTLB(void);
extern void Exit(s32 exit_code) __attribute__((noreturn));
extern s32 ExecPS2(void *entry, void *gp, int num_args, char *args[]);
extern void LoadExecPS2(const char *filename, s32 num_args, char *args[]) __attribute__((noreturn));
extern void ExecOSD(int num_args, char *args[]) __attribute__((noreturn));

/* Alarm update functions */
extern void InitAlarm(void); // Run by _InitSys

/* libosd update functions */
extern void InitExecPS2(void); // ExecPS2 patch only. Run by _InitSys, Exit, LoadExecPS2, ExecPS2 and ExecOSD
extern void InitOsd(void);     // ExecPS2 + System Configuration patches. Please refer to the comments within libosd_full.c

extern int PatchIsNeeded(void); // Indicates whether the patch is required.

// Debug (print) update functions:
extern void InitDebug(void);

/* Glue routines.  */
extern int DIntr(void);
extern int EIntr(void);

extern int EnableIntc(int intc);
extern int DisableIntc(int intc);
extern int EnableDmac(int dmac);
extern int DisableDmac(int dmac);

extern int iEnableIntc(int intc);
extern int iDisableIntc(int intc);
extern int iEnableDmac(int dmac);
extern int iDisableDmac(int dmac);

extern void SyncDCache(void *start, void *end);
extern void iSyncDCache(void *start, void *end);
extern void InvalidDCache(void *start, void *end);
extern void iInvalidDCache(void *start, void *end);

/* System call prototypes */
extern void ResetEE(u32 init_bitfield);
extern void SetGsCrt(s16 interlace, s16 pal_ntsc, s16 field);
extern void KExit(s32 exit_code) __attribute__((noreturn));
extern void _LoadExecPS2(const char *filename, s32 num_args, char *args[]) __attribute__((noreturn));
extern s32 _ExecPS2(void *entry, void *gp, int num_args, char *args[]);
extern void RFU009(u32 arg0, u32 arg1);
extern s32 AddSbusIntcHandler(s32 cause, void (*handler)(int call));
extern s32 RemoveSbusIntcHandler(s32 cause);
extern s32 Interrupt2Iop(s32 cause);
extern void SetVTLBRefillHandler(s32 handler_num, void *handler_func);
extern void SetVCommonHandler(s32 handler_num, void *handler_func);
extern void SetVInterruptHandler(s32 handler_num, void *handler_func);
extern s32 AddIntcHandler(s32 cause, s32 (*handler_func)(s32 cause), s32 next);
extern s32 AddIntcHandler2(s32 cause, s32 (*handler_func)(s32 cause, void *arg, void *addr), s32 next, void *arg);
extern s32 RemoveIntcHandler(s32 cause, s32 handler_id);
extern s32 AddDmacHandler(s32 channel, s32 (*handler)(s32 channel), s32 next);
extern s32 AddDmacHandler2(s32 channel, s32 (*handler)(s32 channel, void *arg, void *addr), s32 next, void *arg);
extern s32 RemoveDmacHandler(s32 channel, s32 handler_id);
extern s32 _EnableIntc(s32 cause);
extern s32 _DisableIntc(s32 cause);
extern s32 _EnableDmac(s32 channel);
extern s32 _DisableDmac(s32 channel);

// Alarm value is in H-SYNC ticks.
extern s32 SetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 _SetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 ReleaseAlarm(s32 alarm_id);
extern s32 _ReleaseAlarm(s32 alarm_id);

extern s32 _iEnableIntc(s32 cause);
extern s32 _iDisableIntc(s32 cause);
extern s32 _iEnableDmac(s32 channel);
extern s32 _iDisableDmac(s32 channel);

extern s32 iSetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 _iSetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 iReleaseAlarm(s32 alarm_id);
extern s32 _iReleaseAlarm(s32 alarm_id);

extern s32 CreateThread(ee_thread_t *thread);
extern s32 DeleteThread(s32 thread_id);
extern s32 StartThread(s32 thread_id, void *args);
extern void ExitThread(void);
extern void ExitDeleteThread(void);
extern s32 TerminateThread(s32 thread_id);
extern s32 iTerminateThread(s32 thread_id);
// extern void DisableDispatchThread(void);  // not supported
// extern void EnableDispatchThread(void);   // not supported
extern s32 ChangeThreadPriority(s32 thread_id, s32 priority);
extern s32 iChangeThreadPriority(s32 thread_id, s32 priority);
extern s32 RotateThreadReadyQueue(s32 priority);
extern s32 _iRotateThreadReadyQueue(s32 priority);
extern s32 ReleaseWaitThread(s32 thread_id);
extern s32 iReleaseWaitThread(s32 thread_id);
extern s32 GetThreadId(void);
extern s32 _iGetThreadId(void); // This is actually GetThreadId(), used for a hack by SCE to work around the iWakeupThread design flaw.
extern s32 ReferThreadStatus(s32 thread_id, ee_thread_status_t *info);
extern s32 iReferThreadStatus(s32 thread_id, ee_thread_status_t *info);
extern s32 SleepThread(void);
extern s32 WakeupThread(s32 thread_id);
extern s32 _iWakeupThread(s32 thread_id);
extern s32 CancelWakeupThread(s32 thread_id);
extern s32 iCancelWakeupThread(s32 thread_id);
extern s32 SuspendThread(s32 thread_id);
extern s32 _iSuspendThread(s32 thread_id);
extern s32 ResumeThread(s32 thread_id);
extern s32 iResumeThread(s32 thread_id);

extern u8 RFU059(void);

extern void *SetupThread(void *gp, void *stack, s32 stack_size, void *args, void *root_func);
extern void SetupHeap(void *heap_start, s32 heap_size);
extern void *EndOfHeap(void);

extern s32 CreateSema(ee_sema_t *sema);
extern s32 DeleteSema(s32 sema_id);
extern s32 SignalSema(s32 sema_id);
extern s32 iSignalSema(s32 sema_id);
extern s32 WaitSema(s32 sema_id);
extern s32 PollSema(s32 sema_id);
extern s32 iPollSema(s32 sema_id);
extern s32 ReferSemaStatus(s32 sema_id, ee_sema_t *sema);
extern s32 iReferSemaStatus(s32 sema_id, ee_sema_t *sema);
extern s32 iDeleteSema(s32 sema_id);
extern void SetOsdConfigParam(void *addr);
extern void GetOsdConfigParam(void *addr);
extern void GetGsHParam(void *addr1, void *addr2, void *addr3);
extern s32 GetGsVParam(void);
extern void SetGsHParam(void *addr1, void *addr2, void *addr3, void *addr4);
extern void SetGsVParam(s32 arg1);

// TLB functions are only available if InitTLBFunctions() is run (Normally run by crt0).
extern int PutTLBEntry(unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int iPutTLBEntry(unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int _SetTLBEntry(unsigned int index, unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int iSetTLBEntry(unsigned int index, unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int GetTLBEntry(unsigned int index, unsigned int *PageMask, unsigned int *EntryHi, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int iGetTLBEntry(unsigned int index, unsigned int *PageMask, unsigned int *EntryHi, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int ProbeTLBEntry(unsigned int EntryHi, unsigned int *PageMask, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int iProbeTLBEntry(unsigned int EntryHi, unsigned int *PageMask, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int ExpandScratchPad(unsigned int page);

extern void EnableIntcHandler(u32 cause);
extern void iEnableIntcHandler(u32 cause);
extern void DisableIntcHandler(u32 cause);
extern void iDisableIntcHandler(u32 cause);
extern void EnableDmacHandler(u32 channel);
extern void iEnableDmacHandler(u32 channel);
extern void DisableDmacHandler(u32 channel);
extern void iDisableDmacHandler(u32 channel);
extern void KSeg0(s32 arg1);
extern s32 EnableCache(s32 cache);
extern s32 DisableCache(s32 cache);
extern u32 GetCop0(s32 reg_id);
extern void FlushCache(s32 operation);
extern u32 CpuConfig(u32 config);
extern u32 iGetCop0(s32 reg_id);
extern void iFlushCache(s32 operation);
extern u32 iCpuConfig(u32 config);
extern void SetCPUTimerHandler(void (*handler)(void));
extern void SetCPUTimer(s32 compval);

// These two are not available in the unpatched Protokernel (Unpatched SCPH-10000 and SCPH-15000 kernels).
extern void SetOsdConfigParam2(void *config, s32 size, s32 offset);
extern void GetOsdConfigParam2(void *config, s32 size, s32 offset);

extern u64 GsGetIMR(void);
extern u64 iGsGetIMR(void);
extern u64 GsPutIMR(u64 imr);
extern u64 iGsPutIMR(u64 imr);
extern void SetPgifHandler(void *handler);
extern void SetVSyncFlag(u32 *, u64 *);
extern void SetSyscall(s32 syscall_num, void *handler);
extern void _print(const char *fmt, ...);    // Disabled by default, must call InitDebug() to enable

extern void sceSifStopDma(void); // Disables SIF0 (IOP -> EE).

extern int sceSifDmaStat(int trid);
extern int sceiSifDmaStat(int trid);
extern int sceSifSetDma(SifDmaTransfer_t *dmat, int count);
extern int isceSifSetDma(SifDmaTransfer_t *dmat, int count);

// Enables SIF0 (IOP -> EE). Sets channel 5 CHCR to 0x184 (CHAIN, TIE and STR).
extern void sceSifSetDChain(void);
extern void isceSifSetDChain(void);

// Sets/gets SIF register values (Refer to sifdma.h for a register list).
extern int sceSifSetReg(u32 register_num, int register_value);
extern int sceSifGetReg(u32 register_num);

extern void _ExecOSD(int num_args, char *args[]) __attribute__((noreturn));
extern s32 Deci2Call(s32, u32 *);
extern void PSMode(void);
extern s32 MachineType(void);
extern s32 GetMemorySize(void);

// Internal function for getting board-specific offsets, only present in later kernels (ROMVER > 20010608).
extern void _GetGsDxDyOffset(int mode, int *dx, int *dy, int *dw, int *dh);

// Internal function for reinitializing the TLB, only present in later kernels. Please use InitTLB() instead to initialize the TLB with all kernels.
extern int _InitTLB(void);
/* (DESR kernels only) Sets the memory size. mode != 1 -> 64MB mode, mode == 1 -> 32MB mode.
   The mode is only binding when either _InitTLB() or the PSX ExecPS2() syscall is called.
   The stack pointer must remain in range of usable memory, or a TLB exception will occur. */
extern int SetMemoryMode(int mode); // Arbitrarily named.
/* (DESR kernels only) Get the value set by SetMemoryMode. */
extern int GetMemoryMode(void); // Arbitrarily named.

extern void _SyncDCache(void *start, void *end);
extern void _InvalidDCache(void *start, void *end);

extern void *GetSyscallHandler(int syscall_no);
extern void *GetExceptionHandler(int except_no);
extern void *GetInterruptHandler(int intr_no);

/* Helper functions for kernel patching */
extern int kCopy(void *dest, const void *src, int size);
extern int kCopyBytes(void *dest, const void *src, int size);
extern int Copy(void *dest, const void *src, int size);
extern void setup(int syscall_num, void *handler); // alias of "SetSyscall"
extern void *GetEntryAddress(int syscall);

// For backwards compatibility
#define SifStopDma(...) sceSifStopDma(__VA_ARGS__)
// SifDmaStat defined in sifdma.h
#define iSifDmaStat(...) isceSifDmaStat(__VA_ARGS__)
// SifSetDma defined in sifdma.h
#define iSifSetDma(...) isceSifSetDma(__VA_ARGS__)
#define SifSetDChain(...) sceSifSetDChain(__VA_ARGS__)
#define iSifSetDChain(...) isceSifSetDChain(__VA_ARGS__)
#define SifSetReg(...) sceSifSetReg(__VA_ARGS__)
#define SifGetReg(...) sceSifGetReg(__VA_ARGS__)

// Helpers marcos for no-patch versions 
// Useful to build a special version of libkernel that does not contain any runtime patches (useful for loaders/resident programs).
#define DISABLE_PATCHED_Exit() \
    void Exit(s32 exit_code) { KExit(exit_code); }
    
#define DISABLE_PATCHED_LoadExecPS2() \
    void LoadExecPS2(const char *filename, s32 num_args, char *args[]) { _LoadExecPS2(filename, num_args, args); }

#define NO_PATCHED_ExecOSD() \
    void ExecOSD(int num_args, char *args[]) { _ExecOSD(num_args, args); }

#define DISABLE_TimerSystemTime() \
    void _ps2sdk_init_timer() {} \
    void _ps2sdk_deinit_timer() {}

#define DISABLE_TimerAlarm() \
    void ForTimer_InitAlarm(void) {}

#define DISABLE_PATCHED_ALARMS() \
    void InitAlarm(void) {}

#define DISABLE_PATCHED_THREADS() \
    int InitThread(void) { return 0; } \
    s32 iRotateThreadReadyQueue(s32 priority) { return _iRotateThreadReadyQueue(priority); } \
    s32 iWakeupThread(s32 thread_id) { return _iWakeupThread(thread_id); } \
    s32 iSuspendThread(s32 thread_id) { return _iSuspendThread(thread_id); }

#define DISABLE_PATCHED_ExecPS2() \
    void InitExecPS2(void) {} \
    s32 ExecPS2(void *entry, void *gp, int num_args, char *args[]) { return _ExecPS2(entry, gp, num_args, args); }

#define DISABLE_PATCHED_TLBFunctions() \
    void InitTLBFunctions(void) {} \
    void InitTLB(void) {}

#define DISABLE_PATCHED_FUNCTIONS() \
    DISABLE_PATCHED_ALARMS() \
    DISABLE_PATCHED_THREADS() \
    DISABLE_PATCHED_ExecPS2() \
    DISABLE_PATCHED_TLBFunctions() \
    DISABLE_PATCHED_Exit() \
    DISABLE_PATCHED_LoadExecPS2()

#define DISABLE_EXTRA_TIMERS_FUNCTIONS() \
    DISABLE_TimerSystemTime() \
    DISABLE_TimerAlarm()

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_H__ */
