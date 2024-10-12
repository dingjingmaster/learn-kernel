#!/usr/bin/env bash
# require
# - qemu

set -eu

sudo -v

tapName='ktap0'

echo 'Please check you system, you have create net dev "virbr0"'

add_tap()
{
    sudo ip tuntap add dev $tapName mode tap
    sudo ip link set $tapName master virbr0
    sudo ip addr add 192.168.122.254/24 dev $tapName
    sudo ip link set $tapName up
}

del_tap()
{
    sudo ip link set $tapName down
    sudo ip tuntap del dev $tapName mode tap
}

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
    trap del_tap EXIT

    add_tap

    qemu-system-x86_64 \
        -k en \
        -kernel "$kernel" \
        -hda "$image" \
        -display sdl \
        -append "root=/dev/sda console=ttyS0 nokaslr ro" \
        -netdev tap,id=knet0,ifname=$tapName,script=no,downscript=no -device virtio-net-pci,netdev=knet0 \
        -nographic \
#        -s -S
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
