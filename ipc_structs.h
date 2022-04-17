#ifndef IPC_STRUCTS_H_
#define IPC_STRUCTS_H_

#include "sync/drepper_mutex.h"
#include "sync/fxsem.h"
#include <sndfile.h>

#define BUF_SIZE 4096
#define SHM_KEY 25590

typedef enum { AMDREAD, AMDWRITE, ARMPROCESS, FINISHED } TURNTYPE;

typedef struct {
  int buf_pos;
  int buf_end;
  int frame_size;
  int size;
} get_samples_data;

typedef struct {
  int len;
} write_samples_data;

// TODO add input and output details for the decoder program
typedef struct {
  uint8_t input_buffer[BUF_SIZE];
  int32_t output_buffer[BUF_SIZE * 4];
  get_samples_data get_samples_var;
  write_samples_data write_samples_var;
  // We use drepper_mutex_t in a wonky,
  // semaphore-like manner in order to
  // solve the producer-consumer problem without
  // spinning the CPU. We can't just use POSIX
  // condvars/mutexes since sizeof(pthread_cond_t) and
  // sizeof(pthread_mutex_t) differs on ARM and x86.
  drepper_mutex_t data_lock;
  fxsem_t empty; // Semaphore to tell the encoder to wait (till something is
                 // decoded)
  fxsem_t
      full; // Semaphore to tell the player to wait (till something is encoded)
  TURNTYPE turn;
  SF_INFO audio_info;
  uint8_t number_of_folds;
} ipc_mqa_struct;

// We define this here so we can use this in both the ARM executable and the x86
// one.
// Hand off execution to ARM/x86 library
void bluos_ssc_ipc_handoff(ipc_mqa_struct *handle, TURNTYPE target_turntype);
// Wait until the turntype matches a certain bitfield.
void bluos_ssc_ipc_wait(ipc_mqa_struct *handle, TURNTYPE target_turntype);

#endif // IPC_STRUCTS_H_
