#!/bin/bash -x

FS_IMAGE=fs-image.raw

dd if=/dev/zero of=${FS_IMAGE} count=$[1024*100] bs=1024
/sbin/mkfs.vfat -F 16 -n TEST­-v ${FS_IMAGE}
mkdir fs_root
sudo mount -t msdos -o loop,rw,showexec,codepage=852,umask=0 ${FS_IMAGE} fs_root
python fs-populate.py fs_root/
sudo umount fs_root
rmdir fs_root
