systemctl disable relay-hub.service
systemctl stop relay-hub.service
rm -rf /opt/relay_ctrl/
rm -rf /etc/systemd/system/relay-hub.service
