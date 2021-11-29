project('libbluos_ssc_ipc', 'c')

sndfile       =  dependency('sndfile')
gmp           =  dependency('gmp')

bits          =  static_library('bits', 'mqa-files/bits.c')
mqa_common    =  static_library('mqa-common', 'mqa-files/mqa-common.c')
blake2s_ref   =  static_library('blake2s-ref', 'mqa-files/blake2s-ref.c')
mqa_keys      =  static_library('mqa-keys', 'mqa-files/mqa-keys.c')
sfbits        =  static_library('sfbits', 'mqa-files/sfbits.c', dependencies: sndfile, link_with: bits)
mqascan       =  static_library('mqascan', 'mqa-files/mqascan.c', dependencies: [gmp, sndfile], link_with: [bits, mqa_common, blake2s_ref, mqa_keys, sfbits])

bluos_ssc_ipc =  library('bluos_ssc_ipc', 'libbluos_ssc_ipc.c', dependencies: sndfile, link_with: mqascan)

executable('sndfile_portaudio', 'samples/sndfile_portaudio.c', dependencies: sndfile, link_with: bluos_ssc_ipc )
