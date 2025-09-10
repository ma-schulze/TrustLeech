#include <stdio.h>
#include <stdlib.h>
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
struct Queue* createQueue() {
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->front = queue->rear = NULL;
    return queue;
}

// Function to enqueue data into the queue
void enqueue(struct Queue* queue, struct tl_comm_packet data) {
    struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    newNode->data = data;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
        return;
    }

    queue->rear->next = newNode;
    queue->rear = newNode;
}

// Function to dequeue data from the queue
struct tl_comm_packet dequeue(struct Queue* queue) {
    if (queue->front == NULL) {
        struct tl_comm_packet empty_packet = {NULL, 0};
        return empty_packet; // or handle underflow appropriately
    }

    struct QueueNode* tempNode = queue->front;
    struct tl_comm_packet data = tempNode->data;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(tempNode);
    return data;
}

// Function to check if queue is empty
int isQueueEmpty(struct Queue* queue) {
    return queue->front == NULL;
}

// Function to destroy the queue and free all allocated memory
void destroyQueue(struct Queue* queue) {
    while (!isQueueEmpty(queue)) {
        dequeue(queue);
    }
    free(queue);
}

int main() {
    // Example usage of the queue
    struct Queue* queue = createQueue();

    // Enqueue some packets
    struct tl_comm_packet packet1 = {(void*)"Packet1", 7};
    struct tl_comm_packet packet2 = {(void*)"Packet2", 7};

    enqueue(queue, packet1);
    enqueue(queue, packet2);

    // Dequeue and print packets
    while (!isQueueEmpty(queue)) {
        struct tl_comm_packet packet = dequeue(queue);
        if(packet.buffer) {
            printf("Dequeued packet: %s, Length: %llu\n", (char*)packet.buffer, packet.len);
        }
    }

    // Destroy the queue
    destroyQueue(queue);

    return 0;
}
