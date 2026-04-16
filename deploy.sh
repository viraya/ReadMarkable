#!/usr/bin/env bash
set -euo pipefail

DEVICE_IP="${RMPP_IP:-10.11.99.1}"
DEVICE_USER="root"
REMOTE_DIR="/home/root"
BINARY_NAME="readmarkable"
DOCKER_IMAGE="readmarkable-toolchain"
BUILD_DIR="build"
BIN_DIR="build/bin"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

cmd_doctor() {
    info "Checking prerequisites..."
    local ok=true

    if ! command -v docker &>/dev/null; then
        error "docker not found"; ok=false
    else
        info "docker: $(docker --version)"
    fi

    if ! docker image inspect "$DOCKER_IMAGE" &>/dev/null; then
        warn "Docker image '$DOCKER_IMAGE' not built yet. Run: docker build -t $DOCKER_IMAGE ."
    else
        info "Docker image '$DOCKER_IMAGE' exists"
    fi

    if ! command -v ssh &>/dev/null; then
        error "ssh not found"; ok=false
    else
        info "ssh: available"
    fi

    if ssh -o ConnectTimeout=3 -o BatchMode=yes "$DEVICE_USER@$DEVICE_IP" true 2>/dev/null; then
        info "Device reachable at $DEVICE_IP"
    else
        warn "Cannot reach device at $DEVICE_IP (set RMPP_IP env var if different)"
    fi

    $ok && info "All prerequisites OK" || error "Some prerequisites missing"
}

cmd_build() {
    info "Building in Docker container..."

    if ! docker image inspect "$DOCKER_IMAGE" &>/dev/null; then
        error "Docker image not found. Build it first:"
        error "  docker build -t $DOCKER_IMAGE ."
        exit 1
    fi

    MSYS_NO_PATHCONV=1 docker run --rm \
        -v "$(pwd)":/src \
        -w /src \
        "$DOCKER_IMAGE" \
        bash -c "cmake . -B $BUILD_DIR -GNinja -Wno-dev && cmake --build $BUILD_DIR"

    if file "$BIN_DIR/$BINARY_NAME" | grep -q "ARM aarch64"; then
        info "Build successful: AArch64 binary at $BIN_DIR/$BINARY_NAME"
    else
        error "Binary is NOT AArch64! Check your toolchain."
        file "$BIN_DIR/$BINARY_NAME"
        exit 1
    fi
}

cmd_push() {
    info "Pushing to $DEVICE_IP..."

    if [ ! -f "$BIN_DIR/$BINARY_NAME" ]; then
        error "Binary not found at $BIN_DIR/$BINARY_NAME. Run './deploy.sh build' first."
        exit 1
    fi

    scp "$BIN_DIR/$BINARY_NAME" "$DEVICE_USER@$DEVICE_IP:$REMOTE_DIR/$BINARY_NAME"
    ssh "$DEVICE_USER@$DEVICE_IP" "chmod +x $REMOTE_DIR/$BINARY_NAME"
    info "Binary pushed to $DEVICE_USER@$DEVICE_IP:$REMOTE_DIR/$BINARY_NAME"

    if [ -d "fonts" ] && [ "$(ls fonts/*.ttf 2>/dev/null | wc -l)" -gt 0 ]; then
        info "Pushing fonts to /home/root/.readmarkable/fonts/..."
        ssh "$DEVICE_USER@$DEVICE_IP" "mkdir -p /home/root/.readmarkable/fonts"
        scp fonts/*.ttf "$DEVICE_USER@$DEVICE_IP:/home/root/.readmarkable/fonts/"
        info "Fonts pushed to /home/root/.readmarkable/fonts/"
    else
        warn "No .ttf files found in fonts/  -  skipping font push."
        warn "Download fonts per fonts/OFL.txt and re-run './deploy.sh push'."
    fi
}

cmd_run() {
    info "Launching on device..."

    ssh "$DEVICE_USER@$DEVICE_IP" bash -s <<'REMOTE_SCRIPT'
        pkill -9 readmarkable 2>/dev/null || true
        sleep 1

        systemctl stop xochitl 2>/dev/null || true

        cd /home/root
        export QT_QUICK_BACKEND=epaper
        export LANG=en_US.UTF-8
        nohup ./readmarkable -platform epaper > /tmp/readmarkable.log 2>&1 &

        echo "PID: $!"
        echo "Log: /tmp/readmarkable.log"
REMOTE_SCRIPT

    info "App launched. View logs: ssh $DEVICE_USER@$DEVICE_IP 'cat /tmp/readmarkable.log'"
}

cmd_stop() {
    info "Stopping ReadMarkable on device..."
    ssh "$DEVICE_USER@$DEVICE_IP" "pkill -9 readmarkable 2>/dev/null || true; sleep 0.5; systemctl start xochitl 2>/dev/null || true"
    info "Stopped. Xochitl restarted."
}

cmd_logs() {
    ssh "$DEVICE_USER@$DEVICE_IP" "cat /tmp/readmarkable.log 2>/dev/null || echo 'No log file found'"
}

cmd_all() {
    cmd_build
    cmd_push
    cmd_run
}

case "${1:-all}" in
    build)  cmd_build ;;
    push)   cmd_push ;;
    run)    cmd_run ;;
    stop)   cmd_stop ;;
    logs)   cmd_logs ;;
    all)    cmd_all ;;
    doctor) cmd_doctor ;;
    *)
        echo "Usage: $0 {build|push|run|stop|logs|all|doctor}"
        exit 1
        ;;
esac
