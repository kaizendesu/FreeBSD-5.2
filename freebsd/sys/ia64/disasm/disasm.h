/*
 * Copyright (c) 2000-2003 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _DISASM_H_
#define	_DISASM_H_

#ifndef _DISASM_INT_H_
#define	ASM_ADDITIONAL_OPCODES		ASM_OP_NUMBER_OF_OPCODES
#endif

/* Application registers. */
#define	AR_K0		0
#define	AR_K1		1
#define	AR_K2		2
#define	AR_K3		3
#define	AR_K4		4
#define	AR_K5		5
#define	AR_K6		6
#define	AR_K7		7
#define	AR_RSC		16
#define	AR_BSP		17
#define	AR_BSPSTORE	18
#define	AR_RNAT		19
#define	AR_FCR		21
#define	AR_EFLAG	24
#define	AR_CSD		25
#define	AR_SSD		26
#define	AR_CFLG		27
#define	AR_FSR		28
#define	AR_FIR		29
#define	AR_FDR		30
#define	AR_CCV		32
#define	AR_UNAT		36
#define	AR_FPSR		40
#define	AR_ITC		44
#define	AR_PFS		64
#define	AR_LC		65
#define	AR_EC		66

/* Control registers. */
#define	CR_DCR		0
#define	CR_ITM		1
#define	CR_IVA		2
#define	CR_PTA		8
#define	CR_IPSR		16
#define	CR_ISR		17
#define	CR_IIP		19
#define	CR_IFA		20
#define	CR_ITIR		21
#define	CR_IIPA		22
#define	CR_IFS		23
#define	CR_IIM		24
#define	CR_IHA		25
#define	CR_LID		64
#define	CR_IVR		65
#define	CR_TPR		66
#define	CR_EOI		67
#define	CR_IRR0		68
#define	CR_IRR1		69
#define	CR_IRR2		70
#define	CR_IRR3		71
#define	CR_ITV		72
#define	CR_PMV		73
#define	CR_CMCV		74
#define	CR_LRR0		80
#define	CR_LRR1		81

enum asm_cmpltr_class {
	ASM_CC_NONE,
	ASM_CC_ACLR,
	ASM_CC_BSW, ASM_CC_BTYPE, ASM_CC_BWH,
	ASM_CC_CHK, ASM_CC_CLRRRB, ASM_CC_CREL, ASM_CC_CTYPE,
	ASM_CC_DEP, ASM_CC_DH,
	ASM_CC_FC, ASM_CC_FCREL, ASM_CC_FCTYPE, ASM_CC_FCVT, ASM_CC_FLDTYPE,
	ASM_CC_FMERGE, ASM_CC_FREL, ASM_CC_FSWAP,
	ASM_CC_GETF,
	ASM_CC_IH, ASM_CC_INVALA, ASM_CC_IPWH, ASM_CC_ITC, ASM_CC_ITR,
	ASM_CC_LDHINT, ASM_CC_LDTYPE, ASM_CC_LFETCH, ASM_CC_LFHINT,
	ASM_CC_LFTYPE, ASM_CC_LR,
	ASM_CC_MF, ASM_CC_MOV, ASM_CC_MWH,
	ASM_CC_PAVG, ASM_CC_PC, ASM_CC_PH, ASM_CC_PREL, ASM_CC_PRTYPE,
	ASM_CC_PTC, ASM_CC_PTR, ASM_CC_PVEC,
	ASM_CC_SAT, ASM_CC_SEM, ASM_CC_SETF, ASM_CC_SF, ASM_CC_SRLZ,
	ASM_CC_STHINT, ASM_CC_STTYPE, ASM_CC_SYNC,
	ASM_CC_RW,
	ASM_CC_TREL, ASM_CC_TRUNC,
	ASM_CC_UNIT, ASM_CC_UNPACK, ASM_CC_UNS,
	ASM_CC_XMA
};

enum asm_cmpltr_type {
	ASM_CT_NONE,
	ASM_CT_COND = ASM_CT_NONE,

