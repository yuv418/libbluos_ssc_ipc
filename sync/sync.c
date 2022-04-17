#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
// syncrhonisation and atomic functions for use when implementing
// synchronisation types

// if ptr is ideal_val, then atomically swaps the value of ptr to to_val.
uint32_t cmpxchg(uint32_t *ptr, uint32_t ideal_val, uint32_t to_val) {
  // Allegedly this is how you implement cmpxchg, since
  // the idea_val_ptr variable will be stored with the ptr's old value
  // if the call fails, and if the call succeeds, well, it's obviously
  // going to be having the ideal value.
  uint32_t *ideal_val_ptr = &ideal_val;
  __atomic_compare_exchange(ptr, ideal_val_ptr, &to_val, false,
                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return *ideal_val_ptr;
}

// atomically swaps the value of ptr to to_val.
uint32_t xchg(uint32_t *ptr, uint32_t to_val) {
  // I have absolutely NO IDEA what the memory order (__ATOMIC_SEQ_CST) does.
  // Zero. Ditto for the cmpxchg function.
  return __atomic_exchange_n(ptr, to_val, __ATOMIC_SEQ_CST);
}

// atomically decrement the contents of *ptr by 1 and return the previous value
// of ptr
uint32_t atomic_dec(uint32_t *ptr) {
  return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST);
}

// atomically increment the contents of *ptr by 1 and return the previous value
// of ptr
uint32_t atomic_inc(uint32_t *ptr) {
  return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST);
}

uint32_t atomic_fetch(uint32_t *ptr) {
  return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

// Waits on the futex_word address GIVEN THAT the pointer futex_word POINTS TO a
// value that is EQUAL to confirm_val.
void futex_wait(uint32_t *futex_word, uint32_t confirm_val) {
  syscall(SYS_futex, futex_word, FUTEX_WAIT, confirm_val, NULL, NULL, 0);
}
// Wakes a maximum of n threads that are waiting on the word futex_word
long futex_wake(uint32_t *futex_word, uint32_t n) {
  return syscall(SYS_futex, futex_word, FUTEX_WAKE, n, NULL, NULL, 0);
}
