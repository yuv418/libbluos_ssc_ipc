#include "ipc_structs.h"

// waits until the target_turntype is target_turntype without spinning the cpu
void bluos_ssc_ipc_wait(ipc_mqa_struct *handle, TURNTYPE target_turntype) {
  // lock the right lock based on what the function call is waiting for.
  if (target_turntype == ARMPROCESS) {
    // the encoder is waiting
    fxsem_down(&handle->empty);
  } else {
    // the client is waiting
    fxsem_down(&handle->full);
  }
  drepper_lock(&handle->data_lock);
}

void bluos_ssc_ipc_handoff(ipc_mqa_struct *handle, TURNTYPE target_turntype) {
  handle->turn = target_turntype;
  drepper_unlock(&handle->data_lock);

  if (target_turntype == ARMPROCESS) {
    // handing off to ARM, so we want to up the empty
    fxsem_up(&handle->empty);
  } else {
    // handing off to x86, so we want to up the full
    fxsem_up(&handle->full);
  }
}
