#!/bin/sh

set -eu

log() { printf '[overlay-setup] %s\n' "$*" >&2; }

if [ "${RM_MOCK_OVERLAY:-1}" = "0" ]; then
    log "RM_MOCK_OVERLAY=0  -  skipping overlay setup (legacy plain-ext4 mode)"
    exit 0
fi

mkdir -p /mock/etc-lower
cp -a /etc/. /mock/etc-lower/

mkdir -p /var/volatile
if ! awk '$2=="/var/volatile" && $3=="tmpfs"' /proc/mounts | grep -q .; then
    mount -t tmpfs -o mode=0755 tmpfs /var/volatile
fi
mkdir -p /var/volatile/etc /var/volatile/.etc-work

if awk '$2=="/etc" && $3=="overlay"' /proc/mounts | grep -q .; then
    log "overlay already mounted on /etc  -  skipping"
    exit 0
fi

mount -t overlay overlay \
    -o "lowerdir=/mock/etc-lower,upperdir=/var/volatile/etc,workdir=/var/volatile/.etc-work" \
    /etc

log "overlay mounted on /etc (lowerdir=/mock/etc-lower, upperdir tmpfs)"
log "writes to /etc now land on tmpfs upper  -  umount -R /etc is required for persistence"
