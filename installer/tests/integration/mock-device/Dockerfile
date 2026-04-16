# Mock reMarkable Paper Pro Move (chiappa) test harness.
#
# What this simulates (fidelity: high enough for shell-level integration tests,
# NOT high enough to actually load XOVI's .so  -  that would need aarch64 + Qt):
#
#   /proc/device-tree/model  → fake file "reMarkable Chiappa"
#   /etc/version             → fake firmware string
#   /usr/bin/xochitl          → sleep stub so systemctl can restart it
#   /etc                     → overlay(lowerdir=/etc, upperdir=tmpfs) mounted
#                              by entrypoint.sh  -  matches chiappa topology so
#                              naive writes to /etc/systemd/system/ evaporate
#                              on "reboot" (upper clear) and only survive when
#                              the install script does `umount -R /etc` first
#                              (the r4 persistence design from 2026-04-15).
#   /home                    → plain directory (simulates encrypted-home
#                              unlocked and mounted)
#   BusyBox sh as /bin/sh    → chiappa uses BusyBox; we match it
#   systemctl                → shim that logs invocations AND creates the
#                              expected enable-symlink, so r4's `systemctl
#                              enable xovi-activate.service` produces the
#                              /etc/systemd/system/multi-user.target.wants/
#                              symlink our smoke test asserts on.
#
# Why Alpine: ships BusyBox natively, matching chiappa's userspace environment.
# Why privileged (required on docker-compose side): we actually perform
# `mount -t overlay` and `umount -R /etc` inside the container to simulate
# chiappa behavior  -  those require CAP_SYS_ADMIN.

FROM alpine:3.19

# --- Base tools that chiappa firmware has -----------------------------------
# util-linux brings the "real" mount/umount with -R (recursive) and -q (quiet)
# flags that BusyBox's minimal implementation lacks. Both are needed by:
#   - install-persistence.sh r4 (umount -R /etc)
#   - asivery's /home/root/xovi/start (umount -q on the tmpfs drop-in)
# Chiappa's real firmware has these flags available, so the mock mirrors that.
RUN apk add --no-cache \
    openssh-server \
    busybox-extras \
    openssh-keygen \
    sudo \
    bash \
    curl \
    bind-tools \
    tar \
    gzip \
    unzip \
    python3 \
    util-linux

# --- User + SSH setup -------------------------------------------------------
# reMarkable chiappa puts root's home at /home/root, not /root. Re-home root
# so sshd's "AuthorizedKeysFile .ssh/authorized_keys" resolves correctly.
RUN sed -i 's|^root:x:0:0:root:/root:|root:x:0:0:root:/home/root:|' /etc/passwd && \
    echo 'PermitRootLogin yes'        >> /etc/ssh/sshd_config && \
    echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \
    echo 'PubkeyAuthentication yes'   >> /etc/ssh/sshd_config && \
    ssh-keygen -A && \
    mkdir -p /home/root/.ssh && \
    chmod 700 /home/root/.ssh

# --- Directory structure mirroring chiappa ----------------------------------
RUN mkdir -p \
    /home/root \
    /usr/lib/systemd/system/xochitl.service.d \
    /usr/lib/systemd/system/multi-user.target.d \
    /etc/systemd/system \
    /opt/etc/draft \
    /var/volatile

# --- xochitl stub so systemctl restart / status have something to hit -------
RUN cat > /usr/bin/xochitl <<'EOF'
#!/bin/sh
echo "mock-xochitl: started with args $* env LD_PRELOAD=${LD_PRELOAD:-unset} XOVI_ROOT=${XOVI_ROOT:-unset}" >&2
# Run indefinitely so a "systemctl is-active xochitl" returns active.
exec sleep infinity
EOF
RUN chmod +x /usr/bin/xochitl

# --- xochitl.service (minimal stand-in) -------------------------------------
RUN cat > /usr/lib/systemd/system/xochitl.service <<'EOF'
[Unit]
Description=reMarkable main application (mock)

[Service]
ExecStart=/usr/bin/xochitl --system
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

# --- A shipped override like the real firmware has, so our drop-ins share space
RUN cat > /usr/lib/systemd/system/xochitl.service.d/xochitl-service-override.conf <<'EOF'
[Service]
Environment="MALLOC_ARENA_MAX=8"
EOF

# --- systemctl shim  -  scripts invoke daemon-reload / enable / disable / restart
# This shim extends the previous version in one important way: `systemctl
# enable <unit>` now creates the target-wants symlink at
# /etc/systemd/system/multi-user.target.wants/<unit>, mirroring what real
# systemd does. The r4 smoke test asserts on this symlink's presence.
RUN mv /usr/bin/systemctl /usr/bin/systemctl.real 2>/dev/null || true && \
    cat > /usr/bin/systemctl <<'EOF'
#!/bin/sh
# Mock systemctl  -  logs to /var/log/systemctl-mock.log, returns success, and
# for `enable`/`disable` creates/removes the expected wants-symlink so r4's
# flow (and its smoke-test assertions) behaves like real systemd.
echo "$(date +%s) systemctl $*" >> /var/log/systemctl-mock.log
case "$1" in
    is-active)   echo "active"; exit 0 ;;
    is-enabled)
        unit="$2"
        if [ -L "/etc/systemd/system/multi-user.target.wants/$unit" ] || \
           [ -f "/run/enabled-$unit" ]; then
            echo enabled
        else
            echo disabled
        fi
        exit 0 ;;
    enable)
        shift
        # Skip any option flags like --now.
        for u in "$@"; do
            case "$u" in --*) continue ;; esac
            touch "/run/enabled-$u"
            if [ -f "/etc/systemd/system/$u" ]; then
                mkdir -p /etc/systemd/system/multi-user.target.wants
                ln -sf "/etc/systemd/system/$u" \
                       "/etc/systemd/system/multi-user.target.wants/$u"
            fi
        done
        exit 0 ;;
    disable)
        shift
        for u in "$@"; do
            case "$u" in --*) continue ;; esac
            rm -f "/run/enabled-$u"
            rm -f "/etc/systemd/system/multi-user.target.wants/$u"
        done
        exit 0 ;;
    daemon-reload|reload|start|stop|try-restart|restart) exit 0 ;;
    show)
        shift
        unit="$1"
        shift
        case "$2" in
            Environment) echo "Environment=" ;;
            After)       echo "After=home.mount xochitl.service" ;;
            *)           echo "$2=" ;;
        esac
        exit 0 ;;
    status) echo "mock: status query for $2"; exit 0 ;;
    reboot) touch /tmp/.mock-rebooted; exit 0 ;;
    *) exit 0 ;;
esac
EOF
RUN chmod +x /usr/bin/systemctl

# --- findmnt is not present on chiappa BusyBox; keep it absent here too -----
RUN rm -f /usr/bin/findmnt /bin/findmnt

# --- Entrypoint handles scenario setup, overlay mount, + sshd ---------------
COPY entrypoint.sh /entrypoint.sh
COPY overlay-setup.sh /overlay-setup.sh
RUN chmod +x /entrypoint.sh /overlay-setup.sh

EXPOSE 22
ENTRYPOINT ["/entrypoint.sh"]
