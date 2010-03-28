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

Azahar::Azahar(QWidget * parent): QObject(parent)
{
  errorStr = "";
  m_mainClient = "undefined";
}

Azahar::~Azahar()
{
  //qDebug()<<"*** AZAHAR DESTROYED ***";
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

bool  Azahar::correctStock(qulonglong pcode, double oldStockQty, double newStockQty, const QString &reason )
{ //each correction is an insert to the table.
  bool result1, result2;
  result1 = result2 = false;
  if (!db.isOpen()) db.open();

  //Check if the desired product is a a group.
  if ( getProductInfo(pcode).isAGroup ) return false;

  QSqlQuery query(db);
  QDate date = QDate::currentDate();
  QTime time = QTime::currentTime();
  query.prepare("INSERT INTO stock_corrections (product_id, new_stock_qty, old_stock_qty, reason, date, time) VALUES(:pcode, :newstock, :oldstock, :reason, :date, :time); ");
  query.bindValue(":pcode", pcode);
  query.bindValue(":newstock", newStockQty);
  query.bindValue(":oldstock", oldStockQty);
  query.bindValue(":reason", reason);
  query.bindValue(":date", date.toString("yyyy-MM-dd"));
  query.bindValue(":time", time.toString("hh:mm"));
  if (!query.exec()) setError(query.lastError().text()); else result1=true;

  //modify stock
  QSqlQuery query2(db);
  query2.prepare("UPDATE products set stockqty=:newstock where code=:pcode;");
  query2.bindValue(":pcode", pcode);
  query2.bindValue(":newstock", newStockQty);
  if (!query2.exec()) setError(query2.lastError().text()); else result2=true;
  return (result1 && result2);
}

double Azahar::getProductStockQty(qulonglong code)
{
  double result=0.0;
  if (db.isOpen()) {
    QString qry = QString("SELECT stockqty from products WHERE code=%1").arg(code);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
    }
    if (query.size() == -1)
      setError(i18n("Error serching product id %1, Rows affected: %2", code,query.size()));
    else {
      while (query.next()) {
        int fieldStock = query.record().indexOf("stockqty");
        result = query.value(fieldStock).toDouble(); //return stock
      }
    }
  }
  return result;
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


ProductInfo Azahar::getProductInfo(qulonglong code)
{
  ProductInfo info;
  info.code=0;
  info.desc="Ninguno";
  info.price=0;
  info.disc=0;
  info.discpercentage=0;
  info.validDiscount=false;
  info.alphaCode = "-NA-";
  info.lastProviderId = 0;
  info.isAGroup = false;
  info.isARawProduct = false;
  QString rawCondition;

  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString qry = QString("SELECT * from products where code=%1").arg(code);
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
        int fieldDesc = query.record().indexOf("name");
        int fieldPrice= query.record().indexOf("price");
        int fieldPhoto= query.record().indexOf("photo");
        int fieldStock= query.record().indexOf("stockqty");
        int fieldCost= query.record().indexOf("cost");
        int fieldUnits= query.record().indexOf("units");
        int fieldTax1= query.record().indexOf("taxpercentage");
        int fieldTax2= query.record().indexOf("extrataxes");
        int fieldCategory= query.record().indexOf("category");
        int fieldPoints= query.record().indexOf("points");
        int fieldLastProviderId = query.record().indexOf("lastproviderid");
        int fieldAlphaCode = query.record().indexOf("alphacode");
        int fieldSoldUnits = query.record().indexOf("soldunits"); //mch 21 NOV 09
        int fieldIsARaw = query.record().indexOf("isARawProduct");
        int fieldIsAGroup = query.record().indexOf("isAGroup");
        int fieldGroupE = query.record().indexOf("groupElements");
        info.code=code;
        info.alphaCode = query.value(fieldAlphaCode).toString();
        info.desc     = query.value(fieldDesc).toString();
        info.price    = query.value(fieldPrice).toDouble();
        info.photo    = query.value(fieldPhoto).toByteArray();
        info.stockqty = query.value(fieldStock).toDouble();
        info.cost     = query.value(fieldCost).toDouble();
        info.tax      = query.value(fieldTax1).toDouble();
        info.extratax = query.value(fieldTax2).toDouble();
        info.units    = query.value(fieldUnits).toInt();
        info.category = query.value(fieldCategory).toInt();
        info.utility  = info.price - info.cost;
        info.row      = -1;
        info.points   = query.value(fieldPoints).toInt();
        info.qtyOnList = -1;
        info.purchaseQty = -1;
        info.lastProviderId = query.value(fieldLastProviderId).toULongLong();
        info.soldUnits = query.value(fieldSoldUnits).toDouble();
        info.isARawProduct = query.value(fieldIsARaw).toBool();
        info.isAGroup = query.value(fieldIsAGroup).toBool();
        if (info.isAGroup) {
          //get group average tax
          info.tax = getGroupAverageTax(code);
          info.extratax = 0; //this is included in the average tax.
        }
        //NOTE: totaltax will not be correct if addTax config option is true.
        double pWOtax = info.price/(1+((info.tax+info.extratax)/100));
        info.totaltax = pWOtax*((info.tax+info.extratax)/100); // in money...
        QString geStr = query.value(fieldGroupE).toString();
        // groupElements is a list like: '1/3,2/1'
        if (!geStr.isEmpty()) {
          //QStringList geList = geStr.split(",");
          info.groupElementsStr = geStr;
          //for (int i = 0; i < geList.size(); ++i) {
            //QStringList l = geList.at(i).split("/");
            //if ( l.count()==2 ) { //==2 means its complete, having product and qty
              //groupedInfo gInfo;
              //gInfo.pInfo = getProductInfo(l.count(0).toULongLong());
              //gInfo.qty   = l.at(1).toDouble();
              //info.groupElements.append( gInfo );
           // }
          //}//for each element
        } //groupedElements are not empty
      }
      //get units descriptions
      qry = QString("SELECT * from measures WHERE id=%1").arg(info.units);
      QSqlQuery query3(db);
      if (query3.exec(qry)) {
        while (query3.next()) {
          int fieldUD = query3.record().indexOf("text");
          info.unitStr=query3.value(fieldUD).toString(); //Added: Dec 15 2007
        }//query3 - get descritptions
      }
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
           info.disc = round((info.discpercentage/100 * info.price)*100)/100; //FIXME:This is not necesary VALID.
         } else {info.disc = 0; info.validDiscount =false;}
     }
    }
  }
  return info;
}

// QList<ProductInfo> Azahar::getTransactionGroupsList(qulonglong tid)
// {
//   QList<ProductInfo> list;
//   QStringList groupsList = getTransactionInfo(tid).groups.split(",");
//   foreach(QString ea, groupsList) {
//     qulonglong c = ea.section('/',0,0).toULongLong();
//     double     q = ea.section('/',1,1).toDouble();
//     ProductInfo pi = getProductInfo(c);
//     pi.qtyOnList = q;
//     list.append(pi);
//   }
//   return list;
// }

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

  //some buggy info can cause troubles.
  bool groupValueOK = false;
  bool rawValueOK = false;
  if (info.isAGroup == 0 || info.isAGroup == 1) groupValueOK=true;
  if (info.isARawProduct == 0 || info.isARawProduct == 1) rawValueOK=true;
  if (!groupValueOK) info.isAGroup = 0;
  if (!rawValueOK) info.isARawProduct = 0;
  
  query.prepare("INSERT INTO products (code, name, price, stockqty, cost, soldunits, datelastsold, units, taxpercentage, extrataxes, photo, category, points, alphacode, lastproviderid, isARawProduct,isAGroup, groupElements ) VALUES (:code, :name, :price, :stock, :cost, :soldunits, :lastsold, :units, :tax1, :tax2, :photo, :category, :points, :alphacode, :lastproviderid, :isARaw, :isAGroup, :groupE);");
  query.bindValue(":code", info.code);
  query.bindValue(":name", info.desc);
  query.bindValue(":price", info.price);
  query.bindValue(":stock", info.stockqty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":soldunits", 0);
  query.bindValue(":lastsold", "0000-00-00");
  query.bindValue(":units", info.units);
  query.bindValue(":tax1", info.tax);
  query.bindValue(":tax2", info.extratax);
  query.bindValue(":photo", info.photo);
  query.bindValue(":category", info.category);
  query.bindValue(":points", info.points);
  query.bindValue(":alphacode", info.alphaCode);
  query.bindValue(":lastproviderid", info.lastProviderId);
  query.bindValue(":isAGroup", info.isAGroup);
  query.bindValue(":isARaw", info.isARawProduct);
  query.bindValue(":groupE", info.groupElementsStr);

  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
  //NOTE: Is it necesary to check if there was an offer for this product code? and delete it.
}



