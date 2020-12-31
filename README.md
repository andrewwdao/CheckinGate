# CheckinGate

## Installation

### Clone the project

```git
git clone https://github.com/minhan74/CheckinGate
```

### Build and run

```sh
cd CheckinGate/main && mkdir obj && mkdir img
make
./main
```

***

## Setting up RabbitMQ

### Install rabbitmq-server

```sh
sudo apt-get install rabbitmq-server
```

### Enabling the service

```sh
sudo systemctl enable rabbitmq-server
```

### Running

```sh
sudo systemctl start rabbitmq-server
```

***

## Optional: Setting up NTP server

### Install ntp

```sh
sudo apt-get install ntp
```

### Configurations: append to `/etc/ntp.conf`

- `restrict 192.168.1.0 mask 255.255.255.0`
- `broadcast 192.168.1.255`
- `broadcast 224.0.1.1`

***

## Optional: Setting up FTP server

### Install `vsftpd`
  
```sh
sudo apt-get install vsftpd
```

### Configurations: `/etc/vsftpd.conf`

- `anonymous_enable=NO`: Disable anonymous login
- `local_enable=YES`: Allow local users to login
- `write_enable=YES`: Write permission
- `local_umask=022`: Set umask
- `chroot_local_user=YES`: Fake root
- `user_sub_token=$USER`: Generate home directory for virtual users
- `local_root=/home/$USER/ftp`: Fake root

### Optional: Create new user

```sh
sudo adduser <username>
mkdir -p /home/<username>/ftp/files
chmod a-w /home/<username>/ftp
```

### Restart the server

```sh
sudo service vsftpd restart
```

***

## Optional: Install MariaDB Server (MySQL)

### Install `mariadb-server`
  
```sh
sudo apt-get install mariadb-server
```

### Create admin user

- Run MySQL shell as `root`

```sh
sudo mysql
```

- Create new user `username`

```mysql
CREATE USER 'username'@'localhost' IDENTIFIED BY 'password';
```

- Grant privileges for `username`

```mysql
GRANT ALL PRIVILEGES ON * . * TO 'newuser'@'localhost';
```

- Reload all the privileges

```mysql
FLUSH PRIVILEGES;
```

***

## Optional: Partitioning the SD card

### Shrinking the partition

```sh
sudo parted /dev/mmcblk0
(parted) resizepart
(parted) 2
(parted) 8GB
(parted) yes
```

### Formatting the partition

```sh
mkfs.ext4 /dev/mmcblk0p3
```



## References

- [Setting up NTP server](http://raspberrypi.tomasgreno.cz/ntp-client-and-server.html)
- [Setting up FTP server](https://www.raspberrypi-spy.co.uk/2018/05/creating-ftp-server-with-raspberry-pi/)
- [Creating MySQL user](https://www.digitalocean.com/community/tutorials/how-to-create-a-new-user-and-grant-permissions-in-mysql)
