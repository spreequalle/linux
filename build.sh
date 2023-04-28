#!/bin/bash

set -o errtrace
trap 'echo Fatal error: script $0 aborting at line $LINENO, command \"$BASH_COMMAND\" returned $?; exit 1' ERR

usage() {
  echo "Usage: $0 [-jN] [-c toolchain_prefix] [steak]"
}

NUM_JOBS=$(grep -c processor /proc/cpuinfo)

# Add git hash to date. This corresponds to scripts/mkcompile_h
# This shows up in /proc/version
export KBUILD_BUILD_TIMESTAMP="$(date) ($(git rev-parse HEAD | head -c 8))"

export ARCH=arm64
export CROSS_COMPILE=aarch64-cros-linux-gnu-
export PLATFORM=Marvell_Dongle_BG2CD_A0
export LD=${CROSS_COMPILE}ld.bfd

deterministic_build=false

while getopts ":j:c:d" arg; do
  case "${arg}" in
    j)
      NUM_JOBS=${OPTARG}
      ;;
    c)
      export CROSS_COMPILE=${OPTARG}
      ;;
    d)
      deterministic_build=true
      ;;
  esac
done
shift $((OPTIND-1))

if [[ "$#" -lt 1 ]]; then
  usage
  exit 1
fi

# Set variables for deterministic kernel builds
# These correspond to scripts/mkcompile_h

if [[ "$deterministic_build" == true ]]; then
  export KBUILD_BUILD_VERSION=0
  export KBUILD_BUILD_TIMESTAMP="$(git log -1 --format=%cd) ($(git rev-parse HEAD | head -c 8))"
  export KBUILD_BUILD_USER=user
  export KBUILD_BUILD_HOST=host
else
  # Add git hash to date. This corresponds to scripts/mkcompile_h
  # This shows up in /proc/version
  export KBUILD_BUILD_TIMESTAMP="$(date) ($(git rev-parse HEAD | head -c 8))"
fi

build_kernel() {
  local product=$1

  # Clean
  make clean

  # Build config
  make ${product}_defconfig

  # Verify kernel config
  diff .config arch/arm64/configs/${product}_defconfig

  make -j${NUM_JOBS}
}

build_dtb() {
  local dtb_file=$1

  # make dtb
  make marvell/${dtb_file}.dtb -j${NUM_JOBS}
  make marvell/${dtb_file}-tz.dtb -j${NUM_JOBS}

  # make recovery-dtb
  make marvell/${dtb_file}-recovery.dtb -j${NUM_JOBS}
  make marvell/${dtb_file}-tz-recovery.dtb -j${NUM_JOBS}

  # make factory-dtb
  make marvell/${dtb_file}-factory.dtb -j${NUM_JOBS}
  make marvell/${dtb_file}-tz-factory.dtb -j${NUM_JOBS}
}

build_perf() {
  pushd tools/perf
  make JOBS=${NUM_JOBS} NO_DWARF=1 NO_LIBPERL=1 NO_LIBPYTHON=1 NO_NEWT=1 \
    NO_GTK2=1 NO_LIBELF=1
  popd
}

# Append $1 which is an integer value to the file at path $2. $1 is appended as
# a binary little endian integer.
append_uint32_le() {
  local val=$1
  local file=$2
  printf "0: %.8x" ${val} | sed -E 's/0: (..)(..)(..)(..)/0: \4\3\2\1/' \
    | xxd -r -g0 >> ${file}
}

# Pack the kernel along with its dtb file
# The format is [header][xz compressed kernel][dtb file]
#
# header is little endian and consists of
# struct {
#   char magic[KDTB_MAGIC_SZ];
#   uint32_t kernel_size;
#   uint32_t dtb_size;
# };
pack_kernel() {
  local product=$1
  local dtb_file=$2
  local compressed_kernel=arch/arm64/boot/kernel.${product}.xz
  local packed_kernel=arch/arm64/boot/kernel.${product}.xz-dtb
  xz -C crc32 -6 -c arch/arm64/boot/Image > ${compressed_kernel}

  local magic="KDTB"
  echo -n ${magic} > ${packed_kernel}
  append_uint32_le $(stat -c %s ${compressed_kernel}) ${packed_kernel}
  append_uint32_le $(stat -c %s ${dtb_file}) ${packed_kernel}

  cat ${compressed_kernel} ${dtb_file} >> ${packed_kernel}
}

