/***************************************************************************
 *   Copyright (C) 2007-2009 by Miguel Chavez Gamboa                       *
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
#include "lemonview.h"
#include "settings.h"
#include "inputdialog.h"
#include "productdelegate.h"
#include "pricechecker.h"
#include "../dataAccess/azahar.h"

//StarMicronics printers
// #include "printers/sp500.h"

#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>

#include <QWidget>
#include <QStringList>
#include <QTimer>
#include <QColor>
#include <QPixmap>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCodec>
#include <QRegExp>
#include <QRegExpValidator>
#include <QValidator>
#include <QGridLayout>
#include <QDesktopWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QTextDocument>
#include <QTextEdit>
#include <QPushButton>

#include <klocale.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kpassivepopup.h>
#include <KNotification>

/* Widgets zone                                                                                                         */
/*======================================================================================================================*/

///NOTE: Testing with KDE 4.2, this TicketPopup dialog was not shown. So it was changed.

class TicketPopup : public QDialog
{
  private:
    QGridLayout *gridLayout;
    QLabel *imagelabel;
    QTextEdit *editText;
    QTimer *timer;
    
  public:
    TicketPopup(QWidget *parent=0, QString text="", QPixmap pixmap=0, int timeToClose=1000)
    {
      setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
      setWindowModality(Qt::ApplicationModal);
      setObjectName("main");
      
      gridLayout = new QGridLayout(this);
      imagelabel = new QLabel(this);
      imagelabel->setPixmap(pixmap);
      imagelabel->setAlignment(Qt::AlignCenter);
      gridLayout->addWidget(imagelabel, 0, 0);
      editText = new QTextEdit(this);
      editText->setHtml(text);
      editText->setReadOnly(true);
      gridLayout->addWidget(editText, 1, 0);
      gridLayout->setMargin(17);
      
      timer = new QTimer(this);
      timer->setInterval(timeToClose);
      connect(timer, SIGNAL(timeout()), this, SLOT(close()));
      
      QString path = KStandardDirs::locate("appdata", "images/");
      QString filen = path + "/imgPrint.png";
      QPixmap pix(filen);
      setMask(pix.mask());
      QString st;
      st = QString("main { background-image: url(%1);}").arg(filen);
      setStyleSheet(st);
    }
    void setPixmap(QPixmap pixmap) { imagelabel->setPixmap(pixmap); }
    void popup()
    {
      //NOTE: Why before show() the frameGeometry is bigger, and after showing it, it resizes itself?
      move(2000,2000);
      show();
      int x = (QApplication::desktop()->width()/2 )-(frameGeometry().width()/2);
      int y = (QApplication::desktop()->height()/2)-(frameGeometry().height()/2);
      setGeometry(x,y,335,340);
      timer->start();
    }
    virtual void paint(QPainter *) {}
  protected:
    void paintEvent(QPaintEvent *e)
    {
      QPainter painter;
      painter.begin(this);
      painter.setClipRect(e->rect());
      painter.setRenderHint(QPainter::Antialiasing);
      
      paint(&painter);
      painter.restore();
      painter.save();
      int level = 180;
      painter.setPen(QPen(QColor(level, level, level), 6));
      painter.setBrush(Qt::NoBrush);
      painter.drawRect(rect());
    }
    
};

class BalanceDialog : public QDialog
{
  private:
    QGridLayout *gridLayout;
    QTextEdit *editText;
    QPushButton *buttonClose;

  public:
    BalanceDialog(QWidget *parent=0, QString str="")
    {
      setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
      setWindowModality(Qt::ApplicationModal);

      gridLayout = new QGridLayout(this);
      editText = new QTextEdit(str);
      editText->setReadOnly(true);
      editText->setMinimumSize(QSize(320,450));
      gridLayout->addWidget(editText, 0, 0);
      buttonClose = new QPushButton(this);
      buttonClose->setText(i18n("Continue"));
      buttonClose->setDefault(true);
      buttonClose->setShortcut(Qt::Key_Enter);
      gridLayout->addWidget(buttonClose, 1,0);

      connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
      //connect(buttonClose, SIGNAL(clicked()), parent, SLOT(slotDoStartOperation()));
    }
    virtual void paint(QPainter *) {}
  protected:
    void paintEvent(QPaintEvent *e)
    {
      QPainter painter;
      painter.begin(this);
      painter.setClipRect(e->rect());
      painter.setRenderHint(QPainter::Antialiasing);

      paint(&painter);
      painter.restore();
      painter.save();
      int level = 180;
      painter.setPen(QPen(QColor(level, level, level), 6));
      painter.setBrush(Qt::NoBrush);
      painter.drawRect(rect());
    }

};


void lemonView::cancelByExit()
{
  clearUsedWidgets();
  refreshTotalLabel();
  preCancelCurrentTransaction();
  Azahar * myDb = new Azahar;
  myDb->setDatabase(db);
  myDb->deleteEmptyTransactions();
  if (db.isOpen()) db.close();
}

lemonView::lemonView(QWidget *parent) //: QWidget(parent)
{
  drawerCreated=false;
  modelsCreated=false;
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  db = QSqlDatabase::addDatabase("QMYSQL"); //moved here because calling multiple times cause a crash on certain installations (Not kubuntu 8.10).
  ui_mainview.setupUi(this);
  dlgLogin = new LoginWindow(this,
                             i18n("Welcome to Lemon"),
                             i18n("Enter username and password to start using the system."),
                             LoginWindow::FullScreen);
  dlgPassword = new LoginWindow(this,
                             i18n("Authorisation Required"),
                             i18n("Enter administrator password please."),
                             LoginWindow::PasswordOnly);

  refreshTotalLabel();
  QTimer::singleShot(1000, this, SLOT(setupDB()));
  setAutoFillBackground(true);
  QTimer::singleShot(1100, this, SLOT(login()));
  QTimer *timerClock = new QTimer(this);


  //Signals
  connect(timerClock, SIGNAL(timeout()), SLOT(timerTimeout()) );
  //connect(ui_mainview.editItemDescSearch, SIGNAL(returnPressed()), this, SLOT(doSearchItemDesc()));
  connect(ui_mainview.editItemDescSearch, SIGNAL(textEdited(const QString&)), this, SLOT(doSearchItemDesc()));
  connect(ui_mainview.editItemCode, SIGNAL(returnPressed()), this, SLOT(doEmitSignalQueryDb()));
  connect(this, SIGNAL(signalQueryDb(QString)), this, SLOT(insertItem(QString)) );
  connect(ui_mainview.tableWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(itemDoubleClicked(QTableWidgetItem*)) );
  connect(ui_mainview.tableSearch, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(itemSearchDoubleClicked(QTableWidgetItem*)) );
  connect(ui_mainview.tableWidget, SIGNAL(itemClicked(QTableWidgetItem*)), SLOT(displayItemInfo(QTableWidgetItem*)));
  //connect(ui_mainview.listView, SIGNAL(activated(const QModelIndex &)), SLOT(listViewOnClick(const QModelIndex &)));
  connect(ui_mainview.listView, SIGNAL(clicked(const QModelIndex &)), SLOT(listViewOnClick(const QModelIndex &)));
  connect(ui_mainview.listView, SIGNAL(entered(const QModelIndex &)), SLOT(listViewOnMouseMove(const QModelIndex &)));
  connect(ui_mainview.buttonSearchDone, SIGNAL(clicked()), SLOT(buttonDone()) );
  connect(ui_mainview.checkCard, SIGNAL(toggled(bool)), SLOT(checksChanged())  );
  connect(ui_mainview.checkCash, SIGNAL(toggled(bool)), SLOT(checksChanged())  );
  connect(ui_mainview.editAmount,SIGNAL(returnPressed()), SLOT(finishCurrentTransaction()) );
  connect(ui_mainview.editAmount, SIGNAL(textChanged(const QString &)), SLOT(refreshTotalLabel()));
  connect(ui_mainview.editCardNumber, SIGNAL(returnPressed()), SLOT(goSelectCardAuthNumber()) );
  connect(ui_mainview.editCardAuthNumber, SIGNAL(returnPressed()), SLOT(finishCurrentTransaction()) );
  connect(ui_mainview.splitter, SIGNAL(splitterMoved(int, int)), SLOT(setUpTable()));
  connect(ui_mainview.comboClients, SIGNAL(currentIndexChanged(int)), SLOT(comboClientsOnChange()));
  connect(ui_mainview.btnChangeSaleDate, SIGNAL(clicked()), SLOT(showChangeDate()));

  ui_mainview.editTicketDatePicker->setDate(QDate::currentDate());
  connect(ui_mainview.editTicketDatePicker, SIGNAL(dateChanged(const QDate &)), SLOT(setHistoryFilter()) );
  connect(ui_mainview.btnTicketDone, SIGNAL(clicked()), SLOT(btnTicketsDone()) );
  connect(ui_mainview.btnTicketPrint, SIGNAL(clicked()), SLOT(printSelTicket()) );
  connect(ui_mainview.ticketView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(itemHIDoubleClicked(const QModelIndex &)) );

  timerClock->start(1000);

  drawer = new Gaveta();
  drawer->setPrinterDevice(Settings::printerDevice());
  drawerCreated = true;
  
  operationStarted = false;
  productsHash.clear();
  clientsHash.clear();
  //ui_mainview.lblClientPhoto->hide();
  ui_mainview.labelInsertCodeMsg->hide();
  transDateTime = QDateTime::currentDateTime();
  ui_mainview.editTransactionDate->setDateTime(transDateTime);
  ui_mainview.groupSaleDate->hide();

  

  clearUsedWidgets();
  loadIcons();
  setUpInputs();
  QTimer::singleShot(500, this, SLOT(setUpTable()));
  ui_mainview.groupWidgets->setCurrentIndex(pageMain);
  ui_mainview.mainPanel->setCurrentIndex(pageMain);

  QTimer::singleShot(1000, this, SLOT(setupGridView()));
}

void lemonView::showChangeDate()
{
  ui_mainview.groupSaleDate->show();
}

void lemonView::setupGridView()
{
  if (Settings::showGrid()) emit signalShowProdGrid();
  else showProductsGrid(false);
  
}

void lemonView::loadIcons()
{
  ui_mainview.labelImageSearch->setPixmap(DesktopIcon("edit-find", 64));
  QString logoBottomFile = KStandardDirs::locate("appdata", "images/logo_bottom.png");
  ui_mainview.labelBanner->setPixmap(QPixmap(logoBottomFile));
  ui_mainview.labelBanner->setAlignment(Qt::AlignCenter);
}

void lemonView::setUpTable()
{
  QSize tableSize = ui_mainview.tableWidget->size();
  int portion = tableSize.width()/10;
  ui_mainview.tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colCode, portion); //BAR CODE
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colDesc, (portion*4)+9); //DESCRIPTION
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colPrice, portion); //PRICE
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colQty, portion-20);  //QTY
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colUnits, portion-15);//UNITS
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colDisc, portion); //Discount
  ui_mainview.tableWidget->horizontalHeader()->resizeSection(colDue, portion+10); //DUE
  //search table
  tableSize = ui_mainview.tableSearch->size();
  ui_mainview.tableSearch->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui_mainview.tableSearch->horizontalHeader()->resizeSection(0, 2*(tableSize.width()/4));
  ui_mainview.tableSearch->horizontalHeader()->resizeSection(1, tableSize.width()/4);
  ui_mainview.tableSearch->horizontalHeader()->resizeSection(2, tableSize.width()/4);
}

void lemonView::setUpInputs()
{
  //TODO: Tratar de poner un filtro con lugares llenos de ceros, e ir insertando los numeros.
  //For amount received.
  QRegExp regexpA("[0-9]*[//.]{0,1}[0-9][0-9]*"); //Cualquier numero flotante (0.1, 100, 0, .10, 100.0, 12.23)
  QRegExpValidator * validatorFloat = new QRegExpValidator(regexpA,this);
  ui_mainview.editAmount->setValidator(validatorFloat);
  //Item code (to insert) //
  QRegExp regexpC("[1-9]+[0-9]*[//.]{0,1}[0-9]{0,2}[xX]{0,1}[0-9]{0,13}");
  QRegExpValidator * validatorEAN13 = new QRegExpValidator(regexpC, this);
  ui_mainview.editItemCode->setValidator(validatorEAN13);
  QRegExp regexpAN("[A-Za-z_0-9\\\\/\\-]+");//any letter, number, both slashes, dash and lower dash.
  QRegExpValidator *regexpAlpha = new QRegExpValidator(regexpAN, this);
  ui_mainview.editCardAuthNumber->setValidator(regexpAlpha);

  //ui_mainview.editAmount->setInputMask("000,000.00");
}

void lemonView::timerTimeout()
{
  emit signalUpdateClock();

  //remover comas..
//   QString s="1,231.22";
//   QString x = s.replace(QRegExp("[\\$,]"), "");
//   qDebug()<<"s:"<<s<<" x:"<<x;

}

void lemonView::clearLabelPayMsg()
{
  ui_mainview.labelPayMsg->clear();
}

void lemonView::clearLabelInsertCodeMsg()
{
  ui_mainview.labelInsertCodeMsg->clear();
  ui_mainview.labelInsertCodeMsg->hide();
}

void::lemonView::clearLabelSearchMsg()
{
  ui_mainview.labelSearchMsg->clear();
}

void lemonView::setTheSplitterSizes(QList<int> s)
{
  ui_mainview.splitter->setSizes(s);
}

QList<int> lemonView::getTheSplitterSizes()
{
  return ui_mainview.splitter->sizes();
}

//This ensures that when not connected to mysql, the user can configure the db settings and then trying to connect
//with the new settings...
void lemonView::settingsChanged()
{
  //Total label (and currency label)
  refreshTotalLabel();

  ///This is a temporal workaround for the crash. I think is due to a kde bug. It does not crashes with kde 4.1.4 on kubuntu
  //Reconnect to db..
  if (db.isOpen()) db.close();
  qDebug()<<"-Config Changed- reconnecting to database..";

  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());
  connectToDb();
  setupModel();
  setupHistoryTicketsModel();
  setupClients();

}

