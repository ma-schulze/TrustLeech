#include <bl31/interrupt_mgmt.h>
#include <common/context_stash.h>
#include <common/debug.h>
#include <common/ep_info.h>
#include <common/runtime_svc.h>
#include <drivers/arm/gic_common.h>
#include <drivers/arm/gicv3.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/spinlock.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <stdint.h>

#include "arch.h"
#include "arch_helpers.h"
#include "context.h"
#include "export/common/context_stash_exp.h"
#include "trustleech_utils.h"

#define TRUSTLEECH_TRIGGER_INJECTION 0xc3000000
#define TRUSTLEECH_EXIT 0xc3000001
#define TRUSTLEECH_FINISH_TIMER_INT 0xc3000002
#define TRUSTLEECH_BP 0xc3000003
#define TRUSTLEECH_SGI QEMU_IRQ_SEC_SGI_0

#define TRUSTLEECH_TIMER_INTERRUPT 29

#define PLAT_NUM_CORES 4 // TODO?

static uint32_t trustleech_cmd __attribute__((aligned(64))) = 0;
static volatile struct atomic64_t trustleech_counter
    __attribute__((aligned(64))) = {0};
static volatile struct atomic64_t trustleech_timer_counter
    __attribute__((aligned(64))) = {0};
static spinlock_t trustleech_main_lock __attribute__((aligned(64))) = {0};
static volatile uint64_t trustleech_lock __attribute__((aligned(64))) = 0;
static volatile uint64_t trustleech_sgi_reason __attribute__((aligned(64))) = 0;

static volatile uint64_t in_bp = 0;

static uint64_t trustleech_handler(void *);

trustleech_context_stash_t int_stash;

static void copy_context(void *handle, trustleech_context_stash_t *stash) {
  el3_state_t *state = get_el3state_ctx(handle);

  stash->elr_el3 = read_ctx_reg(state, CTX_ELR_EL3);
  stash->sp_el2 = read_sp_el2();
  stash->spsr_el3 = read_ctx_reg(state, CTX_SPSR_EL3);
  stash->sctlr_el2 = read_sctlr_el2();
}

static void restore_context(void *handle, trustleech_context_stash_t *stash) {
  el3_state_t *state = get_el3state_ctx(handle);

  if (stash->elr_el3 == 0xc0000038 && (stash->spsr_el3 & 0xD) == 5) {
    ERROR("THIS SHOULD NOT HAPPEN!\n");
  }

  write_ctx_reg(state, CTX_ELR_EL3, stash->elr_el3);
  write_ctx_reg(state, CTX_SPSR_EL3, stash->spsr_el3);
  write_sp_el2(stash->sp_el2);
  // we know in this case that TL was already activated and therefore, SCTLR_EL2
  // does not change anymore. Therefore, we re-use it to save x0, which is
  // overwritten by the SMC ident
  write_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X0, stash->sctlr_el2);
}

static void hook_context(void *handle, size_t my_pos) {
  const entry_point_info_t *trustleech_ep_info =
      bl31_plat_get_trustleech_ep_info();
  assert(trustleech_ep_info);

  // INFO("Returning to TrustLeech hypervisor\n");
  // print_entry_point_info(trustleech_ep_info);

  el3_state_t *state = get_el3state_ctx(handle);
  write_ctx_reg(state, CTX_ELR_EL3, trustleech_ep_info->pc);
  write_ctx_reg(state, CTX_SPSR_EL3, trustleech_ep_info->spsr);
  /* Setup known-to-be-good stack */
  write_sp_el2(bl31_plat_get_trustleech_tos(my_pos));

  /* Since the hypervisor needs to configure page tables for monitoring any
   * nested hypervisor or OS, it's unnecessary to establish EL2 page tables
   * here. As a result, the existing EL2 page tables, whose state is uncertain,
   * stay active. To facilitate the hypervisor's self-bootstrapping, we
   * deactivate the MMU.
   */
  disable_mmu_el2();
}

static void exit_trustleech(void *handle, trustleech_context_stash_t *stash) {
  el3_state_t *state = get_el3state_ctx(handle);

  write_ctx_reg(state, CTX_ELR_EL3, stash->elr_el3);
  write_sp_el2(stash->sp_el2);
  write_ctx_reg(state, CTX_SPSR_EL3, stash->spsr_el3);
  write_sctlr_el2(stash->sctlr_el2);

  // disable timer
  uint64_t scr = read_scr_el3();
  write_scr_el3(scr & ~((uint64_t)SCR_NS_BIT));
  isb();
  write_cntps_ctl_el1(0);

  // TODO unregister interrupt handlers
}

