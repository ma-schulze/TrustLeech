#include "driver/memory_cache.h"
#include "msr-index.h"
#include "private.h"
#include <asm-generic/socket.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "aes.h"
#include "trustleech.h"
#include "trustleech_package_queue.h"

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
  TRUSTLEECH_EVT_BASE,
} TL_CMD;

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

static int sock;
static int nonce;

static atomic_bool vmi_inited = false;

static struct Queue *package_queue = NULL;
static status_t (*tl_process_event[TRUSTLEECH_NUM_EVENTS])(
    vmi_instance_t vmi, struct tl_response_packet *event);

static int aes_xcrypt_ctr(const char *xcrypt, uint8_t *msg, size_t len) {
  // Define the key array (32 bytes for AES-256)
  uint8_t key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  // Define the IV (nonce) array (16 bytes)
  uint8_t iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  struct AES_ctx ctx;

  AES_init_ctx_iv(&ctx, key, iv);
  AES_CTR_xcrypt_buffer(&ctx, msg, len);

  // printf("CTR %s: ", xcrypt);
  return 0;
}

static volatile struct tl_response_packet last_response;
static bool paused = false;

static void trustleech_send(uint8_t *buffer, uint64_t length) {
  struct tl_comm_packet packet = {buffer, length};
  enqueue(package_queue, packet);
}

static void trustleech_recv(struct tl_response_packet *response) {
  while (last_response.packet.tl_cmd == 0) {
  }
  // printf("Used cmd\n");
  *response = last_response;
  last_response.packet.tl_cmd = 0;
}

ssize_t read_n_bytes(int sock, void *buf, size_t n) {
  size_t total_bytes_read = 0;
  char *buffer = (char *)buf;

  while (total_bytes_read < n) {
    ssize_t bytes_read =
        read(sock, buffer + total_bytes_read, n - total_bytes_read);

    if (bytes_read < 0) {
      if (errno == EINTR) {
        continue; // Interrupted by signal, try again
      } else if (errno == EWOULDBLOCK) {
        return EWOULDBLOCK;
      }
      perror("read");
      return -1; // Error occurred
    }
    total_bytes_read += bytes_read;
  }

  aes_xcrypt_ctr("Decrypt", (uint8_t *)buf, total_bytes_read);
  return total_bytes_read; // Return the total number of bytes read
}

enum TL_STATE state = READY;

static void *trustleech_comm(void *arg) {

  struct tl_packet continue_packet = {CONTINUE, 0};
  aes_xcrypt_ctr("Encrypt", (uint8_t *)&continue_packet,
                 sizeof(continue_packet));
  struct tl_response response;

  while (!atomic_load(&vmi_inited)) {
  }

  while (1) {

    struct tl_comm_packet *data = NULL;

    while (state != BUSY) {
      int rb = read_n_bytes(sock, &response, sizeof(response));
      // printf("Recieved package of size %d with state %d\n", rb,
      //  response.status);
      if (response.status >= TRUSTLEECH_EVT_BASE) {
        paused = true;
        state = BUSY;
        last_response.packet.payload_size = response.payload_size;
        last_response.packet.tl_cmd = response.status;
      } else {
        state = response.status == PING ? BUSY : READY;
      }
    }

    if (isQueueEmpty(package_queue)) {
      if (!paused) {
        struct tl_packet packet = {nonce++, RESUME_VM, 0};

        // printf("Sending command %d\n", packet.tl_cmd);
        aes_xcrypt_ctr("Encrypt", (uint8_t *)&packet, sizeof(struct tl_packet));
        send(sock, (uint8_t *)&packet, sizeof(packet), 0);

        int rb = read_n_bytes(sock, &response, sizeof(response));
        // printf("Recieved package of size %d with state %d\n", rb,
        //  response.status);
        if (response.status == CONTINUE) {
          state = READY;
        }
      }
      continue;
    } else {
      data = &package_queue->front->data;
      // printf("Sending msg 0x%lx to TL server len 0x%lx!!\n",
      //         *(uint64_t *)data->buffer, data->len);
      send(sock, data->buffer, data->len, 0);
      state = BUSY;
    }

    if (read_n_bytes(sock, &response, sizeof(response)) == EWOULDBLOCK) {
      continue;
    }
    if (response.status == PING) {
      continue;
    }

    // printf("Generating cmd!\n");
    if (response.payload_size > 0) {
      void *buff = malloc(response.payload_size);
      read_n_bytes(sock, buff, response.payload_size);
      last_response.buffer = buff;
    } else {
      last_response.buffer = NULL;
    }

    last_response.packet.payload_size = response.payload_size;
    last_response.packet.tl_cmd = response.status;

    // if (response.status == 0) {
    ////printf("WTF?!?!?!?!??\n");
    // }
    response.status = 0;
    response.payload_size = 0;

    dequeue(package_queue);

    while (last_response.packet.tl_cmd != 0) {
    }
  }
}

