Files to analyze:

  1. /sys/i386/i386/machdep.c
  2. /sys/i386/i386/exception.s
  3. /sys/i386/i386/trap.c

## /sys/i386/i386/machdep.c

Before we dive into the code for traps and interrupts let's take a look at the data structures for the GDT and IDT in FreeBSD.

### i386 Data Structure Definitions

Here are the data structure fields according to Intel.

![](notes/gdt_entry.png)

![](notes/idt_entry.png)

Here are the data structures represented in FreeBSD.

```c
/*
 * Memory and System segment descriptors
 */

struct	segment_descriptor {
		unsigned sd_lolimit:16;			/* segment extent (lsb) */
		unsigned sd_lobase:24 __packed;	/* segment base address (lsb) */
		unsigned sd_type:5;				/* segment type */
		unsigned sd_dpl:2;				/* segment descriptor priority level */
		unsigned sd_p:1;				/* segment descriptor present */
		unsigned sd_hilimit:4;			/* seg,emt extent (msb) */
		unsigned sd_xx:2;				/* unused */
		unsigned sd_def32:1;			/* default 32 vs 16 bit size */
		unsigned sd_gran:1;				/* limit granularity (byte/page units) */
		unsigned sd_hibase:8;			/* segment base address (msb) */
};

/*
 * Gate descriptors (e.g. indirect descriptors)
 */
struct	gate_descriptor {
		unsigned gd_looffset:16;
		unsigned gd_selector:16;	/* gate segment selector */
		unsigned gd_stkcpy:5;		/* number of stack words to copy */
		unsigned gd_xx:3;			/* unused */
		unsigned gd_type:5;			/* segment type */
		unsigned gd_dpl:2;			/* segment descriptor priority level */
		unsigned gd_p:1;			/* segment descriptor present */
		unsigned gd_hioffset:16;	/* gate offset (msb) */
};

/*
 * Generic descriptor
 */
union	descriptor {
		struct	segment_descriptor sd;
		struct	gate_descriptor gd;
};

/* software prototypes -- in more palatable form */
struct soft_segment_descriptor gdt_segs[] = {
/* GNULL_SEL    0 Null Descriptor */
{       0x0,                    /* segment base address  */
        0x0,                    /* length */
        0,                      /* segment type */
        0,                      /* segment descriptor priority level */
        0,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GCODE_SEL    1 Code Descriptor for kernel */
{       0x0,                    /* segment base address  */
        0xfffff,                /* length - all address space */
        SDT_MEMERA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        1,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GDATA_SEL    2 Data Descriptor for kernel */
{       0x0,                    /* segment base address  */
        0xfffff,                /* length - all address space */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        1,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GPRIV_SEL    3 SMP Per-Processor Private Data Descriptor */
{       0x0,                    /* segment base address  */
        0xfffff,                /* length - all address space */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        1,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GPROC0_SEL   4 Proc 0 Tss Descriptor */
{
        0x0,                    /* segment base address */
        sizeof(struct i386tss)-1,/* length  */
        SDT_SYS386TSS,          /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* unused - default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GLDT_SEL     5 LDT Descriptor */
{       (int) ldt,              /* segment base address  */
        sizeof(ldt)-1,          /* length - all address space */
        SDT_SYSLDT,             /* segment type */
        SEL_UPL,                /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* unused - default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GUSERLDT_SEL 6 User LDT Descriptor per process */
{       (int) ldt,              /* segment base address  */
        (512 * sizeof(union descriptor)-1),             /* length */
        SDT_SYSLDT,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* unused - default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GTGATE_SEL   7 Null Descriptor - Placeholder */
{       0x0,                    /* segment base address  */
        0x0,                    /* length - all address space */
        0,                      /* segment type */
        0,                      /* segment descriptor priority level */
        0,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GBIOSLOWMEM_SEL 8 BIOS access to realmode segment 0x40, must be #8 in GDT */
{       0x400,                  /* segment base address */
        0xfffff,                /* length */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        1,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GPANIC_SEL   9 Panic Tss Descriptor */
{       (int) &dblfault_tss,    /* segment base address  */
        sizeof(struct i386tss)-1,/* length - all address space */
        SDT_SYS386TSS,          /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* unused - default 32 vs 16 bit size */
        0                       /* limit granularity (byte/page units)*/ },
/* GBIOSCODE32_SEL 10 BIOS 32-bit interface (32bit Code) */
{       0,                      /* segment base address (overwritten)  */
        0xfffff,                /* length */
        SDT_MEMERA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GBIOSCODE16_SEL 11 BIOS 32-bit interface (16bit Code) */
{       0,                      /* segment base address (overwritten)  */
        0xfffff,                /* length */
        SDT_MEMERA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GBIOSDATA_SEL 12 BIOS 32-bit interface (Data) */
{       0,                      /* segment base address (overwritten) */
        0xfffff,                /* length */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        1,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GBIOSUTIL_SEL 13 BIOS 16-bit interface (Utility) */
{       0,                      /* segment base address (overwritten) */
        0xfffff,                /* length */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
/* GBIOSARGS_SEL 14 BIOS 16-bit interface (Arguments) */
{       0,                      /* segment base address (overwritten) */
        0xfffff,                /* length */
        SDT_MEMRWA,             /* segment type */
        0,                      /* segment descriptor priority level */
        1,                      /* segment descriptor present */
        0, 0,
        0,                      /* default 32 vs 16 bit size */
        1                       /* limit granularity (byte/page units)*/ },
};
```

