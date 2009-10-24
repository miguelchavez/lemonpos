/***************************************************************************
 *   Copyright (C) 2007-2009 by Miguel Chavez Gamboa                       *
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

#define QT_GUI_LIB
//The above line is because that define is needed, and as i dont use qmake, i must define it here..
//And only caused problems with the QSqlRelationalDelegate.. what a thing.

#include "squeezeview.h"
#include "settings.h"
#include "usersdelegate.h"
#include "usereditor.h"
#include "clienteditor.h"
#include "promoeditor.h"
#include "producteditor.h"
#include "purchaseeditor.h"
#include "../../src/hash.h"
#include "../../src/misc.h"
#include "../../src/structs.h"
#include "../../src/enums.h"
#include "../../src/productdelegate.h"
#include "offersdelegate.h"
#include "../../dataAccess/azahar.h"

// Pie Chart by EresLibre
#include "piechart.h"

#include <QLabel>
#include <QPixmap>
#include <QByteArray>
#include <QBuffer>
#include <QTimer>
#include <QDoubleValidator>
#include <QRegExp>
#include <QTableView>
#include <QInputDialog>

#include <QDataWidgetMapper>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QItemDelegate>
#include <QHeaderView>

#include <klocale.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kstandarddirs.h>

#include <kplotobject.h>
#include <kplotwidget.h>
#include <kplotaxis.h>
#include <kplotpoint.h>

#include <kpassivepopup.h>
#include <KNotification>

//TODO: Change all qDebug to errorDialogs or remove them.
//NOTE: Common configuration fields need to be shared between lemon and squeeze (low stock alarm value).

enum {pWelcome=0, pBrowseProduct=1, pBrowseOffers=2, pBrowseUsers=3, pBrowseMeasures=4, pBrowseCategories=5, pBrowseClients=6, pBrowseTransactions=7, pBrowseBalances=8, pBrowseCashFlow=9};


squeezeView::squeezeView(QWidget *parent)
    : QWidget(parent)//,
      //m_toolBar(0)
{
  ui_mainview.setupUi(this);
  setAutoFillBackground(true);

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  db = QSqlDatabase::addDatabase("QMYSQL");
  
  ///Login dialog
  dlgPassword = new LoginWindow(this,
                                 i18n("Authorisation Required"),
                                 i18n("Enter administrator password please."),
                                 LoginWindow::PasswordOnly);


  
  /// Left Menu
  ui_mainview.titleData->setPixmap(DesktopIcon("kexi",32)); //database icon kexi
  ui_mainview.titleStock->setPixmap(DesktopIcon("lemon-box",32));
  ui_mainview.titleReports->setPixmap(DesktopIcon("view-pim-tasks", 32)); //graphics-viewer-document

  ///icons
  ui_mainview.btnExit->setIcon(DesktopIcon("system-shutdown", 32)); //application-exit
  ui_mainview.btnConfigure->setIcon(DesktopIcon("configure", 32));

  ui_mainview.titleBar->setText(i18n("Basic Information"), Qt::AlignLeft | Qt::AlignBottom); //no funciona bien el ktitlewidget...
  ui_mainview.titleBar->setPixmap((DesktopIcon("view-statistics", 32)));

  ///other things
  counter = 5;
  modelsCreated = false;
  graphSoldItemsCreated = false;
  timerCheckDb = new QTimer(this);
  timerCheckDb->setInterval(1000);
  timerUpdateGraphs = new QTimer(this);
  timerUpdateGraphs->setInterval(10000);
  categoriesHash.clear();
  setupSignalConnections();
  QTimer::singleShot(1100, this, SLOT(setupDb()));
  QTimer::singleShot(2000, timerCheckDb, SLOT(start()));
  QTimer::singleShot(20000, timerUpdateGraphs, SLOT(start()));
  QTimer::singleShot(2010, this, SLOT(showWelcomeGraphs()));
  QTimer::singleShot(2000, this, SLOT(login()));

  ui_mainview.stackedWidget->setCurrentIndex(pWelcome);
  ui_mainview.errLabel->hide();
  ui_mainview.productsViewAlt->hide();

  QString logoBottomFile = KStandardDirs::locate("appdata", "images/logo.png");
  ui_mainview.logoLabel->setPixmap(QPixmap(logoBottomFile));
  ui_mainview.logoLabel->setAlignment(Qt::AlignCenter);

  if (Settings::isProductsGridDefault()) {
    ui_mainview.productsView->show();
    ui_mainview.productsViewAlt->hide();
    ui_mainview.chViewProductsListAsGrid->setChecked(true);
    ui_mainview.chViewProductsListAsTable->setChecked(false);
  } else {
    ui_mainview.productsView->hide();
    ui_mainview.productsViewAlt->show();
    ui_mainview.chViewProductsListAsGrid->setChecked(false);
    ui_mainview.chViewProductsListAsTable->setChecked(true);
  }
  
  //m_toolBar = new FloatingToolBar(this,this); //ui_mainview.groupFilterProducts
  //m_toolBar->winId(); // force it to be a native widget (prevents problem with QX11EmbedContainer)
  //m_toolBar->setSide(FloatingToolBar::Top);
}


void squeezeView::cleanErrorLabel()
{
  ui_mainview.errLabel->clear();
  ui_mainview.errLabel->hide();
}

void squeezeView::login(){
  qDebug()<<"Login()";
  emit signalAdminLoggedOff();
  dlgPassword->clearLines();
  if (!db.isOpen()) {
    db.open(); //try to open connection
    qDebug()<<"(1): Trying to open connection to database..";
  }
  if (!db.isOpen()) {
    QString details = db.lastError().text();

    //testing knotification: seems not to work.
    KPassivePopup::message( i18n("Error:"),details, DesktopIcon("dialog-error", 48), this );
    KNotification *notify = new KNotification(i18n("Unable to connect to the database"), this);
    notify->setText(details);
    QPixmap pixmap = DesktopIcon("dialog-error",32); //NOTE: This does not works
    notify->setPixmap(pixmap);
    notify->sendEvent();
    
    emit signalShowDbConfig();
  } else {
    bool doit = false;
    do  {
      if (Settings::askAdminPassword()) {
        doit = dlgPassword->exec();
      } else doit=true;
    } while (!doit);
    
    if ( doit ) {
        emit signalAdminLoggedOn();
        qDebug()<<"Admin Logged on..";
    } else {
      emit signalAdminLoggedOff();
      qDebug()<<"login cancelled...";
    }
  }
}

void squeezeView::setupSignalConnections()
{
  connect(ui_mainview.usersView, SIGNAL(activated(const QModelIndex &)), SLOT(usersViewOnSelected(const QModelIndex &)));
  connect(ui_mainview.clientsView, SIGNAL(activated(const QModelIndex &)), SLOT(clientsViewOnSelected(const QModelIndex &)));
  connect(ui_mainview.productsView, SIGNAL(activated(const QModelIndex &)), SLOT(productsViewOnSelected(const QModelIndex &)));
  connect(ui_mainview.productsViewAlt, SIGNAL(activated(const QModelIndex &)), SLOT(productsViewOnSelected(const QModelIndex &)));

  connect(ui_mainview.groupFilterOffers, SIGNAL(toggled(bool)), SLOT(setOffersFilter()));
  
  connect(ui_mainview.chOffersSelectDate, SIGNAL(toggled(bool)), SLOT(setOffersFilter()));
  connect(ui_mainview.chOffersTodayDiscounts, SIGNAL(toggled(bool)), SLOT(setOffersFilter()));
  connect(ui_mainview.chOffersOldDiscounts, SIGNAL(toggled(bool)), SLOT(setOffersFilter()));
  connect(ui_mainview.chOffersFilterByProduct, SIGNAL(toggled(bool)), SLOT(setOffersFilter()));
  connect(ui_mainview.editOffersFilterByProduct, SIGNAL( textEdited(const QString &) ), SLOT(setOffersFilter()));
  connect(ui_mainview.btnAddUser, SIGNAL(clicked()), SLOT(createUser()));
  connect(ui_mainview.btnAddOffer, SIGNAL(clicked()), SLOT(createOffer()));
  connect(ui_mainview.btnDeleteUser, SIGNAL(clicked()), SLOT(deleteSelectedUser()));
  connect(ui_mainview.btnDeleteOffer, SIGNAL(clicked()), SLOT(deleteSelectedOffer()));
  connect(ui_mainview.btnAddProduct, SIGNAL(clicked()), SLOT(createProduct()) );
  connect(ui_mainview.btnAddMeasure, SIGNAL(clicked()), SLOT(createMeasure()) );
  connect(ui_mainview.btnAddCategory, SIGNAL(clicked()), SLOT(createCategory()) );
  connect(ui_mainview.btnAddClient, SIGNAL(clicked()), SLOT(createClient()));
  connect(ui_mainview.btnDeleteProduct, SIGNAL(clicked()), SLOT(deleteSelectedProduct()) );
  connect(ui_mainview.btnDeleteMeasure, SIGNAL(clicked()), SLOT(deleteSelectedMeasure()) );
  connect(ui_mainview.btnDeleteCategory, SIGNAL(clicked()), SLOT(deleteSelectedCategory()) );
  connect(ui_mainview.btnDeleteClient, SIGNAL(clicked()), SLOT(deleteSelectedClient()));
  connect(ui_mainview.btnConfigure, SIGNAL(clicked()),  SLOT( showPrefs()));

  connect(timerCheckDb, SIGNAL(timeout()), this, SLOT(checkDBStatus()));
  connect(timerUpdateGraphs, SIGNAL(timeout()), this, SLOT(updateGraphs()));
  connect(ui_mainview.offersDateEditor, SIGNAL(changed(const QDate &)), this, SLOT(setOffersFilter()));

  connect(this, SIGNAL(signalAdminLoggedOn()),  SLOT( enableUI()));
  connect(this, SIGNAL(signalAdminLoggedOff()),  SLOT( disableUI()));
  connect(ui_mainview.btnExit, SIGNAL(clicked()),  SLOT( doEmitSignalSalir()));

  connect(ui_mainview.btnUsers, SIGNAL(clicked()),  SLOT( showUsersPage()));
  connect(ui_mainview.btnClients, SIGNAL(clicked()),  SLOT( showClientsPage()));
  connect(ui_mainview.btnProducts, SIGNAL(clicked()),  SLOT( showProductsPage()));
  connect(ui_mainview.btnCategories, SIGNAL(clicked()),  SLOT( showCategoriesPage()));
  connect(ui_mainview.btnMeasures, SIGNAL(clicked()),  SLOT( showMeasuresPage()));
  connect(ui_mainview.btnOffers, SIGNAL(clicked()),  SLOT( showOffersPage()));
  connect(ui_mainview.btnQuickInfo, SIGNAL(clicked()),  SLOT( showWelcomeGraphs()));
  connect(ui_mainview.btnTransactions, SIGNAL(clicked()),  SLOT( showTransactionsPage()));
  connect(ui_mainview.btnBalances, SIGNAL(clicked()),  SLOT( showBalancesPage()));
  connect(ui_mainview.btnStockCheckIn, SIGNAL(clicked()),  SLOT( doPurchase()));
  connect(ui_mainview.btnCashFlow, SIGNAL(clicked()),  SLOT( showCashFlowPage()));
  
  connect(ui_mainview.chViewProductsListAsGrid, SIGNAL(toggled(bool)), SLOT(showProdListAsGrid()));
  connect(ui_mainview.chViewProductsListAsTable, SIGNAL(toggled(bool)), SLOT(showProdListAsTable() ));

  //connect actions for transactions filters
  connect(ui_mainview.groupFilterTransactions, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByStateFinished, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByStateCancelled, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByPaidCash, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByPaidCredit, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByDate, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.transactionsDateEditor, SIGNAL( changed(const QDate &) ), SLOT(setTransactionsFilter()));
  connect(ui_mainview.rbTransFilterByUser, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.editTransUsersFilter,SIGNAL(returnPressed()), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByClient, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.editTransClientsFilter,SIGNAL(returnPressed()), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByAmountLess, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterByAmountGreater, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.editTransAmountLess ,SIGNAL(valueChanged ( double ) ), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.editTransAmountGreater,SIGNAL(valueChanged ( double ) ), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransFilterTerminalNum, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.editTransTermNum,SIGNAL(valueChanged ( int ) ), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransactionsFilterOnlySales, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransactionsFilterOnlyPurchases, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );
  connect(ui_mainview.rbTransactionsFilterOnlyChangesReturns, SIGNAL(toggled(bool)), this, SLOT( setTransactionsFilter()) );

  //connect actions for balances filters
  connect(ui_mainview.rbBalancesFilterByState, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.rbBalancesFilterBySuspicious, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.rbBalancesFilterByDate, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.rbBalancesFilterByUser, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.rbBalancesFilterByCashInLess, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.rbBalancesFilterByCashInGrater, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.editBalancesFilterByDate, SIGNAL( changed(const QDate &) ), SLOT(setBalancesFilter()));
  connect(ui_mainview.rbBalancesFilgerByTerminalNum, SIGNAL(toggled(bool)), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.editBalancesFilterByVendor,SIGNAL(returnPressed()), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.editBalancesFilterByCasInLess ,SIGNAL(valueChanged ( double ) ), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.editBalancesFilterByCashInGrater,SIGNAL(valueChanged ( double ) ), this, SLOT( setBalancesFilter()) );
  connect(ui_mainview.editBalancesFilterByTermNum,SIGNAL(valueChanged ( int ) ), this, SLOT( setBalancesFilter()) );

  connect(ui_mainview.comboProductsFilterByCategory,SIGNAL(currentIndexChanged(int)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.editProductsFilterByDesc,SIGNAL(returnPressed()), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByDesc, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByCategory, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByAvailable, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByNotAvailable, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByMostSold, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.rbProductsFilterByLessSold, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );
  connect(ui_mainview.groupFilterProducts, SIGNAL(toggled(bool)), this, SLOT( setProductsFilter()) );

  ui_mainview.groupFilterProducts->setMouseTracking(true);
//   connect(ui_mainview.groupFilterProducts, SIGNAL(toggled(bool)), this, SLOT( hideShowFilterProductsGroup() ) );
//   connect(ui_mainview.listView, SIGNAL(entered(const QModelIndex &)), SLOT(listViewOnMouseMove(const QModelIndex &)));
  
  connect(ui_mainview.groupFilterProducts, SIGNAL(entered()), this, SLOT( hideShowFilterProductsGroup() ) );

  // BFB: New, export qtableview
  connect(ui_mainview.btnExportProducts, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportOffers, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportUsers, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportClients, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportMeasures, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportCategories, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportBalances, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportTransactions, SIGNAL(clicked()),  SLOT( exportTable()));
  connect(ui_mainview.btnExportCashFlow, SIGNAL(clicked()),  SLOT( exportTable()));
  //connect(ui_mainview.btnExportCustomReports, SIGNAL(clicked()),  SLOT( exportTable()));
  
}

void squeezeView::hideShowFilterProductsGroup()
{
  if ( ui_mainview.groupFilterProducts->isChecked() ){
    ui_mainview.groupFilterProducts->setMaximumSize(QSize(16777215,150));
  }
  else {
    ui_mainview.groupFilterProducts->setMaximumSize(QSize(16777215,30));
  }
}

void squeezeView::doEmitSignalSalir()
{
  emit signalSalir();
}

void squeezeView::enableUI()
{
  ui_mainview.stackedWidget->show();
  ui_mainview.btnConfigure->show();
  ui_mainview.leftFrame->setEnabled(true);
}

void squeezeView::disableUI()
{
  ui_mainview.stackedWidget->hide();
  ui_mainview.btnConfigure->hide();
  ui_mainview.leftFrame->setDisabled(true);
}

void squeezeView::showWelcomeGraphs()
{
  if (!graphSoldItemsCreated) setupGraphs();
  ui_mainview.stackedWidget->setCurrentIndex(pWelcome);
  ui_mainview.titleBar->setText(i18n("Quick Information"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("view-statistics",48)));
}


void squeezeView::showPrefs()
{
  emit signalShowPrefs();
}

void squeezeView::showProductsPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseProduct);
  if (productsModel->tableName().isEmpty()) setupProductsModel();
  ui_mainview.titleBar->setText(i18n("Products"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-box",32)));

  //m_toolBar->addWidget(ui_mainview.groupFilterProducts);
  //ui_mainview.groupFilterProducts->setChecked(true); //TESTING!!!
  //m_toolBar->showAndAnimate();
}

void squeezeView::showOffersPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseOffers);
  if (offersModel->tableName().isEmpty()) setupOffersModel();
  ui_mainview.titleBar->setText(i18n("Offers"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-offers",32)));
  ui_mainview.offersDateEditor->setDate(QDate::currentDate());
  QTimer::singleShot(500,this, SLOT(adjustOffersTable()));

  //FIXME & NOTE: Offers that does not have existing products are ignored (hidden) on the view, but still exists on Database.
  //              This happens when an offer exists for a product, but the product code is changed or deleted later.
}

void squeezeView::showUsersPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseUsers);
  if (usersModel->tableName().isEmpty()) setupUsersModel();
  ui_mainview.titleBar->setText(i18n("Vendors"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-user",32)));
}

void squeezeView::showMeasuresPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseMeasures);
  if (measuresModel->tableName().isEmpty()) setupMeasuresModel();
  ui_mainview.titleBar->setText(i18n("Weight and Measures"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-ruler",32)));
}

void squeezeView::showCategoriesPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseCategories);
  if (categoriesModel->tableName().isEmpty()) setupCategoriesModel();
  ui_mainview.titleBar->setText(i18n("Categories"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-categories",32)));
}

void squeezeView::showClientsPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseClients);
  if (clientsModel->tableName().isEmpty()) setupClientsModel();
  ui_mainview.titleBar->setText(i18n("Clients"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-user",32)));
}

void squeezeView::showTransactionsPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseTransactions);
  if (transactionsModel->tableName().isEmpty()) setupTransactionsModel();
  ui_mainview.titleBar->setText(i18n("Transactions"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-reports",32)));
  ui_mainview.transactionsDateEditor->setDate(QDate::currentDate());
}

void squeezeView::showBalancesPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseBalances);
  if (balancesModel->tableName().isEmpty()) setupBalancesModel();
  ui_mainview.titleBar->setText(i18n("Balances"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-balance",32)));
}


void squeezeView::showCashFlowPage()
{
  ui_mainview.stackedWidget->setCurrentIndex(pBrowseCashFlow);
  if (cashflowModel->tableName().isEmpty()) setupCashFlowModel();
  ui_mainview.titleBar->setText(i18n("Cash Flow"));
  ui_mainview.titleBar->setPixmap((DesktopIcon("lemon-cashout",32)));
  ui_mainview.cashFlowTable->resizeColumnsToContents();
}

void squeezeView::toggleFilterBox(bool show)
{
  if (show) {
    switch (ui_mainview.stackedWidget->currentIndex())
    {
      case pBrowseProduct: ui_mainview.groupFilterProducts->show(); break;
      case pBrowseOffers:ui_mainview.groupFilterOffers->show(); break;
      case pWelcome: break;
      default: break;
    }
  }
  else {
    switch (ui_mainview.stackedWidget->currentIndex())
    {
      case pBrowseProduct: ui_mainview.groupFilterProducts->hide(); break;
      case pBrowseOffers:ui_mainview.groupFilterOffers->hide(); break;
      case pWelcome: break;
      default: break;
    }
  }
}


void squeezeView::adjustProductsTable()
{
  QSize size = ui_mainview.productsViewAlt->size();
  int portion = size.width()/11;
  ui_mainview.productsViewAlt->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(0, portion*1.5); // CODE
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(1, portion*3.5); //Name
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(2, portion); //Price
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(3, portion);  //Stock
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(4, portion); //Cost
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(5, portion); //Sold Units
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(6, portion); //Last Sold
  ui_mainview.productsViewAlt->horizontalHeader()->resizeSection(7, portion);  //Category
}

void squeezeView::adjustOffersTable()
{
  qDebug()<<"adjusting table size...";
  QSize size = ui_mainview.tableBrowseOffers->size();
  int portion = size.width()/6;
  ui_mainview.tableBrowseOffers->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  ui_mainview.tableBrowseOffers->horizontalHeader()->resizeSection(1, portion*1.5); //PRODUCT DESC
  ui_mainview.tableBrowseOffers->horizontalHeader()->resizeSection(2, portion); //Qty
  ui_mainview.tableBrowseOffers->horizontalHeader()->resizeSection(3, portion); //Date start
  ui_mainview.tableBrowseOffers->horizontalHeader()->resizeSection(4, portion*2.5);  //date end
}

void squeezeView::showProdListAsGrid()
{
 ui_mainview.productsView->show();
 ui_mainview.productsViewAlt->hide();
}

void squeezeView::showProdListAsTable()
{
  ui_mainview.productsViewAlt->show();
  ui_mainview.productsView->hide();
  // BFB: There's no need to adjust product table. We could do a resizeColumnsToContents() after model.select()
  QTimer::singleShot(200,this, SLOT(adjustProductsTable()));
}

squeezeView::~squeezeView()
{
}

void squeezeView::setupGraphs()
{
  //Pie-chart
  pieSoldItems    = ui_mainview.widgetGraphMostSoldItems;
  pieAlmostSoldOutItems = ui_mainview.widgetGraphAlmostSoldOutItems;
  
  pieSoldItems->setKeysPosition(PieChart::Bottom);
  pieSoldItems->setShowKeys(true);

  pieAlmostSoldOutItems->setKeysPosition(PieChart::Bottom);
  pieAlmostSoldOutItems->setShowKeys(true);

  //plots...

  ///TODO: How to get lemon's settings?? We need shared settings, to get the currency string to display at: Month Sales (%1)
  QString mes = (QDate::longMonthName(QDate::currentDate().month())).toUpper();
  ui_mainview.plotSales->setMinimumSize( 200, 200 );
  ui_mainview.plotSales->setAntialiasing( true );
  objSales = new KPlotObject( Qt::red, KPlotObject::Bars);
  ui_mainview.plotSales->addPlotObject( objSales );
  ui_mainview.plotSales->axis( KPlotWidget::BottomAxis )->setLabel( i18n("%1", mes) );
  ui_mainview.plotSales->axis( KPlotWidget::LeftAxis )->setLabel( i18n("Month Sales (%1)", KGlobal::locale()->currencySymbol()) );
  ui_mainview.plotProfit->setMinimumSize( 200, 200 );
  ui_mainview.plotProfit->setAntialiasing( true );
  objProfit = new KPlotObject( Qt::red, KPlotObject::Bars);
  ui_mainview.plotProfit->addPlotObject( objProfit );
  ui_mainview.plotProfit->axis( KPlotWidget::BottomAxis )->setLabel( i18n("%1", mes) );
  ui_mainview.plotProfit->axis( KPlotWidget::LeftAxis )->setLabel( i18n("Month Profit (%1)", KGlobal::locale()->currencySymbol()) );

  objSales->setShowBars(true);
  objSales->setShowPoints(true);
  objSales->setShowLines(true);
  objSales->setLinePen( QPen( Qt::blue, 1.5, Qt::DashDotLine ) );
  objSales->setBarBrush( QBrush( Qt::lightGray, Qt::BDiagPattern ) );
  objSales->setBarPen(QPen(Qt::lightGray));
  objSales->setPointStyle(KPlotObject::Star);

  objProfit->setShowBars(true);
  objProfit->setShowPoints(true);
  objProfit->setShowLines(true);
  objProfit->setLinePen( QPen( Qt::blue, 1.5, Qt::DashDotLine ) );
  objProfit->setBarBrush( QBrush( Qt::lightGray, Qt::BDiagPattern ) );
  objProfit->setBarPen(QPen(Qt::lightGray));
  objProfit->setPointStyle(KPlotObject::Star);
  
  graphSoldItemsCreated = true;
  updateGraphs();
}


// UI and Database -- GRAPHS.
void squeezeView::updateGraphs()
{
  if (!db.isOpen()) openDB();
  if (db.isOpen()) {
    if (!graphSoldItemsCreated ) setupGraphs();
    else {
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      ///First we need to get data for the plots
      QList<TransactionInfo> monthTrans = myDb->getMonthTransactions();
      ProfitRange rangeP = myDb->getMonthProfitRange();
      ProfitRange rangeS = myDb->getMonthSalesRange();
      TransactionInfo info;

      ///plots
      //clear data
      objSales->clearPoints();
      objProfit->clearPoints();
      // X = date, Y=profit
      int hoy=0;
      hoy = QDate::currentDate().day();
      if (hoy==1) hoy=2;//si es el unico dia, exapndir a 2 para los limites

      //NOTE:Set the same scale for both plots?? to compare.. or his own range to each one. if profit>sales?
      ui_mainview.plotSales->setLimits(1, hoy, rangeS.min, rangeS.max);
      ui_mainview.plotProfit->setLimits(1, hoy, rangeP.min, rangeP.max);
      //insert each day's sales and profit.
      int day=0; double AccSales=0.0; double AccProfit=0.0;
      for (int i = 0; i < monthTrans.size(); ++i) {
        info = monthTrans.at(i);
        //qDebug()<<i<<", sales:"<<info.amount<<" profit:"<<info.utility;
        ///we got one result per day (sum)
        //insert the day,profit to the plot
        AccSales  = info.amount;
        AccProfit = info.profit;
        day       = info.date.day();
        objSales->addPoint(day,AccSales, QString::number(AccSales));
        objProfit->addPoint(day,AccProfit, QString::number(AccProfit));
      } //for each eleement
      

      ui_mainview.plotSales->update();
      ui_mainview.plotProfit->update();

//       foreach( KPlotObject *po, ui_mainview.plotSales->plotObjects() ) {
//         foreach( KPlotPoint *p, po->points() ) {
//           // Do stuff to each KPlotPoint
//           qDebug()<<"x:"<<p->x()<<" y:"<<p->y();
//         }
//      }
      

      /// PieCharts
      //FIRST MOST SOLD PRODUCTS
      QList<pieProdInfo> plist = myDb->getTop5SoldProducts();
      
      if (!plist.isEmpty()) {
        double suma = 0;
        pieProdInfo prod1, prod2, prod3, prod4, prod5;
        if (!graphSoldItemsCreated) setupGraphs();
        else {
          pieSoldItems->deleteSlices();
          for (int i = 0; i < plist.size(); ++i) {
             //qDebug()<<"(1) Insertando "<<plist.at(i).name;
            if (i==0) prod1 = plist.at(i);
            else if (i==1) prod2 = plist.at(i);
            else if (i==2) prod3 = plist.at(i);
              else if (i==3) prod4 = plist.at(i);
              else if (i==4) prod5 = plist.at(i);
                else qDebug()<<"NO DEBO ENTRAR AQUI! i=="<<i;
            suma = suma + plist.at(i).count;
          }

          //for (int i = 0; i < plist.size(); ++i) {
          //  qDebug()<<"1|"<<plist.at(i).name<<" count:="<<plist.at(i).count<<" %:="<<plist.at(i).count/suma<<" Suma:="<<suma;
          //}
          
          //INSERT the 5 products to the graph
          
          if (plist.count()>0) pieSoldItems->addSlice(QString("%1 [%2 %3]").arg(prod1.name).arg(prod1.count).arg(prod1.unitStr),(prod1.count/suma)*100, QColor(50, 255,255,255));
          if (plist.count()>1) pieSoldItems->addSlice(QString("%1 [%2 %3]").arg(prod2.name).arg(prod2.count).arg(prod2.unitStr),(prod2.count/suma)*100, QColor(75, 125,255,255));
          if (plist.count()>2) pieSoldItems->addSlice(QString("%1 [%2 %3]").arg(prod3.name).arg(prod3.count).arg(prod3.unitStr),(prod3.count/suma)*100, QColor(50, 0,255,255));
          if (plist.count()>3) pieSoldItems->addSlice(QString("%1 [%2 %3]").arg(prod4.name).arg(prod4.count).arg(prod4.unitStr),(prod4.count/suma)*100, QColor(200, 25,25,255));
          if (plist.count()>4) pieSoldItems->addSlice(QString("%1 [%2 %3]").arg(prod5.name).arg(prod5.count).arg(prod5.unitStr),(prod5.count/suma)*100, QColor(255, 120,120,255));

        }//else pupulate graphic slices
      }
      //NOW ALMOST SOLD OUT PRODUCTS (top 5)
      plist = myDb->getAlmostSoldOutProducts(Settings::mostSoldMinValue(),Settings::mostSoldMaxValue());
      if (!plist.isEmpty()) {
        double suma = 0;
        pieProdInfo prod1, prod2, prod3, prod4, prod5; //up to five slices.
        if (!graphSoldItemsCreated) setupGraphs();
        else {
          pieAlmostSoldOutItems->deleteSlices();
          int numProd = plist.size();
          for (int i = 0; i < numProd; ++i) {
            //qDebug()<<"(2) Insertando "<<plist.at(i).name;
            if (i==0) prod1 = plist.at(i);
            else if (i==1) prod2 = plist.at(i);
            else if (i==2) prod3 = plist.at(i);
            else if (i==3) prod4 = plist.at(i);
            else if (i==4) prod5 = plist.at(i);
            else qDebug()<<"NO DEBO ENTRAR AQUI! i=="<<i;
            suma = suma + plist.at(i).count;
          }

          //for (int i = 0; i < plist.size(); ++i) {
          //  qDebug()<<"2|"<<plist.at(i).name<<" count:="<<plist.at(i).count<<" %:="<<plist.at(i).count/suma<<" Suma:="<<suma;
          //}
          
          //INSERT the 5 products to the graph
          if (numProd>0) pieAlmostSoldOutItems->addSlice(QString("%1 [%2 %3]").arg(prod1.name).arg(prod1.count).arg(prod1.unitStr),(prod1.count/suma)*100, QColor(50, 255,255,255));
          if (numProd>1) pieAlmostSoldOutItems->addSlice(QString("%1 [%2 %3]").arg(prod2.name).arg(prod2.count).arg(prod2.unitStr),(prod2.count/suma)*100, QColor(75, 125,255,255));
          if (numProd>2) pieAlmostSoldOutItems->addSlice(QString("%1 [%2 %3]").arg(prod3.name).arg(prod3.count).arg(prod3.unitStr),(prod3.count/suma)*100, QColor(50, 0,255,255));
          if (numProd>3) pieAlmostSoldOutItems->addSlice(QString("%1 [%2 %3]").arg(prod4.name).arg(prod4.count).arg(prod4.unitStr),(prod4.count/suma)*100, QColor(200, 25,25,255));
          if (numProd>4) pieAlmostSoldOutItems->addSlice(QString("%1 [%2 %3]").arg(prod5.name).arg(prod5.count).arg(prod5.unitStr),(prod5.count/suma)*100, QColor(255, 120,120,255));

          //         pieSoldItems->addSlice(pname, pcount.toInt(), QColor(color,255,255,255)); //pieChart uses integers...
          //         color=color*2;
        }//else pupulate graphic slices
      }
    }
  }// if connected to db
}


/*  ----------------- Database ----------------- */
void squeezeView::setupDb()
{
  if (db.isOpen()) db.close();
  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());
  db.open();
  dlgPassword->setDb(db);
  if (db.isOpen()) {
    emit signalConnected();
    enableUI();
    productsModel   = new QSqlRelationalTableModel();
    offersModel     = new QSqlRelationalTableModel();
    usersModel      = new QSqlTableModel();
    measuresModel   = new QSqlTableModel();
    categoriesModel = new QSqlTableModel();
    clientsModel    = new QSqlTableModel();
    transactionsModel = new QSqlRelationalTableModel();
    balancesModel   = new QSqlTableModel();
    cashflowModel   = new QSqlRelationalTableModel();
    modelsCreated   = true;
    setupProductsModel();
    setupMeasuresModel();
    setupClientsModel();
    setupUsersModel();
    setupTransactionsModel();
    setupCategoriesModel();
    setupOffersModel();
    setupBalancesModel();
    setupCashFlowModel();
  } else {
    emit signalDisconnected();
    disableUI();
  }
}