bool Azahar::updateProduct(ProductInfo info, qulonglong oldcode)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  
  //some buggy info can cause troubles.
  bool groupValueOK = false;
  bool rawValueOK = false;
  if (info.isAGroup == 0 || info.isAGroup == 1) groupValueOK=true;
  if (info.isARawProduct == 0 || info.isARawProduct == 1) rawValueOK=true;
  if (!groupValueOK) info.isAGroup = 0;
  if (!rawValueOK) info.isARawProduct = 0;
  
  query.prepare("UPDATE products SET code=:newcode, photo=:photo, name=:name, price=:price, stockqty=:stock, cost=:cost, units=:measure, taxpercentage=:tax1, extrataxes=:tax2, category=:category, points=:points, alphacode=:alphacode, lastproviderid=:lastproviderid , isARawProduct=:isRaw, isAGroup=:isGroup, groupElements=:ge WHERE code=:id");
  query.bindValue(":newcode", info.code);
  query.bindValue(":name", info.desc);
  query.bindValue(":price", info.price);
  query.bindValue(":stock", info.stockqty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":measure", info.units);
  query.bindValue(":tax1", info.tax);
  query.bindValue(":tax2", info.extratax);
  query.bindValue(":photo", info.photo);
  query.bindValue(":category", info.category);
  query.bindValue(":points", info.points);
  query.bindValue(":id", oldcode);
  query.bindValue(":alphacode", info.alphaCode);
  query.bindValue(":lastproviderid", info.lastProviderId);
  query.bindValue(":isGroup", info.isAGroup);
  query.bindValue(":isRaw", info.isARawProduct);
  query.bindValue(":ge", info.groupElementsStr);

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

bool Azahar::decrementGroupStock(qulonglong code, double qty, QDate date)
{
  bool result = true;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);

  ProductInfo info = getProductInfo(code);
  QStringList lelem = info.groupElementsStr.split(",");
  foreach(QString ea, lelem) {
    qulonglong c = ea.section('/',0,0).toULongLong();
    double     q = ea.section('/',1,1).toDouble();
    ProductInfo pi = getProductInfo(c);
    //FOR EACH ELEMENT, DECREMENT PRODUCT STOCK
    result = result && decrementProductStock(c, q*qty, date);
  }
  
  return result;
}

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

bool Azahar::incrementGroupStock(qulonglong code, double qty)
{
  bool result = true;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  
  ProductInfo info = getProductInfo(code);
  QStringList lelem = info.groupElementsStr.split(",");
  foreach(QString ea, lelem) {
    qulonglong c = ea.section('/',0,0).toULongLong();
    double     q = ea.section('/',1,1).toDouble();
    ProductInfo pi = getProductInfo(c);
    //FOR EACH ELEMENT, DECREMENT PRODUCT STOCK
    result = result && incrementProductStock(c, q*qty);
  }
  
  return result;
}


