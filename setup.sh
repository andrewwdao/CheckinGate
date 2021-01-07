#!/bin/bash

set -eu -o pipefail # fail on error , debug all lines

if [ 'root' != $( whoami ) ] ; then
  echo "Please run as root! ( sudo ${0} )"
  exit 1;
fi

# BLACK='\033[0;30m'
# RED='\033[0;31m'
# GREEN='\033[0;32m'
# BROWN='\033[0;33m'
# BLUE='\033[0;34m'
# PURPLE='\033[0;35m'
# CYAN='\033[0;36m'
# LGREY='\033[0;37m'

# DGREY='\033[1;30m'
# LRED='\033[1;31m'
# LGREEN='\033[1;32m'
# YELLOW='\033[1;33m'
# LBLUE='\033[1;34m'
# LPURPLE='\033[1;35m'
# LCYAN='\033[1;36m'
# WHITE='\033[1;37m'

# BG_DEFAULT='\033[49m'
# BG_BLACK='\033[40m'
# BG_RED='\033[41m'
# BG_GREEN='\033[42m'
# BG_YELLOW='\033[43m'
# BG_BLUE='\033[44m'
# BG_PURPLE='\033[45m'
# BG_CYAN='\033[46m'
# BG_LGREY='\033[47m'
# BG_DGREY='\033[100m'
# BG_LRED='\033[101m'
# BG_LGREEN='\033[102m'
# BG_LYELLOW='\033[103m'
# BG_LBLUE='\033[104m'
# BG_LPURPLE='\033[105m'
# BG_LCYAN='\033[106m'
# BG_WHITE='\033[107m'

#============================================#
#          Disabling serial console          #
#============================================#
# Run sudo raspi-config
# Go to Interface -> Serial -> Disable serial console log in

echo ""
echo "================================================"
echo ""
echo "     Disabling serial console (for UHF RS232)"
echo ""
echo "================================================"
echo ""
sed -i "0,/console=serial0,115200 /s///" /boot/cmdline.txt


#============================================#
#            Adding the host name            #
#============================================#

if [ $(grep 'demo1.gate.mekosoft.vn' /etc/hosts | wc -l) -eq 0 ]; then
	echo ""
	echo "================================================"
	echo ""
	echo "         Changing hostname to checkingate"
	echo ""
	echo "================================================"
	echo ""

	# hostname checkingate
	sed -i "s/raspberrypi/checkingate/g" /etc/hosts
	sed -i "s/127.0.1.1/127.0.0.1/g" /etc/hosts
	echo checkingate > /etc/hostname
	
	echo "127.0.0.1   demo1.gate.mekosoft.vn" >> /etc/hosts
	echo "127.0.0.1   demo1.checkingate.mekosoft.vn" >> /etc/hosts

	echo "sudo /home/pi/demo1.checkingate.mekosoft.vn/setup.sh" >> /etc/bash.bashrc

	echo "System rebooting, please log in again to continue the installation"

	reboot
fi


#============================================#
#        Setting up static IP for eth0       #
#============================================#
if [ $(egrep '^interface eth0' /etc/dhcpcd.conf | wc -l) -eq 0 ]; then
	echo ""
	echo "================================================"
	echo ""
	echo "          Setting up static IP for eth0"
	echo ""
	echo "================================================"
	echo ""
	sed -i "0,/#interface eth0/{s/#\(interface eth0\)/\1/}" /etc/dhcpcd.conf
	sed -i "0,/#static ip_address/{s/#\(static ip_address=\).*/\1192\.168\.1\.234\/24/}" /etc/dhcpcd.conf
	sed -i "0,/#static domain_name_servers/{s/#\(static domain_name_servers=\).*/\1192\.168\.1\.1 172\.18\.27\.6 172\.18\.27\.8 8\.8\.8\.8/}" /etc/dhcpcd.conf
fi


#============================================#
#        Setting up IP for web config        #
#============================================#
echo ""
echo "================================================"
echo ""
echo "          Setting up IP for web config"
echo ""
echo "================================================"
echo ""

echo "
HOST=checkingate
PORT=
USERNAMEDB=admin
PASSWORD=admin
DATABASE=checkingate
" > /home/pi/demo1.checkingate.mekosoft.vn/web/config/config.env