status_t trustleech_init(vmi_instance_t vmi, uint32_t init_flags,
                         vmi_init_data_t *init_data) {

  const char *SERVER_IP = "192.168.1.6"; // Replace with your server's IP
  const int SERVER_PORT = 5555;          // Replace with the server's port

  // Create a socket
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) {
  //printf("Could not create socket!\n");
    return 1;
  }

  // struct timeval timeout;
  // timeout.tv_sec = 5;
  // timeout.tv_usec = 0;
  // setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,  &timeout, sizeof timeout);
  // setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,  &timeout, sizeof timeout);

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(SERVER_PORT);

  // Convert IP from text to binary form
  if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0) {
  //printf("Invalid address/ Address not supported");
    return 1;
  }

  // Connect to the server
  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
  //printf("Connection failed");
    return 1;
  }

  package_queue = createQueue();

  pthread_t thread;
  int thread_id =
      1; // Example ID for the thread; you can pass more complex data if needed

  // Create a new thread that will execute 'task_function'
  if (pthread_create(&thread, NULL, trustleech_comm, (void *)&thread_id) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
}

status_t trustleech_init_vmi(vmi_instance_t vmi, uint32_t init_flags,
                             vmi_init_data_t *init_data) {

  size_t init_data_size = 0;
  if (init_data) {
    size_t init_data_size = init_data->count * sizeof(vmi_init_data_entry_t);
  }

  size_t buff_len =
      sizeof(init_flags) + init_data_size + sizeof(struct tl_packet);

  uint8_t *buffer = malloc(buff_len);

  paused = true;
  struct tl_packet packet = {nonce++, INIT_VMI, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));
  memcpy(buffer + sizeof(struct tl_packet), &init_flags, sizeof(init_flags));
  if (init_data) {
    memcpy(buffer + sizeof(struct tl_packet) + sizeof(init_flags), init_data,
           init_data_size);
  }
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 sizeof(init_flags) + init_data_size);

  trustleech_send(buffer, buff_len);
  atomic_store(&vmi_inited, true);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  if (init_flags & VMI_INIT_EVENTS) {
    trustleech_events_init(vmi, init_flags, init_data);
  }

  free(buffer);
  return 0;
}

void trustleech_destroy(vmi_instance_t vmi) {
  // Close the socket
  close(sock);
  if (package_queue) {
    destroyQueue(package_queue);
  }
}
uint64_t trustleech_get_id_from_name(vmi_instance_t vmi, const char *name) {
  return VMI_SUCCESS;
}

status_t trustleech_get_name_from_id(vmi_instance_t vmi, uint64_t domainid,
                                     char **name) {

  const char internal_name[] = "trustleech";

  *name = malloc(sizeof(internal_name));
  memcpy(*name, internal_name, sizeof(internal_name));

  return VMI_SUCCESS;
}

