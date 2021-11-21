if [ ! -d /opt/relay_ctrl ]; then
    mkdir -p /opt/relay_ctrl
fi

cp relay_hub /opt/relay_ctrl/
cp relay_ctrl /opt/relay_ctrl/
cp ctrl.ini /opt/relay_ctrl/
cp hub.ini /opt/relay_ctrl/
cp -vf systemd/relay-hub.service /etc/systemd/system/relay-hub.service
systemctl enable relay-hub.service
systemctl start relay-hub.service
