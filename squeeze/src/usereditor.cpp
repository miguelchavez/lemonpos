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

#include "usereditor.h"

UserEditorUI::UserEditorUI( QWidget *parent )
: QFrame( parent )
{
    setupUi( this );
}

UserEditor::UserEditor( QWidget *parent )
: KDialog( parent )
{
    ui = new UserEditorUI( this );
    setMainWidget( ui );
    setCaption( i18n("User Editor") );
    setButtons( KDialog::Ok|KDialog::Cancel );

    connect( ui->btnChangeUserPhoto   , SIGNAL( clicked() ), this, SLOT( changePhoto() ) );
    connect( ui->editUsersUsername, SIGNAL(textEdited(const QString &)),this, SLOT(checkName()) );

    QTimer::singleShot(750, this, SLOT(checkName()));
}

UserEditor::~UserEditor()
{
    delete ui;
}

void UserEditor::changePhoto()
{
  QString fname = KFileDialog::getOpenFileName();
  if (!fname.isEmpty()) {
    QPixmap p = QPixmap(fname);
    setPhoto(p);
  }
}

void UserEditor::checkName()
{
  if (ui->editUsersUsername->text().isEmpty()) enableButtonOk(false);
  else enableButtonOk(true);
}


void UserEditor::setUserName(QString uname)
{
  if (uname == "admin") ui->editUsersUsername->setReadOnly(true);
  ui->editUsersUsername->setText(uname);
}


#include "usereditor.moc"
