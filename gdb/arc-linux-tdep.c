/* Target dependent code for ARC processor family, for GDB, the GNU debugger.

   Copyright 2005 Free Software Foundation, Inc.
   Copyright 2009-2013 Synopsys Inc.

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com> on behalf of
   Synopsys Inc.
   Contributed by Codito Technologies Pvt. Ltd. (www.codito.com) on behalf of
   Synopsys Inc.

   Authors: 
      Jeremy Bennett       <jeremy.bennett@embecosm.com>
      Soam Vasani          <soam.vasani@codito.com>
      Ramana Radhakrishnan <ramana.radhakrishnan@codito.com> 
      Richard Stuckey      <richard.stuckey@arc.com>

   This file is part of GDB.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/******************************************************************************/
/*                                                                            */
/* Outline:                                                                   */
/*     This module provides support for the ARC processor family's target     */
/*     dependencies which are specific to the arc-linux-uclibc configuration  */
/*     of the ARC gdb.                                                        */
/*                                                                            */
/******************************************************************************/

/* gdb header files */
#include "defs.h"
#include "gdbarch.h"
#include "frame.h"
#include "osabi.h"
#include "regcache.h"
#include "inferior.h"
#include "reggroups.h"
#include "solib-svr4.h"
#include "block.h"
#include "regset.h"
#include "dis-asm.h"
#include "opcode/arc.h"
#include "opcodes/arc-dis-old.h"
#include "exceptions.h"

/* ARC header files */
#include "arc-tdep.h"


/* -------------------------------------------------------------------------- */
/*                               local data                                   */
/* -------------------------------------------------------------------------- */






/*! Mapping between the general-purpose registers in `struct sigcontext' format
    and GDB's register cache layout.

    arc_linux_sc_reg_offset[i] is the sigcontext offset of GDB regnum `i'.
	
	At the moment of this writing sigcontext is the same as user_regs_struct
	used for core dumps. The only difference is that sigcontext uses only a
	subset of registers from user_regs_struct, namely "scratch" part.*/

