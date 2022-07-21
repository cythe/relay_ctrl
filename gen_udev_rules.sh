#!/bin/bash

ttys=`ls /dev/ttyUSB*`

for t in $ttys
do
        info=`udevadm info $t | grep -n "ID_MODEL_FROM_DATABASE=HL-340 USB-Serial adapter"`
        if [ "X$info"  != "X" ]; then
                tty_relay=$t
        fi

        if [ "X$tty_relay" != "X" ]; then
                USB_ID=`udevadm info $tty_relay | grep "DEVPATH" | cut -d '/' -f7`
                break
        fi
done

if [ "X$USB_ID" != "X" ]; then
        echo $USB_ID
fi

echo "ACTION==\"add\",KERNELS==\"$USB_ID\",SUBSYSTEMS==\"usb\",MODE:=\"0777\",SYMLINK+=\"relay_ctrl\"" > /etc/udev/rules.d/10-local-relay_ctrl.rules
