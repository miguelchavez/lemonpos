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

#include "piechart.h"

#include <math.h>

#include <QSize>
#include <QPainter>
#include <QPaintEvent>

  /* #include <kcolorscheme.h> */

class PieChart::Private
{
public:
    Private(PieChart *q);
    ~Private();

    void drawKeys(QPainter *painter,  const QRect &paintRect) const;
    QSize sizeKeys(QPainter *painter) const;

    class PieChartSlice;
    class PieChartKeysElement;

    QList<PieChartSlice*> slices;
    bool showKeys;
    KeysPosition keysPosition;
    int keysPixelSeparator;
    int maxPenWidth;
    int spacing;
    int height;

    PieChart *q;
};


class PieChart::Private::PieChartSlice
{
public:
    PieChartSlice(const QString &sliceKey, int percent,  const QBrush &brush);
    ~PieChartSlice();

    QString sliceKey;
    int percent;
    QBrush brush;
};


PieChart::Private::PieChartSlice::PieChartSlice(const QString &sliceKey, int percent, const QBrush &brush)
    : sliceKey(sliceKey)
    , percent(percent)
    , brush(brush)
{
}

PieChart::Private::PieChartSlice::~PieChartSlice()
{
}


PieChart::Private::Private(PieChart *q)
    : showKeys(true)
    , keysPosition(Left)
    , keysPixelSeparator(5)
    , maxPenWidth(0)
    , spacing(10)
    , height(25)
    , q(q)
{
}

PieChart::Private::~Private()
{
    qDeleteAll(slices);
    slices.clear();
}