/* From <include/uapi/asm/sigcontext.h> and <include/uapi/asm/ptrace.h>. */
static const int arc_linux_sc_reg_offset[ARC_NUM_RAW_REGS] = {
  22 * BYTES_IN_REGISTER,	/* r0       */
  21 * BYTES_IN_REGISTER,	/* r1       */
  20 * BYTES_IN_REGISTER,	/* r2       */
  19 * BYTES_IN_REGISTER,	/* r3       */
  18 * BYTES_IN_REGISTER,	/* r4       */
  17 * BYTES_IN_REGISTER,	/* r5       */
  16 * BYTES_IN_REGISTER,	/* r6       */
  15 * BYTES_IN_REGISTER,	/* r7       */
  14 * BYTES_IN_REGISTER,	/* r8       */
  13 * BYTES_IN_REGISTER,	/* r9       */
  12 * BYTES_IN_REGISTER,	/* r10      */
  11 * BYTES_IN_REGISTER,	/* r11      */
  10 * BYTES_IN_REGISTER,	/* r12      */
  REGISTER_NOT_PRESENT,  	/* r13      */
  REGISTER_NOT_PRESENT,  	/* r14      */
  REGISTER_NOT_PRESENT,  	/* r15      */
  REGISTER_NOT_PRESENT,  	/* r16      */
  REGISTER_NOT_PRESENT,  	/* r17      */
  REGISTER_NOT_PRESENT,  	/* r18      */
  REGISTER_NOT_PRESENT,  	/* r19      */
  REGISTER_NOT_PRESENT,  	/* r20      */
  REGISTER_NOT_PRESENT,  	/* r21      */
  REGISTER_NOT_PRESENT,  	/* r22      */
  REGISTER_NOT_PRESENT,  	/* r23      */
  REGISTER_NOT_PRESENT,  	/* r24      */
  REGISTER_NOT_PRESENT,  	/* r25      */
  9 * BYTES_IN_REGISTER,	/* r26 (gp) */
  8 * BYTES_IN_REGISTER,	/* fp       */
  23 * BYTES_IN_REGISTER,	/* sp       */
  REGISTER_NOT_PRESENT,  	/* ilink1   */
  REGISTER_NOT_PRESENT,  	/* ilink2   */
  7 * BYTES_IN_REGISTER,	/* blink    */

  REGISTER_NOT_PRESENT,  	/* r32      */
  REGISTER_NOT_PRESENT,  	/* r33      */
  REGISTER_NOT_PRESENT,  	/* r34      */
  REGISTER_NOT_PRESENT,  	/* r35      */
  REGISTER_NOT_PRESENT,  	/* r36      */
  REGISTER_NOT_PRESENT,  	/* r37      */
  REGISTER_NOT_PRESENT,  	/* r38      */
  REGISTER_NOT_PRESENT,  	/* r39      */
  REGISTER_NOT_PRESENT,  	/* r40      */
  REGISTER_NOT_PRESENT,  	/* r41      */
  REGISTER_NOT_PRESENT,  	/* r42      */
  REGISTER_NOT_PRESENT,  	/* r43      */
  REGISTER_NOT_PRESENT,  	/* r44      */
  REGISTER_NOT_PRESENT,  	/* r45      */
  REGISTER_NOT_PRESENT,  	/* r46      */
  REGISTER_NOT_PRESENT,  	/* r47      */
  REGISTER_NOT_PRESENT,  	/* r48      */
  REGISTER_NOT_PRESENT,  	/* r49      */
  REGISTER_NOT_PRESENT,  	/* r50      */
  REGISTER_NOT_PRESENT,  	/* r51      */
  REGISTER_NOT_PRESENT,  	/* r52      */
  REGISTER_NOT_PRESENT,  	/* r53      */
  REGISTER_NOT_PRESENT,  	/* r54      */
  REGISTER_NOT_PRESENT,  	/* r55      */
  REGISTER_NOT_PRESENT,  	/* r56      */
  REGISTER_NOT_PRESENT,  	/* r57      */
  REGISTER_NOT_PRESENT,  	/* r58      */
  REGISTER_NOT_PRESENT,  	/* r59      */
  4 * BYTES_IN_REGISTER,	/* lp_count */
  REGISTER_NOT_PRESENT,  	/* reserved */
  REGISTER_NOT_PRESENT,  	/* limm     */
  REGISTER_NOT_PRESENT,  	/* pcl      */

  REGISTER_NOT_PRESENT,  	/* stop_pc  */
  2 * BYTES_IN_REGISTER,	/* lp_start */
  3 * BYTES_IN_REGISTER,	/* lp_end   */
  5 * BYTES_IN_REGISTER,	/* status32 */
  REGISTER_NOT_PRESENT,  	/* st32_l1  */
  REGISTER_NOT_PRESENT,  	/* st32_l2  */
  REGISTER_NOT_PRESENT,  	/* irq_lv12 */
  REGISTER_NOT_PRESENT,  	/* irq_lev  */
  REGISTER_NOT_PRESENT,  	/* irq_hint */
  6 * BYTES_IN_REGISTER,	/* ret      */
  REGISTER_NOT_PRESENT,  	/* erbta    */
  REGISTER_NOT_PRESENT,  	/* erstatus */
  REGISTER_NOT_PRESENT,  	/* ecr      */
  REGISTER_NOT_PRESENT,  	/* efa      */
  REGISTER_NOT_PRESENT,  	/* icause1  */
  REGISTER_NOT_PRESENT,  	/* icause2  */
  REGISTER_NOT_PRESENT,  	/* ienable  */
  REGISTER_NOT_PRESENT,  	/* itrigger */
  1 * BYTES_IN_REGISTER,	/* bta      */
  REGISTER_NOT_PRESENT,  	/* bta_l1   */
  REGISTER_NOT_PRESENT,  	/* bta_l2   */
  REGISTER_NOT_PRESENT,  	/* irq pulse*/
  REGISTER_NOT_PRESENT,  	/* irq pend */
};


/* arc_linux_core_reg_offsets[i] is the offset in the .reg section of GDB
 * regnum i.
 *
 * From include/uapi/asm/ptrace.h in the ARC Linux sources.
 */