#============================================#
#        Installing the core packages        #
#============================================#
echo ""
echo "================================================"
echo ""
echo "           Installing core packages..."
echo ""
echo "================================================"
echo ""
apt-get update -y && apt-get upgrade -y
apt-get install default-jre git rabbitmq-server mariadb-server wiringpi git npm ntp vim python-pip python3-pip ffmpeg -y
sudo -H -u pi bash -c 'pip install pyserial'
sudo -H -u pi bash -c 'pip3 install pyserial wiringpi pika'

#============================================#
#      Cloning and building the source       #
#============================================#
# git clone https://github.com/minhan74/CheckinGate
# cd CheckinGate/main && mkdir obj && mkdir img
# make


#============================================#
#          CREATING RABBITMQ ACCOUNT         #
#============================================#
echo ""
echo "================================================"
echo ""
echo "              Configuring rabbitmq"
echo ""
echo "================================================"
echo ""

rabbitmqctl stop_app
rabbitmqctl reset
rabbitmqctl start_app
rabbitmqctl add_user admin admin
rabbitmqctl set_user_tags admin administrator
rabbitmqctl set_user_tags admin management
rabbitmqctl set_permissions -p / admin ".*" ".*" ".*"
rabbitmq-plugins enable rabbitmq_management

echo "
BusHost=demo1.gate.mekosoft.vn
BusAccount=admin
BusPassword=admin
# PirScript=/home/pi/demo1.checkingate.mekosoft.vn/resources/pir
# RfidScript=/home/pi/demo1.checkingate.mekosoft.vn/resources/rfid
# FrontCameraScript=./resources/camera-109.sh
# RearCameraScript=./resources/camera-108.sh
# PhotoFolder=./resources/photos
DbHost=localhost
DbAccount=admin
DbPassword=admin
PirSoundFile=/home/pi/demo1.checkingate.mekosoft.vn/audio/TakePhoto.wav
RfidSoundFile=/home/pi/demo1.checkingate.mekosoft.vn/audio/Checkin.wav
WelcomeSoundFile=/home/pi/demo1.checkingate.mekosoft.vn/audio/WelcomeToCheckinGate.wav
" > /home/pi/demo1.checkingate.mekosoft.vn/config.properties

if [ -f /home/pi/demo1.checkingate.mekosoft.vn/vn.mekosoft.checkin.logger.QueueManager.jar ]; then
	echo "Running java -jar vn.mekosoft.checkin.logger.QueueManager.jar"
	cp /home/pi/demo1.checkingate.mekosoft.vn/config.properties /tmp
	#java -jar vn.mekosoft.checkin.logger.QueueManager.jar
	su -c "java -jar /home/pi/demo1.checkingate.mekosoft.vn/vn.mekosoft.checkin.logger.QueueManager.jar" pi
fi


#============================================#
#          Enabling rabbitmq-server          #
#============================================#
echo ""
echo "================================================"
echo ""
echo "        Enabling rabbitmq-server service"
echo ""
echo "================================================"
echo ""
systemctl enable rabbitmq-server
systemctl restart rabbitmq-server


#============================================#
#            Setting up NTP server           #
#============================================#
if [ $(grep 'restrict 192.168.1.0 mask 255.255.255.0' /etc/ntp.conf | wc -l) -eq 0 ]; then
	echo ""
	echo "================================================"
	echo ""
	echo "            Configuring NTP Server"
	echo ""
	echo "================================================"
	echo ""
	echo "
	restrict 192.168.1.0 mask 255.255.255.0
	broadcast 192.168.1.255
	broadcast 224.0.1.1
	" >> /etc/ntp.conf
fi


#============================================#
#          Setting up MariaDB Server         #
#============================================#
echo ""
echo "================================================"
echo ""
echo "           Setting up MariaDB Server"
echo ""
echo "================================================"
echo ""

