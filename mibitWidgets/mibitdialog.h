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

#ifndef MIBITDIALOG_H
#define MIBITDIALOG_H

#include <QSvgWidget>
class QTimeLine;
class QString;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QPixmap;
class QPushButton;

/**
  * This class is used to display animated dialogs appering on screen's
  * top or middle. Are svg themed and borderless.
  * It can also shake it or wave it to take user's attention.
  *
  * The animation types are four:
  *   datGrowCenterV: This makes the dialog appear growing from the center in the Y axe.
  *   datGrowCenterH: This makes the dialog appear growing from the center in the X axe.
  *   datSlideDown:   This makes the dialog appear sliding down from the top of its parent.
  *   datSlideUp:     This makes the dialog appear sliding up from the bottom of its parent.
  *
  *
  */

enum AnimationType { datGrowCenterV=1, datGrowCenterH=2, datSlideDown=3, datSlideUp=4 };
enum Sizes         { dmaxH=300, dmaxW=400 };


class MibitDialog : public QSvgWidget
{
Q_OBJECT
public:
    MibitDialog(QWidget *parent = 0, const QString &file = 0, /*const QPixmap &icon = 0,*/ AnimationType animation = datSlideDown );
    ~MibitDialog();

    void addWidget(QWidget * widget);
    void reParent(QWidget *newparent);

    void showDialog( AnimationType animation = datSlideDown );
    // Tratar de hacer un metodo similar al QDialog::getDouble()... con static
    void setSVG(const QString &file);
    //void setIcon(const QPixmap &icon);

    void setAnimationType(const AnimationType &atype) { animType = atype; }
    void setAnimationRate(const int &r) { animRate = r; }
    void setMaxHeight(const int &m)   { setMaximumHeight(m); dmaxHeight = m; }
    void setMaxWidth(const int &m)    { setMaximumWidth(m); dmaxWidth = m;   }
    void setSize(const int &w, const int &h) { setMaxWidth(w); setMaxHeight(h);    }
    void setShakeTTL(const int &timeToLive = 0){ shakeTimeToLive = timeToLive;} //timeToLive = 0 means shake until closed.
private:
    QTimeLine *timeLine;
    QTimeLine *wTimeLine;
    QTimer *shakeTimer;
    //QHBoxLayout *hLayout;
    QVBoxLayout *vLayout;
    //QLabel *img;
    AnimationType animType;
    QWidget *m_parent;
    int dmaxWidth;
    int dmaxHeight;
    int animRate;
    int shakeTimeToLive;
    bool par;
    unsigned int parTimes;

private slots:
    void animate(const int &step);
    void shakeIt();
    void waveIt(const int &step);
    void onAnimationFinished();
public slots:
    void shake();
    void wave();
    void hideDialog();
protected:
    void keyPressEvent ( QKeyEvent * event );

};

#endif // MIBITDIALOG_H