void PieChart::Private::drawKeys(QPainter *painter, const QRect &paintRect) const
{
    QRect keysRect(paintRect);
    QSize size = sizeKeys(painter);

    if ((keysPosition == Left) ||
        (keysPosition == Right))
    {
        int currentX;

        if (keysPosition == Left)
        {
            keysRect.setRight(keysRect.left() + size.width());

            currentX = spacing;
        }
        else
        {
            keysRect.setLeft(keysRect.right() - size.width());

            currentX = keysRect.right() - painter->fontMetrics().height() /* colored square */ - spacing;
        }

        int currentY = ((float) paintRect.height() / 2) - ((float) size.height() / 2);
        foreach (PieChartSlice *slice, slices)
        {
            painter->save();
            painter->translate(currentX, currentY);
            painter->fillRect(QRect(0, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
            painter->restore();

            if (keysPosition == Left)
            {
                painter->drawText(QPoint(currentX + painter->fontMetrics().height() /* colored square */ + keysPixelSeparator, currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), slice->sliceKey);
            }
            else
            {
                painter->drawText(QPoint(currentX - painter->fontMetrics().width(slice->sliceKey) - keysPixelSeparator, currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), slice->sliceKey);
            }

            currentY += painter->fontMetrics().height() + keysPixelSeparator;
        }
    } else {}
//     else if ((keysPosition == Top) ||
//              (keysPosition == Bottom))
//     {
//         int currentY;
// 
//         if (keysPosition == Top)
//         {
//             keysRect.setBottom(keysRect.top() + size.height());
// 
//             currentY = 0;
//         }
//         else
//         {
//             keysRect.setTop(keysRect.bottom() - size.height());
// 
//             currentY = keysRect.bottom() - painter->fontMetrics().height() /* colored square */;
//         }
// 
//         int currentX = (q->layoutDirection() == Qt::LeftToRight) ? ((float) paintRect.width() / 2) - ((float) size.width() / 2)
//                                                                  : ((float) paintRect.width() / 2) + ((float) size.width() / 2);
//         foreach (PieChartSlice *slice, slices)
//         {
//             if (q->layoutDirection() == Qt::LeftToRight)
//             {
//                 painter->save();
//                 painter->translate(currentX, currentY);
//                 painter->fillRect(QRect(0, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//                 painter->restore();
// 		QString texto = QString("%1").arg(slice->sliceKey)/*.arg(slice->percent)*/;
//                 painter->drawText(QPoint(currentX + painter->fontMetrics().height() /* colored square */ + keysPixelSeparator, currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), texto /*slice->sliceKey*/);
// 
// 		currentX += painter->fontMetrics().width(/*slice->sliceKey*/texto) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//             }
//             else
//             {
//                 painter->save();
//                 painter->translate(currentX, currentY);
//                 painter->fillRect(QRect(-painter->fontMetrics().height() /* colored square */, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//                 painter->restore();
//                 painter->drawText(QPoint(currentX - painter->fontMetrics().height() /* colored square */ - keysPixelSeparator - painter->fontMetrics().width(slice->sliceKey), currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), slice->sliceKey);
// 
//                 currentX -= painter->fontMetrics().width(slice->sliceKey) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//             }
//         }
//     }
//     else { //MCH
//       int current;
//       int contador=0;//mch
//       //TOP
//       keysRect.setBottom(keysRect.top() + size.height());
//       int currentY = 0;
//       int currentX = (q->layoutDirection() == Qt::LeftToRight) ? ((float) paintRect.width() / 2) - ((float) size.width() / 2)
//       : ((float) paintRect.width() / 2) + ((float) size.width() / 2);
//       foreach (PieChartSlice *slice, slices)
//       {
//        contador++;
//        if (contador<4) {
//         if (q->layoutDirection() == Qt::LeftToRight)
//         {
//           painter->save();
//           painter->translate(currentX, currentY);
//           painter->fillRect(QRect(0, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//           painter->restore();
//           QString texto = QString("%1").arg(slice->sliceKey)/*.arg(slice->percent)*/;
//           painter->drawText(QPoint(currentX + painter->fontMetrics().height() /* colored square */ + keysPixelSeparator, currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), texto /*slice->sliceKey*/);
//           
//           currentX += painter->fontMetrics().width(/*slice->sliceKey*/texto) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//         }
//         else
//         {
//           painter->save();
//           painter->translate(currentX, currentY);
//           painter->fillRect(QRect(-painter->fontMetrics().height() /* colored square */, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//           painter->restore();
//           painter->drawText(QPoint(currentX - painter->fontMetrics().height() /* colored square */ - keysPixelSeparator - painter->fontMetrics().width(slice->sliceKey), currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), slice->sliceKey);
//           
//           currentX -= painter->fontMetrics().width(slice->sliceKey) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//         }
//        } //if contador mch
//       } //for each
// 
//       //BOTTOM
//       keysRect.setTop(keysRect.bottom() - size.height());
//       currentY = keysRect.bottom() - painter->fontMetrics().height() /* colored square */;
//       currentX = (q->layoutDirection() == Qt::LeftToRight) ? ((float) paintRect.width() / 2) - ((float) size.width() / 2)
//       : ((float) paintRect.width() / 2) + ((float) size.width() / 2);
// 
//       contador=0;
//       foreach (PieChartSlice *slice, slices)
//       {
//         contador++;
//         if (contador>3) {
//           if (q->layoutDirection() == Qt::LeftToRight)
//           {
//             painter->save();
//             painter->translate(currentX, currentY);
//             painter->fillRect(QRect(0, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//             painter->restore();
//             QString texto = QString("%1").arg(slice->sliceKey)/*.arg(slice->percent)*/;
//             painter->drawText(QPoint(currentX + painter->fontMetrics().height() /* colored square */ + keysPixelSeparator, currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), texto /*slice->sliceKey*/);
//             
//             currentX += painter->fontMetrics().width(/*slice->sliceKey*/texto) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//           }
//           else
//           {
//             painter->save();
//             painter->translate(currentX, currentY);
//             painter->fillRect(QRect(-painter->fontMetrics().height() /* colored square */, 0, painter->fontMetrics().height(), painter->fontMetrics().height()), slice->brush);
//             painter->restore();
//             painter->drawText(QPoint(currentX - painter->fontMetrics().height() /* colored square */ - keysPixelSeparator - painter->fontMetrics().width(slice->sliceKey), currentY + ((float) painter->fontMetrics().height() / 2) + ((float) painter->fontMetrics().height() / 4)), slice->sliceKey);
//             
//             currentX -= painter->fontMetrics().width(slice->sliceKey) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
//           }
//         } //if contador mch
//       } //for each

//     }//MCH
}

QSize PieChart::Private::sizeKeys(QPainter *painter) const
{
    if ((keysPosition == Left) ||
        (keysPosition == Right))
    {
        int maxWidth = 0;

        foreach (PieChartSlice *slice, slices)
        {
            maxWidth = qMax(maxWidth, painter->fontMetrics().width(slice->sliceKey) + keysPixelSeparator + painter->fontMetrics().height() /* colored square */);
        }

        return QSize(maxWidth + keysPixelSeparator + spacing, slices.count() * painter->fontMetrics().height() + (slices.count() - 1) * keysPixelSeparator + spacing * 2);
    }
    else
    {
        int width = 0;

        foreach (PieChartSlice *slice,  slices)
        {
            width += painter->fontMetrics().width(slice->sliceKey) + keysPixelSeparator * 2 + painter->fontMetrics().height() /* colored square */;
        }

        return QSize(width + spacing * 2, painter->fontMetrics().height() + keysPixelSeparator + spacing);
    }
}

PieChart::PieChart(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    resize(600, 600);
}

PieChart::~PieChart()
{
    delete d;
}

void PieChart::addSlice(const QString &sliceKey, double percent, const QBrush &brush)
{
    d->slices << new Private::PieChartSlice(sliceKey, percent, brush);
}

void PieChart::deleteSlices()
{
  qDeleteAll(d->slices);
  d->slices.clear();
}

bool PieChart::showKeys() const
{
    return d->showKeys;
}

void PieChart::setShowKeys(bool showKeys)
{
    d->showKeys = showKeys;

    update();
}

PieChart::KeysPosition PieChart::keysPosition() const
{
    return d->keysPosition;
}

void PieChart::setKeysPosition(PieChart::KeysPosition keysPosition)
{
    d->keysPosition = Left;//keysPosition;

    update();
}

int PieChart::spacing() const
{
    return d->spacing;
}

void PieChart::setSpacing(int spacing)
{
    d->spacing = spacing;
}

int PieChart::height() const
{
    return d->height;
}

void PieChart::setHeight(int height)
{
    d->height = height;
}

void PieChart::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont font = QFont("Trebuchet MS", 8);//MCH
    p.setFont(font);

    QRect pieRect = event->rect();

    if (d->showKeys)
    {
        d->drawKeys(&p, pieRect);

        switch(d->keysPosition)
        {
            case Left:
                pieRect.setLeft(pieRect.left() + d->sizeKeys(&p).width());
                break;

            case Top:
                pieRect.setTop(pieRect.top() + d->sizeKeys(&p).height());
                break;

            case Right:
                pieRect.setRight(pieRect.right() - d->sizeKeys(&p).width());
                break;

            case Bottom:
                pieRect.setBottom(pieRect.bottom() - d->sizeKeys(&p).height());
                break;
        }
    }

    pieRect.setLeft(pieRect.left() + d->spacing);
    pieRect.setRight(pieRect.right() - d->spacing);
    pieRect.setTop(pieRect.top() + d->spacing);
    pieRect.setBottom(pieRect.bottom() - d->spacing);

    double start = 0;
    double totalPixels = M_PI * pieRect.width();
    const int tk = d->height; //thickness of the 3D chart
    foreach (Private::PieChartSlice *slice, d->slices)
    {
        QRectF copyPieRect(pieRect);

        //Set the "perspective".  should this be parameterized?
        copyPieRect.setTop(copyPieRect.top()+pieRect.height()/4);
        copyPieRect.setBottom(copyPieRect.bottom()-pieRect.height()/4);

        double slicePercent = (slice->percent / 100.0) * 360;
        double oldSlicePercent = slicePercent;

        // We need to recalculate angle because of borders
        double localPixels = (slicePercent * totalPixels) / 360.0;
        double localPixelsMod = localPixels - (p.pen().width());
        slicePercent = (slicePercent * localPixelsMod) / localPixels;

        if (!start)
        {
            start += ((abs(slicePercent - oldSlicePercent)) / 2.0);
        }

        // Resize the chart to fit on the widget placement
        copyPieRect.setLeft(copyPieRect.left() + d->maxPenWidth);
        copyPieRect.setRight(copyPieRect.right() - d->maxPenWidth);
        copyPieRect.setTop(copyPieRect.top() + d->maxPenWidth);
        copyPieRect.setBottom(copyPieRect.bottom() - d->maxPenWidth);

        // Draw the slice
        //draw the 3D outline for exposed slices
        if (slicePercent+start>=180){
                double startT=start;
                if (start < 180) startT=180;
                QPainterPath back;
                back.setFillRule(Qt::WindingFill);
                back.arcMoveTo(copyPieRect,startT);
                back.arcTo(copyPieRect, startT,slicePercent-(startT - start));
                back.lineTo(back.currentPosition().x(),back.currentPosition().y()+tk);
                back.arcTo(copyPieRect.translated(0,tk), startT+slicePercent-(startT - start),0-slicePercent+(startT-start));
                back.lineTo(back.currentPosition().x(),back.currentPosition().y()-tk);

                QBrush aBitDarker(slice->brush);
                aBitDarker.setColor(aBitDarker.color());//.darker());
                p.setBrush(aBitDarker);
                p.drawPath(back);
        }
        //Draw the top of the chart.
        //I used QPainterPath also here to get a better alignment with the outline
        //(probably it is unnecessary)
        QPainterPath top;
        top.moveTo(copyPieRect.center());
        top.arcTo(copyPieRect, start,slicePercent);
        top.lineTo(copyPieRect.center());
        p.setBrush(slice->brush);
        p.drawPath(top);

        start += oldSlicePercent;
    }

    //fog TODO: polish and better alignment
    QRectF copyPieRect(pieRect);
    copyPieRect.setTop(copyPieRect.top()+pieRect.height()/4);
    copyPieRect.setBottom(copyPieRect.bottom()-pieRect.height()/4);
    copyPieRect.setBottom(copyPieRect.bottom() -1);
    copyPieRect.setTop(copyPieRect.top() +1);
    copyPieRect.setLeft(copyPieRect.left() +1);
    copyPieRect.setRight(copyPieRect.right() -1);
    QLinearGradient shadow(0,copyPieRect.top(),0,copyPieRect.bottom());
    shadow.setColorAt(0,QColor(0,0,0,155));
    shadow.setColorAt(1,QColor(255,255,255,50));
    p.setBrush(shadow);
    p.setPen(Qt::NoPen);
    p.drawEllipse(copyPieRect);

    //Fog2
    QPainterPath back;
    back.setFillRule(Qt::WindingFill);
    back.arcMoveTo(copyPieRect,180);
    back.arcTo(copyPieRect, 180,180);
    back.lineTo(back.currentPosition().x(),back.currentPosition().y()+tk);
    back.arcTo(copyPieRect.translated(0,tk), 360,-180);
    back.lineTo(back.currentPosition().x(),back.currentPosition().y()-tk);
    QLinearGradient shadow2(copyPieRect.width()/3,0,copyPieRect.right(),0);
    shadow2.setSpread(QGradient::ReflectSpread);
    shadow2.setColorAt(0,QColor(0,0,0,0));
    shadow2.setColorAt(0.3,QColor(0,0,0,125));
    shadow2.setColorAt(1,QColor(0,0,0,200));
    p.setBrush(shadow2);
    p.setPen(Qt::NoPen);
    p.drawPath(back);
}
