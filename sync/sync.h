#ifndef SYNC_H_
#define SYNC_H_

#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
// syncrhonisation and atomic functions for use when implementing
// synchronisation types

// if ptr is ideal_val, then atomically swaps the value of ptr to to_val.
uint32_t cmpxchg(uint32_t *ptr, uint32_t ideal_val, uint32_t to_val);
// atomically swaps the value of ptr to to_val.
uint32_t xchg(uint32_t *ptr, uint32_t to_val);
// atomically decrement the contents of *ptr by 1 and return the previous value
// of ptr
uint32_t atomic_dec(uint32_t *ptr);

// atomically increment the contents of *ptr by 1 and return the previous value
// of ptr
uint32_t atomic_inc(uint32_t *ptr);
uint32_t atomic_fetch(uint32_t *ptr);
// Waits on the futex_word address GIVEN THAT the pointer futex_word POINTS TO a
// value that is EQUAL to confirm_val.
void futex_wait(uint32_t *futex_word, uint32_t confirm_val);

// Wakes a maximum of n threads that are waiting on the word futex_word
long futex_wake(uint32_t *futex_word, uint32_t n);

#endif // SYNC_H_
