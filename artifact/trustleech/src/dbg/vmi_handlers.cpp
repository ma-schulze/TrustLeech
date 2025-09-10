
#include "arm-trusted-firmware/plat/qemu/trustleech_exp.h"
#include "dbg/dbg.hpp"
#include "dbg/vmi_mm.hpp"
#include "drivers/network.hpp"
#include "exception/types.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "memory_backed_regs.hpp"
#include "registers.hpp"
#include "utility.hpp"
#include "vcpu_ctx.hpp"
#include "vmi.hpp"
#include "vmi_registers.hpp"
#include "utils.hpp"
#include "libc.hpp"
#include <types.hpp>

namespace vmi
{

// Handler function declarations
void initVmiHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)network_receive_packet(get_pci_dev(), buffer,
				     packet->payload_size);
	response->status = CONTINUE;
	response->payload_size = 0;
}
void getIdFromNameHandler(tl_response *response, tl_packet *packet,
			  uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void getNameFromIdHandler(tl_response *response, tl_packet *packet,
			  uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void getIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void setIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void checkIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void getNameHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void setNameHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void writeHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)buffer;

	uint64_t pz = packet->payload_size;
	uint64_t write_len = pz - 8; //payload_size - addr - len

	uint64_t inner_buff[4096 / 8];

	uint64_t rec_len = network_receive_packet(
		get_pci_dev(), (uint8_t *)inner_buff, sizeof(inner_buff));

	if (rec_len < 16) {
		return;
	}

	uint64_t pa = inner_buff[0];

	memcpy(&pa, (void *)&inner_buff[1], write_len);

	response->payload_size = 0;
	response->status = CONTINUE;
}
void getMemSizeHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer)
{
	(void)packet;
	response->status = CONTINUE;
	response->payload_size = sizeof(uint64_t) * 2;

	uint64_t max_mem = TRUSTLEECH_PLAT_RAM_LIMIT;
	uint64_t min_mem = GiB(1);

	uint64_t resp[] = { max_mem - min_mem, max_mem };
	memcpy(buffer, (char *)resp, sizeof(resp));
}
void requestPageFaultHandler(tl_response *response, tl_packet *packet,
			     uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void getTscInfoHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not supported!\n");
}
void getVcpuRegHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer)
{
	(void)packet;

	struct {
		uint64_t reg;
		uint64_t vcpu;
	} get_reg_req;
	uint64_t rec_len = network_receive_packet(
		get_pci_dev(), (uint8_t *)&get_reg_req, sizeof(get_reg_req));

	if (rec_len != sizeof(get_reg_req)) {
		return;
	}

	uint64_t reg_val;

	// TODO!
	switch (get_reg_req.reg) {
	case 77: // SCTLR
		reg_val = read_sctlr_el1().v;
		break;
	case 78: // CPSR
		reg_val = read_spsr_el2().v;
		break;
	case 79: // TCR
		reg_val = read_tcr_el1().v;
		break;
	case 80: // TTBR0
		reg_val = read_ttbr0_el1().v;
		break;
	case 81: // TTBR1
		reg_val = read_ttbr1_el1().v;
		break;
	default:
		LOG_INFO("Reg 0x%llx Not implemented yet!", get_reg_req.reg);
		break;
	}

	// if (get_reg_req.reg < 32) {
	// 	// TODO
	// } else if (get_reg_req.reg - 32 <
	// 	   sizeof(memory_backed_regs::nv2_redir_area)) {
	// 	auto *redir_area = vcpu().memory_backend.get_nv2_redir_area();
	// 	reg_val = (*redir_area)[static_cast<size_t>(get_reg_req.reg) /
	// 				sizeof(uint64_t)];
	// } else {
	// 	uint64_t reg_idx = get_reg_req.reg -
	// 			   sizeof(memory_backed_regs::nv2_redir_area) -
	// 			   32;
	// 	reg_val = get_register_by_index(reg_idx);
	// }
	response->status = CONTINUE;
	response->payload_size = sizeof(reg_val);

	LOG_INFO("Val of 0x%llx is 0x%llx\n", get_reg_req.reg, reg_val);

	memcpy(buffer, (char *)&reg_val, sizeof(reg_val));
}
void getVcpuRegsHandler(tl_response *response, tl_packet *packet,
			uint8_t *buffer)
{
	(void)packet;

	uint64_t requested_vcpu;
	uint64_t rec_len = network_receive_packet(get_pci_dev(),
						  (uint8_t *)&requested_vcpu,
						  sizeof(requested_vcpu));

	if (rec_len != 8) {
		return;
	}

	uint64_t redir_size =
		sizeof(*vcpu().memory_backend.get_nv2_redir_area());
	uint64_t dbg_size = sizeof(vmi_debug_registers);

	vmi_debug_registers dbg_regs;
	fill_debug_registers(&dbg_regs);

	uint64_t gpregs[32] = { 0 }; // TODO READ VALUES

	response->status = CONTINUE;
	response->payload_size = redir_size + dbg_size;
	memcpy(buffer, gpregs, sizeof(gpregs));
	memcpy(buffer + sizeof(gpregs),
	       (char *)vcpu().memory_backend.get_nv2_redir_area(), redir_size);
	memcpy(buffer + sizeof(gpregs) + redir_size, (char *)&dbg_regs,
	       dbg_size);
}
void setVcpuRegHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer)
{
	(void)packet;
	(void)buffer;

	struct {
		uint64_t reg;
		uint64_t val;
		uint64_t vcpu;
	} set_reg_req;
	uint64_t rec_len = network_receive_packet(
		get_pci_dev(), (uint8_t *)&set_reg_req, sizeof(set_reg_req));

	if (rec_len != sizeof(set_reg_req)) {
		return;
	}

	// TODO!
	switch (set_reg_req.reg) {
	case 200: // DBGBCR0
		write_dbgbvr0_el1({ set_reg_req.val });
		dbg::set_dbgbvr0_el1(set_reg_req.val);
		break;
	case 201: // DBGBCR0
		write_dbgbcr0_el1({ set_reg_req.val });
		dbg::set_dbgbcr0_el1(set_reg_req.val);
		break;
	default:
		LOG_INFO("Reg 0x%llx Not implemented yet!", set_reg_req.reg);
		break;
	}

	response->status = CONTINUE;
	response->payload_size = 0;
}

