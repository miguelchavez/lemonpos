/***************************************************************************
 *   Copyright (C) 2007-2010 by Miguel Chavez Gamboa                       *
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

#include <QApplication>
#include <QLocale>
#include <QDebug>
#include <QTimer>
#include <QTextCodec>
#include <QtSql>
#include <QSettings>


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../mibitWidgets/mibittip.h"
#include "../mibitWidgets/mibitfloatpanel.h"
#include "../mibitWidgets/mibitdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug()<<tr("Starting dispatch...");

    readSettings();

    modelsCreated=false;

    //Assing controls to panels.
    groupPanel = new MibitFloatPanel(this, "panel_top.svg", Top);
    groupPanel->setSize(311,200);
    groupPanel->addWidget(ui->groupConfig);
    groupPanel->setMode(pmManual);
    groupPanel->setHiddenTotally(true);
    groupPanel->hide();

    confirmDialog = new MibitDialog(this/*ui->unconfirmedView*/,"dialog.svg",QPixmap("icon.png"), datSlideDown );
    confirmDialog->setSize(500,550);
    confirmDialog->addWidget(ui->groupConfirm);


    connect( ui->btnSaveConfig, SIGNAL(clicked()), groupPanel, SLOT(hidePanel()) );
    connect( ui->btnSaveConfig, SIGNAL(clicked()), this, SLOT(writeSettings()) );
    connect( ui->btnSaveConfig, SIGNAL(clicked()), this, SLOT(settingsChanged()));
    connect( ui->actionShow_Settings, SIGNAL(triggered()), this, SLOT(showConfig()));
    connect( ui->actionExit, SIGNAL(triggered()), this, SLOT(quit()));
    connect(ui->splitter, SIGNAL(splitterMoved(int, int)), SLOT(saveSplitter()));
    connect(ui->splitter, SIGNAL(splitterMoved(int, int)), SLOT(adjustTables()));
    connect(ui->unconfirmedView, SIGNAL(clicked(QModelIndex)), SLOT(showConfirmation()) );
    connect(ui->btnConfirmCancel, SIGNAL(clicked()), confirmDialog, SLOT(hideDialog()) );
    connect(ui->btnConfirm, SIGNAL(clicked()), this, SLOT(confirmOrder()));

    QTimer::singleShot(1000, this, SLOT(setupDB()));
    QTimer *updateTimer = new QTimer(this);
    updateTimer->setInterval(1000);
    connect(updateTimer, SIGNAL(timeout()), SLOT(refreshModels()) );
    updateTimer->start();
}

void MainWindow::showConfig()
{
    //load config
    readSettings();
    //show it...
    groupPanel->showPanel();
    //set focus to the db field
    ui->editCfgDatabase->setFocus();
}

void MainWindow::setTheSplitterSizes(QPoint p)
{
  QList<int> s;
  s.append(p.x());
  s.append(p.y());
  qDebug()<<"P:"<<p<<" S:"<<s;
  ui->splitter->setSizes(s);
}

QList<int> MainWindow::getTheSplitterSizes()
{
  return ui->splitter->sizes();
}


void MainWindow::saveSplitter()
{
    QSettings settings("lemonPOS", "LemonPOS Dispatch Mode");
    settings.setValue("splitter", ui->splitter->saveState());
}

void MainWindow::readSettings()
 {
     QSettings settings("lemonPOS", "LemonPOS Dispatch Mode");
     ui->editCfgDatabase->setText(settings.value("dbName", QString("lemondb")).toString());
     ui->editCfgHost->setText(settings.value("dbHost", QString("localhost")).toString());
     ui->editCfgPassword->setText(settings.value("dbPassword", QString("xarwit0721")).toString());
     ui->editCfgProvider->setCurrentIndex( ui->editCfgProvider->findText( (settings.value("dbProvider", QString("PostgreSQL")).toString()) , Qt::MatchExactly ) );
     ui->editCfgUser->setText(settings.value("dbUser", QString("lemonuser")).toString());

     ui->splitter->restoreState(settings.value("splitter").toByteArray());
     restoreGeometry(settings.value("myWidget/geometry").toByteArray());
     restoreState(settings.value("myWidget/windowState").toByteArray());

 }

void MainWindow::writeSettings()
 {
     QSettings settings("lemonPOS", "LemonPOS Dispatch Mode");
     settings.setValue("dbName", ui->editCfgDatabase->text());
     settings.setValue("dbHost", ui->editCfgHost->text());
     settings.setValue("dbPassword", ui->editCfgPassword->text());
     settings.setValue("dbProvider", ui->editCfgProvider->currentText());
     settings.setValue("dbUser", ui->editCfgUser->text());

 }

/* NOTE: Config needs a keyboard!!!.. or use a virtual one.
   Once is configured, its not needed, so its no need to make available the config dialog  */