### I386 Data Structure Declarations

We declare the i386 data structures in /sys/i386/i386/machdep.c. We declare an array of size NGDT * MAXCPU for GDT descriptor entries, where each structure is presumably for each processor in a MP system. We also declare an array of size NIDT gate descriptors to create the IDT in memory.

```c
/*
 * Initialize segments & interrupt table
 */

int	_default_ldt;
union descriptor gdt[NGDT * MAXCPU];
static struct gate_descriptor idt0[NIDT];
struct gate_descriptor *idt = &idt0[0];
```
### Setting the GDT

We initialize the GDT and IDT with the C function *init386* in FreeBSD.

```c
        /*
         * make gdt memory segments, the code segment goes up to end of the
         * page with etext in it, the data segment goes to the end of
         * the address space
         */
        /*
         * XXX text protection is temporarily (?) disabled.  The limit was
         * i386_btop(round_page(etext)) - 1.
         */
        gdt_segs[GCODE_SEL].ssd_limit = atop(0 - 1);	/* Doesn't change GCODE_SEL.ssd_limit */
        gdt_segs[GDATA_SEL].ssd_limit = atop(0 - 1);	/* Doesn't change GDATA_SEL.ssd_limit */
#ifdef SMP
/* For SMP systems, we create an array of privatespace structures where each entry contains a
   per-cpu structure along with an idle stack. */
        pc = &SMP_prvspace[0].pcpu;
        gdt_segs[GPRIV_SEL].ssd_limit =
                atop(sizeof(struct privatespace) - 1);	/* Converts the size of privatespace to 4KB units */
#else
/* For uniprocessor systems, we use the static structure __pcpu to store all out per-cpu data */
        pc = &__pcpu;
        gdt_segs[GPRIV_SEL].ssd_limit =
                atop(sizeof(struct pcpu) - 1);	/* Converts the size of pcpu to 4KB units */
#endif
/* Oddly enough, I cannot find the pc_common_tss field in the pcpu structure */
        gdt_segs[GPRIV_SEL].ssd_base = (int) pc;
        gdt_segs[GPROC0_SEL].ssd_base = (int) &pc->pc_common_tss;

        for (x = 0; x < NGDT; x++)
                ssdtosd(&gdt_segs[x], &gdt[x].sd);	/* Copies data from gdt_segs[] to gdt[],
													   where gdt[] is the array of descriptor
													   structures */
        r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
        r_gdt.rd_base =  (int) gdt;
        lgdt(&r_gdt);
```

