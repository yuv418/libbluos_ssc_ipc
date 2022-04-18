project('libbluos_ssc_ipc', 'c')

sndfile        =  dependency('sndfile')
gmp            =  dependency('gmp')
portaudio      =  dependency('portaudio-2.0')
libconfig      =  dependency('libconfig')

bits           =  static_library('bits', 'mqa-files/bits.c')
sync_fns       =  static_library('sync', 'sync/sync.c')
drepper_mutex  =  static_library('drepper_mutex', 'sync/drepper_mutex.c', link_with: sync_fns)
fxsem          =  static_library('fxsem', 'sync/fxsem.c', link_with: sync_fns)
ipc_structs    =  static_library('ipc_structs', 'ipc_structs.c', link_with: [drepper_mutex, fxsem])
mqa_common     =  static_library('mqa-common', 'mqa-files/mqa-common.c')
blake2s_ref    =  static_library('blake2s-ref', 'mqa-files/blake2s-ref.c')
mqa_keys       =  static_library('mqa-keys', 'mqa-files/mqa-keys.c')
sfbits         =  static_library('sfbits', 'mqa-files/sfbits.c', dependencies: sndfile, link_with: bits)
mqascan        =  static_library('mqascan', 'mqa-files/mqascan.c', dependencies: [gmp, sndfile], link_with: [bits, mqa_common, blake2s_ref, mqa_keys, sfbits])

bluos_ssc_ipc  =  shared_library('bluos_ssc_ipc', 'libbluos_ssc_ipc.c', dependencies: [sndfile, libconfig], link_with: [mqascan, ipc_structs], install: true, install_dir: '/usr/lib')

executable('mqa_sndfile_portaudio', 'samples/sndfile_portaudio.c', dependencies: [sndfile, portaudio], link_with: [bluos_ssc_ipc, ipc_structs], install: true)


# Install stuff

install_subdir('build/arm-bundle', install_dir: '/opt/bluos_ssc_ipc')
install_data('build/arm-bundle/libs/ld-linux-armhf.so.3', install_dir: '/lib')
install_data('packaging/bluos_ssc.cfg', install_dir: '/etc')
install_headers('libbluos_ssc_ipc.h', subdir: 'libbluos_ssc_ipc')
