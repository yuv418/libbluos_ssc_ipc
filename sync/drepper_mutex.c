#include "drepper_mutex.h"

// me when I try implementing Ulrich Drepper's mutex whitepaper
// and i try explaining everything since I actually tried to read the whitepaper

// reset/init the mutex
void drepper_init(drepper_mutex_t *mtx) { mtx->mutex_val = 0; }

void drepper_lock(drepper_mutex_t *mtx) {
  // lock the mutex, obviously

  // Atomically try and check if mutex_val is 0 (unlocked). If it is, then set
  // it to 1 (locked). If that works, then the return of cmpxchg will be zero,
  // and we know stuff is locked. There is thus no contention from other
  // threads/processes trying to lock that mutex.
  int tmp_mutex_val;
  if ((tmp_mutex_val = cmpxchg(&mtx->mutex_val, 0, 1)) != 0) {
    // This code will happen if orig_mutex_val is either 1 or 2, so locked
    // with waiters or locked without waiters. We want to set the val to 2
    // to assert that there is now a waiter (whatever is calling this
    // function) if that was not done already.
    if (tmp_mutex_val != 2) {
      // If tmp_mutex_val is not set, like I foolishly decided to do,
      // then tmp_mutex_val is either 1 or 0.
      //
      // Let's analyse both scenarios.
      //
      // 1: tmp_mutex_val goes into the while loop, and waits (it will wait
      // since the xchg changed mtx->mutex_val). When the thread receives its
      // wakeup, the tmp_mutex_val is set to 0 as it should be and nothing
      // happens. Yay!
      //
      // 0: tmp_mutex_val DOES NOT GO INTO THE WHILE LOOP.
      // drepper_lock exits thinking a lock has been acquired BUT NO LOCK WAS
      // ACQUIRED. Then when the other thread tries to unlock this mutex, the
      // futex_wake will not do anything. This will probably lead to disaster.
      tmp_mutex_val = xchg(&mtx->mutex_val, 2);
    }

    // Now that we know that there is a waiter, we will wait, then
    // when the thread is awoken we unconditionally set the mutex value
    // to 2 (locked with waiters) since in the drepper_unlock
    // function we skip waking any threads if the mutex value is 1. If there are
    // waiters (which we cannot guarantee or know with this implementation),
    // then they will just never wake up, so we set the mutex value to 2.
    while (tmp_mutex_val != 0) {
      // At this point of the mutex was unlocked and set to zero, then
      // futex_wait will immediately exit,
      futex_wait(&mtx->mutex_val, 2);
      // Now we give the caller of drepper_lock the lock for the mutex
      // tmp_mutex_val is set to the OLD value of the mutex.
      // Tht value must have been zero for it to have been legal for us
      // to acquire a lock. If it were not legal, then the loop will continue
      // again.
      //
      // TODO Figure out in what condition will xchg return a value other than
      // 0. Perhaps if a thread unlocks, but by the time we get here some other
      // thread has acquired the lock (between futex_wait and xchg), but we
      // never wake more than one thread... Anyway, it's covered.
      tmp_mutex_val = xchg(&mtx->mutex_val, 2);
    }
  }
}
// Unlock a mutex
void drepper_unlock(drepper_mutex_t *mtx) {
  // Try to decrement the value of the mutex. If the original value of the mutex
  // is 1, then the if statement decrements the mutex value to 0, which unlocks
  // the mutex. And nothing inside the if statement gets executed. If the value
  // of the mutex is zero, then... nothing? Unsigned int haha If the value of
  // the mutex is 2, then atomic_dec returns 2, goes into the if statement. Then
  // in that case we must wake up the waiters, so we do a little futex_wake and
  // then set the mutex_val to zero.
  if (atomic_dec(&mtx->mutex_val) != 1) {
    mtx->mutex_val = 0;
    // We are waking one waiter.
    futex_wake(&mtx->mutex_val, 1);
  }
}
