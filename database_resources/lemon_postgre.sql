-- (C) 2010, Miguel Chavez Gamboa [GPL v2 or later]
-- Run this as : psql lemondb < lemon_postgre.sql

CREATE SEQUENCE transactions_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE transactions (
  id BIGINT DEFAULT nextval('transactions_seq'::regclass) NOT NULL,
  clientid bigint  NOT NULL ,
  type smallint  default NULL ,
  amount numeric  NOT NULL default '0',
  date date NOT NULL default NOW(),
  time time NOT NULL default '00:00',
  paidwith numeric  NOT NULL default '0.0',
  changegiven numeric  NOT NULL default '0.0',
  paymethod smallint NOT NULL default '0' ,
  state smallint NOT NULL default '0' ,
  userid bigint NOT NULL default '0' ,
  cardnumber varchar(20) ,
  itemcount integer  NOT NULL default '0',
  itemslist varchar(1000) ,
  points bigint  NOT NULL default '0',
  discmoney numeric NOT NULL default '0',
  disc numeric NOT NULL default '0',
  cardauthnumber varchar(50)  NOT NULL,
  utility numeric NOT NULL default '0',
  terminalnum integer   NOT NULL default '1',
  providerid bigint   NOT NULL default 1 , --for Purchase orders
  specialOrders varchar(1000) DEFAULT '',
  balanceId bigint  NOT NULL default '1' ,
  totalTax numeric NOT NULL default '0',
  PRIMARY KEY (id)
); --WITH OIDS;

CREATE SEQUENCE products_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE products (
  code BIGINT DEFAULT nextval('products_seq'::regclass) NOT NULL,  
  name varchar(512) NOT NULL default 'New Product',
  price numeric   NOT NULL default '0.0',
  stockqty numeric   NOT NULL default '0',
  cost numeric   NOT NULL default '0',
  soldunits integer   NOT NULL default '0',
  datelastsold date default NOW(), 
  units smallint NOT NULL default '0',
  taxpercentage numeric NOT NULL default '15',
  extrataxes numeric NOT NULL default '0',
  photo BYTEA default NULL,
  category integer NOT NULL default 0,
  points integer NOT NULL DEFAULT 0,
  alphacode VARCHAR( 30 ) NULL,
  lastproviderid bigint NOT NULL default '1', --for grouped and on-demand-made products (special orders)
  isARawProduct BOOLEAN  NOT NULL default false,
  isAGroup BOOLEAN NOT NULL default false, --this is not necesary, with groupElements we can know if its a group
  groupElements varchar(1000),
  groupPriceDrop numeric NOT NULL default 0,
  PRIMARY KEY (code)
) ;

-- special orders are special products, each order is a product containing one or more rawProducts
-- each time its sold one, it is created. If you want predefined products use instead grouped product.
-- TODO: Implement offers for special orders

CREATE SEQUENCE special_oders_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE special_orders (
  orderid BIGINT DEFAULT nextval('special_orders_seq'::regclass) NOT NULL,
  name varchar(512) NOT NULL default 'unknown',
  -- group elements are each products code/qty ['1/3,9/1']
  groupElements varchar(1000) default '',
  qty numeric NOT NULL default 1,
  price numeric   NOT NULL default '0.0',
  cost numeric   NOT NULL default '0',
  units smallint    NOT NULL default '0',
  status smallint default 0, -- 0: pending, 1: inprogress, 2:ready, 3:delivered, 4: cancelled
  saleid bigint   NOT NULL default 1,
  notes varchar(800)  default '', --MCH CHANGED from 255 to 800. March 22 2010.
  payment numeric   NOT NULL default '0',
  completePayment BOOLEAN default false,
  dateTime timestamp NOT NULL default NOW(),
  deliveryDateTime timestamp NOT NULL default NOW(),
  clientId bigint   NOT NULL default 1,
  userId bigint   NOT NULL default 1,
  PRIMARY KEY  (orderid)
  ) ;

CREATE SEQUENCE offers_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE offers (
  id BIGINT DEFAULT nextval('offers_seq'::regclass) NOT NULL,
  discount numeric NOT NULL,
  datestart date NOT NULL default NOW(),
  dateend date NOT NULL default NOW(),
  product_id bigint   NOT NULL,
  PRIMARY KEY  (id)
) ;