void squeezeView::openDB()
{
 bool ok=false;
  if (!db.isOpen()) {
   ok = db.open();
  } else ok = true;

  if (!ok) {
   emit signalDisconnected();
   qDebug()<<db.lastError();
  } else {
    emit signalConnected();
    if (!modelsAreCreated()) setupDb();
  }
}

void squeezeView::connectToDb()
{
  if (!db.open()) {
    db.open(); //try to open connection
    qDebug()<<"(1/connectToDb) Trying to open connection to database..";
    emit signalDisconnected();
    disableUI();
  }
  if (!db.isOpen()) {
    qDebug()<<"(2/connectToDb) Configuring..";
    emit signalDisconnected();
    disableUI();
  } else {
    //finally, when connection stablished, setup all models.
    if (!modelsCreated) { //Create models...
      productsModel   = new QSqlRelationalTableModel();
      offersModel     = new QSqlRelationalTableModel();
      usersModel      = new QSqlTableModel();
      measuresModel   = new QSqlTableModel();
      categoriesModel = new QSqlTableModel();
      clientsModel    = new QSqlTableModel();
      transactionsModel = new QSqlRelationalTableModel();
      balancesModel   = new QSqlTableModel();
      cashflowModel   = new QSqlRelationalTableModel();
      modelsCreated = true;
    }
    dlgPassword->setDb(db);
    emit signalConnected();
    enableUI();
    setupProductsModel();
    setupMeasuresModel();
    setupClientsModel();
    setupUsersModel();
    setupTransactionsModel();
    setupCategoriesModel();
    setupOffersModel();
    setupBalancesModel();
    setupCashFlowModel();
    
  }
}

