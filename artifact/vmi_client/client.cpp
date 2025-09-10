#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <libvmi/events.h>
#include <libvmi/libvmi.h>

#include <iostream>
#include <string>
#include <chrono>

#include "openssl/sha.h"

#include <iostream>

event_response_t breakpoint_cb(vmi_instance_t vmi, vmi_event_t *event) {
  printf("HIT MY BP!\n");
  vmi_set_vcpureg(vmi, 0, DBGBCR0, 0);
  vmi_resume_vm(vmi);
  return VMI_EVENT_RESPONSE_NONE;
}

#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h>

uint8_t ground_truth[32] = {0xd0, 0x9b, 0x27, 0xa2, 0x96, 0x97, 0x3d, 0xd8,
                            0xc5, 0xd8, 0xb6, 0x84, 0x7b, 0x23, 0x4e, 0xfc,
                            0x42, 0xe2, 0x56, 0x04, 0xf9, 0x62, 0xb4, 0x9d,
                            0xb8, 0x6c, 0x3c, 0x60, 0x2a, 0x2a, 0x9c, 0x53};

// Function to compute SHA-256 hash
bool compute_sha256(const unsigned char *input, size_t input_length,
                    unsigned char *output, unsigned int &output_length) {
  // Create a context for the hashing operation
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    std::cerr << "Failed to create OpenSSL context" << std::endl;
    return false;
  }

  // Initialize the context for SHA-256 hashing
  if (!EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
    std::cerr << "Failed to initialize SHA-256 operation" << std::endl;
    EVP_MD_CTX_free(context);
    return false;
  }

  // Update the context with the input data
  if (!EVP_DigestUpdate(context, input, input_length)) {
    std::cerr << "Failed to update SHA-256 operation" << std::endl;
    EVP_MD_CTX_free(context);
    return false;
  }

  // Finalize the hash and write to the output buffer
  if (!EVP_DigestFinal_ex(context, output, &output_length)) {
    std::cerr << "Failed to finalize SHA-256 operation" << std::endl;
    EVP_MD_CTX_free(context);
    return false;
  }

  // Clean up
  EVP_MD_CTX_free(context);
  return true;
}

void walk_module_list(vmi_instance_t &vmi) {
  addr_t next_module = 0;
  addr_t list_head = 0;
  vmi_read_addr_ksym(vmi, "modules", &next_module);

  list_head = next_module;

  /* walk the module list */
  while (1) {

    /* follow the next pointer */
    addr_t tmp_next = 0;

    vmi_read_addr_va(vmi, next_module, 0, &tmp_next);

    /* if we are back at the list head, we are done */
    if (list_head == tmp_next) {
      break;
    }

    /* print out the module name */

    /* Note: the module struct that we are looking at has a string
     * directly following the next / prev pointers.  This is why you
     * can just add the length of 2 address fields to get the name.
     * See include/linux/module.h for mode details */
    char *modname = NULL;

    modname = vmi_read_str_va(vmi, next_module + 16, 0);
    // printf("%s\n", modname);
    free(modname);
    next_module = tmp_next;
  }
}

void init_vmi(vmi_instance_t &vmi) {
  status_t status = VMI_SUCCESS;
  struct sigaction act;
  int retcode = 1;
  vmi_init_data_t *init_data = NULL;

  uint64_t id = 1;

  if (VMI_FAILURE == vmi_init_complete(&vmi, (void *)&id,
                                       VMI_INIT_DOMAINID,
                                       NULL, VMI_CONFIG_FILE_PATH,
                                       (void *)"./libvmi.conf", NULL)) {
    printf("Failed to init LibVMI library.\n");
  }
}

