/***************************************************************************
 *   Copyright (C) 2009 by Miguel Chavez Gamboa                            *
 *   miguel@lemonpos.org                                                   *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General  Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General  Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General  Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "mibitfloatpanel.h"

#include <QPixmap>
#include <QString>
#include <QHBoxLayout>
#include <QTimeLine>
#include <QTimer>
#include <QMouseEvent>
#include <QEvent>
#include <QDebug>

MibitFloatPanel::MibitFloatPanel(QWidget *parent, const QString &file, PanelPosition position, const int &w, const int &h)
        : QSvgWidget( parent )
{
    //setMouseTracking(true);

    if (file != 0) setSVG(file);

    m_position = position;
    m_mode     = pmAuto;
    m_parent = parent;
    m_fileName = file;
    canBeHidden = false; //at the begining is hidden.
    setMinimumHeight(h);
    setMinimumWidth(w);
    setMaxHeight(h);
    setMaxWidth(w);
    setFixedHeight(0);
    animRate = 500; //default animation speed (half a second rate).

    hLayout    = new QHBoxLayout();
    setLayout(hLayout);

    //Postition it on its place
    reposition();

    timeLine  = new QTimeLine(animRate, this);
    connect(timeLine, SIGNAL(frameChanged(int)), this, SLOT(animate(int)));
    connect(timeLine,SIGNAL(finished()), this, SLOT(onAnimationFinished()));
}

void MibitFloatPanel::reposition()
{
    QRect windowGeom = m_parent->geometry();
    int midPointX = (windowGeom.width()/2);
    int midPointY = (windowGeom.height()/2);
    int newX; int newY;
    QRect dRect;
    if ((midPointX-(maxWidth/2)) < 0) newX = 0; else newX = midPointX - (maxWidth/2);
    if ((midPointY-(maxHeight/2)) < 0) newY = 0; else newY = midPointY - (maxHeight/2);

    switch (m_position) {
        case Top:
            newY = 10-maxHeight;
        break;
        case Bottom:
            newY = m_parent->height()+height()-10;
        break;
        case Left:
            newX = 10-maxWidth;
        break;
        case Right:
            newX = m_parent->width()-10;
        break;
    }

    dRect.setX(newX);
    dRect.setY(newY);
    setFixedWidth(maxWidth); //width maybe is not yet defined.
    setFixedHeight(maxHeight);
    setGeometry(dRect);
}

void MibitFloatPanel::addWidget(QWidget * widget)
{
    hLayout->addWidget(widget, 1, Qt::AlignCenter);
}

void MibitFloatPanel::showPanel()
{
    if (timeLine->state() == QTimeLine::NotRunning && !canBeHidden) {
        setGeometry(-1000,-1000,0,0);
        show();
        //update steps for animation, now that the panel is showing.
        int maxStep; int minStep = 0;

        switch (m_position) {
            case Top :
                minStep = -maxHeight+10;
                maxStep =  -6;
            break;
            case Bottom:
                maxStep = m_parent->geometry().height()-maxHeight;
                minStep = m_parent->geometry().height()-10;
            break;
            case Left:
                minStep = -maxWidth+10;
                maxStep = -6;
            break;
            case Right:
                minStep = m_parent->geometry().width()-10;
                maxStep = m_parent->geometry().width()-maxWidth;
            break;
            default:
            break;
        }

        timeLine->setFrameRange(minStep,maxStep);
        //make it grow
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();

    } 
}

void MibitFloatPanel::animate(const int &step)
{
    //get some sizes...
    QRect windowGeom = m_parent->geometry();
    int midPointX = (windowGeom.width()/2);
    int midPointY = (windowGeom.height()/2);
    int newX;  int newY;
    QRect dRect;
    if ((midPointX-(maxWidth/2)) < 0) newX = 0; else newX = midPointX - (maxWidth/2);
    if ((midPointY-(maxHeight/2)) < 0) newY = 0; else newY = midPointY - (maxHeight/2);


    switch (m_position) {
        case Bottom:
        case Top:
            dRect.setX(newX);
            dRect.setY(step);
        break;
        case Left:
        case Right:
            dRect.setY(newY);
            dRect.setX(step);
        break;
        default:
        break;
    }

    setFixedHeight(maxHeight);
    setFixedWidth(maxWidth);
    setGeometry(dRect);
}

void MibitFloatPanel::hideOnUserRequest()
{
    hideDialog();
    emit hiddenOnUserRequest();
}

void MibitFloatPanel::hideDialog()
{
    if ( canBeHidden ) {
        timeLine->setDirection(QTimeLine::Backward); //reverse!
        timeLine->start();
    }
}

void MibitFloatPanel::onAnimationFinished()
{
    if (timeLine->direction() == QTimeLine::Forward) {
        canBeHidden = true;
    } else canBeHidden = false;
}

void MibitFloatPanel::setPosition(const PanelPosition pos)
{
    // only changes the position when the notification is not showing..
    if (timeLine->state() == QTimeLine::NotRunning && !canBeHidden) {
        m_position = pos;
        //recalculate its rect and show it there...
        reposition();
    }
}


//NOTE: The svg file is not rendered correctly. It does not render the blur, i.e. the shadows
void MibitFloatPanel::setSVG(const QString &file)
{
    load(file);
}


void MibitFloatPanel::enterEvent ( QEvent * )
{
    if (m_mode == pmAuto) QTimer::singleShot(100,this,SLOT(showPanel()));
}

void MibitFloatPanel::leaveEvent( QEvent * )
{
    if (m_mode == pmAuto) QTimer::singleShot(100,this,SLOT(hideOnUserRequest()));
}

void MibitFloatPanel::keyPressEvent ( QKeyEvent * event )
{
    if ( event->key() == Qt::Key_Escape )
    {
        if (m_mode == pmManual)
        hideOnUserRequest();
    } //else ignore event.
}

MibitFloatPanel::~MibitFloatPanel() {}