### Setting the IDT

To initialize the IDT, FreeBSD uses some preprocessor magic. Hence, let's examine IDTVEC first, which is located in /sys/i386/include/asmacros.h.

```c
#ifdef LOCORE
 /*
  * Convenience macros for declaring interrupt entry points and trap
  * stubs.
  */
#define IDTVEC(name)    ALIGN_TEXT; .globl __CONCAT(X,name); \
                        .type __CONCAT(X,name),@function; __CONCAT(X,name):		/* Note that the last line is a label! */
#define TRAP(a)         pushl $(a) ; jmp alltraps
 
#endif /* LOCORE */
```
Notice how IDTVEC(name) is a macro that includes the assembler directives .globl and .type and a unique label \_\_CONCAT(X,name):. With this definition, we are able to obtain the address for each assembly interrupt entry point by specifying the macro with the name of the interrupt.

```c
/* Excerpt from sys/i386/i386/apic_vector.s */

/*
 * Macros to create and destroy a trap frame.
 */
#define PUSH_FRAME                                                      \
        pushl   $0 ;            /* dummy error code */                  \
        pushl   $0 ;            /* dummy trap type */                   \
        pushal ;                /* 8 ints */                            \
        pushl   %ds ;           /* save data and extra segments ... */  \
        pushl   %es ;                                                   \
        pushl   %fs

#define POP_FRAME                                                       \
        popl    %fs ;                                                   \
        popl    %es ;                                                   \
        popl    %ds ;                                                   \
        popal ;                                                         \
        addl    $4+4,%esp

/*
 * I/O Interrupt Entry Point.  Rather than having one entry point for
 * each interrupt source, we use one entry point for each 32-bit word
 * in the ISR.  The handler determines the highest bit set in the ISR,
 * translates that into a vector, and passes the vector to the
 * lapic_handle_intr() function.
 */
#define ISR_VEC(index, vec_name)                                        \
        .text ;                                                         \
        SUPERALIGN_TEXT ;                                               \
IDTVEC(vec_name) ;                                                      \
        PUSH_FRAME ;                                                    \
        movl    $KDSEL, %eax ;  /* reload with kernel's data segment */ \
        mov     %ax, %ds ;                                              \
        mov     %ax, %es ;                                              \
        movl    $KPSEL, %eax ;  /* reload with per-CPU data segment */  \
        mov     %ax, %fs ;                                              \
        FAKE_MCOUNT(13*4(%esp)) ;                                       \
        movl    lapic, %edx ;   /* pointer to local APIC */             \
        movl    LA_ISR + 16 * (index)(%edx), %eax ;     /* load ISR */  \
        bsrl    %eax, %eax ;    /* index of highset set bit in ISR */   \
        jz      2f ;                                                    \
        addl    $(32 * index),%eax ;                                    \
1: ;                                                                    \
        pushl   %eax ;          /* pass the IRQ */                      \
        call    lapic_handle_intr ;                                     \
        addl    $4, %esp ;      /* discard parameter */                 \
        MEXITCOUNT ;                                                    \
        jmp     doreti ;                                                \
2:      movl    $-1, %eax ;     /* send a vector of -1 */               \
        jmp     1b

/*
 * Handle "spurious INTerrupts".
 * Notes:
 *  This is different than the "spurious INTerrupt" generated by an
 *  8259 PIC for missing INTs.  See the APIC documentation for details.
 *  This routine should NOT do an 'EOI' cycle.
 */
        .text
        SUPERALIGN_TEXT
IDTVEC(spuriousint)

        /* No EOI cycle used here */

        iret

MCOUNT\_LABEL(bintr2)
        ISR_VEC(1, apic_isr1)
        ISR_VEC(2, apic_isr2)
        ISR_VEC(3, apic_isr3)
        ISR_VEC(4, apic_isr4)
        ISR_VEC(5, apic_isr5)
        ISR_VEC(6, apic_isr6)
        ISR_VEC(7, apic_isr7)
MCOUNT\_LABEL(eintr2)

/* Excerpt from exception.s */

/*****************************************************************************/
/* Trap handling                                                             */
/*****************************************************************************/
/*
 * Trap and fault vector routines.
 *
 * Most traps are 'trap gates', SDT_SYS386TGT.  A trap gate pushes state on
 * the stack that mostly looks like an interrupt, but does not disable 
 * interrupts.  A few of the traps we are use are interrupt gates, 
 * SDT_SYS386IGT, which are nearly the same thing except interrupts are
 * disabled on entry.
 *
 * The cpu will push a certain amount of state onto the kernel stack for
 * the current process.  The amount of state depends on the type of trap 
 * and whether the trap crossed rings or not.  See i386/include/frame.h.  
 * At the very least the current EFLAGS (status register, which includes 
 * the interrupt disable state prior to the trap), the code segment register,
 * and the return instruction pointer are pushed by the cpu.  The cpu 
 * will also push an 'error' code for certain traps.  We push a dummy 
 * error code for those traps where the cpu doesn't in order to maintain 
 * a consistent frame.  We also push a contrived 'trap number'.
 *
 * The cpu does not push the general registers, we must do that, and we 
 * must restore them prior to calling 'iret'.  The cpu adjusts the %cs and
 * %ss segment registers, but does not mess with %ds, %es, or %fs.  Thus we
 * must load them with appropriate values for supervisor mode operation.
 */

MCOUNT_LABEL(user)
MCOUNT_LABEL(btrap)
 
IDTVEC(div)
         pushl $0; TRAP(T_DIVIDE)
IDTVEC(dbg)
         pushl $0; TRAP(T_TRCTRAP)
IDTVEC(nmi)
         pushl $0; TRAP(T_NMI)
IDTVEC(bpt)
         pushl $0; TRAP(T_BPTFLT)
IDTVEC(ofl)
         pushl $0; TRAP(T_OFLOW)
IDTVEC(bnd)
         pushl $0; TRAP(T_BOUND)
IDTVEC(ill)
         pushl $0; TRAP(T_PRIVINFLT)
IDTVEC(dna)
         pushl $0; TRAP(T_DNA)
IDTVEC(fpusegm)
         pushl $0; TRAP(T_FPOPFLT)
IDTVEC(tss)
         TRAP(T_TSSFLT)
IDTVEC(missing)
         TRAP(T_SEGNPFLT)
IDTVEC(stk)
         TRAP(T_STKFLT)
IDTVEC(prot)
         TRAP(T_PROTFLT)
IDTVEC(page)
         TRAP(T_PAGEFLT)
IDTVEC(mchk)
         pushl $0; TRAP(T_MCHK)
IDTVEC(rsvd)
         pushl $0; TRAP(T_RESERVED)
IDTVEC(fpu)
         pushl $0; TRAP(T_ARITHTRAP)
IDTVEC(align)
         TRAP(T_ALIGNFLT)
 
IDTVEC(xmm)
         pushl $0; TRAP(T_XMMFLT)
         
         /*
          * alltraps entry point.  Interrupts are enabled if this was a trap
          * gate (TGT), else disabled if this was an interrupt gate (IGT).
          * Note that int0x80_syscall is a trap gate.  Only page faults
          * use an interrupt gate.
          */
 
         SUPERALIGN_TEXT
         .globl  alltraps
         .type   alltraps,@function
alltraps:
         pushal
         pushl   %ds
         pushl   %es
         pushl   %fs
alltraps_with_regs_pushed:
         mov     $KDSEL,%ax
         mov     %ax,%ds
         mov     %ax,%es
         mov     $KPSEL,%ax
         mov     %ax,%fs
         FAKE_MCOUNT(13*4(%esp))
calltrap:
         FAKE_MCOUNT(btrap)              /* init "from" btrap -> calltrap */
         call    trap
 
         /*
          * Return via doreti to handle ASTs.
          */
         MEXITCOUNT
         jmp     doreti
 
/*
 * SYSCALL CALL GATE (old entry point for a.out binaries)
 *
 * The intersegment call has been set up to specify one dummy parameter.
 *
 * This leaves a place to put eflags so that the call frame can be
 * converted to a trap frame. Note that the eflags is (semi-)bogusly
 * pushed into (what will be) tf_err and then copied later into the
 * final spot. It has to be done this way because esp can't be just
 * temporarily altered for the pushfl - an interrupt might come in
 * and clobber the saved cs/eip.
 */
        SUPERALIGN_TEXT
IDTVEC(lcall_syscall)
        pushfl                          /* save eflags */
        popl    8(%esp)                 /* shuffle into tf_eflags */
        pushl   $7                      /* sizeof "lcall 7,0" */
        subl    $4,%esp                 /* skip over tf_trapno */
        pushal
        pushl   %ds
        pushl   %es
        pushl   %fs
        mov     $KDSEL,%ax              /* switch to kernel segments */
        mov     %ax,%ds
        mov     %ax,%es
        mov     $KPSEL,%ax
        mov     %ax,%fs
        FAKE_MCOUNT(13*4(%esp))
        call    syscall
        MEXITCOUNT
        jmp     doreti
```
Now that we understand what IDTVEC(name) is and where they are used in the header files, we can use the preprocessor magic to supply an address for each IDT entry using only its name.