void setVcpuRegsHandler(tl_response *response, tl_packet *packet,
			uint8_t *buffer)
{
	(void)packet;
	(void)buffer;

	struct {
		uint64_t gpregs[32];
		memory_backed_regs::nv2_redir_area nv2_redir;
		vmi_debug_registers dbg;
	} __attribute__((packed)) set_regs_req;

	uint64_t rec_len = network_receive_packet(
		get_pci_dev(), (uint8_t *)&set_regs_req, sizeof(set_regs_req));

	if (rec_len != sizeof(set_regs_req)) {
		return;
	}

	auto *redir_area = vcpu().memory_backend.get_nv2_redir_area();
	memcpy(redir_area, set_regs_req.nv2_redir,
	       sizeof(memory_backed_regs::nv2_redir_area));
	fill_debug_registers(&set_regs_req.dbg);

	response->status = CONTINUE;
	response->payload_size = 0;
}
void readPageHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)packet;

	uint64_t requested_va;
	uint64_t rec_len = network_receive_packet(
		get_pci_dev(), (uint8_t *)&requested_va, sizeof(requested_va));

	if (rec_len != 8) {
		return;
	}

	response->status = CONTINUE;
	response->payload_size = 4096;

	// const auto ttbr0 = read_ttbr0_el1();
	// const auto ttbr1 = read_ttbr1_el1();
	// const auto tcr = read_tcr_el1();
	//
	// write_ttbr0_el1(vcpu().ttbr0_el2());
	// write_ttbr1_el1(vcpu().ttbr1_el2());
	// write_tcr_el1(xlate::translate_tcr_el2(vcpu().tcr_el2()));
	// isb();

	uint64_t requested_pa = dbg::translate_el1s1(requested_va);

	// write_ttbr0_el1(ttbr0);
	// write_ttbr1_el1(ttbr1);
	// write_tcr_el1(tcr);
	// isb();
	if (!requested_pa) {
		// try direct access, needed for direct PA
		requested_pa = dbg::translate_el2s1(requested_va);
	}

	if (!requested_pa) {
		LOG_ERROR("Failed translation for va 0x%llx\n", requested_va);
		response->status = ERROR;
		return;
	}

	memcpy(buffer, (char *)requested_pa, 4096);
}
void isPvHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
	LOG_ERROR("Not implemented and not supported by ARM!\n");
}
void pauseVmHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
}
void resumeVmHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)response;
	(void)packet;
	(void)buffer;
}
void getNextAvailableGfnHandler(tl_response *response, tl_packet *packet,
				uint8_t *buffer)
{
	(void)packet;

	void *page = allocator.GetNextFreeBlock();
	uint64_t gfn = (uint64_t)page >> 12;

	response->status = CONTINUE;
	response->payload_size = sizeof(page);

	memcpy(buffer, (char *)gfn, sizeof(gfn));
}

void allocGfnHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)packet;

	// We ignore the given GFN and just allocate a new one, as it should lead to the same outcome if executed in order
	uint64_t requested_gfn = 0;
	uint64_t rec_len = network_receive_packet(get_pci_dev(),
						  (uint8_t *)&requested_gfn,
						  sizeof(requested_gfn));
	if (requested_gfn == 0 || rec_len != 8) {
		return;
	}
	void *page = allocator.AllocatePage();
	uint64_t gfn = (uint64_t)page >> 12;

	response->status = CONTINUE;
	response->payload_size = sizeof(page);

	memcpy(buffer, (char *)gfn, sizeof(gfn));
}

void freeGfnHandler(tl_response *response, tl_packet *packet, uint8_t *buffer)
{
	(void)buffer;
	(void)packet;

	uint64_t requested_gfn = 0;
	uint64_t rec_len = network_receive_packet(get_pci_dev(),
						  (uint8_t *)&requested_gfn,
						  sizeof(requested_gfn));
	if (requested_gfn == 0 || rec_len != 8) {
		return;
	}
	allocator.Free((void *)(requested_gfn << 12));

	response->status = CONTINUE;
	response->payload_size = 0;
}

}