//NOTE:There is a problem if connected and then mysql stop... db.isOpen() returns true.
void squeezeView::checkDBStatus()
{
 if (!isConnected()) {
  if (counter < 4) {
    counter++;
    emit signalChangeStatusbar(i18n("Trying connection in %1 seconds...", 5-counter));
  }
  else {
    counter = 0;
    emit signalChangeStatusbar(i18n("Connecting..."));
    openDB();
  }
 } else emit signalChangeStatusbar("");
}

void squeezeView::closeDB()
{
  db.close();
}

void squeezeView::setupUsersModel()
{
  if (db.isOpen()) {
    usersModel->setTable("users");

    ui_mainview.usersView->setModel(usersModel);
    ui_mainview.usersView->setViewMode(QListView::IconMode);
    ui_mainview.usersView->setGridSize(QSize(170,170));
    ui_mainview.usersView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.usersView->setResizeMode(QListView::Adjust);
    ui_mainview.usersView->setModelColumn(usersModel->fieldIndex("photo"));
    ui_mainview.usersView->setSelectionMode(QAbstractItemView::SingleSelection);


    UsersDelegate *delegate = new UsersDelegate(ui_mainview.usersView);
    ui_mainview.usersView->setItemDelegate(delegate);

    usersModel->select();

  }
  else {
      //At this point, what to do?
     // inform to the user about the error and finish app  or retry again some time later?
    QString details = db.lastError().text();
    KMessageBox::detailedError(this, i18n("Squeeze has encountered an error, click details to see the error details."), details, i18n("Error"));
    QTimer::singleShot(10000, this, SLOT(setupUsersModel()));
  }
}

