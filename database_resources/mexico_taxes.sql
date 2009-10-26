-- This are Mexico tax models.
-- LemonPOS version 0.9
-- Miguel Chavez Gamboa

USE lemonposdb;

INSERT INTO lemonposdb.taxmodels (modelid,tname,appway,elementsid) VALUES(1,"General", "__","1");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (1,"IVA", 16);

INSERT INTO lemonposdb.taxmodels (modelid,tname,appway,elementsid) VALUES(2,"Tabaco", "__","1,2");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (2,"Impuesto al tabaco", 2);

INSERT INTO lemonposdb.taxmodels (modelid,tname,appway,elementsid) VALUES(3,"Comunicaciones", "__","1,3");
INSERT INTO lemonposdb.taxelements (elementid, ename, amount) VALUES (3,"Impuesto a las comunicaciones", 2);