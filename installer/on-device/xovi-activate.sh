#!/bin/sh

set -e

touch /run/xovi-activated

if [ -x /home/root/xovi/start ]; then
    exec /home/root/xovi/start
fi

exit 0