static const int arc_linux_core_reg_offsets[ARC_NUM_RAW_REGS] = {
  22 * BYTES_IN_REGISTER,	/* r0       */
  21 * BYTES_IN_REGISTER,	/* r1       */
  20 * BYTES_IN_REGISTER,	/* r2       */
  19 * BYTES_IN_REGISTER,	/* r3       */
  18 * BYTES_IN_REGISTER,	/* r4       */
  17 * BYTES_IN_REGISTER,	/* r5       */
  16 * BYTES_IN_REGISTER,	/* r6       */
  15 * BYTES_IN_REGISTER,	/* r7       */
  14 * BYTES_IN_REGISTER,	/* r8       */
  13 * BYTES_IN_REGISTER,	/* r9       */
  12 * BYTES_IN_REGISTER,	/* r10      */
  11 * BYTES_IN_REGISTER,	/* r11      */
  10 * BYTES_IN_REGISTER,	/* r12      */
  37 * BYTES_IN_REGISTER,	/* r13      */
  36 * BYTES_IN_REGISTER,	/* r14      */
  35 * BYTES_IN_REGISTER,	/* r15      */
  34 * BYTES_IN_REGISTER,	/* r16      */
  33 * BYTES_IN_REGISTER,	/* r17      */
  32 * BYTES_IN_REGISTER,	/* r18      */
  31 * BYTES_IN_REGISTER,	/* r19      */
  30 * BYTES_IN_REGISTER,	/* r20      */
  29 * BYTES_IN_REGISTER,	/* r21      */
  28 * BYTES_IN_REGISTER,	/* r22      */
  27 * BYTES_IN_REGISTER,	/* r23      */
  26 * BYTES_IN_REGISTER,	/* r24      */
  25 * BYTES_IN_REGISTER,	/* r25      */
  9 * BYTES_IN_REGISTER,	/* r26 (gp) */
  8 * BYTES_IN_REGISTER,	/* fp       */
  23 * BYTES_IN_REGISTER,	/* sp       */
  REGISTER_NOT_PRESENT,  	/* ilink1   */
  REGISTER_NOT_PRESENT,  	/* ilink2   */
  7 * BYTES_IN_REGISTER,	/* blink    */

  REGISTER_NOT_PRESENT,  	/* r32      */
  REGISTER_NOT_PRESENT,  	/* r33      */
  REGISTER_NOT_PRESENT,  	/* r34      */
  REGISTER_NOT_PRESENT,  	/* r35      */
  REGISTER_NOT_PRESENT,  	/* r36      */
  REGISTER_NOT_PRESENT,  	/* r37      */
  REGISTER_NOT_PRESENT,  	/* r38      */
  REGISTER_NOT_PRESENT,  	/* r39      */
  REGISTER_NOT_PRESENT,  	/* r40      */
  REGISTER_NOT_PRESENT,  	/* r41      */
  REGISTER_NOT_PRESENT,  	/* r42      */
  REGISTER_NOT_PRESENT,  	/* r43      */
  REGISTER_NOT_PRESENT,  	/* r44      */
  REGISTER_NOT_PRESENT,  	/* r45      */
  REGISTER_NOT_PRESENT,  	/* r46      */
  REGISTER_NOT_PRESENT,  	/* r47      */
  REGISTER_NOT_PRESENT,  	/* r48      */
  REGISTER_NOT_PRESENT,  	/* r49      */
  REGISTER_NOT_PRESENT,  	/* r50      */
  REGISTER_NOT_PRESENT,  	/* r51      */
  REGISTER_NOT_PRESENT,  	/* r52      */
  REGISTER_NOT_PRESENT,  	/* r53      */
  REGISTER_NOT_PRESENT,  	/* r54      */
  REGISTER_NOT_PRESENT,  	/* r55      */
  REGISTER_NOT_PRESENT,  	/* r56      */
  REGISTER_NOT_PRESENT,  	/* r57      */
  REGISTER_NOT_PRESENT,  	/* r58      */
  REGISTER_NOT_PRESENT,  	/* r59      */
  4 * BYTES_IN_REGISTER,	/* lp_count */
  REGISTER_NOT_PRESENT,  	/* reserved */
  REGISTER_NOT_PRESENT,  	/* limm     */
  REGISTER_NOT_PRESENT,  	/* pcl      */

  39 * BYTES_IN_REGISTER,	/* stop_pc  */
  2 * BYTES_IN_REGISTER,	/* lp_start */
  3 * BYTES_IN_REGISTER,	/* lp_end   */
  5 * BYTES_IN_REGISTER,	/* status32 */
  REGISTER_NOT_PRESENT,  	/* st32_l1  */
  REGISTER_NOT_PRESENT,  	/* st32_l2  */
  REGISTER_NOT_PRESENT,  	/* irq_lv12 */
  REGISTER_NOT_PRESENT,  	/* irq_lev  */
  REGISTER_NOT_PRESENT,  	/* irq_hint */
  6 * BYTES_IN_REGISTER,	/* ret      */
  REGISTER_NOT_PRESENT,  	/* erbta    */
  REGISTER_NOT_PRESENT,  	/* erstatus */
  REGISTER_NOT_PRESENT,  	/* ecr      */
  38 * BYTES_IN_REGISTER,	/* efa      */
  REGISTER_NOT_PRESENT,  	/* icause1  */
  REGISTER_NOT_PRESENT,  	/* icause2  */
  REGISTER_NOT_PRESENT,  	/* ienable  */
  REGISTER_NOT_PRESENT,  	/* itrigger */
  1 * BYTES_IN_REGISTER,	/* bta      */
  REGISTER_NOT_PRESENT,  	/* bta_l1   */
  REGISTER_NOT_PRESENT,  	/* bta_l2   */
  REGISTER_NOT_PRESENT,  	/* irq pulse*/
  REGISTER_NOT_PRESENT,  	/* irq pend */

  /* 24 * BYTES_IN_REGISTER,	orig_r8  */
};


