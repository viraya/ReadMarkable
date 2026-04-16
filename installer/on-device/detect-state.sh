#!/bin/sh

set -e

json_str() {
    printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' \
                            -e 's/\r//g' -e ':a;N;$!ba;s/\n/\\n/g'
}

read_first_line() {
    [ -r "$1" ] || { printf ''; return; }
    head -n 1 "$1" 2>/dev/null | tr -d '\000\r' | sed 's/[[:space:]]*$//'
}

device_model=$(read_first_line "${RM_DEVTREE_MODEL:-/proc/device-tree/model}")
firmware=$(read_first_line "${RM_FIRMWARE_FILE:-/etc/version}")

rootfs_mount_opts=$(awk '$2=="/" {print $4; exit}' /proc/mounts 2>/dev/null || true)
case ",$rootfs_mount_opts," in
    *,ro,*|ro,*|*,ro) rootfs_mount="ro" ;;
    *,rw,*|rw,*|*,rw) rootfs_mount="rw" ;;
    *)                rootfs_mount="unknown" ;;
esac

xovi_installed=false
xovi_tag=""
xovi_install_source=""
xovi_path="/home/root/xovi"
boot_persistence=false

if [ -d "$xovi_path" ] && [ -x "$xovi_path/start" ]; then
    xovi_installed=true
    xovi_tag=$(read_first_line "$xovi_path/.install-tag")
    [ -z "$xovi_tag" ] && xovi_tag="unknown"
    xovi_install_source=$(read_first_line "$xovi_path/.install-source")
    [ -z "$xovi_install_source" ] && xovi_install_source="user"
    if [ -r /etc/systemd/system/xovi-activate.service ] && \
       systemctl is-enabled xovi-activate.service 2>/dev/null | grep -q '^enabled$'; then
        boot_persistence=true
    fi
fi

appload_installed=false
appload_tag=""
appload_install_source=""
appload_so="/home/root/xovi/extensions.d/appload.so"
if [ -f "$appload_so" ]; then
    appload_installed=true
    appload_install_source=$(read_first_line /home/root/xovi/extensions.d/.appload-install-source)
    [ -z "$appload_install_source" ] && appload_install_source="user"
    appload_tag=$(read_first_line /home/root/xovi/extensions.d/.appload-install-tag)
    [ -z "$appload_tag" ] && appload_tag="unknown"
fi

rm_installed=false
rm_version=""
rm_root="/home/root/.local/share/readmarkable"
if [ -f "$rm_root/VERSION" ] && [ -x "$rm_root/readmarkable" ]; then
    rm_installed=true
    rm_version=$(read_first_line "$rm_root/VERSION")
fi

printf '{'
printf '"device_model":"%s",'    "$(json_str "$device_model")"
printf '"firmware":"%s",'        "$(json_str "$firmware")"
printf '"rootfs_mount":"%s",'    "$rootfs_mount"
printf '"xovi":{'
printf '"installed":%s,'         "$xovi_installed"
printf '"tag":"%s",'             "$(json_str "$xovi_tag")"
printf '"install_source":"%s",'  "$(json_str "$xovi_install_source")"
printf '"path":"%s",'            "$xovi_path"
printf '"boot_persistence":%s'   "$boot_persistence"
printf '},'
printf '"appload":{'
printf '"installed":%s,'         "$appload_installed"
printf '"tag":"%s",'             "$(json_str "$appload_tag")"
printf '"install_source":"%s"'   "$(json_str "$appload_install_source")"
printf '},'
printf '"readmarkable":{'
printf '"installed":%s,'         "$rm_installed"
printf '"version":"%s"'          "$(json_str "$rm_version")"
printf '}'
printf '}\n'
