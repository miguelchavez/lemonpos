/***************************************************************************
 *   Copyright (C) 2012 by Miguel Chavez Gamboa                            *
 *   miguel.chavez.gamboa@gmail.com                                        *
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
#ifndef SUBCATEGORYEDITOR_H
#define SUBCATEGORYEDITOR_H

#include <KDialog>
#include <QDate>
#include <QtGui>
#include <QtSql>
#include "ui_subcategoryeditor.h"

class SubcategoryEditorUI : public QFrame, public Ui::subcategoryEditor
{
  Q_OBJECT
  public:
    SubcategoryEditorUI( QWidget *parent=0 );
};

class SubcategoryEditor : public KDialog
{
  Q_OBJECT
  public:
    SubcategoryEditor( QWidget *parent=0 );
    ~SubcategoryEditor();

    QString getParentCategoryName()   { return ui->comboParentCategory->currentText(); };
    QString getSubcategoryName()    { return ui->editSubcategory->text(); };
    void    populateCategories(QStringList list);

  private slots:
    void    checkValid();

  private:
    SubcategoryEditorUI *ui;
};

#endif