CREATE SEQUENCE measures_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  measures (
  id BIGINT DEFAULT nextval('measures_seq'::regclass) NOT NULL,
  text varchar(50) NOT NULL,
  PRIMARY KEY  (id)
) ;

CREATE SEQUENCE balances_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  balances (
  id BIGINT DEFAULT nextval('balances_seq'::regclass) NOT NULL,
  datetime_start timestamp NOT NULL default NOW(),
  datetime_end timestamp NOT NULL default NOW(),
  userid bigint NOT NULL,
  usern varchar(50)  NOT NULL,
  initamount numeric NOT NULL,
  "in" numeric NOT NULL,
  out numeric NOT NULL,
  cash numeric NOT NULL,
  card numeric NOT NULL,
  transactions varchar(1000)  NOT NULL,
  terminalnum integer   NOT NULL,
  cashflows varchar(1000)  default '',
  done BOOLEAN NOT NULL default false,
  PRIMARY KEY  (id)
  ) ;

CREATE SEQUENCE categories_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  categories (
  catid BIGINT DEFAULT nextval('categories_seq'::regclass) NOT NULL,
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (catid)
)  ;

CREATE SEQUENCE users_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  users (
  id BIGINT DEFAULT nextval('users_seq'::regclass) NOT NULL,
  username varchar(50)  NOT NULL default '',
  password varchar(50)  default NULL,
  salt varchar(5)  default NULL,
  name varchar(255)  default '',
  address varchar(255)  default NULL,
  phone varchar(50)   default NULL,
  phone_movil varchar(50)  default NULL,
  role integer   default '0',
  photo BYTEA default NULL,
  PRIMARY KEY  (id)
)  ;

CREATE SEQUENCE clients_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  clients (
  id bigint DEFAULT nextval('clients_seq'::regclass) NOT NULL,
  name varchar(255)  default '',
  since date NOT NULL default NOW(),
  address varchar(255)  default NULL,
  phone varchar(50)   default NULL,
  phone_movil varchar(50)  default NULL,
  points bigint   default '0',
  discount numeric NOT NULL,
  photo BYTEA default NULL,
  PRIMARY KEY (id)
) ;

CREATE SEQUENCE paytypes_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  paytypes (
  typeid BIGINT DEFAULT nextval('paytypes_seq'::regclass) NOT NULL,
  text varchar(50) NOT NULL,
  PRIMARY KEY  (typeid)
)  ;

CREATE SEQUENCE transactionstates_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  transactionstates (
  stateid BIGINT DEFAULT nextval('transactionstates_seq'::regclass) NOT NULL,
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (stateid)
)  ;

CREATE SEQUENCE transactiontypes_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  transactiontypes (
  ttypeid BIGINT DEFAULT nextval('transactiontypes_seq'::regclass) NOT NULL,
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (ttypeid)
)  ;

CREATE SEQUENCE so_status_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  so_status (
  id BIGINT DEFAULT nextval('so_status_seq'::regclass) NOT NULL,
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (id)
) ;

CREATE SEQUENCE bool_values_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  bool_values (
  id BIGINT DEFAULT nextval('bool_values_seq'::regclass) NOT NULL, 
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (id)
) ;

CREATE SEQUENCE transactionitems_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  transactionitems (
 transaction_id BIGINT DEFAULT nextval('transactionitems_seq'::regclass) NOT NULL,
 position integer   NOT NULL,
 product_id bigint   NOT NULL,
 qty numeric default NULL,
 points integer default NULL,
 unitstr varchar(50) default NULL,
 cost numeric default NULL,
 price numeric default NULL,
 disc numeric default NULL,
 total numeric default NULL,
 name varchar(255) default NULL,
 payment numeric default 0,
 completePayment BOOLEAN default false,
 soId varchar(255) default '',
 isGroup BOOLEAN default false,
 deliveryDateTime timestamp default NOW(),
 tax numeric default 0,
 PRIMARY KEY (transaction_id)
 -- NOTE: here there was a secondary key (position)
) ;

