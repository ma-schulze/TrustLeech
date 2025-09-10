# KVM Driver

## Overview

This contains the implementation of libVMI for TrustLeech.

## LibVMI API Implementation

This section will give an implementation status of the LibVMI API on TrustLeech.

- [ ] r/w physical memory
- [ ] VCPU registers
    - [ ] read
        - [ ] general purpose registers
        - [ ] control registers
    - [ ] write
        - [ ] general purpose registers
        - [ ] control registers
- [ ] memory size
- [ ] pause / resume
- [ ] request page fault
- [ ] guest memory mapping
- [ ] TSC info
  - [ ] `tsc_mode`
  - [ ] `elapsed_nsec`
  - [ ] `gtsc_khz`
  - [ ] `incarnation`
- [ ] MTRR
- [ ]  SAVE
- [ ] SLAT
- [ ] VMI Events
    - [ ] singlestep (not supported in `KVMi-v6`)
    - [ ] register access
        - [ ] `reg_event.reg`
            - [ ] CR
            - [ ] MSR
                - [ ] `MSR_ALL` (loop over all defined MSRs in LibVMI. Unable to set intercept on any kind of MSR in `KVMi-v6`)
        - [ ] `reg_event.equal`
        - [ ] `reg_event.async`
        - [ ] `reg_event.onchange`
        - [ ] `reg_event.in_access`
            - [ ] `VMI_REGACCESS_N`
            - [ ] `VMI_REGACCESS_W`
            - [ ] `VMI_REGACCESS_R` (not available in `KVMi-v6`)
            - [ ] `VMI_REGACCESS_RW` (not available in `KVMi-v6`)
        - [ ] `reg_event.out_access`
        - [ ] `reg_event.value`
        - [ ] `reg_event.previous`
        - [ ] `reg_event.msr`
    - [ ] interrupt
        - [ ] `interrupt_event.intr`
            - [ ] `INT3`
            - [ ] `INT_NE T`
        - [ ] `interrupt_event.insn_length`
        - [ ] `interrupt_event.reinject`
        - [ ] `interrupt_event.vector`
        - [ ] `interrupt_event.type`
        - [ ] `interrupt_event.error_code`
        - [ ] `interrupt_event.cr2`
        - [ ] `interrupt_event.gla`
        - [ ] `interrupt_event.gfn`
        - [ ] `interrupt_event.offset`
    - [ ] memory access
        - [ ] `mem_event.gfn`
        - [ ] `mem_event.generic`
        - [ ] `mem_event.in_access`
        - [ ] `mem_event.out_access`
        - [ ] `mem_event.gptw`
        - [ ] `mem_event.gla_valid`
        - [ ] `mem_event.gla`
        - [ ] `mem_event.offset`
    - [ ] cpuid
    - [ ] privcall
    - [ ] descriptor
        - [ ] `desc_event.instr_info`
        - [ ] `desc_event.e it_qualification`
        - [ ] `desc_event.e it_info`
        - [ ] `desc_event.descriptor`
        - [ ] `desc_event.is_write`
- [ ] VMI Event response
    - [ ] `VMI_EVENT_RESPONSE_NONE`
    - [ ] `VMI_EVENT_RESPONSE_EMULATE`
    - [ ] `VMI_EVENT_RESPONSE_EMULATE_NOWRITE`
    - [ ] `VMI_EVENT_RESPONSE_SET_EMUL_READ_DATA` (only for memory access events)
    - [ ] `VMI_EVENT_RESPONSE_DENY`
    - [ ] `VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP`
    - [ ] `VMI_EVENT_RESPONSE_SLAT_ID`
    - [ ] `VMI_EVENT_RESPONSE_VMM_PAGETABLE_ID`
    - [ ] `VMI_EVENT_RESPONSE_SET_REGISTERS`
    - [ ] `VMI_EVENT_RESPONSE_SET_EMUL_INSN`
    - [ ] `VMI_EVENT_RESPONSE_GET_NE T_INTERRUPT`