void squeezeView::setupProductsModel()
{
  openDB();
  qDebug()<<"setupProducts.. after openDB";
  if (db.isOpen()) {
    productsModel->setTable("products");

    productCodeIndex = productsModel->fieldIndex("code");
    productDescIndex = productsModel->fieldIndex("name");
    productPriceIndex= productsModel->fieldIndex("price");
    productStockIndex= productsModel->fieldIndex("stockqty");
    productCostIndex = productsModel->fieldIndex("cost");
    productSoldUnitsIndex= productsModel->fieldIndex("soldunits");
    productLastSoldIndex= productsModel->fieldIndex("datelastsold");
    productUnitsIndex= productsModel->fieldIndex("units");
    productPhotoIndex=productsModel->fieldIndex("photo");
    productCategoryIndex=productsModel->fieldIndex("category");
    productPointsIndex=productsModel->fieldIndex("points");
    productLastProviderIndex = productsModel->fieldIndex("lastproviderid");
    productAlphaCodeIndex = productsModel->fieldIndex("alphacode");
    productBrandIndex = productsModel->fieldIndex("brandid");
    productTaxModelIndex = productsModel->fieldIndex("taxmodel");


    ui_mainview.productsView->setModel(productsModel);
    ui_mainview.productsView->setViewMode(QListView::IconMode);
    ui_mainview.productsView->setGridSize(QSize(170,170));
    ui_mainview.productsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.productsView->setResizeMode(QListView::Adjust);
    ui_mainview.productsView->setModelColumn(productsModel->fieldIndex("photo"));

    ui_mainview.productsViewAlt->setModel(productsModel);
    ui_mainview.productsViewAlt->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.productsViewAlt->setSelectionMode(QAbstractItemView::SingleSelection);

    ui_mainview.productsViewAlt->setColumnHidden(productPhotoIndex, true);
    ui_mainview.productsViewAlt->setColumnHidden(productUnitsIndex, true);
    ui_mainview.productsViewAlt->setColumnHidden(productPointsIndex, true);

    productsModel->setRelation(productCategoryIndex, QSqlRelation("categories", "catid", "text"));
    productsModel->setRelation(productLastProviderIndex, QSqlRelation("providers", "id", "provname"));
    productsModel->setRelation(productBrandIndex, QSqlRelation("brands", "brandid", "bname"));
    productsModel->setRelation(productTaxModelIndex, QSqlRelation("taxmodels", "modelid", "tname"));

    productsModel->setHeaderData(productCodeIndex, Qt::Horizontal, i18n("Code"));
    productsModel->setHeaderData(productDescIndex, Qt::Horizontal, i18n("Name"));
    productsModel->setHeaderData(productCategoryIndex, Qt::Horizontal, i18n("Category") );
    productsModel->setHeaderData(productPriceIndex, Qt::Horizontal, i18n("Price") );
    productsModel->setHeaderData(productCostIndex, Qt::Horizontal, i18n("Cost") );
    productsModel->setHeaderData(productStockIndex, Qt::Horizontal, i18n("Stock Qty") );
    productsModel->setHeaderData(productSoldUnitsIndex, Qt::Horizontal, i18n("Sold Units") );
    productsModel->setHeaderData(productLastSoldIndex, Qt::Horizontal, i18n("Last Sold") );
    productsModel->setHeaderData(productLastProviderIndex, Qt::Horizontal, i18n("Last Provider") );
    productsModel->setHeaderData(productAlphaCodeIndex, Qt::Horizontal, i18n("Alpha Code") );
    productsModel->setHeaderData(productTaxModelIndex, Qt::Horizontal, i18n("Tax Model") );
    productsModel->setHeaderData(productBrandIndex, Qt::Horizontal, i18n("Brand") );
    
    ProductDelegate *delegate = new ProductDelegate(ui_mainview.productsView);
    ui_mainview.productsView->setItemDelegate(delegate);
    ui_mainview.productsView->setSelectionMode(QAbstractItemView::SingleSelection);

    productsModel->select();
    ui_mainview.productsViewAlt->resizeColumnsToContents();

    //populate Categories...
    populateCategoriesHash();
    ui_mainview.comboProductsFilterByCategory->clear();
      QHashIterator<QString, int> item(categoriesHash);
      while (item.hasNext()) {
        item.next();
        ui_mainview.comboProductsFilterByCategory->addItem(item.key());
      }
      ui_mainview.comboProductsFilterByCategory->setCurrentIndex(0);

      ui_mainview.rbProductsFilterByAvailable->setChecked(true);
      ui_mainview.productsViewAlt->setCurrentIndex(productsModel->index(0, 0));
      setProductsFilter();
 }
 qDebug()<<"setupProducts.. done.";
}

void squeezeView::populateCategoriesHash()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  categoriesHash.clear();
  categoriesHash = myDb->getCategoriesHash();
}

void squeezeView::setProductsFilter()
{
//NOTE: This is a QT BUG.
//   If filter by description is selected and the text is empty, and later is re-filtered
//   then NO pictures are shown; even if is refiltered again.
QRegExp regexp = QRegExp(ui_mainview.editProductsFilterByDesc->text());
if (!ui_mainview.groupFilterProducts->isChecked()) productsModel->setFilter("");
else {
  if (ui_mainview.rbProductsFilterByDesc->isChecked()) {
  //1st if: Filter by DESC.
    if (!regexp.isValid() || ui_mainview.editProductsFilterByDesc->text().isEmpty())  ui_mainview.editProductsFilterByDesc->setText("*");
    if (ui_mainview.editProductsFilterByDesc->text()=="*") productsModel->setFilter("");
    else  productsModel->setFilter(QString("products.name REGEXP '%1'").arg(ui_mainview.editProductsFilterByDesc->text()));
    productsModel->setSort(productStockIndex, Qt::DescendingOrder);
  }
  else if (ui_mainview.rbProductsFilterByCategory->isChecked()) {
  //2nd if: Filter by CATEGORY
    //Find catId for the text on the combobox.
    int catId=-1;
    QString catText = ui_mainview.comboProductsFilterByCategory->currentText();
    if (categoriesHash.contains(catText)) {
      catId = categoriesHash.value(catText);
    }
    productsModel->setFilter(QString("products.category=%1").arg(catId));
    productsModel->setSort(productStockIndex, Qt::DescendingOrder);
  }
  else if (ui_mainview.rbProductsFilterByAvailable->isChecked()) {
  //3rd if: filter by Available items
    productsModel->setFilter(QString("products.stockqty>0"));
    productsModel->setSort(productStockIndex, Qt::DescendingOrder);
  }
  else if (ui_mainview.rbProductsFilterByNotAvailable->isChecked()) {
  //4th if: filter by NOT Available items
    productsModel->setFilter(QString("products.stockqty=0"));
    productsModel->setSort(productSoldUnitsIndex, Qt::DescendingOrder);
  }
  else if (ui_mainview.rbProductsFilterByMostSold->isChecked()) {
  //5th if: filter by Most Sold items
    productsModel->setFilter("");
    productsModel->setSort(productSoldUnitsIndex, Qt::DescendingOrder);
  }
  else if (ui_mainview.rbProductsFilterByAlmostSoldOut->isChecked()) {
    //6th if: filter by ALMOST sold-out items
    productsModel->setFilter(QString("products.stockqty<%1 AND products.stockqty>0").arg(Settings::mostSoldMaxValue()));
    productsModel->setSort(productSoldUnitsIndex, Qt::AscendingOrder);
  }
  else {
  //else: filter by less sold items
    productsModel->setFilter("");
    productsModel->setSort(productSoldUnitsIndex, Qt::AscendingOrder);
  }

  productsModel->select();
 }
}

void squeezeView::setupOffersModel()
{
  offersModel->setTable("offers");
  offersModel->setEditStrategy(QSqlTableModel::OnFieldChange);

  offerIdIndex       = offersModel->fieldIndex("id");
  offerProdIdIndex   = offersModel->fieldIndex("product_id");
  offerDiscountIndex = offersModel->fieldIndex("discount");
  offerDateStartIndex= offersModel->fieldIndex("datestart");
  offerDateEndIndex  = offersModel->fieldIndex("dateend");

  offersModel->setRelation(offerProdIdIndex, QSqlRelation("products", "code", "name"));

  offersModel->setHeaderData(offerIdIndex, Qt::Horizontal, i18n("Id"));
  offersModel->setHeaderData(offerProdIdIndex, Qt::Horizontal, i18n("Product Affected"));
  offersModel->setHeaderData(offerDiscountIndex, Qt::Horizontal, i18n("Discount Applied (%)") );
  offersModel->setHeaderData(offerDateStartIndex, Qt::Horizontal, i18n("Valid from") );
  offersModel->setHeaderData(offerDateEndIndex, Qt::Horizontal, i18n("Valid until") );

  ui_mainview.tableBrowseOffers->setModel(offersModel);
  //QSqlRelationalDelegate *itemOffersDelegate = new QSqlRelationalDelegate(ui_mainview.tableBrowseOffers);
  OffersDelegate *itemOffersDelegate = new OffersDelegate(ui_mainview.tableBrowseOffers);
  ui_mainview.tableBrowseOffers->setItemDelegate(itemOffersDelegate);

  ui_mainview.tableBrowseOffers->setColumnHidden(offerIdIndex, true);

  offersModel->select();
  setOffersFilter();

  ui_mainview.tableBrowseOffers->setSelectionMode(QAbstractItemView::SingleSelection);

//    connect(ui_mainview.tableBrowseOffers->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
//            this, SLOT(offersTableSelectionChanged(QModelIndex)));
//   connect(ui_mainview.tableBrowseOffers->horizontalHeader(), SIGNAL(sectionClicked(int )),
//           this, SLOT(offerTableHeaderClicked(int)));
//   connect(offersModel, SIGNAL(modelReset()), this, SLOT(onOffersModelReset()) );

  ui_mainview.tableBrowseOffers->setCurrentIndex(offersModel->index(0, 0));
}

void squeezeView::setupMeasuresModel()
{
  if (db.isOpen()) {
    measuresModel->setTable("measures");
    measuresModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    measuresModel->setHeaderData(measuresModel->fieldIndex("text"), Qt::Horizontal, i18n("Description"));

    ui_mainview.tableMeasures->setModel(measuresModel);
    ui_mainview.tableMeasures->setSelectionMode(QAbstractItemView::SingleSelection);
    ui_mainview.tableMeasures->setColumnHidden(measuresModel->fieldIndex("id"), true);
    ui_mainview.tableMeasures->setItemDelegate(new QItemDelegate(ui_mainview.tableMeasures));

    measuresModel->select();
    ui_mainview.tableMeasures->setCurrentIndex(measuresModel->index(0, 0));

  }
  else {
      //At this point, what to do?
     // inform to the user about the error and finish app  or retry again some time later?
    QString details = db.lastError().text();
    KMessageBox::detailedError(this, i18n("Squeeze has encountered an error, click details to see the error details."), details, i18n("Error"));
    QTimer::singleShot(10000, this, SLOT(setupMeasuresModel()));
  }
}

