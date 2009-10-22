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

#include <QByteArray>
#include <QRegExpValidator>

#include "producteditor.h"
#include "../../dataAccess/azahar.h"

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

    status = statusNormal;
    modifyCode = false;
    
    if (newProduct) ui->labelStockQty->setText(i18n("Purchase Qty:")); else ui->labelStockQty->setText(i18n("Stock Qty:"));
    
    QTimer::singleShot(750, this, SLOT(checkIfCodeExists()));

    ui->editStockQty->setText("0.0");
    ui->editPoints->setText("0.0");
    m_tax = 0.0;
    m_totalTax = 0.0;
    m_taxModel = 0;
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
  QSqlQuery query(db);
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->categoriesCombo->addItems(myDb->getCategoriesList());
}

void ProductEditor::populateProvidersCombo()
{
  QSqlQuery query(db);
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->providerCombo->addItems(myDb->getProvidersList());
}

void ProductEditor::populateBrandsCombo()
{
  QSqlQuery query(db);
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->brandCombo->addItems(myDb->getBrandsList());
}

void ProductEditor::populateTaxModelsCombo()
{
  QSqlQuery query(db);
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->taxModelCombo->addItems(myDb->getTaxModelsList());
}

void ProductEditor::populateMeasuresCombo()
{
  QSqlQuery query(db);
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  ui->measuresCombo->addItems(myDb->getMeasuresList());
}

int ProductEditor::getCategoryId()
{
  QSqlQuery query(db);
  int code=-1;
  QString currentText = ui->categoriesCombo->currentText();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  code = myDb->getCategoryId(currentText);
  return code;
}


int ProductEditor::getMeasureId()
{
  QSqlQuery query(db);
  int code=-1;
  QString currentText = ui->measuresCombo->currentText();
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  code = myDb->getMeasureId(currentText);
  return code;
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
  qDebug()<<"setting Tax Str";
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
  if (idx > -1) ui->brandCombo->setCurrentIndex(idx);
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
  m_taxModel = id;
  Azahar *myDb = new Azahar;
  myDb->setDatabase(db);
  QString str = myDb->getTaxModelName(id);
  qDebug()<<"tax id:"<<id;

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
  m_taxElements  = myDb->getTaxModelElements(m_taxModel);
  m_tax          = myDb->getTotalTaxPercent(m_taxElements);
  //way to apply the tax?? -- There is a problem if the cost includes taxes already.
  double cost    = ui->editCost->text().toDouble();
  double utility = ui->editUtility->text().toDouble();
  //Utility is calculated before taxes... Taxes include utility... is it ok?
  utility = ((utility/100)*cost);
  m_totalTax = (cost + utility)*m_tax; //taxes in money.
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
  finalPrice = cost + utility + m_totalTax;
  
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
      //Prepopulate dialog...
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
      //ui->labelError->show(); FIXME: Agregar el mibitTip
      enableButtonOk( false );
    }
  }
  else { //code does not exists...
    status = statusNormal;
    if (!modifyCode) {
      //clear all used edits
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
