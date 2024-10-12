#!/usr/bin/env bash
# require
# - qemu

set -eu

print_help()
{
    local usagetext
    IFS='' read -r -d '' usagetext <<EOF || true
Usage:
    run-debug.sh [options]

Options:
    -h              print help
    -i [image]      image to boot into
    -k [kernel]     set kernel file('bzImage') path.

Example:
    Run an image using UEFI:
    $ ./run-debug.sh -k '/path/to/kernel' -i '/path/to/rootfs.ext4'
EOF
    printf '%s' "${usagetext}"
}

check_image()
{
    if [[ -z "$image" ]]; then
        printf 'ERROR: %s\n' "Image name can not be empty."
        exit 1
    fi
    if [[ ! -f "$image" ]]; then
        printf 'ERROR: %s\n' "Image file (${image}) does not exist."
        exit 1
    fi
}

check_kernel()
{
    if [[ -z "$kernel" ]]; then
        printf 'ERROR: %s\n' "Kernel name can not be empty."
        exit 1
    fi
    if [[ ! -f "$kernel" ]]; then
        printf 'ERROR: %s\n' "Kernel file (${kernel}) does not exist."
        exit 1
    fi
}

run_image()
{
#    if [[ "$boot_type" == 'uefi' ]]; then
#        copy_ovmf_vars
#        if [[ "${secure_boot}" == 'on' ]]; then
#            printf '%s\n' 'Using Secure Boot'
#            local ovmf_code='/usr/share/edk2-ovmf/x64/OVMF_CODE.secboot.fd'
#        else
#            local ovmf_code='/usr/share/edk2-ovmf/x64/OVMF_CODE.fd'
#        fi
#        qemu_options+=(
#            '-drive' "if=pflash,format=raw,unit=0,file=${ovmf_code},readonly"
#            '-drive' "if=pflash,format=raw,unit=1,file=${working_dir}/OVMF_VARS.fd"
#            '-global' "driver=cfi.pflash01,property=secure,value=${secure_boot}"
#        )
#    fi

#    if [[ "${accessibility}" == 'on' ]]; then
#        qemu_options+=(
#            '-chardev' 'braille,id=brltty'
#            '-device' 'usb-braille,id=usbbrl,chardev=brltty'
#        )
#    fi

    qemu-system-x86_64 \
        -k en \
        -kernel "$kernel" \
        -hda "$image" \
        -display sdl \
        -append "root=/dev/sda console=ttyS0 nokaslr ro" \
        -nographic \
        -s -S

#        -enable-kvm \
#        -no-reboot \
#        -serial stdio \
#        -global ICH9-LPC.disable_s3=1 \
#        -vga virtio \
#        -name kernel-debug,process=kernel-debug \
#        -m "size=3072,slots=0,maxmem=$((3072*1024*1024))" \
#        "${qemu_options[@]}" \
#        -audiodev pa,id=snd0 \
#        -device ich9-intel-hda \
#        -device hda-output,audiodev=snd0 \
#        -device virtio-net-pci,romfile=,netdev=net0 -netdev user,id=net0 \
#        -machine type=q35,smm=on,accel=kvm,usb=on,pcspk-audiodev=snd0 \
# -boot order=d,menu=on,reboot-timeout=5000 
}

set_image()
{
    if [[ -z "$image" ]]; then
        printf 'ERROR: %s\n' "Image name can not be empty."
        exit 1
    fi
    if [[ ! -f "$image" ]]; then
        printf 'ERROR: %s\n' "Image (${image}) does not exist."
        exit 1
    fi
    image="$1"
}


#boot_type='bios'
#mediatype='cdrom'
#secure_boot='off'
qemu_options=()
kernel='./bzImage'
image='./000-rootfs/output/images/rootfs.ext4'
#working_dir="$(mktemp -dt run_archiso.XXXXXXXXXX)"
#trap cleanup_working_dir EXIT

if (( ${#@} > 0 )); then
    while getopts 'abdhi:su' flag; do
        case "$flag" in
            h)
                print_help
                exit 0
                ;;
            i)
                image="$OPTARG"
                ;;
            k)
                kernel="$OPTARG"
                ;;
            *)
                print_help
                exit 1
                ;;
        esac
    done
fi

echo "Kernel file   : $kernel"
echo "rootfs        : $image"

check_image
check_kernel

run_image