bool Azahar::deleteProduct(qulonglong code)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM products WHERE code=%1").arg(code);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
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
  if (query.exec("SELECT name,soldunits,units,code FROM products WHERE (soldunits>0 AND isARawProduct=false) ORDER BY soldunits DESC LIMIT 5")) {
    while (query.next()) {
      int fieldName  = query.record().indexOf("name");
      int fieldUnits = query.record().indexOf("units");
      int fieldSoldU = query.record().indexOf("soldunits");
      int fieldCode  = query.record().indexOf("code");
      int unit       = query.value(fieldUnits).toInt();
      info.name    = query.value(fieldName).toString();
      info.count   = query.value(fieldSoldU).toDouble();
      info.unitStr = getMeasureStr(unit);
      info.code    = query.value(fieldCode).toULongLong();
      products.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return products;
}

double Azahar::getTopFiveMaximum()
{
  double result = 0;
  QSqlQuery query(db);
  if (query.exec("SELECT soldunits FROM products WHERE (soldunits>0 AND isARawProduct=false) ORDER BY soldunits DESC LIMIT 5")) {
    if (query.first()) {
      int fieldSoldU = query.record().indexOf("soldunits");
      result   = query.value(fieldSoldU).toDouble();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

double Azahar::getAlmostSoldOutMaximum(int max)
{
double result = 0;
  QSqlQuery query(db);
  if (query.exec(QString("SELECT stockqty FROM products WHERE (isARawProduct=false  AND isAGroup=false AND stockqty<=%1) ORDER BY stockqty DESC LIMIT 5").arg(max))) {
    if (query.first()) {
      int fieldSoldU = query.record().indexOf("stockqty");
      result   = query.value(fieldSoldU).toDouble();
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

QList<pieProdInfo> Azahar::getAlmostSoldOutProducts(int min, int max)
{
  QList<pieProdInfo> products; products.clear();
  pieProdInfo info;
  QSqlQuery query(db);
  //NOTE: Check lower limit for the soldunits range...
  query.prepare("SELECT name,stockqty,units,code FROM products WHERE (isARawProduct=false AND isAGroup=false AND stockqty<=:maxV) ORDER BY stockqty ASC LIMIT 5");
  query.bindValue(":maxV", max);
//   query.bindValue(":minV", min);
  if (query.exec()) {
    while (query.next()) {
      int fieldName  = query.record().indexOf("name");
      int fieldUnits = query.record().indexOf("units");
      int fieldStock = query.record().indexOf("stockqty");
      int fieldCode  = query.record().indexOf("code");
      int unit       = query.value(fieldUnits).toInt();
      info.name    = query.value(fieldName).toString();
      info.count   = query.value(fieldStock).toDouble();
      info.code    = query.value(fieldCode).toULongLong();
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

//returns soldout products only if the product is NOT a group.
QList<ProductInfo> Azahar::getSoldOutProducts()
{
  QList<ProductInfo> products; products.clear();
  ProductInfo info;
  QSqlQuery query(db);
  query.prepare("SELECT code FROM products WHERE stockqty=0 and isAgroup=false ORDER BY code ASC;");
  if (query.exec()) {
    while (query.next()) {
      int fieldCode  = query.record().indexOf("code");
      info = getProductInfo(query.value(fieldCode).toULongLong());
      products.append(info);
    }
  }
  else {
    setError(query.lastError().text());
    qDebug()<<lastError();
  }
  return products;
}

//also discard group products.
QList<ProductInfo> Azahar::getLowStockProducts(double min)
{
  QList<ProductInfo> products; products.clear();
  ProductInfo info;
  QSqlQuery query(db);
  query.prepare("SELECT code FROM products WHERE (stockqty<=:minV and stockqty>1 and isAGroup=false) ORDER BY code,stockqty ASC;");
  query.bindValue(":minV", min);
  if (query.exec()) {
    while (query.next()) {
      int fieldCode  = query.record().indexOf("code");
      info = getProductInfo(query.value(fieldCode).toULongLong());
      products.append(info);
    }
  }
  else {
    setError(query.lastError().text());
    qDebug()<<lastError();
  }
  return products;
}

qulonglong Azahar::getLastProviderId(qulonglong code)
{
  qulonglong result = 0;
  QSqlQuery query(db);
  query.prepare("SELECT lastproviderid FROM products WHERE code=:code");
  query.bindValue(":code", code);
  if (query.exec()) {
    while (query.next()) {
      int fieldProv  = query.record().indexOf("lastproviderid");
      result         = query.value(fieldProv).toULongLong();
    }
  }
  else {
    setError(query.lastError().text());
    qDebug()<<lastError();
  }
  return result;
}

bool Azahar::updateProductLastProviderId(qulonglong code, qulonglong provId)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE products SET lastproviderid=:provid WHERE code=:id");
  query.bindValue(":id", code);
  query.bindValue(":provid", provId);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  qDebug()<<"Rows Affected:"<<query.numRowsAffected();
  return result;
}

QList<ProductInfo> Azahar::getGroupProductsList(qulonglong id)
{
  qDebug()<<"getGroupProductsList...";
  QList<ProductInfo> pList;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString ge = getProductGroupElementsStr(id); //DONOT USE getProductInfo... this will cause an infinite loop because at that method this method is called trough getGroupAverageTax
    qDebug()<<"elements:"<<ge;
    if (ge.isEmpty()) return pList;
    QStringList pq = ge.split(",");
    foreach(QString str, pq) {
      qulonglong c = str.section('/',0,0).toULongLong();
      double     q = str.section('/',1,1).toDouble();
      //get info
      ProductInfo pi = getProductInfo(c);
      pi.qtyOnList = q;
      pList.append(pi);
      qDebug()<<" code:"<<c<<" qty:"<<q;
    }
  }
  return pList;
}

double Azahar::getGroupAverageTax(qulonglong id)
{
  qDebug()<<"Getting averate tax for id:"<<id;
  double result = 0;
  double sum = 0;
  QList<ProductInfo> pList = getGroupProductsList(id);
  foreach( ProductInfo info, pList) {
    sum += info.tax + info.extratax;
  }
  
  result = sum/pList.count();
  qDebug()<<"Group average tax: "<<result <<" sum:"<<sum<<" count:"<<pList.count();
  
  return result;
}

QString Azahar::getProductGroupElementsStr(qulonglong id)
{
  QString result;
  if (db.isOpen()) {
    QString qry = QString("SELECT groupElements from products WHERE code=%1").arg(id);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
    }
    if (query.size() == -1)
      setError(i18n("Error serching product id %1, Rows affected: %2", id,query.size()));
    else {
      while (query.next()) {
        int field = query.record().indexOf("groupElements");
        result    = query.value(field).toString();
      }
    }
  }
  return result;
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

bool Azahar::deleteCategory(qulonglong id)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM categories WHERE catid=%1").arg(id);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
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

bool Azahar::deleteMeasure(qulonglong id)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM measures WHERE id=%1").arg(id);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}

//OFFERS
bool Azahar::createOffer(OfferInfo info)
{
  bool result=false;
  QString qryStr;
  QSqlQuery query(db);
  if (!db.isOpen()) db.open();
    //The product has no offer yet.
    //NOTE: Now multiple offers supported (to save offers history)
    qryStr = "INSERT INTO offers (discount, datestart, dateend, product_id) VALUES(:discount, :datestart, :dateend, :code)";
    query.prepare(qryStr);
    query.bindValue(":discount", info.discount );
    query.bindValue(":datestart", info.dateStart.toString("yyyy-MM-dd"));
    query.bindValue(":dateend", info.dateEnd.toString("yyyy-MM-dd"));
    query.bindValue(":code", info.productCode);
    if (query.exec()) result = true; else setError(query.lastError().text());
//   }

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
    query.prepare("INSERT INTO users (username, password, salt, name, address, phone, phone_movil, role, photo) VALUES(:uname, :pass, :salt, :name, :address, :phone, :cell, :rol, :photo)");
    query.bindValue(":photo", info.photo);
    query.bindValue(":uname", info.username);
    query.bindValue(":name", info.name);
    query.bindValue(":address", info.address);
    query.bindValue(":phone", info.phone);
    query.bindValue(":cell", info.cell);
    query.bindValue(":pass", info.password);
    query.bindValue(":salt", info.salt);
    query.bindValue(":rol", info.role);
    if (!query.exec()) setError(query.lastError().text()); else result = true;
    qDebug()<<"USER insert:"<<query.lastError();
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

UserInfo Azahar::getUserInfo(const qulonglong &userid)
{
  UserInfo info;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    QString qry = QString("SELECT * FROM users where id=%1;").arg(userid);
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
        info.id       = query.value(fielduId).toInt();
        info.username = query.value(fieldUsername).toString();
        info.password = query.value(fieldPassword).toString();
        info.salt     = query.value(fieldSalt).toString();
        info.name     = query.value(fieldName).toString();
        info.photo    = query.value(fieldPhoto).toByteArray();
        info.role     = query.value(fieldRole).toInt();
        //qDebug()<<"got user:"<<info.username;
      }
    }
    else {
      qDebug()<<"**Error** :"<<query.lastError();
    }
  }
  return info; 
}

bool Azahar::updateUser(UserInfo info)
{
  bool result=false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE users SET photo=:photo, username=:uname, name=:name, address=:address, phone=:phone, phone_movil=:cell, salt=:salt, password=:pass, role=:rol  WHERE id=:code;");
  query.bindValue(":code", info.id);
  query.bindValue(":photo", info.photo);
  query.bindValue(":uname", info.username);
  query.bindValue(":name", info.name);
  query.bindValue(":address", info.address);
  query.bindValue(":phone", info.phone);
  query.bindValue(":cell", info.cell);
  query.bindValue(":pass", info.password);
  query.bindValue(":salt", info.salt);
  query.bindValue(":rol", info.role);
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

QString Azahar::getUserName(qulonglong id)
{
  QString name = "";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryUname(db);
    QString qry = QString("SELECT name FROM users WHERE users.id=%1").arg(id);
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

QStringList Azahar::getUsersList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select name from users;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("name");
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

unsigned int Azahar::getUserIdFromName(QString uname)
{
  unsigned int iD = 0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryId(db);
    QString qry = QString("SELECT id FROM users WHERE name='%1'").arg(uname);
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

int Azahar::getUserRole(const qulonglong &userid)
{
  int role = 0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery queryId(db);
    QString qry = QString("SELECT role FROM users WHERE id=%1").arg(userid);
    if (!queryId.exec(qry)) { setError(queryId.lastError().text()); }
    else {
      if (queryId.isActive() && queryId.isSelect()) { //qDebug()<<"queryId select && active.";
        if (queryId.first()) { //qDebug()<<"queryId.first()=true";
        role = queryId.value(0).toInt();
        }
      }
    }
  } else { setError(db.lastError().text()); }
  return role;
}

bool Azahar::deleteUser(qulonglong id)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM users WHERE id=%1").arg(id);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
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
  query.prepare("UPDATE clients SET photo=:photo, name=:name, address=:address, phone=:phone, phone_movil=:cell, points=:points, discount=:disc, since=:since  WHERE id=:code;");
  query.bindValue(":code", info.id);
  query.bindValue(":photo", info.photo);
  query.bindValue(":points", info.points);
  query.bindValue(":disc", info.discount);
  query.bindValue(":name", info.name);
  query.bindValue(":address", info.address);
  query.bindValue(":phone", info.phone);
  query.bindValue(":cell", info.cell);
  query.bindValue(":since", info.since);
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
          int fieldPhone  = qC.record().indexOf("phone");
          int fieldCell   = qC.record().indexOf("phone_movil");
          int fieldAdd    = qC.record().indexOf("address");
          if (qC.value(fieldId).toUInt() == clientId) {
            info.id = qC.value(fieldId).toUInt();
            info.name       = qC.value(fieldName).toString();
            info.points     = qC.value(fieldPoints).toULongLong();
            info.discount   = qC.value(fieldDisc).toDouble();
            info.photo      = qC.value(fieldPhoto).toByteArray();
            info.since      = qC.value(fieldSince).toDate();
            info.phone      = qC.value(fieldPhone).toString();
            info.cell       = qC.value(fieldCell).toString();
            info.address    = qC.value(fieldAdd).toString();
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
      if (qC.exec("select * from clients where id=1;")) {
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

QStringList Azahar::getClientsList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select name from clients;")) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("name");
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

bool Azahar::deleteClient(qulonglong id)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM clients WHERE id=%1").arg(id);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}


//TRANSACTIONS

TransactionInfo Azahar::getTransactionInfo(qulonglong id)
{
  TransactionInfo info;
  info.id = 0;
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
      int fieldUtility   = query.record().indexOf("utility");
      int fieldTerminal  = query.record().indexOf("terminalnum");
      int fieldTax       = query.record().indexOf("totalTax");
      int fieldSpecialOrders = query.record().indexOf("specialOrders");
      
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
      info.cardnumber= query.value(fieldCardNum).toString();//.replace(0,15,"***************"); //FIXED: Only save last 4 digits;
      info.cardauthnum=query.value(fieldCardAuth).toString();
      info.itemcount = query.value(fieldItemCount).toInt();
      info.itemlist  = query.value(fieldItemsList).toString();
      info.disc      = query.value(fieldDiscount).toDouble();
      info.discmoney = query.value(fieldDiscMoney).toDouble();
      info.points    = query.value(fieldPoints).toULongLong();
      info.utility   = query.value(fieldUtility).toDouble();
      info.terminalnum=query.value(fieldTerminal).toInt();
      info.totalTax   = query.value(fieldTax).toDouble();
      info.specialOrders = query.value(fieldSpecialOrders).toString();
    }
  }
  return info;
}

ProfitRange Azahar::getMonthProfitRange()
{
  QList<TransactionInfo> monthTrans = getMonthTransactionsForPie();
  ProfitRange range;
  QList<double> profitList;
  TransactionInfo info;
  for (int i = 0; i < monthTrans.size(); ++i) {
    info = monthTrans.at(i);
    profitList.append(info.utility);
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
  QList<TransactionInfo> monthTrans = getMonthTransactionsForPie();
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

QList<TransactionInfo> Azahar::getMonthTransactionsForPie()
{
  ///just return the amount and the profit.
  QList<TransactionInfo> result;
  TransactionInfo info;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  QDate startDate = QDate(today.year(), today.month(), 1); //get the 1st of the month.
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT date,SUM(amount),SUM(utility) from transactions where (date BETWEEN :dateSTART AND :dateEND ) AND (type=1) AND (state=2) GROUP BY date ASC;");
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
      int fieldProfit = qryTrans.record().indexOf("SUM(utility)");
      int fieldDate = qryTrans.record().indexOf("date");
      info.amount = qryTrans.value(fieldAmount).toDouble();
      info.utility = qryTrans.value(fieldProfit).toDouble();
      info.date = qryTrans.value(fieldDate).toDate();
      result.append(info);
      //qDebug()<<"APPENDING:"<<info.date<< " Sales:"<<info.amount<<" Profit:"<<info.utility;
    }
    //qDebug()<<"executed query:"<<qryTrans.executedQuery();
    //qDebug()<<"Qry size:"<<qryTrans.size();
    
  }
  return result;
}

QList<TransactionInfo> Azahar::getMonthTransactions()
{
  QList<TransactionInfo> result;
  TransactionInfo info;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  QDate startDate = QDate(today.year(), today.month(), 1); //get the 1st of the month.
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT id,date from transactions where (date BETWEEN :dateSTART AND :dateEND ) AND (type=1) AND (state=2) ORDER BY date,id ASC;");
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
      int fieldId = qryTrans.record().indexOf("id");
      info = getTransactionInfo(qryTrans.value(fieldId).toULongLong());
      result.append(info);
      //qDebug()<<"APPENDING: id:"<<info.id<<" "<<info.date;
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
    qryTrans.prepare("SELECT id,time,paidwith,paymethod,amount,utility from transactions where (date = :today) AND (terminalnum=:terminal) AND (type=1) AND (state=2) ORDER BY id ASC;");
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
        int fieldProfit = qryTrans.record().indexOf("utility");
        info.id = qryTrans.value(qryTrans.record().indexOf("id")).toULongLong();
        info.amount = qryTrans.value(fieldAmount).toDouble();
        info.utility = qryTrans.value(fieldProfit).toDouble();
        info.paymethod = qryTrans.value(qryTrans.record().indexOf("paymethod")).toInt();
        info.paywith = qryTrans.value(qryTrans.record().indexOf("paidwith")).toDouble();
        info.time = qryTrans.value(qryTrans.record().indexOf("time")).toTime();
        info.totalTax = qryTrans.value(qryTrans.record().indexOf("totalTax")).toDouble();
        result.append(info);
        //qDebug()<<"APPENDING:"<<info.id<< " Sales:"<<info.amount<<" Profit:"<<info.utility;
      }
      //qDebug()<<"executed query:"<<qryTrans.executedQuery();
      //qDebug()<<"Qry size:"<<qryTrans.size();

    }
    return result;
}

QList<TransactionInfo> Azahar::getDayTransactions()
{
  QList<TransactionInfo> result;
  TransactionInfo info;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT id,time,paidwith,paymethod,amount,utility from transactions where (date = :today) AND (type=1) AND (state=2) ORDER BY id ASC;");
  qryTrans.bindValue("today", today.toString("yyyy-MM-dd"));
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
      int fieldProfit = qryTrans.record().indexOf("utility");
      info.id = qryTrans.value(qryTrans.record().indexOf("id")).toULongLong();
      info.amount = qryTrans.value(fieldAmount).toDouble();
      info.utility = qryTrans.value(fieldProfit).toDouble();
      info.paymethod = qryTrans.value(qryTrans.record().indexOf("paymethod")).toInt();
      info.paywith = qryTrans.value(qryTrans.record().indexOf("paidwith")).toDouble();
      info.time = qryTrans.value(qryTrans.record().indexOf("time")).toTime();
      info.totalTax = qryTrans.value(qryTrans.record().indexOf("totalTax")).toDouble();
      result.append(info);
      //qDebug()<<"APPENDING:"<<info.id<< " Sales:"<<info.amount<<" Profit:"<<info.utility;
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
    qryTrans.prepare("SELECT SUM(amount),SUM(utility) from transactions where (date = :today) AND (terminalnum=:terminal) AND (type=1) AND (state=2) GROUP BY date ASC;");
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
        int fieldProfit = qryTrans.record().indexOf("SUM(utility)");
        result.amount = qryTrans.value(fieldAmount).toDouble();
        result.profit = qryTrans.value(fieldProfit).toDouble();
        //qDebug()<<"APPENDING:"<<info.date<< " Sales:"<<info.amount<<" Profit:"<<info.utility;
      }
      //qDebug()<<"executed query:"<<qryTrans.executedQuery();
      //qDebug()<<"Qry size:"<<qryTrans.size();

    }
    return result;
}

AmountAndProfitInfo  Azahar::getDaySalesAndProfit()
{
  AmountAndProfitInfo result;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT SUM(amount),SUM(utility) from transactions where (date = :today) AND (type=1) AND (state=2) GROUP BY date ASC;");
  qryTrans.bindValue("today", today.toString("yyyy-MM-dd"));
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
      int fieldProfit = qryTrans.record().indexOf("SUM(utility)");
      result.amount = qryTrans.value(fieldAmount).toDouble();
      result.profit = qryTrans.value(fieldProfit).toDouble();
      //qDebug()<<"APPENDING:"<<info.date<< " Sales:"<<info.amount<<" Profit:"<<info.utility;
    }
    //qDebug()<<"executed query:"<<qryTrans.executedQuery();
    //qDebug()<<"Qry size:"<<qryTrans.size();
    
  }
  return result;
}
  
//this returns the sales and profit from the 1st day of the month until today
AmountAndProfitInfo  Azahar::getMonthSalesAndProfit()
{
  AmountAndProfitInfo result;
  QSqlQuery qryTrans(db);
  QDate today = QDate::currentDate();
  QDate startDate = QDate(today.year(), today.month(), 1); //get the 1st of the month.
  //NOTE: in the next query, the state and type are hardcoded (not using the enums) because problems when preparing query.
  qryTrans.prepare("SELECT date,SUM(amount),SUM(utility) from transactions where (date BETWEEN :dateSTART AND :dateEND) AND (type=1) AND (state=2) GROUP BY type ASC;"); //group by type is to get the sum of all trans
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
      int fieldProfit = qryTrans.record().indexOf("SUM(utility)");
      result.amount = qryTrans.value(fieldAmount).toDouble();
      result.profit = qryTrans.value(fieldProfit).toDouble();
      //qDebug()<<"APPENDING --  Sales:"<<result.amount<<" Profit:"<<result.profit;
    }
    //qDebug()<<"executed query:"<<qryTrans.executedQuery();
    //qDebug()<<"Qry size:"<<qryTrans.size();
    
  }
  return result;
}