static int trustleech_sgi(uint32_t intr_raw, uint32_t flags, void *handle,
                          void *cookie) {

  // NOTICE("Received SGI on core 0x%x\n", plat_my_core_pos());

  assert(intr_raw == TRUSTLEECH_SGI);

  if (trustleech_sgi_reason == 0) {
    trustleech_handler(handle);
  } else {
    atomic64_inc(&trustleech_timer_counter);
  }

  /* Deactivate warm reboot SGI */
  plat_ic_end_of_interrupt(TRUSTLEECH_SGI);

  while (!trustleech_lock) {
  }

  return 0;
}
void trustleech_setup_sgi(void) {

  gicv3_disable_interrupt(TRUSTLEECH_SGI, 0);
  gicv3_disable_interrupt(TRUSTLEECH_SGI, 1);
  gicv3_disable_interrupt(TRUSTLEECH_SGI, 2);
  gicv3_disable_interrupt(TRUSTLEECH_SGI, 3);

  gicv3_set_interrupt_group(TRUSTLEECH_SGI, 0, INTR_GROUP0);
  gicv3_set_interrupt_group(TRUSTLEECH_SGI, 1, INTR_GROUP0);
  gicv3_set_interrupt_group(TRUSTLEECH_SGI, 2, INTR_GROUP0);
  gicv3_set_interrupt_group(TRUSTLEECH_SGI, 3, INTR_GROUP0);

  gicv3_set_interrupt_priority(TRUSTLEECH_SGI, 0, 0x20);
  gicv3_set_interrupt_priority(TRUSTLEECH_SGI, 1, 0x20);
  gicv3_set_interrupt_priority(TRUSTLEECH_SGI, 2, 0x20);
  gicv3_set_interrupt_priority(TRUSTLEECH_SGI, 3, 0x20);

  ehf_register_priority_handler(0x20, trustleech_sgi);

  gicv3_enable_interrupt(TRUSTLEECH_SGI, 0);
  gicv3_enable_interrupt(TRUSTLEECH_SGI, 1);
  gicv3_enable_interrupt(TRUSTLEECH_SGI, 2);
  gicv3_enable_interrupt(TRUSTLEECH_SGI, 3);
}

static int trustleech_timer_handler(uint32_t id, uint32_t flags, void *handle,
                                    void *cookie) {
  /*
   * Disable the timer. The barriers ensure that there is
   * no reordering of instructions around the reprogramming code.
   */
  isb();
  write_cntps_ctl_el1(0);
  isb();

  /*
   * Mark this interrupt as complete to avoid a FIQ storm.
   */
  plat_ic_end_of_interrupt(id);

  trustleech_sgi_reason = 1;
  trustleech_lock = 0;

  uint32_t this_cpu = plat_my_core_pos();

  for (int i = 0; i < PLAT_NUM_CORES; i++) {
    if (i == this_cpu) {
      continue;
    }
    plat_ic_raise_el3_sgi(TRUSTLEECH_SGI, i);
  }
  atomic64_inc(&trustleech_timer_counter);

  while (trustleech_timer_counter.counter < 4) {
  }
  // all cores are now waiting in EL3, drop our core to TL

  copy_context(handle, &int_stash);
  int_stash.sctlr_el2 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X0);

  const entry_point_info_t *trustleech_ep_info =
      bl31_plat_get_trustleech_ep_info();
  assert(trustleech_ep_info);

  INFO("Returning to TrustLeech hypervisor interrupt stack!\n");

  el3_state_t *state = get_el3state_ctx(handle);
  write_ctx_reg(state, CTX_ELR_EL3, trustleech_ep_info->pc);
  write_ctx_reg(state, CTX_SPSR_EL3, trustleech_ep_info->spsr);
  /* Setup known-to-be-good stack */
  write_sp_el2(bl31_plat_get_trustleech_tos_int(this_cpu));

  return 0;
}

static int trustleech_timer_finish(void *handle) {

  trustleech_timer_counter.counter = 0;
  trustleech_lock = 1;

  /* Reprogramm the timer */
  INFO("Setting the timer\n");
  uint64_t cval;
  uint32_t ctl = 0;

  // The timer will fire every 4 second
  cval = read_cntpct_el0() + (read_cntfrq_el0() << 5);
  write_cntps_cval_el1(cval);

  restore_context(handle, &int_stash);

  // Enable the secure physical timer
  isb();
  set_cntp_ctl_enable(ctl);
  isb();
  write_cntps_ctl_el1(ctl);
  isb();
  return 0;
}

static void trustleech_setup_timer() {

  INFO("Setting the timer\n");
  uint64_t cval;
  uint32_t ctl = 0;

  uint64_t scr = read_scr_el3();
  write_scr_el3(scr & ~((uint64_t)SCR_NS_BIT));
  isb();

  // The timer will fire every 4 second
  cval = read_cntpct_el0() + (read_cntfrq_el0() << 4);
  write_cntps_cval_el1(cval);

  // Enable the secure physical timer
  isb();
  set_cntp_ctl_enable(ctl);
  isb();
  write_cntps_ctl_el1(ctl);
  isb();

  write_cntp_tval_el0(2 * 1000000);
  write_cntp_ctl_el0(3);

  write_scr_el3(scr);
  isb();

  uint32_t this_cpu = plat_my_core_pos();

  gicv3_disable_interrupt(TRUSTLEECH_TIMER_INTERRUPT, this_cpu);
  gicv3_set_interrupt_group(TRUSTLEECH_TIMER_INTERRUPT, this_cpu, INTR_GROUP0);
  gicv3_set_interrupt_priority(TRUSTLEECH_TIMER_INTERRUPT, this_cpu, 0x30);

  ehf_register_priority_handler(0x30, trustleech_timer_handler);
  gicv3_enable_interrupt(TRUSTLEECH_TIMER_INTERRUPT, this_cpu);
}

