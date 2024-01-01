
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "layLayerToolbox.h"

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QToolButton>
#include <QToolBar>
#include <QPushButton>
#include <QCheckBox>

#include "tlExceptions.h"
#include "layLayoutViewBase.h"
#include "layEditStipplesForm.h"
#include "layEditLineStylesForm.h"
#include "laySelectStippleForm.h"
#include "laySelectLineStyleForm.h"

namespace lay
{

class LCPActiveLabel;
class LCPTreeWidget;
class LayerTreeModel;

// --------------------------------------------------------------------
//  LCPRemitter implementation

LCPRemitter::LCPRemitter (int index, QObject *parent, const char *name)
  : QObject (parent),
    m_index (index)
{
  setObjectName (QString::fromUtf8 (name));
}

void 
LCPRemitter::the_slot ()
{
  emit the_signal (m_index);
}

// --------------------------------------------------------------------
//  LCPActiveLabel implementation

LCPActiveLabel::LCPActiveLabel (int index, QWidget *parent, const char *name)
  : QLabel (parent), m_index (index), m_grabbed (false)
{
  setAutoFillBackground (true);
  setObjectName (QString::fromUtf8 (name));
  setFrameStyle (QFrame::Panel | QFrame::Raised);
  setBackgroundRole (QPalette::Window);
}

void 
LCPActiveLabel::mousePressEvent (QMouseEvent *e)
{
  if (! m_grabbed && e->button () == Qt::LeftButton) {
    setFrameShadow (QFrame::Sunken);
    m_grabbed = true;
  }
}

void 
LCPActiveLabel::mouseReleaseEvent (QMouseEvent *e)
{
  if (m_grabbed) {
   
    setFrameShadow (QFrame::Raised);
    m_grabbed = false;

    if (e->button () == Qt::LeftButton && rect ().contains (e->pos ())) {
      emit clicked (m_index);
    }

  }
}

// --------------------------------------------------------------------
//  LCPDitherPalette implementation

LCPDitherPalette::LCPDitherPalette (QWidget *parent, const char *name)
  : QFrame (parent), mp_view (0)
{
  setObjectName (QString::fromUtf8 (name));

  int n = 0;
  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  for (unsigned int i = 0; i < 4; ++i) {

    QFrame *f = new QFrame (this);
    f->setObjectName (QString::fromUtf8 ("dp_f"));
    l->addWidget (f);
    QHBoxLayout *ll = new QHBoxLayout (f);
    ll->setContentsMargins (0, 0, 0, 0);
    ll->setSpacing (0);

    for (unsigned int j = 0; j < 4; ++j) {
      
      LCPActiveLabel *b = new LCPActiveLabel (n, f);

      b->setMinimumSize (28, 28);
      b->setAlignment (Qt::AlignCenter);
      b->setLineWidth (1);

      QSizePolicy sp (QSizePolicy::Ignored, QSizePolicy::Ignored);
      sp.setHorizontalStretch (0);
      sp.setVerticalStretch (0);
      b->setSizePolicy (sp);

      create_pixmap_for (b, n);
      m_stipple_buttons.push_back (b);

      connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));

      ll->addWidget (b);

      ++n;

    }

  }

  {
    QFrame *f = new QFrame (this);
    f->setObjectName (QString::fromUtf8 ("dp_ll"));
    l->addWidget (f);
    QHBoxLayout *ll = new QHBoxLayout (f);
    ll->setContentsMargins (0, 0, 0, 0);
    ll->setSpacing (0);

    LCPActiveLabel *b;

    //  No pattern
    b = new LCPActiveLabel (-3, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setText (QObject::tr ("None"));
    b->setBackgroundRole (QPalette::Button);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

    //  More pattern 
    b = new LCPActiveLabel (-2, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setText (QObject::tr ("More .."));
    b->setBackgroundRole (QPalette::Button);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);
  }

  {
    //  Edit pattern 
    LCPActiveLabel *b;
    b = new LCPActiveLabel (-1, this, "dp_l2");
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setText (QObject::tr ("Custom Pattern .."));
    b->setBackgroundRole (QPalette::Button);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    l->addWidget (b);
  }

}

