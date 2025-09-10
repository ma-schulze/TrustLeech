#pragma once

#include "log.hpp"

typedef unsigned char u8;
typedef unsigned long long usize;

extern "C" {

struct DeviceWrapper; // Forward declare as opaque struct

DeviceWrapper *setup_pci_tcp_handle();
bool network_is_setup();
void destroy_pci_device(DeviceWrapper*);

DeviceWrapper *get_pci_dev();
void set_pci_dev(DeviceWrapper*);

void network_send_and_receive();

usize network_send_packet(DeviceWrapper *pci_dev, u8 *buffer, usize length);
usize network_receive_packet(DeviceWrapper *pci_dev, u8 *buffer, usize length);

}

inline void setup_network() {
  DeviceWrapper *pci_dev = setup_pci_tcp_handle();
  if(!pci_dev) {
    LOG_ERROR("Could not setup network!");
  }
  set_pci_dev(pci_dev);
  if(!network_is_setup()) {
    LOG_ERROR("Coudl not setup network");
  }
}
