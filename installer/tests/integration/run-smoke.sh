#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLER_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
REPO_ROOT="$(cd "$INSTALLER_DIR/.." && pwd)"

log()  { printf '\033[36m[smoke]\033[0m %s\n' "$*"; }
pass() { printf '\033[32m  PASS\033[0m %s\n' "$*"; }
fail() { printf '\033[31m  FAIL\033[0m %s\n' "$*" >&2; exit 1; }

SSH_OPTS=(
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o LogLevel=ERROR
    -p 2222
)
SSH="ssh ${SSH_OPTS[*]} root@localhost"
SCP_OPTS=(
    -P 2222
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o LogLevel=ERROR
)

wait_for_ssh() {
    log "waiting for sshd TCP on localhost:2222"
    for i in $(seq 1 60); do
        if bash -c "echo > /dev/tcp/localhost/2222" 2>/dev/null; then
            pass "sshd TCP reachable (attempt $i)"
            return 0
        fi
        sleep 2
    done
    fail "sshd TCP never came up"
}

assert_file_exists_on_mock() {
    path="$1"
    if $SSH "[ -f '$path' ]"; then
        pass "exists on mock: $path"
    else
        fail "missing on mock: $path"
    fi
}

assert_file_contains() {
    path="$1"
    needle="$2"
    if $SSH "grep -q '$needle' '$path'"; then
        pass "$path contains '$needle'"
    else
        fail "$path missing '$needle'"
    fi
}

setup_key_auth() {
    tmpkey="$(mktemp -u)"
    rm -f "$tmpkey" "$tmpkey.pub"
    ssh-keygen -t ed25519 -N '' -f "$tmpkey" -q
    log "uploading test key (using docker exec to bypass initial password auth)"
    docker exec -i rm-mock-chiappa sh -c '
        mkdir -p /home/root/.ssh &&
        cat >> /home/root/.ssh/authorized_keys
    ' < "$tmpkey.pub"
    SSH_OPTS+=(-i "$tmpkey" -o PreferredAuthentications=publickey)
    SSH="ssh ${SSH_OPTS[*]} root@localhost"
    SCP_OPTS+=(-i "$tmpkey" -o PreferredAuthentications=publickey)
    export TMPKEY="$tmpkey"
}

cleanup() {
    log "tearing down mock container"
    (cd "$SCRIPT_DIR" && docker compose down -v 2>/dev/null || true)
    [ -n "${TMPKEY:-}" ] && rm -f "$TMPKEY" "$TMPKEY.pub"
}
trap cleanup EXIT

simulate_reboot() {
    log "simulated reboot: tear down /etc overlay, clear tmpfs upper, remount"
    docker exec rm-mock-chiappa sh -c '
        set -e
        if awk "\$2==\"/etc\" && \$3==\"overlay\"" /proc/mounts | grep -q .; then
            umount -R /etc
        fi
        rm -rf /mock/etc-lower
        mkdir -p /mock/etc-lower
        if [ "$(ls -A /etc 2>/dev/null)" ]; then
            cp -a /etc/. /mock/etc-lower/
        fi
        rm -rf /var/volatile/etc /var/volatile/.etc-work
        mkdir -p /var/volatile/etc /var/volatile/.etc-work
        mount -t overlay overlay \
            -o "lowerdir=/mock/etc-lower,upperdir=/var/volatile/etc,workdir=/var/volatile/.etc-work" \
            /etc
    '
}

if [ ! -f "$INSTALLER_DIR/payload/xovi-aarch64.tar.gz" ]; then
    log "payload missing  -  running fetch-bundled-payload.sh first"
    bash "$INSTALLER_DIR/scripts/fetch-bundled-payload.sh"
fi

log "building + starting mock-chiappa (scenario=stock)"
(cd "$SCRIPT_DIR" && SCENARIO=stock docker compose up -d --build)

wait_for_ssh
setup_key_auth

log "verifying /etc overlay is mounted in the mock"
if $SSH 'awk "\$2==\"/etc\" && \$3==\"overlay\"" /proc/mounts | grep -q .'; then
    pass "/etc is overlay-mounted (chiappa-topology fidelity)"
else
    fail "/etc is NOT overlay-mounted  -  overlay-setup.sh must have failed. Aborting."
fi

log "uploading bundled payload + on-device scripts to /tmp/rm-install-test/"
$SSH "mkdir -p /tmp/rm-install-test"
scp "${SCP_OPTS[@]}" \
    "$INSTALLER_DIR/payload/xovi-aarch64.tar.gz" \
    "$INSTALLER_DIR/payload/appload-aarch64.zip" \
    "$INSTALLER_DIR/on-device/"*.sh \
    "$INSTALLER_DIR/on-device/xovi-activate.service" \
    root@localhost:/tmp/rm-install-test/ >/dev/null

