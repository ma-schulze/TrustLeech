
/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Bryan D. Payne (bdpayne@acm.org)
 * Author: Tamas K Lengyel (tamas.lengyel@zentific.com)
 *
 * This file is part of LibVMI.
 *
 * LibVMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * LibVMI is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LibVMI.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRUSTLEECH_H
#define TRUSTLEECH_H

status_t trustleech_init(vmi_instance_t vmi, uint32_t init_flags,
                         vmi_init_data_t *init_data);

status_t trustleech_init_vmi(vmi_instance_t vmi, uint32_t init_flags,
                             vmi_init_data_t *init_data);

void trustleech_destroy(vmi_instance_t vmi);
uint64_t trustleech_get_id_from_name(vmi_instance_t vmi, const char *name);

status_t trustleech_get_name_from_id(vmi_instance_t vmi, uint64_t domainid,
                                     char **name);

uint64_t trustleech_get_id(vmi_instance_t vmi);

void trustleech_set_id(vmi_instance_t vmi, uint64_t domainid);

status_t trustleech_check_id(vmi_instance_t vmi, uint64_t domainid);

status_t trustleech_write(vmi_instance_t vmi, addr_t paddr, void *buf,
                          uint32_t length);

status_t trustleech_get_name(vmi_instance_t vmi, char **name);

void trustleech_set_name(vmi_instance_t vmi, const char *name);

status_t trustleech_get_memsize(vmi_instance_t vmi, uint64_t *allocate_ram_size,
                                addr_t *maximum_physical_address);

status_t trustleech_get_next_available_gfn(vmi_instance_t vmi,
                                           addr_t *next_gfn);

status_t trustleech_request_page_fault(vmi_instance_t vmi, unsigned long vcpu,
                                       uint64_t virtual_address,
                                       uint32_t error_code);

status_t trustleech_get_tsc_info(vmi_instance_t vmi, uint32_t *tsc_mode,
                                 uint64_t *elapsed_nsec, uint32_t *gtsc_khz,
                                 uint32_t *incarnation);

void *trustleech_read_page(vmi_instance_t vmi, addr_t page);

int trustleech_is_pv(vmi_instance_t vmi);

status_t trustleech_test(uint64_t domainid, const char *name,
                         uint64_t init_flags, vmi_init_data_t *init_data);

// pause & resume
status_t trustleech_pause_vm(vmi_instance_t vmi);

status_t trustleech_resume_vm(vmi_instance_t vmi);

// registers
status_t trustleech_get_vcpureg(vmi_instance_t vmi, uint64_t *value, reg_t reg,
                                unsigned long vcpu);

status_t trustleech_get_vcpuregs(vmi_instance_t vmi, registers_t *regs,
                                 unsigned long vcpu);

status_t trustleech_set_vcpureg(vmi_instance_t vmi, uint64_t value, reg_t reg,
                                unsigned long vcpu);
status_t trustleech_set_vcpuregs(vmi_instance_t vmi, registers_t *registers,
                                 unsigned long vcpu);

// physical pages
status_t trustleech_alloc_gfn(vmi_instance_t vmi, uint64_t gfn);

status_t trustleech_free_gfn(vmi_instance_t vmi, uint64_t gfn);

status_t trustleech_set_intr_access(vmi_instance_t vmi,
                                    interrupt_event_t *event, bool enabled);

static inline status_t driver_trustleech_setup(vmi_instance_t vmi) {
  driver_interface_t driver = {0};
  driver.initialized = true;
  driver.init_ptr = &trustleech_init;
  driver.init_vmi_ptr = &trustleech_init_vmi;
  driver.destroy_ptr = &trustleech_destroy;
  driver.get_id_from_name_ptr = &trustleech_get_id_from_name;
  driver.get_name_from_id_ptr = &trustleech_get_name_from_id;
  driver.get_id_ptr = &trustleech_get_id;
  driver.set_id_ptr = &trustleech_set_id;
  driver.check_id_ptr = &trustleech_check_id;
  driver.get_name_ptr = &trustleech_get_name;
  driver.set_name_ptr = &trustleech_set_name;
  driver.write_ptr = &trustleech_write;
  driver.get_memsize_ptr = &trustleech_get_memsize;
  driver.request_page_fault_ptr = &trustleech_request_page_fault;
  driver.get_tsc_info_ptr = &trustleech_get_tsc_info;
  driver.get_vcpureg_ptr = &trustleech_get_vcpureg;
  driver.get_vcpuregs_ptr = &trustleech_get_vcpuregs;
  driver.set_vcpureg_ptr = &trustleech_set_vcpureg;
  driver.set_vcpuregs_ptr = &trustleech_set_vcpuregs;
  driver.read_page_ptr = &trustleech_read_page;
  driver.is_pv_ptr = &trustleech_is_pv;
  driver.pause_vm_ptr = &trustleech_pause_vm;
  driver.resume_vm_ptr = &trustleech_resume_vm;
  driver.get_next_available_gfn_ptr = &trustleech_get_next_available_gfn;
  driver.alloc_gfn_ptr = &trustleech_alloc_gfn;
  driver.free_gfn_ptr = &trustleech_free_gfn;
  vmi->driver = driver;
  return VMI_SUCCESS;
}

#define TRUSTLEECH_EVT_MSR 0
#define TRUSTLEECH_EVT_PF 1
#define TRUSTLEECH_EVT_BP 2
#define TRUSTLEECH_EVT_SS 3
#define TRUSTLEECH_NUM_EVENTS 4

status_t trustleech_events_init(vmi_instance_t vmi, uint32_t init_flags,
                                vmi_init_data_t *init_data);

status_t trustleech_events_listen(vmi_instance_t vmi, uint32_t timeout);
#endif /* TRUSTLEECH_H */