void
LCPDitherPalette::create_pixmap_for (LCPActiveLabel *b, int n)
{
  lay::DitherPattern pattern = !mp_view ? lay::DitherPattern::default_pattern () : mp_view->dither_pattern ();

  QColor color0 = b->palette ().color (QPalette::Normal, b->backgroundRole ());
  QColor color1 = b->palette ().color (QPalette::Normal, b->foregroundRole ());

  const unsigned int h = 24;
  const unsigned int w = 24;

#if QT_VERSION > 0x050000
  unsigned int dpr = devicePixelRatio ();
#else
  unsigned int dpr = 1;
#endif

  pattern.scale_pattern (dpr);

  QImage image (w * dpr, h * dpr, QImage::Format_RGB32);
  image.fill (color0.rgb ());
#if QT_VERSION > 0x050000
  image.setDevicePixelRatio (dpr);
#endif

  QBitmap bitmap = pattern.pattern (n).get_bitmap (w * dpr, h * dpr, dpr);
  QPainter painter (&image);

  painter.setPen (QPen (color1));
  painter.setBackgroundMode (Qt::TransparentMode);
  painter.drawPixmap (0, 0, w, h, bitmap);

  QPixmap pixmap = QPixmap::fromImage (image);
  b->setPixmap (pixmap);
}

void
LCPDitherPalette::set_palette (const lay::StipplePalette &palette)
{
  if (palette != m_palette) {

    m_palette = palette;

    for (unsigned int i = 0; i < m_stipple_buttons.size (); ++i) {
      unsigned int n = i;
      if (i < m_palette.stipples ()) {
        n = m_palette.stipple_by_index (i);
      }
      if (m_stipple_buttons [i]) {
        create_pixmap_for (m_stipple_buttons[i], n);
      }
    }

  }
}

void 
LCPDitherPalette::button_clicked (int index)
{
  if (! mp_view) {
    return;
  }

  if (index == -1) {
    
    //  edit pattern
    lay::DitherPattern pattern (mp_view->dither_pattern ());
    lay::EditStipplesForm stipples_form (this, mp_view, pattern);
    if (stipples_form.exec () && stipples_form.pattern () != pattern) {
      emit pattern_changed (stipples_form.pattern ());
    }

  } else if (index == -2) {
    
    //  select pattern
    lay::SelectStippleForm stipples_form (0, mp_view->dither_pattern ());
    if (stipples_form.exec () && stipples_form.selected () >= 0) {
      emit dither_selected (int (stipples_form.selected ()));
    }

  } else if (index == -3) {
    
    emit dither_selected (-1);

  } else {
    if (index < int (m_palette.stipples ())) {
      emit dither_selected (m_palette.stipple_by_index (index));
    } else {
      emit dither_selected (index);
    }
  }
}

// --------------------------------------------------------------------
//  LCPVisibilityPalette implementation

LCPVisibilityPalette::LCPVisibilityPalette (QWidget *parent, const char *name)
  : QFrame (parent)
{
  setObjectName (QString::fromUtf8 (name));

  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  QFrame *f;
  LCPActiveLabel *b;
  QHBoxLayout *ll;

  QSizePolicy sp (QSizePolicy::Ignored, QSizePolicy::Ignored);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);

  f = new QFrame (this);
  f->setObjectName (QString::fromUtf8 ("vis_f"));
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (0, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Show"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (1, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Hide"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  f = new QFrame (this);
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (2, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Transp."));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (3, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Opaque"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);
}

void 
LCPVisibilityPalette::button_clicked (int index)
{
  if (index == 0) {
    emit visibility_change (true);
  } else if (index == 1) {
    emit visibility_change (false);
  } else if (index == 2) {
    emit transparency_change (true);
  } else if (index == 3) {
    emit transparency_change (false);
  }
}

// --------------------------------------------------------------------
//  LCPAnimationPalette implementation

