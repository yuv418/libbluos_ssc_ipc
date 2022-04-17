#!/bin/sh

# Attempts to download an ArchLinux chroot, compile the entire thing, and move it to the install dir. Yay.

COMPILE_DIR=`mktemp -d`
FULL_PATH_TO_SCRIPT="$(dirname $(realpath "${BASH_SOURCE[-1]}"))" # thanks stackoverflow


cd $COMPILE_DIR

mkdir chroot_root
wget http://os.archlinuxarm.org/os/ArchLinuxARM-odroid-c1-latest.tar.gz -O alarm.tgz
bsdtar -xvpf alarm.tgz -C chroot_root

# otherwise nothing will install
# make sure the user had arch-chroot installed
# ridiculous workaround
sed 's/CheckSpace/#CheckSpace/' chroot_root/etc/pacman.conf > pacman1.conf
cp pacman1.conf chroot_root/etc/pacman.conf

arch-chroot chroot_root pacman-key --init
arch-chroot chroot_root pacman-key --populate archlinuxarm
arch-chroot chroot_root pacman -Sy --noconfirm base-devel meson ninja libsndfile alsa-lib portaudio ccache

mkdir chroot_root/bluos_ssc
mount --bind ${FULL_PATH_TO_SCRIPT} chroot_root/bluos_ssc

# there's probably a better way to do this.
arch-chroot chroot_root bash -c 'cd bluos_ssc/mqa-files && meson build --buildtype=release && cd build && ninja'

umount chroot_root/bluos_ssc # if we don't do this we obliterate the source code
rmdir chroot_root/bluos_ssc

# generate the bundle
mkdir -p ${FULL_PATH_TO_SCRIPT}/build/arm-bundle/{ld_library_libs,libs,bins}


cp chroot_root/lib/ld-linux-armhf.so.3 ${FULL_PATH_TO_SCRIPT}/build/arm-bundle/libs
cp ${FULL_PATH_TO_SCRIPT}/mqa-files/build/mqaplay_ipc ${FULL_PATH_TO_SCRIPT}/build/arm-bundle/bins

cd chroot_root/usr/lib/

cp libasound.so.2 libc.so.6 libdl.so.2 libFLAC.so.8 libgcc_s.so.1 libm.so.6 libogg.so.0 libopus.so.0 \
    libpthread.so.0 librt.so.1 libsndfile.so.1 libstdc++.so.6 libvorbisenc.so.2 libvorbis.so.0 \
    ${FULL_PATH_TO_SCRIPT}/build/arm-bundle/ld_library_libs

cd ${FULL_PATH_TO_SCRIPT}
# rm -rf $COMPILE_DIR # let's be careful here, okay?


# Mkdir -p "${DESTDIR}/${MESON_INSTALL_PREFIX}/bluos_ssc"
