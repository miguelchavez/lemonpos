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
#include <KLocale>
#include <KMessageBox>
#include <KFileDialog>
#include <KStandardDirs>

#include <QByteArray>
#include <QRegExpValidator>

#include "producteditor.h"
#include "../../dataAccess/azahar.h"
#include "../../src/misc.h"
#include "../../mibitWidgets/mibittip.h"

ProductEditorUI::ProductEditorUI( QWidget *parent )
: QFrame( parent )
{
    setupUi( this );
}

ProductEditor::ProductEditor( QWidget *parent, bool newProduct )
: KDialog( parent )
{
    ui = new ProductEditorUI( this );
    setMainWidget( ui );
    setCaption( i18n("Product Editor") );
    setButtons( KDialog::Ok|KDialog::Cancel );

    ui->btnChangeCode->setIcon(QIcon(DesktopIcon("edit-clear", 32)));
    //Locate SVG for the tip.
    QString path = KStandardDirs::locate("appdata", "styles/");
    path = path+"tip.svg";
    codeTip = new MibitTip(this, ui->editCode, path, DesktopIcon("dialog-information", 22));
    codeTip->setSize(100,100);

    //Set Validators for input boxes
    QRegExp regexpC("[0-9]{1,13}"); //(EAN-13 y EAN-8) .. y productos sin codigo de barras?
    QRegExpValidator * validatorEAN13 = new QRegExpValidator(regexpC, this);
    ui->editCode->setValidator(validatorEAN13);
    ui->editUtility->setValidator(new QDoubleValidator(0.00, 999999999999.99, 3,ui->editUtility));
    ui->editCost->setValidator(new QDoubleValidator(0.00, 999999999999.99, 3, ui->editCost));
    ui->editStockQty->setValidator(new QDoubleValidator(0.00,999999999999.99, 3, ui->editStockQty));
    ui->editPoints->setValidator(new QIntValidator(0,999999999, ui->editPoints));
    ui->editFinalPrice->setValidator(new QDoubleValidator(0.00,999999999999.99, 3, ui->editFinalPrice));

    connect( ui->btnPhoto          , SIGNAL( clicked() ), this, SLOT( changePhoto() ) );
    connect( ui->btnCalculatePrice , SIGNAL( clicked() ), this, SLOT( calculatePrice() ) );
    connect( ui->btnChangeCode,      SIGNAL( clicked() ), this, SLOT( changeCode() ) );
    connect( ui->editCode, SIGNAL(textEdited(const QString &)), SLOT(checkIfCodeExists()));
    connect( ui->editCode, SIGNAL(editingFinished()), this, SLOT(checkFieldsState()));

    connect( ui->editDesc, SIGNAL(editingFinished()), this, SLOT(checkFieldsState()));
    connect( ui->editStockQty, SIGNAL(editingFinished()), this, SLOT(checkFieldsState()));
    connect( ui->editPoints, SIGNAL(editingFinished()), this, SLOT(checkFieldsState()));
    connect( ui->editCost, SIGNAL(editingFinished()), this, SLOT(checkFieldsState()));
    connect( ui->editFinalPrice, SIGNAL(textEdited(const QString &)), this, SLOT(checkFieldsState()));

    connect(ui->taxModelCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTax(int)));
    connect(ui->brandCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBrand(int)));
    connect(ui->providerCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateProvider(int)));
    connect(ui->categoriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCategory(int)));
    connect(ui->measuresCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMeasure(int)));
    connect(ui->editCode, SIGNAL(textEdited(const QString &)), this, SLOT(updateCode(const QString &)));
    connect(ui->editAlphacode, SIGNAL(textEdited(const QString &)), this, SLOT(updateACode(const QString &)));
    connect(ui->editCost, SIGNAL(textEdited(const QString &)), this, SLOT(updateCost(const QString &)));
    connect(ui->editDesc, SIGNAL(textEdited(const QString &)), this, SLOT(updateDesc(const QString &)));
    connect(ui->editFinalPrice, SIGNAL(textEdited(const QString &)), this, SLOT(updatePrice(const QString &)));
    connect(ui->editPoints, SIGNAL(textEdited(const QString &)), this, SLOT(updatePoints(const QString &)));
    connect(ui->editStockQty, SIGNAL(textEdited(const QString &)), this, SLOT(updateStockQty(const QString &)));


    status = statusNormal;
    modifyCode = false;
    
    if (newProduct) ui->labelStockQty->setText(i18n("Purchase Qty:")); else ui->labelStockQty->setText(i18n("Stock Qty:"));
    
    QTimer::singleShot(350, this, SLOT(checkIfCodeExists()));

    ui->editStockQty->setText("0.0");
    ui->editPoints->setText("0.0");
    m_pInfo.tax = 0.0;
    m_pInfo.totaltax = 0.0;
    m_pInfo.taxmodelid = 0;
    m_pInfo.brandid = 0;
    m_pInfo.lastProviderId = 0;
}

