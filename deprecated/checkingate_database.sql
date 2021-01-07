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
    