void lemonView::settingsChangedOnInitConfig()
{
  qDebug()<<"==> Initial Config Changed- connecting to database and calling login...";
  //the db = QSqlDatabase... thing is causing a crash.
  //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  //db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());

  ///This is also affected by the weird crash.
  connectToDb();
  setupModel();
  setupHistoryTicketsModel();
  setupClients();

  login();
}

void lemonView::showEnterCodeWidget()
{
  ui_mainview.groupWidgets->setCurrentIndex(pageMain);
  // BFB. Toggle editItemCode and editFilterByDesc.
  if (ui_mainview.editItemCode->hasFocus()){
    ui_mainview.rbFilterByDesc->setChecked(true);
    ui_mainview.editFilterByDesc->setFocus();
  }else
    ui_mainview.editItemCode->setFocus();
  setUpTable();
}

void lemonView::showSearchItemWidget()
{
  ui_mainview.mainPanel->setCurrentIndex(pageSearch); // searchItem
  ui_mainview.editItemDescSearch->setFocus();
  setUpTable();
}

void lemonView::buttonDone()
{
  ui_mainview.tableSearch->setRowCount(0);
  ui_mainview.labelSearchMsg->setText("");
  ui_mainview.editItemDescSearch->setText("");
  ui_mainview.editItemCode->setCursorPosition(0);
  ui_mainview.mainPanel->setCurrentIndex(0); // back to welcome widget
}

void lemonView::checksChanged()
{
  if (ui_mainview.checkCash->isChecked())
  {
    ui_mainview.stackedWidget->setCurrentIndex(0);
    ui_mainview.editAmount->setFocus();
    ui_mainview.editAmount->setSelection(0,ui_mainview.editAmount->text().length());
  }//cash
  else  //Card, need editCardkNumber...
  {
    ui_mainview.stackedWidget->setCurrentIndex(1);
    ui_mainview.editAmount->setText(QString::number(totalSum));
    ui_mainview.editCardNumber->setFocus();
    ui_mainview.editCardNumber->setSelection(0,ui_mainview.editCardNumber->text().length());
  }
  refreshTotalLabel();
}

void lemonView::clearUsedWidgets()
{
  ui_mainview.editAmount->setText("0.0");
  ui_mainview.editCardNumber->setText("");
  ui_mainview.editCardAuthNumber->setText("");
  ui_mainview.tableWidget->clearContents();
  ui_mainview.tableWidget->setRowCount(0);
  totalSum = 0.0;
  buyPoints = 0;
  ui_mainview.labelDetailTax1->setText("");
  ui_mainview.labelDetailTax2->setText("");
  ui_mainview.labelDetailUnits->setText("");
  ui_mainview.labelDetailDesc->setText(i18n("No product selected"));
  ui_mainview.labelDetailPrice->setText("");
  ui_mainview.labelDetailDiscount->setText("");
  ui_mainview.labelDetailTotalTaxes->setText("");
  ui_mainview.labelDetailPhoto->clear();
  ui_mainview.labelDetailPoints->clear();
}

void lemonView::askForIdToCancel()
{
  bool continuar=false;
  if (Settings::lowSecurityMode()) {//qDebug()<<"LOW security mode";
    continuar=true;
  } else if (Settings::requiereDelAuth()) {// qDebug()<<"NO LOW security mode, but AUTH REQUIRED!";
    dlgPassword->show();
    dlgPassword->hide();
    dlgPassword->clearLines();
    continuar = dlgPassword->exec();
  } else {//     qDebug()<<"NO LOW security mode, NO AUTH REQUIRED...";
    continuar=true;
  }

  if (continuar) { //show input dialog to get ticket number
    bool ok=false;
    qulonglong id = 0;
    InputDialog *dlg = new InputDialog(this, true, dialogTicket, i18n("Enter the ticket number to cancel"));
    if (dlg->exec())
    {
      id = dlg->iValue;
      ok = true;
    }
    if (ok) {                 // NOTE :
      cancelTransaction(id); //Mark as cancelled in database..  is this transaction
                            //done in the current operation, or a day ago, a month ago, 10 hours ago?
                           //Allow cancelation of same day of sell, or older ones too?
    }//ok=true
  } //continuar
}

void lemonView::askForTicketToReturnProduct()
{
  bool continuar=false;
  if (Settings::lowSecurityMode()) {//qDebug()<<"LOW security mode";
  continuar=true;
  } else if (Settings::requiereDelAuth()) {// qDebug()<<"NO LOW security mode, but AUTH REQUIRED!";
  dlgPassword->show();
  dlgPassword->hide();
  dlgPassword->clearLines();
  continuar = dlgPassword->exec();
  } else {//     qDebug()<<"NO LOW security mode, NO AUTH REQUIRED...";
  continuar=true;
  }
  
  if (continuar) { //show input dialog to get ticket number
    bool ok=false;
    qulonglong id = 0;
    InputDialog *dlg = new InputDialog(this, true, dialogTicket, i18n("Enter the ticket number"));
    if (dlg->exec())
    {
      id = dlg->iValue;
      ok = true;
    }
    if (ok) {
      // show dialog to select which items to return.
      
    }//ok=true
  } //continuar
}

void lemonView::focusPayInput()
{
  ui_mainview.groupWidgets->setCurrentIndex(pageMain);
  ui_mainview.editAmount->setFocus();
  ui_mainview.editAmount->setSelection(0, ui_mainview.editAmount->text().length());
}

void lemonView::goSelectCardAuthNumber()
{
  ui_mainview.editCardAuthNumber->setFocus();
}

lemonView::~lemonView()
{
  drawerCreated=false;
  delete drawer;
}


/* Users zone                                                                                                          */
/*=====================================================================================================================*/

QString lemonView::getLoggedUser()
{
  return loggedUser;
}

QString lemonView::getLoggedUserName(QString id)
{
  QString uname = "";
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  uname = myDb->getUserName(id);
  return uname;
}

unsigned int lemonView::getLoggedUserId(QString uname)
{
  unsigned int iD=0;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  iD = myDb->getUserId(uname);
  return iD;
}

void lemonView::login()
{
  //Make a corteDeCaja
  if (!loggedUser.isEmpty() && operationStarted) {
    corteDeCaja();
    loggedUser = "";
    loggedUserName ="";
    emit signalNoLoggedUser();
  }

  dlgLogin->clearLines();
  if (!db.isOpen()) {
      qDebug()<<"(login): Calling connectToDb()...";
      connectToDb();
  }

  if (!db.isOpen()) {
    qDebug()<<"(4/login): Still unable to open connection to database....";
    QString msg = i18n("Could not connect to database, please press <i><b>login<b></i> button again to raise a database configuration.");
    KNotification *notify = new KNotification("information", this);
    notify->setText(msg);
    QPixmap pixmap = DesktopIcon("dialog-error",32);
    notify->setPixmap(pixmap);
    notify->sendEvent();
  } else {
    if ( dlgLogin->exec() ) {
      loggedUser = dlgLogin->username();
      loggedUserName = getLoggedUserName(loggedUser);
      loggedUserId = getLoggedUserId(loggedUser);
      emit signalLoggedUser();
      if (loggedUser == "admin") {
	emit signalAdminLoggedOn();
	//if (!canStartSelling()) startOperation();
      } else {
	emit signalAdminLoggedOff();
	slotDoStartOperation();
      }
    } else {
      loggedUser ="";
      loggedUserName = "";
      loggedUserId = 0;
      emit signalNoLoggedUser();
      if (dlgLogin->wantToQuit()) qApp->quit();
    }
  }
}

bool lemonView::validAdminUser()
{
  bool result = false;
  if (Settings::lowSecurityMode()) result = true;
  else {
      dlgPassword->show();
      dlgPassword->hide();
      dlgPassword->clearLines();
      if (dlgPassword->exec())  result = true;
  }
  return result;
}

/* Item things: shopping list, search, insert, delete, calculate total */
/*--------------------------------------------------------------------*/

void lemonView::doSearchItemDesc()
{
  //clear last search
  ui_mainview.tableSearch->clearContents();
  ui_mainview.tableSearch->setRowCount(0);
  //Search
  QString desc = ui_mainview.editItemDescSearch->text();
  QRegExp regexp = QRegExp(desc);
    if (!regexp.isValid() || desc.isEmpty())  desc = "*";
  if (!db.isOpen()) db.open();

  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QList<qulonglong> pList = myDb->getProductsCode(desc); //busca con regexp...
  //iteramos la lista
  for (int i = 0; i < pList.size(); ++i) {
     qulonglong c = pList.at(i);
     ProductInfo pInfo = myDb->getProductInfo(c);
     //insert each product to the search table...
     int rowCount = ui_mainview.tableSearch->rowCount();
      ui_mainview.tableSearch->insertRow(rowCount);
      QTableWidgetItem *tid = new QTableWidgetItem(pInfo.desc);
      if (pInfo.stockqty == 0) {
        QBrush b = QBrush(QColor::fromRgb(255,100,0), Qt::SolidPattern);
        tid->setBackground(b);
      }
      else if (pInfo.stockqty > 0 && pInfo.stockqty < Settings::stockAlertValue() ) {//NOTE:This must be shared between lemon and squeeze
        QBrush b = QBrush(QColor::fromRgb(255,176,73), Qt::SolidPattern);
        tid->setBackground(b);
      }
      ui_mainview.tableSearch->setItem(rowCount, 0, tid);
      //NOTE:bug fixed Sept 26 2008: Without QString::number, no data was inserted.
      // if it is passed a numer as the only parameter to QTableWidgetItem, it is taken as a type
      // and not as a data to display.
      ui_mainview.tableSearch->setItem(rowCount, 1, new QTableWidgetItem(QString::number(pInfo.price)));
      ui_mainview.tableSearch->setItem(rowCount, 2, new QTableWidgetItem(QString::number(pInfo.code)));
      ui_mainview.tableSearch->resizeRowsToContents();
    }
    if (pList.count()>0) ui_mainview.labelSearchMsg->setText(i18np("%1 item found","%1 items found.", pList.count()));
    else ui_mainview.labelSearchMsg->setText(i18n("No items found."));

  }

int lemonView::getItemRow(QString c)
{
  int result = 0;
  for (int row=0; row<ui_mainview.tableWidget->rowCount(); ++row)
  {
    QTableWidgetItem * item = ui_mainview.tableWidget->item(row, colCode);
    QString icode = item->data(Qt::DisplayRole).toString();
    if (icode == c) {
      result = row;
      break;
    }
  }
  return result; //0 if not found
}

void lemonView::refreshTotalLabel()
{
  double sum=0.0;
  qulonglong points=0;
  if (ui_mainview.tableWidget->rowCount()>0) {
    for (int row=0; row<ui_mainview.tableWidget->rowCount(); ++row)
    {
      QTableWidgetItem *item = ui_mainview.tableWidget->item(row, colDue);
      bool isNumber = false;
      if (item->data(Qt::DisplayRole).canConvert(QVariant::String)) {
        QString text = item->data(Qt::DisplayRole).toString();
        double number = text.toDouble(&isNumber);
        if (isNumber) sum += number;
      }
    }
    ProductInfo info;
    QHashIterator<qulonglong, ProductInfo> i(productsHash);
    while (i.hasNext()) {
      i.next();
      info = i.value();
      points += (info.points*info.qtyOnList);
//       qDebug()<<info.desc<<" qtyOnList:"<<info.qtyOnList;
    }
  }
  buyPoints = points;
  discMoney = (clientInfo.discount/100)*sum;
  totalSum = sum - discMoney;
  ui_mainview.labelTotal->setText(QString("%1").arg(KGlobal::locale()->formatMoney(totalSum)));
  ui_mainview.labelClientDiscounted->setText(i18n("Amount Discounted: %1", KGlobal::locale()->formatMoney(discMoney)));
  long double paid, change;
  bool isNum;
  paid = ui_mainview.editAmount->text().toDouble(&isNum);
  if (isNum) change = paid - totalSum; else change = 0.0;
  if (paid <= 0) change = 0.0;
  ui_mainview.labelChange->setText(QString("%1") .arg(KGlobal::locale()->formatMoney(change)));
}

void lemonView::doEmitSignalQueryDb()
{
  emit signalQueryDb(ui_mainview.editItemCode->text());
}