ProductEditor::~ProductEditor()
{
    delete ui;
}

void ProductEditor::setDb(QSqlDatabase database)
{
  db = database;
  if (!db.isOpen()) db.open();
  populateCategoriesCombo();
  populateMeasuresCombo();
  populateProvidersCombo();
  populateBrandsCombo();
  populateTaxModelsCombo();
}

void ProductEditor::populateCategoriesCombo()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->categoriesCombo->addItems(myDb->getCategoriesList());
}

void ProductEditor::populateProvidersCombo()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->providerCombo->addItems(myDb->getProvidersList());
}

void ProductEditor::populateBrandsCombo()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->brandCombo->addItems(myDb->getBrandsList());
}

void ProductEditor::populateTaxModelsCombo()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->taxModelCombo->addItems(myDb->getTaxModelsList());
}

void ProductEditor::populateMeasuresCombo()
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->measuresCombo->addItems(myDb->getMeasuresList());
}


void ProductEditor::setCode(qulonglong c)
{
  ui->editCode->setText(QString::number(c));
  //get product Info...
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  m_pInfo = myDb->getProductInfo(QString::number(c));
}

void ProductEditor::setCategory(QString str)
{
 int idx = ui->categoriesCombo->findText(str,Qt::MatchCaseSensitive);
 if (idx > -1) ui->categoriesCombo->setCurrentIndex(idx);
 else {
  qDebug()<<"Str not found:"<<str;
  }
}

void ProductEditor::setTax(const QString &str)
{
  int idx = ui->taxModelCombo->findText(str,Qt::MatchCaseSensitive);
  if (idx > -1) ui->taxModelCombo->setCurrentIndex(idx);
  else {
    qDebug()<<"Str not found:"<<str;
  }
}

void ProductEditor::setCategory(int i)
{
 QString text = getCategoryStr(i);
 setCategory(text);
}

void ProductEditor::setMeasure(int i)
{
 QString text = getMeasureStr(i);
 setMeasure(text);
}

void ProductEditor::setTax(const int &i)
{
  QString text = getTaxModelStr(i);
  setTax(text);
}

void ProductEditor::setBrand(const int &i)
{
  QString text = getBrandStr(i);
  setBrand(text);
}

void ProductEditor::setProvider(const int &i)
{
  QString text = getProviderStr(i);
  setProvider(text);
}

void ProductEditor::setMeasure(QString str)
{
int idx = ui->measuresCombo->findText(str,Qt::MatchCaseSensitive);
 if (idx > -1) ui->measuresCombo->setCurrentIndex(idx);
 else {
  qDebug()<<"Str not found:"<<str;
  }
}

void ProductEditor::setBrand(const QString &str)
{
  int idx = ui->brandCombo->findText(str,Qt::MatchCaseSensitive);
  if (idx > -1) 
    ui->brandCombo->setCurrentIndex(idx);
  else {
    qDebug()<<"Str not found:"<<str;
  }
}

void ProductEditor::setProvider(const QString &str)
{
  int idx = ui->providerCombo->findText(str,Qt::MatchCaseSensitive);
  if (idx > -1) ui->providerCombo->setCurrentIndex(idx);
  else {
    qDebug()<<"Str not found:"<<str;
  }
}

void ProductEditor::setTaxModel(qulonglong id)
{
  m_pInfo.taxmodelid = id;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getTaxModelName(id);

  int idx = ui->taxModelCombo->findText(str,Qt::MatchCaseSensitive);
  if (idx > -1) ui->taxModelCombo->setCurrentIndex(idx);
  else {
    qDebug()<<"TaxModel Str not found:"<<str;
  }
  updateTax(0);
}

