#!/bin/sh

attach_dev() {
  cmd.exe /C usbipd attach --busid $1 --wsl --auto-attach
}

if [ $# -eq 1 ]
then
  echo Attaching a specified device: BUS_ID = $1
  attach_dev $1
  exit 0
fi

dev_name="WCH-Link"
wch_line=$(cd /mnt/c; cmd.exe /C usbipd list | nkf | grep "$dev_name")

if [ "$wch_line" = "" ]
then
  echo $dev_name not found.
  exit 0
else
  echo Showing $dev_name:
  echo "$wch_line"
  echo "To attach a device: '$0 BUS_ID'"
  exit 0
fi