	ASM_CT_0, ASM_CT_1,
	ASM_CT_A, ASM_CT_ACQ, ASM_CT_AND,
	ASM_CT_B, ASM_CT_BIAS,
	ASM_CT_C_CLR, ASM_CT_C_CLR_ACQ, ASM_CT_C_NC, ASM_CT_CALL,
	ASM_CT_CEXIT, ASM_CT_CLOOP, ASM_CT_CLR, ASM_CT_CTOP,
	ASM_CT_D, ASM_CT_DC_DC, ASM_CT_DC_NT, ASM_CT_DPNT, ASM_CT_DPTK,
	ASM_CT_E, ASM_CT_EQ, ASM_CT_EXCL, ASM_CT_EXIT, ASM_CT_EXP,
	ASM_CT_F, ASM_CT_FAULT, ASM_CT_FEW, ASM_CT_FILL, ASM_CT_FX, ASM_CT_FXU,
	ASM_CT_G, ASM_CT_GA, ASM_CT_GE, ASM_CT_GT,
	ASM_CT_H, ASM_CT_HU,
	ASM_CT_I, ASM_CT_IA, ASM_CT_IMP,
	ASM_CT_L, ASM_CT_LE, ASM_CT_LOOP, ASM_CT_LR, ASM_CT_LT, ASM_CT_LTU,
	ASM_CT_M, ASM_CT_MANY,
	ASM_CT_NC, ASM_CT_NE, ASM_CT_NEQ, ASM_CT_NL, ASM_CT_NLE, ASM_CT_NLT,
	ASM_CT_NM, ASM_CT_NR, ASM_CT_NS, ASM_CT_NT_DC, ASM_CT_NT_NT,
	ASM_CT_NT_TK, ASM_CT_NT1, ASM_CT_NT2, ASM_CT_NTA, ASM_CT_NZ,
	ASM_CT_OR, ASM_CT_OR_ANDCM, ASM_CT_ORD,
	ASM_CT_PR,
	ASM_CT_R, ASM_CT_RAZ, ASM_CT_REL, ASM_CT_RET, ASM_CT_RW,
	ASM_CT_S, ASM_CT_S0, ASM_CT_S1, ASM_CT_S2, ASM_CT_S3, ASM_CT_SA,
	ASM_CT_SE, ASM_CT_SIG, ASM_CT_SPILL, ASM_CT_SPNT, ASM_CT_SPTK,
	ASM_CT_SSS,
	ASM_CT_TK_DC, ASM_CT_TK_NT, ASM_CT_TK_TK, ASM_CT_TRUNC,
	ASM_CT_U, ASM_CT_UNC, ASM_CT_UNORD, ASM_CT_USS, ASM_CT_UUS, ASM_CT_UUU,
	ASM_CT_W, ASM_CT_WEXIT, ASM_CT_WTOP,
	ASM_CT_X, ASM_CT_XF,
	ASM_CT_Z,
};

/* Completer. */
struct asm_cmpltr {
	enum asm_cmpltr_class	c_class;
	enum asm_cmpltr_type	c_type;
};

/* Operand types. */
enum asm_oper_type {
	ASM_OPER_NONE,
	ASM_OPER_AREG,		/* = ar# */
	ASM_OPER_BREG,		/* = b# */
	ASM_OPER_CPUID,		/* = cpuid[r#] */
	ASM_OPER_CREG,		/* = cr# */
	ASM_OPER_DBR,		/* = dbr[r#] */
	ASM_OPER_DISP,		/* IP relative displacement. */
	ASM_OPER_DTR,		/* = dtr[r#] */
	ASM_OPER_FREG,		/* = f# */
	ASM_OPER_GREG,		/* = r# */
	ASM_OPER_IBR,		/* = ibr[r#] */
	ASM_OPER_IMM,		/* Immediate */
	ASM_OPER_IP,		/* = ip */
	ASM_OPER_ITR,		/* = itr[r#] */
	ASM_OPER_MEM,		/* = [r#] */
	ASM_OPER_MSR,		/* = msr[r#] */
	ASM_OPER_PKR,		/* = pkr[r#] */
	ASM_OPER_PMC,		/* = pmc[r#] */
	ASM_OPER_PMD,		/* = pmd[r#] */
	ASM_OPER_PR,		/* = pr */
	ASM_OPER_PR_ROT,	/* = pr.rot */
	ASM_OPER_PREG,		/* = p# */
	ASM_OPER_PSR,		/* = psr */
	ASM_OPER_PSR_L,		/* = psr.l */
	ASM_OPER_PSR_UM,	/* = psr.um */
	ASM_OPER_RR		/* = rr[r#] */
};