/* -------------------------------------------------------------------------- */
/*                               local functions                              */
/* -------------------------------------------------------------------------- */

/*! Returns TRUE if the instruction at PC is a branch (of any kind).

    We always set a breakpoint at the next instruction, even if we have an
    unconditional branch, which means it won't be executed.

    @todo This doesn't work if we are looking at a delay slot. We need to do
          something about that.

    @param[out] fall_thru  Set to the address of the next insn.
    @param[out] target     Set to the branch target. */
static int
arc_linux_next_pc (CORE_ADDR pc, CORE_ADDR *fall_thru, CORE_ADDR *target)
{
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct disassemble_info di;
  struct arcDisState instr;
  int two_targets = FALSE;
  ULONGEST  lp_end, lp_start, lp_count, status32;

  /* so what is the instruction at the given pc? */
  arc_initialize_disassembler (gdbarch, &di);
  instr = arcAnalyzeInstr (pc, &di);

  /* by default, the next instruction is the one immediately after the one at
     pc */
  *fall_thru = pc + instr.instructionLen;

  if (arc_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "--- arc_linux_next_pc (%s) = %s, isBranch = %s, "
			  "tcnt = %d [0x%x],\n"
			  "    flow = %s (%d), reg for indirect jump = %d, "
			  "nullifyMode = %s\n",
			  print_core_address (gdbarch, pc),
			  print_core_address (gdbarch, *fall_thru),
			  instr.isBranch ? "true" : "false", instr.tcnt,
			  instr.targets[0],
			  (instr.flow == direct_jump
			   || instr.flow == direct_call)
			  ? "direct" : "indirect", instr.flow,
			  instr.register_for_indirect_jump,
			  ((instr.nullifyMode == (char) BR_exec_always)
			   ? "delay slot" : "no delay"));
    }

  /* OK, it's a branch */
  if (instr.isBranch)
    {
      two_targets = TRUE;

      if (instr.flow == direct_jump || instr.flow == direct_call)
	{
	  /* It's a direct jump or call, so the destination address is encoded
         in the instruction, so we got it by disassembling the instruction */
	  *target = (CORE_ADDR) instr.targets[0];
	}
      else
	{
	  /* It's an indirect jump to the address held in the register
	     named in the instruction, so we must read that register */
	  ULONGEST val;
	  regcache_cooked_read_unsigned (regcache,
					 instr.register_for_indirect_jump,
					 &val);
	  *target = val;
	}

      /* for instructions with delay slots, the fall thru is not the
         instruction immediately after the branch instruction, but the one
         after that. */
      if (instr.nullifyMode == (char) BR_exec_always)
	{
	  struct arcDisState instr_d = arcAnalyzeInstr (*fall_thru, &di);

	  *fall_thru += instr_d.instructionLen;
	}
    }


  /* Zero-overhead loops */
  regcache_cooked_read_unsigned (regcache, ARC_AUX_LP_START_REGNUM, &lp_start);
  regcache_cooked_read_unsigned (regcache, ARC_AUX_LP_END_REGNUM, &lp_end);
  regcache_cooked_read_unsigned (regcache, ARC_LP_COUNT_REGNUM, &lp_count);
  regcache_cooked_read_unsigned (regcache, ARC_AUX_STATUS32_REGNUM, &status32);

  if (!(status32 & 0x00000100) && *fall_thru == lp_end && lp_count > 1)
    {
      /* the instruction is in effect a jump back to the start of the loop */
      two_targets = TRUE;
      *target = lp_start;
    }

  return two_targets;

}	/* arc_linux_next_pc () */


