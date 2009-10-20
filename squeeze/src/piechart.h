/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef PIECHART_H
#define PIECHART_H

#include <QWidget>
#include <QBrush>
#include <QPen>

class QPaintEvent;

class PieChart
    : public QWidget
{
    Q_ENUMS(KeysPosition)

public:
    enum KeysPosition
    {
        Left = 0,
        Top,
        Right,
        Bottom
    };

    PieChart(QWidget *parent = 0);
    ~PieChart();

    void addSlice(const QString &sliceKey, double percent, const QBrush &brush);
    void deleteSlices();

    bool showKeys() const;
    void setShowKeys(bool showKeys);

    KeysPosition keysPosition() const;
    void setKeysPosition(KeysPosition keysPosition);

    int spacing() const;
    void setSpacing(int spacing);

    int height() const;
    void setHeight(int height);

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    class Private;
    Private *d;
};


#endif
