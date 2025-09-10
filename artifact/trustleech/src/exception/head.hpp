#pragma once

struct exception_frame;

extern "C" const void* exception_vectors;

extern "C" void handle_sync_exception_aarch64(exception_frame* frame);
extern "C" void handle_irq_aarch64();
