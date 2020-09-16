# CheckinGate

## Installation

- Clone the project

```sh
git clone https://github.com/minhan74/CheckinGate
```

- Build and run

```sh
cd CheckinGate/main
make
./main
```

***

## Optional: Setting up NTP server

- Installing ntp

```sh
sudo apt-get install ntp
```

- Configurations: append to `/etc/ntp.conf`
  - `restrict 192.168.1.0 mask 255.255.255.0`
  - `broadcast 192.168.1.255`
  - `broadcast 224.0.1.1`

***

## Optional: Setting up FTP server

- Installing `vsftpd`
  
```sh
sudo apt-get install vsftpd
```

- Configurations: `/etc/vsftpd.conf`
  - `anonymous_enable=NO`: Disable anonymous login
  - `local_enable=YES`: Allow local users to login
  - `write_enable=YES`: Write permission
  - `local_umask=022`: Set umask
  - `chroot_local_user=YES`: Fake root
  - `user_sub_token=$USER`: Generate home directory for virtual users
  - `local_root=/home/$USER/ftp`: Fake root

- Optional: Creating new user

```sh
sudo adduser <username>
mkdir -p /home/<username>/ftp/files
chmod a-w /home/<username>/ftp
```

- Restart the server

```sh
sudo service vsftpd restart
```