void ProductEditor::updateTax(int)
{
  //get data from selected model.
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  //refresh taxmodel if changed...
  m_pInfo.taxmodelid   = myDb->getTaxModelId(ui->taxModelCombo->currentText());
  m_pInfo.taxElements  = myDb->getTaxModelElements(m_pInfo.taxmodelid);
  m_pInfo.tax          = myDb->getTotalTaxPercent(m_pInfo.taxElements);
  //way to apply the tax?? -- There is a problem if the cost includes taxes already.
  double cost    = ui->editCost->text().toDouble();
  double utility = ui->editUtility->text().toDouble();
  //Utility is calculated before taxes... Taxes include utility... is it ok?
  utility = ((utility/100)*cost);
  m_pInfo.totaltax = (cost + utility)*m_pInfo.tax; //taxes in money.
  qDebug()<<"Updating TaxModel:"<<m_pInfo.taxmodelid;
}

void ProductEditor::updateCategory(int)
{
  int code=-1;
  QString currentText = ui->categoriesCombo->currentText();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  code = myDb->getCategoryId(currentText);
  m_pInfo.category = code;
}

void ProductEditor::updateMeasure(int)
{
  int code=-1;
  QString currentText = ui->measuresCombo->currentText();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  code = myDb->getMeasureId(currentText);
  m_pInfo.units = code;
}

void ProductEditor::updateCode(const QString &str)
{
  m_pInfo.code = str.toULongLong();
}

void ProductEditor::updateACode(const QString &str)
{
  m_pInfo.alphaCode = str;
}

void ProductEditor::updateCost(const QString &str)
{
  m_pInfo.cost = str.toDouble();
}

void ProductEditor::updatePrice(const QString &str)
{
  m_pInfo.price = str.toDouble();
}

void ProductEditor::updatePoints(const QString &str)
{
  m_pInfo.points = str.toULongLong();
}

void ProductEditor::updateStockQty(const QString &str)
{
  m_pInfo.stockqty = str.toDouble();
}

void ProductEditor::updateDesc(const QString &str)
{
  m_pInfo.desc = str;
}

void ProductEditor::updateBrand(int)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  m_pInfo.brandid = myDb->getBrandId(ui->brandCombo->currentText());
}

void ProductEditor::updateProvider(int)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  m_pInfo.lastProviderId = myDb->getProviderId(ui->providerCombo->currentText());
}

QString ProductEditor::getCategoryStr(int c)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getCategoryStr(c);
  return str;
}

QString ProductEditor::getMeasureStr(int c)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getMeasureStr(c);
  return str;
}

QString ProductEditor::getBrandStr(int c)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getBrandName(c);
  return str;
}

QString ProductEditor::getProviderStr(int c)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getProviderName(c);
  return str;
}

QString ProductEditor::getTaxModelStr(int c)
{
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getTaxModelName(c);
  return str;
}

void ProductEditor::changePhoto()
{
 QString fname = KFileDialog::getOpenFileName();
  if (!fname.isEmpty()) {
    QPixmap p = QPixmap(fname);
    setPhoto(p);
    //update photo ba to the m_pInfo
    m_pInfo.photo = Misc::pixmap2ByteArray(new QPixmap(p));
  }
}

void ProductEditor::calculatePrice()
{
 double finalPrice=0.0;
 if (ui->editCost->text().isEmpty()) {
   ui->editCost->setFocus();
 }
 else if (ui->editUtility->text().isEmpty()) {
   ui->editUtility->setFocus();
 }
//  else if (ui->editTax->text().isEmpty()) {
//    ui->editTax->setText("0.0");
//    ui->editTax->setFocus();
//    ui->editTax->selectAll();
//  }
 else {
//   if (ui->editExtraTaxes->text().isEmpty()) {
//    ui->editExtraTaxes->setText("0.0");
//    ui->editExtraTaxes->setFocus();
//    ui->editExtraTaxes->selectAll();
//   }
  //TODO: if TAXes are included in cost...
  double cost    = ui->editCost->text().toDouble();
  double utility = ui->editUtility->text().toDouble();
  //Utility is calculated before taxes... Taxes include utility... is it ok?
  utility = ((utility/100)*cost);
  finalPrice = cost + utility + m_pInfo.totaltax;
  
  // BFB: avoid more than 2 decimal digits in finalPrice. Round.
  ui->editFinalPrice->setText(QString::number(finalPrice,'f',2));
  ui->editFinalPrice->selectAll();
  ui->editFinalPrice->setFocus();
  }
}