void MainWindow::settingsChanged()
{
  //Reconnect to db..
  if (db.isOpen()) db.close();

  //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  //See if this does not crash dispatch

  QString toCompare;
  qDebug()<<" Database STR ON COMBO:"<<ui->editCfgProvider->currentText()<<" Database still in use:"<<db.driverName();
  if (ui->editCfgProvider->currentText() == "MySQL") toCompare = "QMYSQL"; else toCompare = "QPSQL";

  if (toCompare != db.driverName() ) {
      qDebug()<<"Database driver was changed...  new driver:"<<db.driverName();
      db.close();
      db.removeDatabase(db.connectionName());
      if (ui->editCfgProvider->currentText() == "MySQL")
          db = QSqlDatabase::addDatabase("QMYSQL");
      else //for now only mysql and psql
          db = QSqlDatabase::addDatabase("QPSQL");
  } else {
      return;
  }


  db.setHostName(ui->editCfgHost->text());
  db.setDatabaseName(ui->editCfgDatabase->text());
  db.setUserName(ui->editCfgUser->text());
  db.setPassword(ui->editCfgPassword->text());

  connectToDb();
  //setupModel(); TODO!
}

void MainWindow::setupDB()
{
  qDebug()<<"Setting up database...";
  if (db.isOpen()) db.close();
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

  //See if this does not crash dispatch
  if (ui->editCfgProvider->currentText() == "MySQL")
      db = QSqlDatabase::addDatabase("QMYSQL");
  else //for now only mysql and psql
      db = QSqlDatabase::addDatabase("QPSQL");

  db.setHostName(ui->editCfgHost->text());
  db.setDatabaseName(ui->editCfgDatabase->text());
  db.setUserName(ui->editCfgUser->text());
  db.setPassword(ui->editCfgPassword->text());


  connectToDb();
}

void MainWindow::connectToDb()
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
    groupPanel->showPanel();
  } else {
      qDebug()<<" CONNECTED. ";
    //finally, when connection stablished, setup all models.
    if (!modelsCreated) { //Create models...
      shipmentsModel       = new QSqlTableModel();
      modelsCreated = true;
    }
    setupModel();
  }
}


void MainWindow::setupModel()
{
  if (!db.isOpen()) {
    connectToDb();
  }
  else {
    //workaround for a stupid crash: when re-connecting after Config, on setTable crashes.
    //Crashes without debug information.

    qDebug()<<"setting up database model   ";
    shipmentsModel->setTable("shipments");
    qDebug()<<shipmentsModel->tableName();
    if (shipmentsModel->tableName() != "shipments")
      shipmentsModel->setTable("shipments");


    shipmentsModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    shipmentsModel->setHeaderData(shipmentsModel->fieldIndex("id"), Qt::Horizontal, tr("Id"));
    shipmentsModel->setHeaderData(shipmentsModel->fieldIndex("clientName"), Qt::Horizontal, tr("Client"));


    ui->unconfirmedView->setModel(shipmentsModel);
    ui->unconfirmedView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //ui->unconfirmedList->setItemDelegate(new QItemDelegate());

    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("clientId"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("clientAddr"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("clientPosition"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderNumber"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderList"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderPosition"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderStatus"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderETA"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderTimeToCook"), true);
    ui->unconfirmedView->setColumnHidden(shipmentsModel->fieldIndex("orderComments"), true);


    shipmentsModel->setFilter( "shipments.orderStatus < 3 and shipments.orderStatus > 0" );

    shipmentsModel->select();
    ui->unconfirmedView->resizeColumnsToContents();

    QTimer::singleShot(200,this, SLOT(adjustTables()));

    qDebug()<<shipmentsModel->database();
    qDebug()<<shipmentsModel->record(2).value("clientName").toString();
    qDebug()<<"Row Count:"<<shipmentsModel->rowCount();

  }
}

void MainWindow::refreshModels()
{
    if ( modelsCreated && db.isOpen() ) {
       shipmentsModel->select();
    }
}


void MainWindow::adjustTables()
{
  QSize size = ui->unconfirmedView->size();
  int portion = size.width()/5;
  ui->unconfirmedView->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui->unconfirmedView->horizontalHeader()->resizeSection(shipmentsModel->fieldIndex("id"), portion*1.5); // Id
  ui->unconfirmedView->horizontalHeader()->resizeSection(shipmentsModel->fieldIndex("clientName"), portion*3.3); //Name
}

void MainWindow::showConfirmation()
{
    //get all needed data...
    //show the dialog
    confirmDialog->showDialog();
    ui->editEstimatedMinutes->setFocus();
}

void MainWindow::confirmOrder()
{

    //finally close the dialog
    confirmDialog->hideDialog();
}

void MainWindow::quit()
{
    // about to quit? --> if any orders are not processed.
    qApp->exit();
}

void MainWindow::closeEvent(QCloseEvent *event)
 {
     QSettings settings("lemonPOS", "LemonPOS Dispatch Mode");
     settings.setValue("geometry", saveGeometry());
     settings.setValue("windowState", saveState());
     QMainWindow::closeEvent(event);
 }

MainWindow::~MainWindow()
{
    delete ui;
}

