#!/bin/sh
# Copyright (C) 2026 Luke Jenkins
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Kill any existing bridge
killall qmiserial2qmuxd_tcp 2>/dev/null
sleep 1
rm -f /tmp/qmi_bridge.sock

# Launch in new session with inet group for potential future use
setsid /tmp/qmiserial2qmuxd_tcp /tmp/qmi_bridge.sock /var/ </dev/null >/tmp/qmuxd_tcp.log 2>&1 &
sleep 2

# Report status
if [ -S /tmp/qmi_bridge.sock ]; then
    echo "BRIDGE OK - socket exists"
    cat /tmp/qmuxd_tcp.log
else
    echo "BRIDGE FAILED"
    cat /tmp/qmuxd_tcp.log
fi
