project('libbluos_ssc_ipc_client', 'c')

cc              =  meson.get_compiler('c')

dl              =  cc.find_library('dl')
sndfile         =  dependency('sndfile')

bluos_ssc       =  static_library('bluos_ssc', 'bluos_ssc.c')
sync_fns       =  static_library('sync', '../sync/sync.c')
drepper_mutex  =  static_library('drepper_mutex', '../sync/drepper_mutex.c', link_with: sync_fns)
fxsem          =  static_library('fxsem', '../sync/fxsem.c', link_with: sync_fns)
ipc_structs     =  static_library('ipc_structs', '../ipc_structs.c', link_with: [drepper_mutex, fxsem])

executable('mqaplay_ipc', 'mqaplay_ipc.c', dependencies: [sndfile, dl], link_with: [bluos_ssc, ipc_structs])
