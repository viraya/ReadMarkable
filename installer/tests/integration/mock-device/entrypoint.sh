#!/bin/sh

set -e

log() { printf '[mock-entrypoint] %s\n' "$*" >&2; }

SCENARIO="${SCENARIO:-stock}"
DEVICE_MODEL="${DEVICE_MODEL:-reMarkable Chiappa}"
FIRMWARE="${FIRMWARE:-20260310084634}"
ROOT_PASSWORD="${ROOT_PASSWORD:-testpass}"

mkdir -p /mock/device-tree
printf '%s' "$DEVICE_MODEL" > /mock/device-tree/model
log "mock device-tree model at /mock/device-tree/model ($DEVICE_MODEL)"

cat > /etc/environment <<EOF
RM_DEVTREE_MODEL=/mock/device-tree/model
RM_FIRMWARE_FILE=/etc/version
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
EOF

mkdir -p /run
chmod 1777 /run

printf '%s\n' "$FIRMWARE" > /etc/version

echo "root:${ROOT_PASSWORD}" | chpasswd

case "$SCENARIO" in
    stock)
        log "scenario: stock (nothing pre-installed)"
        ;;
    xovi-installed)
        log "scenario: XOVI already installed (user-installed)"
        mkdir -p /home/root/xovi
        cat > /home/root/xovi/start <<'EOF'
echo "mock xovi/start called"
EOF
        chmod +x /home/root/xovi/start
        echo "v17-01012026" > /home/root/xovi/.install-tag
        echo "user" > /home/root/xovi/.install-source
        ;;
    xovi-installed-old)
        log "scenario: XOVI installed but too old"
        mkdir -p /home/root/xovi
        cat > /home/root/xovi/start <<'EOF'
echo "mock xovi/start called"
EOF
        chmod +x /home/root/xovi/start
        echo "v01-01012024" > /home/root/xovi/.install-tag
        echo "user" > /home/root/xovi/.install-source
        ;;
    rm-installed-xovi-compat)
        log "scenario: ReadMarkable v1.1 installed atop compatible XOVI"
        mkdir -p /home/root/xovi/extensions.d
        cat > /home/root/xovi/start <<'EOF'
echo "mock xovi/start called"
EOF
        chmod +x /home/root/xovi/start
        echo "v18-23032026" > /home/root/xovi/.install-tag
        echo "readmarkable-installer" > /home/root/xovi/.install-source
        touch /home/root/xovi/extensions.d/appload.so
        echo "v0.5.0" > /home/root/xovi/extensions.d/.appload-install-tag
        echo "readmarkable-installer" > /home/root/xovi/extensions.d/.appload-install-source
        mkdir -p /home/root/.local/share/readmarkable
        echo "v1.1.0" > /home/root/.local/share/readmarkable/VERSION
        touch /home/root/.local/share/readmarkable/readmarkable
        chmod +x /home/root/.local/share/readmarkable/readmarkable
        ;;
    *)
        log "unknown scenario '$SCENARIO', falling back to stock"
        ;;
esac

mkdir -p /home/root/.ssh
touch /home/root/.ssh/authorized_keys
chmod 700 /home/root/.ssh
chmod 600 /home/root/.ssh/authorized_keys

: > /var/log/systemctl-mock.log

/overlay-setup.sh

log "starting sshd on port 22 (scenario=$SCENARIO model='$DEVICE_MODEL' fw=$FIRMWARE)"
exec /usr/sbin/sshd -D -e