echo "drop user if exists admin@localhost;" | mysql
echo "FLUSH PRIVILEGES;" | mysql
echo "DELETE FROM mysql.user WHERE User = 'admin';" | mysql
echo "CREATE USER 'admin'@'localhost' IDENTIFIED BY 'admin';" | mysql
echo "GRANT ALL PRIVILEGES ON * . * TO 'admin'@'localhost';" | mysql
echo "FLUSH PRIVILEGES;" | mysql
echo "
drop database if exists checkingate;
create database checkingate;
use checkingate;

create table checkin_events(
	id int(11) auto_increment primary key,
	gateId varchar(255) not null ,
    timestamp bigint(20) not null ,
    datetime datetime not null ,
    direction varchar(255) not null ,
    rfidTag varchar(255) not null ,
    personalName varchar(255) not null ,
    personalCode varchar(255) not null);
    
create table events(
	id int(11) auto_increment primary key,
    version varchar(255) not null ,
    host varchar(255) not null ,
    timestamp bigint(20) not null ,
    event_type varchar(255) not null ,
    source varchar(255) not null ,
    short_message varchar(255) not null ,
    full_message text not null ,
    data varchar(255) not null);
    
create table nhan_vien(
	id int(11) auto_increment primary key,
    manv varchar(255) not null ,
    rfid_tag varchar(255) not null ,
    ho_ten varchar(255) not null ,
    don_vi varchar(255) not null ,
    dien_thoai varchar(255) not null ,
    email varchar(255) not null);
" | mysql


echo "Done"


#============================================#
#          INSTALLING NODE PACKAGES          #
#============================================#
echo ""
echo "================================================"
echo ""
echo "            Setting up node packages"
echo ""
echo "================================================"
echo ""

cd /home/pi/demo1.checkingate.mekosoft.vn/web
npm cache verify
npm install


#============================================#
#          COMPILING SENSOR READER           #
#============================================#
echo ""
echo "================================================"
echo ""
echo "            Compiling sensor reader..."
echo ""
echo "================================================"
echo ""

sed -i 's/\r$//' /home/pi/demo1.checkingate.mekosoft.vn/sensor_reader/src/cam
chmod +x /home/pi/demo1.checkingate.mekosoft.vn/sensor_reader/src/cam

cd /home/pi/demo1.checkingate.mekosoft.vn/sensor_reader
make clean
make > /dev/null



#============================================#
#              Enabling services             #
#============================================#
echo ""
echo "================================================"
echo ""
echo "           Creating service files..."
echo ""
echo "================================================"
echo ""

echo "Changing timezone to Asia/Ho_Chi_Minh"
timedatectl set-timezone Asia/Ho_Chi_Minh

echo "
[Unit]
Description=Logger service
DefaultDependencies=true
After=multi-user.target

[Service]
Type=simple
ExecStart=java -jar checkingate.jar vn.mekosoft.checkin.logger.Main
WorkingDirectory=/home/pi/demo1.checkingate.mekosoft.vn
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
ExecStart=/home/pi/demo1.checkingate.mekosoft.vn/run_sensor_reader
WorkingDirectory=/home/pi/demo1.checkingate.mekosoft.vn
Restart=always
RestartSec=1000ms

User=pi

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
WorkingDirectory=/home/pi/demo1.checkingate.mekosoft.vn
Restart=always
RestartSec=1000ms

User=pi

[Install]
WantedBy=sysinit.target multi-user.target

# reference: https://www.raspberrypi.org/forums/viewtopic.php?t=221507
" > /etc/systemd/system/web.service

echo "
Description=RESTART_SENSOR_READER_SERVICE

Wants=network.target
After=syslog.target network-online.target

[Service]
Type=simple
ExecStartPre=echo \"RELOAD SENSOR READER SERVICE\"
ExecStart=systemctl restart sensor_reader.service
Restart=always
RestartSec=30
KillMode=process

[Install]
WantedBy=multi-user.target
" > /etc/systemd/system/reload_sensor_reader.service

echo "
[Unit]
Description=usb switch-mode service
DefaultDependencies=true
After=multi-user.target

[Service]
Type=oneshot
ExecStartPre=sleep 7
ExecStart=usb_modeswitch -W -I -v 12d1 -p 1446 -M 55534243123456780000000000000011062000000100010100000000000000

[Install]
WantedBy=multi-user.target

# reference: https://www.raspberrypi.org/forums/viewtopic.php?t=221507
"  > /etc/systemd/system/sms_enabler.service

