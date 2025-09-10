#include "network.hpp"

static DeviceWrapper *pci_dev;


DeviceWrapper *get_pci_dev() {
  return pci_dev;
}
void set_pci_dev(DeviceWrapper* dev) {
  pci_dev = dev;
}