/* Operand */
struct asm_oper {
	enum asm_oper_type	o_type;
	uint64_t		o_value;
};

/* Instruction formats. */
enum asm_fmt {
	ASM_FMT_NONE,
	ASM_FMT_A = 0x0100,
	ASM_FMT_A1,  ASM_FMT_A2,  ASM_FMT_A3,  ASM_FMT_A4,
	ASM_FMT_A5,  ASM_FMT_A6,  ASM_FMT_A7,  ASM_FMT_A8,
	ASM_FMT_A9,  ASM_FMT_A10,
	ASM_FMT_B = 0x0200,
	ASM_FMT_B1,  ASM_FMT_B2,  ASM_FMT_B3,  ASM_FMT_B4,
	ASM_FMT_B5,  ASM_FMT_B6,  ASM_FMT_B7,  ASM_FMT_B8,
	ASM_FMT_B9,
	ASM_FMT_F = 0x0300,
	ASM_FMT_F1,  ASM_FMT_F2,  ASM_FMT_F3,  ASM_FMT_F4,
	ASM_FMT_F5,  ASM_FMT_F6,  ASM_FMT_F7,  ASM_FMT_F8,
	ASM_FMT_F9,  ASM_FMT_F10, ASM_FMT_F11, ASM_FMT_F12,
	ASM_FMT_F13, ASM_FMT_F14, ASM_FMT_F15,
	ASM_FMT_I = 0x0400,
	ASM_FMT_I1,  ASM_FMT_I2,  ASM_FMT_I3,  ASM_FMT_I4,
	ASM_FMT_I5,  ASM_FMT_I6,  ASM_FMT_I7,  ASM_FMT_I8,
	ASM_FMT_I9,  ASM_FMT_I10, ASM_FMT_I11, ASM_FMT_I12,
	ASM_FMT_I13, ASM_FMT_I14, ASM_FMT_I15, ASM_FMT_I16,
	ASM_FMT_I17, ASM_FMT_I19, ASM_FMT_I20, ASM_FMT_I21,
	ASM_FMT_I22, ASM_FMT_I23, ASM_FMT_I24, ASM_FMT_I25,
	ASM_FMT_I26, ASM_FMT_I27, ASM_FMT_I28, ASM_FMT_I29,
	ASM_FMT_M = 0x0500,
	ASM_FMT_M1,  ASM_FMT_M2,  ASM_FMT_M3,  ASM_FMT_M4,
	ASM_FMT_M5,  ASM_FMT_M6,  ASM_FMT_M7,  ASM_FMT_M8,
	ASM_FMT_M9,  ASM_FMT_M10, ASM_FMT_M11, ASM_FMT_M12,
	ASM_FMT_M13, ASM_FMT_M14, ASM_FMT_M15, ASM_FMT_M16,
	ASM_FMT_M17, ASM_FMT_M18, ASM_FMT_M19, ASM_FMT_M20,
	ASM_FMT_M21, ASM_FMT_M22, ASM_FMT_M23, ASM_FMT_M24,
	ASM_FMT_M25, ASM_FMT_M26, ASM_FMT_M27, ASM_FMT_M28,
	ASM_FMT_M29, ASM_FMT_M30, ASM_FMT_M31, ASM_FMT_M32,
	ASM_FMT_M33, ASM_FMT_M34, ASM_FMT_M35, ASM_FMT_M36,
	ASM_FMT_M37, ASM_FMT_M38, ASM_FMT_M39, ASM_FMT_M40,
	ASM_FMT_M41, ASM_FMT_M42, ASM_FMT_M43, ASM_FMT_M44,
	ASM_FMT_M45, ASM_FMT_M46,
	ASM_FMT_X = 0x0600,
	ASM_FMT_X1,  ASM_FMT_X2,  ASM_FMT_X3,  ASM_FMT_X4
};