uint64_t trustleech_get_id(vmi_instance_t vmi) { return VMI_SUCCESS; }

void trustleech_set_id(vmi_instance_t vmi, uint64_t domainid) { return; }

status_t trustleech_check_id(vmi_instance_t vmi, uint64_t domainid) {
  return VMI_SUCCESS;
}

status_t trustleech_write(vmi_instance_t vmi, addr_t paddr, void *buf,
                          uint32_t length) {
  size_t buff_len = sizeof(paddr) + sizeof(struct tl_packet) + length;
  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, WRITE, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &paddr, sizeof(paddr));
  memcpy(buffer + sizeof(struct tl_packet) + sizeof(paddr), buf, length);
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 sizeof(paddr) + length);

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);
  return VMI_SUCCESS;
}

status_t trustleech_get_name(vmi_instance_t vmi, char **name) {
  *name = calloc(1, sizeof("trustleech") + 1);
  memcpy(*name, "trustleech", sizeof("trustleech"));
  return VMI_SUCCESS;
}

void trustleech_set_name(vmi_instance_t vmi, const char *name) { return; }

status_t trustleech_get_memsize(vmi_instance_t vmi, uint64_t *allocate_ram_size,
                                addr_t *maximum_physical_address) {
  struct tl_packet packet = {nonce++, GET_MEMSIZE, 0};

//printf("Sending command %d\n", packet.tl_cmd);
  aes_xcrypt_ctr("Encrypt", (uint8_t *)&packet, sizeof(struct tl_packet));
  trustleech_send((uint8_t *)&packet, sizeof(packet));

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  if (response.buffer) {
    *allocate_ram_size = *(addr_t *)(response.buffer);
    *maximum_physical_address = *(((addr_t *)response.buffer) + 1);
    free(response.buffer);
    response.buffer = 0;
  }

  return VMI_SUCCESS;
}

status_t trustleech_get_next_available_gfn(vmi_instance_t vmi,
                                           addr_t *next_gfn) {
  struct tl_packet packet = {nonce++, GET_NEXT_AVAILALE_GFN, 0};

//printf("Sending command %d\n", packet.tl_cmd);
  aes_xcrypt_ctr("Encrypt", (uint8_t *)&packet, sizeof(struct tl_packet));
  trustleech_send((uint8_t *)&packet, sizeof(packet));

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  if (response.buffer) {
    *next_gfn = *(addr_t *)(response.buffer);

    free(response.buffer);
    response.buffer = 0;
  }
  return VMI_SUCCESS;
}

status_t trustleech_request_page_fault(vmi_instance_t vmi, unsigned long vcpu,
                                       uint64_t virtual_address,
                                       uint32_t error_code) {
//printf("Request Page Fault not supported with TrustLeech!\n");
  return VMI_FAILURE;
}

status_t trustleech_get_tsc_info(vmi_instance_t vmi, uint32_t *tsc_mode,
                                 uint64_t *elapsed_nsec, uint32_t *gtsc_khz,
                                 uint32_t *incarnation) {
//printf("Get TSC Info not supported on TrustLeech!\n");
  return VMI_FAILURE;
}

void *trustleech_read_page(vmi_instance_t vmi, addr_t page) {

//printf("Reading page 0x%llx\n", page);

  page <<= (12);
  size_t buff_len = sizeof(page) + sizeof(struct tl_packet);

  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, READ_PAGE, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &page, sizeof(page));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 sizeof(page));

  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);
  clock_gettime(CLOCK_MONOTONIC, &end);

  double time_taken =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

//printf("Function took %f nano seconds to execute.\n", time_taken);
  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  if (!response.buffer) {
    return NULL;
  }

  if (response.packet.payload_size != 4096) {
    free(response.buffer);
    return NULL;
  }

  char *page_buff = malloc(4096);
  memcpy(page_buff, response.buffer, 4096);
  free(response.buffer);
  free(buffer);
  return page_buff;
}

