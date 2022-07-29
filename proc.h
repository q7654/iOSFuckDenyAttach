//
//  proc.h
//  iOSFuckDenyAttach
//
//  Created by q7654 on 2022/7/27.
//

#ifndef proc_h
#define proc_h

//#define CONFIG_DTRACE 1

#define CONFIG_PERSONAS 1
//#define _PROC_HAS_SCHEDINFO_ 1

typedef char command_t[MAXCOMLEN + 1];
typedef char proc_name_t[2*MAXCOMLEN + 1];
typedef struct _lck_mtx_ {
    union {
        uintptr_t                                       lck_mtx_data;   /* Thread pointer plus lock bits */
        uintptr_t                                       lck_mtx_tag;    /* Tag for type */
    };                                                                                              /* arm: 4   arm64: 8 */
    union {
        struct {
            uint16_t                                lck_mtx_waiters;/* Number of waiters */
            uint8_t                                 lck_mtx_pri;    /* unused */
            uint8_t                                 lck_mtx_type;   /* Type */
        };
        struct {
            struct _lck_mtx_ext_    *lck_mtx_ptr;   /* Indirect pointer */
        };
    };                                                                                              /* arm: 4   arm64: 8 */
} lck_mtx_t;

typedef void * kauth_cred_t;

typedef struct {
    uintptr_t               opaque[2];
} lck_spin_t;
typedef union {
    struct {
        uint16_t        shared_count;           /* No. of shared granted request */
        uint16_t        interlock:              1,      /* Interlock */
            priv_excl:              1,                          /* priority for Writer */
            want_upgrade:   1,                          /* Read-to-write upgrade waiting */
            want_excl:              1,                          /* Writer is waiting, or locked for write */
            r_waiting:              1,                          /* Someone is sleeping on lock */
            w_waiting:              1,                          /* Writer is sleeping on lock */
            can_sleep:              1,                          /* Can attempts to lock go to sleep? */
            _pad2:                  8,                          /* padding */
            tag_valid:              1;                          /* Field is actually a tag, not a bitfield */
#if __arm64__
        uint32_t        _pad4;
#endif
    };
    struct {
        uint32_t        data;                                           /* Single word version of bitfields and shared count */
#if __arm64__
        uint32_t        lck_rw_pad4;
#endif
    };
} lck_rw_word_t;

typedef struct {
    lck_rw_word_t   word;
    thread_t                lck_rw_owner;
} lck_rw_t;     /* arm: 8  arm64: 16 */
// https://opensource.apple.com/source/xnu/xnu-7195.141.2/bsd/sys/proc_internal.h.auto.html

/*
 * Description of a process.
 *
 * This structure contains the information needed to manage a thread of
 * control, known in UN*X as a process; it has references to substructures
 * containing descriptions of things that the process uses, but may share
 * with related processes.  The process structure and the substructures
 * are always addressible except for those marked "(PROC ONLY)" below,
 * which might be addressible only on a processor on which the process
 * is running.
 */
struct  proc {
    LIST_ENTRY(proc) p_list;                /* List of all processes. */

    void *          task;       /* corresponding task (static)*/
    struct  proc *  p_pptr;   /* Pointer to parent process.(LL) */
    pid_t           p_ppid;                 /* process's parent pid number */
    pid_t           p_original_ppid;        /* process's original parent pid number, doesn't change if reparented */
    pid_t           p_pgrpid;               /* process group id of the process (LL)*/
    uid_t           p_uid;
    gid_t           p_gid;
    uid_t           p_ruid;
    gid_t           p_rgid;
    uid_t           p_svuid;
    gid_t           p_svgid;
    uint64_t        p_uniqueid;             /* process unique ID - incremented on fork/spawn/vfork, remains same across exec. */
    uint64_t        p_puniqueid;            /* parent's unique ID - set on fork/spawn/vfork, doesn't change if reparented. */

    lck_mtx_t       p_mlock;                /* mutex lock for proc */
    pid_t           p_pid;                  /* Process identifier. (static)*/
    char            p_stat;                 /* S* process status. (PL)*/
    char            p_shutdownstate;
    char            p_kdebug;               /* P_KDEBUG eq (CC)*/
    char            p_btrace;               /* P_BTRACE eq (CC)*/

