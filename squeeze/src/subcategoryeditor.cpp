/***************************************************************************
 *   Copyright © 2012 by Miguel Chavez Gamboa                              *
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

#include "subcategoryeditor.h"
#include "../../dataAccess/azahar.h"

SubcategoryEditorUI::SubcategoryEditorUI( QWidget *parent )
: QFrame( parent )
{
    setupUi( this );
}

SubcategoryEditor::SubcategoryEditor( QWidget *parent )
: KDialog( parent )
{
    ui = new SubcategoryEditorUI( this );
    setMainWidget( ui );
    setCaption( i18n("Subcategory Editor") );
    setButtons( KDialog::Ok|KDialog::Cancel );

    enableButtonOk(false);
    connect(ui->editName, SIGNAL(editingFinished()), SLOT(checkValid()) );
}

SubcategoryEditor::~SubcategoryEditor()
{
    delete ui;
}

//this needs to be called after creating the dialog.
void SubcategoryEditor::populateList(QStringList list)
{
  ui->listView->clear();
  
  ui->listView->addItems( list );

  for(int i=0;i<ui->listView->count();i++){
    ui->listView->item(i)->setFlags(ui->listView->item(i)->flags() |Qt::ItemIsUserCheckable);
    ui->listView->item(i)->setCheckState(Qt::Unchecked); //FIXME: Check if the item belongs to the parent!, so check it.
  }
}

QStringList SubcategoryEditor::getChildren() {
    //returns a list of checked children.
    QStringList result;

    for(int i=0;i<ui->listView->count();i++){
        if (ui->listView->item(i)->checkState())
            result.append( ui->listView->item(i)->text() );
    }

    return result;
}


void SubcategoryEditor::checkValid()
{
  bool validText = !ui->editName->text().isEmpty();
  bool validSubcat = false;
  //if (ui->comboParentCategory->currentIndex() > 0) // 1 is the " --- " or None category.
  //    validSubcat = true;
  
  //enableButtonOk(validSubcat && validText);
  enableButtonOk(validText);
}

#include "subcategoryeditor.moc"
