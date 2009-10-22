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

#ifndef SQUEEZEVIEW_H
#define SQUEEZEVIEW_H

#include <QWidget>
#include <QtSql>

#include "ui_squeezeview_base.h"
#include "../../src/loginwindow.h"

class QPainter;
class PieChart;
class LoginWindow;
class KPlotObject;

/**
 * This is the main view class for squeeze.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * This squeeze uses an HTML component as an example.
 *
 * @short Main view
 * @author Miguel Chavez Gamboa <miguel@lemonpos.org>
 * @version 0.1
 */
class squeezeView : public QWidget, public Ui::squeezeview_base
{
    Q_OBJECT
public:
    squeezeView(QWidget *parent);
    virtual ~squeezeView();
    bool isConnected() { return db.isOpen(); };
    bool modelsAreCreated() { return modelsCreated; };
    void closeDB();
    void openDB();

  private:
    Ui::squeezeview_base ui_mainview;
    QString activeDb;
    QSqlDatabase db;
    bool adminIsLogged;
    LoginWindow *dlgPassword;
    QHash<QString, int> categoriesHash;
    QSqlRelationalTableModel *productsModel;
    QSqlRelationalTableModel *offersModel;
    QSqlRelationalTableModel *cashflowModel;
    QSqlTableModel *usersModel;
    QSqlTableModel *measuresModel;
    QSqlTableModel *categoriesModel;
    QSqlTableModel *balancesModel;
    QSqlTableModel *clientsModel;
    QSqlRelationalTableModel *transactionsModel;
    int productCodeIndex, productDescIndex, productPriceIndex, productStockIndex, productCostIndex,
    productSoldUnitsIndex, productLastSoldIndex, productUnitsIndex, productBrandIndex,productTaxModelIndex,
    productPhotoIndex, productCategoryIndex, productPointsIndex, productLastProviderIndex, productAlphaCodeIndex;
    int offerIdIndex, offerDiscountIndex, offerDateStartIndex, offerDateEndIndex,offerProdIdIndex;
    int userIdIndex, usernameIndex, nameIndex, passwordIndex, saltIndex, addressIndex, phoneIndex, cellIndex, roleIndex,
    photoIndex;
    int transIdIndex, transClientidIndex, transTypeIndex,transAmountIndex,transDateIndex,transTimeIndex,transPaidWithIndex,
    transChangeGivenIndex,transPayMethodIndex,transStateIndex,transUseridIndex,transCardNumIndex,transItemCountIndex,transPointsIndex,
    transDiscMoneyIndex,transDiscIndex,transCardAuthNumberIndex,transUtilityIndex,transTerminalNumIndex,transItemsListIndex;
    QTimer *timerCheckDb, *timerUpdateGraphs;
    int balanceIdIndex, balanceDateEndIndex, balanceUserNameIndex, balanceInitAmountIndex, balanceInIndex, balanceOutIndex, balanceCashIndex, balanceCardIndex,balanceTransIndex, balanceTerminalNumIndex, balanceDateStartIndex, balanceUseridIndex;
    int cashflowIdIndex, cashflowDateIndex, cashflowTimeIndex, cashflowUseridIndex, cashflowReasonIndex, cashflowAmountIndex,    cashflowTerminalNumIndex, cashflowTypeIndex;
    int counter;
    bool modelsCreated,graphSoldItemsCreated;
    PieChart *pieSoldItems, *pieAlmostSoldOutItems;
    KPlotObject *objProfit, *objSales;


signals:
    void signalChangeStatusbar(const QString& text);
    void signalChangeCaption(const QString& text);
    void signalConnected();
    void signalDisconnected();
    void signalConnectActions();
    void signalShowPrefs();
    void signalAdminLoggedOn();
    void signalAdminLoggedOff();
    void signalSalir();
    void signalShowDbConfig();


 private slots:
   /* Ui related slot */
   void login();
   void settingsChanged();
   void settingsChangedOnInitConfig();
   void setupSignalConnections();
   void setOffersFilter();
   void toggleFilterBox(bool show);
   void showProductsPage();
   void showOffersPage();
   void showUsersPage();
   void showMeasuresPage();
   void showCategoriesPage();
   void showClientsPage();
   void showTransactionsPage();
   void usersViewOnSelected(const QModelIndex & index);
   void productsViewOnSelected(const QModelIndex &index);
   void clientsViewOnSelected(const QModelIndex &index);
   void doPurchase();
   void adjustOffersTable();
   void showPrefs();
   void cleanErrorLabel();
   void showWelcomeGraphs();
   void setupGraphs();
   void updateGraphs();
   void disableUI();
   void enableUI();
   void doEmitSignalSalir();
   void updateCategoriesCombo();
   void showProdListAsGrid();
   void showProdListAsTable();
   void adjustProductsTable();
   void showBalancesPage();
   void setupBalancesModel();
   void showCashFlowPage();
   void setupCashFlowModel();
   void hideShowFilterProductsGroup();

    /* DB slots */
   void createUser();
   void createOffer();
   void createProduct();
   void createMeasure();
   void createCategory();
   void createClient();
   void deleteSelectedOffer();
   void deleteSelectedUser();
   void deleteSelectedProduct();
   void deleteSelectedMeasure();
   void deleteSelectedCategory();
   void deleteSelectedClient();
   void populateCategoriesHash();
   void setProductsFilter();
   void setTransactionsFilter();
   void setBalancesFilter();

   void setupDb();
   void setupUsersModel();
   void setupOffersModel();
   void setupProductsModel();
   void setupMeasuresModel();
   void setupCategoriesModel();
   void setupClientsModel();
   void setupTransactionsModel();
   void checkDBStatus();
   void connectToDb();

   //Biel - export products
   void exportTable();
   void exportQTableView(QAbstractItemView *tableview);
};

#endif // SQUEEZEVIEW_H