//TRANSACTIONS
qulonglong Azahar::insertTransaction(TransactionInfo info)
{
  qulonglong result=0;
  //NOTE:The next commented code was deprecated because it will cause to overwrite transactions
  //    When two or more terminals were getting an empty transacion at the same time, getting the same one.
  //first look for an empty transaction.
  //qulonglong availableId = getEmptyTransactionId();
  //if (availableId > 0 ) {
  //  qDebug()<<"The empty transaction # "<<availableId <<" is available to reuse.";
  //  info.id = availableId;
  //  if (updateTransaction(info)) result = availableId;
  //}
  //else {
    // insert a new one.
    QSqlQuery query2(db);
    query2.prepare("INSERT INTO transactions (clientid, type, amount, date,  time, paidwith, changegiven, paymethod, state, userid, cardnumber, itemcount, itemslist, cardauthnumber, utility, terminalnum, providerid, specialOrders, balanceId, totalTax) VALUES (:clientid, :type, :amount, :date, :time, :paidwith, :changegiven, :paymethod, :state, :userid, :cardnumber, :itemcount, :itemslist, :cardauthnumber, :utility, :terminalnum, :providerid, :specialOrders, :balance, :tax)"); //removed groups 29DIC09
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
    query2.bindValue(":cardnumber", info.cardnumber); //.replace(0,15,"***************")); //FIXED: Only save last 4 digits
    query2.bindValue(":itemcount", info.itemcount);
    query2.bindValue(":itemslist", info.itemlist);
    query2.bindValue(":cardauthnumber", info.cardauthnum);
    query2.bindValue(":utility", info.utility);
    query2.bindValue(":terminalnum", info.terminalnum);
    query2.bindValue(":providerid", info.providerid);
    query2.bindValue(":tax", info.totalTax);
    query2.bindValue(":specialOrders", info.specialOrders);
    query2.bindValue(":balance", info.balanceId);
    if (!query2.exec() ) {
      int errNum = query2.lastError().number();
      QSqlError::ErrorType errType = query2.lastError().type();
      QString errStr = query2.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else result=query2.lastInsertId().toULongLong();
  //}

  return result;
}

bool Azahar::updateTransaction(TransactionInfo info)
{
  bool result=false;
  QSqlQuery query2(db);
  query2.prepare("UPDATE transactions SET disc=:disc, discmoney=:discMoney, amount=:amount, date=:date,  time=:time, paidwith=:paidw, changegiven=:change, paymethod=:paymethod, state=:state, cardnumber=:cardnumber, itemcount=:itemcount, itemslist=:itemlist, cardauthnumber=:cardauthnumber, utility=:utility, terminalnum=:terminalnum, points=:points, clientid=:clientid, specialOrders=:sorders, balanceId=:balance, totalTax=:tax WHERE id=:code");
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
  query2.bindValue(":cardnumber", info.cardnumber);//.replace(0,15,"***************")); //FIXED: Only save last 4 digits
  query2.bindValue(":itemcount", info.itemcount);
  query2.bindValue(":itemlist", info.itemlist);
  query2.bindValue(":cardauthnumber", info.cardauthnum);
  query2.bindValue(":utility", info.utility);
  query2.bindValue(":terminalnum", info.terminalnum);
  query2.bindValue(":points", info.points);
  query2.bindValue(":clientid", info.clientid);
  query2.bindValue(":tax", info.totalTax);
  query2.bindValue(":sorders", info.specialOrders);
  query2.bindValue(":balance", info.balanceId);
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


//NOTE: Is it convenient to reuse empty transactions or simply delete them?
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

qulonglong Azahar::getEmptyTransactionId()
{
    qulonglong result = 0;
    if (!db.isOpen()) db.open();
    if (db.isOpen()) {
     QSqlQuery query(db);
     QString qry = QString("SELECT id from transactions WHERE itemcount<=0 and amount<=0");
     if (!query.exec(qry)) {
       int errNum = query.lastError().number();
       QSqlError::ErrorType errType = query.lastError().type();
       QString errStr = query.lastError().text();
       QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
       setError(details);
     } else {
        while (query.next()) {
          int fieldId = query.record().indexOf("id");
          result      = query.value(fieldId).toULongLong();
          return result;
        }
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
  bool transExists = false;
  if (tinfo.id    >  0)          transExists      = true;
  if (tinfo.state == tCompleted && transExists) transCompleted   = true;
  if (tinfo.state == tCancelled && transExists) alreadyCancelled = true;
  
  
  if (ok) {
    QSqlQuery query(db);
    QString qry;
    
    if (!inProgress && !alreadyCancelled && transExists) {
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
        //TODO: when cancelling a transacion, take into account the groups sold to be returned. new feature
        QStringList soProducts;
        ///if there is any special order (product)
        if ( !tinfo.specialOrders.isEmpty() ) {
          //get each special order
          QStringList pSoList = tinfo.specialOrders.split(",");
          for (int i = 0; i < pSoList.size(); ++i) {
            QStringList l = pSoList.at(i).split("/");
            if ( l.count()==2 ) { //==2 means its complete, having product and qty
              qulonglong soid = l.at(0).toULongLong();
              //set as cancelled
              specialOrderSetStatus(soid, 4); //4 == cancelled
              //get each product of the special order to increment its stock later
              soProducts.append( getSpecialOrderProductsStr(soid) ); //are normal products (raw or not)
            } //if count
          } //for
        }//if there are special orders
        QString soProductsStr = soProducts.join(",");
        ///increment stock for each product. including special orders and groups
        QStringList plist = (tinfo.itemlist.split(",") + soProductsStr.split(","));
        qDebug()<<"[*****] plist: "<< plist;
        for (int i = 0; i < plist.size(); ++i) {
          QStringList l = plist.at(i).split("/");
          if ( l.count()==2 ) { //==2 means its complete, having product and qty
            //check if the product is a group
            //NOTE: rawProducts ? affect stock when cancelling = YES but only if affected when sold one of its parents (specialOrders) and stockqty is set. But they would not be here, if not at specialOrders List
            ProductInfo pi = getProductInfo(l.at(0).toULongLong());
            if ( pi.isAGroup ) 
              incrementGroupStock(l.at(0).toULongLong(), l.at(1).toDouble()); //code at 0, qty at 1
            else //there is a normal product
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
      int fieldUtility   = query.record().indexOf("utility");
      int fieldTerminal  = query.record().indexOf("terminalnum");
      int fieldTax    = query.record().indexOf("totalTax");
      int fieldSOrd      = query.record().indexOf("specialOrders");
      int fieldBalance   = query.record().indexOf("balanceId");
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
      info.cardnumber= query.value(fieldCardNum).toString();//.replace(0,15,"***************"); //FIXED: Only save last 4 digits;
      info.cardauthnum=query.value(fieldCardAuth).toString();
      info.itemcount = query.value(fieldItemCount).toInt();
      info.itemlist  = query.value(fieldItemsList).toString();
      info.disc      = query.value(fieldDiscount).toDouble();
      info.discmoney = query.value(fieldDiscMoney).toDouble();
      info.points    = query.value(fieldPoints).toULongLong();
      info.utility   = query.value(fieldUtility).toDouble();
      info.terminalnum=query.value(fieldTerminal).toInt();
      info.totalTax  = query.value(fieldTax).toDouble();
      info.specialOrders  = query.value(fieldSOrd).toString();
      info.balanceId = query.value(fieldBalance).toULongLong();
      result.append(info);
    }
  }
  else {
    setError(query.lastError().text());
  }
  return result;
}

//NOTE: The next method is not used... Also, for what pourpose? is it missing the STATE condition?
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


// TRANSACTIONITEMS
bool Azahar::insertTransactionItem(TransactionItemInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO transactionitems (transaction_id, position, product_id, qty, points, unitstr, cost, price, disc, total, name, payment, completePayment, soId, isGroup, deliveryDateTime, tax) VALUES(:transactionid, :position, :productCode, :qty, :points, :unitStr, :cost, :price, :disc, :total, :name, :payment, :completeP, :soid, :isGroup, :deliveryDT, :tax)");
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
    query.bindValue(":payment", info.payment);
    query.bindValue(":completeP", info.completePayment);
    query.bindValue(":soid", info.soId);
    query.bindValue(":isGroup", info.isGroup);
    query.bindValue(":deliveryDT", info.deliveryDateTime);
    query.bindValue(":tax", info.tax);
    if (!query.exec()) {
      setError(query.lastError().text());
      qDebug()<<"Insert TransactionItems error:"<<query.lastError().text();
    } else result = true;
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
      int fieldPayment = query.record().indexOf("payment");
      int fieldCPayment = query.record().indexOf("completePayment");
      int fieldSoid = query.record().indexOf("soId");
      int fieldIsG = query.record().indexOf("isGroup");
      int fieldDDT = query.record().indexOf("deliveryDateTime");
      int fieldTax = query.record().indexOf("tax");
      
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
      info.payment       = query.value(fieldPayment).toDouble();
      info.completePayment  = query.value(fieldCPayment).toBool();
      info.soId          = query.value(fieldSoid).toString();
      info.isGroup       = query.value(fieldIsG).toBool();
      info.deliveryDateTime=query.value(fieldDDT).toDateTime();
      info.tax           = query.value(fieldTax).toDouble();
      result.append(info);
    }
  }
  else {
    setError(query.lastError().text());
    qDebug()<<"Get TransactionItems error:"<<query.lastError().text();
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
    queryBalance.prepare("INSERT INTO balances (balances.datetime_start, balances.datetime_end, balances.userid, balances.usern, balances.initamount, balances.in, balances.out, balances.cash, balances.card, balances.transactions, balances.terminalnum, balances.cashflows, balances.done) VALUES (:date_start, :date_end, :userid, :user, :initA, :in, :out, :cash, :card, :transactions, :terminalNum, :cashflows, :isDone)");
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
    queryBalance.bindValue(":cashflows", info.cashflows);
    queryBalance.bindValue(":isDone", info.done);

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

BalanceInfo Azahar::getBalanceInfo(qulonglong id)
{
  BalanceInfo info;
  info.id = 0;
  QString qry = QString("SELECT * FROM balances WHERE id=%1").arg(id);
  QSqlQuery query;
  if (!query.exec(qry)) { qDebug()<<query.lastError(); }
  else {
    while (query.next()) {
      int fieldId = query.record().indexOf("id");
      int fieldDtStart = query.record().indexOf("datetime_start");
      int fieldDtEnd   = query.record().indexOf("datetime_end");
      int fieldUserId  = query.record().indexOf("userid");
      int fieldUsername= query.record().indexOf("usern");
      int fieldInitAmount = query.record().indexOf("initamount");
      int fieldIn      = query.record().indexOf("in");
      int fieldOut     = query.record().indexOf("out");
      int fieldCash    = query.record().indexOf("cash");
      int fieldTransactions    = query.record().indexOf("transactions");
      int fieldCard    = query.record().indexOf("card");
      int fieldTerminalNum   = query.record().indexOf("terminalnum");
      int fieldCashFlows     = query.record().indexOf("cashflows");
      int fieldDone    = query.record().indexOf("done");
      info.id     = query.value(fieldId).toULongLong();
      info.dateTimeStart = query.value(fieldDtStart).toDateTime();
      info.dateTimeEnd   = query.value(fieldDtEnd).toDateTime();
      info.userid   = query.value(fieldUserId).toULongLong();
      info.username= query.value(fieldUsername).toString();
      info.initamount = query.value(fieldInitAmount).toDouble();
      info.in      = query.value(fieldIn).toDouble();
      info.out = query.value(fieldOut).toDouble();
      info.cash   = query.value(fieldCash).toDouble();
      info.card   = query.value(fieldCard).toDouble();
      info.transactions= query.value(fieldTransactions).toString();
      info.terminal = query.value(fieldTerminalNum).toInt();
      info.cashflows= query.value(fieldCashFlows).toString();
      info.done = query.value(fieldDone).toBool();
    }
  }
  return info;
}

bool Azahar::updateBalance(BalanceInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen())
  {
    QSqlQuery queryBalance(db);
    queryBalance.prepare("UPDATE balances SET balances.datetime_start=:date_start, balances.datetime_end=:date_end, balances.userid=:userid, balances.usern=:user, balances.initamount=:initA, balances.in=:in, balances.out=:out, balances.cash=:cash, balances.card=:card, balances.transactions=:transactions, balances.terminalnum=:terminalNum, cashflows=:cashflows, done=:isDone WHERE balances.id=:bid");
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
    queryBalance.bindValue(":cashflows", info.cashflows);
    queryBalance.bindValue(":bid", info.id);
    queryBalance.bindValue(":isDone", info.done);
    
    if (!queryBalance.exec() ) {
      int errNum = queryBalance.lastError().number();
      QSqlError::ErrorType errType = queryBalance.lastError().type();
      QString errStr = queryBalance.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),errStr);
      setError(details);
    } else result = true;
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

QList<CashFlowInfo> Azahar::getCashFlowInfoList(const QList<qulonglong> &idList)
{
  QList<CashFlowInfo> result;
  result.clear();
  if (idList.count() == 0) return result;
  QSqlQuery query(db);

  foreach(qulonglong currId, idList) {
    QString qry = QString(" \
    SELECT CF.id as id, \
    CF.type as type, \
    CF.userid as userid, \
    CF.amount as amount, \
    CF.reason as reason, \
    CF.date as date, \
    CF.time as time, \
    CF.terminalNum as terminalNum, \
    CFT.text as typeStr \
    FROM cashflow AS CF, cashflowtypes AS CFT \
    WHERE id=%1 AND (CFT.typeid = CF.type) ORDER BY CF.id;").arg(currId);
    if (query.exec(qry)) {
      while (query.next()) {
        CashFlowInfo info;
        int fieldId      = query.record().indexOf("id");
        int fieldType    = query.record().indexOf("type");
        int fieldUserId  = query.record().indexOf("userid");
        int fieldAmount  = query.record().indexOf("amount");
        int fieldReason  = query.record().indexOf("reason");
        int fieldDate    = query.record().indexOf("date");
        int fieldTime    = query.record().indexOf("time");
        int fieldTermNum = query.record().indexOf("terminalNum");
        int fieldTStr    = query.record().indexOf("typeStr");

        info.id          = query.value(fieldId).toULongLong();
        info.type        = query.value(fieldType).toULongLong();
        info.userid      = query.value(fieldUserId).toULongLong();
        info.amount      = query.value(fieldAmount).toDouble();
        info.reason      = query.value(fieldReason).toString();
        info.typeStr     = query.value(fieldTStr).toString();
        info.date        = query.value(fieldDate).toDate();
        info.time        = query.value(fieldTime).toTime();
        info.terminalNum = query.value(fieldTermNum).toULongLong();
        result.append(info);
      }
    }
    else {
      setError(query.lastError().text());
    }
  } //foreach
  return result;
}

QList<CashFlowInfo> Azahar::getCashFlowInfoList(const QDateTime &start, const QDateTime &end)
{
  QList<CashFlowInfo> result;
  result.clear();
  QSqlQuery query(db);

  query.prepare(" \
  SELECT CF.id as id, \
  CF.type as type, \
  CF.userid as userid, \
  CF.amount as amount, \
  CF.reason as reason, \
  CF.date as date, \
  CF.time as time, \
  CF.terminalNum as terminalNum, \
  CFT.text as typeStr \
  FROM cashflow AS CF, cashflowtypes AS CFT \
  WHERE (date BETWEEN :dateSTART AND :dateEND) AND (CFT.typeid = CF.type) ORDER BY CF.id");
  query.bindValue(":dateSTART", start.date());
  query.bindValue(":dateEND", end.date());
  if (query.exec()) {
    while (query.next()) {
      QTime ttime = query.value(query.record().indexOf("time")).toTime();
      if ( (ttime >= start.time()) &&  (ttime <= end.time()) ) {
        //its inside the requested time period.
        CashFlowInfo info;
        int fieldId      = query.record().indexOf("id");
        int fieldType    = query.record().indexOf("type");
        int fieldUserId  = query.record().indexOf("userid");
        int fieldAmount  = query.record().indexOf("amount");
        int fieldReason  = query.record().indexOf("reason");
        int fieldDate    = query.record().indexOf("date");
        int fieldTime    = query.record().indexOf("time");
        int fieldTermNum = query.record().indexOf("terminalNum");
        int fieldTStr    = query.record().indexOf("typeStr");
        
        info.id          = query.value(fieldId).toULongLong();
        info.type        = query.value(fieldType).toULongLong();
        info.userid      = query.value(fieldUserId).toULongLong();
        info.amount      = query.value(fieldAmount).toDouble();
        info.reason      = query.value(fieldReason).toString();
        info.typeStr     = query.value(fieldTStr).toString();
        info.date        = query.value(fieldDate).toDate();
        info.time        = query.value(fieldTime).toTime();
        info.terminalNum = query.value(fieldTermNum).toULongLong();
        result.append(info);
      } //if time...
    } //while
  }// if query
  else {
    setError(query.lastError().text());
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


//LOGS

bool Azahar::insertLog(const qulonglong &userid, const QDate &date, const QTime &time, const QString actionStr)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO logs (userid, date, time, action) VALUES(:userid, :date, :time, :action);");
    query.bindValue(":userid", userid);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":time", time.toString("hh:mm"));
    query.bindValue(":action", actionStr);
    if (!query.exec()) {
      setError(query.lastError().text());
      qDebug()<<"ERROR ON SAVING LOG:"<<query.lastError().text();
    } else result = true;
  }
  return result;
}

bool Azahar::getConfigFirstRun()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select firstrun from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("firstrun");
        QString value = myQuery.value(fieldText).toString();
        if (value == "yes, it is February 6 1978")
          result = true;
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

bool Azahar::getConfigTaxIsIncludedInPrice()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select taxIsIncludedInPrice from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("taxIsIncludedInPrice");
        result = myQuery.value(fieldText).toBool();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

void Azahar::cleanConfigFirstRun()
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set firstrun='yes, i like the rainy days';"))) {
      qDebug()<<"Change config firstRun...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void Azahar::setConfigTaxIsIncludedInPrice(bool option)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set taxIsIncludedInPrice=%1;").arg(option))) {
      qDebug()<<"Change config taxIsIncludedInPrice...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

QPixmap  Azahar::getConfigStoreLogo()
{
  QPixmap result;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select storeLogo from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("storeLogo");
        result.loadFromData(myQuery.value(fieldText).toByteArray());
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString  Azahar::getConfigStoreName()
{
  QString result;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select storeName from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("storeName");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString  Azahar::getConfigStoreAddress()
{
  QString result;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select storeAddress from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("storeAddress");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QString  Azahar::getConfigStorePhone()
{
  QString result;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select storePhone from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("storePhone");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

bool     Azahar::getConfigSmallPrint()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select smallPrint from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("smallPrint");
        result = myQuery.value(fieldText).toBool();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

bool     Azahar::getConfigLogoOnTop()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select logoOnTop from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("logoOnTop");
        result = myQuery.value(fieldText).toBool();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

bool     Azahar::getConfigUseCUPS()
{
  bool result = false;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select useCUPS from config;"))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("useCUPS");
        result = myQuery.value(fieldText).toBool();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

void   Azahar::setConfigStoreLogo(const QByteArray &baPhoto)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    myQuery.prepare("update config set storeLogo=:logo;");
    myQuery.bindValue(":logo", baPhoto);
    if (myQuery.exec()) {
      qDebug()<<"Change config store logo...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigStoreName(const QString &str)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set storeName='%1';").arg(str))) {
      qDebug()<<"Change config storeName...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigStoreAddress(const QString &str)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set storeAddress='%1';").arg(str))) {
      qDebug()<<"Change config storeAddress...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigStorePhone(const QString &str)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set storePhone='%1';").arg(str))) {
      qDebug()<<"Change config taxIsIncludedInPrice...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigSmallPrint(bool yes)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set smallPrint=%1;").arg(yes))) {
      qDebug()<<"Change config SmallPrint...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigUseCUPS(bool yes)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set useCUPS=%1;").arg(yes))) {
      qDebug()<<"Change config useCUPS...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void   Azahar::setConfigLogoOnTop(bool yes)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update config set logoOnTop=%1;").arg(yes))) {
      qDebug()<<"Change config logoOnTop...";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}


