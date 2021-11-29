project('libbluos_ssc_ipc_client', 'c')

cc              =  meson.get_compiler('c')

dl              =  cc.find_library('dl')
sndfile         =  dependency('sndfile')

bluos_ssc       =  static_library('bluos_ssc', 'bluos_ssc.c')

executable('mqaplay_ipc', 'mqaplay_ipc.c', dependencies: [sndfile, dl], link_with: bluos_ssc)