```c
        /* exceptions */
        for (x = 0; x < NIDT; x++)
                setidt(x, &IDTVEC(rsvd), SDT_SYS386TGT, SEL_KPL,
                    GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_DE, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_DB, &IDTVEC(dbg),  SDT_SYS386IGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_NMI, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_BP, &IDTVEC(bpt),  SDT_SYS386IGT, SEL_UPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_OF, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_UPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_BR, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_UD, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_NM, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL
            , GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_DF, 0,  SDT_SYSTASKGT, SEL_KPL, GSEL(GPANIC_SEL, SEL_KPL));
        setidt(IDT_FPUGP, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_TS, &IDTVEC(tss),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_NP, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_SS, &IDTVEC(stk),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_GP, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_PF, &IDTVEC(page),  SDT_SYS386IGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_MF, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_AC, &IDTVEC(align), SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_MC, &IDTVEC(mchk),  SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_XF, &IDTVEC(xmm), SDT_SYS386TGT, SEL_KPL,
            GSEL(GCODE_SEL, SEL_KPL));
        setidt(IDT_SYSCALL, &IDTVEC(int0x80_syscall), SDT_SYS386TGT, SEL_UPL,
            GSEL(GCODE_SEL, SEL_KPL));

        r_idt.rd_limit = sizeof(idt0) - 1;
        r_idt.rd_base = (int) idt;
        lidt(&r_idt);
```
## /sys/i386/i386/trap.c

