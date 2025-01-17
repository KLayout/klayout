
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "layBackgroundAwareTreeStyle.h"

#include <QPainter>
#include <QStyleOption>

namespace lay
{

// -------------------------------------------------------------
//  BackgroundAwareTreeStyle implementation

BackgroundAwareTreeStyle::BackgroundAwareTreeStyle (QStyle *org_style)
  : QProxyStyle (org_style)
{
  //  .. nothing yet ..
}

void
BackgroundAwareTreeStyle::drawPrimitive (QStyle::PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const
{
  if (pe == PE_IndicatorBranch) {

    static const int sz = 9;

    int mid_h = opt->rect.x () + opt->rect.width () / 2;
    int mid_v = opt->rect.y () + opt->rect.height () / 2;

    if (opt->state & State_Children) {

      QColor c;

      QPalette::ColorGroup cg = QPalette::Disabled;
      if ((w && w->isEnabled ()) || (!w && (opt->state & State_Enabled))) {
        if ((w && w->hasFocus ()) || (!w && (opt->state & State_HasFocus))) {
          cg = QPalette::Normal;
        } else {
          cg = QPalette::Inactive;
        }
      }
      if ((opt->state & State_Selected) && styleHint(QStyle::SH_ItemView_ShowDecorationSelected, opt, w)) {
        c = opt->palette.color (cg, QPalette::HighlightedText);
      } else {
        c = opt->palette.color (cg, QPalette::Text);
      }
      if (! (opt->state & State_MouseOver)) {
        if (c.green () < 128) {
          c = QColor ((c.red () * 2 + 255) / 3, (c.green () * 2 + 255) / 3, (c.blue () * 2 + 255) / 3);
        } else {
          c = QColor ((c.red () * 8) / 9, (c.green () * 8) / 9, (c.blue () * 8) / 9);
        }
      }

      QPen old_pen = p->pen ();
      p->setPen (Qt::NoPen);
      QBrush old_brush = p->brush ();
      p->setBrush (c);
      QPainter::RenderHints old_rh = p->renderHints ();
      p->setRenderHints (QPainter::Antialiasing);

      if (opt->state & State_Open) {
        QPoint points[] = {
          QPoint (mid_h - sz / 2, mid_v - sz / 3),
          QPoint (mid_h + sz / 2, mid_v - sz / 3),
          QPoint (mid_h, mid_v + sz / 3)
        };
        p->drawPolygon (points, sizeof (points) / sizeof (points[0]));
      } else {
        QPoint points[] = {
          QPoint (mid_h - sz / 3, mid_v - sz / 2),
          QPoint (mid_h + sz / 3, mid_v),
          QPoint (mid_h - sz / 3, mid_v + sz / 2)
        };
        p->drawPolygon (points, sizeof (points) / sizeof (points[0]));
      }

      p->setPen (old_pen);
      p->setBrush (old_brush);
      p->setRenderHints (old_rh);
      return;

    }

  }

  QProxyStyle::drawPrimitive (pe, opt, p, w);
}

}

#endif