bool lemonView::incrementTableItemQty(QString code, double q)
{
  double qty  = 1;
  double discount_old=0.0;
  double qty_old=0.0;
  double stockqty=0;
  bool done=false;
  ProductInfo info;

   if (productsHash.contains(code.toULongLong())) {
    //qDebug()<<"Product on hash: "<<code;
    //get product info...
    info = productsHash.value(code.toULongLong());
    //qDebug()<<"  Product on hash discount :"<<info.disc;
    //qDebug()<<"  Product on hash stock qty:"<<info.stockqty<<"real: "<<(myDb->getProductInfo(code.toULongLong())).stockqty;
    //qDebug()<<"  Inserted at row:"<<info.row;

    stockqty = info.stockqty;
    qty = info.qtyOnList;
    qty_old = qty;
    if (stockqty>=q+qty) qty+=q; else {
      QString msg = i18n("<html><font color=red><b>Product not available in stock.</b></font>");
      if (ui_mainview.mainPanel->currentIndex() == pageMain) {
         ui_mainview.labelInsertCodeMsg->setText(msg);
         ui_mainview.labelInsertCodeMsg->show();
         QTimer::singleShot(3000, this, SLOT(clearLabelInsertCodeMsg()));
      }
      if (ui_mainview.mainPanel->currentIndex() == pageSearch) {
         ui_mainview.labelSearchMsg->setText(msg);
         ui_mainview.labelInsertCodeMsg->show();
         QTimer::singleShot(3000, this, SLOT(clearLabelSearchMsg()) );
      }
    }
    QTableWidgetItem *itemQ = ui_mainview.tableWidget->item(info.row, colQty);//item qty
    itemQ->setData(Qt::EditRole, QVariant(qty));
    done = true;
    QTableWidgetItem *itemD = ui_mainview.tableWidget->item(info.row, colDisc);//item discount
    discount_old = itemD->data(Qt::DisplayRole).toDouble();
    //calculate new discount
    double discountperitem = (discount_old/qty_old);
    double newdiscount = discountperitem*qty;
    itemD->setData(Qt::EditRole, QVariant(newdiscount));

    info.qtyOnList = qty;
    //qDebug()<<"  New qty on list:"<<info.qtyOnList;
    productsHash.remove(code.toULongLong());
    productsHash.insert(info.code, info);

    //get item Due to update it.
    QTableWidgetItem *itemDue = ui_mainview.tableWidget->item(info.row, colDue); //4 item Due
    itemDue->setData(Qt::EditRole, QVariant((info.price*qty)-newdiscount));//fixed on april 30 2009 00:35. Added *qty
    refreshTotalLabel();
    QTableWidgetItem *item = ui_mainview.tableWidget->item(info.row, colCode);//item code
    displayItemInfo(item); //TODO: Cambiar para desplegar de ProductInfo.
    ui_mainview.editItemCode->clear();
   }//if productsHash.contains...

  return done;
}

void lemonView::insertItem(QString code)
{
  double qty  = 1;
  QString codeX = code;
  ProductInfo info;
  info.code = 0;
  info.desc = "[INVALID]";

  //now code could contain number of items to insert,example: 10x12345678990
  QStringList list = code.split(QRegExp("[xX]{1,1}"),QString::SkipEmptyParts);
  if (list.count()==2) {
    qty =   list.takeAt(0).toDouble();
    codeX = list.takeAt(0);
  }

  //verify item units and qty..
  if (productsHash.contains(codeX.toULongLong())) {
    info = productsHash.value(codeX.toULongLong());
  
    if (info.units == uPiece) { 
     unsigned int intqty = qty;
     qty = intqty;
    }
  }

  if (!incrementTableItemQty(codeX, qty) ) {
    //As it was not incremented on tableView, so there is not in the productsHash... so we get it from db.
    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);
    info = myDb->getProductInfo(codeX.toULongLong()); //includes discount and validdiscount

    //verify item units and qty..
    if (info.units == uPiece) { 
      unsigned int intqty = qty;
      qty = intqty;
    }
    
    info.qtyOnList = qty;

    QString msg;
    int insertedAtRow = -1;
    bool productFound = false;
    if (info.code > 0) productFound = true;
    double descuento=0.0;
    if (info.validDiscount) descuento = info.disc*qty;//fixed on april 30 2009 00:35. Added *qty
    //qDebug()<<"insertItem:: descuento total del producto:"<<descuento;
    if ( !productFound )  msg = i18n("<html><font color=red><b>Product not found in database.</b></font></html>");
    else if ( productFound && info.stockqty >=  qty )
      insertedAtRow = doInsertItem(codeX, info.desc, qty, info.price, descuento, info.unitStr);
    else msg = i18n("<html><font color=red><b>There are only %1 articles of your choice at stock.</b></font></html>", info.stockqty);
    
    if (!msg.isEmpty()) {
        if (ui_mainview.mainPanel->currentIndex() == pageMain) {
          ui_mainview.labelInsertCodeMsg->setText(msg);
          ui_mainview.labelInsertCodeMsg->show();
          QTimer::singleShot(3000, this, SLOT(clearLabelInsertCodeMsg()));
        }
        if (ui_mainview.mainPanel->currentIndex() == pageSearch) {
          ui_mainview.labelSearchMsg->setText(msg);
          ui_mainview.labelInsertCodeMsg->show();
          QTimer::singleShot(3000, this, SLOT(clearLabelSearchMsg()) );
        }
    ui_mainview.editItemCode->clear();
    }
    info.row = insertedAtRow;
    if (info.row >-1 && info.desc != "[INVALID]" && info.code>0){
      productsHash.insert(codeX.toULongLong(), info);
//       qDebug()<<"INSERTED AT ROW:"<<insertedAtRow;
      QTableWidgetItem *item = ui_mainview.tableWidget->item(info.row, colCode);
      displayItemInfo(item);
    }
  }//if !increment...
}//insertItem


int lemonView::doInsertItem(QString itemCode, QString itemDesc, double itemQty, double itemPrice, double itemDiscount, QString itemUnits)
{
  int rowCount = ui_mainview.tableWidget->rowCount();
  ui_mainview.tableWidget->insertRow(rowCount);
  ui_mainview.tableWidget->setItem(rowCount, colCode, new QTableWidgetItem(itemCode));
  ui_mainview.tableWidget->setItem(rowCount, colDesc, new QTableWidgetItem(itemDesc));
  ui_mainview.tableWidget->setItem(rowCount, colPrice, new QTableWidgetItem(""));//must be empty for HACK
  ui_mainview.tableWidget->setItem(rowCount, colQty, new QTableWidgetItem(QString::number(itemQty)));
  ui_mainview.tableWidget->setItem(rowCount, colUnits, new QTableWidgetItem(itemUnits));
  ui_mainview.tableWidget->setItem(rowCount, colDisc, new QTableWidgetItem(""));//must be empty for HACK
  ui_mainview.tableWidget->setItem(rowCount, colDue, new QTableWidgetItem(""));//must be empty for HACK

  QTableWidgetItem *item = ui_mainview.tableWidget->item(rowCount, colDisc);
  if (itemDiscount>0) {
    QBrush b = QBrush(QColor::fromRgb(255,0,0), Qt::SolidPattern);
    item->setForeground(b);
  }
  //HACK:The next 4 lines are for setting numbers with comas (1,234.00) instead of 1234.00.
  //      seems to be an effect of QVariant(double d)
  item->setData(Qt::EditRole, QVariant(itemDiscount));
  item = ui_mainview.tableWidget->item(rowCount, colDue);
  item->setData(Qt::EditRole, QVariant((itemQty*itemPrice)-itemDiscount)); //fixed on april 30 2009 00:35.
  item = ui_mainview.tableWidget->item(rowCount, colPrice);
  item->setData(Qt::EditRole, QVariant(itemPrice));

  //This resizes the heigh... looks beter...
  ui_mainview.tableWidget->resizeRowsToContents();

  if (productsHash.contains(itemCode.toULongLong())) { //Codigo mudado de int a Unsigned long long: qulonlong
    ProductInfo  info = productsHash.value(itemCode.toULongLong());
    if (info.units != uPiece) itemDoubleClicked(item);//NOTE: Pieces must be id=1 at database!!!! its a workaround.
    //STqDebug()<<"itemDoubleClicked at doInsertItem...";
  }

  refreshTotalLabel();
  // BFB: editFilterbyDesc keeps the focus,
  if (!ui_mainview.editFilterByDesc->hasFocus())
    ui_mainview.editItemCode->setFocus();

  ui_mainview.editItemCode->setText("");
  ui_mainview.editItemCode->setCursorPosition(0);
  ui_mainview.mainPanel->setCurrentIndex(pageMain);

  return rowCount;
}

void lemonView::deleteSelectedItem()
{
  bool continueIt=false;
  bool reinsert = false;
  double qty=0;
  if (ui_mainview.tableWidget->currentRow()!=-1 && ui_mainview.tableWidget->selectedItems().count()>4) {
    if ( !Settings::lowSecurityMode() ) {
        if (Settings::requiereDelAuth() ) {
          dlgPassword->show();
          dlgPassword->hide();
          dlgPassword->clearLines();
          if ( dlgPassword->exec() ) continueIt=true;
        } else continueIt=true; //if requiereDelAuth
    } else continueIt=true; //if no low security

    if (continueIt) {
      int row = ui_mainview.tableWidget->currentRow();
      QTableWidgetItem *item = ui_mainview.tableWidget->item(row, colCode);
      qulonglong code = item->data(Qt::DisplayRole).toULongLong();
      ProductInfo info = productsHash.take(code); //insert it later...
      qty = info.qtyOnList; //this must be the same as obtaining from the table... this arrived on Dec 18 2007
     //if the itemQty is more than 1, decrement it, if its 1, delete it
     //item = ui_mainview.tableWidget->item(row, colDisc);
     //double discount_old = item->data(Qt::DisplayRole).toDouble();
      item = ui_mainview.tableWidget->item(row, colUnits);//get item Units in strings...
      QString iUnitString = item->data(Qt::DisplayRole).toString();
      item = ui_mainview.tableWidget->item(row, colQty); //get Qty
      if ((item->data(Qt::DisplayRole).canConvert(QVariant::Double))) {
        qty = item->data(Qt::DisplayRole).toDouble();
       //NOTE and FIXME:
       //  Here, we are going to delete only items that are bigger than 1. and remove them one by one..
       //  or are we goint to decrement items only sold by pieces?
        if (qty>1 && info.units==uPiece) {
          qty--;
          item->setData(Qt::EditRole, QVariant(qty));
          double price    = info.price;
          double discountperitem = info.disc;
          double newdiscount = discountperitem*qty;
          item = ui_mainview.tableWidget->item(row, colDue);
          item->setData(Qt::EditRole, QVariant((qty*price)-newdiscount));
          item = ui_mainview.tableWidget->item(row, colDisc);
          item->setData(Qt::EditRole, QVariant(newdiscount));
          info.qtyOnList = qty;
          reinsert = true;
        }//if qty>1
        else { //Remove from the productsHash and tableWidget...
          //get item code
          //int removed = productsHash.remove(code);
          productsHash.remove(code);
          ui_mainview.tableWidget->removeRow(row);
          reinsert = false;
        }//qty = 1...
      }//if canConvert
      if (reinsert) productsHash.insert(code, info); //we remove it with .take...
    }//continueIt
  }//there is something to delete..
  refreshTotalLabel();
}

void lemonView::itemDoubleClicked(QTableWidgetItem* item)
{
  int row = item->row();
  QTableWidgetItem *i2Modify = ui_mainview.tableWidget->item(row, colCode);
  qulonglong code = i2Modify->data(Qt::DisplayRole).toULongLong();
  ProductInfo info = productsHash.take(code);
  double dmaxItems = info.stockqty;
  QString msg = i18n("Enter the number of %1", info.unitStr); //Added on Dec 15, 2007

  //Launch a dialog to as the new qty
  double dqty = 0.0;
  bool   ok   = false;
  int    iqty = 0;
  if (info.units == uPiece) {
    if (dmaxItems > 0) {
      ok = true;
      iqty = info.qtyOnList+1;
      //NOTE: Present a dialog to enter a qty or increment by one ?
    }
  }
  else {
    ///FIXME: Alert the user why is restricted to a max items!
    InputDialog *dlg = new InputDialog(this, false, dialogMeasures, msg, 0.001, dmaxItems);
    if (dlg->exec() ) {
      dqty = dlg->dValue;
      ok=true;
    }
  }
  if (ok) {
    double newqty = dqty+iqty; //one must be zero
    //modify Qty and discount...
    i2Modify = ui_mainview.tableWidget->item(row, colQty);
    i2Modify->setData(Qt::EditRole, QVariant(newqty));
    double price    = info.price;
    double discountperitem = info.disc;
    double newdiscount = discountperitem*newqty;
    i2Modify = ui_mainview.tableWidget->item(row, colDue);
    i2Modify->setData(Qt::EditRole, QVariant((newqty*price)-newdiscount));
    i2Modify = ui_mainview.tableWidget->item(row, colDisc);
    i2Modify->setData(Qt::EditRole, QVariant(newdiscount));
    info.qtyOnList = newqty;

    ui_mainview.editItemCode->setFocus();
  } else {
    msg = i18n("<html><font color=red><b>Product not available in stock for the requested quantity.</b></font></html>");
    ui_mainview.labelInsertCodeMsg->setText(msg);
    ui_mainview.labelInsertCodeMsg->show();
    QTimer::singleShot(3000, this, SLOT(clearLabelInsertCodeMsg()));
  }
  productsHash.insert(code, info);
  refreshTotalLabel();
}

void lemonView::itemSearchDoubleClicked(QTableWidgetItem *item)
{
  int row = item->row();
  QTableWidgetItem *cItem = ui_mainview.tableSearch->item(row,2); //get item code
  qulonglong code = cItem->data(Qt::DisplayRole).toULongLong();
  qDebug()<<"Linea 981: Data at column 2:"<<cItem->data(Qt::DisplayRole).toString();
  if (productsHash.contains(code)) {
    int pos = getItemRow(QString::number(code));
    if (pos>=0) {
      QTableWidgetItem *thisItem = ui_mainview.tableWidget->item(pos, colCode);
      ProductInfo info = productsHash.value(code);
      if (info.units == uPiece) incrementTableItemQty(QString::number(code), 1);
      else itemDoubleClicked(thisItem);
    }
  }
  else {
    insertItem(QString::number(code));
  }
  ui_mainview.mainPanel->setCurrentIndex(pageMain);
}