    LIST_ENTRY(proc) p_pglist;              /* List of processes in pgrp.(PGL) */
    LIST_ENTRY(proc) p_sibling;             /* List of sibling processes. (LL)*/
    LIST_HEAD(, proc) p_children;           /* Pointer to list of children. (LL)*/
    TAILQ_HEAD(, uthread) p_uthlist;        /* List of uthreads  (PL) */

    LIST_ENTRY(proc) p_hash;                /* Hash chain. (LL)*/

#if CONFIG_PERSONAS
    struct persona  *p_persona;
    LIST_ENTRY(proc) p_persona_list;
#endif

    lck_mtx_t       p_fdmlock;              /* proc lock to protect fdesc */
    lck_mtx_t       p_ucred_mlock;          /* mutex lock to protect p_ucred */

    /* substructures: */
    kauth_cred_t    p_ucred; /* Process owner's identity. (PUCL) */
    struct  filedesc *p_fd;                 /* Ptr to open files structure. (PFDL) */
    struct  pstats *p_stats;                /* Accounting/statistics (PL). */
    struct  plimit *p_limit;                /* Process limits.(PL) */

    struct  sigacts *p_sigacts;             /* Signal actions, state (PL) */
    lck_spin_t      p_slock;                /* spin lock for itimer/profil protection */

    int             p_siglist;              /* signals captured back from threads */
    unsigned int    p_flag;                 /* P_* flags. (atomic bit ops) */
    unsigned int    p_lflag;                /* local flags  (PL) */
    unsigned int    p_listflag;             /* list flags (LL) */
    unsigned int    p_ladvflag;             /* local adv flags (atomic) */
    int             p_refcount;             /* number of outstanding users(LL) */
    int             p_childrencnt;          /* children holding ref on parent (LL) */
    int             p_parentref;            /* children lookup ref on parent (LL) */
    pid_t           p_oppid;                /* Save parent pid during ptrace. XXX */
    u_int           p_xstat;                /* Exit status for wait; also stop signal. */

#ifdef _PROC_HAS_SCHEDINFO_
    /* may need cleanup, not used */
    u_int           p_estcpu;               /* Time averaged value of p_cpticks.(used by aio and proc_comapre) */
    fixpt_t         p_pctcpu;               /* %cpu for this process during p_swtime (used by aio)*/
    u_int           p_slptime;              /* used by proc_compare */
#endif /* _PROC_HAS_SCHEDINFO_ */

    struct  itimerval p_realtimer;          /* Alarm timer. (PSL) */
    struct  timeval p_rtime;                /* Real time.(PSL)  */
    struct  itimerval p_vtimer_user;        /* Virtual timers.(PSL)  */
    struct  itimerval p_vtimer_prof;        /* (PSL) */

    struct  timeval p_rlim_cpu;             /* Remaining rlim cpu value.(PSL) */
    int             p_debugger;             /*  NU 1: can exec set-bit programs if suser */
    boolean_t       sigwait;        /* indication to suspend (PL) */
    void    *sigwait_thread;        /* 'thread' holding sigwait(PL)  */
    void    *exit_thread;           /* Which thread is exiting(PL)  */
    void *  p_vforkact;             /* activation running this vfork proc)(static)  */
    int     p_vforkcnt;             /* number of outstanding vforks(PL)  */
    int     p_fpdrainwait;          /* (PFDL) */
    /* Following fields are info from SIGCHLD (PL) */
    pid_t   si_pid;                 /* (PL) */
    u_int   si_status;              /* (PL) */
    u_int   si_code;                /* (PL) */
    uid_t   si_uid;                 /* (PL) */