void ProductEditor::changeCode()
{
  //this enables the code editing... to prevent unwanted code changes...
  enableCode();
  ui->editCode->setFocus();
  ui->editCode->selectAll();
}

void ProductEditor::checkIfCodeExists()
{
  enableButtonOk( false );
  QString codeStr = ui->editCode->text();
  if (codeStr.isEmpty()) {
    codeStr="-1";
  }

  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ProductInfo pInfo = myDb->getProductInfo(codeStr);

  if (pInfo.code > 0) {
    //code exists...
    status = statusMod;
    if (!modifyCode){
      //Prepopulate dialog... NOTE: Check m_pInfo !!!
      ui->editDesc->setText(pInfo.desc);
      ui->editAlphacode->setText(pInfo.alphaCode);
      ui->editStockQty->setText(QString::number(pInfo.stockqty));
      setCategory(pInfo.category);
      setMeasure(pInfo.units);
      ui->editCost->setText(QString::number(pInfo.cost));
      ui->editFinalPrice->setText(QString::number(pInfo.price));
      ui->editPoints->setText(QString::number(pInfo.points));
      setTax(pInfo.taxmodelid);
      setBrand(pInfo.brandid);
      setProvider(pInfo.lastProviderId);
      if (!pInfo.photo.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(pInfo.photo);
        setPhoto(photo);
      }
    }//if !modifyCode
    else {
      codeTip->showTip(i18n("The product already exists."), 3000);
      enableButtonOk( false );
    }
  }
  else { //code does not exists...
    status = statusNormal;
    if (!modifyCode) {
      //clear all used edits NOTE: Check m_pInfo!!
      ui->editDesc->clear();
      ui->editStockQty->clear();
      setCategory(1);
      setMeasure(1);
      setBrand(1);
      setProvider(1);
      setTax(1);
      ui->editCost->clear();
      ui->editFinalPrice->clear();
      ui->editPoints->clear();
      ui->editUtility->clear();
      ui->editFinalPrice->clear();
      ui->labelPhoto->setText("No Photo");
      }
      //qDebug()<< "no product found with code "<<codeStr<<" .query.size()=="<<query.size();
  }
}

void ProductEditor::checkFieldsState()
{
  bool ready = false;
  if ( !ui->editCode->text().isEmpty()    &&
    !ui->editDesc->text().isEmpty()       &&
    //!ui->editStockQty->text().isEmpty()   &&   Comment: This requirement was removed in order to use check-in/check-out procedures.
    !ui->editPoints->text().isEmpty()     &&
    !ui->editCost->text().isEmpty()       &&
    //!ui->editTax->text().isEmpty()        &&
    //!ui->editExtraTaxes->text().isEmpty() &&
    !ui->editFinalPrice->text().isEmpty()
    )  {
    ready = true;
  }
  enableButtonOk(ready);
  
  if (!ready  && ui->editCode->hasFocus() ) {
    ui->editDesc->setFocus();
  }
}

void ProductEditor::setPhoto(QPixmap p)
{
  int max = 150;
  QPixmap newPix;
  if ((p.height() > max) || (p.width() > max) ) {
    if (p.height() == p.width()) {
      newPix = p.scaled(QSize(max, max));
    }
    else if (p.height() > p.width() ) {
      newPix = p.scaledToHeight(max);
    }
    else  {
      newPix = p.scaledToWidth(max);
    }
  } else newPix=p;
  ui->labelPhoto->setPixmap(newPix);
  pix=newPix;
}

void ProductEditor::slotButtonClicked(int button)
{
  //update all information...
  updateTax(0);
  updateBrand(0);
  updateProvider(0);
  updateMeasure(0);
  updateCategory(0);
  updateCode(ui->editCode->text());
  updateACode(ui->editAlphacode->text());
  updateCost(ui->editCost->text());
  updateDesc(ui->editDesc->text());
  updatePoints(ui->editPoints->text());
  updatePrice(ui->editFinalPrice->text());
  updateStockQty(ui->editStockQty->text());
  
  if (button == KDialog::Ok) {
    if (status == statusNormal) QDialog::accept();
    else {
      qDebug()<< "Button = OK, status == statusMOD";
      done(statusMod);
    }
  }
  else QDialog::reject();
}

#include "producteditor.moc"