void lemonView::displayItemInfo(QTableWidgetItem* item)
{
  int row = item->row();
  qulonglong code  = (ui_mainview.tableWidget->item(row, colCode))->data(Qt::DisplayRole).toULongLong();
  QString desc  = (ui_mainview.tableWidget->item(row, colDesc))->data(Qt::DisplayRole).toString();
  double price = (ui_mainview.tableWidget->item(row, colPrice))->data(Qt::DisplayRole).toDouble();
  if (productsHash.contains(code)) {
    ProductInfo info = productsHash.value(code);
    QString uLabel=info.unitStr; // Dec 15  2007

    double discP=0.0;
    if (info.validDiscount) discP = info.discpercentage;
    QString str;
    QString tTotalTax= i18n("Taxes:");
    QString tTax    = i18n("Tax:");
    QString tOTax   = i18n("Other taxes:");
    QString tUnits  = i18n("Sold by:");
    QString tPrice  = i18n("Price:");
    QString tDisc   = i18n("Discount:");
    QString tPoints = i18n("Points:");
    //double pWOtax = info.price/(1+((info.tax/*info.tax+info.extratax*/)/100));
    //double tax1m = (info.tax/100)*pWOtax;
    //double tax2m = (info.extratax/100)*pWOtax;
    QPixmap pix;
    pix.loadFromData(info.photo);

    ui_mainview.labelDetailPhoto->setPixmap(pix);
    str = QString("%1 (%2 %)")
        .arg(KGlobal::locale()->formatMoney(info.totaltax)).arg(info.tax);
    ui_mainview.labelDetailTotalTaxes->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tTotalTax).arg(str));
    str = QString("%1 (%2 %)")
        .arg(KGlobal::locale()->formatMoney(info.totaltax)).arg(info.tax);
    ui_mainview.labelDetailTax1->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tTax).arg(str));
    str = QString("%1 (%2 %)")
        .arg(KGlobal::locale()->formatMoney(info.tax));
    ui_mainview.labelDetailTax2->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tOTax).arg(str));
    ui_mainview.labelDetailUnits->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tUnits).arg(uLabel));
    ui_mainview.labelDetailDesc->setText(QString("<html><b>%1</b></html>").arg(desc));
    ui_mainview.labelDetailPrice->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tPrice).arg(KGlobal::locale()->formatMoney(price)));
    ui_mainview.labelDetailDiscount->setText(QString("<html>%1 <b>%2 (%3 %)</b></html>")
        .arg(tDisc).arg(KGlobal::locale()->formatMoney(info.disc)).arg(discP));
    if (info.points>0) {
      ui_mainview.labelDetailPoints->setText(QString("<html>%1 <b>%2</b></html>")
        .arg(tPoints).arg(info.points));
      ui_mainview.labelDetailPoints->show();
    } else ui_mainview.labelDetailPoints->hide();
  }
}

/* TRANSACTIONS ZONE */
/*------------------*/

QString lemonView::getCurrentTransactionString()
{
  return QString::number(currentTransaction);
}

qulonglong  lemonView::getCurrentTransaction()
{
  return currentTransaction;
}

void lemonView::createNewTransaction(TransactionType type)
{
  //If there is an operation started, doit...
  if ( operationStarted ) {
    TransactionInfo info;
    info.type = type;
    info.amount = 0;
    info.date   = QDate::currentDate();
    info.time   = QTime::currentTime();
    info.paywith= 0;
    info.changegiven =0;
    info.paymethod = pCash;
    info.state = tNotCompleted;
    info.userid = loggedUserId;
    info.clientid = clientInfo.id;
    info.cardnumber ="-NA-";
    info.cardauthnum="-NA-";
    info.itemcount= 0;
    info.itemlist = "";
    info.disc = 0;
    info.discmoney = 0;
    info.points = 0;
    info.profit = 0;
    info.terminalnum=Settings::editTerminalNumber();

    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);
    currentTransaction = myDb->insertTransaction(info);
    if (currentTransaction <= 0) {
      KMessageBox::detailedError(this, i18n("Lemon has encountered an error when openning database, click details to see the error details."), myDb->lastError(), i18n("Create New Transaction: Error"));
    }
    else {
      transactionInProgress = true;
      emit signalUpdateTransactionInfo();
    }
   }
  productsHash.clear();
}

void lemonView::finishCurrentTransaction()
{
  bool canfinish = true;
  TicketInfo ticket;
  
  refreshTotalLabel();
  QString msg;
  ui_mainview.mainPanel->setCurrentIndex(pageMain);
  if (ui_mainview.editAmount->text().isEmpty()) ui_mainview.editAmount->setText("0.0");
  if (ui_mainview.checkCash->isChecked()) {
    if (ui_mainview.editAmount->text().toDouble()<totalSum) {
      canfinish = false;
      ui_mainview.editAmount->setFocus();
      ui_mainview.editAmount->setStyleSheet("background-color: rgb(255,100,0); color:white; selection-color: white; font-weight:bold;");
      ui_mainview.editCardNumber->setStyleSheet("");
      ui_mainview.editAmount->setSelection(0, ui_mainview.editAmount->text().length());
      msg = i18n("<html><font color=red><b>Please fill the correct pay amount before finishing a transaction.</b></font></html>");
      ui_mainview.labelPayMsg->setText(msg);
      QTimer::singleShot(3000, this, SLOT(clearLabelPayMsg()));
    }
  }
  else {
    if (!ui_mainview.editCardNumber->hasAcceptableInput()) {
      canfinish = false;
      ui_mainview.editCardNumber->setFocus();
      ui_mainview.editCardNumber->setStyleSheet("background-color: rgb(255,100,0); color:white; font-weight:bold; selection-color: white;");
      ui_mainview.editAmount->setStyleSheet("");
      ui_mainview.editCardNumber->setSelection(0, ui_mainview.editCardNumber->text().length());
      msg = i18n("<html><font color=red><b>Please enter the card number.</b></font></html>");
    }
    else if (!ui_mainview.editCardAuthNumber->hasAcceptableInput()) {
      canfinish = false;
      ui_mainview.editCardAuthNumber->setFocus();
      ui_mainview.editCardAuthNumber->setStyleSheet("background-color: rgb(255,100,0); color:white; font-weight:bold; selection-color: white;");
      ui_mainview.editAmount->setStyleSheet("");
      ui_mainview.editCardAuthNumber->setSelection(0, ui_mainview.editCardAuthNumber->text().length());
      msg = i18n("<html><font color=red><b>Please enter the Authorisation number from the bank voucher.</b></font></html>");
    }
    ui_mainview.labelPayMsg->setText(msg);
    QTimer::singleShot(3000, this, SLOT(clearLabelPayMsg()));
  }
  if (ui_mainview.tableWidget->rowCount() == 0) canfinish = false;
  if (!canStartSelling()) {
    canfinish=false;
    KNotification *notify = new KNotification("information", this);
    notify->setText(i18n("Before selling, you must start operations."));
    QPixmap pixmap = DesktopIcon("dialog-error",32); //NOTE: This does not works
    notify->setPixmap(pixmap);
    notify->sendEvent();
  }

  if (canfinish) // Ticket #52: Allow ZERO DUE.
  {
    ui_mainview.editAmount->setStyleSheet("");
    ui_mainview.editCardNumber->setStyleSheet("");
    TransactionInfo tInfo;
    PaymentType      pType;
    double           payWith = 0.0;
    double           payTotal = 0.0;
    double           changeGiven = 0.0;
    QString          authnumber = "'[Not Used]'";
    QString          cardNum = "'[Not Used]'";
    QString          paidStr = "'[Not Available]'";
    QString qry;
    QStringList ilist;
    payTotal = totalSum;
    if (ui_mainview.checkCash->isChecked()) {
      pType = pCash;
      if (!ui_mainview.editAmount->text().isEmpty()) payWith = ui_mainview.editAmount->text().toDouble();
      changeGiven = payWith- totalSum;
    } else {
      pType = pCard;
      if (ui_mainview.editCardNumber->hasAcceptableInput()) cardNum = ui_mainview.editCardNumber->text();
      if (ui_mainview.editCardAuthNumber->hasAcceptableInput()) authnumber = ui_mainview.editCardAuthNumber->text();
      cardNum = "'"+cardNum+"'";
      authnumber = "'"+authnumber+"'";
      payWith = payTotal;
    }

    tInfo.id = currentTransaction;
    tInfo.type = 0;//already on db.
    tInfo.amount = totalSum;

    //new feature from biel : Change sale date time
    bool printticket=true;
    if (!ui_mainview.groupSaleDate->isHidden()) { //not hidden, change date.
      QDateTime datetime = ui_mainview.editTransactionDate->dateTime();
      tInfo.date   =  datetime.date();
      tInfo.time   =  datetime.time();
      ticket.datetime = datetime;
      if (!Settings::printChangedDateTicket()) printticket = false;
    } else  { // hidden, keep current date as sale date.
      tInfo.date   = QDate::currentDate();
      tInfo.time   = QTime::currentTime();
      ticket.datetime = QDateTime::currentDateTime();
    }
    
    tInfo.paywith= payWith;
    tInfo.changegiven =changeGiven;
    tInfo.paymethod = pType;
    tInfo.state = tCompleted;
    tInfo.userid = loggedUserId;
    tInfo.clientid = clientInfo.id;
    tInfo.cardnumber = cardNum;
    tInfo.cardauthnum= authnumber;
    tInfo.itemcount= 0;//later
    tInfo.itemlist = ""; //at the for..
    tInfo.disc = clientInfo.discount;
    tInfo.discmoney = discMoney; //global variable...
    tInfo.points = buyPoints; //global variable...
    tInfo.profit = 0; //later
    tInfo.terminalnum=Settings::editTerminalNumber();

    QStringList productIDs; productIDs.clear();
    int cantidad=0;
    double utilidad=0;

    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);

    QHashIterator<qulonglong, ProductInfo> i(productsHash);
    int position=0;
    QList<TicketLineInfo> ticketLines;
    ticketLines.clear();
    TransactionItemInfo tItemInfo;
    
    // NOTE: utilidad (profit): Also take into account client discount! after this...
    while (i.hasNext()) {
      i.next();
      position++;
      productIDs.append(QString::number(i.key())+"/"+QString::number(i.value().qtyOnList));
      if (i.value().units == uPiece) cantidad += i.value().qtyOnList; else cantidad += 1; // :)
      utilidad += (i.value().price - i.value().cost - i.value().disc) * i.value().qtyOnList;
      //decrement stock qty, increment soldunits
      myDb->decrementProductStock(i.key(), i.value().qtyOnList, QDate::currentDate() );

      //qDebug()<<"Utilidad acumulada de la venta:"<<utilidad<<" |price:"<<i.value().price<<" cost:"<<i.value().cost<<" desc:"<<i.value().disc;

      //from Biel
      // save transactionItem
      tItemInfo.transactionid   = tInfo.id;
      tItemInfo.position        = position;
      tItemInfo.productCode     = i.key();
      tItemInfo.points          = i.value().points; // qtyOnList; //MCH: changed...
      tItemInfo.unitStr         = i.value().unitStr;
      tItemInfo.qty             = i.value().qtyOnList;
      tItemInfo.cost            = i.value().cost;
      tItemInfo.price           = i.value().price;
      tItemInfo.disc            = i.value().disc;
      tItemInfo.total           = (i.value().price - i.value().disc) * i.value().qtyOnList;
      tItemInfo.name            = i.value().desc;
      myDb->insertTransactionItem(tItemInfo);

      //re-select the transactionItems model
      historyTicketsModel->select();

      // add line to ticketLines 
      TicketLineInfo tLineInfo;
      tLineInfo.qty     = i.value().qtyOnList;
      tLineInfo.unitStr = i.value().unitStr;
      tLineInfo.desc    = i.value().desc;
      tLineInfo.price   = i.value().price;
      tLineInfo.disc    = i.value().disc;
      tLineInfo.total   = tItemInfo.total;
      ticketLines.append(tLineInfo);
    }
    tInfo.itemcount = cantidad;
    // taking into account the client discount. Applied over other products discount.
    // discMoney is the money discounted because of client discount.
    tInfo.profit = utilidad - discMoney; // utilidad = profit
    tInfo.itemlist  = productIDs.join(",");

    //update transactions
    myDb->updateTransaction(tInfo);
    //increment client points
    myDb->incrementClientPoints(tInfo.clientid, tInfo.points);

    if (drawerCreated) {
        //FIXME: What to di first?... add or substract?... when there is No money or there is less money than the needed for the change.. what to do?
        if (ui_mainview.checkCash->isChecked()) {
          drawer->addCash(payWith);
          drawer->substractCash(changeGiven);
          drawer->incCashTransactions();
          //open drawer only if there is a printer available.
          if (Settings::openDrawer()) drawer->open();
        } else {
          drawer->incCardTransactions();
          drawer->addCard(payWith);
        }
        drawer->insertTransactionId(getCurrentTransaction());
    }
    else {
      KNotification *notify = new KNotification("information", this);
      notify->setText(i18n("The Drawer is not initialized, please start operation first."));
      QPixmap pixmap = DesktopIcon("dialog-information",32); //NOTE: This does not works
      notify->setPixmap(pixmap);
      notify->sendEvent();
    }
    //update client info in the hash....
    clientInfo.points += buyPoints;
    clientsHash.remove(QString::number(clientInfo.id));
    clientsHash.insert(QString::number(clientInfo.id), clientInfo);
    updateClientInfo();

    //Ticket
    ticket.number = currentTransaction;
    ticket.total  = payTotal;
    ticket.change = changeGiven;
    ticket.paidwith = payWith;
    ticket.itemcount = cantidad;
    ticket.cardnum = cardNum;
    ticket.cardAuthNum = authnumber;
    ticket.paidWithCard = ui_mainview.checkCard->isChecked();
    ticket.clientDisc = clientInfo.discount;
    ticket.clientDiscMoney = discMoney;
    ticket.buyPoints = buyPoints;
    ticket.clientPoints = clientInfo.points;
    ticket.lines = ticketLines;

    if (printticket) printTicket(ticket);
    
    transactionInProgress = false;
    updateModelView();
    ui_mainview.editItemCode->setFocus();

    //Check level of cash in drawer
    if (drawer->getAvailableInCash() < Settings::cashMinLevel() && Settings::displayWarningOnLowCash()) {
      KNotification *notify = new KNotification("information", this);
      notify->setText(i18n("Cash level in drawer is low."));
      QPixmap pixmap = DesktopIcon("dialog-warning",32); //NOTE: This does not works
      notify->setPixmap(pixmap);
      notify->sendEvent();
    }
   }
   
   if (!ui_mainview.groupSaleDate->isHidden()) ui_mainview.groupSaleDate->hide(); //finally we hide the sale date group
}

