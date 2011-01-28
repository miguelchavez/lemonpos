/***************************************************************************
 *   Copyright (C) 2009 by Miguel Chavez Gamboa                            *
 *   miguel@lemonpos.org                                                   *
 *                                                                         *
 *   This is based on the KLineEdit class                                  *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General  Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#ifndef MIBITNUMEDIT_H
#define MIBITNUMEDIT_H

#include <QLineEdit>
class QTimer;
class QIntValidator;
class QDoubleValidator;


/**
 * @short MibitNumEdit - A line edit with number oriented features.
 * @author Miguel Chavez Gamboa <miguel@lemonpos.org>
 * @version 0.1
 * @date Created at December 30th 2010.
 *
 * This makes the line edit display a grayed-out hinting text as long as
 * the user didn't enter any text. It is often used as indication about
 * the purpose of the line edit.
 * The numeric value is formated, according to the locale. And when empty
 * a '0' or '0.0' or '0,0' is drawn with a prepended '$' if its money.
 *
 * Also, the user can add, substract, multiply and divide using the +,-
 * *,/,= keys of the numberic keypad (only?) like a calculator.
 * After an operation (=), the result is set in the line edit and also is
 * kept in memory the last result to allow make another calculation.
 * @warning: The calculator function is very experimental and buggy.
 *
 * The limitations this has is that the numeric value (or text) cannot be
 * edited in parts, cannot be selected all/some text. The value is stored
 * on a member variable and is cleared when any number is entered after it
 * looses the focus (and gains focus again), and if you want to correct the
 * number, you must enter it all again.
 */
class MibitNumEdit : public QLineEdit
{
  Q_OBJECT
  public:
    explicit MibitNumEdit(const bool &setDouble=false, QWidget *parent = 0 );
    virtual ~MibitNumEdit();
    /**
      * sets background color to indicate an error on input.
      */
    void setError( const QString& msg );

    /**
      * sets automatic clear of errors
      */
    void setAutoClearError( const bool& state );

    /**
      * sets the prepending '$' to the formated text.
      **/
    void setIsMoney(const bool &yes);

    /**
      * sets if the number is double
      **/
    void setIsDouble(const bool &yes);

    /**
      * sets the prepending '$' to the formated text, if false, is appended.
      **/
    void setPrependCurrencySymbol(const bool &yes);

    /**
      * enables or disables the calculator function.
      **/
    void setCalculatorEnabled(const bool &yes);

    /**
     * Passes a custom currency symbol.
     **/
    void setCurrencySymbol(const QString &symbol);

    /**
     * Sets the values.
     **/
    void setValue(const double &val) { dValue = val; iValue= int(val); onTextChange(QString::number(val)); }

    void colorize();
    
    /**
      *  Gets the numeric value.
      **/
    QVariant getValue();

    /**
      *  Gets the string value, localized.
      *  Do not use text(), it will return an empty string or a single digit number (the temp string).
      **/
    QString getText();

  protected:
    virtual void paintEvent( QPaintEvent *ev );
    virtual void focusInEvent( QFocusEvent *ev );
    virtual void focusOutEvent( QFocusEvent *ev );
    virtual void keyPressEvent( QKeyEvent * event );

  private:
    bool    drawEmptyMsg;
    bool    drawError;
    bool    autoClear;
    bool    calcEnabled;
    int     actualColor;
    QTimer  *timer;
    QTimer  *shakeTimer;
    int shakeTimeToLive;
    bool par;
    unsigned int parTimes;
    bool isDouble;
    bool isMoney;
    double dValue;
    qulonglong iValue;
    QString currencySymbol;
    bool prependCurrSymbol; //defines if the currency symbol is prepended or appended.
    QString tmp;
    double stackValue;
    int lastKey; // 1:+  2:-  3:*  4:/  5:=  6:isNUM
    int currKey; //the same as above.
    int lastOp; //the last operation before currentOperation
    int currOp;
    QString opSymbol;
    QDoubleValidator *doubleValidator;
    QIntValidator *intValidator;

  private slots:
    void    onTextChange(const QString &text);
    void    clearError();
    void    stepColors();
    void shakeIt();
public slots:
    void shake();

signals:
    void plusKeyPressed();
};

#endif // MIBITNUMEDIT_H
