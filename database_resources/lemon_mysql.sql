# (C) 2007-2010, Miguel Chavez Gamboa [GPL v2 or later]
# run this as: cat lemon_mysql.sql | mysql -u root -p

CREATE DATABASE lemondb;
USE lemondb;

CREATE TABLE IF NOT EXISTS `transactions` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `clientid` int(10) unsigned NOT NULL,
  `type` smallint(5) unsigned default NULL,
  `amount` double unsigned NOT NULL default '0',
  `date` date NOT NULL default '2009-01-01',
  `time` time NOT NULL default '00:00',
  `paidwith` double unsigned NOT NULL default '0.0',
  `changegiven` double unsigned NOT NULL default '0.0',
  `paymethod` int(10) NOT NULL default '0',
  `state` int(10) NOT NULL default '0',
  `userid` int(10) NOT NULL default '0',
  `cardnumber` varchar(20) character set utf8 collate utf8_general_ci,
  `itemcount` int(10) unsigned NOT NULL default '0',
  `itemslist` varchar(250) character set utf8 collate utf8_general_ci NOT NULL,
  `points` bigint(20) unsigned NOT NULL default '0',
  `discmoney` double NOT NULL default '0',
  `disc` double NOT NULL default '0',
  `cardauthnumber` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  `utility` double NOT NULL default '0',
  `terminalnum` int(10) unsigned NOT NULL default '1',
  `providerid` int(10) unsigned NOT NULL default 1 , #for Purchase orders
  `specialOrders` varchar(255) collate utf8_general_ci default '',
  `balanceId` bigint(20) unsigned NOT NULL default '1',
  `totalTax` double NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY  `SEC` (`clientid`, `type`, `date`, `time`, `state`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;


CREATE TABLE IF NOT EXISTS `products` (
  `code` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(255) NOT NULL collate utf8_general_ci default 'unknown',
  `price` double unsigned NOT NULL default '0.0',
  `stockqty` double unsigned NOT NULL default '0',
  `cost` double unsigned NOT NULL default '0',
  `soldunits` double unsigned NOT NULL default '0',
  `datelastsold` date default '2009-01-01', 
  `units` int(10) unsigned collate utf8_general_ci NOT NULL default '0',
  `taxpercentage` double unsigned NOT NULL default '15',
  `extrataxes` double unsigned NOT NULL default '0',
  `photo` blob default NULL,
  `category` int(10) unsigned NOT NULL default 0,
  `points` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `alphacode` VARCHAR( 30 ) NULL,
  `lastproviderid` int(10) unsigned NOT NULL default '1',
  # for grouped and on-demand-made products (special orders)
  `isARawProduct` bool NOT NULL default false,
  `isAGroup` bool NOT NULL default false, #this is not necesary, with groupElements we can know if its a group
  `groupElements` varchar(255) collate utf8_general_ci default '',
  PRIMARY KEY  (`code`),
  KEY `SEC` (`category`, `name`, `alphacode`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

# special orders are special products, each order is a product containing one or more rawProducts
# each time its sold one, it is created. If you want predefined products use instead grouped product.
# TODO: Implement offers for special orders

CREATE TABLE IF NOT EXISTS `special_orders` (
  `orderid` bigint(20) unsigned NOT NULL auto_increment,
  `name` varchar(255) NOT NULL collate utf8_general_ci default 'unknown',
  # group elements are each products code/qty ['1/3,9/1']
  `groupElements` varchar(255) collate utf8_general_ci default '',
  `qty` double unsigned NOT NULL default 1,
  `price` double unsigned NOT NULL default '0.0',
  `cost` double unsigned NOT NULL default '0',
  `units` int(10) unsigned collate utf8_general_ci NOT NULL default '0',
  `status` int(10) default 0, # 0: pending, 1: inprogress, 2:ready, 3:delivered, 4: cancelled
  `saleid` bigint(20) unsigned NOT NULL default 1,
  `notes` varchar(255) collate utf8_general_ci default '',
  `payment` double unsigned NOT NULL default '0',
  `completePayment` bool default false,
  `dateTime` datetime NOT NULL default '2009-01-01',
  `deliveryDateTime` datetime NOT NULL default '2009-01-01',
  `clientId` bigint(20) unsigned NOT NULL default 1,
  `userId` bigint(20) unsigned NOT NULL default 1,
  PRIMARY KEY  (`orderid`),
  KEY `SEC` (`saleid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `offers` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `discount` double NOT NULL,
  `datestart` date NOT NULL default '2009-01-01',
  `dateend` date NOT NULL default '2009-01-01',
  `product_id` bigint(20) unsigned NOT NULL,
  PRIMARY KEY  (`id`, `product_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `measures` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `balances` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `datetime_start` datetime NOT NULL default '2009-01-01',
  `datetime_end` datetime NOT NULL default '2009-01-01',
  `userid` bigint(20) unsigned NOT NULL,
  `usern` varchar(50) collate utf8_general_ci NOT NULL,
  `initamount` double NOT NULL,
  `in` double NOT NULL,
  `out` double NOT NULL,
  `cash` double NOT NULL,
  `card` double NOT NULL,
  `transactions` varchar(250) collate utf8_general_ci NOT NULL default "",
  `terminalnum` bigint(20) unsigned NOT NULL,
  `cashflows` varchar(250) collate utf8_general_ci default "",
  `done` bool NOT NULL default false,
  PRIMARY KEY  (`id`),
  KEY `SEC` (`datetime_start`,`datetime_end`, `userid` )
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `categories` (
  `catid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`catid`)
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
  PRIMARY KEY  (`id`),
  KEY `SEC` (`username`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `clients` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `name` varchar(100) collate utf8_general_ci default NULL,
  `since` date NOT NULL default '2009-01-01',
  `address` varchar(255) collate utf8_general_ci default NULL,
  `phone` varchar(50) character set utf8 collate utf8_general_ci default NULL,
  `phone_movil` varchar(50) collate utf8_general_ci default NULL,
  `points` bigint(20) unsigned default '0',
  `discount` double NOT NULL,
  `photo` blob default NULL,
  PRIMARY KEY  USING BTREE (`id`)
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

CREATE TABLE IF NOT EXISTS `so_status` (
  `id` int(10) unsigned NOT NULL default 0,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `bool_values` (
  `id` int(10) unsigned NOT NULL default 0,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;


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
 `payment` double default 0,
 `completePayment` bool default false,
 `soId` varchar(255) default "",
 `isGroup` bool default false,
 `deliveryDateTime` datetime default '2009-01-01',
 `tax` double default 0,
 UNIQUE KEY `transaction_id` (`transaction_id`,`position`),
 KEY `product_id` (`product_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `cashflow` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `type` smallint(5) unsigned NOT NULL default '1',
  `userid` bigint(20) NOT NULL default '1',
  `reason` varchar(100) default NULL,                                     
  `amount` double unsigned NOT NULL default '0',
  `date` date NOT NULL default '2009-01-01',
  `time` time NOT NULL default '00:00',
  `terminalnum` int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (`id`),
  KEY SEC (`date`, `time`, `type`, `userid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `cashflowtypes` (
  `typeid` int(10) unsigned NOT NULL auto_increment,
  `text` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  PRIMARY KEY  (`typeid`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;



CREATE TABLE IF NOT EXISTS `providers` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `name` VARCHAR( 20 ) NULL,
  `address` varchar(255) collate utf8_general_ci default NULL,
  `phone` varchar(50) character set utf8 collate utf8_general_ci default NULL,
  `cellphone` varchar(50) collate utf8_general_ci default NULL,
  PRIMARY KEY  (`id`, `name`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

#CREATE TABLE IF NOT EXISTS `products_providers` (
#  `id` bigint(20) unsigned NOT NULL auto_increment,
#  `provider_id` int(10) unsigned NOT NULL,
#  `product_id` bigint(20) unsigned NOT NULL,
#  `price` double unsigned NOT NULL default '0.0', #price?? implement later if decided
#  PRIMARY KEY  (`product_id`, `provider_id`)
#) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;


# Introduced on Sept 7 2009.
CREATE TABLE IF NOT EXISTS `stock_corrections` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `product_id` bigint(20) unsigned NOT NULL,
  `new_stock_qty` bigint(20) unsigned NOT NULL,
  `old_stock_qty` bigint(20) unsigned NOT NULL,
  `reason` varchar(50) character set utf8 collate utf8_general_ci NOT NULL,
  `date` varchar(20) NOT NULL default '2009-01-01',
  `time` varchar(20) NOT NULL default '00:00',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

# Some general config that is gonna be taken from azahar. For shared configuration
CREATE TABLE IF NOT EXISTS `config` (
  `firstrun` varchar(30) character set utf8 collate utf8_general_ci NOT NULL,
  `taxIsIncludedInPrice` bool NOT NULL default true,
  `storeLogo` blob default NULL,
  `storeName` varchar(255) character set utf8 collate utf8_general_ci NULL,
  `storeAddress` varchar(255) character set utf8 collate utf8_general_ci NULL,
  `storePhone` varchar(100) character set utf8 collate utf8_general_ci NULL,
  `logoOnTop` bool NOT NULL default true,
  `useCUPS` bool NOT NULL default true,
  `smallPrint` bool NOT NULL default true,
  PRIMARY KEY  (`firstrun`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `logs` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `userid` bigint(20) unsigned NOT NULL,
  `date` varchar(20) NOT NULL default '2009-01-01',
  `time` varchar(20) NOT NULL default '00:00',
  `action` varchar(512) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

CREATE TABLE IF NOT EXISTS `random_msgs` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `message` varchar(512),
  `season` int(10) unsigned NOT NULL default 1,
  `count` bigint(20) unsigned NOT NULL default 0,
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

CREATE OR REPLACE VIEW `v_groupedSO` AS
select * from `special_orders`
group by `special_orders`.`saleid`;

CREATE OR REPLACE VIEW `v_transS` AS
select `transactions`.`id`,
 `transactions`.`userid`,
 `transactions`.`clientid`,
 `transactions`.`date`,
 `transactions`.`time`,
 `transactions`.`state`,
 `transactions`.`itemslist`,
 `transactions`.`terminalnum`
 from `transactions` WHERE (`transactions`.`state`= 1) AND (`transactions`.`type` = 1)
order by `transactions`.`id`;

# ---------------------------------------------
# -- Create the database user for lemon...   --

# This user is for connecting to mysql... which makes queries to mysql.
#If setting up a network of POS add each host (@box1, @box2, @box3)
#Here are only 'localhost' to ensure nobody else can do any changes from other host.

# Note: if you change the password to the lemonclient user (which is a must),
# also re-grant it again with the new password. see the grant clause below.

#CREATE USER 'lemonclient'@'localhost' IDENTIFIED BY 'xarwit0721';
GRANT ALL ON lemondb.* TO 'lemonclient'@'localhost' IDENTIFIED BY 'xarwit0721';


# CREATE lemon users (users using lemon, cashiers... )
#With password 'linux'. Note that this password is salt-hashed (SHA56).

INSERT INTO lemondb.users (id, username, password, salt, name, role) VALUES (1, 'admin', 'C07B1E799DC80B95060391DDF92B3C7EF6EECDCB', 'h60VK', 'Administrator', 2);

##You may change the string values for the next fields


#Insert a default measure (very important to keep this id)
INSERT INTO lemondb.measures (id, text) VALUES(1, 'Pc');
#Insert a default client
INSERT INTO lemondb.clients (id, name, points, discount) VALUES (1, 'General', 0, 0);
#Insert a default category
INSERT INTO lemondb.categories (catid, text) VALUES (1, 'General');

#Insert default payment types (very important to keep these ids)
INSERT INTO lemondb.paytypes (typeid, text) VALUES(1, 'Cash');
INSERT INTO lemondb.paytypes (typeid, text) VALUES(2, 'Card');
#Insert default transactions states (very important to keep these ids)
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(1, 'Not Completed');
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(2, 'Completed');
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(3, 'Cancelled');
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(4, 'PO Pending');
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(5, 'PO Completed');
INSERT INTO lemondb.transactionstates (stateid, text) VALUES(6, 'PO Incomplete');
#Insert default transactions types (very important to keep these ids)
INSERT INTO lemondb.transactiontypes (ttypeid, text) VALUES(1, 'Sell');
INSERT INTO lemondb.transactiontypes (ttypeid, text) VALUES(2, 'Purchase');
INSERT INTO lemondb.transactiontypes (ttypeid, text) VALUES(3, 'Change');
INSERT INTO lemondb.transactiontypes (ttypeid, text) VALUES(4, 'Return');
#Insert default cashFLOW types
INSERT INTO lemondb.cashflowtypes (typeid, text) VALUES(1, 'Normal cash OUT');
INSERT INTO lemondb.cashflowtypes (typeid, text) VALUES(2, 'Money return on ticket cancel');
INSERT INTO lemondb.cashflowtypes (typeid, text) VALUES(3, 'Money return on product return');
INSERT INTO lemondb.cashflowtypes (typeid, text) VALUES(4, 'Normal Cash IN');
#Insert default provider
INSERT INTO lemondb.providers (id,name,address,phone,cellphone) VALUES(1,'No provider', '-NA-', '-NA-', '-NA-');

INSERT INTO lemondb.so_status (id, text) VALUES(0, 'Pending');
INSERT INTO lemondb.so_status (id, text) VALUES(1, 'In Progress');
INSERT INTO lemondb.so_status (id, text) VALUES(2, 'Ready');
INSERT INTO lemondb.so_status (id, text) VALUES(3, 'Delivered');
INSERT INTO lemondb.so_status (id, text) VALUES(4, 'Cancelled');

INSERT INTO lemondb.bool_values (id, text) VALUES(0, 'NO');
INSERT INTO lemondb.bool_values (id, text) VALUES(1, 'YES');

INSERT INTO lemondb.config (firstrun, taxIsIncludedInPrice, storeLogo, storeName, storeAddress, storePhone, logoOnTop, useCUPS, smallPrint) VALUES ('yes, it is February 6 1978', true, '', '', '', '', true, true, true);


