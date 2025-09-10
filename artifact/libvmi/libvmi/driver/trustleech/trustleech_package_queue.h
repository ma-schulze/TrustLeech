#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

// Define the tl_comm_packet structure
struct tl_comm_packet {
    void *buffer;
    uint64_t len;
};

// Node structure for the queue
struct QueueNode {
    struct tl_comm_packet data;
    struct QueueNode* next;
};

// Queue structure
struct Queue {
    struct QueueNode* front;
    struct QueueNode* rear;
};

// Function to create a new queue
struct Queue* createQueue();

// Function to enqueue data into the queue
void enqueue(struct Queue* queue, struct tl_comm_packet data);

// Function to dequeue data from the queue
struct tl_comm_packet dequeue(struct Queue* queue);

// Function to check if queue is empty
int isQueueEmpty(struct Queue* queue);

// Function to destroy the queue and free all allocated memory
void destroyQueue(struct Queue* queue);

#endif // QUEUE_H
