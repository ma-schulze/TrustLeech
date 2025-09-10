#include "vmi.hpp"
#include "drivers/network.hpp"
#include "log.hpp"
#include "spinlock.hpp"

#include <libc.hpp>

namespace vmi
{

static volatile bool in_network = false;
static uint8_t buffer[4096] = { 0 }; // maximum response size
static bool paused = false;
static uint64_t nonce = 0;

void set_vmi_paused(bool pause)
{
	paused = pause;
}

void receive_and_handle_command_with_response(tl_response inital_response)
{
	if (in_network) {
		return;
	}
	in_network = true;

	LOG_INFO("starting stuff!");
	network_send_packet(get_pci_dev(), (u8 *)&inital_response,
			    sizeof(inital_response));

	while (true) {
		tl_response response = { CONTINUE, 0 };
		u8 rec_buffer[sizeof(tl_packet)] = { 0 };
		memset(buffer, 0, 4096);

		LOG_INFO("receiving stuff!");
		usize rec_len = network_receive_packet(
			get_pci_dev(), rec_buffer, sizeof(tl_packet));

		if (rec_len == 0 && !paused) {
			in_network = false;
			return;
		}

		tl_packet *packet = (tl_packet *)rec_buffer;
		if (nonce != packet->nonce) {
          	LOG_INFO("Invalid nonce!");
			in_network = false;
			return;
		}
        nonce++;

		LOG_DEBUG("Received command %d\n", packet->tl_cmd);

		switch (packet->tl_cmd) {
		case INIT_VMI:
			initVmiHandler(&response, packet, buffer);
			paused = true;
			break;
		case GET_ID_FROM_NAME:
			getIdFromNameHandler(&response, packet, buffer);
			break;
		case GET_NAME_FROM_ID:
			getNameFromIdHandler(&response, packet, buffer);
			break;
		case GET_ID:
			getIdHandler(&response, packet, buffer);
			break;
		case SET_ID:
			setIdHandler(&response, packet, buffer);
			break;
		case CHECK_ID:
			checkIdHandler(&response, packet, buffer);
			break;
		case GET_NAME:
			getNameHandler(&response, packet, buffer);
			break;
		case SET_NAME:
			setNameHandler(&response, packet, buffer);
			break;
		case WRITE:
			writeHandler(&response, packet, buffer);
			break;
		case GET_MEMSIZE:
			getMemSizeHandler(&response, packet, buffer);
			break;
		case REQUEST_PAGE_FAULT:
			requestPageFaultHandler(&response, packet, buffer);
			break;
		case GET_TSC_INFO:
			getTscInfoHandler(&response, packet, buffer);
			break;
		case GET_VCPUREG:
			getVcpuRegHandler(&response, packet, buffer);
			break;
		case GET_VCPUREGS:
			getVcpuRegsHandler(&response, packet, buffer);
			break;
		case SET_VCPUREG:
			setVcpuRegHandler(&response, packet, buffer);
			break;
		case SET_VCPUREGS:
			setVcpuRegsHandler(&response, packet, buffer);
			break;
		case READ_PAGE:
			readPageHandler(&response, packet, buffer);
			break;
		case IS_PV:
			isPvHandler(&response, packet, buffer);
			break;
		case PAUSE_VM:
			pauseVmHandler(&response, packet, buffer);
			paused = true;
			break;
		case RESUME_VM:
			resumeVmHandler(&response, packet, buffer);
			paused = false;
			break;
		case GET_NEXT_AVAILALE_GFN:
			getNextAvailableGfnHandler(&response, packet, buffer);
			break;
		case ALLOC_GFN:
			allocGfnHandler(&response, packet, buffer);
			break;
		case FREE_GFN:
			freeGfnHandler(&response, packet, buffer);
			break;
		case CONTINUE: {
			if (!paused) {
				return;
			}
			break;
		};
		default:
			LOG_ERROR("Invalid TL VMI CMD!");
			continue;
			break;
		}

		uint64_t payload_size = response.payload_size;

		uint64_t len = network_send_packet(
			get_pci_dev(), (uint8_t *)&response, sizeof(response));
		LOG_INFO("Sent %lld bytes", len);

		if (payload_size > 0) {
			network_send_packet(get_pci_dev(), buffer,
					    payload_size);
			LOG_INFO("Sent %lld bytes", payload_size);
		}

		if (!paused) {
			break;
		}
	}

	in_network = false;
}

void receive_and_handle_command()
{
	receive_and_handle_command_with_response({ PING, 0 });
}

}
