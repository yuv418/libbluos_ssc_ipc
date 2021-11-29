#ifndef IPC_STRUCTS_H_
#define IPC_STRUCTS_H_

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
  TURNTYPE turn;
  SF_INFO audio_info;
  uint8_t number_of_folds;
} ipc_mqa_struct;

#endif // IPC_STRUCTS_H_
