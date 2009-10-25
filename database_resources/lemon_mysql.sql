# (C) 2007-2009, Miguel Chavez Gamboa
# run this as: cat lemon_mysql.sql | mysql -u root -p

CREATE DATABASE lemonposdb;
USE lemonposdb;

CREATE TABLE IF NOT EXISTS `transactions` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `clientid` int(10) unsigned NOT NULL,
  `userid` int(10) NOT NULL default '0',
  `type` smallint(5) unsigned default NULL,
  `amount` double unsigned NOT NULL default '0',
  `date` date NOT NULL,
  `time` time NOT NULL,
  `paidwith` double unsigned NOT NULL default '0.0',
  `changegiven` double NOT NULL default '0',
  `paymethod` int(10) NOT NULL default '0',
  `state` int(10) NOT NULL default '0',
  `cardnumber` varchar(20) character set utf8 collate utf8_general_ci, #Card/Check
  `itemcount` int(10) unsigned NOT NULL default '0',
  `itemslist` varchar(250) character set utf8 collate utf8_general_ci NOT NULL,
  `points` bigint(20) unsigned NOT NULL default '0',
  `discmoney` double NOT NULL default '0',
  `disc` double NOT NULL default '0',
  `cardauthnumber` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  `profit` double NOT NULL default '0', #RENAMED
  `terminalnum` int(10) unsigned NOT NULL default '1',
  `providerid` int(10) unsigned NOT NULL, #for Purchase orders
  `groups` VARCHAR(50), -- to indicate there are groups sold in this transaction.
 PRIMARY KEY  (`id`, `clientid`, `type`, `date`, `time`, `state`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;


CREATE TABLE IF NOT EXISTS `products` (
  `code` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(255) NOT NULL collate utf8_general_ci default 'unknown',
  `price` double unsigned NOT NULL default '0.0',
  `cost` double unsigned NOT NULL default '0',
  `stockqty` double unsigned NOT NULL default '0',
  `brandid` bigint(20) unsigned NOT NULL default '0',
  `units` int(10) unsigned collate utf8_general_ci NOT NULL default '0',
  `taxmodel` bigint(20) unsigned NOT NULL default 1,
  `photo` blob default NULL,
  `category` int(10) unsigned NOT NULL default 0,
  `points` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `alphacode` VARCHAR( 30 ) NULL,
  `lastproviderid` int(10) unsigned NOT NULL default '1',
  `soldunits` double unsigned NOT NULL default '0',
  `datelastsold` date ,
 PRIMARY KEY  (`code`, `name`, `brandid`, `alphacode`),
 KEY `SEC` (`category`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `group_elements` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `gid` bigint(20) unsigned NOT NULL,
  `product_id` bigint(20) unsigned NOT NULL,
  `qty` bigint(20) unsigned NOT NULL,
  PRIMARY KEY  (`id`, `gid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `groups` (
  `groupid` bigint(20) unsigned NOT NULL auto_increment,
  `gname` VARCHAR(50) NOT NULL,
  `discount` double unsigned NOT NULL DEFAULT '0',
  `soldunits` int(10) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY  (`groupid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `offers` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `discount` double NOT NULL,
  `datestart` date NOT NULL,
  `dateend` date NOT NULL,
  `product_id` bigint(20) unsigned NOT NULL,
  PRIMARY KEY  (`id`, `product_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `measures` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `categories` (
  `catid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`catid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `brands` (
  `brandid` bigint(20) unsigned NOT NULL auto_increment,
  `bname` VARCHAR(50) NOT NULL,
  PRIMARY KEY  (`brandid`, `bname`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `taxmodels` (
  `modelid` bigint(20) unsigned NOT NULL auto_increment,
  `tname` VARCHAR(50) NOT NULL,
  `appway` VARCHAR(50) NOT NULL,
  `elementsid` VARCHAR(50) NOT NULL,
  PRIMARY KEY  (`modelid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `taxelements` (
  `elementid` bigint(20) unsigned NOT NULL auto_increment,
  `ename` VARCHAR(50) NOT NULL,
  `amount` double unsigned NOT NULL,
  PRIMARY KEY  (`elementid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `balances` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `datetime_start` datetime NOT NULL,
  `datetime_end` datetime NOT NULL,
  `userid` bigint(20) unsigned NOT NULL,
  `usern` varchar(50) collate utf8_general_ci NOT NULL,
  `initamount` double NOT NULL,
  `in` double NOT NULL,
  `out` double NOT NULL,
  `cash` double NOT NULL,
  `card` double NOT NULL,
  `transactions` varchar(250) collate utf8_general_ci NOT NULL,
  `terminalnum` bigint(20) unsigned NOT NULL,
  PRIMARY KEY  (`id`, `datetime_start`, `datetime_end`, `userid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `invoices` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `client_id` int(10) unsigned NOT NULL,
  `transaction_id` bigint(20) unsigned NOT NULL,
  `total` double unsigned NOT NULL default '0',
  `subtotal` double unsigned NOT NULL default '0',
  `taxAmount` double unsigned NOT NULL default '0',
  `date` date NOT NULL,
  `time` time NOT NULL,
  PRIMARY KEY  (`id`, `client_id`, `transaction_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;


CREATE TABLE IF NOT EXISTS `users` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `username` varchar(50) collate utf8_general_ci NOT NULL default '',
  `password` varchar(50) collate utf8_general_ci default NULL,
  `salt` varchar(5) collate utf8_general_ci default NULL,
  `name` varchar(100) collate utf8_general_ci default NULL,
  `address` varchar(255) collate utf8_general_ci default NULL,
  `phone` varchar(50) character set utf8 collate utf8_general_ci default NULL,
  `phone_movil` varchar(50) collate utf8_general_ci default NULL,
  `role` int(10) unsigned default '0',
  `photo` blob default NULL,
  PRIMARY KEY (`id`,`username`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `clients` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `name` varchar(100) collate utf8_general_ci default NULL,
  `since` date NOT NULL,
  `address` varchar(255) collate utf8_general_ci default NULL,
  `phone` varchar(50) character set utf8 collate utf8_general_ci default NULL,
  `phone_movil` varchar(50) collate utf8_general_ci default NULL,
  `taxid` varchar(100) collate utf8_general_ci default NULL, #Renombrar luego. USA:EIN/FTIN/taxid
  `points` bigint(20) unsigned default '0',
  `discount` double NOT NULL,
  `photo` blob default NULL,
  PRIMARY KEY (`id`, `name`, `taxid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `paytypes` (
  `typeid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`typeid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `transactionstates` (
  `stateid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`stateid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `transactiontypes` (
  `ttypeid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`ttypeid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `transactionitems` (
 `transaction_id` bigint(20) unsigned NOT NULL,
 `position` int(10) unsigned NOT NULL,
 `product_id` bigint(20) unsigned NOT NULL,
 `qty` double default NULL,
 `points` double default NULL,
 `unitstr` varchar(50) default NULL,
 `cost` double default NULL,
 `price` double default NULL,
 `disc` double default NULL,
 `total` double default NULL,
 `name` varchar(255) default NULL,
 UNIQUE KEY `transaction_id` (`transaction_id`,`position`),
 KEY `product_id` (`product_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `cashflow` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `type` smallint(5) unsigned NOT NULL default '1',
  `userid` bigint(20) NOT NULL default '1',
  `reason` varchar(100) default NULL,
  `amount` double unsigned NOT NULL default '0',
  `date` date NOT NULL,
  `time` time NOT NULL,
  `terminalnum` int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (`id`, `date`, `time`, `type`, `userid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `cashflowtypes` (
  `typeid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`typeid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `providers` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `provname` VARCHAR( 20 ) NULL,
  `address` varchar(255) collate utf8_general_ci default NULL,
  `phone` varchar(50) character set utf8 collate utf8_general_ci default NULL,
  `cellphone` varchar(50) collate utf8_general_ci default NULL,
  PRIMARY KEY  (`id`, `provname`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `products_providers` (
  `provider_id` int(10) unsigned NOT NULL,
  `product_id` bigint(20) unsigned NOT NULL,
  `price` double unsigned NOT NULL default '0.0', #price?? implement later if decided
  KEY  (`product_id`, `provider_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `stock_corrections` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `product_id` bigint(20) unsigned NOT NULL,
  `new_stock_qty` bigint(20) unsigned NOT NULL,
  `old_stock_qty` bigint(20) unsigned NOT NULL,
  `reason` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE OR REPLACE VIEW `v_transactions` AS
select
concat( DATE_FORMAT( t.date, '%d/%m/%Y' ) , ' ', TIME_FORMAT( t.time, '%H:%i' ) ) AS datetime,
t.id AS id,
t.clientid AS clientid,
t.userid AS userid,
t.itemcount AS itemcount,
t.disc AS disc,
t.amount AS amount,
t.date AS date
from transactions t
where t.type = 1 and t.state=2 group by datetime;

CREATE OR REPLACE VIEW `v_transactionitems` AS
select
concat( DATE_FORMAT( t.date, '%d/%m/%Y' ) , ' ', TIME_FORMAT( t.time, '%H:%i' ) ) AS datetime,
t.id AS id,
ti.points AS points,
ti.name AS name,
ti.price AS price,
ti.disc AS disc,
ti.total AS total,
t.clientid AS clientid,
t.userid AS userid,
t.date AS date,
t.time AS time,
ti.position AS position,
ti.product_id AS product_id,
ti.cost AS cost
from (transactions t join transactionitems ti)
where ((t.id = ti.transaction_id) and (t.type = 1) and (t.state=2));


CREATE OR REPLACE VIEW `v_transactionsbydate` AS
select `transactions`.`date` AS `date`,
count(1) AS `transactions`,
sum(`transactions`.`itemcount`) AS `items`,
sum(`transactions`.`amount`) AS `total`
from `transactions`
where ((`transactions`.`type` = 1) and (`transactions`.`itemcount` > 0) and (`transactions`.`state`=2))
group by `transactions`.`date`;

# ---------------------------------------------
# -- Create the database user for lemon...   --

# This user is for connecting to mysql... which makes queries to mysql.
#If setting up a network of POS add each host (@box1, @box2, @box3)
#Here are only 'localhost' to ensure nobody else can do any changes from other host.

# Note: if you change the password to the lemonclient user (which is a must),
# also re-grant it again with the new password. see the grant clause below.

#CREATE USER 'lemonclient'@'localhost' IDENTIFIED BY 'xarwit0721';
GRANT ALL ON lemonposdb.* TO 'lemonclient'@'localhost' IDENTIFIED BY 'xarwit0721';


# CREATE lemon users (users using lemon, cashiers... )
#With password 'linux'. Note that this password is salt-hashed (SHA56).

INSERT INTO lemonposdb.users (id, username, password, salt, name, role) VALUES (1, 'admin', 'C07B1E799DC80B95060391DDF92B3C7EF6EECDCB', 'h60VK', 'Administrator', 0);

##You may change the string values for the next fields


#Insert a default measure (very important to keep this id)
INSERT INTO lemonposdb.measures (id, text) VALUES(1, 'Pc');
#Insert a default client
INSERT INTO lemonposdb.clients (id, name, points, discount) VALUES (1, 'General', 0, 0);
#Insert a default category
INSERT INTO lemonposdb.categories (catid, text) VALUES (1, 'General');

#Insert default payment types (very important to keep these ids)
INSERT INTO lemonposdb.paytypes (typeid, text) VALUES(1, 'Cash');
INSERT INTO lemonposdb.paytypes (typeid, text) VALUES(2, 'Card');
#Insert default transactions states (very important to keep these ids)
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(1, 'Not Completed');
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(2, 'Completed');
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(3, 'Cancelled');
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(4, 'PO Pending');
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(5, 'PO Completed');
INSERT INTO lemonposdb.transactionstates (stateid, text) VALUES(6, 'PO Incomplete');
#Insert default transactions types (very important to keep these ids)
INSERT INTO lemonposdb.transactiontypes (ttypeid, text) VALUES(1, 'Sell');
INSERT INTO lemonposdb.transactiontypes (ttypeid, text) VALUES(2, 'Purchase');
INSERT INTO lemonposdb.transactiontypes (ttypeid, text) VALUES(3, 'Change');
INSERT INTO lemonposdb.transactiontypes (ttypeid, text) VALUES(4, 'Return');
#Insert default cashFLOW types
INSERT INTO lemonposdb.cashflowtypes (typeid, text) VALUES(1, 'Normal cash OUT');
INSERT INTO lemonposdb.cashflowtypes (typeid, text) VALUES(2, 'Money return on ticket cancel');
INSERT INTO lemonposdb.cashflowtypes (typeid, text) VALUES(3, 'Money return on product return');
INSERT INTO lemonposdb.cashflowtypes (typeid, text) VALUES(4, 'Normal Cash IN');
#Insert default provider
INSERT INTO lemonposdb.providers (id,provname,address,phone,cellphone) VALUES(1,'Default Provider', '-NA-', '-NA-', '-NA-');

#Insert default tax model and elements
INSERT INTO lemonposdb.taxmodels (modelid,tname,appway,elementsid) VALUES(1,"General Tax", "*.15","1");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (1,"Simple 15%", 15);
INSERT INTO lemonposdb.brands (brandid, bname) VALUES(1,"ACME"); #CHANGE THIS BRAND NAME!