//SPECIAL ORDERS
QList<SpecialOrderInfo> Azahar::getAllSOforSale(qulonglong saleId)
{
  QList<SpecialOrderInfo> list;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString qry = QString("SELECT orderid,saleid from special_orders where saleid=%1").arg(saleId);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
      setError(i18n("Error getting special Order information, id: %1, Rows affected: %2", saleId,query.size()));
    }
    else {
      while (query.next()) {
        int fieldId  = query.record().indexOf("orderid");
        qulonglong num = query.value(fieldId).toULongLong();
        SpecialOrderInfo soInfo = getSpecialOrderInfo(num);
        list.append(soInfo);
      }
    }
  }
  return list;
}

//NOTE: Here the question is, what status to take into account? pending,inprogress,ready...
//      We will return all with status < 3 .
QList<SpecialOrderInfo> Azahar::getAllReadySOforSale(qulonglong saleId)
{
  QList<SpecialOrderInfo> list;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString qry = QString("SELECT orderid,saleid from special_orders where saleid=%1 and status<3").arg(saleId);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
      setError(i18n("Error getting special Order information, id: %1, Rows affected: %2", saleId,query.size()));
    }
    else {
      while (query.next()) {
        int fieldId  = query.record().indexOf("orderid");
        qulonglong num = query.value(fieldId).toULongLong();
        SpecialOrderInfo soInfo = getSpecialOrderInfo(num);
        list.append(soInfo);
      }
    }
  }
  return list;
}