void walk_keyboard_notifiers(vmi_instance_t &vmi) {

  addr_t next_not = 0;
  addr_t not_list_head = 0;

  addr_t notifier_list;

  vmi_translate_ksym2v(vmi, "keyboard_notifier_list", &notifier_list);

  vmi_read_addr_va(vmi, notifier_list + 8, 0, &not_list_head);

  if (not_list_head == 0) {
    return;
  }

  next_not = not_list_head;

  /* walk the notifiers list */
  while (1) {

    addr_t not_cb = 0;

    vmi_read_addr_va(vmi, next_not, 0, &not_cb);

    if (not_cb > 0xffffffc080b376cc || not_cb < 0xffffffc080000000) {
      // printf("HOOKED VIRTFS\n");
    }

    /* follow the next pointer */
    addr_t tmp_next = 0;
    vmi_read_addr_va(vmi, next_not + 8, 0, &tmp_next);

    /* if we are back at the list head, we are done */
    if (not_list_head == tmp_next) {
      break;
    }

    next_not = tmp_next;
  }
}

void check_sct(vmi_instance_t &vmi) {

  // scan syscall table
  unsigned char sct[8192] = {0};
  unsigned char sha[32] = {0};
  status_t res = vmi_read_va(vmi, 0xffffffc0808508b0, 0, 8192, sct, NULL);
  if (res != VMI_SUCCESS) {
    fprintf(stderr, "Failed to get sct\n");
  }

  unsigned int len = 32;
  compute_sha256(sct, 8192, sha, len);
  // std::cout << "SHA-256 Hash: ";
  // for (unsigned int i = 0; i < len; ++i) {
  //   printf("%02x, ", sha[i]);
  // }
  // std::cout << std::endl;
  bool sct_good =
      std::equal(std::begin(sha), std::end(sha), std::begin(ground_truth));

  // std::cout << "SCT is good? " << (sct_good ? "true" : "false") << std::endl;
}

void walk_process_list(vmi_instance_t &vmi) {

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
    // printf("[%5d] %s (struct addr:%" PRIx64 ")\n", pid, procname,
    //        current_process);
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

void walk_process_list_open_files(vmi_instance_t &vmi) {

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
    // printf("[%5d] %s (struct addr:%" PRIx64 ")\n", pid, procname,
    //        current_process);
    if (procname) {
      free(procname);
      procname = NULL;
    }

    // Get FDS
    addr_t files;
    vmi_read_addr_va(vmi, current_process + 1560, 0, &files);
    addr_t fdt;
    vmi_read_addr_va(vmi, files + 32, 0, &fdt);
    uint64_t max_fds;
    vmi_read_addr_va(vmi, fdt, 0, &max_fds);
    addr_t fd;
    vmi_read_addr_va(vmi, fdt + 8, 0, &fd);
    uint64_t open_fds;
    vmi_read_addr_va(vmi, fdt + 24, 0, &open_fds);

    for (int i = 0; (1 << i) < max_fds; i++) {
      if (open_fds == 0) {
        continue;
      }
      vmi_read_addr_va(vmi, open_fds, 0, &open_fds);

      if ((open_fds & (1 << i)) == 0) {
        continue;
      }
      addr_t file;
      vmi_read_addr_va(vmi, fd + 8 * i, 0, &file);

      if (file == 0) {
        continue;
      }

      addr_t dentry;
      vmi_read_addr_va(vmi, file + 0xa0, 0, &dentry);

      char *d_iname = vmi_read_str_va(vmi, dentry + 0x38, 0);
      // std::cout << "Open File: " << d_iname << std::endl;
      free(d_iname);
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
    // printf("[%5d] %s (struct addr:%" PRIx64 ")\n", pid, procname,
    //        current_process);
    if (procname) {
      free(procname);
      procname = NULL;
    }
    addr_t creds;
    vmi_read_addr_va(vmi, current_process + 1496, 0, &creds);
    addr_t gid_uid;
    vmi_read_addr_va(vmi, creds + 4, 0, &gid_uid);
    uint32_t uid = gid_uid & (0xFFFFFFFF);
    uint32_t gid = (gid_uid >> 32);
    // std::cout << "Got UID:GID " << uid << ":" << gid << std::endl;

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

void check_virt_fs_hook(vmi_instance_t &vmi) {

  addr_t proc_root;

  if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "proc_root", &proc_root))
    return;

  addr_t proc_fops;
  vmi_read_addr_va(vmi, proc_root + 48, 0, &proc_fops);

  addr_t read_fops;
  vmi_read_addr_va(vmi, proc_fops + 16, 0, &read_fops);

  if (read_fops > 0xffffffc080b376cc || read_fops < 0xffffffc080000000) {
    // printf("HOOKED VIRTFS\n");
  }
}

// in Related work this was still tcp4_seq_afinfo
void check_tcp4_seq_ops(vmi_instance_t &vmi) {

  addr_t tcp4_seq_ops;

  if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "tcp4_seq_ops", &tcp4_seq_ops))
    return;

  addr_t show;
  vmi_read_addr_va(vmi, tcp4_seq_ops + 24, 0, &show);

  if (show > 0xffffffc080b376cc || show < 0xffffffc080000000) {
    // printf("HOOKED TCP4 SEQ OPS\n");
  }
  // open does not exist anymore
}

