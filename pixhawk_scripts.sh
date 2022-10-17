# check whether the device is busy
sudo fuser -k /dev/ttyACM0

# register the device by vendor&product ID and make symlink
sudo echo "SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"26ac\", ATTRS{idProduct}==\"0011\", SYMLINK+=\"ttyPixhawk\"" >> /etc/udev/rules.d/99-pixhawk.rules

# avahi install
sudo apt install avahi-daemon

SERVICE_NAME=rpi-drone # gcs, video-receiver etc (don't use snacke case of service naming)
sudo echo $SERVICE_NAME > /etc/hostname

# IMPORTANT: disable BT on RPI3/4 as bluetooth use the same serial port on rpi as Pixhawk and Pix is not able to read data correctly
sudo apt-get purge bluez -y
sudo apt-get autoremove -y
# BT removed as it doesn't need in our setup

sudo reboot