int  Azahar::getReadySOCountforSale(qulonglong saleId)
{
  int count=0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString qry = QString("SELECT orderid from special_orders where saleid=%1 and status<3").arg(saleId);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
      setError(i18n("Error getting special Order information, id: %1, Rows affected: %2", saleId,query.size()));
    }
    else {
//       while (query.next()) {
//         count++;
//       }
    count = query.size();
    }
  }
  return count;
}

void Azahar::specialOrderSetStatus(qulonglong id, int status)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update special_orders set status=%1 where orderid=%2;").arg(status).arg(id))) {
      qDebug()<<"Status Order updated";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

void Azahar::soTicketSetStatus(qulonglong ticketId, int status)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update special_orders set status=%1 where saleid=%2;").arg(status).arg(ticketId))) {
      qDebug()<<"Status Order updated";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

QString Azahar::getSpecialOrderProductsStr(qulonglong id)
{
  QString result = "";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select groupElements from special_orders where orderid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("groupElements");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

QList<ProductInfo> Azahar::getSpecialOrderProductsList(qulonglong id)
{
  QList<ProductInfo> pList;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString ge = getSpecialOrderProductsStr(id);
    QStringList pq = ge.split(",");
    foreach(QString str, pq) {
      qulonglong c = str.section('/',0,0).toULongLong();
      double     q = str.section('/',1,1).toDouble();
      //get info
      ProductInfo pi = getProductInfo(c);
      pi.qtyOnList = q;
      pList.append(pi);
    }
  }
  return pList;
}

