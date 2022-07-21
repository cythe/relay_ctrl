#!/bin/bash

if [ `whoami` != "root" ]; then
    echo Please use this script with Root!
    exit 1
fi

. _gen_udev_rules.sh

if [ ! -d /opt/relay_ctrl ]; then
    mkdir -p /opt/relay_ctrl
fi

cp dist/relay_hub /opt/relay_ctrl/
cp dist/relay_ctrl /opt/relay_ctrl/
cp dist/ctrl.ini /opt/relay_ctrl/
cp dist/hub.ini /opt/relay_ctrl/

_gen_udev_rules

cp -vf systemd/relay-hub.service /etc/systemd/system/relay-hub.service
systemctl enable relay-hub.service
systemctl start relay-hub.service
