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

void init_vmi(vmi_instance_t &vmi) {
  status_t status = VMI_SUCCESS;
  struct sigaction act;
  int retcode = 1;
  vmi_init_data_t *init_data = NULL;

  uint64_t id = 1;

  if (VMI_FAILURE == vmi_init_complete(&vmi, (void *)&id, VMI_INIT_DOMAINID,
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
  vmi_resume_vm(vmi);

  while (1) {
    vmi_events_listen(vmi, 100000);
  }

  /* cleanup any memory associated with the libvmi instance */
  vmi_destroy(vmi);
  return 0;
}