/* Instruction opcodes. */
enum asm_op {
	ASM_OP_NONE,
	ASM_OP_ADD, ASM_OP_ADDL, ASM_OP_ADDP4, ASM_OP_ADDS, ASM_OP_ALLOC,
	ASM_OP_AND, ASM_OP_ANDCM,
	ASM_OP_BR, ASM_OP_BREAK, ASM_OP_BRL, ASM_OP_BRP, ASM_OP_BSW,
	ASM_OP_CHK, ASM_OP_CLRRRB, ASM_OP_CMP, ASM_OP_CMP4, ASM_OP_CMP8XCHG16,
	ASM_OP_CMPXCHG1, ASM_OP_CMPXCHG2, ASM_OP_CMPXCHG4, ASM_OP_CMPXCHG8,
	ASM_OP_COVER, ASM_OP_CZX1, ASM_OP_CZX2,
	ASM_OP_DEP,
	ASM_OP_EPC, ASM_OP_EXTR,
	ASM_OP_FAMAX, ASM_OP_FAMIN, ASM_OP_FAND, ASM_OP_FANDCM, ASM_OP_FC,
	ASM_OP_FCHKF, ASM_OP_FCLASS, ASM_OP_FCLRF, ASM_OP_FCMP, ASM_OP_FCVT,
	ASM_OP_FETCHADD4, ASM_OP_FETCHADD8, ASM_OP_FLUSHRS, ASM_OP_FMA,
	ASM_OP_FMAX, ASM_OP_FMERGE, ASM_OP_FMIN, ASM_OP_FMIX, ASM_OP_FMS,
	ASM_OP_FNMA, ASM_OP_FOR, ASM_OP_FPACK, ASM_OP_FPAMAX, ASM_OP_FPAMIN,
	ASM_OP_FPCMP, ASM_OP_FPCVT, ASM_OP_FPMA, ASM_OP_FPMAX, ASM_OP_FPMERGE,
	ASM_OP_FPMIN, ASM_OP_FPMS, ASM_OP_FPNMA, ASM_OP_FPRCPA,
	ASM_OP_FPRSQRTA, ASM_OP_FRCPA, ASM_OP_FRSQRTA, ASM_OP_FSELECT,
	ASM_OP_FSETC, ASM_OP_FSWAP, ASM_OP_FSXT, ASM_OP_FWB, ASM_OP_FXOR,
	ASM_OP_GETF,
	ASM_OP_INVALA, ASM_OP_ITC, ASM_OP_ITR,
	ASM_OP_LD1, ASM_OP_LD16, ASM_OP_LD2, ASM_OP_LD4, ASM_OP_LD8,
	ASM_OP_LDF, ASM_OP_LDF8, ASM_OP_LDFD, ASM_OP_LDFE, ASM_OP_LDFP8,
	ASM_OP_LDFPD, ASM_OP_LDFPS, ASM_OP_LDFS, ASM_OP_LFETCH, ASM_OP_LOADRS,
	ASM_OP_MF, ASM_OP_MIX1, ASM_OP_MIX2, ASM_OP_MIX4, ASM_OP_MOV,
	ASM_OP_MOVL, ASM_OP_MUX1, ASM_OP_MUX2,
	ASM_OP_NOP,
	ASM_OP_OR,
	ASM_OP_PACK2, ASM_OP_PACK4, ASM_OP_PADD1, ASM_OP_PADD2, ASM_OP_PADD4,
	ASM_OP_PAVG1, ASM_OP_PAVG2, ASM_OP_PAVGSUB1, ASM_OP_PAVGSUB2,
	ASM_OP_PCMP1, ASM_OP_PCMP2, ASM_OP_PCMP4, ASM_OP_PMAX1, ASM_OP_PMAX2,
	ASM_OP_PMIN1, ASM_OP_PMIN2, ASM_OP_PMPY2, ASM_OP_PMPYSHR2,
	ASM_OP_POPCNT, ASM_OP_PROBE, ASM_OP_PSAD1, ASM_OP_PSHL2, ASM_OP_PSHL4,
	ASM_OP_PSHLADD2, ASM_OP_PSHR2, ASM_OP_PSHR4, ASM_OP_PSHRADD2,
	ASM_OP_PSUB1, ASM_OP_PSUB2, ASM_OP_PSUB4, ASM_OP_PTC, ASM_OP_PTR,
	ASM_OP_RFI, ASM_OP_RSM, ASM_OP_RUM,
	ASM_OP_SETF, ASM_OP_SHL, ASM_OP_SHLADD, ASM_OP_SHLADDP4, ASM_OP_SHR,
	ASM_OP_SHRP, ASM_OP_SRLZ, ASM_OP_SSM, ASM_OP_ST1, ASM_OP_ST16,
	ASM_OP_ST2, ASM_OP_ST4, ASM_OP_ST8, ASM_OP_STF, ASM_OP_STF8,
	ASM_OP_STFD, ASM_OP_STFE, ASM_OP_STFS, ASM_OP_SUB, ASM_OP_SUM,
	ASM_OP_SXT1, ASM_OP_SXT2, ASM_OP_SXT4, ASM_OP_SYNC,
	ASM_OP_TAK, ASM_OP_TBIT, ASM_OP_THASH, ASM_OP_TNAT, ASM_OP_TPA,
	ASM_OP_TTAG,
	ASM_OP_UNPACK1, ASM_OP_UNPACK2, ASM_OP_UNPACK4,
	ASM_OP_XCHG1, ASM_OP_XCHG2, ASM_OP_XCHG4, ASM_OP_XCHG8, ASM_OP_XMA,
	ASM_OP_XOR,
	ASM_OP_ZXT1, ASM_OP_ZXT2, ASM_OP_ZXT4,
	/* Additional opcodes used only internally. */
	ASM_ADDITIONAL_OPCODES
};