echo "Enabling services"
systemctl daemon-reload
systemctl enable logger sensor_reader web sms_enabler #reload_sensor_reader
systemctl start logger sensor_reader web sms_enabler #reload_sensor_reader


#============================================#
#             DISABLING BLUETOOTH            #
#============================================#
echo ""
echo "================================================"
echo ""
echo "              Disabling bluetooth"
echo ""
echo "================================================"
echo ""

if [ 0 -eq $( grep -c 'dtoverlay=disable-bt' /boot/config.txt ) ]; then # check if the phrase "dtoverlay=disable-bt" existed in the file or not
  echo "dtoverlay=disable-bt" | tee -a /boot/config.txt # Disable Bluetooth boot
fi
systemctl disable hciuart.service # Disable systemd service that initializes Bluetooth Modems connected by UART
#systemctl disable bluealsa.service
systemctl disable bluetooth.service
systemctl disable bluetooth cron 
apt-get remove -y bluez > /dev/null
apt-get autoremove -y > /dev/null

if [ $(cat /boot/config.txt | egrep 'enable_uart=1' | wc -l) -eq 0 ]; then
	echo "Disabling UART..."
	echo "enable_uart=1" >> /boot/config.txt
fi


#============================================#
#            Partitioning the card           #
#============================================#



#============================================#
#                    RTC                     #
#============================================#
echo ""
echo "================================================"
echo ""
echo "                Configuring RTC..."
echo ""
echo "================================================"
echo ""

FLAGDIR=/home

echo "Please make sure this device is connected to the internet."
echo "This setup will automatically restart the system."
# echo -n "Confirm to install? [Y/N]: "
# read input
# if ! [ $input == "y" ] || [ $input == "Y" ]; then
#   { echo "Exiting..."; exit 1; }
# fi

## https://raspberry-projects.com/pi/programming-in-python/i2c-programming-in-python/using-the-i2c-interface-2
if [ 0 -eq $( grep -c 'i2c-dev' /etc/modules ) ] ; then # check if the phrase "i2c-dev" existed in the file or not
  echo "i2c-dev" | tee -a /etc/modules
fi
if [ 0 -eq $( grep -c 'i2c-bcm2708' /etc/modules ) ] ; then # check if the phrase existed in the file or not
  echo "i2c-bcm2708" | tee -a /etc/modules
fi
if [ 0 -eq $( grep -c 'dtoverlay=i2c-rtc,ds3231' /boot/config.txt ) ] ; then # check if the phrase existed in the file or not
  echo "dtoverlay=i2c-rtc,ds3231" | tee -a /boot/config.txt
fi

# apt-get install -y python-smbus python3-smbus i2c-tools

apt-get -y remove fake-hwclock
update-rc.d -f fake-hwclock remove
systemctl disable fake-hwclock

sed -i -e '/^if \[ -e \/run\/systemd\/system \] ; then$/,+2 s/^/#/' /lib/udev/hwclock-set
sed -i -e '/^.*\/sbin\/hwclock --rtc=\$dev --systz --badyear$/ s/^/#/' /lib/udev/hwclock-set
sed -i -e '/^.*\/sbin\/hwclock --rtc=\$dev --systz$/ s/^/#/' /lib/udev/hwclock-set

touch $FLAGDIR/rtc_setup_flag
echo "
  	sudo ifconfig eth0 down
	sleep 2

	# automatically get and set time from the internet (workaround for proxy setting)
	date -s "$(wget -qSO- --timeout=10 --max-redirect=0 google.vn 2>&1 | grep Date: | cut -d' ' -f5-8)Z"
	hwclock --verbose -r
	hwclock -w
	sleep 2

	sudo ifconfig eth0 up

	rm -f $FLAGDIR/rtc_setup_flag
	update-rc.d -f rtc_setup remove
	rm /etc/init.d/rtc_setup
" > /etc/init.d/rtc_setup
update-rc.d rtc_setup defaults

echo "DONE! REBOOTING"

sed -i "s/sudo \/home\/pi\/demo1.checkingate.mekosoft.vn\/setup.sh//" /etc/bash.bashrc

reboot