    void * vm_shm;                  /* (SYSV SHM Lock) for sysV shared memory */

#if CONFIG_DTRACE
    user_addr_t                     p_dtrace_argv;                  /* (write once, read only after that) */
    user_addr_t                     p_dtrace_envp;                  /* (write once, read only after that) */
    lck_mtx_t                       p_dtrace_sprlock;               /* sun proc lock emulation */
    uint8_t                         p_dtrace_stop;                  /* indicates a DTrace-desired stop */
    int                             p_dtrace_probes;                /* (PL) are there probes for this proc? */
    u_int                           p_dtrace_count;                 /* (sprlock) number of DTrace tracepoints */
    struct dtrace_ptss_page*        p_dtrace_ptss_pages;            /* (sprlock) list of user ptss pages */
    struct dtrace_ptss_page_entry*  p_dtrace_ptss_free_list;        /* (atomic) list of individual ptss entries */
    struct dtrace_helpers*          p_dtrace_helpers;               /* (dtrace_lock) DTrace per-proc private */
    struct dof_ioctl_data*          p_dtrace_lazy_dofs;             /* (sprlock) unloaded dof_helper_t's */
#endif /* CONFIG_DTRACE */

/* XXXXXXXXXXXXX BCOPY'ed on fork XXXXXXXXXXXXXXXX */
/* The following fields are all copied upon creation in fork. */
#define p_startcopy     p_argslen

    u_int   p_argslen;       /* Length of process arguments. */
    int     p_argc;                 /* saved argc for sysctl_procargs() */
    user_addr_t user_stack;         /* where user stack was allocated */
    struct  vnode * p_textvp;       /* Vnode of executable. */
    off_t   p_textoff;              /* offset in executable vnode */

    sigset_t p_sigmask;             /* DEPRECATED */
    sigset_t p_sigignore;   /* Signals being ignored. (PL) */
    sigset_t p_sigcatch;    /* Signals being caught by user.(PL)  */

    u_char  p_priority;     /* (NU) Process priority. */
    u_char  p_resv0;        /* (NU) User-priority based on p_cpu and p_nice. */
    char    p_nice;         /* Process "nice" value.(PL) */
    u_char  p_resv1;        /* (NU) User-priority based on p_cpu and p_nice. */

    // types currently in sys/param.h
    command_t   p_comm;
    proc_name_t p_name;     /* can be changed by the process */
    uint8_t p_xhighbits;    /* Stores the top byte of exit status to avoid truncation*/
    pid_t   p_contproc;     /* last PID to send us a SIGCONT (PL) */

    struct  pgrp *  p_pgrp; /* Pointer to process group. (LL) */
    uint32_t        p_csflags;      /* flags for codesign (PL) */
    uint32_t        p_pcaction;     /* action  for process control on starvation */
    uint8_t p_uuid[16];             /* from LC_UUID load command */

    /*
     * CPU type and subtype of binary slice executed in
     * this process.  Protected by proc lock.
     */
    cpu_type_t      p_cputype;
    cpu_subtype_t   p_cpusubtype;

    uint8_t  *syscall_filter_mask;          /* syscall filter bitmask (length: nsysent bits) */
    uint32_t        p_platform;
    uint32_t        p_min_sdk;
    uint32_t        p_sdk;

/* End area that is copied on creation. */
/* XXXXXXXXXXXXX End of BCOPY'ed on fork (AIOLOCK)XXXXXXXXXXXXXXXX */
#define p_endcopy       p_aio_total_count
    int             p_aio_total_count;              /* all allocated AIO requests for this proc */
    TAILQ_HEAD(, aio_workq_entry ) p_aio_activeq;   /* active async IO requests */
    TAILQ_HEAD(, aio_workq_entry ) p_aio_doneq;     /* completed async IO requests */

    struct klist p_klist;  /* knote list (PL ?)*/

    struct  rusage_superset *p_ru;  /* Exit information. (PL) */
    thread_t        p_signalholder;
    thread_t        p_transholder;
    int             p_sigwaitcnt;
    /* DEPRECATE following field  */
    u_short p_acflag;       /* Accounting flags. */
    volatile u_short p_vfs_iopolicy;        /* VFS iopolicy flags. (atomic bit ops) */

    user_addr_t     p_threadstart;          /* pthread start fn */
    user_addr_t     p_wqthread;             /* pthread workqueue fn */
    int     p_pthsize;                      /* pthread size */
    uint32_t        p_pth_tsd_offset;       /* offset from pthread_t to TSD for new threads */
    user_addr_t     p_stack_addr_hint;      /* stack allocation hint for wq threads */
    struct workqueue *_Atomic p_wqptr;                      /* workq ptr */