int trustleech_is_pv(vmi_instance_t vmi) {
//printf("Not supported on TrustLeech!\n");
  return VMI_FAILURE;
}

status_t trustleech_test(uint64_t domainid, const char *name,
                         uint64_t init_flags, vmi_init_data_t *init_data) {
  return VMI_SUCCESS;
}

// pause & resume
status_t trustleech_pause_vm(vmi_instance_t vmi) {
  struct tl_packet packet = {nonce++, PAUSE_VM, 0};

//printf("Sending command %d\n", packet.tl_cmd);
  aes_xcrypt_ctr("Encrypt", (uint8_t *)&packet, sizeof(struct tl_packet));
  trustleech_send((uint8_t *)&packet, sizeof(packet));

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  return VMI_SUCCESS;
}

status_t trustleech_resume_vm(vmi_instance_t vmi) {
  struct tl_packet packet = {nonce++, RESUME_VM, 0};

//printf("Sending command %d\n", packet.tl_cmd);
  aes_xcrypt_ctr("Encrypt", (uint8_t *)&packet, sizeof(struct tl_packet));
  trustleech_send((uint8_t *)&packet, sizeof(packet));

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  paused = false;
  state = READY;
  return VMI_SUCCESS;
}

static reg_t to_tl_reg(reg_t reg) {
  switch (reg) {
  case SCTLR:
    reg = 66;
    break;
  case TTBCR:
    reg = 68;
    break;
  case TTBR0:
    reg = 96;
    break;
  case TTBR1:
    reg = 98;
    break;
  case CPSR:
    reg = 76; // TODO this requires special care i guess...
    break;
  // case R0:
  //     reg = ctx.c.user_regs.r0_usr;
  //     break;
  // case R1:
  //     reg = ctx.c.user_regs.r1_usr;
  //     break;
  // case R2:
  //     reg = ctx.c.user_regs.r2_usr;
  //     break;
  // case R3:
  //     reg = ctx.c.user_regs.r3_usr;
  //     break;
  // case R4:
  //     reg = ctx.c.user_regs.r4_usr;
  //     break;
  // case R5:
  //     reg = ctx.c.user_regs.r5_usr;
  //     break;
  // case R6:
  //     reg = ctx.c.user_regs.r6_usr;
  //     break;
  // case R7:
  //     reg = ctx.c.user_regs.r7_usr;
  //     break;
  // case R8:
  //     reg = ctx.c.user_regs.r8_usr;
  //     break;
  // case R9:
  //     reg = ctx.c.user_regs.r9_usr;
  //     break;
  // case R10:
  //     reg = ctx.c.user_regs.r10_usr;
  //     break;
  // case R11:
  //     reg = ctx.c.user_regs.r11_usr;
  //     break;
  // case R12:
  //     reg = ctx.c.user_regs.r12_usr;
  //     break;
  // case SP_USR:
  //     reg = ctx.c.user_regs.sp_usr;
  //     break;
  // case LR_USR:
  //     reg = ctx.c.user_regs.lr_usr;
  //     break;
  // case LR_IRQ:
  //     reg = ctx.c.user_regs.lr_irq;
  //     break;
  // case SP_IRQ:
  //     reg = ctx.c.user_regs.sp_irq;
  //     break;
  // case LR_SVC:
  //     reg = ctx.c.user_regs.lr_svc;
  //     break;
  // case SP_SVC:
  //     reg = ctx.c.user_regs.sp_svc;
  //     break;
  // case LR_ABT:
  //     reg = ctx.c.user_regs.lr_abt;
  //     break;
  // case SP_ABT:
  //     reg = ctx.c.user_regs.sp_abt;
  //     break;
  // case LR_UND:
  //     reg = ctx.c.user_regs.lr_und;
  //     break;
  // case SP_UND:
  //     reg = ctx.c.user_regs.sp_und;
  //     break;
  // case R8_FIQ:
  //     reg = ctx.c.user_regs.r8_fiq;
  //     break;
  // case R9_FIQ:
  //     reg = ctx.c.user_regs.r9_fiq;
  //     break;
  // case R10_FIQ:
  //     reg = ctx.c.user_regs.r10_fiq;
  //     break;
  // case R11_FIQ:
  //     reg = ctx.c.user_regs.r11_fiq;
  //     break;
  // case R12_FIQ:
  //     reg = ctx.c.user_regs.r12_fiq;
  //     break;
  // case SP_FIQ:
  //     reg = ctx.c.user_regs.sp_fiq;
  //     break;
  // case LR_FIQ:
  //     reg = ctx.c.user_regs.lr_fiq;
  //     break;
  // case PC:
  //     reg = ctx.c.user_regs.pc32;
  //     break;
  // case SPSR_SVC:
  //     reg = ctx.c.user_regs.spsr_svc;
  //     break;
  // case SPSR_FIQ:
  //     reg = ctx.c.user_regs.spsr_fiq;
  //     break;
  // case SPSR_IRQ:
  //     reg = ctx.c.user_regs.spsr_irq;
  //     break;
  // case SPSR_UND:
  //     reg = ctx.c.user_regs.spsr_und;
  //     break;
  // case SPSR_ABT:
  //     reg = ctx.c.user_regs.spsr_abt;
  //     break;
  // case SP_EL0:
  //     reg = ctx.c.user_regs.sp_el0;
  //     break;
  // case SP_EL1:
  //     reg = ctx.c.user_regs.sp_el1;
  //     break;
  // case ELR_EL1:
  //     reg = ctx.c.user_regs.elr_el1;
  //     break;
  default:
    return 0;
  }
  return reg;
}