create_kernel_pkg() {
    local kernel_path=$(readlink -f $1)
    local pkg_dir=$(mktemp -d)
    local wd=$(pwd)

    for f in ${copy_file_list[@]}
    do
      s=${f%%:*}
      d=${f##*:}
      mkdir -p `dirname $pkg_dir/$d`
      echo "cp $kernel_path/$s $pkg_dir/$d"
      cp $kernel_path/$s $pkg_dir/$d
    done

    (cd $pkg_dir; tar zcvf $wd/kernel.tgz *)
    rm -fr $pkg_dir
}

declare -a copy_file_list=()
readonly kernel_dir=.

readonly dts_dir="arch/arm64/boot/dts/marvell"
readonly nandpart_prefix="berlin4cdp-dongle-nandpart"

dtb_nandpart_cleanup() {
  rm -f ${dts_dir}/${nandpart_prefix}.dtsi
}
trap dtb_nandpart_cleanup EXIT

for product in $*; do
  case $product in
    steak)
      readonly kernel=arch/arm64/boot/kernel.${product}.xz-dtb
      rm -f ${kernel}
      build_kernel berlin4cdp

      # TODO(bcf): Fix perf build
      #copy_file_list+=(tools/perf/perf:${product}/prebuilt/sdk/bin/perf)

      for board in luther-b1 luther-b2 luther-b4; do
        case ${board} in
          luther-b1)
            chip_rev=z1
            ;;
          luther-b2|luther-b4)
            chip_rev=a0
            ;;
          *)
            echo "Unknown board: ${board}"
            exit 1
            ;;
        esac

        # Symlink the appropriate partition map .dtsi file. The file
        # ${dts_dir}/${nandpart_prefix}.dtsi is the one included in
        # the dts file for all boards, but changing what it symlinks
        # to lets us use different partition maps for different boards.
        dtb_nandpart_cleanup
        ln -s ${nandpart_prefix}-${board}.dtsi ${dts_dir}/${nandpart_prefix}.dtsi

        dtb=berlin4cdp-${chip_rev}-dongle

        rm -f ${dts_dir}/${dtb}.dtb-${product}.${board}
        rm -f ${dts_dir}/${dtb}-tz.dtb-${product}.${board}
        rm -f ${dts_dir}/${dtb}-factory.dtb-${product}.${board}
        rm -f ${dts_dir}/${dtb}-tz-factory.dtb-${product}.${board}
        rm -f ${dts_dir}/${dtb}-recovery.dtb-${product}.${board}
        rm -f ${dts_dir}/${dtb}-tz-recovery.dtb-${product}.${board}

        build_dtb ${dtb}

        # Create a ktb file if one doesn't exist.
        # During the OTA build, the correct DTB will be put into place.
        if [ ! -f ${kernel} ]; then
          pack_kernel ${product} ${dts_dir}/${dtb}-tz-recovery.dtb
          copy_file_list+=(arch/arm64/boot/kernel.${product}.xz-dtb:${product}/prebuilt/kernel/Image.xz-dtb)
        fi

        mv ${dts_dir}/${dtb}.dtb ${dts_dir}/${dtb}.dtb-${product}.${board}
        mv ${dts_dir}/${dtb}-tz.dtb ${dts_dir}/${dtb}-tz.dtb-${product}.${board}
        mv ${dts_dir}/${dtb}-factory.dtb ${dts_dir}/${dtb}-factory.dtb-${product}.${board}
        mv ${dts_dir}/${dtb}-tz-factory.dtb ${dts_dir}/${dtb}-tz-factory.dtb-${product}.${board}
        mv ${dts_dir}/${dtb}-recovery.dtb ${dts_dir}/${dtb}-recovery.dtb-${product}.${board}
        mv ${dts_dir}/${dtb}-tz-recovery.dtb ${dts_dir}/${dtb}-tz-recovery.dtb-${product}.${board}

        copy_file_list+=(COPYING:${product}/prebuilt/kernel/COPYING)

        copy_file_list+=(${dts_dir}/${dtb}.dtb-${product}.${board}:${product}/prebuilt/kernel/${board}.dtb)
        copy_file_list+=(${dts_dir}/${dtb}-tz.dtb-${product}.${board}:${product}/prebuilt/kernel/${board}-tz.dtb)
        copy_file_list+=(${dts_dir}/${dtb}-recovery.dtb-${product}.${board}:${product}/prebuilt/kernel/recovery-${board}.dtb)
        copy_file_list+=(${dts_dir}/${dtb}-tz-recovery.dtb-${product}.${board}:${product}/prebuilt/kernel/recovery-${board}-tz.dtb)
        copy_file_list+=(${dts_dir}/${dtb}-factory.dtb-${product}.${board}:${product}/prebuilt/kernel/factory-${board}.dtb)
        copy_file_list+=(${dts_dir}/${dtb}-tz-factory.dtb-${product}.${board}:${product}/prebuilt/kernel/factory-${board}-tz.dtb)
      done
      ;;
    *)
      echo "unknown product: $product"
      exit 1
  esac
done


# TODO(bcf): Fix perf build
# Build perf
#build_perf

# Create a kernel package for all products
create_kernel_pkg $kernel_dir