    struct  timeval p_start;                /* starting time */
    void *  p_rcall;
    int             p_ractive;
    int     p_idversion;            /* version of process identity */
    void *  p_pthhash;                      /* pthread waitqueue hash */
    volatile uint64_t was_throttled __attribute__((aligned(8))); /* Counter for number of throttled I/Os */
    volatile uint64_t did_throttle __attribute__((aligned(8)));  /* Counter for number of I/Os this proc throttled */

#if DIAGNOSTIC
    unsigned int p_fdlock_pc[4];
    unsigned int p_fdunlock_pc[4];
#if SIGNAL_DEBUG
    unsigned int lockpc[8];
    unsigned int unlockpc[8];
#endif /* SIGNAL_DEBUG */
#endif /* DIAGNOSTIC */
    uint64_t        p_dispatchqueue_offset;
    uint64_t        p_dispatchqueue_serialno_offset;
    uint64_t        p_dispatchqueue_label_offset;
    uint64_t        p_return_to_kernel_offset;
    uint64_t        p_mach_thread_self_offset;
#if VM_PRESSURE_EVENTS
    struct timeval  vm_pressure_last_notify_tstamp;
#endif

#if CONFIG_MEMORYSTATUS
    /* Fields protected by proc list lock */
    TAILQ_ENTRY(proc) p_memstat_list;               /* priority bucket link */
    uint32_t          p_memstat_state;              /* state. Also used as a wakeup channel when the memstat's LOCKED bit changes */
    int32_t           p_memstat_effectivepriority;  /* priority after transaction state accounted for */
    int32_t           p_memstat_requestedpriority;  /* active priority */
    int32_t           p_memstat_assertionpriority;  /* assertion driven priority */
    uint32_t          p_memstat_dirty;              /* dirty state */
#if CONFIG_FREEZE
    uint8_t           p_memstat_freeze_skip_reason; /* memorystaus_freeze_skipped_reason_t. Protected by the freezer mutex. */
#endif
    uint64_t          p_memstat_userdata;           /* user state */
    uint64_t          p_memstat_idledeadline;       /* time at which process became clean */
    uint64_t          p_memstat_idle_start;         /* abstime process transitions into the idle band */
    uint64_t          p_memstat_idle_delta;         /* abstime delta spent in idle band */
    int32_t           p_memstat_memlimit;           /* cached memory limit, toggles between active and inactive limits */
    int32_t           p_memstat_memlimit_active;    /* memory limit enforced when process is in active jetsam state */
    int32_t           p_memstat_memlimit_inactive;  /* memory limit enforced when process is in inactive jetsam state */
    int32_t           p_memstat_relaunch_flags;     /* flags indicating relaunch behavior for the process */
#if CONFIG_FREEZE
    uint32_t          p_memstat_freeze_sharedanon_pages; /* shared pages left behind after freeze */
    uint32_t          p_memstat_frozen_count;
    uint32_t          p_memstat_thaw_count;
    uint32_t          p_memstat_last_thaw_interval; /* In which freezer interval was this last thawed? */
#endif /* CONFIG_FREEZE */
#endif /* CONFIG_MEMORYSTATUS */

    /* cached proc-specific data required for corpse inspection */
    pid_t             p_responsible_pid;    /* pid resonsible for this process */
    _Atomic uint32_t  p_user_faults; /* count the number of user faults generated */

    uint32_t          p_memlimit_increase; /* byte increase for memory limit for dyld SPI rdar://problem/49950264, structure packing 32-bit and 64-bit */

    struct os_reason     *p_exit_reason;

#if CONFIG_PROC_UDATA_STORAGE
    uint64_t        p_user_data;                    /* general-purpose storage for userland-provided data */
#endif /* CONFIG_PROC_UDATA_STORAGE */

    char * p_subsystem_root_path;
    lck_rw_t        p_dirs_lock;                    /* keeps fd_cdir and fd_rdir stable across a lookup */
    pid_t           p_sessionid;
};


#endif /* proc_h */
