#pragma once
#include "sync.h"
#include <stdint.h>

// actual struct definition
typedef struct {
  // It can be either 0 (unlocked), 1 (locked but no waiters), or 2 (locked with
  // waiters) sometimes when it's 2 there are no waiters, but you err on the
  // side of caution or something and end up sending a futex_wake for no reason
  // when you call drepper_unlock. Such is life.
  uint32_t mutex_val;
} drepper_mutex_t;

// reset the mutex
void drepper_init(drepper_mutex_t *mtx);

void drepper_lock(drepper_mutex_t *mtx);

// Unlock a mutex
void drepper_unlock(drepper_mutex_t *mtx);