void squeezeView::setupCategoriesModel()
{
  if (db.isOpen()) {
    categoriesModel->setTable("categories");
    categoriesModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    categoriesModel->setHeaderData(categoriesModel->fieldIndex("text"), Qt::Horizontal, i18n("Description"));

    ui_mainview.tableCategories->setModel(categoriesModel);
    ui_mainview.tableCategories->setSelectionMode(QAbstractItemView::SingleSelection);
    ui_mainview.tableCategories->setColumnHidden(categoriesModel->fieldIndex("catid"), true);
    ui_mainview.tableCategories->setItemDelegate(new QItemDelegate(ui_mainview.tableCategories));

    categoriesModel->select();
    ui_mainview.tableCategories->setCurrentIndex(categoriesModel->index(0, 0));
    // BFB: Adjust column width to content
    ui_mainview.tableCategories->resizeColumnsToContents();

  }
  else {
      //At this point, what to do?
     // inform to the user about the error and finish app  or retry again some time later?
    QString details = db.lastError().text();
    KMessageBox::detailedError(this, i18n("Squeeze has encountered an error, click details to see the error details."), details, i18n("Error"));
    QTimer::singleShot(10000, this, SLOT(setupMeasuresModel()));
  }
}

void squeezeView::setupClientsModel()
{
  if (db.isOpen()) {
    clientsModel->setTable("clients");
    ui_mainview.clientsView->setViewMode(QListView::IconMode);
    ui_mainview.clientsView->setGridSize(QSize(170,170));
    ui_mainview.clientsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.clientsView->setResizeMode(QListView::Adjust);
    ui_mainview.clientsView->setModel(clientsModel);
    ui_mainview.clientsView->setModelColumn(clientsModel->fieldIndex("photo"));
    ui_mainview.clientsView->setSelectionMode(QAbstractItemView::SingleSelection);

    UsersDelegate *delegate = new UsersDelegate(ui_mainview.clientsView);
    ui_mainview.clientsView->setItemDelegate(delegate);

    clientsModel->select();
    ui_mainview.clientsView->setCurrentIndex(clientsModel->index(0, 0));

  }
  else {
      //At this point, what to do?
     // inform to the user about the error and finish app  or retry again some time later?
    QString details = db.lastError().text();
    KMessageBox::detailedError(this, i18n("Squeeze has encountered an error, click details to see the error details."), details, i18n("Error"));
    QTimer::singleShot(10000, this, SLOT(setupClientsModel()));
  }
}

void squeezeView::setupTransactionsModel()
{
  openDB();
  qDebug()<<"setupTransactions.. after openDB";
  if (db.isOpen()) {
    transactionsModel->setTable("transactions");
    
    transIdIndex = transactionsModel->fieldIndex("id");
    transClientidIndex = transactionsModel->fieldIndex("clientid");
    transTypeIndex= transactionsModel->fieldIndex("type");
    transAmountIndex= transactionsModel->fieldIndex("amount");
    transDateIndex = transactionsModel->fieldIndex("date");
    transTimeIndex= transactionsModel->fieldIndex("time");
    transPaidWithIndex= transactionsModel->fieldIndex("paidwith");
    transChangeGivenIndex= transactionsModel->fieldIndex("changegiven");
    transPayMethodIndex = transactionsModel->fieldIndex("paymethod");
    transStateIndex= transactionsModel->fieldIndex("state");
    transUseridIndex=transactionsModel->fieldIndex("userid");
    transCardNumIndex=transactionsModel->fieldIndex("cardnumber");
    transPointsIndex=transactionsModel->fieldIndex("points");
    transDiscMoneyIndex=transactionsModel->fieldIndex("discmoney");
    transDiscIndex=transactionsModel->fieldIndex("disc");
    transCardAuthNumberIndex=transactionsModel->fieldIndex("cardauthnumber");
    transUtilityIndex=transactionsModel->fieldIndex("utility");
    transTerminalNumIndex=transactionsModel->fieldIndex("terminalnum");
    transItemsListIndex=transactionsModel->fieldIndex("itemslist");
    transItemCountIndex=transactionsModel->fieldIndex("itemcount");
    
    
    ui_mainview.transactionsTable->setModel(transactionsModel);
    ui_mainview.transactionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.transactionsTable->setColumnHidden(transItemsListIndex, true);

    transactionsModel->setRelation(transTypeIndex, QSqlRelation("transactiontypes", "ttypeid", "text"));
    transactionsModel->setRelation(transStateIndex, QSqlRelation("transactionstates", "stateid", "text"));
    transactionsModel->setRelation(transPayMethodIndex, QSqlRelation("paytypes", "typeid", "text"));
    transactionsModel->setRelation(transClientidIndex, QSqlRelation("clients", "id", "name"));
    transactionsModel->setRelation(transUseridIndex, QSqlRelation("users", "id", "username"));
    
    transactionsModel->setHeaderData(transIdIndex, Qt::Horizontal, i18n("Id"));
    transactionsModel->setHeaderData(transClientidIndex, Qt::Horizontal, i18n("Client"));
    transactionsModel->setHeaderData(transTypeIndex, Qt::Horizontal, i18n("Type") );
    transactionsModel->setHeaderData(transAmountIndex, Qt::Horizontal, i18n("Amount") );
    transactionsModel->setHeaderData(transDateIndex, Qt::Horizontal, i18n("Date") );
    transactionsModel->setHeaderData(transTimeIndex, Qt::Horizontal, i18n("Time") );
    transactionsModel->setHeaderData(transPaidWithIndex, Qt::Horizontal, i18n("Paid with") );
    transactionsModel->setHeaderData(transChangeGivenIndex, Qt::Horizontal, i18n("Change Given") );
    transactionsModel->setHeaderData(transPayMethodIndex, Qt::Horizontal, i18n("Pay Method") );
    transactionsModel->setHeaderData(transStateIndex, Qt::Horizontal, i18n("State") );
    transactionsModel->setHeaderData(transUseridIndex, Qt::Horizontal, i18n("Vendor") );
    transactionsModel->setHeaderData(transCardNumIndex, Qt::Horizontal, i18n("Card Num") );
    transactionsModel->setHeaderData(transItemCountIndex, Qt::Horizontal, i18n("Items Count") );
    transactionsModel->setHeaderData(transPointsIndex, Qt::Horizontal, i18n("Points") );
    transactionsModel->setHeaderData(transDiscMoneyIndex, Qt::Horizontal, i18n("Discount %1", KGlobal::locale()->currencySymbol()) );
    transactionsModel->setHeaderData(transDiscIndex, Qt::Horizontal, i18n("Discount %") );
    transactionsModel->setHeaderData(transCardAuthNumberIndex, Qt::Horizontal, i18n("Card Authorization #") );
    transactionsModel->setHeaderData(transUtilityIndex, Qt::Horizontal, i18n("Profit") );
    transactionsModel->setHeaderData(transTerminalNumIndex, Qt::Horizontal, i18n("Terminal #") );
    
    
    ui_mainview.transactionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
//     ProductDelegate *delegate = new ProductDelegate(ui_mainview.productsView);
//     ui_mainview.productsView->setItemDelegate(delegate);
    
    transactionsModel->select();
    
  }
  qDebug()<<"setupTransactions.. done, "<<transactionsModel->lastError();
}

//FIXME: When filtering by User/Client, we need filter by user or username? and just = or with a regexp or a 'like' search??
void squeezeView::setTransactionsFilter()
{
  //NOTE: This is a QT BUG.
  //   If filter by description is selected and the text is empty, and later is re-filtered
  //   then NO pictures are shown; even if is refiltered again.
  QRegExp regexp;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  
  if (!ui_mainview.groupFilterTransactions->isChecked()) transactionsModel->setFilter("");
  else {
    if (ui_mainview.rbTransFilterByUser->isChecked()) {
      //1st if: Filter by Users.
      regexp = QRegExp(ui_mainview.editTransUsersFilter->text());
      if (!regexp.isValid() || ui_mainview.editTransUsersFilter->text().isEmpty())  ui_mainview.editTransUsersFilter->setText("*");
      if (ui_mainview.editTransUsersFilter->text()=="*") transactionsModel->setFilter("");
      else {
        unsigned int uid = myDb->getUserId(ui_mainview.editTransUsersFilter->text());
        transactionsModel->setFilter(QString("transactions.userid=%1").arg(uid));
      }
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByClient->isChecked()) {
      //2nd if: Filter by CLIENTS
      regexp = QRegExp(ui_mainview.editTransClientsFilter->text());
      if (!regexp.isValid() || ui_mainview.editTransClientsFilter->text().isEmpty())  ui_mainview.editTransClientsFilter->setText("*");
      if (ui_mainview.editTransClientsFilter->text()=="*") transactionsModel->setFilter("");
      else {
        unsigned int cid = myDb->getClientId(ui_mainview.editTransClientsFilter->text());
        transactionsModel->setFilter(QString("transactions.clientid=%1").arg(cid));
      }
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByStateFinished->isChecked()) {
      //3rd if: filter by FINISHED TRANSACTIONS
      transactionsModel->setFilter(QString("transactions.state=2")); //tCompleted=2
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByStateCancelled->isChecked()) {
      //4th if: filter by CANCELLED TRANSACTIONS
      transactionsModel->setFilter(QString("transactions.state=3")); //tCancelled=3
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByPaidCash->isChecked()) {
      //5th if: filter by PAID IN CASH
      transactionsModel->setFilter("transactions.paymethod=1 and transactions.state=2"); // paid in cash and finished
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByPaidCredit->isChecked()) {
      //6th if: filter by PAID WITH CARD
      transactionsModel->setFilter("transactions.paymethod=2 and transactions.state=2"); //paid with card and finished only
      transactionsModel->setSort(transIdIndex, Qt::AscendingOrder);
    }
    else if (ui_mainview.rbTransFilterByDate->isChecked()) {
      //7th if: filter by DATE
      QDate date = ui_mainview.transactionsDateEditor->date();
      transactionsModel->setFilter(QString("transactions.date = '%1'").arg(date.toString("yyyy-MM-dd")));
      qDebug()<<"Filtro:"<<transactionsModel->filter();
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransFilterByAmountLess->isChecked()) {
      //6th if: filter by AMOUNT <
      double amo = ui_mainview.editTransAmountLess->value();
      transactionsModel->setFilter(QString("transactions.amount<%1").arg(amo));
      transactionsModel->setSort(transIdIndex, Qt::AscendingOrder);
    }
    else if (ui_mainview.rbTransFilterByAmountGreater->isChecked()) {
      //6th if: filter by AMOUNT >
      double amo = ui_mainview.editTransAmountGreater->value();
      transactionsModel->setFilter(QString("transactions.amount>%1").arg(amo));
      transactionsModel->setSort(transIdIndex, Qt::AscendingOrder);
    }
    //NOTE: in the next 3 ifs, transactions.type=X is hardcoded... I assume the user did not change the default values.
    else if (ui_mainview.rbTransactionsFilterOnlySales->isChecked()) {
      transactionsModel->setFilter("transactions.type=1");
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransactionsFilterOnlyPurchases->isChecked()) {
      transactionsModel->setFilter("transactions.type=2");
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbTransactionsFilterOnlyChangesReturns->isChecked()) {
      transactionsModel->setFilter("transactions.type=3 OR transactions.type=4");
      transactionsModel->setSort(transIdIndex, Qt::DescendingOrder);
    }
    else {
      //else: filter by terminal number
      unsigned int tnum = ui_mainview.editTransTermNum->value();
      transactionsModel->setFilter(QString("transactions.terminalnum=%1").arg(tnum));
      transactionsModel->setSort(transIdIndex, Qt::AscendingOrder);
    }
    
    transactionsModel->select();
  }
}


