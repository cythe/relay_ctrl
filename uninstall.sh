#!/bin/bash

if [ `whoami` != "root" ]; then
    echo Please use this script with Root!
    exit 1
fi

systemctl disable relay-hub.service
systemctl stop relay-hub.service
rm -rf /opt/relay_ctrl/
rm -f /etc/systemd/system/relay-hub.service
rm -f /etc/udev/rules.d/10-local-relay_ctrl.rules
