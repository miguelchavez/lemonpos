/***************************************************************************
 *   Copyright (C) 2007-2010 by Miguel Chavez Gamboa                       *
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>

class MibitFloatPanel;
class MibitTip;
class MibitDialog;


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    ///Shows configuration panel
    void showConfig();
    ///Saves/Reads configuration information.
    void writeSettings();
    void readSettings();
    void setTheSplitterSizes(QPoint p);
    QList<int> getTheSplitterSizes();
    void saveSplitter();
    void adjustTables();
    void showConfirmation();
    void confirmOrder();
    //quit
    void quit();
    //React when settings changed
    void settingsChanged();
    void setupDB();
    void connectToDb();
    void refreshModels();

    //each list on view methods
    //void listViewOnClick( const QModelIndex & index );

    //Save an action log.. each processed order,etc..
    //void log(const qulonglong &uid, const QDate &date, const QTime &time, const QString &text);

private:
    Ui::MainWindow *ui;

    MibitFloatPanel *groupPanel;
    MibitDialog     *confirmDialog;

    QSqlDatabase db;
    QSqlTableModel *shipmentsModel;
    bool   modelsCreated;


    void setupModel();

protected:
     void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