// registers
status_t trustleech_get_vcpureg(vmi_instance_t vmi, uint64_t *value, reg_t reg,
                                unsigned long vcpu) {

  if (reg < SCTLR) {
    return VMI_FAILURE;
  }

  // reg = to_tl_reg(reg);

//printf("Getting register 0x%lx\n", reg);

  size_t buff_len = sizeof(reg) + sizeof(vcpu) + sizeof(struct tl_packet);

  uint8_t *buffer = calloc(buff_len, 1);

  struct tl_packet packet = {nonce++, GET_VCPUREG, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &reg, sizeof(reg));
  memcpy(buffer + sizeof(struct tl_packet) + sizeof(reg), &vcpu, sizeof(vcpu));

  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 buff_len - sizeof(struct tl_packet));

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);

  if (response.buffer) {
    *value = *(uint64_t *)response.buffer;
    // free(response.buffer);
    response.buffer = 0;
  }
//printf("GOT CPU REG VAL 0x%llx\n", *value);

  return VMI_SUCCESS;
}

status_t trustleech_get_vcpuregs(vmi_instance_t vmi, registers_t *regs,
                                 unsigned long vcpu) {

  size_t buff_len = sizeof(vcpu) + sizeof(struct tl_packet);
  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, GET_VCPUREGS, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &vcpu, sizeof(vcpu));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 buff_len - sizeof(struct tl_packet));
  sizeof(vcpu);

  trustleech_send((uint8_t *)&packet, sizeof(packet));
  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }

  if (response.buffer) {
    memcpy(regs, response.buffer, sizeof(aarch64_registers_t));
    free(response.buffer);
    response.buffer = 0;
  }
  free(buffer);
}

status_t trustleech_set_vcpureg(vmi_instance_t vmi, uint64_t value, reg_t reg,
                                unsigned long vcpu) {
  if (reg < SCTLR) {
    return VMI_FAILURE;
  }

  size_t buff_len =
      sizeof(reg) + sizeof(vcpu) + sizeof(value) + sizeof(struct tl_packet);
  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, SET_VCPUREG, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &reg, sizeof(reg));
  memcpy(buffer + sizeof(struct tl_packet) + sizeof(reg), &value,
         sizeof(value));
  memcpy(buffer + sizeof(struct tl_packet) + sizeof(reg) + sizeof(value), &vcpu,
         sizeof(vcpu));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 buff_len - sizeof(struct tl_packet));

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);

  return VMI_SUCCESS;
}