log "PHASE 1: detect-state on stock"
state=$($SSH "sh /tmp/rm-install-test/detect-state.sh")
log "  got: $state"
echo "$state" | grep -q '"installed":true' && fail "XOVI should NOT be installed on stock" || pass "stock state: XOVI not installed"

log "PHASE 2: install-xovi"
$SSH "sh /tmp/rm-install-test/install-xovi.sh /tmp/rm-install-test/xovi-aarch64.tar.gz v18-23032026 readmarkable-installer"
assert_file_exists_on_mock /home/root/xovi/start
assert_file_exists_on_mock /home/root/xovi/xovi.so
assert_file_exists_on_mock /home/root/xovi/.install-tag
assert_file_exists_on_mock /home/root/xovi/.install-source
assert_file_contains /home/root/xovi/.install-tag v18-23032026
assert_file_contains /home/root/xovi/.install-source readmarkable-installer

log "PHASE 3: install-appload"
$SSH "sh /tmp/rm-install-test/install-appload.sh /tmp/rm-install-test/appload-aarch64.zip v0.5.0 readmarkable-installer"
assert_file_exists_on_mock /home/root/xovi/extensions.d/appload.so
assert_file_exists_on_mock /home/root/xovi/extensions.d/.appload-install-tag
assert_file_contains /home/root/xovi/extensions.d/.appload-install-tag v0.5.0

log "PHASE 3b: rebuild-hashtab"
log "  installing mock rebuild_hashtable stub (real one needs a live xochitl)"
$SSH 'cat > /home/root/xovi/rebuild_hashtable <<"STUB"
cat >/dev/null
mkdir -p /home/root/xovi/exthome/qt-resource-rebuilder
printf "mock-hashtab-bytes" > /home/root/xovi/exthome/qt-resource-rebuilder/hashtab
echo "[qmldiff]: Hashtab saved to /home/root/xovi/exthome/qt-resource-rebuilder/hashtab"
STUB
chmod +x /home/root/xovi/rebuild_hashtable'

$SSH "sh /tmp/rm-install-test/rebuild-hashtab.sh"
assert_file_exists_on_mock /home/root/xovi/exthome/qt-resource-rebuilder/hashtab
assert_file_contains /home/root/xovi/exthome/qt-resource-rebuilder/hashtab mock-hashtab

log "PHASE 3c: install-home-tile"
log "  seeding home-tile payload for happy path"
$SSH "mkdir -p /tmp/rm-install-test/home-tile"
MOCK_FW=$($SSH "cat /etc/version")
log "  mock firmware (from /etc/version): $MOCK_FW"
MOCK_QMD="readmarkable-${MOCK_FW}.qmd"
tmp_manifest=$(mktemp)
cat > "$tmp_manifest" <<MANIFEST
icon_png = "readmarkable-icon.png"

[[firmware]]
version = "$MOCK_FW"
qmd     = "$MOCK_QMD"
MANIFEST
scp "${SCP_OPTS[@]}" "$tmp_manifest" \
    root@localhost:/tmp/rm-install-test/home-tile/manifest.toml >/dev/null
rm -f "$tmp_manifest"
$SSH "printf 'mock-png-bytes' > /tmp/rm-install-test/home-tile/readmarkable-icon.png"
$SSH "printf 'mock-qmd-bytes' > /tmp/rm-install-test/home-tile/$MOCK_QMD"

$SSH "RM_FIRMWARE_FILE=/etc/version sh /tmp/rm-install-test/install-home-tile.sh /tmp/rm-install-test/home-tile"

assert_file_exists_on_mock "/home/root/xovi/exthome/qt-resource-rebuilder/$MOCK_QMD"
assert_file_exists_on_mock /home/root/xovi/exthome/qt-resource-rebuilder/readmarkable-icon.png
assert_file_contains "/home/root/xovi/exthome/qt-resource-rebuilder/$MOCK_QMD" mock-qmd-bytes
assert_file_contains /home/root/xovi/exthome/qt-resource-rebuilder/readmarkable-icon.png mock-png-bytes

log "  silent-degrade path (firmware mismatch)"
tmp_manifest=$(mktemp)
cat > "$tmp_manifest" <<'MANIFEST'
icon_png = "readmarkable-icon.png"

[[firmware]]
version = "99.99.99.99"
qmd     = "readmarkable-99.99.99.99.qmd"
MANIFEST
scp "${SCP_OPTS[@]}" "$tmp_manifest" \
    root@localhost:/tmp/rm-install-test/home-tile/manifest.toml >/dev/null
rm -f "$tmp_manifest"

$SSH "rm -f /home/root/xovi/exthome/qt-resource-rebuilder/readmarkable-*.qmd /home/root/xovi/exthome/qt-resource-rebuilder/readmarkable-icon.png"
OUT=$($SSH "RM_FIRMWARE_FILE=/etc/version sh /tmp/rm-install-test/install-home-tile.sh /tmp/rm-install-test/home-tile 2>&1")
if echo "$OUT" | grep -q "AppLoad drawer tile retained"; then
    pass "silent-degrade log line emitted"
