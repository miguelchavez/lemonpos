/***************************************************************************
 *   Copyright (C) 2008-2009 by Miguel Chavez Gamboa                       *
 *   miguel.chavez.gamboa@gmail.com                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QWidget>
#include <QByteArray>
#include "azahar.h"
#include <klocale.h>

Azahar::Azahar(): QObject()
{
  errorStr = "";
  m_mainClient = "undefined";
}

Azahar::~Azahar()
{
}

void Azahar::initDatabase(QString user, QString server, QString password, QString dbname)
{
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName(server);
  db.setDatabaseName(dbname);
  db.setUserName(user);
  db.setPassword(password);

  if (!db.isOpen()) db.open();
  if (!db.isOpen()) db.open();
}

void Azahar::setDatabase(const QSqlDatabase& database)
{
  db = database;
  if (!db.isOpen()) db.open();
}

bool Azahar::isConnected()
{
  return db.isOpen();
}

void Azahar::setError(QString err)
{
  errorStr = err;
}

QString Azahar::lastError()
{
  return errorStr;
}

qulonglong Azahar::getProductOfferCode(qulonglong code)
{
  qulonglong result=0;
  if (db.isOpen()) {
    QString qry = QString("SELECT id,product_id from offers WHERE product_id=%1").arg(code);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
    }
    if (query.size() == -1)
      setError(i18n("Error serching offer id %1, Rows affected: %2", code,query.size()));
    else {
      while (query.next()) {
        int fieldId = query.record().indexOf("id");
        result = query.value(fieldId).toULongLong(); //return offer id
      }
    }
  }
  return result;
}


ProductInfo Azahar::getProductInfo(QString code)
{
  ProductInfo info;
  info.desc="Ninguno";
  info.code=0;
  info.price=0;
  info.cost=0;
  info.lastProviderId = 0;
  info.category=0;
  info.taxmodelid=0;
  info.units=0;
  info.points=0;
  info.row=-1;info.qtyOnList=-1;info.purchaseQty=-1;
  info.discpercentage=0;
  info.validDiscount=false;
  info.alphaCode = "-NA-";
  info.lastProviderId = 0;

  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    //QString qry = QString("SELECT * from products where code=%1").arg(code);
    QString qry = QString("SELECT  P.code as CODE, \
    P.alphacode as ALPHACODE, \
    P.name as NAME ,\
    P.price as PRICE, \
    P.cost as COST ,\
    P.stockqty as STOCKQTY, \
    P.units as UNITS, \
    P.points as POINTS, \
    P.photo as PHOTO, \
    P.category as CATID, \
    P.brandid as BRANDID, \
    P.lastproviderid as PROVIDERID, \
    P.taxmodel as TAXMODELID, \
    U.text as UNITSDESC, \
    C.text as CATEGORY, \
    B.bname  as BRAND, \
    PR.provname as LASTPROVIDER ,\
    T.tname as TAXNAME, \
    T.appway as TAXAPP , \
    T.elementsid as TAXELEM \
    FROM products AS P, brands as B, taxmodels as T, providers as PR, categories as C, measures as U \
    WHERE B.brandid=P.brandid AND PR.id=P.lastproviderid AND T.modelid=P.taxmodel \
    AND C.catid=P.category AND U.id=P.units\
    AND (CODE='%1' or ALPHACODE='%1');").arg(code);
    
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
      setError(i18n("Error getting product information for code %1, Rows affected: %2", code,query.size()));
    }
    else {
      while (query.next()) {
        int fieldCODE = query.record().indexOf("CODE");
        int fieldDesc = query.record().indexOf("NAME");
        int fieldPrice= query.record().indexOf("PRICE");
        int fieldPhoto= query.record().indexOf("PHOTO");
        int fieldCost= query.record().indexOf("COST");
        int fieldUnits= query.record().indexOf("UNITS");
        int fieldUnitsDESC= query.record().indexOf("UNITSDESC");
        //int fieldTaxName= query.record().indexOf("TAXNAME");
        int fieldTaxModelId= query.record().indexOf("TAXMODELID");
        //int fieldCategoryName= query.record().indexOf("CATEGORY");
        int fieldCategoryId= query.record().indexOf("CATID");
        int fieldPoints= query.record().indexOf("POINTS");
        //int fieldLastProviderName = query.record().indexOf("LASTPROVIDER");
        int fieldLastProviderId = query.record().indexOf("PROVIDERID");
        int fieldAlphaCode = query.record().indexOf("ALPHACODE");
        //int fieldBrandName = query.record().indexOf("BRAND");
        int fieldBrandId = query.record().indexOf("BRANDID");
        //int fieldTaxApp = query.record().indexOf("TAXAPP");
        int fieldTaxElem = query.record().indexOf("TAXELEM");
        int fieldStock= query.record().indexOf("STOCKQTY");

        info.code     = query.value(fieldCODE).toULongLong(); //code entry now is QString because could be the alphacode.
        info.desc     = query.value(fieldDesc).toString();
        info.price    = query.value(fieldPrice).toDouble();
        info.photo    = query.value(fieldPhoto).toByteArray();
        info.cost     = query.value(fieldCost).toDouble();
        info.units    = query.value(fieldUnits).toInt();
        info.unitStr  = query.value(fieldUnitsDESC).toString();
        info.category = query.value(fieldCategoryId).toInt();
        info.points   = query.value(fieldPoints).toInt();
        info.lastProviderId = query.value(fieldLastProviderId).toULongLong();
        info.alphaCode = query.value(fieldAlphaCode).toString();
        info.brandid    = query.value(fieldBrandId).toULongLong();
        info.taxmodelid = query.value(fieldTaxModelId).toULongLong();
        info.taxElements = query.value(fieldTaxElem).toString();
        info.profit  = info.price - info.cost;
        info.stockqty  = query.value(fieldStock).toDouble();
      }
      //get missing stuff - tax,offers for the requested product
      info.tax = getTotalTaxPercent(info.taxElements);
      double pWOtax = info.price/(1+((info.tax)/100)); //here we assume tax is already included in the price.
      info.totaltax = pWOtax*((info.tax)/100); // in money...
      
      //get units descriptions
//       qry = QString("SELECT * from measures WHERE id=%1").arg(info.units);
//       QSqlQuery query3(db);
//       if (query3.exec(qry)) {
//         while (query3.next()) {
//           int fieldUD = query3.record().indexOf("text");
//           info.unitStr=query3.value(fieldUD).toString(); //Added: Dec 15 2007
//         }//query3 - get descritptions
//       }
     //get discount info... if have one.
     QSqlQuery query2(db);
     if (query2.exec(QString("Select * from offers where product_id=%1").arg(info.code) )) {
       QList<double> descuentos; descuentos.clear();
       while (query2.next()) // return the valid discount only (and the greater one if many).
         {
           int fieldDisc = query2.record().indexOf("discount");
           int fieldDateStart = query2.record().indexOf("datestart");
           int fieldDateEnd   = query2.record().indexOf("dateend");
           double descP= query2.value(fieldDisc).toDouble();
           QDate dateStart = query2.value(fieldDateStart).toDate();
           QDate dateEnd   = query2.value(fieldDateEnd).toDate();
           QDate now = QDate::currentDate();
           //See if the offer is in a valid range...
           if ((dateStart<dateEnd) && (dateStart<=now) && (dateEnd>=now)  ) {
             //save all discounts here and decide later to return the bigger valid discount.
             descuentos.append(descP);
           }
         }
         //now which is the bigger valid discount?
         qSort(descuentos.begin(), descuentos.end(), qGreater<int>());
         //qDebug()<<"DESCUENTOS ORDENADOS DE MAYOR A MENOR:"<<descuentos;
         if (!descuentos.isEmpty()) {
           //get the first item, which is the greater one.
           info.validDiscount = true;
           info.discpercentage = descuentos.first();
           info.disc = round((info.discpercentage/100 * info.price)*100)/100;
         } else {info.disc = 0; info.validDiscount =false;}
     }
    }
  }
  return info;
}

qulonglong Azahar::getProductCode(QString text)
{
  QSqlQuery query(db);
  qulonglong code=0;
  if (query.exec(QString("select code from products where name='%1';").arg(text))) {
    while (query.next()) { 
      int fieldId   = query.record().indexOf("code");
      code = query.value(fieldId).toULongLong();
    }
  }
  else {
    //qDebug()<<"ERROR: "<<query.lastError();
    setError(query.lastError().text());
  }
  return code;
}

QList<qulonglong> Azahar::getProductsCode(QString regExpName)
{
  QList<qulonglong> result;
  result.clear();
  QSqlQuery query(db);
  QString qry;
  if (regExpName == "*") qry = "SELECT code from products;";
  else qry = QString("select code,name from products WHERE `name` REGEXP '%1'").arg(regExpName);
  if (query.exec(qry)) {
    while (query.next()) {
      int fieldId   = query.record().indexOf("code");
      qulonglong id = query.value(fieldId).toULongLong();
      result.append(id);
//       qDebug()<<"APPENDING TO PRODUCTS LIST:"<<id;
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

QStringList Azahar::getProductsList()
{
  QStringList result; result.clear();
  QSqlQuery query(db);
  if (query.exec("select name from products;")) {
    while (query.next()) {
      int fieldText = query.record().indexOf("name");
      QString text = query.value(fieldText).toString();
      result.append(text);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}


bool Azahar::insertProduct(ProductInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("INSERT INTO products (code, name, price, stockqty, cost, soldunits, datelastsold, units, brandid, taxmodel, photo, category, points, alphacode, lastproviderid) VALUES (:code, :name, :price, :stock, :cost, :soldunits, :lastsold, :units, :brand, :taxmodel, :photo, :category, :points, :alphacode, :lastproviderid);");
  query.bindValue(":code", info.code);
  query.bindValue(":name", info.desc);
  query.bindValue(":price", info.price);
  query.bindValue(":stock", info.stockqty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":soldunits", 0);
  query.bindValue(":lastsold", "0000-00-00");
  query.bindValue(":units", info.units);
  query.bindValue(":brand", info.brandid);
  query.bindValue(":taxmodel", info.taxmodelid);
  query.bindValue(":photo", info.photo);
  query.bindValue(":category", info.category);
  query.bindValue(":points", info.points);
  query.bindValue(":alphacode", info.alphaCode);
  query.bindValue(":lastproviderid", info.lastProviderId);

  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
  //NOTE: Is it necesary to check if there was an offer for this product code? and delete it.
}



bool Azahar::updateProduct(ProductInfo info, qulonglong oldcode)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE products SET code=:newcode, photo=:photo, name=:name, price=:price, stockqty=:stock, cost=:cost, units=:measure, taxmodelid=:taxmodel, category=:category, points=:points, alphacode=:alphacode, lastproviderid=:lastproviderid, brandid=:brand WHERE code=:id");
  query.bindValue(":newcode", info.code);
  query.bindValue(":name", info.desc);
  query.bindValue(":price", info.price);
  query.bindValue(":stock", info.stockqty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":measure", info.units);
  query.bindValue(":taxmodel", info.taxmodelid);
  query.bindValue(":brand", info.brandid);
  query.bindValue(":photo", info.photo);
  query.bindValue(":category", info.category);
  query.bindValue(":points", info.points);
  query.bindValue(":id", oldcode);
  query.bindValue(":alphacode", info.alphaCode);
  query.bindValue(":lastproviderid", info.lastProviderId);

  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}

bool Azahar::decrementProductStock(qulonglong code, double qty, QDate date)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  double qtys=qty;
  query.prepare("UPDATE products SET datelastsold=:dateSold , stockqty=stockqty-:qty , soldunits=soldunits+:qtys WHERE code=:id");
  query.bindValue(":id", code);
  query.bindValue(":qty", qty);
  query.bindValue(":qtys", qtys);
  query.bindValue(":dateSold", date.toString("yyyy-MM-dd"));
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  //qDebug()<<"Rows:"<<query.numRowsAffected();
  return result;
}

///NOTE or FIXME: increment is used only when returning a product? or also on purchases??
///               Because this method is decrementing soldunits... not used in lemonview.cpp nor squeezeview !!!!!
bool Azahar::incrementProductStock(qulonglong code, double qty)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  double qtys=qty;
  query.prepare("UPDATE products SET stockqty=stockqty+:qty , soldunits=soldunits-:qtys WHERE code=:id");
  query.bindValue(":id", code);
  query.bindValue(":qty", qty);
  query.bindValue(":qtys", qtys);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  //qDebug()<<"Increment Stock - Rows:"<<query.numRowsAffected();
  return result;
}


double Azahar::getProductDiscount(qulonglong code)
{
  double result = 0.0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query2(db);
    QString qry = QString("SELECT * FROM offers WHERE product_id=%1").arg(code);
    if (!query2.exec(qry)) { setError(query2.lastError().text()); }
    else {
      QList<double> descuentos; descuentos.clear();
      while (query2.next())
      {
        int fieldDisc = query2.record().indexOf("discount");
        int fieldDateStart = query2.record().indexOf("datestart");
        int fieldDateEnd   = query2.record().indexOf("dateend");
        double descP= query2.value(fieldDisc).toDouble();
        QDate dateStart = query2.value(fieldDateStart).toDate();
        QDate dateEnd   = query2.value(fieldDateEnd).toDate();
        QDate now = QDate::currentDate();
        //See if the offer is in a valid range...
        if ((dateStart<dateEnd) && (dateStart<=now) && (dateEnd>=now)  ) {
          //save all discounts here and decide later to return the bigger valid discount.
          descuentos.append(descP);
        }
      }
      //now which is the bigger valid discount?
      qSort(descuentos.begin(), descuentos.end(), qGreater<int>());
      if (!descuentos.isEmpty()) {
        //get the first item, which is the greater one.
        result = descuentos.first();
      } else result = 0;
    }
  } else { setError(db.lastError().text()); }
  return result;
}

QList<pieProdInfo> Azahar::getTop5SoldProducts()
{
  QList<pieProdInfo> products; products.clear();
  pieProdInfo info;
  QSqlQuery query(db);
  if (query.exec("SELECT name,soldunits,units FROM products WHERE soldunits>0 ORDER BY soldunits DESC LIMIT 5")) {
    while (query.next()) {
      int fieldName  = query.record().indexOf("name");
      int fieldUnits = query.record().indexOf("units");
      int fieldSoldU = query.record().indexOf("soldunits");
      int unit       = query.value(fieldUnits).toInt();
      info.name    = query.value(fieldName).toString();
      info.count   = query.value(fieldSoldU).toDouble();
      info.unitStr = getMeasureStr(unit);
      products.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return products;
}

QList<pieProdInfo> Azahar::getAlmostSoldOutProducts(int min, int max)
{
  QList<pieProdInfo> products; products.clear();
  pieProdInfo info;
  QSqlQuery query(db);
  //NOTE: Check lower limit for the soldunits range...
  query.prepare("SELECT name,stockqty,units FROM products WHERE stockqty<=:maxV ORDER BY stockqty ASC LIMIT 5");
  query.bindValue(":maxV", max);
//   query.bindValue(":minV", min);
  if (query.exec()) {
    while (query.next()) {
      int fieldName  = query.record().indexOf("name");
      int fieldUnits = query.record().indexOf("units");
      int fieldStock = query.record().indexOf("stockqty");
      int unit       = query.value(fieldUnits).toInt();
      info.name    = query.value(fieldName).toString();
      info.count   = query.value(fieldStock).toDouble();
      info.unitStr = getMeasureStr(unit);
      products.append(info);
    }
  }
  else {
    setError(query.lastError().text());
    qDebug()<<lastError();
  }
  return products;
}

//CATEGORIES
bool Azahar::insertCategory(QString text)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("INSERT INTO categories (text) VALUES (:text);");
  query.bindValue(":text", text);
  if (!query.exec()) {
    setError(query.lastError().text());
  }
  else result=true;
  
  return result;
}

QHash<QString, int> Azahar::getCategoriesHash()
{
  QHash<QString, int> result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select * from categories;")) {
      while (myQuery.next()) {
        int fieldId   = myQuery.record().indexOf("catid");
        int fieldText = myQuery.record().indexOf("text");
        int id = myQuery.value(fieldId).toInt();
        QString text = myQuery.value(fieldText).toString();
        result.insert(text, id);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QStringList Azahar::getCategoriesList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select text from categories;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("text");
        QString text = myQuery.value(fieldText).toString();
        result.append(text);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

qulonglong Azahar::getCategoryId(QString texto)
{
  qulonglong result=0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    QString qryStr = QString("SELECT categories.catid FROM categories WHERE text='%1';").arg(texto);
    if (myQuery.exec(qryStr) ) {
      while (myQuery.next()) {
        int fieldId   = myQuery.record().indexOf("catid");
        qulonglong id= myQuery.value(fieldId).toULongLong();
        result = id;
      }
    }
    else {
      setError(myQuery.lastError().text());
    }
  }
  return result;
}

QString Azahar::getCategoryStr(qulonglong id)
{
  QString result;
  QSqlQuery query(db);
  QString qstr = QString("select text from categories where catid=%1;").arg(id);
  if (query.exec(qstr)) {
    while (query.next()) {
      int fieldText = query.record().indexOf("text");
      result = query.value(fieldText).toString();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}



//MEASURES
bool Azahar::insertMeasure(QString text)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("INSERT INTO measures (text) VALUES (:text);");
  query.bindValue(":text", text);
  if (!query.exec()) {
    setError(query.lastError().text());
  }
  else result=true;
  
  return result;
}

qulonglong Azahar::getMeasureId(QString texto)
{
  qulonglong result=0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    QString qryStr = QString("select measures.id from measures where text='%1';").arg(texto);
    if (myQuery.exec(qryStr) ) {
      while (myQuery.next()) {
        int fieldId   = myQuery.record().indexOf("id");
        qulonglong id = myQuery.value(fieldId).toULongLong();
        result = id;
      }
    }
    else {
      setError(myQuery.lastError().text());
    }
  }
  return result;
}

QString Azahar::getMeasureStr(qulonglong id)
{
  QString result;
  QSqlQuery query(db);
  QString qstr = QString("select text from measures where measures.id=%1;").arg(id);
  if (query.exec(qstr)) {
    while (query.next()) {
      int fieldText = query.record().indexOf("text");
      result = query.value(fieldText).toString();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

QStringList Azahar::getMeasuresList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select text from measures;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("text");
        QString text = myQuery.value(fieldText).toString();
        result.append(text);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

//BRANDS
QStringList Azahar::getBrandsList()
{
  qDebug()<<"getBrandsList...";
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select bname from brands;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("bname");
        QString text = myQuery.value(fieldText).toString();
        result.append(text);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

//PROVIDERS
QStringList Azahar::getProvidersList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select provname from providers;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("provname");
        QString text = myQuery.value(fieldText).toString();
        result.append(text);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString Azahar::getProviderName(const qulonglong &id)
{
  QString result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select provname from providers where id=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("provname");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

// TAX MODELS
double Azahar::getTotalTaxPercent(const QString& elementsid)
{
  double result=0;
  //first parse string to get a list of tax elementids
  QStringList eList = elementsid.split(",");

  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    for (int i = 0; i < eList.size(); ++i) {
      //get amount for each element
      if (myQuery.exec(QString("select amount from taxelements where elementid=%1;").arg(eList.at(i).toULongLong()))) {
        while (myQuery.next()) {
          int fieldAmnt = myQuery.record().indexOf("amount");
          double amount = myQuery.value(fieldAmnt).toDouble();
          result += amount;
          qDebug()<<"Sum of taxes ["<<eList.size()<<" elements, i:"<<i<<"] total = "<<result;
        }
      }
      else {
        qDebug()<<"ERROR: "<<myQuery.lastError();
      }
    } //for each one
  }
  return result;
}

TaxModelInfo Azahar::getTaxModelInfo(const qulonglong id)
{
  TaxModelInfo result;
  
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select * from taxmodels where modelid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldName = myQuery.record().indexOf("tname");
        QString name  = myQuery.value(fieldName).toString();
        int fieldAppw = myQuery.record().indexOf("appway");
        QString appw  = myQuery.value(fieldAppw).toString();
        int fieldElem = myQuery.record().indexOf("elementsid");
        QString elem  = myQuery.value(fieldElem).toString();
        
        result.id     = id;
        result.name   = name;
        result.appway = appw;
        result.elements = elem;
        result.taxAmount =0; //not yet calculated!!!
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString Azahar::getTaxModelElements(const qulonglong id)
{
  QString result="";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select elementsid from taxmodels where modelid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldElem = myQuery.record().indexOf("elementsid");
        result = myQuery.value(fieldElem).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QStringList Azahar::getTaxModelsList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select tname from taxmodels;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("tname");
        QString text = myQuery.value(fieldText).toString();
        result.append(text);
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString Azahar::getTaxModelName(const qulonglong &id)
{
  QString result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select tname from taxmodels where modelid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("tname");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

//OFFERS
bool Azahar::createOffer(OfferInfo info)
{
  bool result=false;
  QString qryStr;
  QSqlQuery query(db);
  if (!db.isOpen()) db.open();
  //NOTE: Now multiple offers supported (to save offers history)
  qryStr = "INSERT INTO offers (discount, datestart, dateend, product_id) VALUES(:discount, :datestart, :dateend, :code)";
  query.prepare(qryStr);
  query.bindValue(":discount", info.discount );
  query.bindValue(":datestart", info.dateStart.toString("yyyy-MM-dd"));
  query.bindValue(":dateend", info.dateEnd.toString("yyyy-MM-dd"));
  query.bindValue(":code", info.productCode);
  if (query.exec()) result = true; else setError(query.lastError().text());

  return result;
}

bool Azahar::deleteOffer(qlonglong id)
{
  bool result=false;
  if (db.isOpen()) {
    QString qry = QString("DELETE from offers WHERE offers.id=%1").arg(id);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
    }
    if (query.numRowsAffected() == 1) result = true;
    else setError(i18n("Error deleting offer id %1, Rows affected: %2", id,query.numRowsAffected()));
  }
  return result;
}


QString Azahar::getOffersFilterWithText(QString text)
{
  QStringList codes;
  QString result="";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery qry(db);
    QString qryStr= QString("SELECT P.code, P.name, O.product_id FROM offers AS O, products AS P WHERE P.code = O.product_id and P.name REGEXP '%1' ").arg(text);
    if (!qry.exec(qryStr)) setError(qry.lastError().text());
    else {
      codes.clear();
      while (qry.next()) {
        int fieldId   = qry.record().indexOf("code");
        qulonglong c = qry.value(fieldId).toULongLong();
        codes.append(QString("offers.product_id=%1 ").arg(c));
      }
      result = codes.join(" OR ");
    }
  }
  return result;
}

bool Azahar::moveOffer(qulonglong oldp, qulonglong newp)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery q(db);
  QString qs = QString("UPDATE offers SET product_id=%1 WHERE product_id=%2;").arg(newp).arg(oldp);
  if (!q.exec( qs )) setError(q.lastError().text()); else result = true;
  return result;
}


//USERS
bool Azahar::insertUser(UserInfo info)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password, salt, name, address, phone, phone_movil, photo) VALUES(:uname, :pass, :salt, :name, :address, :phone, :cell, :photo)");
    query.bindValue(":photo", info.photo);
    query.bindValue(":uname", info.username);
    query.bindValue(":name", info.name);
    query.bindValue(":address", info.address);
    query.bindValue(":phone", info.phone);
    query.bindValue(":cell", info.cell);
    query.bindValue(":pass", info.password);
    query.bindValue(":salt", info.salt);
    if (!query.exec()) setError(query.lastError().text()); else result = true;
    //FIXME: We must see error types, which ones are for duplicate KEYS (codes) to advertise the user.
  }//db open
  return result;
}

QHash<QString,UserInfo> Azahar::getUsersHash()
{
  QHash<QString,UserInfo> result;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    QString qry = "SELECT * FROM users;";
    if (query.exec(qry)) {
      while (query.next()) {
        int fielduId       = query.record().indexOf("id");
        int fieldUsername  = query.record().indexOf("username");
        int fieldPassword  = query.record().indexOf("password");
        int fieldSalt      = query.record().indexOf("salt");
        int fieldName      = query.record().indexOf("name");
        int fieldRole      = query.record().indexOf("role"); // see role numbers at enums.h
        int fieldPhoto     = query.record().indexOf("photo");
        //more fields, now im not interested in that...
        UserInfo info;
        info.id       = query.value(fielduId).toInt();
        info.username = query.value(fieldUsername).toString();
        info.password = query.value(fieldPassword).toString();
        info.salt     = query.value(fieldSalt).toString();
        info.name     = query.value(fieldName).toString();
        info.photo    = query.value(fieldPhoto).toByteArray();
        info.role     = query.value(fieldRole).toInt();
        result.insert(info.username, info);
        //qDebug()<<"got user:"<<info.username;
      }
    }
    else {
      qDebug()<<"**Error** :"<<query.lastError();
    }
  }
 return result;
}

bool Azahar::updateUser(UserInfo info)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE users SET photo=:photo, username=:uname, name=:name, address=:address, phone=:phone, phone_movil=:cell, salt=:salt, password=:pass  WHERE id=:code;");
  query.bindValue(":code", info.id);
  query.bindValue(":photo", info.photo);
  query.bindValue(":uname", info.username);
  query.bindValue(":name", info.name);
  query.bindValue(":address", info.address);
  query.bindValue(":phone", info.phone);
  query.bindValue(":cell", info.cell);
  query.bindValue(":pass", info.password);
  query.bindValue(":salt", info.salt);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}

QString Azahar::getUserName(QString username)
{
  QString name = "";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryUname(db);
    QString qry = QString("SELECT name FROM users WHERE username='%1'").arg(username);
    if (!queryUname.exec(qry)) { setError(queryUname.lastError().text()); }
    else {
      if (queryUname.isActive() && queryUname.isSelect()) { //qDebug()<<"queryUname select && active.";
        if (queryUname.first()) { //qDebug()<<"queryUname.first()=true";
          name = queryUname.value(0).toString();
        }
      }
    }
  } else { setError(db.lastError().text()); }
  return name;
}

unsigned int Azahar::getUserId(QString uname)
{
  unsigned int iD = 0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryId(db);
    QString qry = QString("SELECT id FROM users WHERE username='%1'").arg(uname);
    if (!queryId.exec(qry)) { setError(queryId.lastError().text()); }
    else {
      if (queryId.isActive() && queryId.isSelect()) { //qDebug()<<"queryId select && active.";
        if (queryId.first()) { //qDebug()<<"queryId.first()=true";
        iD = queryId.value(0).toUInt();
        }
      }
    }
  } else { setError(db.lastError().text()); }
  return iD;
}


//CLIENTS
bool Azahar::insertClient(ClientInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO clients (name, address, phone, phone_movil, points, discount, photo, since) VALUES(:name, :address, :phone, :cell,:points, :discount, :photo, :since)");
    query.bindValue(":photo", info.photo);
    query.bindValue(":points", info.points);
    query.bindValue(":discount", info.discount);
    query.bindValue(":name", info.name);
    query.bindValue(":address", info.address);
    query.bindValue(":phone", info.phone);
    query.bindValue(":cell", info.cell);
    query.bindValue(":since", info.since);
    if (!query.exec()) setError(query.lastError().text()); else result = true;
  }
  return result;
}

bool Azahar::updateClient(ClientInfo info)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE clients SET photo=:photo, name=:name, address=:address, phone=:phone, phone_movil=:cell, points=:points, discount=:disc  WHERE id=:code;");
  query.bindValue(":code", info.id);
  query.bindValue(":photo", info.photo);
  query.bindValue(":points", info.points);
  query.bindValue(":disc", info.discount);
  query.bindValue(":name", info.name);
  query.bindValue(":address", info.address);
  query.bindValue(":phone", info.phone);
  query.bindValue(":cell", info.cell);
  if (!query.exec()) setError(query.lastError().text()); else result = true;

  return result;
}

bool Azahar::incrementClientPoints(qulonglong id, qulonglong points)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE clients SET points=points+:points WHERE id=:code;");
  query.bindValue(":code", id);
  query.bindValue(":points", points);
  if (!query.exec()) setError(query.lastError().text()); else result = true;
  return result;
}

bool Azahar::decrementClientPoints(qulonglong id, qulonglong points)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE clients SET points=points-:points WHERE id=:code;");
  query.bindValue(":code", id);
  query.bindValue(":points", points);
  if (!query.exec()) setError(query.lastError().text()); else result = true;
  return result;
}

ClientInfo Azahar::getClientInfo(qulonglong clientId) //NOTE:FALTA PROBAR ESTE METODO.
{
  ClientInfo info;
    if (!db.isOpen()) db.open();
    if (db.isOpen()) {
      QSqlQuery qC(db);
      if (qC.exec("select * from clients;")) {
        while (qC.next()) {
          int fieldId     = qC.record().indexOf("id");
          int fieldName   = qC.record().indexOf("name");
          int fieldPoints = qC.record().indexOf("points");
          int fieldPhoto  = qC.record().indexOf("photo");
          int fieldDisc   = qC.record().indexOf("discount");
          int fieldSince  = qC.record().indexOf("since");
          if (qC.value(fieldId).toUInt() == clientId) {
            info.id = qC.value(fieldId).toUInt();
            info.name       = qC.value(fieldName).toString();
            info.points     = qC.value(fieldPoints).toULongLong();
            info.discount   = qC.value(fieldDisc).toDouble();
            info.photo      = qC.value(fieldPhoto).toByteArray();
            info.since      = qC.value(fieldSince).toDate();
            break;
          }
        }
      }
      else {
        qDebug()<<"ERROR: "<<qC.lastError();
      }
    }
 return info;
}

QString Azahar::getMainClient()
{
 QString result;
 ClientInfo info;
  if (m_mainClient == "undefined") {
    if (!db.isOpen()) db.open();
    if (db.isOpen()) {
      QSqlQuery qC(db);
      if (qC.exec("select * from clients where clientid=1;")) {
        while (qC.next()) {
          int fieldName   = qC.record().indexOf("name");
          info.name       = qC.value(fieldName).toString();
          m_mainClient = info.name;
          result = info.name;
        }
      }
      else {
        qDebug()<<"ERROR: "<<qC.lastError();
      }
    }
  } else result = m_mainClient;
return result;
}

QHash<QString, ClientInfo> Azahar::getClientsHash()
{
 QHash<QString, ClientInfo> result;
 ClientInfo info;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery qC(db);
    if (qC.exec("select * from clients;")) {
      while (qC.next()) {
        int fieldId     = qC.record().indexOf("id");
        int fieldName   = qC.record().indexOf("name");
        int fieldPoints = qC.record().indexOf("points");
        int fieldPhoto  = qC.record().indexOf("photo");
        int fieldDisc   = qC.record().indexOf("discount");
        int fieldSince  = qC.record().indexOf("since");
        info.id = qC.value(fieldId).toUInt();
        info.name       = qC.value(fieldName).toString();
        info.points     = qC.value(fieldPoints).toULongLong();
        info.discount   = qC.value(fieldDisc).toDouble();
        info.photo      = qC.value(fieldPhoto).toByteArray();
        info.since      = qC.value(fieldSince).toDate();
        result.insert(info.name, info);
        if (info.id == 1) m_mainClient = info.name;
      }
    }
    else {
      qDebug()<<"ERROR: "<<qC.lastError();
    }
  }
  return result;
}

unsigned int Azahar::getClientId(QString uname)
{
  unsigned int iD = 0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryId(db);
    QString qry = QString("SELECT clients.id FROM clients WHERE clients.name='%1'").arg(uname);
    if (!queryId.exec(qry)) { setError(queryId.lastError().text()); }
    else {
      if (queryId.isActive() && queryId.isSelect()) { //qDebug()<<"queryId select && active.";
       if (queryId.first()) { //qDebug()<<"queryId.first()=true";
        iD = queryId.value(0).toUInt();
       }
      }
    }
  } else { setError(db.lastError().text()); }
  return iD;
}


//TRANSACTIONS

TransactionInfo Azahar::getTransactionInfo(qulonglong id)
{
  TransactionInfo info;
  QString qry = QString("SELECT * FROM transactions WHERE id=%1").arg(id);
  QSqlQuery query;
  if (!query.exec(qry)) { qDebug()<<query.lastError(); }
  else {
    while (query.next()) {
      int fieldId = query.record().indexOf("id");
      int fieldAmount = query.record().indexOf("amount");
      int fieldDate   = query.record().indexOf("date");
      int fieldTime   = query.record().indexOf("time");
      int fieldPaidWith = query.record().indexOf("paidwith");
      int fieldPayMethod = query.record().indexOf("paymethod");
      int fieldType      = query.record().indexOf("type");
      int fieldChange    = query.record().indexOf("changegiven");
      int fieldState     = query.record().indexOf("state");
      int fieldUserId    = query.record().indexOf("userid");
      int fieldClientId  = query.record().indexOf("clientid");
      int fieldCardNum   = query.record().indexOf("cardnumber");
      int fieldCardAuth  = query.record().indexOf("cardauthnumber");
      int fieldItemCount = query.record().indexOf("itemcount");
      int fieldItemsList = query.record().indexOf("itemsList");
      int fieldDiscount  = query.record().indexOf("disc");
      int fieldDiscMoney = query.record().indexOf("discmoney");
      int fieldPoints    = query.record().indexOf("points");
      int fieldUtility   = query.record().indexOf("profit");
      int fieldTerminal  = query.record().indexOf("terminalnum");
      int fieldGroups    = query.record().indexOf("groups");
      info.id     = query.value(fieldId).toULongLong();
      info.amount = query.value(fieldAmount).toDouble();
      info.date   = query.value(fieldDate).toDate();
      info.time   = query.value(fieldTime).toTime();
      info.paywith= query.value(fieldPaidWith).toDouble();
      info.paymethod = query.value(fieldPayMethod).toInt();
      info.type      = query.value(fieldType).toInt();
      info.changegiven = query.value(fieldChange).toDouble();
      info.state     = query.value(fieldState).toInt();
      info.userid    = query.value(fieldUserId).toULongLong();
      info.clientid  = query.value(fieldClientId).toULongLong();
      info.cardnumber= query.value(fieldCardNum).toString();
      info.cardauthnum=query.value(fieldCardAuth).toString();
      info.itemcount = query.value(fieldItemCount).toInt();
      info.itemlist  = query.value(fieldItemsList).toString();
      info.disc      = query.value(fieldDiscount).toDouble();
      info.discmoney = query.value(fieldDiscMoney).toDouble();
      info.points    = query.value(fieldPoints).toULongLong();
      info.profit    = query.value(fieldUtility).toDouble();
      info.terminalnum=query.value(fieldTerminal).toInt();
      info.groups    = query.value(fieldGroups).toString();
    }
  }
  return info;
}

ProfitRange Azahar::getMonthProfitRange()
{
  QList<TransactionInfo> monthTrans = getMonthTransactions();
  ProfitRange range;
  QList<double> profitList;
  TransactionInfo info;
  for (int i = 0; i < monthTrans.size(); ++i) {
    info = monthTrans.at(i);
    profitList.append(info.profit);
  }

  if (!profitList.isEmpty()) {
   qSort(profitList.begin(),profitList.end()); //sorting in ascending order (1,2,3..)
   range.min = profitList.first();
   range.max = profitList.last();
  } else {range.min=0.0; range.max=0.0;}

  return range;
}

ProfitRange Azahar::getMonthSalesRange()
{
  QList<TransactionInfo> monthTrans = getMonthTransactions();
  ProfitRange range;
  QList<double> salesList;
  TransactionInfo info;
  for (int i = 0; i < monthTrans.size(); ++i) {
    info = monthTrans.at(i);
    salesList.append(info.amount);
  }
  
  if (!salesList.isEmpty()) {
    qSort(salesList.begin(),salesList.end()); //sorting in ascending order (1,2,3..)
    range.min = salesList.first();
    range.max = salesList.last();
  } else {range.min=0.0; range.max=0.0;}

  return range;
}

QList<TransactionInfo> Azahar::getMonthTransactions()
{
  ///just return the amount and the profit.
  QList<TransactionInfo> result;
  TransactionInfo info;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  QDate startDate = QDate(today.year(), today.month(), 1); //get the 1st of the month.
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT date,SUM(amount),SUM(profit) from transactions where (date BETWEEN :dateSTART AND :dateEND ) AND (type=1) AND (state=2) GROUP BY date ASC;");
  qryTrans.bindValue("dateSTART", startDate.toString("yyyy-MM-dd"));
  qryTrans.bindValue("dateEND", today.toString("yyyy-MM-dd"));
  //tCompleted=2, tSell=1. With a placeholder, the value is inserted as a string, and cause the query to fail.
  if (!qryTrans.exec() ) {
    int errNum = qryTrans.lastError().number();
    QSqlError::ErrorType errType = qryTrans.lastError().type();
    QString errStr = qryTrans.lastError().text();
    QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
    setError(details);
  } else {
    while (qryTrans.next()) {
      int fieldAmount = qryTrans.record().indexOf("SUM(amount)");
      int fieldProfit = qryTrans.record().indexOf("SUM(profit)");
      int fieldDate = qryTrans.record().indexOf("date");
      info.amount = qryTrans.value(fieldAmount).toDouble();
      info.profit = qryTrans.value(fieldProfit).toDouble();
      info.date = qryTrans.value(fieldDate).toDate();
      result.append(info);
      //qDebug()<<"APPENDING:"<<info.date<< " Sales:"<<info.amount<<" Profit:"<<info.profit;
    }
    //qDebug()<<"executed query:"<<qryTrans.executedQuery();
    //qDebug()<<"Qry size:"<<qryTrans.size();
    
  }
  return result;
}


 QList<TransactionInfo> Azahar::getDayTransactions(int terminal)
 {
     QList<TransactionInfo> result;
      TransactionInfo info;
      QSqlQuery qryTrans(db);
      QDate today = QDate::currentDate();
      //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
      qryTrans.prepare("SELECT id,time,paidwith,paymethod,amount,profit from transactions where (date = :today) AND (terminalnum=:terminal) AND (type=1) AND (state=2) ORDER BY id ASC;");
      qryTrans.bindValue("today", today.toString("yyyy-MM-dd"));
      qryTrans.bindValue(":terminal", terminal);
      //tCompleted=2, tSell=1. With a placeholder, the value is inserted as a string, and cause the query to fail.
      if (!qryTrans.exec() ) {
        int errNum = qryTrans.lastError().number();
        QSqlError::ErrorType errType = qryTrans.lastError().type();
        QString errStr = qryTrans.lastError().text();
        QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
        setError(details);
      } else {
        while (qryTrans.next()) {
          int fieldAmount = qryTrans.record().indexOf("amount");
          int fieldProfit = qryTrans.record().indexOf("profit");
          info.id = qryTrans.value(qryTrans.record().indexOf("id")).toULongLong();
          info.amount = qryTrans.value(fieldAmount).toDouble();
          info.profit = qryTrans.value(fieldProfit).toDouble();
          info.paymethod = qryTrans.value(qryTrans.record().indexOf("paymethod")).toInt();
          info.paywith = qryTrans.value(qryTrans.record().indexOf("paidwith")).toDouble();
          info.time = qryTrans.value(qryTrans.record().indexOf("time")).toTime();
          result.append(info);
          //qDebug()<<"APPENDING:"<<info.id<< " Sales:"<<info.amount<<" Profit:"<<info.profit;
        }
        //qDebug()<<"executed query:"<<qryTrans.executedQuery();
        //qDebug()<<"Qry size:"<<qryTrans.size();

      }
      return result;
 }

 AmountAndProfitInfo  Azahar::getDaySalesAndProfit(int terminal)
 {
     AmountAndProfitInfo result;
      QSqlQuery qryTrans(db);
      QDate today = QDate::currentDate();
      //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
      qryTrans.prepare("SELECT SUM(amount),SUM(profit) from transactions where (date = :today) AND (terminalnum=:terminal) AND (type=1) AND (state=2) GROUP BY date ASC;");
      qryTrans.bindValue("today", today.toString("yyyy-MM-dd"));
      qryTrans.bindValue(":terminal", terminal);
      //tCompleted=2, tSell=1. With a placeholder, the value is inserted as a string, and cause the query to fail.
      if (!qryTrans.exec() ) {
        int errNum = qryTrans.lastError().number();
        QSqlError::ErrorType errType = qryTrans.lastError().type();
        QString errStr = qryTrans.lastError().text();
        QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
        setError(details);
      } else {
        while (qryTrans.next()) {
          int fieldAmount = qryTrans.record().indexOf("SUM(amount)");
          int fieldProfit = qryTrans.record().indexOf("SUM(profit)");
          result.amount = qryTrans.value(fieldAmount).toDouble();
          result.profit = qryTrans.value(fieldProfit).toDouble();
          //qDebug()<<"APPENDING:"<<info.date<< " Sales:"<<info.amount<<" Profit:"<<info.profit;
        }
        //qDebug()<<"executed query:"<<qryTrans.executedQuery();
        //qDebug()<<"Qry size:"<<qryTrans.size();

      }
      return result;
 }



qulonglong Azahar::insertTransaction(TransactionInfo info)
{
  qulonglong result=0;
  QSqlQuery query2(db);
  query2.prepare("INSERT INTO transactions ( \
  clientid, userid, type, amount, date,  time, \
  paidwith,  paymethod, changegiven, state,    \
  cardnumber, itemcount, itemslist, points, \
  discmoney, disc, cardauthnumber, profit,  \
  terminalnum, providerid, groups) \
  VALUES ( \
  :clientid, :userid, :type, :amount, :date, :time, \
  :paidwith, :paymethod, :changegiven, :state,  \
  :cardnumber, :itemcount, :itemslist, :points, \
  :discmoney, :disc, :cardauthnumber, :utility, \
  :terminalnum, :providerid, :groups)");
  
  query2.bindValue(":type", info.type);
  query2.bindValue(":amount", info.amount);
  query2.bindValue(":date", info.date.toString("yyyy-MM-dd"));
  query2.bindValue(":time", info.time.toString("hh:mm"));
  query2.bindValue(":paidwith", info.paywith );
  query2.bindValue(":changegiven", info.changegiven);
  query2.bindValue(":paymethod", info.paymethod);
  query2.bindValue(":state", info.state);
  query2.bindValue(":userid", info.userid);
  query2.bindValue(":clientid", info.clientid);
  query2.bindValue(":cardnumber", info.cardnumber);
  query2.bindValue(":itemcount", info.itemcount);
  query2.bindValue(":itemslist", info.itemlist);
  query2.bindValue(":points", info.points);
  query2.bindValue(":discmoney", info.discmoney);
  query2.bindValue(":disc", info.disc);
  query2.bindValue(":cardauthnumber", info.cardauthnum);
  query2.bindValue(":utility", info.profit);
  query2.bindValue(":terminalnum", info.terminalnum);
  query2.bindValue(":providerid", info.providerid);
  query2.bindValue(":groups", info.groups);
  if (!query2.exec() ) {
    int errNum = query2.lastError().number();
    QSqlError::ErrorType errType = query2.lastError().type();
    QString errStr = query2.lastError().text();
    QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
    setError(details);
  } else result=query2.lastInsertId().toULongLong();
  return result;
}

bool Azahar::updateTransaction(TransactionInfo info)
{
  bool result=false;
  QSqlQuery query2(db);
  query2.prepare("UPDATE transactions SET disc=:disc, discmoney=:discMoney, amount=:amount, date=:date,  time=:time, paidwith=:paidw, changegiven=:change, paymethod=:paymethod, state=:state, cardnumber=:cardnumber, itemcount=:itemcount, itemslist=:itemlist, cardauthnumber=:cardauthnumber, profit=:utility, terminalnum=:terminalnum, points=:points, clientid=:clientid, groups=:groups WHERE id=:code");
  query2.bindValue(":disc", info.disc);
  query2.bindValue(":discMoney", info.discmoney);
  query2.bindValue(":code", info.id);
  query2.bindValue(":amount", info.amount);
  query2.bindValue(":date", info.date.toString("yyyy-MM-dd"));
  query2.bindValue(":time", info.time.toString("hh:mm"));
  query2.bindValue(":paidw", info.paywith );
  query2.bindValue(":change", info.changegiven);
  query2.bindValue(":paymethod", info.paymethod);
  query2.bindValue(":state", info.state);
  query2.bindValue(":cardnumber", info.cardnumber);
  query2.bindValue(":itemcount", info.itemcount);
  query2.bindValue(":itemlist", info.itemlist);
  query2.bindValue(":cardauthnumber", info.cardauthnum);
  query2.bindValue(":utility", info.profit);
  query2.bindValue(":terminalnum", info.terminalnum);
  query2.bindValue(":points", info.points);
  query2.bindValue(":clientid", info.clientid);
  query2.bindValue(":groups", info.groups);
  if (!query2.exec() ) {
    int errNum = query2.lastError().number();
    QSqlError::ErrorType errType = query2.lastError().type();
    QString errStr = query2.lastError().text();
    QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
    setError(details);
  } else result=true;
  return result;
}

bool Azahar::deleteTransaction(qulonglong id)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    QString qry = QString("DELETE FROM transactions WHERE id=%1").arg(id);
    if (!query.exec(qry)) {
      result = false;
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString errStr = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else {
      result = true;
    }
  }
  return result;
}

bool Azahar::deleteEmptyTransactions()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    QString qry = QString("DELETE FROM transactions WHERE itemcount<=0 and amount<=0");
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString errStr = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else {
      result = true;
    }
  }
  return result;
}

bool Azahar::cancelTransaction(qulonglong id, bool inProgress)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  bool ok = db.isOpen();
  
  TransactionInfo tinfo = getTransactionInfo(id);
  bool transCompleted = false;
  bool alreadyCancelled = false;
  if (tinfo.state == tCompleted) transCompleted = true;
  if (tinfo.state == tCancelled) alreadyCancelled = true;
  
  if (ok) {
    QSqlQuery query(db);
    QString qry;
    
    if (!inProgress && !alreadyCancelled) {
      qry = QString("UPDATE transactions SET  state=%1 WHERE id=%2")
      .arg(tCancelled)
      .arg(id);
      if (!query.exec(qry)) {
        int errNum = query.lastError().number();
        QSqlError::ErrorType errType = query.lastError().type();
        QString errStr = query.lastError().text();
        QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
        setError(details);
      } else { //Cancelled...
        result = true;
        qDebug()<<"Marked as Cancelled!";
      }
      ///not in progress, it means stockqty,points... are affected.
      if (transCompleted) {
        if (tinfo.points >0) decrementClientPoints(tinfo.clientid,tinfo.points);
        ///increment stock for each product.
        QStringList plist = tinfo.itemlist.split(",");
        for (int i = 0; i < plist.size(); ++i) {
          QStringList l = plist.at(i).split("/");
          if ( l.count()==2 ) { //==2 means its complete, having product and qty
            incrementProductStock(l.at(0).toULongLong(), l.at(1).toDouble()); //code at 0, qty at 1
          }
        }//for each product
        ///save cashout for the money return
        qDebug()<<"Saving cashout-cancel";
        CashFlowInfo cinfo;
        cinfo.userid = tinfo.userid;
        cinfo.amount = tinfo.amount;
        cinfo.date   = QDate::currentDate();
        cinfo.time   = QTime::currentTime();
        cinfo.terminalNum = tinfo.terminalnum;
        cinfo.type   = ctCashOutMoneyReturnOnCancel;
        cinfo.reason = i18n("Money return on cancelling ticket #%1 ", id);
        insertCashFlow(cinfo);
      }//transCompleted
    } //not in progress
  }
  if ( alreadyCancelled ) {
    //The transaction was already canceled
    setError(i18n("Ticket #%1 was already canceled.", id));
    result = false;
    qDebug()<<"Transaction already cancelled...";
  }
  return result;
}



QList<TransactionInfo> Azahar::getLastTransactions(int pageNumber,int numItems,QDate beforeDate)
{
  QList<TransactionInfo> result;
  result.clear();
  QSqlQuery query(db);
  QString qry;
  qry = QString("SELECT * from transactions where type=1 and  date <= STR_TO_DATE('%1', '%d/%m/%Y') order by date desc, id desc LIMIT %2,%3").arg(beforeDate.toString("dd/MM/yyyy")).arg((pageNumber-1)*numItems+1).arg(numItems);
  if (query.exec(qry)) {
    while (query.next()) {
      TransactionInfo info;
      int fieldId = query.record().indexOf("id");
      int fieldAmount = query.record().indexOf("amount");
      int fieldDate   = query.record().indexOf("date");
      int fieldTime   = query.record().indexOf("time");
      int fieldPaidWith = query.record().indexOf("paidwith");
      int fieldPayMethod = query.record().indexOf("paymethod");
      int fieldType      = query.record().indexOf("type");
      int fieldChange    = query.record().indexOf("changegiven");
      int fieldState     = query.record().indexOf("state");
      int fieldUserId    = query.record().indexOf("userid");
      int fieldClientId  = query.record().indexOf("clientid");
      int fieldCardNum   = query.record().indexOf("cardnumber");
      int fieldCardAuth  = query.record().indexOf("cardauthnumber");
      int fieldItemCount = query.record().indexOf("itemcount");
      int fieldItemsList = query.record().indexOf("itemsList");
      int fieldDiscount  = query.record().indexOf("disc");
      int fieldDiscMoney = query.record().indexOf("discmoney");
      int fieldPoints    = query.record().indexOf("points");
      int fieldUtility   = query.record().indexOf("profit");
      int fieldTerminal  = query.record().indexOf("terminalnum");
      int fieldGroups    = query.record().indexOf("groups");
      info.id     = query.value(fieldId).toULongLong();
      info.amount = query.value(fieldAmount).toDouble();
      info.date   = query.value(fieldDate).toDate();
      info.time   = query.value(fieldTime).toTime();
      info.paywith= query.value(fieldPaidWith).toDouble();
      info.paymethod = query.value(fieldPayMethod).toInt();
      info.type      = query.value(fieldType).toInt();
      info.changegiven = query.value(fieldChange).toDouble();
      info.state     = query.value(fieldState).toInt();
      info.userid    = query.value(fieldUserId).toULongLong();
      info.clientid  = query.value(fieldClientId).toULongLong();
      info.cardnumber= query.value(fieldCardNum).toString();
      info.cardauthnum=query.value(fieldCardAuth).toString();
      info.itemcount = query.value(fieldItemCount).toInt();
      info.itemlist  = query.value(fieldItemsList).toString();
      info.disc      = query.value(fieldDiscount).toDouble();
      info.discmoney = query.value(fieldDiscMoney).toDouble();
      info.points    = query.value(fieldPoints).toULongLong();
      info.profit    = query.value(fieldUtility).toDouble();
      info.terminalnum=query.value(fieldTerminal).toInt();
      info.groups    = query.value(fieldGroups).toString();
      result.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

//NOTE: The next method is not used... Also, for what pourpose? is it missing the STATE condition?... Biel?
QList<TransactionInfo> Azahar::getTransactionsPerDay(int pageNumber,int numItems, QDate beforeDate)
{
  QList<TransactionInfo> result;
  result.clear();
  QSqlQuery query(db);
  QString qry;
  qry = QString("SELECT date, count(1) as transactions, sum(itemcount) as itemcount, sum(amount) as amount FROM transactions WHERE TYPE =1 AND date <= STR_TO_DATE('%1', '%d/%m/%Y') GROUP BY date(DATE) ORDER BY date DESC LIMIT %2,%3").arg(beforeDate.toString("dd/MM/yyyy")).arg((pageNumber-1)*numItems+1).arg(numItems);
  if (query.exec(qry)) {
    while (query.next()) {
      TransactionInfo info;
      int fieldTransactions = query.record().indexOf("transactions");
      int fieldAmount = query.record().indexOf("amount");
      int fieldDate   = query.record().indexOf("date");
      int fieldItemCount   = query.record().indexOf("itemcount");
      info.amount = query.value(fieldAmount).toDouble();
      info.date   = query.value(fieldDate).toDate();
      info.state     = query.value(fieldTransactions).toInt();
      info.itemcount = query.value(fieldItemCount).toInt();
      result.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

// TRANSACTIONITEMS  ///FIXME: Falta tomar en cuenta el nuevo campo 'groups'
bool Azahar::insertTransactionItem(TransactionItemInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO transactionitems (transaction_id, position, product_id, qty, points, unitstr, cost, price, disc, total, name) VALUES(:transactionid, :position, :productCode, :qty, :points, :unitStr, :cost, :price, :disc, :total, :name)");
    query.bindValue(":transactionid", info.transactionid);
    query.bindValue(":position", info.position);
    query.bindValue(":productCode", info.productCode);
    query.bindValue(":qty", info.qty);
    query.bindValue(":points", info.points);
    query.bindValue(":unitStr", info.unitStr);
    query.bindValue(":cost", info.cost);
    query.bindValue(":price", info.price);
    query.bindValue(":disc", info.disc);
    query.bindValue(":total", info.total);
    query.bindValue(":name", info.name);
    if (!query.exec()) setError(query.lastError().text()); else result = true;
  }
  return result;
}

bool Azahar::deleteAllTransactionItem(qulonglong id)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    QString qry = QString("DELETE FROM transactionitems WHERE transaction_id=%1").arg(id);
    if (!query.exec(qry)) {
      result = false;
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString errStr = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else {
      result = true;
    }
  }
  return result;
}

//FIXME: tomer en cuenta groups de las transacciones...
QList<TransactionItemInfo> Azahar::getTransactionItems(qulonglong id)
{
  QList<TransactionItemInfo> result;
  result.clear();
  QSqlQuery query(db);
  QString qry = QString("SELECT * FROM transactionitems WHERE transaction_id=%1 ORDER BY POSITION").arg(id);
  if (query.exec(qry)) {
    while (query.next()) {
      TransactionItemInfo info;
      int fieldPosition = query.record().indexOf("position");
      int fieldProductCode   = query.record().indexOf("product_id");
      int fieldQty     = query.record().indexOf("qty");
      int fieldPoints  = query.record().indexOf("points");
      int fieldCost    = query.record().indexOf("cost");
      int fieldPrice   = query.record().indexOf("price");
      int fieldDisc    = query.record().indexOf("disc");
      int fieldTotal   = query.record().indexOf("total");
      int fieldName    = query.record().indexOf("name");
      int fieldUStr    = query.record().indexOf("unitstr");
      
      info.transactionid     = id;
      info.position      = query.value(fieldPosition).toInt();
      info.productCode   = query.value(fieldProductCode).toULongLong();
      info.qty           = query.value(fieldQty).toDouble();
      info.points        = query.value(fieldPoints).toDouble();
      info.unitStr       = query.value(fieldUStr).toString();
      info.cost          = query.value(fieldCost).toDouble();
      info.price         = query.value(fieldPrice).toDouble();
      info.disc          = query.value(fieldDisc).toDouble();
      info.total         = query.value(fieldTotal).toDouble();
      info.name          = query.value(fieldName).toString();
      result.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

//BALANCES

qulonglong Azahar::insertBalance(BalanceInfo info)
{
  qulonglong result =0;
  if (!db.isOpen()) db.open();
  if (db.isOpen())
  {
    QSqlQuery queryBalance(db);
    queryBalance.prepare("INSERT INTO balances (balances.datetime_start, balances.datetime_end, balances.userid, balances.usern, balances.initamount, balances.in, balances.out, balances.cash, balances.card, balances.transactions, balances.terminalnum) VALUES (:date_start, :date_end, :userid, :user, :initA, :in, :out, :cash, :card, :transactions, :terminalNum)");
    queryBalance.bindValue(":date_start", info.dateTimeStart.toString("yyyy-MM-dd hh:mm:ss"));
    queryBalance.bindValue(":date_end", info.dateTimeEnd.toString("yyyy-MM-dd hh:mm:ss"));
    queryBalance.bindValue(":userid", info.userid);
    queryBalance.bindValue(":user", info.username);
    queryBalance.bindValue(":initA", info.initamount);
    queryBalance.bindValue(":in", info.in);
    queryBalance.bindValue(":out", info.out);
    queryBalance.bindValue(":cash", info.cash);
    queryBalance.bindValue(":card", info.card);
    queryBalance.bindValue(":transactions", info.transactions);
    queryBalance.bindValue(":terminalNum", info.terminal);

    if (!queryBalance.exec() ) {
      int errNum = queryBalance.lastError().number();
      QSqlError::ErrorType errType = queryBalance.lastError().type();
      QString errStr = queryBalance.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else result = queryBalance.lastInsertId().toULongLong();
  }
  return result;
}

qulonglong Azahar::insertCashFlow(CashFlowInfo info)
{
  qulonglong result =0;
  if (!db.isOpen()) db.open();
  if (db.isOpen())
  {
    QSqlQuery query(db);
    query.prepare("INSERT INTO cashflow ( cashflow.userid, cashflow.type, cashflow.reason, cashflow.amount, cashflow.date, cashflow.time, cashflow.terminalnum) VALUES (:userid, :type, :reason, :amount, :date, :time,  :terminalNum)");
    query.bindValue(":date", info.date.toString("yyyy-MM-dd"));
    query.bindValue(":time", info.time.toString("hh:mm:ss"));
    query.bindValue(":userid", info.userid);
    query.bindValue(":terminalNum", info.terminalNum);
    query.bindValue(":reason", info.reason);
    query.bindValue(":amount", info.amount);
    query.bindValue(":type", info.type);
    
    if (!query.exec() ) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString errStr = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else result = query.lastInsertId().toULongLong();
  }
  return result;
}



//TransactionTypes
QString  Azahar::getPayTypeStr(qulonglong type)
{
  QString result;
  QSqlQuery query(db);
  QString qstr = QString("select text from paytypes where paytypes.typeid=%1;").arg(type);
  if (query.exec(qstr)) {
    while (query.next()) {
      int fieldText = query.record().indexOf("text");
      result = query.value(fieldText).toString();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

qulonglong  Azahar::getPayTypeId(QString type)
{
  qulonglong result=0;
  QSqlQuery query(db);
  QString qstr = QString("select typeid from paytypes where paytypes.text='%1';").arg(type);
  if (query.exec(qstr)) {
    while (query.next()) {
      int fieldText = query.record().indexOf("typeid");
      result = query.value(fieldText).toULongLong();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

//Brand
QString Azahar::getBrandName(const qulonglong &id)
{
  qDebug()<<"getBrandName...";
  QString result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select bname from brands where brandid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("bname");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}



#include "azahar.moc"