Both *trap()* and *syscall()* make use of a *trapframe* data structure that represents the stack after *pushal* and pushing the segment registers. The *trapframe* structure is thus defined as follows:

```c
/* Excerpt from /sys/i386/include/frame.h */

/*
 * Exception/Trap Stack Frame
 */

struct trapframe {
        int     tf_fs;		/* Last thing we push before calling trap() or syscall() */
        int     tf_es;
        int     tf_ds;		/* Every field below tf_ds is the from pushal */
        int     tf_edi;
        int     tf_esi;
        int     tf_ebp;
        int     tf_isp;
        int     tf_ebx;
        int     tf_edx;
        int     tf_ecx;
        int     tf_eax;
        int     tf_trapno;
        /* below portion defined in 386 hardware */
        int     tf_err;
        int     tf_eip;
        int     tf_cs;
        int     tf_eflags;
        /* below only when crossing rings (e.g. user to kernel) */
        int     tf_esp;
        int     tf_ss;
};
```

dd.Furthermore, *trap()* introduces some global data structures we should take note of. First, it makes a reference to a global variable *cnt* in an atomic operation, which is a global *vmmeter* structure used to gather statistics on system operation.

```c
/* Excerpt from /sys/sys/vmmeter.h */

/*
 * System wide statistics counters.
 */
struct vmmeter {
        /*
         * General system activity.
         */
        u_int v_swtch;          /* context switches */
        u_int v_trap;           /* calls to trap */
        u_int v_syscall;        /* calls to syscall() */
        u_int v_intr;           /* device interrupts */
        u_int v_soft;           /* software interrupts */
        /*
         * Virtual memory activity.
         */
        u_int v_vm_faults;      /* number of address memory faults */
  u_int v_cow_faults;     /* number of copy-on-writes */
        u_int v_cow_optim;      /* number of optimized copy-on-writes */
        u_int v_zfod;           /* pages zero filled on demand */
        u_int v_ozfod;          /* optimized zero fill pages */
        u_int v_swapin;         /* swap pager pageins */
        u_int v_swapout;        /* swap pager pageouts */
        u_int v_swappgsin;      /* swap pager pages paged in */
        u_int v_swappgsout;     /* swap pager pages paged out */
        u_int v_vnodein;        /* vnode pager pageins */
        u_int v_vnodeout;       /* vnode pager pageouts */
        u_int v_vnodepgsin;     /* vnode_pager pages paged in */
        u_int v_vnodepgsout;    /* vnode pager pages paged out */
        u_int v_intrans;        /* intransit blocking page faults */
        u_int v_reactivated;    /* number of pages reactivated from free list */
        u_int v_pdwakeups;      /* number of times daemon has awaken from sleep */
        u_int v_pdpages;        /* number of pages analyzed by daemon */

        u_int v_dfree;          /* pages freed by daemon */
        u_int v_pfree;          /* pages freed by exiting processes */
        u_int v_tfree;          /* total pages freed */
        /*
         * Distribution of page usages.
         */
        u_int v_page_size;      /* page size in bytes */
        u_int v_page_count;     /* total number of pages in system */
        u_int v_free_reserved;  /* number of pages reserved for deadlock */
        u_int v_free_target;    /* number of pages desired free */
        u_int v_free_min;       /* minimum number of pages desired free */
        u_int v_free_count;     /* number of pages free */
        u_int v_wire_count;     /* number of pages wired down */
        u_int v_active_count;   /* number of pages active */
        u_int v_inactive_target; /* number of pages desired inactive */
        u_int v_inactive_count; /* number of pages inactive */
        u_int v_cache_count;    /* number of pages on buffer cache queue */
        u_int v_cache_min;      /* min number of pages desired on cache queue */
        u_int v_cache_max;      /* max number of pages in cached obj */
        u_int v_pageout_free_min;   /* min number pages reserved for kernel */
        u_int v_interrupt_free_min; /* reserved number of pages for int code */
        u_int v_free_severe;    /* severe depletion of pages below this pt */
        /*
         * Fork/vfork/rfork activity.
         */
        u_int v_forks;          /* number of fork() calls */
        u_int v_vforks;         /* number of vfork() calls */
        u_int v_rforks;         /* number of rfork() calls */
        u_int v_kthreads;       /* number of fork() calls by kernel */
        u_int v_forkpages;      /* number of VM pages affected by fork() */
        u_int v_vforkpages;     /* number of VM pages affected by vfork() */
        u_int v_rforkpages;     /* number of VM pages affected by rfork() */
        u_int v_kthreadpages;   /* number of VM pages affected by fork() by kernel */
};

```

### trap()



### syscall()
