#!/usr/bin/env python3

# TODO: generate filesystem image.

from os import system
from pathlib import Path
from argparse import ArgumentParser

def sh(command):
    print(f'> {command}')
    assert system(command) == 0

sector_size = 512
n_sectors = 256 * 1024
boot_offset = 2048
n_boot_sectors = 128 * 1024
filesystem_offset = boot_offset + n_boot_sectors
n_filesystem_sectors = n_sectors - filesystem_offset

def generate_boot_image(target, files):
    sh(f'dd if=/dev/zero of={target} seek={n_boot_sectors - 1} bs={sector_size} count=1')

    # "-F 32" specifies FAT32.
	# "-s 1" specifies one sector per cluster so that we can create a smaller one.
    sh(f'mkfs.vfat -F 32 -s 1 {target}')

	# copy files into boot partition.
    for file in files:
        sh(f'mcopy -i {target} {file} ::{Path(file).name};')

def generate_fs_image(target, files):
	sh(f'cc ../src/user/mkfs/main.c -o ../build/mkfs -I../src/')
	file_list=""
	for file in files:
		file_list = file_list + "../build/src/user/" + str(file) + ' '
	print(file_list)
	sh(f'../build/mkfs {target} {file_list}')

def generate_sd_image(target, boot_image, fs_image):
    sh(f'dd if=/dev/zero of={target} seek={n_sectors - 1} bs={sector_size} count=1')

    boot_line = f'{boot_offset}, {n_boot_sectors * sector_size // 1024}K, c,'
    filesystem_line = f'{filesystem_offset}, {n_filesystem_sectors * sector_size // 1024}K, L,'
    sh(f'printf "{boot_line}\\n{filesystem_line}\\n" | sfdisk {target}')

    sh(f'dd if={boot_image} of={target} seek={boot_offset} conv=notrunc')
    sh(f'dd if={fs_image} of={target} seek={filesystem_offset} conv=notrunc')

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('root')
    parser.add_argument('files', nargs=14)
    parser.add_argument('user_files', nargs='*')

    args = parser.parse_args()

    boot_image = f'{args.root}/boot.img'
    sd_image = f'{args.root}/sd.img'
    fs_image = f'{args.root}/fs.img'

    generate_boot_image(boot_image, args.files)
    generate_fs_image(fs_image, args.user_files)
    generate_sd_image(sd_image, boot_image, fs_image)
