#!/bin/bash

set -eu -o pipefail # fail on error , debug all lines

if [ 'root' != $( whoami ) ] ; then
  echo "Please run as root! ( sudo ${0} )"
  exit 1;
fi

#============================================#
#            Adding the host name            #
#============================================#
echo "127.0.0.1   demo1.gate.mekosoft.vn" >> /etc/hosts
echo "127.0.1.1   demo1.gate.mekosoft.vn" >> /etc/hosts

#============================================#
#        Installing the core packages        #
#============================================#
apt-get update && apt-get upgrade
apt-get install git rabbitmq-server mariadb-server wiringpi -y

#============================================#
#      Cloning and building the source       #
#============================================#
git clone https://github.com/minhan74/CheckinGate
cd CheckinGate/main && mkdir obj && mkdir img
make

#============================================#
#          Enabling rabbitmq-server          #
#============================================#
systemctl enable  rabbitmq-server
systemctl start rabbitmq-server

#============================================#
#            Setting up NTP server           #
#============================================#
apt-get install ntp

echo "
restrict 192.168.1.0 mask 255.255.255.0
broadcast 192.168.1.255
broadcast 224.0.1.1
" >> /etc/ntp.conf


#============================================#
#          Setting up MariaDB Server         #
#============================================#
echo "CREATE USER 'username'@'localhost' IDENTIFIED BY 'password';" | mysql
echo "GRANT ALL PRIVILEGES ON * . * TO 'newuser'@'localhost';" | mysql
echo "FLUSH PRIVILEGES;" | mysql


#============================================#
#              Enabling services             #
#============================================#
echo "
[Unit]
Description=Logger service
DefaultDependencies=true
After=multi-user.target

[Service]
Type=simple
ExecStart=java -jar checkingate.jar vn.mekosoft.checkin.logger.Main
WorkingDirectory=$HOME/demo1.checkingate.mekosoft.vn
Restart=always
RestartSec=1000ms

[Install]
WantedBy=sysinit.target multi-user.target

# reference: https://www.raspberrypi.org/forums/viewtopic.php?t=221507
" > /etc/systemd/system/logger.service

echo "
[Unit]
Description=Sensor reader service
DefaultDependencies=true
After=multi-user.target

[Service]
Type=simple
ExecStart=$HOME/demo1.checkingate.mekosoft.vn/run_sensor_reader
WorkingDirectory=$HOME/demo1.checkingate.mekosoft.vn
Restart=always
RestartSec=1000ms

User=$(whoami)

[Install]
WantedBy=sysinit.target multi-user.target

# reference: https://www.raspberrypi.org/forums/viewtopic.php?t=221507
" > /etc/systemd/system/sensor_reader.service

echo "
[Unit]
Description=Web service
DefaultDependencies=true
After=multi-user.target

[Service]
Type=simple
ExecStart=node web/index
WorkingDirectory=$HOME/demo1.checkingate.mekosoft.vn
Restart=always
RestartSec=1000ms

User=$(whoami)

[Install]
WantedBy=sysinit.target multi-user.target

# reference: https://www.raspberrypi.org/forums/viewtopic.php?t=221507
" > /etc/systemd/system/web.service

systemctl enable logger sensor_reader web
systemctl start logger sensor_reader web


#============================================#
#             DISABLING BLUETOOTH            #
#============================================#
if [ 0 -eq $( grep -c 'dtoverlay=disable-bt' /boot/config.txt ) ]; then # check if the phrase "dtoverlay=disable-bt" existed in the file or not
  echo "dtoverlay=disable-bt" | tee -a /boot/config.txt # Disable Bluetooth boot
fi
systemctl disable hciuart.service # Disable systemd service that initializes Bluetooth Modems connected by UART
#systemctl disable bluealsa.service
systemctl disable bluetooth.service
systemctl disable bluetooth cron 
apt-get remove -y bluez
apt-get autoremove -y

echo "enable_uart=1" >> /boot/config.txt


#============================================#
#          CREATING RABBITMQ ACCOUNT         #
#============================================#
rabbitmqctl add_user admin admin123
rabbitmqctl set_user_tags admin administrator
rabbitmqctl set_user_tags admin management
rabbitmqctl set_permissions -p / admin ".*" ".*" ".*"
rabbitmq-plugins enable rabbitmq_management