LCPAnimationPalette::LCPAnimationPalette (QWidget *parent, const char *name)
  : QFrame (parent)
{
  setObjectName (QString::fromUtf8 (name));

  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  QFrame *f;
  LCPActiveLabel *b;
  QHBoxLayout *ll;

  QSizePolicy sp (QSizePolicy::Ignored, QSizePolicy::Ignored);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);

  f = new QFrame (this);
  f->setObjectName (QString::fromUtf8 ("anim_f"));
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (0, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("None"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (1, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Scroll"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  f = new QFrame (this);
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (2, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("Blink"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (3, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setSizePolicy (sp);
  b->setText (QObject::tr ("/Blink"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);
}

void 
LCPAnimationPalette::button_clicked (int index)
{
  emit animation_selected (index);
}

// --------------------------------------------------------------------
//  LCPStylePalette implementation

LCPStylePalette::LCPStylePalette (QWidget *parent, const char *name)
  : QFrame (parent), mp_view (0)
{
  setObjectName (QString::fromUtf8 (name));

  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  QFrame *f;
  LCPActiveLabel *b;
  QHBoxLayout *ll;

  QSizePolicy sp (QSizePolicy::Ignored, QSizePolicy::Ignored);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);

  f = new QFrame (this);
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  for (int i = 0; i < 4; ++i) {

    b = new LCPActiveLabel (300 + i, f);
    b->setMinimumSize (25, 18);
    b->setAlignment (Qt::AlignCenter);
    b->setLineWidth (1);
    b->setSizePolicy (sp);

    create_pixmap_for_line_style (b, i);
    m_style_buttons.push_back (b);

    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

  }

  f = new QFrame (this);
  f->setObjectName (QString::fromUtf8 ("ls_ll"));
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  //  No style
  b = new LCPActiveLabel (-3, f);
  b->setFrameStyle (QFrame::Panel | QFrame::Raised);
  b->setLineWidth (1);
  b->setText (QObject::tr ("None"));
  b->setBackgroundRole (QPalette::Button);
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  //  More styles
  b = new LCPActiveLabel (-2, f);
  b->setFrameStyle (QFrame::Panel | QFrame::Raised);
  b->setLineWidth (1);
  b->setText (QObject::tr ("More .."));
  b->setBackgroundRole (QPalette::Button);
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  //  Edit pattern
  b = new LCPActiveLabel (-1, this, "ls_l2");
  b->setFrameStyle (QFrame::Panel | QFrame::Raised);
  b->setLineWidth (1);
  b->setText (QObject::tr ("Custom Style .."));
  b->setBackgroundRole (QPalette::Button);
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  l->addWidget (b);

  f = new QFrame (this);
  f->setObjectName (QString::fromUtf8 ("style_f"));
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (0, f);
  b->setMinimumSize (25, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("0px"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (1, f);
  b->setMinimumSize (25, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("1px"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (2, f);
  b->setMinimumSize (25, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("2px"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (3, f);
  b->setMinimumSize (25, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("3px"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  f = new QFrame (this);
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (200, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("No Cross"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (201, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("Cross"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  f = new QFrame (this);
  l->addWidget (f);
  ll = new QHBoxLayout (f);
  ll->setContentsMargins (0, 0, 0, 0);
  ll->setSpacing (0);

  b = new LCPActiveLabel (100, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("Simple"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);

  b = new LCPActiveLabel (101, f);
  b->setMinimumSize (50, 16);
  b->setAlignment (Qt::AlignCenter);
  b->setLineWidth (1);
  b->setText (QObject::tr ("Marked"));
  connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
  ll->addWidget (b);
}

void
LCPStylePalette::set_palette (const LineStylePalette &palette)
{
  if (palette != m_palette) {

    m_palette = palette;

    for (unsigned int i = 0; i < m_style_buttons.size (); ++i) {
      unsigned int n = i;
      if (i < m_palette.styles ()) {
        n = m_palette.style_by_index (i);
      }
      if (m_style_buttons [i]) {
        create_pixmap_for_line_style (m_style_buttons[i], n);
      }
    }

  }
}

void
LCPStylePalette::create_pixmap_for_line_style (LCPActiveLabel *b, int n)
{
  const lay::LineStyles &styles = !mp_view ? lay::LineStyles::default_style () : mp_view->line_styles ();

  QColor color0 = b->palette ().color (QPalette::Normal, b->backgroundRole ());
  QColor color1 = b->palette ().color (QPalette::Normal, b->foregroundRole ());

  //  NOTE: we intentionally don't apply devicePixelRatio here as this way, the
  //  image looks more like the style applied on the layout canvas.

  const unsigned int h = 14;
  const unsigned int w = 24;

  QImage image (w, h, QImage::Format_RGB32);
  image.fill (color0.rgb ());

  QBitmap bitmap = styles.style (n).get_bitmap (w, h);
  QPainter painter (&image);
  painter.setPen (QPen (color1));
  painter.setBackgroundMode (Qt::TransparentMode);
  painter.drawPixmap (0, 0, w, h, bitmap);

  QPixmap pixmap = QPixmap::fromImage (image);
  b->setPixmap (pixmap);
}

void
LCPStylePalette::button_clicked (int index)
{
  if (index >= 0 && index < 16) {
    emit width_selected (index);
  } else if (index == 100) {
    emit marked_selected (false);
  } else if (index == 101) {
    emit marked_selected (true);
  } else if (index == 200) {
    emit xfill_selected (false);
  } else if (index == 201) {
    emit xfill_selected (true);
  } else if (index >= 300 && index < 400) {

    index -= 300;
    if (index < int (m_palette.styles ())) {
      emit line_style_selected (m_palette.style_by_index (index));
    } else {
      emit line_style_selected (index);
    }

  } else if (index == -1) {

    //  edit pattern
    lay::LineStyles styles (mp_view->line_styles ());
    lay::EditLineStylesForm line_styles_form (this, mp_view, styles);
    if (line_styles_form.exec () && line_styles_form.styles () != styles) {
      emit line_styles_changed (line_styles_form.styles ());
    }

  } else if (index == -2) {

    //  select pattern
    lay::SelectLineStyleForm styles_form (0, mp_view->line_styles ());
    if (styles_form.exec () && styles_form.selected () >= 0) {
      emit line_style_selected (int (styles_form.selected ()));
    }

  } else if (index == -3) {
    emit line_style_selected (-1);
  }
}

// --------------------------------------------------------------------
//  LCPColorPalette implementation

LCPColorPalette::LCPColorPalette (QWidget *parent, const char *name)
  : QFrame (parent)
{
  setObjectName (QString::fromUtf8 (name));

  QSizePolicy sp (QSizePolicy::Ignored, QSizePolicy::Ignored);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);

  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);
  for (unsigned int i = 0; i < 6; ++i) {

    QFrame *f = new QFrame (this);
    f->setObjectName (QString::fromUtf8 ("color_f"));
    l->addWidget (f);
    QHBoxLayout *ll = new QHBoxLayout (f);
    ll->setContentsMargins (0, 0, 0, 0);
    ll->setSpacing (0);

    for (unsigned int j = 0; j < 7; ++j) {
      
      unsigned int n = j * 6 + i;

      LCPActiveLabel *b = new LCPActiveLabel (n, f);
      while (m_color_buttons.size () <= n) {
        m_color_buttons.push_back (0);
      }
      m_color_buttons [n] = b;
      b->setMinimumSize (16, 16);
      b->setLineWidth (1);
      b->setSizePolicy (sp);
      b->setText (QString ());
      connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));

      ll->addWidget (b);

    }

  }

  {
    //  darker and brighter colors
    QFrame *f = new QFrame (this);
    f->setObjectName (QString::fromUtf8 ("color_l1"));
    l->addWidget (f);
    QHBoxLayout *ll = new QHBoxLayout (f);
    ll->setContentsMargins (0, 0, 0, 0);
    ll->setSpacing (0);

    LCPActiveLabel *b;

    //  No color
    b = new LCPActiveLabel (-1, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setText (QObject::tr ("None"));
    b->setBackgroundRole (QPalette::Button);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

    //  More colors ..
    b = new LCPActiveLabel (-2, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setText (QObject::tr ("More .."));
    b->setBackgroundRole (QPalette::Button);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);
  }

  {
    //  darker and brighter colors
    QFrame *f = new QFrame (this);
    f->setObjectName (QString::fromUtf8 ("color_l2"));
    l->addWidget (f);
    QHBoxLayout *ll = new QHBoxLayout (f);
    ll->setContentsMargins (0, 0, 0, 0);
    ll->setSpacing (0);

    LCPActiveLabel *b;

    QLabel *lbl = new QLabel (QObject::tr ("S/V"), f);
    ll->addWidget (lbl);

    b = new LCPActiveLabel (-10, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setPixmap (QPixmap (QString::fromUtf8 (":dark_12px@2x.png")));
    b->setBackgroundRole (QPalette::Button);
    b->setAlignment (Qt::AlignHCenter);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

    b = new LCPActiveLabel (-11, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setPixmap (QPixmap (QString::fromUtf8 (":bright_12px@2x.png")));
    b->setBackgroundRole (QPalette::Button);
    b->setAlignment (Qt::AlignHCenter);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

    b = new LCPActiveLabel (-12, f);
    b->setFrameStyle (QFrame::Panel | QFrame::Raised);
    b->setLineWidth (1);
    b->setPixmap (QPixmap (QString::fromUtf8 (":neutral_12px@2x.png")));
    b->setBackgroundRole (QPalette::Button);
    b->setAlignment (Qt::AlignHCenter);
    connect (b, SIGNAL (clicked (int)), this, SLOT (button_clicked (int)));
    ll->addWidget (b);

  }

}

void
LCPColorPalette::set_palette (const lay::ColorPalette &palette)
{
  if (palette != m_palette) {

    m_palette = palette;

    for (unsigned int i = 0; i < m_color_buttons.size (); ++i) {
      QColor color;
      if (i < m_palette.colors ()) {
        color = QColor (m_palette.color_by_index (i));
      }
      if (m_color_buttons [i]) {
        QPalette pl;
        pl.setColor (QPalette::Window, color);
        m_color_buttons [i]->setPalette (pl);
      }
    }

  }
}

void 
LCPColorPalette::button_clicked (int index)
{
  if (index >= 0) {
    QColor color;
    if (index < int (m_palette.colors ())) {
      color = QColor (m_palette.color_by_index (index));
    }
    emit color_selected (color);
  } else if (index == -1) {
    //  no color
    emit color_selected (QColor ());
  } else if (index == -2) {
    QColor c = QColorDialog::getColor ();
    if (c.isValid ()) {
      emit color_selected (c);
    }
  } else if (index == -10) {
    emit color_brightness_selected (-16); // darker
  } else if (index == -11) {
    emit color_brightness_selected (16); // brighter
  } else if (index == -12) {
    emit color_brightness_selected (0); // brighter
  }
}

// --------------------------------------------------------------------
//  LayerToolbox implementation

LayerToolbox::LayerToolbox (QWidget *parent, const char *name)
  : QWidget (parent), mp_view (0)
{
  setObjectName (QString::fromUtf8 (name));

  LCPVisibilityPalette *vp = new LCPVisibilityPalette (this, "vis");
  add_panel (vp, tl::to_string (QObject::tr ("Visibility")).c_str ());
  connect (vp, SIGNAL (visibility_change (bool)), this, SLOT (visibility_changed (bool)));
  connect (vp, SIGNAL (transparency_change (bool)), this, SLOT (transparency_changed (bool)));

  LCPStylePalette *sp = new LCPStylePalette (this, "styles");
  mp_style_palette = sp;
  add_panel (sp, tl::to_string (QObject::tr ("Style")).c_str ());
  connect (sp, SIGNAL (width_selected (int)), this, SLOT (width_changed (int)));
  connect (sp, SIGNAL (marked_selected (bool)), this, SLOT (marked_changed (bool)));
  connect (sp, SIGNAL (xfill_selected (bool)), this, SLOT (xfill_changed (bool)));
  connect (sp, SIGNAL (line_style_selected (int)), this, SLOT (line_style_changed (int)));
  connect (sp, SIGNAL (line_styles_changed (const lay::LineStyles &)), this, SLOT (line_styles_changed (const lay::LineStyles &)));

  LCPAnimationPalette *ap = new LCPAnimationPalette (this, "anim");
  add_panel (ap, tl::to_string (QObject::tr ("Animation")).c_str ());
  connect (ap, SIGNAL (animation_selected (int)), this, SLOT (animation_changed (int)));

  LCPDitherPalette *dp = new LCPDitherPalette (this, "dither");
  mp_dither_palette = dp;
  add_panel (dp, tl::to_string (QObject::tr ("Stipple")).c_str ());
  connect (dp, SIGNAL (dither_selected (int)), this, SLOT (dither_changed (int)));
  connect (dp, SIGNAL (pattern_changed (const lay::DitherPattern &)), this, SLOT (dither_pattern_changed (const lay::DitherPattern &)));

  LCPColorPalette *pf = new LCPColorPalette (this, "colors");
  mp_frame_palette = pf;
  add_panel (pf, tl::to_string (QObject::tr ("Frame color")).c_str ());
  connect (pf, SIGNAL (color_selected (QColor)), this, SLOT (frame_color_changed (QColor)));
  connect (pf, SIGNAL (color_brightness_selected (int)), this, SLOT (frame_color_brightness (int)));

  LCPColorPalette *p = new LCPColorPalette (this, "colors_frame");
  mp_palette = p;
  add_panel (p, tl::to_string (QObject::tr ("Color")).c_str ());
  connect (p, SIGNAL (color_selected (QColor)), this, SLOT (fill_color_changed (QColor)));
  connect (p, SIGNAL (color_brightness_selected (int)), this, SLOT (fill_color_brightness (int)));

  //  make the height equal to the computed height 
  int h = sizeHint ().height ();
  setMinimumHeight (h);
  setMaximumHeight (h);
}

LayerToolbox::~LayerToolbox ()
{
  //  .. nothing yet ..
}

void 
LayerToolbox::set_view (LayoutViewBase *view)
{
  mp_dither_palette->set_view (view);
  mp_style_palette->set_view (view);
  mp_view = view;
}

void 
LayerToolbox::add_panel (QWidget *panel_widget, const char *text)
{
  panel_widget->hide ();

  QFrame *f = new QFrame (this);
  f->setAutoFillBackground (true);
  f->setObjectName (QString::fromUtf8 ("panel"));
  QHBoxLayout *l = new QHBoxLayout (f);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  f->setFrameStyle (QFrame::Panel | QFrame::Raised);
  f->setLineWidth (1);
  f->setBackgroundRole (QPalette::Highlight);

  QCheckBox *b = new QCheckBox (f);
  l->addWidget (b);

  b->setFocusPolicy (Qt::NoFocus);
  b->setBackgroundRole (QPalette::Highlight);
  QPalette pl (b->palette ());
  pl.setColor (QPalette::WindowText, pl.color (QPalette::Active, QPalette::HighlightedText));
  b->setPalette (pl);
  b->setText (tl::to_qstring (text));
  b->setMaximumSize (QSize (b->maximumSize ().width (), b->sizeHint ().height () - 4));

  LCPRemitter *e = new LCPRemitter (int (m_tool_panels.size ()), this);
  connect (b, SIGNAL(clicked ()), e, SLOT (the_slot ()));
  connect (e, SIGNAL(the_signal (int)), this, SLOT (panel_button_clicked (int)));

  m_tool_panels.push_back (std::make_pair (f, panel_widget));
}

QSize
LayerToolbox::sizeHint () const
{
  //  override the min width to account for the tree behaviour of Qt 4.5.x:
  int w = 148;
  for (std::vector <std::pair <QWidget *, QWidget *> >::const_iterator i = m_tool_panels.begin (); i != m_tool_panels.end (); ++i) {
    w = std::max (std::max (i->first->sizeHint ().width (), i->second->sizeHint ().width ()), w);
  }

  //  get the required height
  int h = 0;
  for (std::vector <std::pair <QWidget *, QWidget *> >::const_iterator i = m_tool_panels.begin (); i != m_tool_panels.end (); ++i) {
    if (!i->second->isHidden ()) {
      h += i->second->sizeHint ().height ();
    }
    h += i->first->sizeHint ().height ();
  }

  return QSize (w, h);
}

void 
LayerToolbox::resizeEvent (QResizeEvent *re)
{
  rearrange (re->size ().width (), re->size ().height ());
}

void 
LayerToolbox::resize (int w, int h)
{
  QWidget::resize (w, h);
  rearrange (w, h);
}

void 
LayerToolbox::setGeometry (int x, int y, int w, int h)
{
  QWidget::setGeometry (x, y, w, h);
  rearrange (w, h);
}

void 
LayerToolbox::rearrange (int w, int h)
{
  for (std::vector <std::pair <QWidget *, QWidget *> >::iterator i = m_tool_panels.begin (); i != m_tool_panels.end (); ++i) {

    int hh;

    if (!i->second->isHidden ()) {
      hh = i->second->sizeHint ().height ();
      h -= hh;
      i->second->setGeometry (0, h, w, hh);
    }

    hh = i->first->sizeHint ().height ();
    h -= hh;
    i->first->setGeometry (0, h, w, hh);

  }
}

void 
LayerToolbox::panel_button_clicked (int index)
{
  if (index < 0 || index >= int (m_tool_panels.size ())) {
    return;
  }

  if (!m_tool_panels [index].second->isHidden ()) {
    m_tool_panels [index].second->hide ();
  } else {
    m_tool_panels [index].second->show ();
  }

  //  make the height equal to the computed height 
  int h = sizeHint ().height ();
  setMinimumHeight (h);
  setMaximumHeight (h);

  updateGeometry ();
}

template <class Op>
void 
LayerToolbox::foreach_selected (const Op &op)
{
  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    op (props);
    mp_view->set_properties (*l, props);
  }
}

struct SetColor
{
  /** 
   *  @brief set (some) colors of the properties
   *
   *  @param flags Bitmask that defines which colors to change: #0=fill,#1=frame,#2=vertex,#3=text
   *  @param c The color to apply
   */
  SetColor (QColor c, unsigned int flags)
    : m_color (c), m_flags (flags)
  {
    // .. nothing yet ..
  }

  void operator() (lay::LayerProperties &props) const
  {
    if (m_flags & 2) {
      if (! m_color.isValid ()) {
        props.clear_fill_color ();
      } else {
        props.set_fill_color (m_color.rgb ());
        props.set_fill_brightness (0);
      }
    } 
    if (m_flags & 1) {
      if (! m_color.isValid ()) {
        props.clear_frame_color ();
      } else {
        props.set_frame_color (m_color.rgb ());
        props.set_frame_brightness (0);
      }
    }
  }

private:
  QColor m_color;
  unsigned int m_flags;
};

void 
LayerToolbox::fill_color_changed (QColor c)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change fill color")));

  SetColor op (c, 3 /*fill,frame and vertex*/);
  foreach_selected (op);
}

void 
LayerToolbox::frame_color_changed (QColor c)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change frame color")));

  SetColor op (c, 1 /*frame and vertex*/);
  foreach_selected (op);
}

struct SetBrightness
{
  /** 
   *  @brief set (some) colors of the properties
   *
   *  @param flags Bitmask that defines which colors to change: #0=fill,#1=frame,#2=vertex,#3=text
   *  @param c The color to apply
   */
  SetBrightness (int delta, unsigned int flags)
    : m_delta (delta), m_flags (flags)
  {
    // .. nothing yet ..
  }

  void operator() (lay::LayerProperties &props) const
  {
    if (m_flags & 2) {
      if (m_delta == 0) {
        props.set_fill_brightness (0);
      } else {
        props.set_fill_brightness (props.fill_brightness (false) + m_delta);
      }
    } 
    if (m_flags & 1) {
      if (m_delta == 0) {
        props.set_frame_brightness (0);
      } else {
        props.set_frame_brightness (props.frame_brightness (false) + m_delta);
      }
    }
  }

private:
  int m_delta;
  unsigned int m_flags;
};

void 
LayerToolbox::fill_color_brightness (int delta)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change fill color brightness")));

  SetBrightness op (delta, 3 /*fill,frame and vertex*/);
  foreach_selected (op);
}

void 
LayerToolbox::frame_color_brightness (int delta)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change frame color brightness")));

  SetBrightness op (delta, 1 /*frame and vertex*/);
  foreach_selected (op);
}

struct SetDither
{
  SetDither (int di)
    : m_di (di)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    if (m_di < 0) {
      props.clear_dither_pattern ();
    } else {
      props.set_dither_pattern ((unsigned int) m_di);
    }
  }

private:
  int m_di;
};

void
LayerToolbox::line_styles_changed (const lay::LineStyles &styles)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Edit line styles")));
  mp_view->set_line_styles (styles);
}

void
LayerToolbox::dither_pattern_changed (const lay::DitherPattern &pattern)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Edit stipple pattern")));
  mp_view->set_dither_pattern (pattern);
}

void
LayerToolbox::dither_changed (int di)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Set stipple pattern")));

  SetDither op (di);
  foreach_selected (op);
}

struct SetVisible
{
  SetVisible (bool v)
    : m_visible (v)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_visible (m_visible);
  }

private:
  bool m_visible;
};

void 
LayerToolbox::visibility_changed (bool visible)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (visible ? QObject::tr ("Show layer") : QObject::tr ("Hide layer")));

  SetVisible op (visible);
  foreach_selected (op);
}

struct SetTransparency
{
  SetTransparency (bool t)
    : m_transparent (t)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_transparent (m_transparent);
  }

private:
  bool m_transparent;
};

void 
LayerToolbox::transparency_changed (bool transparent)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change transparency")));

  SetTransparency op (transparent);
  foreach_selected (op);
}

struct SetAnimation
{
  SetAnimation (int mode)
    : m_mode (mode)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_animation (m_mode);
  }

private:
  int m_mode;
};

void 
LayerToolbox::animation_changed (int mode)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change animation mode")));

  SetAnimation op (mode);
  foreach_selected (op);
}

struct SetWidth
{
  SetWidth (int w)
    : m_width (w)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_width (m_width);
  }

private:
  int m_width;
};

