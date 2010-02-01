-- This are Mexico tax models.
-- LemonPOS version 1.0 preview
-- Miguel Chavez Gamboa

USE lemonposdb;

INSERT INTO lemonposdb.taxmodels (modelid,tname,elementsid) VALUES(1,"Exento", "0");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (1,"Exento de impuestos", 0);

INSERT INTO lemonposdb.taxmodels (modelid,tname,elementsid) VALUES(2,"General","2");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (2,"IVA", 16);

INSERT INTO lemonposdb.taxmodels (modelid,tname,elementsid) VALUES(3,"Tabaco", "2,3");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (3,"Impuesto al tabaco", 5);

INSERT INTO lemonposdb.taxmodels (modelid,tname,elementsid) VALUES(4,"Comunicaciones","3,4");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (4,"Impuesto a las comunicaciones", 2);


