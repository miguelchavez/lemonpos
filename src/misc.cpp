/**************************************************************************
*   Copyright © 2007-2010 by Miguel Chavez Gamboa                         *
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

#include "misc.h"
#include <QByteArray>
#include <QBuffer>
#include <QIODevice>
#include <QPixmap>
#include <QSize>

QByteArray Misc::pixmap2ByteArray(QPixmap *pix)
{
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  int max = 150;

  if ((pix->height() > max) || (pix->width() > max) ) {
    QPixmap newPix;
    if (pix->height() == pix->width()) {
      newPix = pix->scaled(QSize(max, max));
    }
    else if (pix->height() > pix->width() ) {
      newPix = pix->scaledToHeight(max);
    }
    else  {
      newPix = pix->scaledToWidth(max);
    }
    if (newPix.hasAlpha()) { newPix.save(&buffer, "PNG"); }
    else newPix.save(&buffer, "JPG");
  }
  else {
    //NO scaling needed...
    if (pix->hasAlpha()) { pix->save(&buffer, "PNG"); }
    else pix->save(&buffer, "JPG");
  }

  return bytes;
}

QByteArray Misc::pixmap2ByteArray(QPixmap *pix, int maxW, int maxH)
{
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);

  if ((pix->height() > maxH) || (pix->width() > maxW) ) {
    QPixmap newPix;
    if (pix->height() == pix->width()) {
      newPix = pix->scaled(QSize(maxW, maxH));
    }
    else if (pix->height() > pix->width() ) {
      newPix = pix->scaledToHeight(maxH);
    }
    else  {
      newPix = pix->scaledToWidth(maxW);
    }
    if (newPix.hasAlpha()) { newPix.save(&buffer, "PNG"); }
    else newPix.save(&buffer, "JPG");
  }
  else {
    //NO scaling needed...
    if (pix->hasAlpha()) { pix->save(&buffer, "PNG"); }
    else pix->save(&buffer, "JPG");
  }

  return bytes;
}