CREATE SEQUENCE cashflow_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  cashflow (
  id BIGINT DEFAULT nextval('cashflow_seq'::regclass) NOT NULL,
  type smallint   NOT NULL default '1',
  userid bigint NOT NULL default '1',
  reason varchar(255) default '',
  amount numeric   NOT NULL default '0',
  date date NOT NULL default NOW(),
  time time NOT NULL default NOW(),
  terminalnum integer   NOT NULL default '1',
  PRIMARY KEY  (id)
)  ;

CREATE SEQUENCE cashflowtypes_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  cashflowtypes (
  typeid BIGINT DEFAULT nextval('cashflowtypes_seq'::regclass) NOT NULL, 
  text varchar(50)   NOT NULL,
  PRIMARY KEY  (typeid)
)  ;

CREATE SEQUENCE providers_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  providers (
  id BIGINT DEFAULT nextval('providers_seq'::regclass) NOT NULL,
  name VARCHAR( 255 )  default '',
  address varchar(255)  default NULL,
  phone varchar(50)   default NULL,
  cellphone varchar(50)  default NULL,
  PRIMARY KEY  (id, name)
)  ;


--CREATE SEQUENCE products_providers_seq
--    START WITH 1
--    INCREMENT BY 1
--    NO MAXVALUE
--    NO MINVALUE
--    CACHE 1;

--CREATE TABLE  products_providers (
--  id BIGINT DEFAULT nextval('products_providers_seq'::regclass) NOT NULL, 
--  provider_id integer   NOT NULL,
--  product_id bigint   NOT NULL,
--  price numeric  NOT NULL default '0.0', --price?? implement later if decided
--  PRIMARY KEY  (product_id, provider_id)
--)  ;

CREATE SEQUENCE stock_corrections_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

-- Introduced on Sept 7 2009.
CREATE TABLE  stock_corrections (
  id BIGINT DEFAULT nextval('stock_corrections_seq'::regclass) NOT NULL,
  product_id bigint  NOT NULL,
  new_stock_qty numeric   NOT NULL,
  old_stock_qty numeric   NOT NULL,
  reason varchar(255)  NOT NULL,
  date varchar(20) NOT NULL , --TODO: convert it to date 
  time varchar(20) NOT NULL , --TODO: convert it to time 
  PRIMARY KEY  (id)
)  ;

-- Some general config that is gonna be taken from azahar. For shared configuration
CREATE TABLE  config (
  firstrun varchar(30)   NOT NULL,
  taxIsIncludedInPrice BOOLEAN NOT NULL default true,
  storeLogo BYTEA default NULL,
  storeName varchar(255)   NULL,
  storeAddress varchar(255)   NULL,
  storePhone varchar(100)   NULL,
  logoOnTop BOOLEAN NOT NULL default true,
  useCUPS BOOLEAN NOT NULL default true,
  smallPrint BOOLEAN NOT NULL default true,
  PRIMARY KEY  (firstrun)
)  ;

CREATE SEQUENCE logs_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  logs (
  id BIGINT DEFAULT nextval('logs_seq'::regclass) NOT NULL,
  userid bigint   NOT NULL,
  date varchar(20) NOT NULL, --TODO: convert it to date
  time varchar(20) NOT NULL, --TODO: convert it to time
  action varchar(512) NOT NULL,
  PRIMARY KEY  (id)
)  ;

CREATE SEQUENCE random_msgs_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;

CREATE TABLE  random_msgs (
  id BIGINT DEFAULT nextval('random_msgs_seq'::regclass) NOT NULL, 
  message varchar(512),
  season integer NOT NULL default 1,
  count integer  NOT NULL default 0,
  PRIMARY KEY  (id)
)  ;


CREATE OR REPLACE VIEW v_transactions AS
select
EXTRACT( day FROM t.date  ) || '/' || EXTRACT( month FROM t.date ) || EXTRACT( year FROM t.date ) || ' ' || EXTRACT(hour FROM t.time ) || ':' || EXTRACT( minute FROM t.time ) AS datetime,
t.id AS id,
t.clientid AS clientid,
t.userid AS userid,
t.itemcount AS itemcount,
t.disc AS disc,
t.amount AS amount,
t.date AS date
from transactions t
where t.type = 1 and t.state=2 order by datetime; --group by hides some transactions