void check_netdev_hooks(vmi_instance_t &vmi) {

  addr_t init_net;

  if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_net", &init_net))
    return;

  addr_t net_dev;
  vmi_read_addr_va(vmi, init_net + 136, 0, &net_dev);

  addr_t first_dev = net_dev;
  uint16_t num_hook_entries;
  addr_t hook;

  while (net_dev != 0) {

    // printf("VA: 0x%lx\n", net_dev);
    // Ingress
    addr_t nf;
    vmi_read_addr_va(vmi, net_dev + 808, 0, &nf);

    if (nf) {
      vmi_read_16_va(vmi, nf, 0, &num_hook_entries);

      // printf("num hook entries: 0x%x\n", num_hook_entries);
      for (int i = 0; i < num_hook_entries; i++) {

        vmi_read_addr_va(vmi, nf + 2 + i * 16, 0, &hook);
        if (hook > 0xffffffc080b376cc || hook < 0xffffffc080000000) {
          // printf("HOOKED NETDEV OPS\n");
        }
      }
    }

    // Egress
    vmi_read_addr_va(vmi, net_dev + 952, 0, &nf);

    if (nf) {
      vmi_read_16_va(vmi, nf, 0, &num_hook_entries);

      // printf("num hook entries: 0x%x\n", num_hook_entries);
      for (int i = 0; i < num_hook_entries; i++) {

        vmi_read_addr_va(vmi, nf + 2 + i * 16, 0, &hook);
        if (hook > 0xffffffc080b376cc || hook < 0xffffffc080000000) {
          // printf("HOOKED NETDEV OPS\n");
        }
      }
    }

    vmi_read_addr_va(vmi, net_dev + 0x40, 0, &net_dev);
    net_dev -= 0x40;
    if (net_dev == first_dev) {
      break;
    }
  }
}

void check_tty_driver_hooks(vmi_instance_t &vmi) {

  addr_t tty_drivers;

  if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "tty_drivers", &tty_drivers))
    return;

  addr_t tty_driver;
  vmi_read_addr_va(vmi, tty_drivers, 0, &tty_driver);
  tty_driver -= 168;
  addr_t tty_driver_first = tty_driver;

  while (tty_driver != 0) {

    // printf("VA: 0x%lx\n", tty_driver);

    uint32_t num;
    vmi_read_32_va(vmi, tty_driver + 52, 0, &num);

    // printf("NUM: 0x%x\n", num);
    if (num) {

      addr_t ttys;
      vmi_read_addr_va(vmi, tty_driver + 128, 0, &ttys);

      if (ttys) {

        addr_t tty;
        addr_t ldisc;
        for (int i = 0; i < num; i++) {

          vmi_read_addr_va(vmi, ttys + 8 * i, 0, &tty);

          if (tty == 0) {
            continue;
          }
          vmi_read_addr_va(vmi, tty + 0x28, 0, &ldisc);
          if (ldisc == 0) {
            continue;
          }
          vmi_read_addr_va(vmi, ldisc, 0, &ldisc);
          if (ldisc == 0) {
            continue;
          }
          addr_t hook;
          vmi_read_addr_va(vmi, ldisc + 0x10, 0, &ldisc);

          if (hook > 0xffffffc080b376cc || hook < 0xffffffc080000000) {
            // printf("HOOKED TTY DRIVER OPS\n");
          }
        }
      }
    }

    vmi_read_addr_va(vmi, tty_driver + 168, 0, &tty_driver);
    if (!tty_driver) {
      break;
    }
    if (tty_driver == tty_drivers) {
      break;
    }

    tty_driver -= 168;
  }
}