void lemonView::printTicket(TicketInfo ticket)
{
  //TRanslateable strings:
  QString salesperson    = i18n("Salesperson: %1", loggedUserName);
  QString hQty           = i18n("Qty");
  QString hProduct       = i18n("Product");
  QString hPrice         = i18n("Price");
  QString hDisc          = i18n("Offer");
  QString hTotal         = i18n("Total");
  QString hClientDisc    = i18n("Your discount: %1", discMoney);
  QString hClientBuyPoints  = i18n("Your points this buy: %1", buyPoints);
  QString hClientPoints  = i18n("Your total points: %1", clientInfo.points);
  QString hTicket  = i18n("Ticket # %1", ticket.number);
  QString terminal = i18n("Terminal #%1", Settings::editTerminalNumber());
  //HTML Ticket
  QStringList ticketHtml;
  double tDisc = 0.0;
  //Ticket header
  ticketHtml.append(QString("<html><body><b>%1 - %2</b> [%3]<br>Ticket #%4 %5 %6<br>")
      .arg(Settings::editStoreName())
      .arg(Settings::storeAddress())
      .arg(Settings::storePhone())
      .arg(ticket.number)
      .arg(salesperson)
      .arg(terminal));
  //Ticket Table header
  ticketHtml.append(QString("<table border=0><tr><th>%1</th><th>%2</th><th>%3</th><th>%4</th><th>%5</th></tr>")
      .arg(hQty).arg(hProduct).arg(hPrice).arg(hDisc).arg(hTotal));

  //TEXT Ticket
  QStringList itemsForPrint;
  QString line;
  line = QString("%1, %2").arg(Settings::editStoreName()).arg(Settings::storeAddress()); //FIXME:Check Address lenght for ticket
  itemsForPrint.append(line);
  line = QString("%1").arg(terminal);
  itemsForPrint.append(line);
  line = QString("%1  %2").arg(hTicket).arg(salesperson);
  itemsForPrint.append(line);
  line = KGlobal::locale()->formatDateTime(ticket.datetime, KLocale::LongDate);
  itemsForPrint.append(line);
  itemsForPrint.append("          ");
  hQty.append("      ").truncate(6);
  hProduct.append("              ").truncate(14);
  hPrice.append("       ").truncate(7);
  hTotal.append("      ").truncate(6);
  //qDebug()<< "Strings:"<< hQty;qDebug()<< ", "<< hProduct<<", "<<hPrice<<", "<<hTotal;
  itemsForPrint.append(hQty +"  "+ hProduct +"  "+ hPrice+ "  "+ hTotal);
  itemsForPrint.append("------  --------------  -------  -------");

  QLocale localeForPrinting; // needed to convert double to a string better 
  for (int i = 0; i < ticket.lines.size(); ++i)
  {
    TicketLineInfo tLine = ticket.lines.at(i);
    QString  idesc =  tLine.desc;
    QString iprice =  localeForPrinting.toString(tLine.price,'f',2);
    QString iqty   =  localeForPrinting.toString(tLine.qty,'f',2);
    iqty = iqty+" "+tLine.unitStr;
    QString idiscount =  localeForPrinting.toString(tLine.qty*tLine.disc,'f',2);
    bool hasDiscount = false;
    if (tLine.disc > 0) {
      hasDiscount = true;
      tDisc = tDisc + tLine.qty*tLine.disc;
    }
    QString idue =  localeForPrinting.toString(tLine.total,'f',2);

    
    //HTML Ticket
    ticketHtml.append(QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
        .arg(iqty).arg(idesc).arg(iprice).arg(idiscount).arg(idue));
    //TEXT TICKET
    //adjusting length
    if (idesc.length()>14) idesc.truncate(14); //idesc = idesc.insert(19, '\n');
    else {
      while (idesc.length()<14) idesc = idesc.insert(idesc.length(), ' ');
    }
    iqty.append("      ").truncate(6);
    while (iprice.length()<7) iprice = QString(" ").append(iprice);
    while (idue.length()<7) idue = QString(" ").append(idue);
    
//     while (iqty.length()<7) iqty = iqty.insert(iqty.length(), ' ');
//     while (idiscount.length()<4) idiscount = idiscount.insert(idiscount.length(), ' ');

    line = QString("%1  %2  %3  %4").
        arg(iqty).
        arg(idesc).
        arg(iprice).
        arg(idue);
    itemsForPrint.append(line);
    if (hasDiscount) itemsForPrint.append(QString("        * %1 *     -%2").arg(hDisc).arg(idiscount) );
  }//for each item


  //HTML Ticket
  QString harticles = i18np("%1 article.", "%1 articles.", ticket.itemcount);
  QString htotal    = i18n("A total of");
  ticketHtml.append(QString("</table><br><br><b>%1</b> %2 <b>%3</b>")
      .arg(harticles).arg(htotal).arg(KGlobal::locale()->formatMoney(ticket.total, QString(), 2)));
  ticketHtml.append(i18n("<br>Paid with %1, your change is <b>%2</b><br>",
                          KGlobal::locale()->formatMoney(ticket.paidwith, QString(), 2),
                          KGlobal::locale()->formatMoney(ticket.change, QString(), 2)));
  ticketHtml.append(Settings::editTicketMessage());
  //Text Ticket
  itemsForPrint.append("  ");
  line = QString("%1  %2 %3").arg(harticles).arg(htotal).arg(KGlobal::locale()->formatMoney(ticket.total, QString(), 2));
  itemsForPrint.append(line);
  if (tDisc > 0) {
    line = i18n("You saved %1", KGlobal::locale()->formatMoney(tDisc, QString(), 2));
    itemsForPrint.append(line);
  }
  if (clientInfo.discount>0) itemsForPrint.append(hClientDisc);
  if (buyPoints>0) itemsForPrint.append(hClientBuyPoints);
  if (clientInfo.points>0) itemsForPrint.append(hClientPoints);
  itemsForPrint.append(" ");
  line = i18n("Paid with %1, your change is %2",
              KGlobal::locale()->formatMoney(ticket.paidwith, QString(), 2), KGlobal::locale()->formatMoney(ticket.change, QString(), 2));
  itemsForPrint.append(line);
  itemsForPrint.append(" ");
  if (ticket.paidWithCard) {
    ticketHtml.append(i18n("<br>Card # %1<br>Authorisation # %2",ticket.cardnum, ticket.cardAuthNum));
    line = i18n("Card Number:%1 \nAuthorisation #:%2",ticket.cardnum,ticket.cardAuthNum);
    itemsForPrint.append(line);
    itemsForPrint.append(" ");
  }
  line = QString(Settings::editTicketMessage());
  itemsForPrint.append(line);
  ticketHtml.append("</body></html>");

  //Printing...
  qDebug()<< itemsForPrint.join("\n");

  //FIXME:This is a test... fix it later.
  if (Settings::printTicket()) {
    if (Settings::smallTicket()) {
      qDebug()<<"Printing small ticket";
      QString printerFile=Settings::printerDevice();
      if (printerFile.length() == 0) printerFile="/dev/lp0";
      QFile file(printerFile);
      QString printerCodec=Settings::printerCodec();
      if (file.open(QIODevice::ReadWrite)) {
        qDebug()<<"Printing ticket...";
        QTextStream out(&file);
        //out.setCodec(Utf8Codec);
        if (printerCodec.length() != 0) out.setCodec(QTextCodec::codecForName(printerCodec.toLatin1()));
        else out.setCodec(QTextCodec::codecForName("UTF-8"));
        qDebug()<<"PRINTER CODEC:"<<printerCodec.toLatin1();
        out << "\x1b\x4b\x30";              // Feed back x30 dot lines
        out << "\x1b\x4b\x20";              // Feed back x20 dot lines
        out << itemsForPrint.join("\n");    // Print data

        out << "\x1b\x64\x06";              // Feed 6 lines
        file.close();
      } else qDebug()<<"ERROR: Could not open printer...";
    } //smalTicket
    else { // some code taken from Daniel O'Neill contribution.
      qDebug()<<"Printing big receipt";

      QFont header = QFont("Impact", 30);
      QFont sectionHeader = QFont("Bitstream Vera Sans", 14);
      const int Margin = 50;
      //int pageNo = 1;
      QPixmap logoPixmap;
      logoPixmap.load(Settings::storeLogo());


      QPrinter printer;
      printer.setFullPage( true );
      QPrintDialog printDialog( &printer, this );
      printDialog.setWindowTitle(i18n("Print Receipt"));
      if ( printDialog.exec() )
      {
        QPainter painter;
        painter.begin( &printer );


        int yPos        = 0;
        QFontMetrics fm = painter.fontMetrics();

        // Header: Store Name, Store Address, Store Phone, Store Logo...
        painter.setFont(header);
        painter.drawText(Margin,Margin, Settings::editStoreName() );
        yPos = yPos + fm.lineSpacing();
        // Store Address
        painter.setFont(QFont("Bitstream Vera Sans", 10));
        QPen normalPen = painter.pen();
        painter.setPen(Qt::darkGray);
        painter.drawText(Margin, Margin + yPos, printer.width(), fm.lineSpacing(), Qt::TextExpandTabs | Qt::TextDontClip, Settings::storeAddress() + ", " +i18n("Phone: ") + Settings::storePhone());
        yPos = yPos + fm.lineSpacing();
        // Store Logo
        painter.drawPixmap(printer.width() - logoPixmap.width() - Margin, Margin - logoPixmap.height()/2, logoPixmap);
        // Header line
        painter.setPen(QPen(Qt::gray, 5, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawLine(Margin, 100, printer.width()-Margin, 100);
        yPos = yPos + 3 * fm.lineSpacing(); // 3times the height of the line
        // Ticket Number, Date
        painter.setPen(normalPen);
        QString text = KGlobal::locale()->formatDateTime(ticket.datetime, KLocale::LongDate)+", "+hTicket;
        QSize textWidth = fm.size(Qt::TextExpandTabs | Qt::TextDontClip, text);
        painter.drawText(printer.width()-Margin-textWidth.width()-20, Margin + yPos, text); // I think -20 is because fm is not updated.
        yPos = yPos + fm.lineSpacing();
        // Vendor name, terminal number
        text = salesperson + ", " + terminal;
        textWidth = fm.size(Qt::TextExpandTabs | Qt::TextDontClip, text);
        painter.drawText(printer.width()-Margin-textWidth.width()-20, Margin + yPos, text);
        yPos = yPos + 3*fm.lineSpacing();
        // Products Subheader
        painter.setPen(Qt::darkBlue);
        QFont tmpFont = QFont("Bitstream Vera Sans", 10 );
        tmpFont.setWeight(QFont::Bold);
        painter.setFont(tmpFont);
        painter.drawText(Margin,Margin+yPos, hProduct);
        text = i18n("Quantity") +QChar::fromLatin1(9)+ hPrice+ QChar::fromLatin1(9)+ hDisc +QChar::fromLatin1(9)+ hTotal;
        painter.drawText(printer.width()/2, Margin + yPos, text);
        yPos = yPos + fm.lineSpacing();
        painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawLine(Margin, Margin + yPos - 8, printer.width()-Margin, Margin + yPos - 8);
        painter.setPen(normalPen);
        painter.setFont(QFont("Bitstream Vera Sans", 10 ));
        yPos = yPos + fm.lineSpacing();
        // End of Header Information.

        // Content : Each product
        QLocale localeForPrinting;
        for (int i = 0; i < ticket.lines.size(); ++i)
        {
          TicketLineInfo tLine = ticket.lines.at(i);
          QString  idesc =  tLine.desc;
          QString iprice =  localeForPrinting.toString(tLine.price,'f',2);
          QString iqty   =  localeForPrinting.toString(tLine.qty, 'f', 2);
          iqty = iqty+" "+tLine.unitStr;
          QString idiscount =  localeForPrinting.toString(-(tLine.qty*tLine.disc),'f',2);
          QString idue =  localeForPrinting.toString(tLine.total,'f',2);
          while (fm.size(Qt::TextExpandTabs | Qt::TextDontClip, idesc).width() >= ((printer.width()/2)-Margin-40)) { idesc.chop(2); qDebug()<<"Chopped:"<<idesc;}
          painter.drawText(Margin, Margin+yPos, idesc); //first product description...
          text = iqty+QChar::fromLatin1(9)+ iprice+QChar::fromLatin1(9)+ idiscount +QChar::fromLatin1(9)+ idue;
          painter.drawText(printer.width()/2, Margin+yPos, text);
          yPos = yPos + fm.lineSpacing();
          //Check if space for the next text line
          if ( Margin + yPos > printer.height() - Margin ) {
            printer.newPage();             // no more room on this page
            yPos = 0;                       // back to top of page
          }
        } //for each item

        //now the totals...
        //Check if space for the next text 3 lines
        if ( (Margin + yPos +fm.lineSpacing()*3) > printer.height() - Margin ) {
          printer.newPage();             // no more room on this page
          yPos = 0;                       // back to top of page
        }
        painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawLine(Margin, Margin + yPos - 8, printer.width()-Margin, Margin + yPos - 8);
        painter.setPen(normalPen);
        painter.setFont(tmpFont);
        yPos = yPos + fm.lineSpacing();
        painter.drawText(Margin, Margin+yPos, i18n("Totals"));
        text = harticles + QChar::fromLatin1(9)+ localeForPrinting.toString(-tDisc, 'f', 2) +QChar::fromLatin1(9)+ KGlobal::locale()->formatMoney(ticket.total, QString(), 2);
        painter.drawText((printer.width()/2)-15, Margin + yPos , text);
        yPos = yPos + fm.lineSpacing();
        // NOTE: I think its redundant to say again the savings.
        //if (tDisc > 0) {
        //    painter.drawText(Margin, Margin + yPos , line = i18n("You saved %1", KGlobal::locale()->formatMoney(tDisc, QString(), 2)););
        //    yPos = yPos + fm.lineSpacing();
        //}

        //if space, the ticket message.
        if ( (Margin + yPos +fm.lineSpacing()*4) <= printer.height() - Margin ) {
            tmpFont = QFont("Bitstream Vera Sans", 12);
            tmpFont.setItalic(true);
            painter.setPen(Qt::darkGreen);
            painter.setFont(tmpFont);
            yPos = yPos + fm.lineSpacing()*4;
            painter.drawText((printer.width()/2)-(fm.size(Qt::TextExpandTabs | Qt::TextDontClip, Settings::editTicketMessage()).width()/2)-Margin, Margin+yPos, Settings::editTicketMessage());
        }

        painter.end();
        // this makes the print job start
    } //printDialog.exec()
  }//bigTicket
  } //printTicket

  //Using SP500 for now...
//   StarPrinter *printer = new StarPrinter();
//   printer->openPrinterPort();
//   printer->setCharacterSet_toLatinamerican();
//   if (printer->isPortOpen()) {
//     qDebug()<<"Printer port opened, printing ticket...";
//     printer->writeToPort(itemsForPrint.join("\n"));
//     printer->closePrinterPort();
//   } else { qDebug()<<"Could not open port, lastError:"<<printer->lastError(); }
//   delete printer;

  if (Settings::showDialogOnPrinting())
  {
    TicketPopup *popup = new TicketPopup(this, ticketHtml.join(" "), DesktopIcon("lemon-printer", 48), Settings::ticketTime()*1000);
    QApplication::beep();
    popup->popup();
  }
  //Start Again a new transaction and clear all used widgets..
  QTimer::singleShot(1000, this, SLOT(startAgain()));

}

void lemonView::startAgain()
{
  qDebug()<<"startAgain(): New Transaction";
  productsHash.clear();
  setupClients(); //clear the clientInfo (sets the default client info)
  clearUsedWidgets();
  buyPoints =0;
  discMoney=0;
  refreshTotalLabel();
  createNewTransaction(tSell);
}

void lemonView::cancelCurrentTransaction()
{
  cancelTransaction(getCurrentTransaction());
}


void lemonView::preCancelCurrentTransaction()
{
  if (ui_mainview.tableWidget->rowCount()==0 ) { //empty transaction
    productsHash.clear();
    setupClients(); //clear the clientInfo (sets the default client info)
    clearUsedWidgets();
    buyPoints =0;
    discMoney=0;
    refreshTotalLabel();
    ///Next two lines were deleted to do not increment transactions number. reuse it.
    //if (Settings::deleteEmptyCancelledTransactions()) deleteCurrentTransaction();
    //else cancelCurrentTransaction();
  }
  else {
    cancelCurrentTransaction();
  }

  //if change sale date is in progress (but cancelled), hide it.
  if (!ui_mainview.groupSaleDate->isHidden()) {
    ui_mainview.groupSaleDate->hide();
  }
}

void lemonView::deleteCurrentTransaction()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  if (myDb->deleteTransaction(getCurrentTransaction())) {
    transactionInProgress=false;
    createNewTransaction(tSell);
  }
  else {
    KMessageBox::detailedError(this, i18n("Lemon has encountered an error when querying the database, click details to see the error details."), myDb->lastError(), i18n("Delete Transaction: Error"));
  }
}

//NOTE: This substracts points given to the client, restore stockqty, register a cashout for the money return. All if it applies.
void lemonView::cancelTransaction(qulonglong transactionNumber)
{
  bool enoughCashAvailable=false;
  bool transToCancelIsInProgress=false;
  bool transCompleted=false;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  //get amount to return
  TransactionInfo tinfo = myDb->getTransactionInfo(transactionNumber);
  if (tinfo.state == tCancelled ) transCompleted = true;
  
  if (getCurrentTransaction() == transactionNumber) {
    ///this transaction is not saved yet (more than the initial data when transaction is created)
    transToCancelIsInProgress = true;
    clearUsedWidgets();
    refreshTotalLabel();
  } else {
    ///this transaction is saved (amount,products,points...)
    clearUsedWidgets();
    refreshTotalLabel();
    if (drawer->getAvailableInCash() > tinfo.amount){ // == or >= or > ?? to dont let empty the drawer
      enoughCashAvailable = true;
    }
  }
  
  //Mark as cancelled if possible
  if  (enoughCashAvailable || transToCancelIsInProgress) {
    if (myDb->cancelTransaction(transactionNumber, transToCancelIsInProgress)) {
      qDebug()<<"Cancelling ticket was ok";
      if (transCompleted) {
        //if was completed, then return the money...
        drawer->substractCash(tinfo.amount);
        if (Settings::openDrawer()) drawer->open();
      }
      transactionInProgress = false; //reset
      createNewTransaction(tSell);
    }
    else { //myDB->cancelTransaction() returned some error...
      qDebug()<<"Not cancelled!";
      if (!transToCancelIsInProgress) {
        KMessageBox::error(this, myDb->lastError(), i18n("Cancel Transaction: Error"));
      } else {
        //Reuse the transaction instead of creating a new one.
        qDebug()<<"Transaction to cancel is in progress. Clearing all to reuse transaction number...";
        productsHash.clear();
        setupClients(); //clear the clientInfo (sets the default client info)
        clearUsedWidgets();
        buyPoints =0;
        discMoney=0;
        refreshTotalLabel();
      }
    }
  } else {
    //not cash available in drawer to return money to the client
    KNotification *notify = new KNotification("information", this);
    notify->setText(i18n("There is not enough cash available in the drawer."));
    QPixmap pixmap = DesktopIcon("dialog-error",32); //NOTE: This does not works with plasma themed notification
    notify->setPixmap(pixmap);
    notify->sendEvent();
  }
}




void lemonView::startOperation()
{
  qDebug()<<"Starting operations...";
  bool ok=false;
  double qty=0.0;
  InputDialog *dlg = new InputDialog(this, false, dialogMoney, i18n("Enter the amount of money to deposit in the drawer"));
  dlg->setEnabled(true);
  if (dlg->exec() ) {
    qty = dlg->dValue;
    if (qty >= 0) ok = true; //allow no deposit...
  }
  
  if (ok) {
    if (!drawerCreated) {
      drawer = new Gaveta();
      drawer->setPrinterDevice(Settings::printerDevice());
      drawerCreated = true;
    }
    if (Settings::openDrawer()) drawer->open();
   // Set drawer amount.
    drawer->setStartDateTime(QDateTime::currentDateTime());
    drawer->setAvailableInCash(qty);
    drawer->setInitialAmount(qty);
    operationStarted = true;
    createNewTransaction(tSell);
    emit signalStartedOperation();
  }
  else {
    operationStarted = false;
    //emit signalOperationNOTStarted();
    emit signalNoLoggedUser();
  }
}

void lemonView::slotDoStartOperation()
{

  //NOTE: For security reasons, we must ask for admin's password.
  //But, can we be more flexible -for one person store- and do not ask for password?
  // Settings::lowSecurityMode()
  
  qDebug()<<"doStartOperations..";
  if (!operationStarted) {
    bool doit = false;
    if (Settings::lowSecurityMode()) {
      doit = true;
    } else {
        do  {
          dlgPassword->show();
          dlgPassword->clearLines();
          dlgPassword->hide();
          doit = dlgPassword->exec();
        } while (!doit);
    }//else lowsecurity
    if (doit) startOperation();
  }
}

/* REPORTS ZONE */
/*--------------*/

void lemonView::corteDeCaja()
{
  qDebug()<<"Doing Balance..";
  preCancelCurrentTransaction();
  QStringList lines;
  QStringList linesHTML;
  QString line;

  QString dId;
  QString dAmount;
  QString dHour;
  QString dMinute;
  QString dPaidWith;
  QString dPayMethod;

  saveBalance();

  // Create lines to print and/or show on dialog...

  //----------Translated strings--------------------
  QString strTitle      = i18n("Balance for user %1", loggedUserName);
  QString strInitAmount = i18n("Initial Amount deposited:");
  QString strInitAmountH= i18n("Deposit");
  QString strInH         = i18n("In");
  QString strOutH        = i18n("Out");
  QString strInDrawerH   = i18n("In Drawer");
  QString strTitlePre    = i18n("Drawer Balance");
  QString strTitleTrans  = i18n("Transactions Details");
  QString strTitleTransH = i18n("Transactions");
  QString strId          = i18n("Id");
  QString strTimeH       = i18n("Time");
  QString strAmount      = i18n("Amount");
  QString strPaidWith    = i18n("Paid with");
  QString strPayMethodH =  i18n("Pay Method");

 //TODO: Hacer el dialogo de balance mejor, con un look uniforme a los demas dialogos.
//       Incluso insertar imagenes en el html del dialogo.

  //HTML
  line = QString("<html><body><h3>%1</h3>").arg(strTitle);
  linesHTML.append(line);
  line = QString("<center><table border=1 cellpadding=5><tr><th colspan=4>%9</th></tr><tr><th>%1</th><th>%2</th><th>%3</th><th>%4</th></tr><tr><td>%5</td><td>%6</td><td>%7</td><td>%8</td></tr></table></ceter><br>")
      .arg(strInitAmountH)
      .arg(strInH)
      .arg(strOutH)
      .arg(strInDrawerH)
      .arg(KGlobal::locale()->formatMoney(drawer->getInitialAmount(), QString(), 2))
      .arg(KGlobal::locale()->formatMoney(drawer->getInAmount(), QString(), 2))
      .arg(KGlobal::locale()->formatMoney(drawer->getOutAmount(), QString(), 2))
      .arg(KGlobal::locale()->formatMoney(drawer->getAvailableInCash(), QString(), 2))
      .arg(strTitlePre);
  linesHTML.append(line);
  line = QString("<table border=1 cellpadding=5><tr><th colspan=5>%1</th></tr><tr><th>%2</th><th>%3</th><th>%4</th><th>%5</th><th>%6</th></tr>")
      .arg(strTitleTransH)
      .arg(strId)
      .arg(strTimeH)
      .arg(strAmount)
      .arg(strPaidWith)
      .arg(strPayMethodH);
  linesHTML.append(line);

  //TXT
  lines.append(strTitle);
  line = QString(KGlobal::locale()->formatDateTime(QDateTime::currentDateTime(), KLocale::LongDate));
  lines.append(line);
  lines.append("----------------------------------------");
  line = QString("%1 %2").arg(strInitAmount).arg(KGlobal::locale()->formatMoney(drawer->getInitialAmount(), QString(), 2));
  lines.append(line);
  line = QString("%1 :%2, %3 :%4")
      .arg(strInH)
      .arg(KGlobal::locale()->formatMoney(drawer->getInAmount(), QString(), 2))
      .arg(strOutH)
      .arg(KGlobal::locale()->formatMoney(drawer->getOutAmount(), QString(), 2));
  lines.append(line);
  line = QString(" %1 %2").arg(KGlobal::locale()->formatMoney(drawer->getAvailableInCash(), QString(), 2)).arg(strInDrawerH);
  lines.append(line);
  //Now, add a transactions report per user and for today.
  //At this point, drawer must be initialized and valid.
  line = QString("----------%1----------").arg(strTitleTrans);
  lines.append(line);
  line = QString("%1           %2      %3").arg(strId).arg(strAmount).arg(strPaidWith);
  lines.append(line);
  lines.append("----------  ----------  ----------");
  QList<qulonglong> transactionsByUser = drawer->getTransactionIds();
  //This gets all transactions ids done since last corteDeCaja.
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  for (int i = 0; i < transactionsByUser.size(); ++i) {
    qulonglong idNum = transactionsByUser.at(i);
    TransactionInfo info;
    info = myDb->getTransactionInfo(idNum);

    dId       = QString::number(info.id);
    dAmount   = QString::number(info.amount);
    dHour     = info.time.toString("hh");
    dMinute   = info.time.toString("mm");
    dPaidWith = QString::number(info.paywith);

    while (dId.length()<10) dId = dId.insert(dId.length(), ' ');
    while (dAmount.length()<14) dAmount = dAmount.insert(dAmount.length(), ' ');
    while ((dHour+dMinute).length()<6) dMinute = dMinute.insert(dMinute.length(), ' ');
    while (dPaidWith.length()<10) dPaidWith = dPaidWith.insert(dPaidWith.length(), ' ');

    //if (info.paymethod == pCash) dPayMethod = i18n("Cash");/*dPaidWith;*/
    myDb->getPayTypeStr(info.paymethod);//using payType methods
    //else if (info.paymethod == pCard) dPayMethod = i18n("Card");  else dPayMethod = i18n("Unknown");
    line = QString("%1 %2 %3")
       .arg(dId)
       //.arg(dHour)
       //.arg(dMinute)
       .arg(dAmount)
       //.arg(dPaidWith);
       .arg(dPayMethod);
    lines.append(line);
    line = QString("<tr><td>%1</td><td>%2:%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
       .arg(dId)
       .arg(dHour)
       .arg(dMinute)
       .arg(dAmount)
       .arg(dPaidWith)
       .arg(dPayMethod);
    linesHTML.append(line);
  }
  line = QString("</table></body></html>");
  linesHTML.append(line);
  operationStarted = false;
  showBalance(linesHTML);
  printBalance(lines);
  slotDoStartOperation();
}


void lemonView::endOfDay() {
  // Get every transaction from all day, calculate sales, profit, and profit margin (%). From the same terminal

  AmountAndProfitInfo amountProfit;
  QList<TransactionInfo> transactionsList;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);

  amountProfit     = myDb->getDaySalesAndProfit(Settings::editTerminalNumber());
  transactionsList = myDb->getDayTransactions(Settings::editTerminalNumber());

  QFont header = QFont("Impact", 30);
  const int Margin = 50;
  QPixmap logoPixmap;
  logoPixmap.load(Settings::storeLogo());
  QPrinter printer;
  printer.setFullPage( true );
  QPrintDialog printDialog( &printer, this );
  printDialog.setWindowTitle(i18n("Print end of day report"));
  if ( printDialog.exec() )
    {
      QPainter painter;
      painter.begin( &printer );


      int yPos        = 0;
      QFontMetrics fm = painter.fontMetrics();

      // Header: REPORT, Store Name, Store Logo...
      painter.setFont(header);
      painter.drawText(Margin,Margin, i18n("End of day Report") );
      yPos = yPos + fm.lineSpacing();
      // Store Name
      painter.setFont(QFont("Bitstream Vera Sans", 10));
      QPen normalPen = painter.pen();
      painter.setPen(Qt::darkGray);
      painter.drawText(Margin, Margin + yPos, printer.width(), fm.lineSpacing(), Qt::TextExpandTabs | Qt::TextDontClip, Settings::editStoreName());
      yPos = yPos + fm.lineSpacing();
      // Store Logo
      painter.drawPixmap(printer.width() - logoPixmap.width() - Margin, Margin - logoPixmap.height()/2, logoPixmap);
      // Header line
      painter.setPen(QPen(Qt::gray, 5, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
      painter.drawLine(Margin, 100, printer.width()-Margin, 100);
      yPos = yPos + 3 * fm.lineSpacing(); // 3times the height of the line
      // Date
      painter.setPen(normalPen);
      QString text = KGlobal::locale()->formatDate(QDate::currentDate(), KLocale::LongDate);
      QSize textWidth = fm.size(Qt::TextExpandTabs | Qt::TextDontClip, text);
      painter.drawText(printer.width()-Margin-textWidth.width()-20, Margin + yPos, text); // I think -20 is because fm is not updated.
      yPos = yPos + fm.lineSpacing();
      // terminal number
      text = i18n("Terminal # ") + QString::number(Settings::editTerminalNumber());
      textWidth = fm.size(Qt::TextExpandTabs | Qt::TextDontClip, text);
      painter.drawText(printer.width()-Margin-textWidth.width()-20, Margin + yPos, text);
      yPos = yPos + 3*fm.lineSpacing();
      // Transactions Subheader:  trans_id - time - amount - profit - paidwith - paymethod
      QString headerTrans = i18n("Time") +QChar::fromLatin1(9)+ i18n("Amount") + QChar::fromLatin1(9)+ QChar::fromLatin1(9)+ i18n("Profit")+ QChar::fromLatin1(9)+ QChar::fromLatin1(9)+ i18n("Pay Method");
      painter.setPen(Qt::darkBlue);
      QFont tmpFont = QFont("Bitstream Vera Sans", 10 );
      tmpFont.setWeight(QFont::Bold);
      painter.setFont(tmpFont);
      painter.drawText(Margin,Margin+yPos, i18n("Transaction Id"));
      painter.drawText(printer.width()/4,Margin+yPos, headerTrans);
      yPos = yPos + fm.lineSpacing();
      painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
      painter.drawLine(Margin, Margin + yPos - 8, printer.width()-Margin, Margin + yPos - 8);
      painter.setPen(normalPen);
      painter.setFont(QFont("Bitstream Vera Sans", 10 ));
      yPos = yPos + fm.lineSpacing();
      // End of Header Information.

      // Content : Each Transaction
      QLocale localeForPrinting;
      qDebug()<<"Transactions size:"<<transactionsList.size()<<" Count"<<transactionsList.count();
      for (int i = 0; i < transactionsList.size(); ++i)
      {
        TransactionInfo info = transactionsList.at(i);
        QString amount   =  localeForPrinting.toString(info.amount,'f',2);
        QString profit   =  localeForPrinting.toString(info.profit, 'f', 2);
        QString payMethod;
        //if (info.paymethod == pCash) payMethod= "Cash"; else payMethod = "Card";
        payMethod = myDb->getPayTypeStr(info.paymethod);//using payType methods
        QString line = info.time.toString("hh:mm")+ QChar::fromLatin1(9)+ amount+ QChar::fromLatin1(9)+ QChar::fromLatin1(9)+ profit + QChar::fromLatin1(9)+ QChar::fromLatin1(9)+ payMethod;
        painter.drawText(Margin, Margin+yPos, QString::number(info.id));
        painter.drawText(printer.width()/4, Margin+yPos, line);
        yPos = yPos + fm.lineSpacing();
        //Check if space for the next text line
        if ( Margin + yPos > printer.height() - Margin ) {
          printer.newPage();             // no more room on this page
          yPos = 0;                       // back to top of page
        }
      } //for each item

      //now the totals...
      tmpFont = QFont("Bitstream Vera Sans", 14 );
      tmpFont.setWeight(QFont::Bold);
      painter.setFont(tmpFont);
      //Check if space for the next text 6 lines --CHECK if page is about to end but space for 2 lines of info without spaces.
      bool roomFor6Lines = false;
      bool roomFor3Lines = false;
      if ( (Margin + yPos +fm.lineSpacing()*6) > printer.height() - Margin ) {
        //No room for 6 lines
        if ( (Margin + yPos +fm.lineSpacing()*3) > printer.height() - Margin ) {
          printer.newPage();             // no more room on this page for 3 lines
          yPos = 0;                       // back to top of page
        } else roomFor3Lines = true;
      } else roomFor6Lines = true;


      painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
      painter.drawLine(Margin, Margin + yPos - 8, printer.width()-Margin, Margin + yPos - 8);

      if (roomFor6Lines) yPos = yPos + fm.lineSpacing()*3; else yPos = yPos + fm.lineSpacing();

      painter.setPen(Qt::blue);
      text = i18n("Total Sales for") +  QChar::fromLatin1(9)+ QChar::fromLatin1(9)+  QChar::fromLatin1(9)+ KGlobal::locale()->formatMoney(amountProfit.amount, QString(), 2);
      painter.drawText(Margin, Margin+yPos, text);
      yPos = yPos + fm.lineSpacing()*2;
      text = i18n("Total Profit for") +  QChar::fromLatin1(9)+  QChar::fromLatin1(9)+ QChar::fromLatin1(9)+ KGlobal::locale()->formatMoney(amountProfit.profit, QString(), 2);
      painter.drawText(Margin, Margin+yPos, text);


      painter.end();
      // this makes the print job start
  } //printDialog.exec()
}


void lemonView::saveBalance()
{
  
  BalanceInfo info;
  info.id = 0;
  info.dateTimeStart = drawer->getStartDateTime();
  info.dateTimeEnd   = QDateTime::currentDateTime();
  info.userid     = loggedUserId;
  info.username   =  loggedUserName;
  info.initamount = drawer->getInitialAmount();
  info.in         = drawer->getInAmount();
  info.out        = drawer->getOutAmount();
  info.cash       = drawer->getAvailableInCash();
  info.card       = drawer->getAvailableInCard();
  info.terminal   = Settings::editTerminalNumber();
  
  QStringList transactionList;
  QList<qulonglong> intList = drawer->getTransactionIds();
  if (intList.isEmpty()) transactionList.append("EMPTY");
  else {
    for (int i = 0; i < intList.size(); ++i) {
      transactionList.append( QString::number(intList.at(i)) );
    }
  }
  info.transactions=transactionList.join(",");
  
  //Save balance on Database
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  myDb->insertBalance(info);
}

void lemonView::showBalance(QStringList lines)
{
  if (Settings::showDialogOnPrinting())
  {
    BalanceDialog *popup = new BalanceDialog(this, lines.join("\n"));
    popup->show();
    popup->hide();
    int result = popup->exec();
    if (result) {
      //qDebug()<<"exec=true";
    }
  }
}

void lemonView::printBalance(QStringList lines)
{
  if (Settings::printBalances() && Settings::smallTicket()) {
    QString printerFile=Settings::printerDevice();
    if (printerFile.length() == 0) printerFile="/dev/lp0";
    QString printerCodec=Settings::printerCodec();
    QFile file(printerFile);
    if (file.open(QIODevice::ReadWrite)) {
      qDebug()<<"Printing balance...";
      QTextStream out(&file);
      if (printerCodec.length() != 0) out.setCodec(QTextCodec::codecForName(printerCodec.toLatin1()));
      else out.setCodec(QTextCodec::codecForName("UTF-8"));
      out << "\x1b\x4b\x30";              // Feed back x30 dot lines
      out << "\x1b\x4b\x20";              // Feed back x20 dot lines
      out << lines.join("\n");    // Print data
      out << "\x1b\x64\x06";              // Feed 6 lines
      file.close();
    } else qDebug()<<"ERROR: Could not open printer...";
  }
}


/* MODEL Zone */

void lemonView::setupModel()
{
  if (!db.isOpen()) {
    connectToDb();
  }
  else {
    //workaround for a stupid crash: when re-connecting after Config, on setTable crashes.
    //Crashes without debug information.
    if (productsModel->tableName() != "products")
      productsModel->setTable("products");
    productsModel->setEditStrategy(QSqlTableModel::OnRowChange);
    ui_mainview.listView->setModel(productsModel);
    ui_mainview.listView->setResizeMode(QListView::Adjust);

    ui_mainview.listView->setModelColumn(productsModel->fieldIndex("photo"));
    ui_mainview.listView->setViewMode(QListView::IconMode);
    ui_mainview.listView->setGridSize(QSize(170,170));
    ui_mainview.listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.listView->setMouseTracking(true); //for the tooltip

    ProductDelegate *delegate = new ProductDelegate(ui_mainview.listView);
    ui_mainview.listView->setItemDelegate(delegate);

    productsModel->select();

    //BFB. Added QCompleter to editFilterByDesc
    productsFilterModel->setQuery("");
    QCompleter *completer = new QCompleter(this);
    completer->setModel(productsFilterModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive); 
    //Show all possible results, because completer only works with prefix. The filter is done modifying the model
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    ui_mainview.editFilterByDesc->setCompleter(completer);

    //Categories popuplist
    populateCategoriesHash();
    QHashIterator<QString, int> item(categoriesHash);
    while (item.hasNext()) {
      item.next();
      ui_mainview.comboFilterByCategory->addItem(item.key());
      //qDebug()<<"iterando por el hash en el item:"<<item.key()<<"/"<<item.value();
    }

    ui_mainview.comboFilterByCategory->setCurrentIndex(0);
    connect(ui_mainview.comboFilterByCategory,SIGNAL(currentIndexChanged(int)), this, SLOT( setFilter()) );
    connect(ui_mainview.editFilterByDesc,SIGNAL(returnPressed()), this, SLOT( setFilter()) );
    connect(ui_mainview.editFilterByDesc,SIGNAL(textEdited(const QString)), this, SLOT( modifyProductsFilterModel()) );
    connect(ui_mainview.rbFilterByDesc, SIGNAL(toggled(bool)), this, SLOT( setFilter()) );
    connect(ui_mainview.rbFilterByCategory, SIGNAL(toggled(bool)), this, SLOT( setFilter()) );

    ui_mainview.rbFilterByCategory->setChecked(true);
    setFilter();
  }
 }

void lemonView::populateCategoriesHash()
{
  Azahar * myDb = new Azahar;
  myDb->setDatabase(db);
  categoriesHash = myDb->getCategoriesHash();
}

void lemonView::listViewOnMouseMove(const QModelIndex & index)
{
  //NOTE: Problem: here the data on the view does not change. This is because we do not
  //      update this view's data, we modify directly the data at database until we sell a product.
  //      and until that moment we can update this view.
  // UPDATE: We have at productsHash the property qtyOnList, we can use such qty to display available qty.
  //      But if we are working on a network (multiple POS). It will not be true the information.
  QString tprice = i18n("Price: ");
  QString tstock = i18n("Available: ");
  QString tdisc  = i18n("Discount:"); //TODO: Only include if valid until now...
  QString tcategory = i18n("Category:");
  QString tmoreAv = i18n("in stock");
  QString tmoreAv2= i18n("in your shopping cart, Total Available");

  //getting data from model...
  const QAbstractItemModel *model = index.model();
  int row = index.row();
  QModelIndex indx = model->index(row, 1);
  QString desc = model->data(indx, Qt::DisplayRole).toString();
  indx = model->index(row, 2);
  double price = model->data(indx, Qt::DisplayRole).toDouble();
  indx = model->index(row, 3);
  double stockqty = model->data(indx, Qt::DisplayRole).toDouble();
  indx = model->index(row, 0);
  QString code = model->data(indx, Qt::DisplayRole).toString();
  ProductInfo pInfo;
  bool onList=false;
  if (productsHash.contains(code.toULongLong())) {
    pInfo = productsHash.value(code.toULongLong());
    onList = true;
  }

  QString line1 = QString("<p><b><i>%1</i></b><br>").arg(desc);
  QString line2 = QString("<b>%1</b>%2<br>").arg(tprice).arg(KGlobal::locale()->formatMoney(price));
  QString line3;
  if (onList) line3 = QString("<b>%1</b> %2 %5 %6, %3 %7: %4<br></p>").arg(tstock).arg(stockqty).arg(pInfo.qtyOnList).arg(stockqty - pInfo.qtyOnList).arg(pInfo.unitStr).arg(tmoreAv).arg(tmoreAv2);
  else line3 = QString("<b>%1</b> %2 %3 %4<br></p>").arg(tstock).arg(stockqty).arg(pInfo.unitStr).arg(tmoreAv);
  QString text = line1+line2+line3;

  ui_mainview.listView->setToolTip(text);
}

void lemonView::listViewOnClick(const QModelIndex & index)
{
  //getting data from model...
  const QAbstractItemModel *model = index.model();
  int row = index.row();
  QModelIndex indx = model->index(row, 0);
  QString code = model->data(indx, Qt::DisplayRole).toString();
  insertItem(code);
}

//This is done at the end of each transaction...
void lemonView::updateModelView()
{
  //Submit and select causes a flick and costs some milliseconds
  productsModel->submitAll();
  productsModel->select();
}

void lemonView::showProductsGrid(bool show)
{
  if (show) {
    ui_mainview.frameGridView->show();
  }
  else {
    ui_mainview.frameGridView->hide();
  }
}

void lemonView::hideProductsGrid()
{
  ui_mainview.frameGridView->hide();
}

void lemonView::showPriceChecker()
{
  PriceChecker *priceDlg = new PriceChecker(this);
  priceDlg->setDb(db);
  priceDlg->show();
}

void lemonView::setFilter()
{
  //NOTE: This is a QT BUG.
  //   If filter by description is selected and the text is empty, and later is re-filtered
  //   then NO pictures are shown; even if is refiltered again.
  QRegExp regexp = QRegExp(ui_mainview.editFilterByDesc->text());
  
  if (ui_mainview.rbFilterByDesc->isChecked()) {//by description
    if (!regexp.isValid() || ui_mainview.editFilterByDesc->text().isEmpty())  ui_mainview.editFilterByDesc->setText("*");
    if (ui_mainview.editFilterByDesc->text()=="*") productsModel->setFilter("");
    else  productsModel->setFilter(QString("products.name REGEXP '%1'").arg(ui_mainview.editFilterByDesc->text().split("(").at(0).trimmed()));
    // BFB: If the user choose a product from the completer, this product is added to the list.
    modifyProductsFilterModel();
    if (productsFilterModel->rowCount() == 1){
      if (ui_mainview.editFilterByDesc->text() == productsFilterModel->data(productsFilterModel->index(0,0)).toString()){
              qulonglong idProduct = productsFilterModel->data(productsFilterModel->index(0, 1)).toULongLong();
        insertItem(QString::number(idProduct));
        ui_mainview.editFilterByDesc->selectAll();
      }
    }
  }
  else {
    if (ui_mainview.rbFilterByCategory->isChecked()) {//by category
      //Find catId for the text on the combobox.
      int catId=-1;
      QString catText = ui_mainview.comboFilterByCategory->currentText();
      if (categoriesHash.contains(catText)) {
        catId = categoriesHash.value(catText);
      }
      productsModel->setFilter(QString("products.category=%1").arg(catId));
    }else{//by most sold products in current month --biel
      productsModel->setFilter("products.code IN (SELECT * FROM (SELECT product_id FROM (SELECT product_id, sum( units ) AS sold_items FROM transactions t, transactionitems ti WHERE t.id = ti.transaction_id AND t.date > ADDDATE( sysdate( ) , INTERVAL -31 DAY ) GROUP BY ti.product_id) month_sold_items ORDER BY sold_items DESC LIMIT 5) popular_products)");
    }
  }
  productsModel->select();
}

void lemonView::modifyProductsFilterModel()
{    
    if (ui_mainview.editFilterByDesc->text().length() > 2){
        QString sql;
        QString productName=ui_mainview.editFilterByDesc->text().split("(").at(0).trimmed();
        if (KGlobal::locale()->positivePrefixCurrencySymbol())
          sql = QString("select concat(name,  ' (%1 ' ,price , ')' ) as nameprice, code, name from products where products.name REGEXP '%2'").arg(KGlobal::locale()->currencySymbol()).arg(productName);
        else
          sql = QString("select concat(name,  ' (' ,price , ' %1)' ) as nameprice, code, name from products where products.name REGEXP '%2'").arg(KGlobal::locale()->currencySymbol()).arg(productName);
        productsFilterModel->setQuery(sql);
    }else{
        productsFilterModel->setQuery("");
    }

}

void lemonView::setupDB()
{
  qDebug()<<"Setting up database...";
  if (db.isOpen()) db.close();
  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());
  connectToDb();
}

void lemonView::connectToDb()
{
  if (!db.open()) {
    db.open(); //try to open connection
    qDebug()<<"(1/connectToDb) Trying to open connection to database..";
  }
  if (!db.isOpen()) {
    db.open(); //try to open connection again...
    qDebug()<<"(2/connectToDb) Trying to open connection to database..";
  }
  if (!db.isOpen()) {
    qDebug()<<"(3/connectToDb) Configuring..";
    emit signalShowDbConfig();
  } else {
    //finally, when connection stablished, setup all models.
    if (!modelsCreated) { //Create models...
      productsModel       = new QSqlTableModel();
      historyTicketsModel = new QSqlRelationalTableModel();
      //BFB. New combo for comboFilterByDesc
      productsFilterModel = new QSqlQueryModel();
      modelsCreated = true;
    }
    setupModel();
    setupHistoryTicketsModel();
    setupClients();
    //pass db to login/pass dialogs
    dlgLogin->setDb(db);
    dlgPassword->setDb(db);
  }
}

void lemonView::setupClients()
{
  qDebug()<<"Setting up clients...";
  ClientInfo info;
  QString mainClient;
  clientsHash.clear();
  ui_mainview.comboClients->clear();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  clientsHash = myDb->getClientsHash();
  mainClient  = myDb->getMainClient();

    //Set by default the 'general' client.
    QHashIterator<QString, ClientInfo> i(clientsHash);
    while (i.hasNext()) {
      i.next();
      info = i.value();
      ui_mainview.comboClients->addItem(info.name);
    }

    int idx = ui_mainview.comboClients->findText(mainClient,Qt::MatchCaseSensitive);
    if (idx>-1) ui_mainview.comboClients->setCurrentIndex(idx);
    clientInfo = clientsHash.value(mainClient);
    updateClientInfo();
}

void lemonView::comboClientsOnChange()
{
  QString newClientName    = ui_mainview.comboClients->currentText();
  if (clientsHash.contains(newClientName)) {
    clientInfo = clientsHash.value(newClientName);
    updateClientInfo();
    refreshTotalLabel();
    ui_mainview.editItemCode->setFocus();
  }
}

void lemonView::updateClientInfo()
{
  QString pStr = i18n("Points: %1", clientInfo.points);
  QString dStr = i18n("Discount: %1%",clientInfo.discount);
  ui_mainview.lblClientDiscount->setText(dStr);
  //ui_mainview.lblClientPoints->setText(pStr);
  QPixmap pix;
  pix.loadFromData(clientInfo.photo);
  ui_mainview.lblClientPhoto->setPixmap(pix);
}

void lemonView::setHistoryFilter() {
  historyTicketsModel->setFilter(QString("date <= STR_TO_DATE('%1', '%d/%m/%Y')").
    arg(ui_mainview.editTicketDatePicker->date().toString("dd/MM/yyyy")));
  historyTicketsModel->setSort(historyTicketsModel->fieldIndex("id"),Qt::DescendingOrder); //change this when implemented headers click
}

void lemonView::setupHistoryTicketsModel()
{
  //qDebug()<<"Db name:"<<db.databaseName()<<", Tables:"<<db.tables();
  if (historyTicketsModel->tableName().isEmpty()) {
    if (!db.isOpen()) db.open();
    historyTicketsModel->setTable("v_transactions");
    historyTicketsModel->setRelation(historyTicketsModel->fieldIndex("clientid"), QSqlRelation("clients", "id", "name"));
    historyTicketsModel->setRelation(historyTicketsModel->fieldIndex("userid"), QSqlRelation("users", "id", "username"));
    
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("id"), Qt::Horizontal, i18n("Tr"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("clientid"), Qt::Horizontal, i18n("Client"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("datetime"), Qt::Horizontal, i18n("Date"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("userid"), Qt::Horizontal, i18n("User"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("itemcount"), Qt::Horizontal, i18n("Items"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("amount"), Qt::Horizontal, i18n("Total"));
    historyTicketsModel->setHeaderData(historyTicketsModel->fieldIndex("disc"), Qt::Horizontal, i18n("Discount"));

    ui_mainview.ticketView->setModel(historyTicketsModel);
    ui_mainview.ticketView->setColumnHidden(historyTicketsModel->fieldIndex("date"), true);
    ui_mainview.ticketView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui_mainview.ticketView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui_mainview.ticketView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.ticketView->resizeColumnsToContents();
    ui_mainview.ticketView->setCurrentIndex(historyTicketsModel->index(0, 0));
    
    historyTicketsModel->setSort(historyTicketsModel->fieldIndex("id"),Qt::DescendingOrder);
    historyTicketsModel->select();
  }
  setHistoryFilter();
}

void lemonView::setupTicketView()
{
  QSize tableSize = ui_mainview.ticketView->size();
  int portion = tableSize.width()/7;
  ui_mainview.ticketView->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("id"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("name"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("datetime"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("username"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("itemcount"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("amount"), portion);
  ui_mainview.ticketView->horizontalHeader()->resizeSection(historyTicketsModel->fieldIndex("disc"), portion);
}

void lemonView::itemHIDoubleClicked(const QModelIndex &index){
  if (db.isOpen()) {
    //getting data from model...
    const QAbstractItemModel *model = index.model();
    int row = index.row();
    QModelIndex indx = model->index(row, 1); // id = columna 1
    qulonglong transactionId = model->data(indx, Qt::DisplayRole).toULongLong();
    printTicketFromTransaction(transactionId);
  }
}

void lemonView::printSelTicket()
{
  QModelIndex index = ui_mainview.ticketView->currentIndex();
  if (historyTicketsModel->tableName().isEmpty()) setupHistoryTicketsModel();
  if (index == historyTicketsModel->index(-1,-1) ) {
    KMessageBox::information(this, i18n("Please select a ticket to print."), i18n("Cannot print ticket"));
  }
  else  {
    qulonglong id = historyTicketsModel->record(index.row()).value("id").toULongLong();
    printTicketFromTransaction(id);
  }
}

void lemonView::printTicketFromTransaction(qulonglong transactionNumber){
  QList<TicketLineInfo> ticketLines;
  ticketLines.clear();
  
  if (!db.isOpen()) db.open();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  
  TransactionInfo trInfo = myDb->getTransactionInfo(transactionNumber);
  QList<TransactionItemInfo> pListItems = myDb->getTransactionItems(transactionNumber);
  for (int i = 0; i < pListItems.size(); ++i){
    TransactionItemInfo trItem = pListItems.at(i);
    // add line to ticketLines
    TicketLineInfo tLineInfo;
    tLineInfo.qty     = trItem.qty;
    tLineInfo.unitStr = trItem.unitStr;
    tLineInfo.desc    = trItem.name;
    tLineInfo.price   = trItem.price;
    tLineInfo.disc    = trItem.disc;
    tLineInfo.total   = trItem.total;
    ticketLines.append(tLineInfo);
  }
  //Ticket
  TicketInfo ticket;
  QDateTime dt;
  dt.setDate(trInfo.date);
  dt.setTime(trInfo.time);
  ticket.datetime = dt;
  ticket.number = transactionNumber;
  ticket.total  = trInfo.amount;
  ticket.change = trInfo.changegiven;
  ticket.paidwith = trInfo.paywith;
  ticket.itemcount = trInfo.itemcount;
  ticket.cardnum = trInfo.cardnumber;
  ticket.cardAuthNum = trInfo.cardauthnum;
  ticket.paidWithCard = (trInfo.paymethod == 2) ? true:false;
  ticket.clientDisc = 0;
  ticket.clientDiscMoney = 0;
  ticket.buyPoints = 0;
  ticket.clientPoints = 0;
  ticket.lines = ticketLines;
  printTicket(ticket);
  
}

void lemonView::showReprintTicket()
{
  ui_mainview.mainPanel->setCurrentIndex(pageReprintTicket);
  QTimer::singleShot(500, this, SLOT(setupTicketView()));
}

void lemonView::cashOut()
{
  // ASK FOR ADMIN PASSWORD ??
  double max = drawer->getAvailableInCash();
  if (!max>0) {
    KNotification *notify = new KNotification("information", this);
    notify->setText(i18n("Cash not available at drawer!"));
    QPixmap pixmap = DesktopIcon("dialog-error",32);
    notify->setPixmap(pixmap);
    notify->sendEvent();
  } else {
    InputDialog *dlg = new InputDialog(this, false, dialogCashOut, i18n("Cash Out"), 0.001, max);
    if (dlg->exec() ) {
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);

      CashFlowInfo info;
      info.amount = dlg->dValue;
      info.reason = dlg->reason;
      info.date = QDate::currentDate();
      info.time = QTime::currentTime();
      info.terminalNum = Settings::editTerminalNumber();
      info.userid = loggedUserId;
      info.type   = ctCashOut; //Normal cash-out
      myDb->insertCashFlow(info);
      //affect drawer
      if (Settings::openDrawer()) drawer->open();
      drawer->substractCash(info.amount);
    }
  }
}

void lemonView::cashIn()
{
  // ASK FOR ADMIN PASSWORD ??
  InputDialog *dlg = new InputDialog(this, false, dialogCashOut, i18n("Cash In"));
  if (dlg->exec() ) {
    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);

    CashFlowInfo info;
    info.amount = dlg->dValue;
    info.reason = dlg->reason;
    info.date = QDate::currentDate();
    info.time = QTime::currentTime();
    info.terminalNum = Settings::editTerminalNumber();
    info.userid = loggedUserId;
    info.type   = ctCashIn; //normal cash-out
    myDb->insertCashFlow(info);
    //affect drawer
    if (Settings::openDrawer()) drawer->open();
    drawer->addCash(info.amount);
  }
}

void lemonView::cashAvailable()
{
  double available = drawer->getAvailableInCash();
  KNotification *notify = new KNotification("information", this);
  notify->setText(i18n("There are <b> %1 in cash </b> available at the drawer.", KGlobal::locale()->formatMoney(available)));
  QPixmap pixmap = DesktopIcon("dialog-information",32);
  notify->setPixmap(pixmap);
  notify->sendEvent();
}

#include "lemonview.moc"