void 
LayerToolbox::width_changed (int width)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change line width")));

  SetWidth op (width);
  foreach_selected (op);
}

struct SetXFill
{
  SetXFill (bool x)
    : m_xfill (x)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_xfill (m_xfill);
  }

private:
  bool m_xfill;
};

void
LayerToolbox::xfill_changed (bool xf)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change cross fill")));

  SetXFill op (xf);
  foreach_selected (op);
}

struct SetLineStyle
{
  SetLineStyle (int x)
    : m_style (x)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_line_style (m_style);
  }

private:
  int m_style;
};

void
LayerToolbox::line_style_changed (int ls)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change line style")));

  SetLineStyle op (ls);
  foreach_selected (op);
}

struct SetMarked
{
  SetMarked (bool m)
    : m_marked (m)
  { }

  void operator() (lay::LayerProperties &props) const
  {
    props.set_marked (m_marked);
  }

private:
  bool m_marked;
};

void 
LayerToolbox::marked_changed (bool marked)
{
  if (! mp_view) {
    return;
  }

  db::Transaction tr (mp_view->manager (), tl::to_string (QObject::tr ("Change marked vertices")));

  SetMarked op (marked);
  foreach_selected (op);
}

void
LayerToolbox::set_palette (const lay::ColorPalette &p)
{
  mp_palette->set_palette (p);
  mp_frame_palette->set_palette (p);
}

void
LayerToolbox::set_palette (const lay::StipplePalette &p)
{
  mp_dither_palette->set_palette (p);
}

void
LayerToolbox::set_palette (const lay::LineStylePalette &p)
{
  mp_style_palette->set_palette (p);
}

}

#endif