/*! Is THIS frame a sigtramp function.

    Does THIS frame correspond to a GNU/Linux sigtramp function? This is the
    case if the PC is either at the start of, or in the middle of the two
    instructions:

    @verbatim
    mov r8,nr_sigreturn
    swi
    @endverbatim

    @note This function has changed from GDB 6.8. It now takes a reference to
          THIS frame, not the NEXT frame.

    @param[in] this_frame  The frame to consider.
    @return                Non-zero (TRUE) if this is a sigtramp frame, zero
                           (FALSE) otherwise. */
static int
arc_linux_is_sigtramp (struct frame_info *this_frame)
{
  static const gdb_byte arc_sigtramp_insns_le[] =
    { 0x8a, 0x20, 0xc2, 0x12,		/* mov  r8,nr_rt_sigreturn */
      0x6f, 0x22, 0x3f, 0x00		/* swi */
    };
  static const gdb_byte arc_sigtramp_insns_be[] =
    { 0x12, 0xc2, 0x20, 0x8a,		/* mov  r8,nr_rt_sigreturn */
      0x00, 0x3f, 0x22, 0x6f		/* swi */
    };

  static const int INSNS_SIZE = sizeof (arc_sigtramp_insns_le);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *arc_sigtramp_insns = byte_order == BFD_ENDIAN_LITTLE
    ? arc_sigtramp_insns_le : arc_sigtramp_insns_be;
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[sizeof (arc_sigtramp_insns_le)];

  /* Read the memory at the PC. Since we are stopped any breakpoints will
     have been removed (despite the name, this really does take THIS frame). */
  if (!safe_frame_unwind_memory (this_frame, pc, buf, INSNS_SIZE))
    {
      /* failed to unwind frame */
      return FALSE;
    }

  /* Is that code the sigtramp instruction sequence? */
  if (memcmp (buf, arc_sigtramp_insns, INSNS_SIZE) == 0)
    {
      return TRUE;
    }

  /* No - look one instruction earlier in the code... */
  if (!safe_frame_unwind_memory (this_frame, pc - 4, buf, (int) INSNS_SIZE))
    {
      /* failed to unwind frame */
      return FALSE;
    }
  
  return memcmp (buf, arc_sigtramp_insns, INSNS_SIZE) == 0;

}	/* arc_linux_is_sigtramp () */


/*! Get sigcontext of sigtramp frame.

    Assuming THIS frame is a frame following a GNU/Linux sigtramp routine,
    return the address of the associated sigcontext structure.

    That structure can be found in the stack pointer of the frame.

    @note This function has changed from GDB 6.8. It now takes a reference to
          THIS frame, not the NEXT frame.

    @param[in] this_frame  The frame to consider.
    @return                The address of the sigcontext structure. */
static CORE_ADDR
arc_linux_sigcontext_addr (struct frame_info *this_frame)
{
  return (CORE_ADDR) get_frame_sp (this_frame);

}	/* arc_linux_sigcontext_addr () */


/* -------------------------------------------------------------------------- */
/*		   ARC specific GDB architectural functions		      */
/*									      */
/* Functions are listed in the order they are used in arc_linux_init_abi.     */
/* -------------------------------------------------------------------------- */

/*! Determine whether a register can be read.

    A Linux target can only read registers writable in user space and for now
    only sees a small subset of aux registers and no core extension registers.

    @todo We'll need a more complex interface once the aux registers are
          defined via XML.

    @param[in] gdbarch  The current GDB architecture.
    @param[in] regnum   The register of interest.
    @return             Non-zero (TRUE) if we _cannot_ read the register,
                        false otherwise. */