CREATE OR REPLACE VIEW v_transactionitems AS
select
EXTRACT( day FROM t.date  ) || '/' || EXTRACT( month FROM t.date ) || EXTRACT( year FROM t.date ) || ' ' || EXTRACT(hour FROM t.time ) || ':' || EXTRACT( minute FROM t.time ) AS datetime,
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
from transactions t join transactionitems ti ON (t.id = ti.transaction_id AND t.type = 1 and t.state=2);


CREATE OR REPLACE VIEW v_transactionsbydate AS
select transactions.date AS date,
count(1) AS transactions,
sum(transactions.itemcount) AS items,
sum(transactions.amount) AS total
from transactions
where ((transactions.type = 1) and (transactions.itemcount > 0) and (transactions.state=2))
group by transactions.date;

-- select * <- error en el *
--CREATE OR REPLACE VIEW v_groupedSO AS
--SELECT * FROM special_orders
--group by saleid;

CREATE OR REPLACE VIEW v_transS AS
select
 transactions.id,
 transactions.userid,
 transactions.clientid,
 transactions.date,
 transactions.time,
 transactions.state,
 transactions.itemslist,
 transactions.terminalnum,
 transactions.itemcount
 from transactions WHERE (transactions.state= 1) AND (transactions.type = 1) AND (transactions.itemcount > 0)
order by transactions.id;

-- ---------------------------------------------
-- CREATE lemon users (users using lemon, cashiers... )
-- With password 'linux'. Note that this password is salt-hashed (SHA56).

INSERT INTO users (id, username, password, salt, name, role) VALUES (1, 'admin', 'C07B1E799DC80B95060391DDF92B3C7EF6EECDCB', 'h60VK', 'Administrator', 2);

-- You may change the string values for the next fields


--Insert a default measure (very important to keep this id)
INSERT INTO measures (text) VALUES('Pc');
--Insert a default client
INSERT INTO clients (name, points, discount) VALUES ('General', 0, 0);
--Insert a default category
INSERT INTO categories (text) VALUES ('General');

--Insert default payment types (very important to keep these ids)
INSERT INTO paytypes (text) VALUES('Cash');
INSERT INTO paytypes (text) VALUES('Card');
--Insert default transactions states (very important to keep these ids)
INSERT INTO transactionstates (text) VALUES('Not Completed');
INSERT INTO transactionstates (text) VALUES('Completed');
INSERT INTO transactionstates (text) VALUES('Cancelled');
INSERT INTO transactionstates (text) VALUES('PO Pending');
INSERT INTO transactionstates (text) VALUES('PO Completed');
INSERT INTO transactionstates (text) VALUES('PO Incomplete');
--Insert default transactions types (very important to keep these ids)
INSERT INTO transactiontypes (text) VALUES('Sell');
INSERT INTO transactiontypes (text) VALUES('Purchase');
INSERT INTO transactiontypes (text) VALUES('Change');
INSERT INTO transactiontypes (text) VALUES('Return');
--Insert default cashFLOW types
INSERT INTO cashflowtypes (text) VALUES('Normal cash OUT');
INSERT INTO cashflowtypes (text) VALUES('Money return on ticket cancel');
INSERT INTO cashflowtypes (text) VALUES('Money return on product return');
INSERT INTO cashflowtypes (text) VALUES('Normal Cash IN');
--Insert default provider
INSERT INTO providers (name,address,phone,cellphone) VALUES('No provider', '-NA-', '-NA-', '-NA-');

INSERT INTO so_status (text) VALUES('Pending');
INSERT INTO so_status (text) VALUES('In Progress');
INSERT INTO so_status (text) VALUES('Ready');
INSERT INTO so_status (text) VALUES('Delivered');
INSERT INTO so_status (text) VALUES('Cancelled');

INSERT INTO bool_values (text) VALUES('NO');
INSERT INTO bool_values (text) VALUES('YES');

INSERT INTO config (firstrun, taxIsIncludedInPrice, storeLogo, storeName, storeAddress, storePhone, logoOnTop, useCUPS, smallPrint) VALUES ('yes, it is February 6 1978', true, '', '', '', '', true, true, true);


