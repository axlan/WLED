#!/usr/bin/env bash

# This really should be part of the build (along with generate_roll_info.py)
# Running this for testing.

if [ $# -eq 0 ]
  then
    echo "Must specify output directory as CLI argument."
    exit 1
fi

BUILD_DIR=.pio/build/t_qt_pro_8MB
INSTALL_DIR=$1

# Both these builds target ESP32-S3_8MB_qspi and use the same bootloader/partitions.
cp $BUILD_DIR/bootloader.bin $INSTALL_DIR/bootloader_s3.bin
cp $BUILD_DIR/partitions.bin $INSTALL_DIR/partitions_s3_8m.bin
cp $BUILD_DIR/firmware.bin $INSTALL_DIR/WLED_0.15.0-b4_T-QT-PRO-8MB.bin

BUILD_DIR=.pio/build/esp32s3dev_8MB_qspi

cp $BUILD_DIR/firmware.bin $INSTALL_DIR/WLED_0.15.0-b4_ESP32-S3_8MB_qspi.bin
