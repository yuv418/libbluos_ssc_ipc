#ifndef FXSEM_H_
#define FXSEM_H_
#include "sync.h"
#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

// futex-based binary semaphore

typedef struct {
  uint32_t sem_val;
} fxsem_t;

void fxsem_init(fxsem_t *sem);

// implement the "up" operation of a semaphore that increments the value
void fxsem_up(fxsem_t *sem);
// implement the "down" operation of a semaphore that decrements the value of
// the semaphore, and if the semaphore is zero (zero resources is how I like to
// think about it), then the value waits until it is woken up and there is a new
// resource.
void fxsem_down(fxsem_t *sem);
#endif
