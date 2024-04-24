#!/bin/bash
PDIR=$(dirname $(readlink -f $0))
. $PDIR/color

if [ `whoami` != "root" ]; then
    echo_c SC_B_RED "Please use this script with Root!"
    exit 1
fi

function gen_udev_rules() {
    ttys=`ls /dev/ttyUSB* 2>/dev/null`

    for t in $ttys
    do
	info=`udevadm info $t | grep -n "ID_VENDOR_ID=1a86"`
	if [ "X$info"  != "X" ]; then
	    tty_relay=$t
	fi

	if [ "X$tty_relay" != "X" ]; then
	    USB_ID=`udevadm info $tty_relay | grep "DEVPATH" | cut -d '/' -f7`
	    break
	fi
    done

    if [ "X$USB_ID" != "X" ]; then
	echo_c SC_GREEN "TTY relay USB_ID = $USB_ID"
	echo "ACTION==\"add\",KERNELS==\"$USB_ID\",SUBSYSTEMS==\"usb\",MODE:=\"0777\",SYMLINK+=\"relay_ctrl\"" > /etc/udev/rules.d/10-local-relay_ctrl.rules
	ln -s $tty_relay /dev/relay_ctrl
	return 0
    else
	echo_c SC_RED "USB_ID is missing, Please check ttyUSB? for relay is plugin..."
	return 2
    fi
}

gen_udev_rules

if [ $? -ne 0 ]; then
    echo_c SC_RED "Generate udev_rules failed. Please check log to fix..."
    exit 2
fi

if [ ! -d /opt/relay_ctrl ]; then
    mkdir -p /opt/relay_ctrl
fi

cp dist/relay_hub /opt/relay_ctrl/
cp dist/relay_ctrl /opt/relay_ctrl/
cp dist/ctrl.ini /opt/relay_ctrl/
cp dist/hub.ini /opt/relay_ctrl/

cp -vf systemd/relay-hub.service /etc/systemd/system/relay-hub.service
systemctl enable relay-hub.service
systemctl start relay-hub.service

echo_c SC_GREEN "Install successfully."
exit 0