else
    fail "silent-degrade path did not log expected message; got: $OUT"
fi
if $SSH "ls /home/root/xovi/exthome/qt-resource-rebuilder/readmarkable-*.qmd 2>/dev/null"; then
    fail "firmware-mismatch path unexpectedly created a .qmd"
else
    pass "no .qmd created on firmware mismatch"
fi

log "  verifying idempotence fast-path on repeat invocation"
hashtab_path=/home/root/xovi/exthome/qt-resource-rebuilder/hashtab
before_mtime=$($SSH "stat -c %Y $hashtab_path")
sleep 1
$SSH "sh /tmp/rm-install-test/rebuild-hashtab.sh" >/dev/null
after_mtime=$($SSH "stat -c %Y $hashtab_path")
if [ "$before_mtime" = "$after_mtime" ]; then
    pass "idempotence fast-path: hashtab mtime unchanged ($before_mtime)"
else
    fail "idempotence failed: mtime was $before_mtime, now $after_mtime  -  script regenerated unnecessarily"
fi

log "PHASE 4: install-persistence (r4  -  the load-bearing redesigned step)"
$SSH "sh /tmp/rm-install-test/install-persistence.sh /tmp/rm-install-test"

assert_file_exists_on_mock /etc/systemd/system/xovi-activate.service
assert_file_contains /etc/systemd/system/xovi-activate.service "Wants=home.mount"
assert_file_contains /etc/systemd/system/xovi-activate.service "ExecStart=/home/root/xovi-activate/xovi-activate.sh"
assert_file_exists_on_mock /home/root/xovi-activate/xovi-activate.sh
if $SSH "[ -L /etc/systemd/system/multi-user.target.wants/xovi-activate.service ]"; then
    pass "multi-user.target.wants/xovi-activate.service symlink created (enable)"
else
    fail "missing enable symlink at /etc/systemd/system/multi-user.target.wants/xovi-activate.service"
fi

log "  checking xochitl.service.d integrity (critical invariant)"
dropins=$($SSH "ls /usr/lib/systemd/system/xochitl.service.d/")
if echo "$dropins" | grep -q xovi; then
    fail "INVARIANT VIOLATED: xochitl.service.d contains xovi drop-in (stock should only have xochitl-service-override.conf)"
fi
pass "xochitl.service.d untouched by install-persistence (stock invariant holds)"

log "PHASE 4b: simulated reboot  -  verify writes survive the overlay re-mount"
simulate_reboot
assert_file_exists_on_mock /etc/systemd/system/xovi-activate.service
if $SSH "[ -L /etc/systemd/system/multi-user.target.wants/xovi-activate.service ]"; then
    pass "enable symlink survived simulated reboot"
else
    fail "enable symlink did NOT survive simulated reboot  -  r4 persistence broken"
fi
assert_file_exists_on_mock /home/root/xovi-activate/xovi-activate.sh

if $SSH 'awk "\$2==\"/etc\" && \$3==\"overlay\"" /proc/mounts | grep -q .'; then
    pass "/etc overlay re-established after simulated reboot"
else
    fail "/etc overlay was not re-mounted by simulate_reboot"
fi

log "PHASE 5: uninstall --remove-persistence --remove-xovi"
$SSH "sh /tmp/rm-install-test/uninstall.sh --remove-persistence --remove-xovi"

if $SSH "! [ -f /etc/systemd/system/xovi-activate.service ]"; then
    pass "xovi-activate.service removed"
else
    fail "xovi-activate.service still present after uninstall"
fi
if $SSH "! [ -L /etc/systemd/system/multi-user.target.wants/xovi-activate.service ]"; then
    pass "wants-symlink removed"
else
    fail "wants-symlink still present after uninstall"
fi
if $SSH "! [ -d /home/root/xovi-activate ]"; then
    pass "/home/root/xovi-activate removed"
else
    fail "/home/root/xovi-activate still present after uninstall"
fi
if $SSH "! [ -d /home/root/xovi ]"; then
    pass "/home/root/xovi removed"
else
    fail "XOVI dir still present"
fi

log "PHASE 5b: simulated reboot  -  verify removals survive"
simulate_reboot
if $SSH "! [ -f /etc/systemd/system/xovi-activate.service ]"; then
    pass "unit file stays gone after simulated reboot"
else
    fail "unit file reappeared after simulated reboot  -  uninstall didn't persist"
fi

log "PHASE 6: detect-state after full uninstall"
state=$($SSH "sh /tmp/rm-install-test/detect-state.sh")
log "  got: $state"
echo "$state" | grep -q '"installed":false' && pass "XOVI reports uninstalled" || fail "expected XOVI installed=false"

log ""
log "=================================================="
log "ALL SMOKE TESTS PASSED  -  r4 overlay-defeat holds "
log "=================================================="