// BALANCES

void squeezeView::setupBalancesModel()
{
  openDB();
  qDebug()<<"setupBalances.. after openDB";
  if (db.isOpen()) {
    balancesModel->setTable("balances");
    
    balanceIdIndex = balancesModel->fieldIndex("id");
    balanceDateEndIndex = balancesModel->fieldIndex("datetime_end"); //just one date...
    balanceUserNameIndex= balancesModel->fieldIndex("usern");
    balanceInitAmountIndex= balancesModel->fieldIndex("initamount");
    balanceInIndex = balancesModel->fieldIndex("in");
    balanceOutIndex= balancesModel->fieldIndex("out");
    balanceCashIndex= balancesModel->fieldIndex("cash");
    balanceCardIndex= balancesModel->fieldIndex("card");
    balanceTransIndex = balancesModel->fieldIndex("transactions");
    balanceTerminalNumIndex= balancesModel->fieldIndex("terminalnum");
    balanceDateStartIndex=balancesModel->fieldIndex("datetime_start"); //to hide
    balanceUseridIndex=balancesModel->fieldIndex("userid"); //to hide
    
    
    ui_mainview.balancesTable->setModel(balancesModel);
    ui_mainview.balancesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_mainview.balancesTable->setColumnHidden(balanceDateStartIndex, true);
    ui_mainview.balancesTable->setColumnHidden(balanceUseridIndex, true);
    
    balancesModel->setHeaderData(balanceIdIndex, Qt::Horizontal, i18n("Id"));
    balancesModel->setHeaderData(balanceDateEndIndex, Qt::Horizontal, i18n("Date"));
    balancesModel->setHeaderData(balanceUserNameIndex, Qt::Horizontal, i18n("Vendor") );
    balancesModel->setHeaderData(balanceInitAmountIndex, Qt::Horizontal, i18n("Initial amount") );
    balancesModel->setHeaderData(balanceInIndex, Qt::Horizontal, i18n("In") );
    balancesModel->setHeaderData(balanceOutIndex, Qt::Horizontal, i18n("Out") );
    balancesModel->setHeaderData(balanceCashIndex, Qt::Horizontal, i18n("Cash in drawer") );
    balancesModel->setHeaderData(balanceCardIndex, Qt::Horizontal, i18n("Received by Card") );
    balancesModel->setHeaderData(balanceTransIndex, Qt::Horizontal, i18n("Transactions") );
    balancesModel->setHeaderData(balanceTerminalNumIndex, Qt::Horizontal, i18n("Terminal #") );
    
    
    ui_mainview.balancesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    //     ProductDelegate *delegate = new ProductDelegate(ui_mainview.productsView);
    //     ui_mainview.productsView->setItemDelegate(delegate);
    
    balancesModel->select();
  }
  qDebug()<<"setupBalances.. done.";
}

//FIXME: When filtering by User, we need filter by user or username? and just = or with a regexp or a 'like' search??
void squeezeView::setBalancesFilter()
{
  //NOTE: This is a QT BUG.
  //   If filter by description is selected and the text is empty, and later is re-filtered
  //   then NO pictures are shown; even if is refiltered again.
  
  if (!ui_mainview.groupFilterBalances->isChecked()) balancesModel->setFilter("");
  else {
    if (ui_mainview.rbBalancesFilterByState->isChecked()) {
      //1st if: Filter by NOT EMPTY Transactions on balances
      balancesModel->setFilter(QString("balances.transactions!='EMPTY'"));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbBalancesFilterBySuspicious->isChecked()) {
      //2nd if: Filter by Suspicious balances
      balancesModel->setFilter(QString("balances.initamount+balances.in-balances.out!=balances.cash"));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbBalancesFilterByDate->isChecked()) {
      //3rd if: filter by DATE
      QDate date = ui_mainview.editBalancesFilterByDate->date();
      QDateTime dt = QDateTime(date); //time 00:00:00
      QString dtStr = dt.toString("yyyy-MM-dd hh:mm:ss");
      QString dtStr2= date.toString("yyyy-MM-dd")+" 23:59:59";
      balancesModel->setFilter(QString("balances.datetime_end>='%1' and balances.datetime_end<='%2'").arg(dtStr).arg(dtStr2));
      qDebug()<<"Filtro:"<<balancesModel->filter();
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbBalancesFilterByCashInLess->isChecked()) {
      //4th if: filter by CASH IN <
      double amo = ui_mainview.editBalancesFilterByCasInLess->value();
      balancesModel->setFilter(QString("balances.in<%1").arg(amo));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbBalancesFilterByCashInGrater->isChecked()) {
      //5th if: filter by CASH IN >
      double csh = ui_mainview.editBalancesFilterByCashInGrater->value();
      balancesModel->setFilter(QString("balances.in>%1").arg(csh));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else if (ui_mainview.rbBalancesFilterByUser->isChecked()) {
      //6th if: filter by vendor
      balancesModel->setFilter(QString("balances.usern='%1'").arg(ui_mainview.editBalancesFilterByVendor->text()));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    else {
      //7th else: filter by terminal number
      unsigned int tnum = ui_mainview.editBalancesFilterByTermNum->value();
      balancesModel->setFilter(QString("balances.terminalnum=%1").arg(tnum));
      balancesModel->setSort(balanceIdIndex, Qt::DescendingOrder);
    }
    
    balancesModel->select();
  }
}


/* widgets */

void squeezeView::settingsChanged()
{
  emit signalChangeStatusbar( i18n("Settings changed") );
  
  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());
  
  //setupDb();
  connectToDb();
}

void squeezeView::settingsChangedOnInitConfig()
{
  qDebug()<<"==> Initial Config Changed- connecting to database and calling login...";
  //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  //db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName(Settings::editDBServer());
  db.setDatabaseName(Settings::editDBName());
  db.setUserName(Settings::editDBUsername());
  db.setPassword(Settings::editDBPassword());
  
  connectToDb();
  login();
}

void squeezeView::setOffersFilter()
{
  if (ui_mainview.groupFilterOffers->isChecked()) {
    if (ui_mainview.chOffersFilterByProduct->isChecked()) {
      ui_mainview.editOffersFilterByProduct->setFocus();
      //Get codes and names from offers and products
      QString myFilter;
      QStringList codes;
      QString desc;
      if (ui_mainview.editOffersFilterByProduct->text().isEmpty()) desc = "."; else desc =ui_mainview.editOffersFilterByProduct->text();

      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      myFilter = myDb->getOffersFilterWithText(desc);

      if (myFilter == "") myFilter = "offers.product_id=0"; //there should not be a product with code=0
      offersModel->setFilter(myFilter);
    }
    else if (ui_mainview.chOffersTodayDiscounts->isChecked()) {
        //Today Offers
        QDate date = QDate::currentDate();
        QString today = date.toString("yyyy-MM-dd");
        offersModel->setFilter(QString(" offers.datestart <= '%1' and offers.dateend >='%1' ").arg(today));
        //offers.datestart between '%1' and '%2' or offers.dateend between %3 and %4
        qDebug()<<"Filtro:"<<offersModel->filter();
    }
    else if (ui_mainview.chOffersSelectDate->isChecked()) {
        //Selected Date Offers
        QDate date = ui_mainview.offersDateEditor->date();
//         offersModel->setFilter(QString("offers.dateend='%1'").arg());
        offersModel->setFilter(QString(" offers.datestart <= '%1' and offers.dateend >='%1' ").arg(date.toString("yyyy-MM-dd")));
        qDebug()<<"Filtro:"<<offersModel->filter();
    }
    else { //old offers, non valid anymore...
        QDate date = QDate::currentDate();
        offersModel->setFilter(QString("offers.dateend<'%1'").arg(date.toString("yyyy-MM-dd")));
        qDebug()<<"Filtro: "<<offersModel->filter();
    }
    //Faltarian las ofertas aun no validas (futuras)
  }
  else offersModel->setFilter(""); //show all offers...
  offersModel->select();
}


void squeezeView::usersViewOnSelected(const QModelIndex & index)
{
 if (db.isOpen()) {
    //getting data from model...
    const QAbstractItemModel *model = index.model();
    int row = index.row();
    QModelIndex indx = model->index(row, usersModel->fieldIndex("id"));
    int id = model->data(indx, Qt::DisplayRole).toInt();
    indx = model->index(row, usersModel->fieldIndex("username"));
    QString username = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("name"));
    QString name = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("address"));
    QString address = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("phone"));
    QString phone = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("phone_movil"));
    QString cell = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("photo"));
    QByteArray photoBA = model->data(indx, Qt::DisplayRole).toByteArray();
    indx = model->index(row, usersModel->fieldIndex("password"));
    QString oldPassword = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, usersModel->fieldIndex("salt"));
    QString oldSalt = model->data(indx, Qt::DisplayRole).toString();

    QPixmap photo;
    photo.loadFromData(photoBA);
    UserInfo uInfo;

    //Launch Edit dialog
    UserEditor *userEditorDlg = new UserEditor(this);
    //Set data on dialog
    userEditorDlg->setId(id);
    userEditorDlg->setUserName(username);
    userEditorDlg->setRealName(name);
    userEditorDlg->setAddress(address);
    userEditorDlg->setPhone(phone);
    userEditorDlg->setCell(cell);
    userEditorDlg->setPhoto(photo);

    if (userEditorDlg->exec() ) {
      uInfo.id = id;
      uInfo.username = userEditorDlg->getUserName();
      uInfo.name     = userEditorDlg->getRealName();
      uInfo.address  = userEditorDlg->getAddress();
      uInfo.phone    = userEditorDlg->getPhone();
      uInfo.cell     = userEditorDlg->getCell();
      photo    = userEditorDlg->getPhoto();

      uInfo.photo = Misc::pixmap2ByteArray(new QPixmap(photo));

      //Password
      if (!userEditorDlg->getNewPassword().isEmpty()) {
        QByteArray saltBA = Hash::getSalt();
        uInfo.salt = QString(saltBA);
        QString pswdTmp = uInfo.salt+userEditorDlg->getNewPassword();
        QByteArray passwdBA = pswdTmp.toLocal8Bit();
        uInfo.password = Hash::password2hash(passwdBA);
      }
      else {
        uInfo.password = oldPassword;
        uInfo.salt = oldSalt;
      }

      //Modify data on mysql...
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      if (!myDb->updateUser(uInfo)) qDebug()<<"ERROR | Updating user:"<<myDb->lastError();

      usersModel->select();
    }
    delete userEditorDlg;
  }
}


