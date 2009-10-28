/**************************************************************************
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

#include "print-dev.h"

#include <QString>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QTextCodec>

bool PrintDEV::printSmallBalance(const QString &dev, const QString &codec, const QString &lines)
{
  QFile file(printerFile);
  if (file.open(QIODevice::ReadWrite)) {
    QTextStream out(&file);
    if (printerCodec.length() != 0) out.setCodec(QTextCodec::codecForName(printerCodec.toLatin1()));
    else out.setCodec(QTextCodec::codecForName("UTF-8"));
    out << "\x1b\x4b\x30";              // Feed back x30 dot lines
    out << "\x1b\x4b\x20";              // Feed back x20 dot lines
    out << lines.join("\n");    // Print data
    out << "\x1b\x64\x06";              // Feed 6 lines
    file.close();
  } else qDebug()<<"ERROR: Could not open printer port:"<<printerFile;
}