static int
arc_linux_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  /* Default is to be able to read regs, pick out the others explicitly. */
  switch (regnum)
    {
    case 32: case 33: case 34: case 35:	/* Extension core registers */
    case 36: case 37: case 38: case 39:
    case 40: case 41: case 42: case 43:
    case 44: case 45: case 46: case 47:
    case 48: case 49: case 50: case 51:
    case 52: case 53: case 54: case 55:
    case 56: case 57: case 58: case 59:
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
    case ARC_ILINK1_REGNUM:
    case ARC_ILINK2_REGNUM:
    case ARC_AUX_STATUS32_L1_REGNUM:
    case ARC_AUX_STATUS32_L2_REGNUM:
    case ARC_AUX_AUX_IRQ_LV12_REGNUM:
    case ARC_AUX_AUX_IRQ_LEV_REGNUM:
    case ARC_AUX_AUX_IRQ_HINT_REGNUM:
    case ARC_AUX_ERET_REGNUM:
    case ARC_AUX_ERBTA_REGNUM:
    case ARC_AUX_ERSTATUS_REGNUM:
    case ARC_AUX_ECR_REGNUM:
    case ARC_AUX_ICAUSE1_REGNUM:
    case ARC_AUX_ICAUSE2_REGNUM:
    case ARC_AUX_AUX_IENABLE_REGNUM:
    case ARC_AUX_AUX_ITRIGGER_REGNUM:
    case ARC_AUX_BTA_REGNUM:
    case ARC_AUX_BTA_L1_REGNUM:
    case ARC_AUX_BTA_L2_REGNUM:
    case ARC_AUX_AUX_IRQ_PULSE_CANCEL_REGNUM:
    case ARC_AUX_AUX_IRQ_PENDING_REGNUM:
      return 1;				/* Privileged/debugger read only. */

    default:
      /* Only Aux regs available are LP_START, LP_END, STATUS32 and EFA. The
	 last being odd, because it is a priviledged register, but the kernel
	 gives it to us to read anyway. */
      return 0;				/* Always readable. */
    }
}	/* arc_linux_cannot_fetch_register () */


/*! Determine whether a register can be written.

    A Linux target can only write registers writable in user space and for now
    only sees a small subset of aux registers and no core extension registers.

    @todo We'll need a more complex interface once the aux registers are
          defined via XML.

    @param[in] gdbarch  The current GDB architecture.
    @param[in] regnum   The register of interest.
    @return             Non-zero (TRUE) if we _cannot_ write the register,
                        false otherwise. */
static int
arc_linux_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  /* Default is to be able to write regs, pick out the others explicitly. */
  switch (regnum)
    {
    case ARC_ILINK1_REGNUM:
    case ARC_ILINK2_REGNUM:
    case 32: case 33: case 34: case 35:	/* Extension core registers */
    case 36: case 37: case 38: case 39:
    case 40: case 41: case 42: case 43:
    case 44: case 45: case 46: case 47:
    case 48: case 49: case 50: case 51:
    case 52: case 53: case 54: case 55:
    case 56: case 57: case 58: case 59:
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
    case ARC_PCL_REGNUM:
    case ARC_AUX_STATUS32_REGNUM:
    case ARC_AUX_STATUS32_L1_REGNUM:
    case ARC_AUX_STATUS32_L2_REGNUM:
    case ARC_AUX_AUX_IRQ_LV12_REGNUM:
    case ARC_AUX_AUX_IRQ_LEV_REGNUM:
    case ARC_AUX_AUX_IRQ_HINT_REGNUM:
    case ARC_AUX_ERET_REGNUM:
    case ARC_AUX_ERBTA_REGNUM:
    case ARC_AUX_ERSTATUS_REGNUM:
    case ARC_AUX_ECR_REGNUM:
    case ARC_AUX_EFA_REGNUM:
    case ARC_AUX_ICAUSE1_REGNUM:
    case ARC_AUX_ICAUSE2_REGNUM:
    case ARC_AUX_AUX_IENABLE_REGNUM:
    case ARC_AUX_AUX_ITRIGGER_REGNUM:
    case ARC_AUX_BTA_REGNUM:
    case ARC_AUX_BTA_L1_REGNUM:
    case ARC_AUX_BTA_L2_REGNUM:
    case ARC_AUX_AUX_IRQ_PULSE_CANCEL_REGNUM:
    case ARC_AUX_AUX_IRQ_PENDING_REGNUM:
      return 1;				/* Privileged/debugger write only. */

    default:
      /* Only Aux regs available are LP_START, LP_END, STATUS32. Unlike fetch,
	 we cannot store the privileged EFA register. */
      return 0;				/* Always writable. */
    }
}	/* arc_linux_cannot_store_register () */


