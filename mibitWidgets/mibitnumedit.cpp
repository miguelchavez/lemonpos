/***************************************************************************
 *   Copyright (C) 2009 by Miguel Chavez Gamboa                            *
 *   miguel@lemonpos.org                                                   *
 *                                                                         *
 *   This is based on the KLineEdit class                                  *
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

#include "mibitnumedit.h"

#include <QtGui>
#include <QTimer>
#include <QLocale>
#include <QIntValidator>
#include <QDoubleValidator>

#include <locale.h>

MibitNumEdit::MibitNumEdit(const bool &setDouble, QWidget *parent)
    : QLineEdit( parent )
{
    isMoney  = false;
    calcEnabled = false;
    iValue = 0;
    dValue = 0;
    tmp = "";
    lastKey = 0; //none
    currKey = 0; //none
    lastOp  = 0; //none
    opSymbol = "";
    actualColor  = 0;
    timer = new QTimer(this);
    shakeTimeToLive = 0;
    par = false; parTimes = 0;

    //crating both validators
    doubleValidator = new QDoubleValidator(this);
    intValidator    = new QIntValidator(this);
    
    setIsDouble(setDouble);

    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(20);
    timer->setInterval(30);
    connect(this, SIGNAL(textEdited( const QString & ) ), SLOT(onTextChange(const QString &)) );
    connect(timer, SIGNAL(timeout()), this, SLOT(stepColors()));
    connect(shakeTimer,SIGNAL(timeout()), this, SLOT(shakeIt()));

    //get curency symbol from locale... test this! maybe its the system locale and its not configured (default will be C locale which is for US)
    struct lconv * lc;
    lc = localeconv();
    currencySymbol = lc->currency_symbol;
    qDebug()<<" <mibitNumEdit> Currency Symbol reported from locale:"<< currencySymbol;
}

MibitNumEdit::~MibitNumEdit ()
{
}

void MibitNumEdit::setIsMoney(const bool &yes)
{
    isMoney = yes;
}

void MibitNumEdit::setIsDouble(const bool &yes)
{
    isDouble = yes;

    if ( isDouble )
        setValidator(doubleValidator);
    else
        setValidator(intValidator);
}

void MibitNumEdit::setCalculatorEnabled(const bool &yes)
{
    calcEnabled = yes;
}

void MibitNumEdit::setPrependCurrencySymbol(const bool &yes)
{
    prependCurrSymbol = yes;
}

void MibitNumEdit::paintEvent( QPaintEvent *ev )
{
    QLineEdit::paintEvent( ev );
    QString newText;
    QLocale locale = QLocale::system();
    QPainter p(this);
    QFont f = font();

    QStyleOptionFrame opt;
    initStyleOption(&opt);
    QRect cr = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);
    cr.setLeft(cr.left() + 2);
    cr.setRight(cr.right() - 2);

    if ( !opSymbol.isEmpty() && calcEnabled ) {
        f.setBold(true);
        p.setFont(f);
        QColor color( Qt::red );
        color.setAlphaF(0.5);
        p.setPen(color);
        p.drawText(cr, Qt::AlignLeft|Qt::AlignVCenter, opSymbol);
    }
    //dValue and iValue contains the same but iValue is truncated.
    if ( dValue  <= 0) {
        //draw a 0.0 string according to locale and type (money).
        newText = "0";
        newText = isDouble ? locale.toString(newText.toDouble(),'f',2) : locale.toString(newText.toInt());

        //If the text is empty, draw it grayed out
        f.setItalic(true);
        f.setBold(false);
        p.setFont(f);
        QColor color(palette().color(foregroundRole()));
        color.setAlphaF(0.5);
        p.setPen(color);
    } else {
        newText = isDouble ? locale.toString( dValue ,'f',2) : locale.toString( iValue );
    }

    //now check prepending or apending curr. symbol if is money.
    if ( isMoney )
        newText = ( prependCurrSymbol ) ? newText.prepend(currencySymbol+" ") : newText.append(" "+currencySymbol);

    if ( hasFocus() ) {
        f.setItalic(true);
        f.setBold(false);
        p.setFont(f);
        QColor color(palette().color(foregroundRole()));
        color.setAlphaF(0.5);
        p.setPen(color);
        p.drawText(cr, Qt::AlignRight|Qt::AlignVCenter, newText); //align right for numeric values is ok?  i think so!
    }

    if ( !hasFocus() ) {
        p.drawText(cr, Qt::AlignRight|Qt::AlignVCenter, newText); //align right for numeric values is ok?  i think so!
        setText("");
    }

}


void MibitNumEdit::setError( const QString& msg )
    {
        timer->start(); //timer for colors
        //set tooltip
        setToolTip(msg);
        if (autoClear) {
            //create a timer to clear the error.
            QTimer::singleShot(5000,this,SLOT(clearError()));
        }
    }

void MibitNumEdit::colorize()
{
    // TODO: Code THIS!
}

void MibitNumEdit::stepColors()
{
    if (actualColor > 199) {
        actualColor=0;
        timer->stop();
        return;
    } else {
        actualColor=actualColor+2;
    }
    setStyleSheet(QString(" QLineEdit { background: rgb(255,%1,0); color:white; font-weight: bold;}").arg(actualColor));
}

void MibitNumEdit::clearError( )
{
    setStyleSheet("");
    setToolTip("");
}

///This function needs to be called before the setError() one... to enable the Qtimer::singleShot
void MibitNumEdit::setAutoClearError( const bool& state )
{
    autoClear = state;
}

void MibitNumEdit::setCurrencySymbol(const QString &symbol)
{
    currencySymbol = symbol;
}

void MibitNumEdit::onTextChange(const QString & text)
{
    //clear the error
    clearError();
    //save numeric value
    if ( !text.isEmpty() ) {
        if ( dValue != 0 && text !="0" && text.toDouble() == 0 ) {
            //When calcEnabled=false, the +,-... keys are not filtered bt the keyPressEvent() method. Remove it here.
            setText("");
            return;
        }
        tmp.append(text);
        iValue = tmp.toInt();
        dValue = tmp.toDouble();
        setText("");
    }

    if ( !calcEnabled ) return; //it will never reach this part if caclEnabled=false, the if above exits the method.

    //save stack if needed
    bool isOperation = (currKey > 0 && currKey < 6) ? true : false;

    qDebug()<<"isOperation="<<isOperation<<" CurrentKey:"<<currKey<<"lastKey:"<<lastKey;

    if ( lastKey == 6 && isOperation ) {
        if ( currKey == 1 ) {
            stackValue += dValue;
            qDebug()<<" Adding "<<dValue<<" to stack, now is:"<<stackValue;
        }
        else if ( currKey == 2 ) {
            if (stackValue == 0)
                stackValue = dValue; //TODO: test and review this
            else
                stackValue -= dValue;
            qDebug()<<" Substracting "<<dValue<<" to stack, now is:"<<stackValue;
        }
        else if ( currKey == 3 ) {
            if (stackValue == 0) stackValue=1; //to avoid multiplying by 0.
            stackValue *= dValue;//TODO: test and review this
            qDebug()<<" Multiplying by "<<dValue<<" the stack, now is:"<<stackValue;
        }
        else if ( currKey == 4 ) {
            if (stackValue == 0)
                stackValue = dValue / 1; //to avoid dividing by 0.
            else
                stackValue /= dValue;
            qDebug()<<" Dividing by "<<dValue<<" the stack, now is:"<<stackValue;
        }
        else if ( currKey == 5 ) {
            qDebug()<<" Getting result, the operation Will be:"<<lastOp;
            if ( lastOp == 1 )
                dValue = stackValue + dValue;
            else if ( lastOp == 2 )
                dValue = stackValue - dValue;
            else if ( lastOp == 3 )
                dValue = stackValue * dValue;
            else if ( lastOp == 4 )
                dValue = stackValue / dValue;
            //reset last operator after an ENTER
            lastOp = 0;
            //FALTAN LOS DEMAS!
            iValue = dValue;
            stackValue = dValue;
            qDebug()<<" RESULT ="<<dValue;
        }
    }
}

void MibitNumEdit::keyPressEvent ( QKeyEvent * event )
{
  // Check for our special keys
  // The Enter key is the one located at the numeric pad. The other is called RETURN.

    if (!calcEnabled) {
        qDebug()<<"- dValue:"<<dValue;
        QLineEdit::keyPressEvent(event);
        qDebug()<<"+ dValue:"<<dValue;
        return;
    }
  
    if ( event->key() == Qt::Key_Plus && calcEnabled ) {
        lastKey = currKey;
        currKey = 1; //sum
        lastOp = currOp;
        currOp = currKey;
        tmp.clear();
        //qDebug()<<" <SUMA> ";
        opSymbol = "+";
        onTextChange("");
    }
    else if ( event->key() == Qt::Key_Minus && calcEnabled ) {
        qDebug()<<"\n";
        lastKey = currKey;
        currKey = 2; //substraction
        lastOp = currOp;
        currOp = currKey;
        tmp.clear();
        //qDebug()<<" <RESTA> ";
        opSymbol = "-";
        onTextChange("");
    }
    else if ( event->key() == Qt::Key_Asterisk && calcEnabled ) {
        qDebug()<<"\n";
        lastKey = currKey;
        currKey = 3; //Multiplication
        lastOp = currOp;
        currOp = currKey;
        tmp.clear();
        //qDebug()<<" <MULT> ";
        opSymbol = "*";
        onTextChange("");
    }
    else if ( event->key() == Qt::Key_Slash && calcEnabled) {
        qDebug()<<"\n";
        lastKey = currKey;
        currKey = 4; //Division
        lastOp = currOp;
        currOp = currKey;
        tmp.clear();
        //qDebug()<<" <DIV> ";
        opSymbol = "/";
        onTextChange("");
    }
    else if ( (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && calcEnabled ) {
        qDebug()<<"\n";
        lastKey = currKey;
        currKey = 5; // TOTAL
        lastOp = currOp;
        currOp = currKey;
        qDebug()<<" <IGUAL> ";
        opSymbol = "";
        onTextChange("");
    }
    else if ( (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9 ) && calcEnabled ) {
        qDebug()<<"\n<NUMBER> "<<QString::number( event->key() ).toInt() - 0x30;
        lastKey = currKey;
        currKey = 6; // isNUM
        opSymbol = "";
        QLineEdit::keyPressEvent(event);
    }
    else if ( event->key() == Qt::Key_Backspace ) { //This is for deleting the last digit in the string stack
        lastKey = currKey;
        currKey = 0; // none
        tmp.chop(1);
        iValue = tmp.toInt();
        dValue = tmp.toDouble();
        opSymbol = "";
        setText("");
    } else if ( event->key() == Qt::Key_Cut ) {  //This is for deleting the whole number string stack
        lastKey = currKey;
        currKey = 0; // none
        tmp.clear(); //but do not copy to clipboard.
        iValue = tmp.toInt();
        dValue = tmp.toDouble();
        opSymbol = "";
        setText("");
    } else {
        lastKey = currKey;
        currKey = 0; // none
        opSymbol = "";
        qDebug()<<"PressEvent passed to parent class";
        QLineEdit::keyPressEvent(event);
    }
}

void MibitNumEdit::focusInEvent( QFocusEvent *ev )
{
    tmp.clear();
    if ( drawEmptyMsg )
    {
        drawEmptyMsg = false;
        update();
    }
    QLineEdit::focusInEvent( ev );
}

void MibitNumEdit::focusOutEvent( QFocusEvent *ev )
{
    tmp.clear();
    if ( text().isEmpty() )
    {
        drawEmptyMsg = true;
        update();
    }
    QLineEdit::focusOutEvent( ev );
}

void MibitNumEdit::shake()
{
    shakeTimer->start();
    QTimer::singleShot(3000,shakeTimer,SLOT(stop()));
}

void MibitNumEdit::shakeIt()
{
    if (par) {
        if (parTimes < 5) {
            if ( parTimes % 2 == 0 )
                setGeometry(geometry().x()-3, geometry().y()+3, geometry().width(), geometry().height());
            else
                setGeometry(geometry().x()+3, geometry().y()+3, geometry().width(), geometry().height());
        }
        parTimes++;
        if (parTimes >39) {
            parTimes = 0;
        }
    }
    else {
        if (parTimes < 5) {
            if ( parTimes % 2 == 0 )
                setGeometry(geometry().x()+3, geometry().y()-3, geometry().width(), geometry().height());
            else
                setGeometry(geometry().x()-3, geometry().y()-3, geometry().width(), geometry().height());
        }
    }

    par = !par;
}

QVariant MibitNumEdit::getValue()
{
    if ( isDouble )
        return QVariant( dValue );
    else
        return QVariant( iValue );
}

QString MibitNumEdit::getText()
{
    QLocale locale = QLocale::system();
    QString newText = isDouble ? locale.toString( dValue ,'f',2) : locale.toString( iValue );
    if ( isMoney )
        newText = ( prependCurrSymbol ) ? newText.prepend(currencySymbol+" ") : newText.append(" "+currencySymbol);
    return newText;
}
