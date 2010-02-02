/**************************************************************************
*   Copyright © 2007-2010 by Miguel Chavez Gamboa                         *
*   miguel@lemonpos.org                                                   *
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

// #include <QString>
#include <QDebug>
#include <QDateTime>
#include <QFile>

#include "gaveta.h"


Gaveta::Gaveta()
{
  unused = true;
  setAvailableInCash(0.0);
  startDateTime = QDateTime::currentDateTime();
  tIds.clear();
  cashflowIds.clear();
}

Gaveta::~Gaveta()
{
}

void Gaveta::setPrinterDevice(QString file)
{
  printerDevice = file;
}

void Gaveta::setInitialAmount(double qty)
{
  initialAmount = qty;
}

//This is executed only when a new user is logged ( each startOperation() )
void Gaveta::setAvailableInCash(double amount)
{
  availableInCash = amount;
  initialAmount = amount;
  setAvailableInCard(0.0);
  in = 0;
  out = 0;
  initialAmount = 0.0;
  tIds.clear();
  cashflowIds.clear();
  if (amount>0) unused=false; else unused=true;
}

void Gaveta::setAvailableInCard(double amount)
{
  availableInCard = amount;
}

void Gaveta::substractCash(double amount)
{
  if (availableInCash < amount) qDebug()<<"WARNING: Available money in drawer is less than the amount to substract.";
  availableInCash -= amount;
  out += amount;
}

void Gaveta::substractCard(double amount)
{
  //Can we do this???
  qDebug()<<"Not implemented Gaveta::substractInCard:"<<amount;
}

void Gaveta::addCash(double amount)
{
  availableInCash += amount;
  in += amount;
}

void Gaveta::addCard(double amount)
{
  availableInCard += amount;
  //Not increment in amount... because is not money physically in the drawer...
}

double Gaveta::getAvailableInCash()
{
   return availableInCash;
}

double Gaveta::getAvailableInCard()
{
  return availableInCard;
}

bool Gaveta::isUnused()
{
  bool result = false;
  if (unused && (availableInCash==0) && (tIds.count()==0)) result = true; else result = false;
  return result;
}

void Gaveta::open()
{
  QFile file(printerDevice);
  if (file.open(QIODevice::ReadWrite)) {
    qDebug()<<"Pinter | Openning drawer...";
    QTextStream out(&file);
    out << "\x10\x14\x01\x00\x01";              // Pulse on pin 2 100 ms
    out << "\x10\x14\x01\x01\x01";              // Pulse on pin 5 100 ms
    QString msg = "\x10\x14\x01\x00\x01";
    qint64 r = file.write( msg.toUtf8() );
    qDebug()<<"bytes sent:"<<r;
    if (r == -1) { qDebug()<<"ERRROR Writing file.";}
    file.close();
  } else qDebug()<<"ERROR: Could not open printer...";
}

double Gaveta::getInitialAmount()
{
  return initialAmount;
}

double Gaveta::getInAmount()
{
  return in;
}

double Gaveta::getOutAmount()
{
  return out;
}

void Gaveta::insertTransactionId(qulonglong id)
{
  tIds.append(id);
}

QList<qulonglong> Gaveta::getTransactionIds()
{
  return tIds;
}

QList<qulonglong> Gaveta::getCashflowIds()
{
  return cashflowIds;
}

void Gaveta::insertCashflow(qulonglong id)
{
  cashflowIds.append(id);
}

void Gaveta::incCardTransactions()
{
  cardTransactions  += 1;
  totalTransactions +=1;
}

void Gaveta::incCashTransactions()
{
  cashTransactions  += 1;
  totalTransactions +=1;
}

int Gaveta::getCardTransactionsCount()
{
  return cardTransactions;
}

int Gaveta::getCashTransactionsCount()
{
  return cashTransactions;
}

int Gaveta::getTransactionsCount()
{
  return totalTransactions;
}

void Gaveta::setStartDateTime(QDateTime datetime)
{
  startDateTime = datetime;
}

QDateTime Gaveta::getStartDateTime()
{
  return startDateTime;
}