/*! Get breakpoint which is approriate for address at which it is to be set.

    For ARC under Linux, breakpoint uses the 16-bit TRAP_S 1 instruction,
    which is 0x3e78 (little endian) or 0x783e (big endian).

    @todo Surely we should be able to use the same breakpoint instruction for
          both Linux and ELF?

    @param[in]     gdbarch  Current GDB architecture
    @param[in,out] pcptr    Pointer to the PC where we want to place a
                            breakpoint
    @param[out]    lenptr   Number of bytes used by the breakpoint.
    @return                 The byte sequence of a breakpoint instruction. */
static const unsigned char *
arc_linux_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR * pcptr,
			      int *lenptr)
{
  static const unsigned char breakpoint_instr_be[] = { 0x78, 0x3e };
  static const unsigned char breakpoint_instr_le[] = { 0x3e, 0x78 };

  *lenptr = sizeof (breakpoint_instr_be);
  return (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    ? breakpoint_instr_be
    : breakpoint_instr_le;

}	/* arc_linux_breakpoint_from_pc () */


/*! Single step in software.

    Insert breakpoints at all the locations where the program could end up
    after the current instruction.

    One breakpoint is always the next instruction to be executed (may not be
    the following instruction if there an unconditional branch). For a
    conditional branch instruction, there will be a second instruction.

    We don't need to worry about removing the breakpoints - GDB will tidy them
    up for us automatically. However if first breakpoint has been inserted but
    the second one fails to insert (for example because of invalid address)
    then we need to cleanup first breakpoint by ourselves. Otherwise it will be
    left there and consequent call to insert_single_step_breakpoint will result
    in a failed assertion in GDB due to inconsistent state. This might happen
    in there is a code gen error (like bl.d 0), and GDB can do nothing about
    it, however if first breakpoint is not cleaned up GDB might fail due to an
    assertion which is not user friendly.

    @param[in] frame  The current frame (THIS frame).
    @return           Non-zero (TRUE) if the breakpoints are inserted
 */
static int
arc_linux_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  CORE_ADDR pc = get_frame_pc (frame);
  CORE_ADDR fall_thru, branch_target;
  int two_breakpoints = arc_linux_next_pc (pc, &fall_thru, &branch_target);

  /* No need to check result. If it fails breakpoint hasn't been created and
   * GDB's internal state will be consistent. */
  insert_single_step_breakpoint (gdbarch, aspace, fall_thru);

  if (two_breakpoints)
    {
      if ((pc != branch_target) && (fall_thru != branch_target))
        {
          /* If second insert fails, first breakpoint has to be removed
           * manually. */
          volatile struct gdb_exception ex;
          TRY_CATCH(ex, RETURN_MASK_ERROR)
            {
              insert_single_step_breakpoint (gdbarch, aspace, branch_target);
            }

          if (ex.reason < 0 && single_step_breakpoints_inserted()) {
              remove_single_step_breakpoints();
		      /* There is nothing else we can do. Pass exception further. */
              throw_exception(ex);
		  }
        }
    }

  return 1;			/* returns always true for now */

}	/* arc_linux_software_single_step () */


/*! Find where to put a breakpoint after the resolver.

    See the comments for SKIP_SOLIB_RESOLVER at the top of infrun.c.
 
    This is called on every single step through the PLT and runtime resolver.
   
    This function:
       1) decides whether a PLT has sent us into the linker to resolve a
          function reference, and
       2) if so, tells us where to set a temporary breakpoint that will
          trigger when the dynamic linker is done. */