QString  Azahar::getSONotes(qulonglong id)
{
  QString result = "";
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select notes from special_orders where orderid=%1;").arg(id))) {
      while (myQuery.next()) {
        int fieldText = myQuery.record().indexOf("notes");
        result = myQuery.value(fieldText).toString();
      }
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
  return result;
}

qulonglong Azahar::insertSpecialOrder(SpecialOrderInfo info)
{
  qulonglong result = 0;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("INSERT INTO special_orders (name, price, qty, cost, units, groupElements, status, saleid, notes, payment, completePayment, dateTime, deliveryDateTime,clientId,userId) VALUES (:name, :price, :qty, :cost, :units,  :groupE, :status, :saleId, :notes, :payment, :completeP, :dateTime, :deliveryDT, :client, :user);");
  query.bindValue(":name", info.name);
  query.bindValue(":price", info.price);
  query.bindValue(":qty", info.qty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":status", info.status);
  query.bindValue(":units", info.units);
  query.bindValue(":groupE", info.groupElements);
  query.bindValue(":saleId", info.saleid);
  query.bindValue(":notes", info.notes);
  query.bindValue(":payment", info.payment);
  query.bindValue(":completeP", info.completePayment);
  query.bindValue(":dateTime", info.dateTime);
  query.bindValue(":deliveryDT", info.deliveryDateTime);
  query.bindValue(":user", info.userId);
  query.bindValue(":client", info.clientId);
  
  if (!query.exec()) setError(query.lastError().text()); else {
    result=query.lastInsertId().toULongLong();
  }
  
  return result;
}

