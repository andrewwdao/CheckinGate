# CheckinGate

## Installation

***

### Clone the project

```git
git clone https://github.com/minhan74/CheckinGate
```

### Build and run

```sh
cd CheckinGate/main
make
./main
```

***

## Optional: Setting up NTP server

***

### Installing ntp

```sh
sudo apt-get install ntp
```

### Configurations: append to `/etc/ntp.conf`

- `restrict 192.168.1.0 mask 255.255.255.0`
- `broadcast 192.168.1.255`
- `broadcast 224.0.1.1`

***

## Optional: Setting up FTP server

***

### Installing `vsftpd`
  
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

### Optional: Creating new user

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

## Optional: Installing MariaDB Server (MySQL)

***

### Installing `mariadb-server`
  
```sh
sudo apt-get install mariadb-server
```

### Creating admin user

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

## References

***

- [Setting up NTP server](http://raspberrypi.tomasgreno.cz/ntp-client-and-server.html)
- [Setting up FTP server](https://www.raspberrypi-spy.co.uk/2018/05/creating-ftp-server-with-raspberry-pi/)
- [Creating MySQL user](https://www.digitalocean.com/community/tutorials/how-to-create-a-new-user-and-grant-permissions-in-mysql)
