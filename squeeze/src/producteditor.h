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
#ifndef PRODUCTEDITOR_H
#define PRODUCTEDITOR_H

#include <KDialog>
#include <QDate>
#include <QtGui>
#include <QPixmap>
#include <QtSql>

#include "ui_producteditor.h"

class MibitFloatPanel;
class MibitTip;
#include "../../src/structs.h"

enum returnType {statusNormal=998, statusMod=999};

class ProductEditorUI : public QFrame, public Ui::productEditor
{
  Q_OBJECT
  public:
    ProductEditorUI( QWidget *parent=0 );
};

class ProductEditor : public KDialog
{
  Q_OBJECT
  public:
    ProductEditor( QWidget *parent=0, bool newProduct = false, const QSqlDatabase& database=QSqlDatabase()  );
    ~ProductEditor();

    ProductInfo getProductInfo() { return m_pInfo; };
    void    setDb(QSqlDatabase database);
    void    setCode(qulonglong c);
    void    disableCode()              { ui->editCode->setReadOnly(true); modifyCode=false; };
    void    enableCode()               { ui->editCode->setReadOnly(false); modifyCode=true; };
    void    disableStockCorrection()   { ui->btnStockCorrect->hide(); }
    int     getCategoryId();
    int     getMeasureId();
    QString getReason()      { return reason; };
    bool    isCorrectingStock() {return correctingStockOk;};
    double  getOldStock()    { return oldStockQty; };
    double  getGRoupStockMax();
    void    setStockQtyReadOnly(bool enabled) { ui->editStockQty->setReadOnly(enabled); };

    void    setModel(QSqlRelationalTableModel *model);
    void    setIsAGroup(bool value);
    void    setIsARaw(bool value);
    void    setGroupElements(QString e);
    
    GroupInfo getGroupHash();
    QString getGroupElementsStr();
    QString getSpecialOrdersStr();
    bool    isGroup();
    bool    isRaw();
    
  signals:
    void    updateCategoriesModel();
    void    updateMeasuresModel();
    void    updateBrandsModel();
    void    updateTaxModels();
    void    updateProvidersModel();
    void    updateProductsModel();
    
private slots:
    void    changePhoto();
    void    changeCode();
    void    calculatePrice();
    void    checkIfCodeExists();
    void    checkFieldsState();
    void    updateTax(int);
    void    updateBrand(int);
    void    updateProvider(int);
    void    updateCode(const QString &str);
    void    updateACode(const QString &str);
    void    updatePrice(const QString &str);
    void    updateCost(const QString &str);
    void    updateDesc(const QString &str);
    void    updatePoints(const QString &str);
    void    updateStockQty(const QString &str);
    void    updateCategory(int);
    void    updateMeasure(int);
    void    updateGroupNRaw();
    void    updateBtn(); //for floatpanel - ok button
    void    showPanelBrand();
    void    showPanelStock();
    void    showPanelCategory();
    void    showPanelMeasures();
    void    showPanelProviders();
    void    showBtns();
    void    toggleGroup(bool checked);
    void    toggleRaw(bool checked);
    void    applyFilter(const QString &text);
    void    addItem();
    void    removeItem();
    void    itemDoubleClicked(QTableWidgetItem* item);
  protected slots:
    virtual void slotButtonClicked(int button);
  private:
    ProductEditorUI *ui;
    QSqlDatabase db;
    QPixmap pix;
    returnType status;
    bool modifyCode;
    ProductInfo m_pInfo;
    MibitTip *codeTip;
    MibitFloatPanel *panel;
    MibitFloatPanel *groupPanel;
    QString reason;
    bool correctingStockOk;
    double oldStockQty;
    GroupInfo groupInfo;
    bool m_modelAssigned;
    QSqlRelationalTableModel *m_model;

    QString getCategoryStr(int c);
    QString getMeasureStr(int c);
    QString getBrandStr(int c);
    QString getProviderStr(int c);
    QString getTaxModelStr(int c);

    void    setCategory(QString str);
    void    setCategory(int i);
    void    setMeasure(QString str);
    void    setMeasure(int i);
    void    setTax(const QString &str);
    void    setTax(const int &i);
    void    setTaxModel(qulonglong id);
    void    setBrand(const QString &str);
    void    setBrand(const int &i);
    void    setProvider(const QString &str);
    void    setProvider(const int &i);
    void    setPhoto(QPixmap p);
    void    populateCategoriesCombo();
    void    populateMeasuresCombo();
    void    populateBrandsCombo();
    void    populateProvidersCombo();
    void    populateTaxModelsCombo();
    //void    setProviderId(qulonglong id) { m_pInfo.lastProviderId = id; };
    //void    setBrandId(qulonglong id) { m_pInfo.brandid = id; };
    
};

#endif
