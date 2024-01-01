
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

#ifndef HDR_layLayerToolbox
#define HDR_layLayerToolbox

#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>

#include <vector>
#include <set>

#include "layuiCommon.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layLineStylePalette.h"

namespace lay
{

class LayoutViewBase;
class LCPDitherPalette;
class LCPColorPalette;
class LCPActiveLabel;
class DitherPattern;
class LineStyles;
class ColorPalette;
class StipplePalette;

/**
 *  @brief Qt signal remitter class
 *
 *  The purpose of this class is to re-emit a signal at the
 *  input with a integer argument that is given to the class
 *  instance.
 */
class LCPRemitter
  : public QObject
{
Q_OBJECT

public:
  LCPRemitter (int index, QObject *parent, const char *name = 0);

public slots:
  void the_slot ();

signals:
  void the_signal (int index);

private:
  int m_index;
};

/** 
 *  @brief A color palette
 *
 *  This class implements a color chooser palette.
 *  The only signal emitted by this class is the color_selected
 *  signal which is emitted if a color (either from the
 *  palette or the "more colors" dialog) is selected.
 */
class LCPColorPalette
  : public QFrame
{
Q_OBJECT

public:
  LCPColorPalette (QWidget *parent, const char *name = 0); 

  void set_palette (const lay::ColorPalette &p);

public slots:
  void button_clicked (int index);

signals:
  void color_selected (QColor);
  void color_brightness_selected (int);

private:
  lay::ColorPalette m_palette;
  std::vector <LCPActiveLabel *> m_color_buttons;
};

/** 
 *  @brief A animation palette
 *
 *  This class implements a animation chooser palette.
 *  The signal emitted by this class is the animation_selected
 *  signal which is emitted if a animation mode is selected.
 */
class LCPAnimationPalette
  : public QFrame
{
Q_OBJECT

public:
  LCPAnimationPalette (QWidget *parent, const char *name = 0); 

public slots:
  void button_clicked (int index);

signals:
  void animation_selected (int mode);
};

/** 
 *  @brief A style palette
 *
 *  This class implements a style chooser palette.
 *  The signals emitted by this class is the width_selected
 *  signal which is emitted if a width is selected and
 *  the marked_selected which is emitted if one of the marked button
 *  was pressed.
 */
class LCPStylePalette
  : public QFrame
{
Q_OBJECT

public:
  LCPStylePalette (QWidget *parent, const char *name = 0); 

  void set_view (LayoutViewBase *view)
  {
    mp_view = view;
  }

  void set_palette (const lay::LineStylePalette &p);

public slots:
  void button_clicked (int index);

signals:
  void width_selected (int width);
  void marked_selected (bool marked);
  void xfill_selected (bool xfill);
  void line_style_selected (int index);
  void line_styles_changed (const lay::LineStyles &);

private:
  void create_pixmap_for_line_style (LCPActiveLabel *b, int n);

  lay::LineStylePalette m_palette;
  lay::LayoutViewBase *mp_view;
  std::vector <LCPActiveLabel *> m_style_buttons;
};

/** 
 *  @brief A dither pattern palette
 *
 *  This class implements a dither pattern palette.
 *  The only signal emitted by this class is the dither_selected
 *  signal which is emitted if a pattern is selected.
 */
class LCPDitherPalette
  : public QFrame
{
Q_OBJECT

public:
  LCPDitherPalette (QWidget *parent, const char *name = 0); 

  void set_palette (const lay::StipplePalette &p);

  void set_view (LayoutViewBase *view)
  {
    mp_view = view;
  }

public slots:
  void button_clicked (int index);

signals:
  void dither_selected (int dither_index);
  void pattern_changed (const lay::DitherPattern &);

private:
  lay::StipplePalette m_palette;
  lay::LayoutViewBase *mp_view;
  std::vector <LCPActiveLabel *> m_stipple_buttons;

  void create_pixmap_for (LCPActiveLabel *b, int n);
};

/**
 *  @brief A palette with the buttons for visibility settings
 *
 *  This palette exhibits two button groups: for visibility
 *  and transparency. Two signals are emitted: 
 *  visibility_change if "show" or "hide" are clicked,
 *  transparency_changed if "transparent" or "opaque" are clicked.
 */
class LCPVisibilityPalette
  : public QFrame
{
Q_OBJECT

public:
  LCPVisibilityPalette (QWidget *parent, const char *name = 0); 

public slots:
  void button_clicked (int index);

signals:
  void visibility_change (bool visible);
  void transparency_change (bool transparent);
};

/**
 *  @brief A lightweight button class
 *
 *  The lightweight buttons are implemented from labels that
 *  have a "push-down" behaviour. The only signal emitted is
 *  "clicked" with an integer value that can be assigned to
 *  the button.
 */
class LCPActiveLabel 
  : public QLabel
{
Q_OBJECT 

public:
  LCPActiveLabel (int index, QWidget *parent, const char *name = "button");

  virtual void mousePressEvent (QMouseEvent *e);
  virtual void mouseReleaseEvent (QMouseEvent *e);

signals: 
  void clicked (int);

private:
  int m_index;
  bool m_grabbed;
};

/**
 *  @brief A widget implementing the layer toolbox
 */
class LAYUI_PUBLIC LayerToolbox
  : public QWidget
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LayerToolbox (QWidget *parent, const char *name);

  /**
   *  @brief Destructor
   */
  ~LayerToolbox ();

  /**
   *  @brief Associate a toolbox with a layout view
   *
   *  This will make the toolbox control the given layout view
   */
  void set_view (LayoutViewBase *view);

  /**
   *  @brief Changing of the color palette
   */
  void set_palette (const lay::ColorPalette &p);

  /**
   *  @brief Changing of the stipple palette
   */
  void set_palette (const lay::StipplePalette &p);

  /**
   *  @brief Changing of the stipple palette
   */
  void set_palette (const lay::LineStylePalette &p);

  /**
   *  @brief The sizeHint implementation for Qt layout management
   */
  virtual QSize sizeHint () const;

  /** 
   *  @brief The Qt resize event
   */
  virtual void resizeEvent (QResizeEvent *re);

  /** 
   *  @brief The Qt resize function that does the layout management
   */
  virtual void resize (int w, int h);

  /** 
   *  @brief The Qt geometry setting function that also does the layout management
   */
  virtual void setGeometry (int x, int y, int w, int h);

protected slots:
  void panel_button_clicked (int index);
  void fill_color_changed (QColor c);
  void frame_color_changed (QColor c);
  void fill_color_brightness (int delta);
  void frame_color_brightness (int delta);
  void dither_changed (int di);
  void dither_pattern_changed (const lay::DitherPattern &pattern);
  void line_styles_changed (const lay::LineStyles &styles);
  void visibility_changed (bool visible);
  void transparency_changed (bool transparent);
  void width_changed (int width);
  void marked_changed (bool marked);
  void xfill_changed (bool xfill);
  void line_style_changed (int index);
  void animation_changed (int mode);

private:
  LayoutViewBase *mp_view;
  std::vector <std::pair <QWidget *, QWidget *> > m_tool_panels;
  LCPDitherPalette *mp_dither_palette;
  LCPStylePalette *mp_style_palette;
  LCPColorPalette *mp_palette;
  LCPColorPalette *mp_frame_palette;

  template <class Op>
  void foreach_selected (const Op &op);
  void rearrange (int w, int h);
  void add_panel (QWidget *panel_widget, const char *text);
};

}

#endif

#endif  //  defined(HAVE_QT)
