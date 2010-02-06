# Fixes to migrate from 0.9 to 0.9.1 database version
# (C) Miguel Chavez Gamboa 2009-2010 [GPL v2 or later]

use lemondb; #modify the database name according to yours.

alter table transactionitems add `tax` double default 0 after deliveryDateTime;
alter table transactions add `totalTax` double default 0 after balanceId;