// in Related work this was still tcp4_seq_afinfo
void read_single_page(vmi_instance_t &vmi) {

  uint8_t page[4096];
  vmi_read_pa(vmi, 0x80000000, 4096, (void *)page, NULL);
}

typedef void (*vmi_func_t)(vmi_instance_t &);
void time_vmi_func(vmi_instance_t &vmi, vmi_func_t vmi_func,
                   const std::string &test_name) {

  vmi_pagecache_flush(vmi);
  auto start_time = std::chrono::steady_clock::now();

  vmi_func(vmi);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);


  std::cout << test_name << ": " << duration.count() << " µs" << std::endl;
}

int main() {
  std::cout << "Starting TL VMI!" << std::endl;

  vmi_instance_t vmi = {0};
  time_vmi_func(vmi, init_vmi, "Init VMI");

#if 1
  for (int i = 0; i < 5; i++) {
    printf("LibVMI init succeeded, waiting for CMD!\n");
    time_vmi_func(vmi, walk_module_list, std::string{"Module List"});

    time_vmi_func(vmi, walk_keyboard_notifiers, "Check Keyboard Notifiers");

    time_vmi_func(vmi, check_sct, "Checking SCT");

    time_vmi_func(vmi, walk_process_list, "Walking Process List");

    time_vmi_func(vmi, walk_process_list_open_files,
                  "Walking Process List Open Files");

    time_vmi_func(vmi, walk_process_list_priv_esc,
                  "Walking Process List Priv Esc");

    time_vmi_func(vmi, check_virt_fs_hook, "Virt FS Hook");

    time_vmi_func(vmi, check_tcp4_seq_ops, "Netstat Ops");

    time_vmi_func(vmi, check_netdev_hooks, "Netdev Ops");

    time_vmi_func(vmi, check_tty_driver_hooks, "TTY Drivers");

    time_vmi_func(vmi, read_single_page, "Single Page");
  }

// #define DIAMORPHINE

#ifdef SUTEKH
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, 0xffffffc078f20000, DBGBVR0, 0);
#endif
#ifdef SPY
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, 0xffffffc078f20000, DBGBVR0, 0);
#endif
#ifdef DIAMORPHINE
  uint64_t dents = vmi_read_addr_ksym(vmi, "__arm64_sys_getdents64", &dents);
  vmi_set_vcpureg(vmi, 0x21e7, DBGBCR0, 0);
  vmi_set_vcpureg(vmi, dents, DBGBVR0, 0);
#endif
#endif
  // vmi_event_t int_event;
  // memset(&int_event, 0, sizeof(vmi_event_t));
  // int_event.version = VMI_EVENTS_VERSION;
  // int_event.type = VMI_EVENT_INTERRUPT;
  // int_event.callback = breakpoint_cb;
  //
  // printf("Register interrupt event\n");
  // if (VMI_FAILURE == vmi_register_event(vmi, &int_event)) {
  //   fprintf(stderr, "Failed to register interrupt event\n");
  // }

  vmi_resume_vm(vmi);

  while (1) {
    vmi_events_listen(vmi, 100000);
  }

  /* cleanup any memory associated with the libvmi instance */
  vmi_destroy(vmi);
  return 0;
}
