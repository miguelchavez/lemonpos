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

#include "clienteditor.h"

ClientEditorUI::ClientEditorUI( QWidget *parent )
: QFrame( parent )
{
    setupUi( this );
}

ClientEditor::ClientEditor( QWidget *parent )
: KDialog( parent )
{
    ui = new ClientEditorUI( this );
    setMainWidget( ui );
    setCaption( i18n("Client Editor") );
    setButtons( KDialog::Ok|KDialog::Cancel );

    connect( ui->btnChangeClientPhoto   , SIGNAL( clicked() ), this, SLOT( changePhoto() ) );
    connect( ui->editClientName, SIGNAL(textEdited(const QString &)),this, SLOT(checkName()) );

    QRegExp regexpC("[0-9]{1,13}");
    QRegExpValidator * validator = new QRegExpValidator(regexpC, this);
    ui->editClientPoints->setValidator(validator);
    ui->editClientDiscount->setValidator((new QDoubleValidator(0.00, 100.000, 3,ui->editClientDiscount)));

    QTimer::singleShot(750, this, SLOT(checkName()));
}

ClientEditor::~ClientEditor()
{
    delete ui;
}

void ClientEditor::changePhoto()
{
  QString fname = KFileDialog::getOpenFileName();
  if (!fname.isEmpty()) {
    QPixmap p = QPixmap(fname);
    setPhoto(p);
  }
}


void ClientEditor::checkName()
{
  if (ui->editClientName->text().isEmpty()) enableButtonOk(false);
  else enableButtonOk(true);
}

#include "clienteditor.moc"
