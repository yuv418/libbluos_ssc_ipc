// TODO don't hardcode anything
#include <libconfig.h>
#include <limits.h>
#include <sndfile.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "ipc_structs.h"
#include "libbluos_ssc_ipc.h"
#include "mqa-files/mqascan.h"
#include "sync/drepper_mutex.h"

// Copied from the ARM version
// Code from Mans Rullgard:
// https://code.videolan.org/mansr/mqa/-/blob/master/mqadec.c How do I "cite"
// this? Hmm...
void bluos_ssc_sndfile_read_samples_shm(ipc_mqa_struct *handle,
                                        SNDFILE *in_file) {
  // printf("reading audio\n");
  get_samples_data *gs_var = &handle->get_samples_var;

  gs_var->size = 0;
  if (gs_var->buf_pos > 0) {
    gs_var->size = gs_var->buf_end - gs_var->buf_pos;
    memmove(handle->input_buffer, handle->input_buffer + gs_var->buf_pos,
            gs_var->size);

    gs_var->buf_pos = 0;
    gs_var->buf_end = gs_var->size;
  }

  if (gs_var->buf_end < BUF_SIZE) {
    int frames = (BUF_SIZE - gs_var->buf_end) / gs_var->frame_size;

    frames = sf_readf_int(
        in_file, (int *)&handle->input_buffer[gs_var->buf_end], frames);
    gs_var->buf_end += frames * gs_var->frame_size;
  }

  gs_var->size = gs_var->buf_end - gs_var->buf_pos;
  gs_var->size -= gs_var->size % gs_var->frame_size;
  // printf("gsvar size %d\n", gs_var->size);
}

// start
bool bluos_ssc_ipc_armproc_start() {
  config_t armproc_cfg;
  bool retval;

  const char *proc_ld_library_path;
  const char *proc_binary_path;
  const char *proc_qemu_arm_path;

  config_init(&armproc_cfg);

  // try /etc/bluos_ssc.cfg, then ~/.config/bluos_ssc/bluos_ssc.cfg, then
  // BLUOS_SSC_CFG envvar (can be relative)

  char *etc_bluos = "/etc/bluos_ssc.cfg";
  char home_config_bluos[PATH_MAX];
  sprintf(home_config_bluos, "%s/.config/bluos_ssc/bluos_ssc.cfg",
          getenv("HOME"));
  char *custom_cfg = realpath(getenv("BLUOS_SSC_CFG"), NULL);
  printf("etc_bluos: %s\nhome_config_bluos: %s\ncustom_cfg: %s\n", etc_bluos,
         home_config_bluos, custom_cfg);

  // actual cfg file that is determined after access() calls
  char *cfg_file_path;

  if (access(etc_bluos, F_OK) == 0) {
    cfg_file_path = etc_bluos;
  } else if (access(home_config_bluos, F_OK) == 0) {
    cfg_file_path = home_config_bluos;
  } else if (access(custom_cfg, F_OK) == 0) {
    cfg_file_path = custom_cfg;
  } else {
    fprintf(stderr, "no valid bluos_ssc.cfg fiile was provided.\n");
    goto FAIL;
  }

  printf("chose %s for bluos_ssc.cfg\n", cfg_file_path);
  if (!config_read_file(&armproc_cfg, cfg_file_path)) {
    // from the libconfig examples
    fprintf(stderr, "config file read failure, %s:%d - %s\n",
            config_error_file(&armproc_cfg), config_error_line(&armproc_cfg),
            config_error_text(&armproc_cfg));
    goto FAIL;
  }

  // Get the details to call execve
  if (!config_lookup_string(&armproc_cfg, "armproc_details.binary_path",
                            &proc_binary_path)) {
    fprintf(stderr, "missing armproc_details.binary_path");
    goto FAIL;
  }
  if (!config_lookup_string(&armproc_cfg, "armproc_details.libs_path",
                            &proc_ld_library_path)) {
    fprintf(stderr, "missing armproc_details.ld_library_path");
    goto FAIL;
  }
  if (!config_lookup_string(&armproc_cfg, "armproc_details.qemu_arm_path",
                            &proc_qemu_arm_path)) {
    fprintf(stderr, "missing armproc_details.qemu_arm_path");
    goto FAIL;
  }

  // call execve
  char ld_library_path[PATH_MAX + 16];
  sprintf(ld_library_path, "LD_LIBRARY_PATH=%s",
          realpath(proc_ld_library_path, NULL));

  proc_binary_path = realpath(proc_binary_path, NULL);

  printf("ld_library_path is %s\n", ld_library_path);

  // cast to avoid warnings
  char *execve_argv[] = {(char *)proc_qemu_arm_path, (char *)proc_binary_path,
                         NULL};
  char *execve_envp[] = {ld_library_path, NULL};

  int pid = fork();
  if (pid == 0) {
    int ret = execve(proc_qemu_arm_path, execve_argv, execve_envp);
    printf("execve returned with %d", ret);
    perror("execve");
  }

  // TODO check if the exec worked out
  retval = true;
  goto CLEANUP;

FAIL:
  retval = false;
  goto CLEANUP;

CLEANUP:
  config_destroy(&armproc_cfg);
  return retval;
}

// Initialise the IPC structs for shared memory communication
ipc_mqa_struct *bluos_ssc_ipc_init(SF_INFO in_file_info,
                                   uint8_t number_of_folds) {
  int shmid;
  ipc_mqa_struct *shared_mqa_struct;

  // Get the shared memory for this location
  shmid = shmget(SHM_KEY, sizeof(ipc_mqa_struct), IPC_CREAT | 0777);
  shared_mqa_struct = shmat(shmid, NULL, 0);

  // IPC renderer code
  ipc_mqa_struct temp_mqa_struct = {
      .input_buffer = {0},
      .output_buffer = {0},
      .get_samples_var =
          {
              .buf_end = 0,
              .buf_pos = 0,
              .frame_size = 0,
              // This allows the while loop to start
              // later. It won't affect anything since
              // size gets overwritten immediately anyway.
              .size = 1,
          },
      .write_samples_var = {.len = 0},
      // sync stays "uninitialised"
      .turn = ARMPROCESS,
      .audio_info = in_file_info,
      .number_of_folds = number_of_folds,
  };

  drepper_init(&temp_mqa_struct.data_lock);
  fxsem_init(&temp_mqa_struct.full);
  fxsem_init(&temp_mqa_struct.empty);

  // Copy the initial struct to shared memory.
  // We will start to use the shared_mqa_struct pointer exclusively from here
  // onwards.
  *shared_mqa_struct = temp_mqa_struct;

  // We block until the decoder has put the initial details required into the
  // shared_mqa_struct, and then return the handle.
  printf("starting MQA encoder process\n");
  if (!bluos_ssc_ipc_armproc_start()) {
    printf("Failed to start MQA encoder process\n");
    exit(1);
  }
  fxsem_down(&shared_mqa_struct->full);

  // TODO fork mqaplay_ipc here using a config file
  // that has paths for qemu-arm-static and the binary file

  return shared_mqa_struct;
}

mqa_sample_rates bluos_ssc_sndfile_mqa_rate_info(SNDFILE *in_file,
                                                 SF_INFO *in_file_info) {
  // wrapper for the MQA scan stuff
  scan_file(in_file, in_file_info);
  return get_mqa_sample_rates();
}