/* Instruction. */
struct asm_inst {
	uint64_t		i_bits;
	struct asm_oper		i_oper[7];
	struct asm_cmpltr	i_cmpltr[5];
	enum asm_fmt		i_format;
	enum asm_op		i_op;
	int			i_ncmpltrs;
	int			i_srcidx;
};

struct asm_bundle {
	const char		*b_templ;
	struct asm_inst		b_inst[3];
};

/* Functional units. */
enum asm_unit {
	ASM_UNIT_NONE,
	ASM_UNIT_A = 0x0100,	/* A unit. */
	ASM_UNIT_B = 0x0200,	/* B unit. */
	ASM_UNIT_F = 0x0300,	/* F unit. */
	ASM_UNIT_I = 0x0400,	/* I unit. */
	ASM_UNIT_M = 0x0500,	/* M unit. */
	ASM_UNIT_X = 0x0600	/* X unit. */
};

#ifdef _DISASM_INT_H_
int asm_extract(enum asm_op, enum asm_fmt, uint64_t, struct asm_bundle *, int);
#endif

int asm_decode(uint64_t, struct asm_bundle *);

void asm_completer(const struct asm_cmpltr *, char *);
void asm_mnemonic(const enum asm_op, char *);
void asm_operand(const struct asm_oper *, char *, uint64_t);
void asm_print_bundle(const struct asm_bundle *, uint64_t);
void asm_print_inst(const struct asm_bundle *, int, uint64_t);

#endif /* _DISASM_H_ */