void squeezeView::productsViewOnSelected(const QModelIndex &index)
{
  ProductInfo pInfo;
  
 if (db.isOpen()) {
    //getting data from model...
    const QAbstractItemModel *model = index.model();
    int row = index.row();
    QModelIndex indx = model->index(row, productsModel->fieldIndex("code"));
    qulonglong id = model->data(indx, Qt::DisplayRole).toULongLong();

    //Launch Edit dialog
    ProductEditor *productEditorDlg = new ProductEditor(this, false);

    //Set data on dialog
    productEditorDlg->disableCode(); //On Edit product, code cannot be changed.
    productEditorDlg->setDb(db);
    productEditorDlg->setCode(id); //this method get all data for such product code.
    qulonglong newcode=0;

    //Launch dialog, and if dialog is accepted...
    if (productEditorDlg->exec() ) {
      //get changed|unchanged values
      pInfo = productEditorDlg->getProductInfo();
      newcode = pInfo.code; //to check if code change...
      //Update database
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      if (!myDb->updateProduct(pInfo, id)) qDebug()<<"Product update error:"<<myDb->lastError();
      // Check offers, to move or delete them.
      if (id != newcode) {
        if (!myDb->moveOffer(id, newcode)) qDebug()<<"Offer update error:"<<myDb->lastError();
      }
      //FIXME: We must see error types, which ones are for duplicate KEYS (codes) to advertise the user.
      productsModel->select();
    }
    delete productEditorDlg;
  }
}

void squeezeView::clientsViewOnSelected(const QModelIndex & index)
{
  if (db.isOpen()) {
    //getting data from model...
    const QAbstractItemModel *model = index.model();
    int row = index.row();
    QModelIndex indx = model->index(row, clientsModel->fieldIndex("id"));
    int id = model->data(indx, Qt::DisplayRole).toInt();
    indx = model->index(row, clientsModel->fieldIndex("name"));
    QString name = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, clientsModel->fieldIndex("address"));
    QString address = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, clientsModel->fieldIndex("phone"));
    QString phone = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, clientsModel->fieldIndex("phone_movil"));
    QString cell = model->data(indx, Qt::DisplayRole).toString();
    indx = model->index(row, clientsModel->fieldIndex("points"));
    qulonglong points = model->data(indx, Qt::DisplayRole).toULongLong();
    indx = model->index(row, clientsModel->fieldIndex("discount"));
    double discount = model->data(indx, Qt::DisplayRole).toDouble();
    indx = model->index(row, clientsModel->fieldIndex("photo"));
    QByteArray photoBA = model->data(indx, Qt::DisplayRole).toByteArray();

    ClientInfo cInfo;
    QPixmap photo;
    photo.loadFromData(photoBA);

    //Launch Edit dialog
    ClientEditor *clientEditorDlg = new ClientEditor(this);
    //Set data on dialog
    clientEditorDlg->setId(id);
    clientEditorDlg->setName(name);
    clientEditorDlg->setAddress(address);
    clientEditorDlg->setPhone(phone);
    clientEditorDlg->setCell(cell);
    clientEditorDlg->setPhoto(photo);
    clientEditorDlg->setPoints(points);
    clientEditorDlg->setDiscount(discount);

    if (clientEditorDlg->exec() ) {
      cInfo.id       = id;
      cInfo.name     = clientEditorDlg->getName();
      cInfo.address  = clientEditorDlg->getAddress();
      cInfo.phone    = clientEditorDlg->getPhone();
      cInfo.cell     = clientEditorDlg->getCell();
      photo          = clientEditorDlg->getPhoto();
      cInfo.points   = clientEditorDlg->getPoints();
      cInfo.discount = clientEditorDlg->getDiscount();
      cInfo.since    = QDate::currentDate();

      cInfo.photo    = Misc::pixmap2ByteArray(new QPixmap(photo));

      //Modify data on mysql...
      if (!db.isOpen()) openDB();
      QSqlQuery query(db);
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      myDb->updateClient(cInfo);

      clientsModel->select();
    }
    delete clientEditorDlg;
  }
}

void squeezeView::doPurchase()
{
  if (db.isOpen()) {
    QStringList items;
    items.clear();

    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);

    qDebug()<<"doPurchase...";
    PurchaseEditor *purchaseEditorDlg = new PurchaseEditor(this);
    purchaseEditorDlg->setDb(db);
    if (purchaseEditorDlg->exec()) {
      QHash<qulonglong, ProductInfo> hash = purchaseEditorDlg->getHash();
      ProductInfo info;
      //Iterate the hash
      QHashIterator<qulonglong, ProductInfo> i(hash);
      while (i.hasNext()) {
        i.next();
        info = i.value();
        double oldstockqty = info.stockqty;
        info.stockqty = info.purchaseQty+oldstockqty;
        //Modify data on mysql...
        if (!db.isOpen()) openDB();
        QSqlQuery query(db);
        //validDiscount is for checking if product already exists on db. see line # 383 of purchaseeditor.cpp
        if (info.validDiscount) {
          if (!myDb->updateProduct(info, info.code)) qDebug()<<myDb->lastError();
          qDebug()<<"product already on db...";
        } else if (!myDb->insertProduct(info)) qDebug()<<myDb->lastError();

        productsModel->select();
        items.append(QString::number(info.code)+"/"+QString::number(info.purchaseQty));
        }
        //Now add a transaction for buy
        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();
        TransactionInfo tInfo;
        tInfo.type    = tBuy;
        tInfo.amount  = purchaseEditorDlg->getTotalBuy();
        tInfo.date    = date;
        tInfo.time    = time;
        tInfo.paywith = 0.0;
        tInfo.changegiven = 0.0;
        tInfo.paymethod   = pCash;
        tInfo.state   = tCompleted;
        tInfo.userid  = 1;
        tInfo.clientid= 1; //FIXME for 0.8+ version!!! Put here the provider id.. but relations?????
        tInfo.cardnumber  = "-NA-";
        tInfo.cardauthnum = "-NA-";
        tInfo.itemcount   = purchaseEditorDlg->getItemCount();
        tInfo.itemlist    = items.join(";");
        tInfo.profit      = 0; //FIXME: utility is calculated until products are sold, not before.
        tInfo.terminalnum = 0; //NOTE: Not really a terminal... from admin computer.
        myDb->insertTransaction(tInfo);

    }
  }
}

void squeezeView::createUser()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  UserInfo info;

  if (!db.isOpen()) openDB();
  if (db.isOpen()) {
    UserEditor *userEditorDlg = new UserEditor(this);
    QPixmap photo;

    if (userEditorDlg->exec() ) {
      info.username = userEditorDlg->getUserName();
      info.name     = userEditorDlg->getRealName();
      info.address  = userEditorDlg->getAddress();
      info.phone    = userEditorDlg->getPhone();
      info.cell     = userEditorDlg->getCell();
      photo    = userEditorDlg->getPhoto();
      info.photo = Misc::pixmap2ByteArray(new QPixmap(photo));

      QByteArray saltBA = Hash::getSalt();
      info.salt = QString(saltBA);
      QString pswdTmp = info.salt+userEditorDlg->getNewPassword();
      QByteArray passwdBA = pswdTmp.toLocal8Bit();
      info.password = Hash::password2hash(passwdBA);

      if (!myDb->insertUser(info)) qDebug()<<myDb->lastError();

      usersModel->select();
    }
    delete userEditorDlg;
  }
}

void squeezeView::createOffer()
{
  if (db.isOpen()) {
    PromoEditor *promoEditorDlg = new PromoEditor(this);
    promoEditorDlg->setDb(db);
    if (promoEditorDlg->exec() ) {

      QDate dateStart = promoEditorDlg->getDateStart();
      QDate dateEnd = promoEditorDlg->getDateEnd();

      OfferInfo offerInfo;
      offerInfo.productCode = promoEditorDlg->getSelectedProductCode();
      offerInfo.discount = promoEditorDlg->getDiscount();
      offerInfo.dateStart = dateStart;
      offerInfo.dateEnd   = dateEnd;

      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      if ( !myDb->createOffer(offerInfo) ) qDebug()<<myDb->lastError();
      offersModel->select();
    }
    delete promoEditorDlg;
  }
}

void squeezeView::createProduct()
{
 if (db.isOpen()) {
  ProductEditor *prodEditorDlg = new ProductEditor(this, true);
  prodEditorDlg->setDb(db);
  prodEditorDlg->enableCode();

  if (prodEditorDlg->exec()) {
    int resultado = prodEditorDlg->result();

    Azahar *myDb = new Azahar;
    myDb->setDatabase(db);
    ProductInfo info;

    //The next is for the prodEditorDlg new feature: When adding a new product, if entered code exists, it will be edited. to save time...
    switch (resultado) {
      case QDialog::Accepted:
      case statusNormal:
        info = prodEditorDlg->getProductInfo();
        if (!myDb->insertProduct(info)) qDebug()<<"ERROR:"<<myDb->lastError();
        productsModel->select();
      break;
      case statusMod: //Here is not allowed to modify a product... just create new ones...
      break;
      case QDialog::Rejected:
      default:
      break;
    }
  }
 }
}

void squeezeView::createMeasure()
{
 if (db.isOpen()) {
//     int row = ui_mainview.tableMeasures->currentIndex().row();
//     if (row==-1) row=0;
//     if (ui_mainview.stackedWidget->currentIndex() != pBrowseMeasures)
//       ui_mainview.stackedWidget->setCurrentIndex(pBrowseMeasures);
//     if (measuresModel->tableName().isEmpty()) setupMeasuresModel();
//
//     measuresModel->insertRow(row);
  bool ok=false;
  QString meas = QInputDialog::getText(this, i18n("New Weight or Measure"), i18n("Enter the new weight or measure to insert:"),
                                     QLineEdit::Normal, "", &ok );
  if (ok && !meas.isEmpty()) {
    Azahar *myDb = new Azahar;
    if (!db.isOpen()) openDB();
    myDb->setDatabase(db);
    if (!myDb->insertMeasure(meas)) qDebug()<<"Error:"<<myDb->lastError();
    measuresModel->select();
  }
 }
}

void squeezeView::createCategory()
{
  if (db.isOpen()) {
  bool ok=false;
  QString cat = QInputDialog::getText(this, i18n("New category"), i18n("Enter the new category to insert:"),
                                     QLineEdit::Normal, "", &ok );
  if (ok && !cat.isEmpty()) {
    Azahar *myDb = new Azahar;
    if (!db.isOpen()) openDB();
    myDb->setDatabase(db);
    if (!myDb->insertCategory(cat)) qDebug()<<"Error:"<<myDb->lastError();
    categoriesModel->select();
    updateCategoriesCombo();
  }
 }
}

void squeezeView::updateCategoriesCombo()
{
  populateCategoriesHash();
  ui_mainview.comboProductsFilterByCategory->clear();
  QHashIterator<QString, int> item(categoriesHash);
  while (item.hasNext()) {
    item.next();
    ui_mainview.comboProductsFilterByCategory->addItem(item.key());
  }
}

void squeezeView::createClient()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);

  if (db.isOpen()) {
    ClientEditor *clientEditorDlg = new ClientEditor(this);
    ClientInfo info;
    QPixmap photo;

    if (clientEditorDlg->exec() ) {
      info.name     = clientEditorDlg->getName();
      info.address  = clientEditorDlg->getAddress();
      info.phone    = clientEditorDlg->getPhone();
      info.cell     = clientEditorDlg->getCell();
      photo    = clientEditorDlg->getPhoto();
      info.points   = clientEditorDlg->getPoints();
      info.discount = clientEditorDlg->getDiscount();
      info.since    = QDate::currentDate();

      info.photo = Misc::pixmap2ByteArray(new QPixmap(photo));
      if (!db.isOpen()) openDB();
      if (!myDb->insertClient(info)) qDebug()<<myDb->lastError();

      clientsModel->select();
    }
    delete clientEditorDlg;
  }
}

