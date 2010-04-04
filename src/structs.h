/***************************************************************************
 *   Copyright (C) 2007-2009 by Miguel Chavez Gamboa                  *
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

#ifndef MYSTRUCTS_H
#define MYSTRUCTS_H

#include <QString>
#include <QByteArray>
#include <QPixmap>
#include <QDateTime>
#include <QDate>
#include <QHash>

struct ProductInfo {
  qulonglong code;
  QString desc;
  double price;
  double disc;
  double discpercentage;
  bool   validDiscount;
  double cost;
  double tax;
  double extratax;
  double totaltax;//in money...
  QByteArray photo;
  double stockqty;
  int units;
  QString unitStr;
  int category;
  double utility;
  int row; // New: Sept 30, 2007: To store the row where the product is located in the listView.
  qulonglong points; //New: Dec 15 2007: To store the points the product gives.
  double qtyOnList;
  double purchaseQty; //New: Jan 13 2007: For purchase editor...
  qulonglong lastProviderId;
  QString alphaCode;
  double soldUnits; // mch 21Nov09 FOR PRINTED REPORT - LOW STOCK
  // for grouped products and special orders
  bool isAGroup;
  bool isARawProduct;
  QString groupElementsStr;
};

struct UserInfo
{
  unsigned int id;
  QString username;
  QString password;
  QString salt;
  QString name;
  QString address;
  QString cell;
  QString phone;
  QByteArray photo;
  int role;
};

struct ClientInfo
{
  unsigned int id;
  QString      name;
  QString      address;
  QString      phone;
  QString      cell;
  qulonglong   points;
  double discount;
  QByteArray   photo;
  QDate        since;
};

struct OfferInfo
{
  qulonglong productCode;
  double     discount;
  QDate      dateStart;
  QDate      dateEnd;
};

struct TransactionInfo
{
  qulonglong id;
  int        type;
  double     amount;
  QDate      date;
  QTime      time;
  double     paywith;
  double     changegiven;
  int        paymethod;
  int        state;
  qulonglong userid;
  qulonglong clientid;
  QString    cardnumber;
  QString    cardauthnum;
  int        itemcount;
  QString    itemlist;
  double     disc;
  double     discmoney;
  qulonglong points;
  double     utility;
  int        terminalnum;
  //QString    groups;  //DEPRECATED
  QString    specialOrders;
  qulonglong providerid;
  qulonglong balanceId; //to store balance where it was sold. For SESSIONS.
  double     totalTax; //in money.
};

struct BalanceInfo
{
  qulonglong id;
  QDateTime  dateTimeStart;
  QDateTime  dateTimeEnd;
  qulonglong userid;
  QString    username;
  double     initamount;
  double     in;
  double     out;
  double     cash;
  double     card;
  QString    transactions;
  int        terminal;
  QString    cashflows;//28DIC09
  bool       done;
};

struct PrintBalanceInfo
{
  QString     thBalanceId;
  QString     storeName;
  QString     storeAddr;
  QString     thTitle;
  QString     thDeposit;
  QString     thIn;
  QString     thOut;
  QString     thInDrawer;
  QString     thTitleDetails;
  QString     thTitleCFDetails;
  QString     thTrId;
  QString     thTrTime;
  QString     thTrAmount;
  QString     thTrPaidW;
  QString     thTrPayMethod;
  QPixmap     storeLogo;
  QString     startDate;
  QString     endDate;
  // qtys as string - local aware / translated
  QString     initAmount;
  QString     inAmount;
  QString     outAmount;
  QString     cashAvailable;
  bool        logoOnTop;
  QString     thCFType;
  QString     thCFReason;
  QString     thCFDate;
  QStringList trList;
  QStringList cfList;
};

struct pieProdInfo
{
  double count;
  QString name;
  QString unitStr;
  qulonglong code;
};

struct ProfitRange
{
  double min;
  double max;
};

struct TicketLineInfo {
  double qty;
  QString unitStr;
  QString desc;
  double price;
  double disc;
  double total;
  QString geForPrint;
  bool   completePayment;
  bool   isGroup;
  double payment;
  QDateTime  deliveryDateTime;
  double tax; //total tax in Percentage.
};

struct TicketInfo {
  qulonglong number;
  double total;
  double change;
  double paidwith;
  int itemcount;
  QString cardnum;
  QString cardAuthNum;
  bool paidWithCard;
  double clientDisc;
  double clientDiscMoney;
  qulonglong buyPoints;
  qulonglong clientPoints;
  qulonglong clientid; // 14 Abril 08
  QList<TicketLineInfo> lines;
  QDateTime datetime;
  bool hasSpecialOrders;
  bool completingSpecialOrder;
  double totalTax;
};

struct PrintTicketInfo {
  QString    storeName;
  QString    storeAddr;
  QString    storePhone;
  QString    ticketMsg;
  QPixmap    storeLogo;
  QString    salesPerson;
  QString    terminal;
  QString    thPhone;
  QString    thDate;
  QString    thProduct;
  QString    thQty;
  QString    thPrice;
  QString    thTotal;
  QString    thTotals;
  QString    thDiscount;
  QString    thArticles;
  QString    thPoints;
  QString    thTicket;
  QString    thPaid;
  QString    tDisc;
  QString    thCard;
  QString    thCardAuth;
  double     totDisc;
  TicketInfo ticketInfo;
  bool       logoOnTop;
  QString    paymentStrComplete;
  QString    paymentStrPrePayment;
  QString    nextPaymentStr;
  QString    lastPaymentStr;
  QString    deliveryDateStr;
  QString    clientDiscountStr;
  double     clientDiscMoney;
  QString    randomMsg;
  QString    taxes;
  QString    thTax;
};

//TODO: add grouped products and special orders
//      is it convenient? in case a pack of 6 water botles is not convenient, but a combo "My burger package"
//      consisting of one burger, one soda and one fried potatoes.
struct TransactionItemInfo
{
  qulonglong transactionid;
  int        position;
  qulonglong productCode;
  double     qty;
  double     points;
  QString    unitStr;
  double     cost;
  double     price;
  double     disc;
  double     total;
  QString    name;
  //for special orders, also productCode will be 0 for special orders
  QString    soId;
  double     payment;
  bool       completePayment;
  bool       isGroup;
  QDateTime  deliveryDateTime;
  double     tax; // total tax in percentage.
};


struct CashFlowInfo
{
  qulonglong id;
  int type;
  qulonglong userid;
  double amount;
  QString reason;
  QDate date;
  QTime time;
  qulonglong terminalNum;
  //next are for cashflow into balance printing
  QString typeStr;
};

struct AmountAndProfitInfo
{
    double amount;
    double profit;
};

struct PrintEndOfDayInfo
{
  QString    storeName;
  QString    storeAddr;
  QPixmap    storeLogo;
  QString    salesPerson;
  QString    terminal;
  QString    thTitle;
  QString    thDate;
  QString    thTime;
  QString    thTicket;
  QString    thAmount;
  QString    thProfit;
  QString    thPayMethod;
  QString    thTotalSales;
  QString    thTotalProfit;
  QStringList trLines;
  bool       logoOnTop;
};

struct PrintLowStockInfo
{
  QString    storeName;
  QString    storeAddr;
  QPixmap    storeLogo;
  QString    hTitle;
  QString    hDate; //and time
  QString    hDesc;
  QString    hCode;
  QString    hQty;
  QString    hUnitStr;
  QString    hSoldU;
  QStringList pLines;
  bool       logoOnTop;
};

struct SpecialOrderInfo
{
  qulonglong orderid;
  qulonglong saleid;
  QString    name;
  double     qty;
  double     price;
  double     cost;
  int        status;
  int        units;
  QString    unitStr;
  QString    groupElements;
  QString    notes;
  int        insertedAtRow;
  QString    geForPrint;
  double     payment; //anticipos
  bool       completePayment;
  QDateTime  dateTime;
  qulonglong completedOnTrNum;
  QDateTime  deliveryDateTime;
  qulonglong userId;
  qulonglong clientId;
  double     averageTax;
};

struct GroupInfo
{
  QString name;
  double  cost;
  double  price;
  double  taxMoney;
  double  tax;
  double  count; // total of items in the group
  bool    isAvailable; //based on stockqty for each product (and its qtys).
  QHash<qulonglong, ProductInfo> productsList;
};

#endif
