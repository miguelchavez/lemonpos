# Fixes to migrate from 0.9.4rc8 (or rc7,6,5,4,3,2) to newer database version
# (C) Miguel Chavez Gamboa 2009-2010 [GPL v2 or later]
#
# run this as: cat fix_0.9.4rc8.sql | mysql -u root -p


use lemondb;

ALTER TABLE products CHANGE stockqty  stockqty double NOT NULL default '0';
ALTER TABLE products ADD `hasUnlimitedStock` bool NOT NULL default false AFTER `taxmodel`;
ALTER TABLE products ADD `isNotDiscountable` bool NOT NULL default false AFTER `hasUnlimitedStock`;

