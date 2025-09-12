#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <libvmi/events.h>
#include <libvmi/libvmi.h>

#include <chrono>
#include <iostream>
#include <string>

#include "openssl/sha.h"

#include <iostream>

void walk_process_list_priv_esc(vmi_instance_t &vmi) {

  unsigned long tasks_offset = 0, pid_offset = 0, name_offset = 0;
  addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
  addr_t current_process = 0;
  char *procname = NULL;
  vmi_pid_t pid = 0;
  status_t status = VMI_FAILURE;

  if (VMI_FAILURE == vmi_get_offset(vmi, "linux_tasks", &tasks_offset))
    return;
  if (VMI_FAILURE == vmi_get_offset(vmi, "linux_name", &name_offset))
    return;
  if (VMI_FAILURE == vmi_get_offset(vmi, "linux_pid", &pid_offset))
    return;
  if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_task", &list_head))
    return;

  list_head += tasks_offset;

  cur_list_entry = list_head;
  if (VMI_FAILURE ==
      vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry)) {
    printf("Failed to read next pointer at %" PRIx64 "\n", cur_list_entry);
    return;
  }

  /* walk the task list */
  while (1) {

    current_process = cur_list_entry - tasks_offset;

    vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t *)&pid);

    procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

    if (!procname) {
      printf("Failed to find procname\n");
      return;
    }

    /* print out the process name */
    addr_t proc_state;
    vmi_read_addr_va(vmi, current_process + 24, 0, &proc_state);

    if (proc_state == 0) {
      // TAST_RUNNING
      printf("[%5d] %s (struct addr:%" PRIx64 ")\n", pid, procname,
             current_process);
    }

    if (procname) {
      free(procname);
      procname = NULL;
    }

    /* follow the next pointer */
    cur_list_entry = next_list_entry;
    status = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
    if (status == VMI_FAILURE) {
      printf("Failed to read next pointer in loop at %" PRIx64 "\n",
             cur_list_entry);
      return;
    }
    if (cur_list_entry == list_head) {
      break;
    }
  };
}

event_response_t breakpoint_cb(vmi_instance_t vmi, vmi_event_t *event) {
  printf("HIT MY BP!\n");
  // vmi_set_vcpureg(vmi, 0, DBGBCR0, 0);
  walk_process_list_priv_esc(vmi);
  return VMI_EVENT_RESPONSE_NONE;
}

void init_vmi(vmi_instance_t &vmi) {
  status_t status = VMI_SUCCESS;
  struct sigaction act;
  int retcode = 1;
  vmi_init_data_t *init_data = NULL;

  uint64_t id = 1;

  if (VMI_FAILURE == vmi_init_complete(&vmi, (void *)&id,
                                       VMI_INIT_DOMAINID | VMI_INIT_EVENTS,
                                       NULL, VMI_CONFIG_FILE_PATH,
                                       (void *)"./libvmi.conf", NULL)) {
    printf("Failed to init LibVMI library.\n");
  }
}

typedef void (*vmi_func_t)(vmi_instance_t &);
void time_vmi_func(vmi_instance_t &vmi, vmi_func_t vmi_func,
                   const std::string &test_name) {

  vmi_pagecache_flush(vmi);
  auto start_time = std::chrono::steady_clock::now();

  vmi_func(vmi);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  std::cout << test_name << ": " << duration.count() << " ms" << std::endl;
}

int main() {
  std::cout << "Starting TL VMI!" << std::endl;

  vmi_instance_t vmi = {0};
  time_vmi_func(vmi, init_vmi, "Init VMI");

#define DIAMORPHINE

#ifdef SUTEKH
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, 0xffffffc078f20000, DBGBVR0, 0);
#endif
#ifdef SPY
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, 0xffffffc078f20000, DBGBVR0, 0);
#endif

#ifdef DIAMORPHINE
  uint64_t addr = 0;
  vmi_read_64_va(vmi, 0xffffffc080850cb8, 0, &addr);
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, addr, DBGBVR0, 0);
#endif
  vmi_event_t int_event;
  memset(&int_event, 0, sizeof(vmi_event_t));
  int_event.version = VMI_EVENTS_VERSION;
  int_event.type = VMI_EVENT_INTERRUPT;
  int_event.callback = breakpoint_cb;

  printf("Register interrupt event\n");
  if (VMI_FAILURE == vmi_register_event(vmi, &int_event)) {
    fprintf(stderr, "Failed to register interrupt event\n");
  }

  vmi_resume_vm(vmi);

  while (1) {
    vmi_events_listen(vmi, 100000);
  }

  /* cleanup any memory associated with the libvmi instance */
  vmi_destroy(vmi);
  return 0;
}