status_t trustleech_set_vcpuregs(vmi_instance_t vmi, registers_t *registers,
                                 unsigned long vcpu) {
  size_t buff_len =
      sizeof(registers_t) + sizeof(vcpu) + sizeof(struct tl_packet);
  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, SET_VCPUREGS, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), registers, sizeof(*registers));
  memcpy(buffer + sizeof(struct tl_packet) + sizeof(*registers), &vcpu,
         sizeof(vcpu));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 buff_len - sizeof(struct tl_packet));

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);
}

// physical pages
status_t trustleech_alloc_gfn(vmi_instance_t vmi, uint64_t gfn) {

  size_t buff_len = sizeof(gfn) + sizeof(struct tl_packet);

  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, ALLOC_GFN, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &gfn, sizeof(gfn));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 sizeof(gfn));

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);

  if (response.buffer) {
    free(response.buffer);
    response.buffer = 0;
  }
  return VMI_SUCCESS;
}

status_t trustleech_free_gfn(vmi_instance_t vmi, uint64_t gfn) {
  size_t buff_len = sizeof(gfn) + sizeof(struct tl_packet);

  uint8_t *buffer = malloc(buff_len);

  struct tl_packet packet = {nonce++, FREE_GFN, buff_len - sizeof(struct tl_packet)};

//printf("Sending command %d\n", packet.tl_cmd);
  memcpy(buffer, &packet, sizeof(struct tl_packet));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer, sizeof(struct tl_packet));

  memcpy(buffer + sizeof(struct tl_packet), &gfn, sizeof(gfn));
  aes_xcrypt_ctr("Encrypt", (uint8_t *)buffer + sizeof(struct tl_packet),
                 sizeof(gfn));

  trustleech_send(buffer, buff_len);

  struct tl_response_packet response;
  uint64_t len;
  trustleech_recv(&response);

  if (response.packet.tl_cmd == CONTINUE) {
  //printf("Received valid response!\n");
  }
  free(buffer);

  if (response.buffer) {
    free(response.buffer);
    response.buffer = 0;
  }
  return VMI_SUCCESS;
}

static event_response_t call_event_callback(vmi_instance_t vmi,
                                            vmi_event_t *libvmi_event) {
  event_response_t response;
  vmi->event_callback = 1;
  response = libvmi_event->callback(vmi, libvmi_event);
  vmi->event_callback = 0;
  return response;
}

static status_t trustleech_process_bp(vmi_instance_t vmi,
                                      struct tl_response_packet *event) {

//printf("Got BP event FR!\n");

  if (event->packet.tl_cmd != TRUSTLEECH_EVT_BP + TRUSTLEECH_EVT_BASE) {
    return VMI_FAILURE;
  }

//printf("Got BP event FRFR!\n");
  // if (event->buffer) {
  //   uint64_t elr = *(uint64_t *)event->buffer;
  //   uint64_t elr_phys_page = *((uint64_t *)event->buffer + 1);
  // lookup vmi_event
  vmi_event_t *libvmi_event =
      g_hash_table_lookup(vmi->interrupt_events, GUINT_TO_POINTER(0));
  // libvmi_event->debug_event.gla = elr;
  // libvmi_event->debug_event.gfn = elr_phys_page;
  // libvmi_event->debug_event.offset = elr & VMI_BIT_MASK(0, 11);

  // call user callback
  event_response_t response = call_event_callback(vmi, libvmi_event);

  //   free(event->buffer);
  // }

  return VMI_SUCCESS;
}

