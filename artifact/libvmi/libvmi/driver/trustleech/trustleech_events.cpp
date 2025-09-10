
#include "trustleech.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libvmi.h>

status_t trustleech_events_init(vmi_instance_t vmi, uint32_t init_flags,
                                vmi_init_data_t *init_data) {

  return VMI_SUCCESS;
}

static status_t process_single_event(vmi_instance_t vmi,
                                     struct kvmi_dom_event **event) {
  status_t status = VMI_SUCCESS;
  unsigned int ev_reason = 0;
  kvm_instance_t *kvm = kvm_get_instance(vmi);

  // handle event
  ev_reason = (*event)->event.common.event;

  // special case to handle PAUSE events
  // since they have to managed by vmi_resume_vm(), we simply store them
  // in the kvm_instance for later use by this function
  if (KVMI_EVENT_PAUSE_VCPU == ev_reason) {
#ifdef ENABLE_SAFETY_CHECKS
    uint16_t vcpu = (*event)->event.common.vcpu;
    // silence unused variable warnings if not asserts
    (void)vcpu;
    assert(vcpu < vmi->num_vcpus);
#endif
    dbprint(VMI_DEBUG_KVM, "--Moving PAUSE_VPCU event in the buffer\n");
    kvm->pause_events_list[(*event)->event.common.vcpu] = (*event);
    (*event) = NULL;
    return VMI_SUCCESS;
  }
#ifdef ENABLE_SAFETY_CHECKS
  if (ev_reason >= KVMI_NUM_EVENTS || !kvm->process_event[ev_reason]) {
    errprint("Undefined handler for %u event reason\n", ev_reason);
    status = VMI_FAILURE;
    goto cleanup;
  }
#endif
  // call handler
  status = kvm->process_event[ev_reason](vmi, (*event));

cleanup:
  free((*event));
  (*event) = NULL;
  return status;
}

static status_t process_pending_events(vmi_instance_t vmi) {
  kvm_instance_t *kvm = kvm_get_instance(vmi);
  struct kvmi_dom_event *event = NULL;

  while (kvm->libkvmi.kvmi_get_pending_events(kvm->kvmi_dom) > 0) {
    if (kvm->libkvmi.kvmi_pop_event(kvm->kvmi_dom, &event)) {
      errprint("%s: kvmi_pop_event failed: %s\n", __func__, strerror(errno));
      return VMI_FAILURE;
    }

    process_single_event(vmi, &event);
  }

  return VMI_SUCCESS;
}

status_t kvm_process_events_with_timeout(vmi_instance_t vmi, uint32_t timeout) {
  kvm_instance_t *kvm = kvm_get_instance(vmi);
  struct kvmi_dom_event *event = NULL;

  if (kvm_get_next_event(kvm, &event, (kvmi_timeout_t)timeout) == VMI_FAILURE) {
    errprint("%s: Failed to get next KVMi event: %s\n", __func__,
             strerror(errno));
    return VMI_FAILURE;
  }
  if (!event) {
    return VMI_SUCCESS;
  }

  process_single_event(vmi, &event);

  // make sure that all pending events are processed
  return process_pending_events(vmi);
}
