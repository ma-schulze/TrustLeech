#pragma once

#include "log.hpp"
#include <types.hpp>
#include <registers.hpp>

namespace vmi
{

// Define a struct to hold the first four of each register type
typedef struct {
	uint64_t dbgbcr0_el1;
	uint64_t dbgbcr1_el1;
	uint64_t dbgbcr2_el1;
	uint64_t dbgbcr3_el1;
	uint64_t dbgbvr0_el1;
	uint64_t dbgbvr1_el1;
	uint64_t dbgbvr2_el1;
	uint64_t dbgbvr3_el1;
} vmi_debug_registers;

// Function to fill the struct with register values
inline void fill_debug_registers(vmi_debug_registers *regs)
{
	regs->dbgbcr0_el1 = read_dbgbcr0_el1().v;
	regs->dbgbcr1_el1 = read_dbgbcr1_el1().v;
	regs->dbgbcr2_el1 = read_dbgbcr2_el1().v;
	regs->dbgbcr3_el1 = read_dbgbcr3_el1().v;
	regs->dbgbvr0_el1 = read_dbgbvr0_el1().v;
	regs->dbgbvr1_el1 = read_dbgbvr1_el1().v;
	regs->dbgbvr2_el1 = read_dbgbvr2_el1().v;
	regs->dbgbvr3_el1 = read_dbgbvr3_el1().v;
}

inline void write_debug_registers(const vmi_debug_registers *regs)
{
	write_dbgbcr0_el1({ regs->dbgbcr0_el1 });
	write_dbgbcr1_el1({ regs->dbgbcr1_el1 });
	write_dbgbcr2_el1({ regs->dbgbcr2_el1 });
	write_dbgbcr3_el1({ regs->dbgbcr3_el1 });
	write_dbgbvr0_el1({ regs->dbgbvr0_el1 });
	write_dbgbvr1_el1({ regs->dbgbvr1_el1 });
	write_dbgbvr2_el1({ regs->dbgbvr2_el1 });
	write_dbgbvr3_el1({ regs->dbgbvr3_el1 });
}

// Function to get a register value by index
uint64_t inline get_register_by_index(int index)
{
	switch (index) {
	case 0:
		return read_dbgbcr0_el1().v;
	case 1:
		return read_dbgbcr1_el1().v;
	case 2:
		return read_dbgbcr2_el1().v;
	case 3:
		return read_dbgbcr3_el1().v;
	case 4:
		return read_dbgbvr0_el1().v;
	case 5:
		return read_dbgbvr1_el1().v;
	case 6:
		return read_dbgbvr2_el1().v;
	case 7:
		return read_dbgbvr3_el1().v;
	default:
		LOG_ERROR("Invalid register index\n");
		return 0; // or handle as needed
	}
}

// Function to set a register value by index
void inline set_register_by_index(int index, uint64_t value)
{
	switch (index) {
	case 0:
		write_dbgbcr0_el1({ value });
		break;
	case 1:
		write_dbgbcr1_el1({ value });
		break;
	case 2:
		write_dbgbcr2_el1({ value });
		break;
	case 3:
		write_dbgbcr3_el1({ value });
		break;
	case 4:
		write_dbgbvr0_el1({ value });
		break;
	case 5:
		write_dbgbvr1_el1({ value });
		break;
	case 6:
		write_dbgbvr2_el1({ value });
		break;
	case 7:
		write_dbgbvr3_el1({ value });
		break;
		LOG_ERROR("Invalid register index\n");
	}
}

}