static CORE_ADDR
arc_linux_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* For uClibc 0.9.26.
   
     An unresolved PLT entry points to "__dl_linux_resolve", which calls
     "__dl_linux_resolver" to do the resolving and then eventually jumps to
     the function.
   
     So we look for the symbol `_dl_linux_resolver', and if we are there, gdb
     sets a breakpoint at the return address, and continues.

     lookup_minimal_symbol didn't work, for some reason.  */
  struct symbol *resolver = lookup_symbol_global ("_dl_linux_resolver", NULL,
						  VAR_DOMAIN);
  if (arc_debug)
    {
      if (resolver)
	{
	  CORE_ADDR res_addr = BLOCK_START (SYMBOL_BLOCK_VALUE (resolver));
	  fprintf_unfiltered (gdb_stdlog, "--- %s : pc = %s, resolver at %s\n",
			      __FUNCTION__, print_core_address (gdbarch, pc),
			      print_core_address (gdbarch, res_addr));
	}
      else
	{
	  fprintf_unfiltered (gdb_stdlog, "--- %s : pc = %s, no resolver found",
			      __FUNCTION__, print_core_address (gdbarch, pc));
	}
    }

  if (resolver && ((BLOCK_START (SYMBOL_BLOCK_VALUE (resolver))) == pc))
    {
      /* find the return address */
      return  gdbarch_unwind_pc (gdbarch, get_current_frame());
    }
  else
    {
      /* no breakpoint required */
      return 0;
    }
}	/* arc_linux_skip_solib_resolver () */


/*! Call the right architecture variant's supply_gregset function.

    For now, we have only ARCompact. */
static void
arc_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t size)
{
  const bfd_byte *buf = gregs;
  unsigned int reg;

  for (reg = 0; reg < ARC_NUM_RAW_REGS; reg++)
    {
      if (arc_linux_core_reg_offsets[reg] != REGISTER_NOT_PRESENT)
	regcache_raw_supply (regcache,
			     (int) reg,
			     buf + arc_linux_core_reg_offsets[reg]);
    }
}	/* arc_linux_supply_gregset () */


/*! Identify functions for handling core files.

    The first element is a parameter to pass the rest of the functions.  We
    don't need it.
    - supply_gregset is for reading the core file.
    - collect_regset, which we haven't defined, would be for writing the core
      file. */
static const struct regset *
arc_linux_regset_from_core_section (struct gdbarch *core_arch,
				    const char *sect_name, size_t sect_size)
{
  static struct regset arc_linux_gregset = {
    .descr          = NULL,
    .supply_regset  = arc_linux_supply_gregset,
    .collect_regset = NULL,
    .arch           = NULL,
  };

  arc_linux_gregset.arch = core_arch;	/* Not const, so not in initializer */

  if (strcmp (sect_name, ".reg") == 0)
    {
      return &arc_linux_gregset;
    }

  return NULL;

}	/* arc_linux_regset_from_core_section () */


/* -------------------------------------------------------------------------- */
/*                               externally visible functions                 */
/* -------------------------------------------------------------------------- */


/*! Function to determine the OSABI variant.

    Every target variant must define this appropriately.

    @return  The OSABI variant. */
enum gdb_osabi
arc_get_osabi (void)
{
  return GDB_OSABI_LINUX;

}	/* arc_get_osabi () */


/*! Function to initialize for this target variant.

    Every target variant must define this appropriately.

    @param[in,out] gdbarch  The gdbarch we are initializing. */
void
arc_gdbarch_osabi_init (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Fill in target-dependent info in ARC-private structure. */
  tdep->is_sigtramp = arc_linux_is_sigtramp;
  tdep->sigcontext_addr = arc_linux_sigcontext_addr;
  tdep->sc_reg_offset = arc_linux_sc_reg_offset;
  tdep->sc_num_regs = ARC_NUM_RAW_REGS;

  /* Set up target dependent GDB architecture entries. */
  set_gdbarch_cannot_fetch_register (gdbarch, arc_linux_cannot_fetch_register);
  set_gdbarch_cannot_store_register (gdbarch, arc_linux_cannot_store_register);
  set_gdbarch_breakpoint_from_pc (gdbarch, arc_linux_breakpoint_from_pc);
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);
  set_gdbarch_software_single_step (gdbarch, arc_linux_software_single_step);
  set_gdbarch_skip_solib_resolver (gdbarch, arc_linux_skip_solib_resolver);
  set_gdbarch_regset_from_core_section (gdbarch,
					arc_linux_regset_from_core_section);
  /* No need for any other GDB architecture core file functions. */

  /* GNU/Linux uses SVR4-style shared libraries, with 32-bit ints, longs and
     pointers (ILP32). */
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_ilp32_fetch_link_map_offsets);

}	/* arc_gdbarch_osabi_init () */
