#pragma once

#include <types.hpp>

namespace vmi
{

typedef enum {
	INIT_VMI = 1,
	GET_ID_FROM_NAME,
	GET_NAME_FROM_ID,
	GET_ID,
	SET_ID,
	CHECK_ID,
	GET_NAME,
	SET_NAME,
	WRITE,
	GET_MEMSIZE,
	REQUEST_PAGE_FAULT,
	GET_TSC_INFO,
	GET_VCPUREG,
	GET_VCPUREGS,
	SET_VCPUREG,
	SET_VCPUREGS,
	READ_PAGE,
	IS_PV,
	PAUSE_VM,
	RESUME_VM,
	GET_NEXT_AVAILALE_GFN,
	ALLOC_GFN,
	FREE_GFN,
	CONTINUE,
	PING,
	ERROR,
    EVT_MSR,
    EVT_PF,
    EVT_BREAKPOINT,
	EVT_STEPPING,
} TL_CMD;

struct tl_comm_packet {
	void *buffer;
	uint64_t len;
};

struct tl_packet {
    uint64_t nonce;
	uint8_t tl_cmd;
	uint64_t payload_size;
} __attribute__((packed));

typedef enum {
	OK = 0,
	ERR = 1,
} TL_STATUS;

struct tl_response {
	uint8_t status;
	uint64_t payload_size;
} __attribute__((packed));

enum TL_STATE {
	READY,
	BUSY,
};

struct tl_response_packet {
	struct tl_packet packet;
	void *buffer;
};

void set_vmi_paused(bool pause);

// Handler function declarations
void initVmiHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void getIdFromNameHandler(tl_response *response, tl_packet *packet,
			  uint8_t *buffer);
void getNameFromIdHandler(tl_response *response, tl_packet *packet,
			  uint8_t *buffer);
void getIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void setIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void checkIdHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void getNameHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void setNameHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void writeHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void getMemSizeHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer);
void requestPageFaultHandler(tl_response *response, tl_packet *packet,
			     uint8_t *buffer);
void getTscInfoHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer);
void getVcpuRegHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer);
void getVcpuRegsHandler(tl_response *response, tl_packet *packet,
			uint8_t *buffer);
void setVcpuRegHandler(tl_response *response, tl_packet *packet,
		       uint8_t *buffer);
void setVcpuRegsHandler(tl_response *response, tl_packet *packet,
			uint8_t *buffer);
void readPageHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void isPvHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void pauseVmHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void resumeVmHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void getNextAvailableGfnHandler(tl_response *response, tl_packet *packet,
				uint8_t *buffer);
void allocGfnHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);
void freeGfnHandler(tl_response *response, tl_packet *packet, uint8_t *buffer);

void receive_and_handle_command();
void receive_and_handle_command_with_response(tl_response inital_response);

}