static int trustleech_start_bp(void *handle) {

  isb();
  write_cntps_ctl_el1(0);
  isb();
  in_bp = true;

  trustleech_sgi_reason = 1;
  trustleech_lock = 0;

  uint32_t this_cpu = plat_my_core_pos();

  for (int i = 0; i < PLAT_NUM_CORES; i++) {
    if (i == this_cpu) {
      continue;
    }
    plat_ic_raise_el3_sgi(TRUSTLEECH_SGI, i);
  }
  atomic64_inc(&trustleech_timer_counter);

  while (trustleech_timer_counter.counter < 4) {
  }
  return 0;
}

static int trustleech_finish_bp(void *handle) {

  uint64_t scr = read_scr_el3();
  write_scr_el3(scr & ~((uint64_t)SCR_NS_BIT));
  isb();

  // The timer will fire every 4 second
  uint64_t cval = read_cntpct_el0() + (read_cntfrq_el0() << 4);
  write_cntps_cval_el1(cval);
  trustleech_lock = 1;
  uint32_t ctl = 0;

  isb();
  set_cntp_ctl_enable(ctl);
  isb();
  write_cntps_ctl_el1(ctl);
  isb();

  return 0;
}

static int32_t trustleech_svc_setup() {

  trustleech_setup_sgi();
  return 0;
}

static uint64_t trustleech_handler(void *handle) {

  const size_t my_pos = plat_my_core_pos();

  trustleech_context_stash_t *context_stash = NULL;
  size_t context_num_elements = 0;

  bl31_plat_get_trustleech_context_stash(&context_stash, &context_num_elements);

  assert(context_stash);
  assert(my_pos < context_num_elements);

  if (trustleech_cmd == TRUSTLEECH_EXIT) {
    exit_trustleech(handle, &context_stash[my_pos]);
  } else {
    /* Copy over register we overwrite inside the firmware, for the hypervisor
     * to be able to restore them later
     */
    copy_context(handle, &context_stash[my_pos]);
    hook_context(handle, my_pos);
  }

  atomic64_inc(&trustleech_counter);
  return 0;
}

static uintptr_t trustleech_svc_smc_handler(uint32_t smc_fid, u_register_t x1,
                                            u_register_t x2, u_register_t x3,
                                            u_register_t x4, void *cookie,
                                            void *handle, u_register_t flags) {

  // we are ok with unaligned EL3 mem access
  uint64_t sctlr_el3 = read_sctlr_el3();
  sctlr_el3 &= ~(SCTLR_A_BIT);
  sctlr_el3 &= ~(SCTLR_SA_BIT);
  write_sctlr_el3(sctlr_el3);

  if (smc_fid == TRUSTLEECH_BP) {
    if (x1 == 1) {
      trustleech_start_bp(handle);
    } else {
      trustleech_finish_bp(handle);
    }

    SMC_RET0(handle);
  }

  if (smc_fid == TRUSTLEECH_FINISH_TIMER_INT) {
    trustleech_timer_finish(handle);
    trustleech_lock = 1;

    SMC_RET0(handle);
  }

  if (smc_fid == TRUSTLEECH_TRIGGER_INJECTION) {
    spin_lock(&trustleech_main_lock);
  }

  trustleech_lock = 0;

  if (smc_fid != TRUSTLEECH_TRIGGER_INJECTION && smc_fid != TRUSTLEECH_EXIT) {
    // WARN("Unimplemented TrustLeech Service Call: 0x%x \n",
    // 	smc_fid);
    SMC_RET1(handle, SMC_UNK);
  }

  trustleech_cmd = smc_fid;

  trustleech_handler(handle);

  if (smc_fid == TRUSTLEECH_TRIGGER_INJECTION) {

    trustleech_setup_timer();

    trustleech_sgi_reason = 0;
    uint32_t this_cpu = plat_my_core_pos();

    for (int i = 0; i < PLAT_NUM_CORES; i++) {
      if (i == this_cpu) {
        continue;
      }
      plat_ic_raise_el3_sgi(TRUSTLEECH_SGI, i);
    }
    while (trustleech_counter.counter < 4) {
    }
  }

  bl31_plat_trustleech_configure_tzasc(smc_fid == TRUSTLEECH_EXIT);

  trustleech_cmd = 0;
  trustleech_counter.counter = 0;

  trustleech_lock = 1;

  // NOTICE("Continueing main CPU!\n");

  if (smc_fid == TRUSTLEECH_TRIGGER_INJECTION) {
    spin_unlock(&trustleech_main_lock);
  }

  SMC_RET0(handle);
}

DECLARE_RT_SVC(oem_svc, OEN_OEM_START, OEN_OEM_END, SMC_TYPE_FAST,
               trustleech_svc_setup, trustleech_svc_smc_handler);