static status_t trustleech_process_msr(vmi_instance_t vmi,
                                       struct tl_response_packet *event) {
  return VMI_SUCCESS;
}
static status_t trustleech_process_ss(vmi_instance_t vmi,
                                      struct tl_response_packet *event) {
  if (event->packet.tl_cmd != TRUSTLEECH_EVT_BP + TRUSTLEECH_EVT_BASE) {
    return VMI_FAILURE;
  }

  if (event->buffer) {
    uint64_t elr = *(uint64_t *)event->buffer;
    uint64_t elr_phys_page = *((uint64_t *)event->buffer + 1);

    // lookup vmi_event
    vmi_event_t *libvmi_event =
        g_hash_table_lookup(vmi->ss_events, GUINT_TO_POINTER(0));

    libvmi_event->ss_event.gla = elr;
    libvmi_event->ss_event.gfn = elr_phys_page;
    libvmi_event->ss_event.offset = elr & VMI_BIT_MASK(0, 11);

    // call user callback
    event_response_t response = call_event_callback(vmi, libvmi_event);

    free(event->buffer);
  }

  return VMI_SUCCESS;
}
static status_t trustleech_process_pf(vmi_instance_t vmi,
                                      struct tl_response_packet *event) {
  return VMI_SUCCESS;
}

status_t trustleech_events_init(vmi_instance_t vmi, uint32_t init_flags,
                                vmi_init_data_t *init_data) {

  (void)init_flags;
  (void)init_data;

  // bind driver functions
  vmi->driver.events_listen_ptr = &trustleech_events_listen;
  // vmi->driver.are_events_pending_ptr = &kvm_are_events_pending;
  // vmi->driver.set_reg_access_ptr = &kvm_set_reg_access;
  vmi->driver.set_intr_access_ptr = &trustleech_set_intr_access;
  // vmi->driver.set_mem_access_ptr = &kvm_set_mem_access;
  // vmi->driver.set_mem_access_range_ptr = &kvm_set_mem_access_range;
  // vmi->driver.set_desc_access_event_ptr = &kvm_set_desc_access_event;
  // vmi->driver.start_single_step_ptr = &kvm_start_single_step;
  // vmi->driver.stop_single_step_ptr = &kvm_stop_single_step;
  // vmi->driver.shutdown_single_step_ptr = &kvm_shutdown_single_step;
  // vmi->driver.set_cpuid_event_ptr = &kvm_set_cpuid_event;

  // fill event dispatcher
  tl_process_event[TRUSTLEECH_EVT_MSR] = &trustleech_process_msr;
  tl_process_event[TRUSTLEECH_EVT_PF] = &trustleech_process_pf;
  tl_process_event[TRUSTLEECH_EVT_SS] = &trustleech_process_ss;
  tl_process_event[TRUSTLEECH_EVT_BP] = &trustleech_process_bp;

  return VMI_SUCCESS;
}

static status_t process_single_event(vmi_instance_t vmi,
                                     struct tl_response_packet *event) {
  status_t status = VMI_SUCCESS;
  unsigned int ev_reason = 0;

  // handle event
  ev_reason = event->packet.tl_cmd - TRUSTLEECH_EVT_BASE;

//printf("Got DBG EVENT!\n");

  // call handler
  status = tl_process_event[ev_reason](vmi, event);

cleanup:
  return status;
}

static status_t process_pending_events(vmi_instance_t vmi) {
  struct tl_response_packet event;

  while (last_response.packet.tl_cmd != 0) {

    trustleech_recv(&event);
    process_single_event(vmi, &event);
  }

  return VMI_SUCCESS;
}

status_t trustleech_events_listen(vmi_instance_t vmi, uint32_t timeout) {

  process_pending_events(vmi);

  return VMI_SUCCESS;
}

status_t trustleech_set_intr_access(vmi_instance_t vmi,
                                    interrupt_event_t *event, bool enabled) {
  return VMI_SUCCESS;
}
