
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#if defined(HAVE_QT)

#include "rdbInfoWidget.h"

#include <stdio.h>

namespace rdb
{

InfoWidget::InfoWidget (QWidget *parent)
  : QTextBrowser (parent)
{
#if QT_VERSION >= 0x040300
  setOpenLinks (false);
#endif
}

void 
InfoWidget::set_image (const QImage &image)
{
  m_image = image;

  int overview_width = 200;

  int w = std::min (image.width (), overview_width);
  int h = (image.height () * w) / image.width ();

  m_overview_image = image.scaled (QSize (w, h), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QVariant 
InfoWidget::loadResource (int type, const QUrl &name)
{
  if (type == QTextDocument::ImageResource && name.isRelative () && name.path () == QString::fromUtf8 ("item.image")) {
    return QVariant (m_image);
  } else if (type == QTextDocument::ImageResource && name.isRelative () && name.path () == QString::fromUtf8 ("item.overview-image")) {
    return QVariant (m_overview_image);
  } else {
    return QTextBrowser::loadResource (type, name);
  }
}

}

#endif