void squeezeView::deleteSelectedClient()
{
  if (db.isOpen()) {
    QModelIndex index = ui_mainview.clientsView->currentIndex();
    if (clientsModel->tableName().isEmpty()) setupClientsModel();
    if (index == clientsModel->index(-1,-1) ) {
      KMessageBox::information(this, i18n("Please select a client to delete, then press the delete button again."), i18n("Cannot delete"));
    }
    else  {
      QString uname = clientsModel->record(index.row()).value("name").toString();
      qulonglong clientId = clientsModel->record(index.row()).value("id").toULongLong();
      if (clientId > 1) {
        int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the client named %1?",uname),
                                              i18n("Delete"));
        if (answer == KMessageBox::Yes) {
          clientsModel->removeRow(index.row());
          clientsModel->submitAll();
          clientsModel->select();
        }
    } else KMessageBox::information(this, i18n("Default client cannot be deleted."), i18n("Cannot delete"));
   }
 }
}

void squeezeView::deleteSelectedUser()
{
  if (db.isOpen()) {
    QModelIndex index = ui_mainview.usersView->currentIndex();
    if (usersModel->tableName().isEmpty()) setupUsersModel();
    if (index == usersModel->index(-1,-1) ) {
      KMessageBox::information(this, i18n("Please select a user to delete, then press the delete button again."), i18n("Cannot delete"));
      //TODO: Present a dialog to select which user to delete...
    }
    else  {
      QString uname = usersModel->record(index.row()).value("name").toString();
      QString usr = usersModel->record(index.row()).value("username").toString();
      if (usr != "admin")
      {
        int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the user named %1?",uname),
                                              i18n("Delete"));
        if (answer == KMessageBox::Yes) {
          usersModel->removeRow(index.row());
          usersModel->submitAll();
          usersModel->select();
        }
      } else KMessageBox::information(this, i18n("Admin user cannot be deleted."), i18n("Cannot delete"));
   }
 }
}

void squeezeView::deleteSelectedOffer()
{
  if (db.isOpen()) {
    QModelIndex index = ui_mainview.tableBrowseOffers->currentIndex();
    if (offersModel->tableName().isEmpty()) setupOffersModel();
    if (index == offersModel->index(-1,-1) ) {
      //NOTE: Hey, I think the word "offer" does not mean what i mean...
      KMessageBox::information(this, i18n("Please select an offer to delete, then press the delete button again."), i18n("Cannot delete"));
      //NOTE: Present a dialog to select which user to delete?...
    }
    else  {
      int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selected discount?"),
                                              i18n("Delete"));
      if (answer == KMessageBox::Yes) {
        //same weird error when deleting offers that the products! :S
        Azahar *myDb = new Azahar;
        myDb->setDatabase(db);
        qulonglong  code = offersModel->record(index.row()).value("id").toULongLong();
        if (!offersModel->removeRow(index.row(), index)) myDb->deleteOffer(code);
        offersModel->submitAll();
        offersModel->select();
      }
    }
  }
}

void squeezeView::deleteSelectedProduct()
{
  if (db.isOpen()) {
    QModelIndex index;
    if (ui_mainview.productsView->isHidden()) {
      index = ui_mainview.productsViewAlt->currentIndex();
    } else {
      index = ui_mainview.productsView->currentIndex();
    }
    
    if (productsModel->tableName().isEmpty()) setupProductsModel();
    if (index == productsModel->index(-1,-1) ) {
      KMessageBox::information(this, i18n("Please select a product to delete, then press the delete button."), i18n("Cannot delete"));
    }
    else  {
      int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the selected product?"),
                                              i18n("Delete"));
      if (answer == KMessageBox::Yes) {
        Azahar *myDb = new Azahar;
        myDb->setDatabase(db);
        //first we obtain the product code to be deleted.
        qulonglong  iD = productsModel->record(index.row()).value("code").toULongLong();
        if (!productsModel->removeRow(index.row(), index)) {
          // weird:  since some time, removeRow does not work... it worked fine on versions < 0.9 ..
          bool d = myDb->deleteProduct(iD); qDebug()<<"Deleteing product ("<<iD<<") manually...";
          if (d) qDebug()<<"Deletion succed...";
        }
        productsModel->submitAll();
        productsModel->select();
        //We must delete the product's offers also.
        //in case of multiple offers for a product
        qulonglong oID = myDb->getProductOfferCode(iD);
        while (oID != 0) {
          qDebug()<<"DELETING product code:"<<iD<<" offer code:"<<oID;
          qulonglong oID = myDb->getProductOfferCode(iD);
          if (myDb->deleteOffer(oID)) qDebug()<<"Ok, offer also deleted...";
          else qDebug()<<"DEBUG:"<<myDb->lastError();
        }
      }
    }
  }
}

void squeezeView::deleteSelectedMeasure()
{
  if (db.isOpen()) {
    QModelIndex index = ui_mainview.tableMeasures->currentIndex();
    if (measuresModel->tableName().isEmpty()) setupMeasuresModel();
    if (index == measuresModel->index(-1,-1) ) {
      KMessageBox::information(this, i18n("Please select a row to delete, then press the delete button again."), i18n("Cannot delete"));
    }
    else  {
      QString measureText = measuresModel->record(index.row()).value("text").toString();
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      qulonglong measureId = myDb->getMeasureId(measureText);
      if (measureId > 1) {
        int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the measure '%1'?", measureText),
                                                i18n("Delete"));
        if (answer == KMessageBox::Yes) {
          measuresModel->removeRow(index.row());
          measuresModel->submitAll();
          measuresModel->select();
        }
      } else KMessageBox::information(this, i18n("Default measure cannot be deleted."), i18n("Cannot delete"));
    }
  }
}

void squeezeView::deleteSelectedCategory()
{
  if (db.isOpen()) {
    QModelIndex index = ui_mainview.tableCategories->currentIndex();
    if (categoriesModel->tableName().isEmpty()) setupCategoriesModel();
    if (index == offersModel->index(-1,-1) ) {
      KMessageBox::information(this, i18n("Please select a category to delete, then press the delete button again."), i18n("Cannot delete"));
    }
    else  {
      QString catText = categoriesModel->record(index.row()).value("text").toString();
      Azahar *myDb = new Azahar;
      myDb->setDatabase(db);
      qulonglong catId = myDb->getCategoryId(catText);
      if (catId >0) {
        int answer = KMessageBox::questionYesNo(this, i18n("Do you really want to delete the category '%1'?", catText),
                                                i18n("Delete"));
        if (answer == KMessageBox::Yes) {
          categoriesModel->removeRow(index.row());
          categoriesModel->submitAll();
          categoriesModel->select();
          updateCategoriesCombo();
        }
      } else KMessageBox::information(this, i18n("Default category cannot be deleted."), i18n("Cannot delete"));
    }
  }
}


//CASH OUTS
void squeezeView::setupCashFlowModel()
{
  openDB();
  qDebug()<<"setupcashflow.. after openDB";
  if (db.isOpen()) {
    cashflowModel->setTable("cashflow");
    
    cashflowIdIndex = cashflowModel->fieldIndex("id");
    cashflowTypeIndex = cashflowModel->fieldIndex("type");
    cashflowDateIndex = cashflowModel->fieldIndex("date");
    cashflowTimeIndex= cashflowModel->fieldIndex("time");
    cashflowUseridIndex= cashflowModel->fieldIndex("userid");
    cashflowReasonIndex = cashflowModel->fieldIndex("reason");
    cashflowAmountIndex= cashflowModel->fieldIndex("amount");
    cashflowTerminalNumIndex= cashflowModel->fieldIndex("terminalnum");
    
    
    ui_mainview.cashFlowTable->setModel(cashflowModel);
    ui_mainview.cashFlowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    cashflowModel->setHeaderData(cashflowIdIndex, Qt::Horizontal, i18n("Id"));
    cashflowModel->setHeaderData(cashflowTypeIndex, Qt::Horizontal, i18n("Type"));
    cashflowModel->setHeaderData(cashflowDateIndex, Qt::Horizontal, i18n("Date"));
    cashflowModel->setHeaderData(cashflowUseridIndex, Qt::Horizontal, i18n("Vendor") );
    cashflowModel->setHeaderData(cashflowTimeIndex, Qt::Horizontal, i18n("Time") );
    cashflowModel->setHeaderData(cashflowReasonIndex, Qt::Horizontal, i18n("Reason") );
    cashflowModel->setHeaderData(cashflowAmountIndex, Qt::Horizontal, i18n("Amount") );
    cashflowModel->setHeaderData(cashflowTerminalNumIndex, Qt::Horizontal, i18n("Terminal Num.") );
    
    cashflowModel->setRelation(cashflowUseridIndex, QSqlRelation("users", "id", "username"));
    cashflowModel->setRelation(cashflowTypeIndex, QSqlRelation("cashflowtypes", "typeid", "text"));
    
    ui_mainview.cashFlowTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    cashflowModel->select();
    //qDebug()<<"Cashouts ERROR:"<<cashflowModel->lastError();
  }
  qDebug()<<"setupCashFlow.. done, "<<cashflowModel->lastError();
}

void squeezeView::exportTable()
{
  switch(ui_mainview.stackedWidget->currentIndex()){
    case pBrowseProduct: exportQTableView(ui_mainview.productsViewAlt);break;
    case pBrowseOffers: exportQTableView(ui_mainview.tableBrowseOffers);break;
    case pBrowseUsers: exportQTableView(ui_mainview.usersView);break;
    case pBrowseMeasures: exportQTableView(ui_mainview.tableMeasures);break;
    case pBrowseCategories: exportQTableView(ui_mainview.tableCategories);break;
    case pBrowseClients: exportQTableView(ui_mainview.clientsView);break;
    case pBrowseTransactions: exportQTableView(ui_mainview.transactionsTable);break;
    case pBrowseBalances: exportQTableView(ui_mainview.balancesTable);break;
    case pBrowseCashFlow: exportQTableView(ui_mainview.cashFlowTable);break;
    //case pCustomReports: exportQTableView(ui_mainview.customReportsView);break;
    default:break;
  }
}

void squeezeView::exportQTableView(QAbstractItemView *tableview)
{
  if (tableview->model()){
    const QAbstractItemModel *model = tableview->model();
    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save As"),"",i18n("CSV files (*.csv)"));
    if (fileName != ""){
      QFile file(fileName);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
      QTextStream out(&file);
      
      // Headers
      for (int j=0;j<model->columnCount();j++){
        out << "\"" << model->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString() << "\";";
      }
      out << "\n";
      
      // Data
      QProgressDialog progress(i18n("Exporting data..."), i18n("Abort"), 0, model->rowCount(), this);
      progress.setWindowModality(Qt::WindowModal);
      // If there're more than 1 row selected, then export only this rows
      QModelIndexList selected = tableview->selectionModel()->selectedRows();
      if (selected.count()>1){
        for (int i=0;i<selected.count();i++){
          for (int j=0;j<model->columnCount();j++){
            out << "\"" << model->data(model->index(selected.at(i).row(), j)).toString() << "\";";
          }
          out <<"\n";
        }
      }else{
        // export everything in the model
        for (int i=0;i<model->rowCount();i++){
          progress.setValue(i);
          if (progress.wasCanceled())
            break;
          for (int j=0;j<model->columnCount();j++){
            out << "\"" << model->data(model->index(i, j)).toString() << "\";";
          }
          out <<"\n";
        }
      }
      file.close();
      progress.setValue(model->rowCount());
      //if (KMessageBox::questionYesNo(this, i18n("Data exported succesfully to %1.\n\n Would you like to open it?").arg(fileName), i18n("Finished")) == KMessageBox::Yes ){
 //  system(QString("oocalc \""+fileName+ "\"").toLatin1());
    }
  }
}


#include "squeezeview.moc"