//saleid, dateTime/userid/clientid are not updated.
bool Azahar::updateSpecialOrder(SpecialOrderInfo info)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("UPDATE special_orders SET  name=:name, price=:price, qty=:qty, cost=:cost, units=:units,  groupElements=:groupE, status=:status, notes=:notes, payment=:payment, completePayment=:completeP, deliveryDateTime=:deliveryDT WHERE orderid=:code;");
  query.bindValue(":code", info.orderid);
  query.bindValue(":name", info.name);
  query.bindValue(":price", info.price);
  query.bindValue(":qty", info.qty);
  query.bindValue(":cost", info.cost);
  query.bindValue(":status", info.status);
  query.bindValue(":units", info.units);
  query.bindValue(":groupE", info.groupElements);
  query.bindValue(":notes", info.notes);
  query.bindValue(":payment", info.payment);
  query.bindValue(":completeP", info.completePayment);
  query.bindValue(":deliveryDT", info.deliveryDateTime);
  
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}

bool Azahar::deleteSpecialOrder(qulonglong id)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query = QString("DELETE FROM special_orders WHERE orderid=%1").arg(id);
  if (!query.exec()) setError(query.lastError().text()); else result=true;
  return result;
}

SpecialOrderInfo Azahar::getSpecialOrderInfo(qulonglong id)
{
  SpecialOrderInfo info;
  info.orderid=0;
  info.name="Ninguno";
  info.price=0;

  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QString qry = QString("SELECT * from special_orders where orderid=%1").arg(id);
    QSqlQuery query(db);
    if (!query.exec(qry)) {
      int errNum = query.lastError().number();
      QSqlError::ErrorType errType = query.lastError().type();
      QString error = query.lastError().text();
      QString details = i18n("Error #%1, Type:%2\n'%3'",QString::number(errNum), QString::number(errType),error);
      setError(i18n("Error getting special Order information, id: %1, Rows affected: %2", id,query.size()));
    }
    else {
      while (query.next()) {
        int fieldDesc = query.record().indexOf("name");
        int fieldPrice= query.record().indexOf("price");
        int fieldQty= query.record().indexOf("qty");
        int fieldCost= query.record().indexOf("cost");
        int fieldUnits= query.record().indexOf("units");
        int fieldStatus= query.record().indexOf("status");
        int fieldSaleId= query.record().indexOf("saleid");
        int fieldGroupE = query.record().indexOf("groupElements");
        int fieldPayment = query.record().indexOf("payment");
        int fieldCPayment = query.record().indexOf("completePayment");
        int fieldDateT   = query.record().indexOf("dateTime");
        int fieldDDT     = query.record().indexOf("deliveryDateTime");
        int fieldNotes   = query.record().indexOf("notes");
        int fieldClientId = query.record().indexOf("clientId");
        int fieldUserId = query.record().indexOf("userId");
        
        info.orderid=id;
        info.name      = query.value(fieldDesc).toString();
        info.price    = query.value(fieldPrice).toDouble();
        info.qty      = query.value(fieldQty).toDouble();
        info.cost     = query.value(fieldCost).toDouble();
        info.units    = query.value(fieldUnits).toInt();
        info.status   = query.value(fieldStatus).toInt();
        info.saleid   = query.value(fieldSaleId).toULongLong();
        info.groupElements = query.value(fieldGroupE).toString();
        info.payment  = query.value(fieldPayment).toDouble();
        info.completePayment  = query.value(fieldCPayment).toBool();
        info.dateTime = query.value(fieldDateT).toDateTime();
        info.deliveryDateTime = query.value(fieldDDT).toDateTime();
        info.notes = query.value(fieldNotes).toString();
        info.clientId = query.value(fieldClientId).toULongLong();
        info.userId = query.value(fieldUserId).toULongLong();
      }
      //get units descriptions
      qry = QString("SELECT * from measures WHERE id=%1").arg(info.units);
      QSqlQuery query3(db);
      if (query3.exec(qry)) {
        while (query3.next()) {
          int fieldUD = query3.record().indexOf("text");
          info.unitStr=query3.value(fieldUD).toString();
        }//query3 - get descritptions
      }
    }
  }
  return info;
}

double Azahar::getSpecialOrderAverageTax(qulonglong id)
{
  double result = 0;
  double sum = 0;
  QList<ProductInfo> pList = getSpecialOrderProductsList(id);
  foreach( ProductInfo info, pList) {
    sum += info.tax + info.extratax;
  }

  result = sum/pList.count();
  qDebug()<<"SO average tax: "<<result <<" sum:"<<sum<<" count:"<<pList.count();

  return result;
}

///This gets discounts on a special order based on its raw products discount, returned in %.
double Azahar::getSpecialOrderAverageDiscount(qulonglong id)
{
  double result = 0;
  double sum = 0;
  QList<ProductInfo> pList = getSpecialOrderProductsList(id);
  foreach( ProductInfo info, pList) {
    if (info.validDiscount) {
      sum += info.discpercentage;
    }
  }
  
  result = sum/pList.count();
  qDebug()<<"SO average discount: "<<result <<" sum:"<<sum<<" count:"<<pList.count();
  
  return result;
}


QStringList Azahar::getStatusList()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec("select text from so_status;")) {
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

QStringList Azahar::getStatusListExceptDelivered()
{
  QStringList result;
  result.clear();
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("select text from so_status where id!=%1;").arg(stDelivered))) {
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

int Azahar::getStatusId(QString texto)
{
  qulonglong result=0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    QString qryStr = QString("SELECT so_status.id FROM so_status WHERE text='%1';").arg(texto);
    if (myQuery.exec(qryStr) ) {
      while (myQuery.next()) {
        int fieldId   = myQuery.record().indexOf("id");
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

QString Azahar::getRandomMessage(QList<qulonglong> &excluded, const int &season)
{
  QString result;
  QString firstMsg;
  qulonglong firstId = 0;
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    QString qryStr = QString("SELECT message,id FROM random_msgs WHERE season=%1 order by count ASC;").arg(season);
    if (myQuery.exec(qryStr) ) {
      while (myQuery.next()) {
        int fieldId      = myQuery.record().indexOf("id");
        int fieldMsg     = myQuery.record().indexOf("message");
        qulonglong id    = myQuery.value(fieldId).toULongLong();
        if ( firstMsg.isEmpty() ) {
          firstMsg = myQuery.value(fieldMsg).toString(); //get the first msg.
          firstId  = myQuery.value(fieldId).toULongLong();
        }
        qDebug()<<"Examining msg Id: "<<id;
        //check if its not on the excluded list.
        if ( !excluded.contains(id) ) {
          //ok, return the msg, increment count.
          result = myQuery.value(fieldMsg).toString();
          randomMsgIncrementCount(id);
          //modify the excluded list, insert this one.
          excluded.append(id);
          //we exit from the while loop.
          qDebug()<<" We got msg:"<<result;
          break;
        }
      }
    }
    else {
      setError(myQuery.lastError().text());
    }
  }
  if (result.isEmpty() && firstId > 0) {
    result = firstMsg;
    randomMsgIncrementCount(firstId);
    excluded << firstId;
    qDebug()<<"Returning the fist message!";
  }
  return result;
}

void Azahar::randomMsgIncrementCount(qulonglong id)
{
  if (!db.isOpen()) db.open();
  if (db.isOpen()) {
    QSqlQuery myQuery(db);
    if (myQuery.exec(QString("update random_msgs set count=count+1 where id=%1;").arg(id))) {
      qDebug()<<"Random Messages Count updated";
    }
    else {
      qDebug()<<"ERROR: "<<myQuery.lastError();
    }
  }
}

bool Azahar::insertRandomMessage(const QString &msg, const int &season)
{
  bool result = false;
  if (!db.isOpen()) db.open();
  QSqlQuery query(db);
  query.prepare("INSERT INTO random_msgs (message, season, count) VALUES (:message, :season, :count);");
  query.bindValue(":msg", msg);
  query.bindValue(":season", season);
  query.bindValue(":count", 0);
  if (!query.exec()) {
    setError(query.lastError().text());
  }
  else result=true;
  return result;
}

#include "azahar.moc"
