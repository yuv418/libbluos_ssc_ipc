
#include "fxsem.h"
#include "sync.h"

#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

// futex-based binary semaphore

void fxsem_init(fxsem_t *sem) { sem->sem_val = 0; }

// implement the "up" operation of a semaphore that increments the value
void fxsem_up(fxsem_t *sem) {
  // we will only wake up another waiting thread if the value was already zero.
  if (atomic_inc(&sem->sem_val) == 0) {
    futex_wake(&sem->sem_val, 1);
  }
}
// implement the "down" operation of a semaphore that decrements the value of
// the semaphore, and if the semaphore is zero (zero resources is how I like to
// think about it), then the value waits until it is woken up and there is a new
// resource.
void fxsem_down(fxsem_t *sem) {
  // we will only wake up another waiting thread if the value was already zero.
  // binary semaphore means that we check if sem_val is 1, and if it is we
  // change it zero. Othewrise we sleep. TODO how do we write a fully-functional
  // semaphore using one instruction?
  while (cmpxchg(&sem->sem_val, 1, 0) == 0) {
    futex_wait(&sem->sem_val, 0);
  }
}
