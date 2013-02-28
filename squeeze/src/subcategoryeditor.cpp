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
    connect(ui->editSubcategory, SIGNAL(editingFinished()), SLOT(checkValid()) );
    connect(ui->comboParentCategory, SIGNAL(currentIndexChanged( int )), SLOT(checkValid()) );
}

SubcategoryEditor::~SubcategoryEditor()
{
    delete ui;
}

//this needs to be called after creating the dialog.
void SubcategoryEditor::populateCategories(QStringList list)
{
  ui->comboParentCategory->clear();
  //FIXME: check if list contains the " --- " element, if not, add it.
  
  ui->comboParentCategory->addItems( list );
}

void SubcategoryEditor::checkValid()
{
  bool validText = !ui->editSubcategory->text().isEmpty();
  bool validSubcat = false;
  if (ui->comboParentCategory->currentIndex() > 0) // 1 is the " --- " or None category.
      validSubcat = true;
  
  enableButtonOk(validSubcat && validText);
}

#include "subcategoryeditor.moc"
