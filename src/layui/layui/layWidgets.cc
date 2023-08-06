
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include <QGridLayout>
#include <QMenu>
#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>

#include "layWidgets.h"

#include "layLayoutViewBase.h"
#include "layDialogs.h"
#include "tlExceptions.h"
#include "layStipplePalette.h"
#include "layColorPalette.h"
#include "laybasicConfig.h"
#include "layPlugin.h"

#include "dbLayout.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"

#include "tlInternational.h"

#include "laySelectStippleForm.h"

#include <vector>

namespace lay
{

// -------------------------------------------------------------
//  DitherPatternSelectionButton implementation

DitherPatternSelectionButton::DitherPatternSelectionButton (QWidget *parent)
  : QPushButton (parent), mp_view (0), m_dither_pattern (-1)
{
  setMenu (new QMenu (this));
  update_pattern ();
  connect (menu (), SIGNAL (aboutToShow ()), this, SLOT (menu_about_to_show ()));
}

DitherPatternSelectionButton::~DitherPatternSelectionButton ()
{
  // .. nothing yet ..
}

void 
DitherPatternSelectionButton::set_view (lay::LayoutViewBase *view)
{
  if (view != mp_view) {
    mp_view = view;
    update_menu ();
  }
}

void 
DitherPatternSelectionButton::set_dither_pattern (int dp)
{
  if (dp != m_dither_pattern) {
    m_dither_pattern = dp;
    update_pattern ();
  }
}

int 
DitherPatternSelectionButton::dither_pattern () const
{
  return m_dither_pattern;
}

void
DitherPatternSelectionButton::menu_selected () 
{
  QAction *action = dynamic_cast <QAction *> (sender ());
  if (action) {

    m_dither_pattern = action->data ().toInt ();
    update_pattern ();
    emit (dither_pattern_changed (m_dither_pattern));

  }
}

void 
DitherPatternSelectionButton::browse_selected ()
{
  if (mp_view) {

    SelectStippleForm stipples_form (0, mp_view->dither_pattern (), true);
    stipples_form.set_selected (m_dither_pattern);

    if (stipples_form.exec ()) {

      m_dither_pattern = stipples_form.selected ();
      update_pattern ();
      emit (dither_pattern_changed (m_dither_pattern));

    }

  } else {

    //  Use the default (non-custom) pattern if no view is set.
    lay::DitherPattern default_pattern;

    SelectStippleForm stipples_form (0, default_pattern, true);
    stipples_form.set_selected (m_dither_pattern);

    if (stipples_form.exec ()) {

      m_dither_pattern = stipples_form.selected ();
      update_pattern ();
      emit (dither_pattern_changed (m_dither_pattern));

    }

  }
}

void 
DitherPatternSelectionButton::update_pattern ()
{  
  QPushButton::setText (QString::fromUtf8 (" "));

  QString text = QString::fromUtf8 ("XXXXXXX");
  QFontMetrics fm (font (), this);
  QRect rt (fm.boundingRect (text)); // dummy text to be compliant with the other color button

  QPushButton::setIconSize (QSize (rt.width (), rt.height ()));

#if QT_VERSION >= 0x050000
  double dpr = devicePixelRatio ();
#else
  double dpr = 1.0;
#endif

  if (m_dither_pattern < 0) {

    QPixmap pixmap (rt.width () * dpr, rt.height () * dpr);
#if QT_VERSION >= 0x050000
    pixmap.setDevicePixelRatio (dpr);
#endif
    pixmap.fill (QColor (0, 0, 0, 0));

    QPainter pxpainter (&pixmap);
    pxpainter.setFont (font ());
    QColor text_color = palette ().color (QPalette::Active, QPalette::Text);
    pxpainter.setPen (QPen (text_color));

    QRectF r (0, 0, rt.width () - pxpainter.pen ().widthF (), rt.height () - pxpainter.pen ().widthF ());
    pxpainter.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, QObject::tr ("None"));

    QPushButton::setIcon (QIcon (pixmap));

  } else {

    const lay::DitherPatternInfo *dp_info;
    if (mp_view) {
      dp_info = & mp_view->dither_pattern ().pattern ((unsigned int) m_dither_pattern).scaled (dpr);
    } else {
      static lay::DitherPattern default_pattern;
      dp_info = & default_pattern.pattern ((unsigned int) m_dither_pattern).scaled (dpr);
    }

    QPushButton::setIcon (dp_info->get_bitmap (rt.width () * dpr, rt.height () * dpr, dpr));

  }
}

void
DitherPatternSelectionButton::menu_about_to_show ()
{
  update_menu ();
}

void 
DitherPatternSelectionButton::update_menu ()
{  
  menu ()->clear ();
  menu ()->addAction (QObject::tr ("None"), this, SLOT (menu_selected ()))->setData (-1);
  menu ()->addAction (QObject::tr ("Choose ..."), this, SLOT (browse_selected ()));
  menu ()->addSeparator ();

  //  from_string might throw an exception ...
  try {

    lay::DitherPattern patterns;

    std::string s;
    if (lay::Dispatcher::instance ()) {
      lay::Dispatcher::instance ()->config_get (cfg_stipple_palette, s);
    }
    lay::StipplePalette palette = lay::StipplePalette::default_palette ();
    if (! s.empty ()) {
      palette.from_string (s);
    }

    //  fill the list of stipple palette items
    for (unsigned int i = 0; i < palette.stipples (); ++i) {

      unsigned int n = palette.stipple_by_index (i);
      if (int (n) < std::distance (patterns.begin (), patterns.end ())) {
      
#if QT_VERSION > 0x050000
        double dpr = devicePixelRatio ();
#else
        double dpr = 1.0;
#endif

        lay::DitherPatternInfo info = patterns.begin () [n];
        info.scale_pattern (dpr);

        std::string name (info.name ());
        if (name.empty ()) {
          name = tl::sprintf ("#%d", n);
        }

        menu ()->addAction (QIcon (info.get_bitmap (-1, -1, dpr)), tl::to_qstring (name), this, SLOT (menu_selected ()))->setData (n);

      }
    }

  } catch (...) { }
}

// -------------------------------------------------------------
//  CellViewSelectionComboBox implementation

struct CellViewSelectionComboBoxPrivateData
{
  const lay::LayoutViewBase *layout_view;
};

CellViewSelectionComboBox::CellViewSelectionComboBox (QWidget *parent)
  : QComboBox (parent)
{
  mp_private = new CellViewSelectionComboBoxPrivateData ();
  mp_private->layout_view = 0;
}

CellViewSelectionComboBox::~CellViewSelectionComboBox ()
{
  delete mp_private;
  mp_private = 0;
}

const lay::LayoutViewBase *
CellViewSelectionComboBox::layout_view () const
{
  return mp_private->layout_view;
}

void 
CellViewSelectionComboBox::set_layout_view (const lay::LayoutViewBase *layout_view)
{
  //  TODO: should register a listener, so it does the update automatically.
  mp_private->layout_view = layout_view;

  int current = current_cv_index ();

  clear ();
  for (unsigned int cv = 0; cv < layout_view->cellviews (); ++cv) {
    if (layout_view->cellview (cv).is_valid ()) {
      addItem (tl::to_qstring (layout_view->cellview (cv)->name () + ", " + tl::to_string (QObject::tr ("Cell")) + " '" 
                + layout_view->cellview (cv)->layout ().cell_name (layout_view->cellview (cv).cell_index ()) + "'"));
    } else {
      addItem (tl::to_qstring (layout_view->cellview (cv)->name () + ", " + tl::to_string (QObject::tr ("Undefined cell"))));
    }
  }

  if (current < 0 || current >= int (layout_view->cellviews ())) {
    set_current_cv_index (layout_view->cellviews () > 0 ? 0 : -1);
  } else {
    set_current_cv_index (current);
  }
}

void 
CellViewSelectionComboBox::set_current_cv_index (int cv)
{
  setCurrentIndex (cv);
}

int 
CellViewSelectionComboBox::current_cv_index () const
{
  return currentIndex ();
}

// -------------------------------------------------------------
//  LayerSelectionComboBox implementation

struct LayerSelectionComboBoxPrivateData
{
  std::vector <std::pair <db::LayerProperties, int> > layers;
  bool no_layer_available;
  bool new_layer_enabled;
  bool all_layers;
  const db::Layout *layout;
  lay::LayoutViewBase *view;
  int cv_index;
  db::LayerProperties last_props;
};

LayerSelectionComboBox::LayerSelectionComboBox (QWidget *parent)
  : QComboBox (parent), dm_update_layer_list (this, &LayerSelectionComboBox::do_update_layer_list)
{
  mp_private = new LayerSelectionComboBoxPrivateData ();
  mp_private->no_layer_available = false;
  mp_private->new_layer_enabled = true;
  mp_private->layout = 0;
  mp_private->view = 0;
  mp_private->cv_index = -1;
  mp_private->all_layers = false;

  connect (this, SIGNAL (activated (int)), this, SLOT (item_selected (int)));
}

LayerSelectionComboBox::~LayerSelectionComboBox ()
{
  delete mp_private;
  mp_private = 0;
}

void  
LayerSelectionComboBox::set_new_layer_enabled (bool f)
{
  if (mp_private->new_layer_enabled != f) {
    mp_private->new_layer_enabled = f;
    update_layer_list ();
  }
}

bool  
LayerSelectionComboBox::is_new_layer_enabled () const
{
  return mp_private->new_layer_enabled;
}

void  
LayerSelectionComboBox::set_no_layer_available (bool f)
{
  if (mp_private->no_layer_available != f) {
    mp_private->no_layer_available = f;
    update_layer_list ();
  }
}

bool  
LayerSelectionComboBox::is_no_layer_available () const
{
  return mp_private->no_layer_available;
}

void 
LayerSelectionComboBox::item_selected (int index)
{
BEGIN_PROTECTED

  if (mp_private->view != 0 && index == count () - 1 && mp_private->new_layer_enabled) {

    setCurrentIndex (-1);

    const lay::CellView &cv = mp_private->view->cellview (mp_private->cv_index);
    db::LayerProperties lp;

    if (! mp_private->view->current_layer ().is_null ()) {
      int li = mp_private->view->current_layer ()->layer_index ();
      if (li >= 0) {
        lp = mp_private->view->cellview (mp_private->view->current_layer ()->cellview_index ())->layout ().get_properties (li);
      }
    }

    lay::NewLayerPropertiesDialog prop_dia (this);
    if (prop_dia.exec_dialog (cv, lp)) {

      for (unsigned int l = 0; l < cv->layout ().layers (); ++l) {
        if (cv->layout ().is_valid_layer (l) && cv->layout ().get_properties (l).log_equal (lp)) {
          throw tl::Exception (tl::to_string (QObject::tr ("A layer with that signature already exists: ")) + lp.to_string ());
        }
      }

      if (mp_private->view->manager ()) {
        mp_private->view->manager ()->transaction (tl::to_string (QObject::tr ("New layer")));
      }

      unsigned int l = cv->layout ().insert_layer (lp);
      std::vector <unsigned int> nl;
      nl.push_back (l);
      mp_private->view->add_new_layers (nl, mp_private->cv_index);
      mp_private->view->update_content ();

      if (mp_private->view->manager ()) {
        mp_private->view->manager ()->commit ();
      }

      //  NOTE: add_new_layers has triggered update_layer_list which already added the new layer
      set_current_layer (lp);

    }

  }

END_PROTECTED;
}

struct LPIPairCompareOp
{
  bool operator() (const std::pair <db::LayerProperties, int> &a, const std::pair <db::LayerProperties, int> &b) const
  {
    if (! a.first.log_equal (b.first)) {
      return a.first.log_less (b.first);
    }
    return a.second < b.second;
  }
};

void 
LayerSelectionComboBox::set_view (lay::LayoutViewBase *view, int cv_index, bool all_layers)
{
  if (view == 0 || cv_index < 0) {
    set_layout (0);
    return;
  }

  mp_private->layout = 0;
  mp_private->view = view;
  mp_private->cv_index = cv_index;
  mp_private->all_layers = all_layers;

  view->layer_list_changed_event.add (this, &LayerSelectionComboBox::on_layer_list_changed);

  update_layer_list ();
}

void
LayerSelectionComboBox::on_layer_list_changed (int)
{
  update_layer_list ();
}

void 
LayerSelectionComboBox::set_layout (const db::Layout *layout)
{
  mp_private->layout = layout;
  mp_private->view = 0;
  mp_private->cv_index = -1;
  mp_private->all_layers = false;

  update_layer_list ();
}

void
LayerSelectionComboBox::update_layer_list ()
{
  dm_update_layer_list ();
}

void
LayerSelectionComboBox::do_update_layer_list ()
{
  int i = currentIndex ();
  db::LayerProperties props = mp_private->last_props;
  if (i >= 0 && i < int (mp_private->layers.size ())) {
    props = mp_private->layers [i].first;
  }

  mp_private->layers.clear ();
  if (mp_private->no_layer_available) {
    mp_private->layers.push_back (std::make_pair (db::LayerProperties (), -1));
  }

  clear ();

  if (mp_private->view) {

    const db::Layout *layout = 0;

    const CellView &cv = mp_private->view->cellview (mp_private->cv_index);
    if (cv.is_valid ()) {
      layout = & cv->layout ();
    }

    if (! layout) {

      set_current_layer (-1);

    } else {

      LPIPairCompareOp cmp_op;
      std::map<std::pair <db::LayerProperties, int>, std::string, LPIPairCompareOp> name_for_layer (cmp_op);
      LayerPropertiesConstIterator lp = mp_private->view->begin_layers ();
      while (! lp.at_end ()) {
        if (lp->cellview_index () == mp_private->cv_index && ! lp->has_children () && (mp_private->all_layers || lp->layer_index () >= 0) && lp->source (true).layer_props () != db::LayerProperties ()) {
          std::pair <db::LayerProperties, int> k (lp->source (true).layer_props (), lp->layer_index ());
          name_for_layer.insert (std::make_pair (k, lp->display_string (mp_private->view, true, true /*always show source*/)));
          mp_private->layers.push_back (k);
        }
        ++lp;
      }

      size_t nk = mp_private->layers.size ();

      for (unsigned int l = 0; l < layout->layers (); ++l) {
        if (layout->is_valid_layer (l)) {
          std::pair <db::LayerProperties, int> k (layout->get_properties (l), int (l));
          if (name_for_layer.find (k) == name_for_layer.end ()) {
            mp_private->layers.push_back (k);
          }
        }
      }

      std::sort (mp_private->layers.begin () + nk, mp_private->layers.end ());

      for (std::vector <std::pair <db::LayerProperties, int> >::iterator ll = mp_private->layers.begin (); ll != mp_private->layers.end (); ++ll) {
        std::map<std::pair <db::LayerProperties, int>, std::string, LPIPairCompareOp>::const_iterator ln = name_for_layer.find (*ll);
        if (ln != name_for_layer.end ()) {
          addItem (tl::to_qstring (ln->second));
        } else {
          addItem (tl::to_qstring (ll->first.to_string ()));
        }
      }

      if (mp_private->new_layer_enabled) {
        addItem (QObject::tr ("New Layer .."));
      }

      set_current_layer (props);

    }

  } else if (mp_private->layout) {

    size_t n = mp_private->layers.size ();

    for (unsigned int l = 0; l < mp_private->layout->layers (); ++l) {
      if (mp_private->layout->is_valid_layer (l)) {
        mp_private->layers.push_back (std::make_pair (mp_private->layout->get_properties (l), int (l)));
      }
    }

    std::sort (mp_private->layers.begin () + n, mp_private->layers.end ());

    for (std::vector <std::pair <db::LayerProperties, int> >::iterator ll = mp_private->layers.begin (); ll != mp_private->layers.end (); ++ll) {
      addItem (tl::to_qstring (ll->first.to_string ()));
    }

    set_current_layer (props);

  } else {
    set_current_layer (-1);
  }
}

void 
LayerSelectionComboBox::set_current_layer (const db::LayerProperties &props)
{
  mp_private->last_props = props;

  for (std::vector <std::pair <db::LayerProperties, int> >::iterator ll = mp_private->layers.begin (); ll != mp_private->layers.end (); ++ll) {
    if (ll->first.log_equal (props)) {
      setCurrentIndex (std::distance (mp_private->layers.begin (), ll));
      return;
    }
  }

  setCurrentIndex (-1);
}

void 
LayerSelectionComboBox::set_current_layer (int l)
{
  if (l < 0) {
    setCurrentIndex (-1);
  } else {
    for (std::vector <std::pair <db::LayerProperties, int> >::iterator ll = mp_private->layers.begin (); ll != mp_private->layers.end (); ++ll) {
      if (ll->second == l) {
        setCurrentIndex (std::distance (mp_private->layers.begin (), ll));
      }
    }
  }
}

int 
LayerSelectionComboBox::current_layer () const
{
  int i = currentIndex ();
  if (i < 0 || i > int (mp_private->layers.size ())) {
    return -1;
  } else {
    return mp_private->layers [i].second;
  }
}

db::LayerProperties 
LayerSelectionComboBox::current_layer_props () const
{
  int i = currentIndex ();
  if (i < 0 || i > int (mp_private->layers.size ())) {
    return mp_private->last_props;
  } else {
    return mp_private->layers [i].first;
  }
}

// -------------------------------------------------------------
//  LibrarySelectionComboBox implementation

LibrarySelectionComboBox::LibrarySelectionComboBox (QWidget *parent)
  : QComboBox (parent), m_tech_set (false)
{
  update_list ();
}

void
LibrarySelectionComboBox::set_technology_filter (const std::string &tech, bool enabled)
{
  if (m_tech != tech || m_tech_set != enabled) {
    m_tech = tech;
    m_tech_set = enabled;
    update_list ();
  }
}

void 
LibrarySelectionComboBox::update_list ()
{
  bool wasBlocked = blockSignals (true);

  db::Library *lib = current_library ();

  clear ();

  addItem (QObject::tr ("Local (no library)"), QVariant ());
  for (db::LibraryManager::iterator l = db::LibraryManager::instance ().begin (); l != db::LibraryManager::instance ().end (); ++l) {

    db::Library *lib = db::LibraryManager::instance ().lib (l->second);
    if (! m_tech_set || !lib->for_technologies () || lib->is_for_technology (m_tech)) {

      std::string item_text = lib->get_name ();
      if (! lib->get_description ().empty ()) {
        item_text += " - " + lib->get_description ();
      }
      if (m_tech_set && lib->for_technologies ()) {
        item_text += " ";
        std::string tn = tl::join (std::vector<std::string> (lib->get_technologies ().begin (), lib->get_technologies ().end ()), ",");
        item_text += tl::to_string (QObject::tr ("[Technology %1]").arg (tl::to_qstring (tn)));
      }

      addItem (tl::to_qstring (item_text), QVariant ((unsigned int) lib->get_id ()));

    }

  }

  set_current_library (lib);

  blockSignals (wasBlocked);
}

LibrarySelectionComboBox::~LibrarySelectionComboBox ()
{
  //  .. nothing yet ..
}

void 
LibrarySelectionComboBox::set_current_library (db::Library *lib)
{
  if (lib != current_library ()) {

    for (int i = 0; i < count (); ++i) {
      QVariant data = itemData (i);
      db::Library *item_lib = 0;
      if (! data.isNull ()) {
        item_lib = db::LibraryManager::instance ().lib (data.value<db::lib_id_type> ());
      }
      if (item_lib == lib) {
        setCurrentIndex (i);
        return;
      }
    }

    //  fallback: not a valid library pointer
    setCurrentIndex (-1);

  }
}

db::Library *
LibrarySelectionComboBox::current_library () const
{
  QVariant data = itemData (currentIndex ());
  if (data.isNull ()) {
    return 0;
  } else {
    return db::LibraryManager::instance ().lib (data.value<db::lib_id_type> ());
  }
}

// -------------------------------------------------------------
//  SimpleColorButton implementation

SimpleColorButton::SimpleColorButton (QWidget *parent, const char *name)
  : QPushButton (parent)
{
  setObjectName (QString::fromUtf8 (name));

  connect (this, SIGNAL (clicked ()), this, SLOT (selected ()));
}

SimpleColorButton::SimpleColorButton (QPushButton *&to_replace, const char *name)
  : QPushButton (to_replace->parentWidget ())
{
  setObjectName (QString::fromUtf8 (name));

  //  If the push button was part of a layout, replace it.
  //  This is somewhat tricky because there is no common method of
  //  the layout managers to replace a widget.

  QLayout *ly = to_replace->parentWidget ()->layout ();
  if (ly) {

    QBoxLayout *bx_ly = dynamic_cast <QBoxLayout *> (ly);
    if (bx_ly) {
      int i = ly->indexOf (to_replace);
      bx_ly->insertWidget (i, this);
    }

    QGridLayout *grid_ly = dynamic_cast <QGridLayout *> (ly);
    if (grid_ly) {
      int i = ly->indexOf (to_replace);
      int row = 0, column = 0;
      int row_span = 0, column_span = 0;
      grid_ly->getItemPosition (i, &row, &column, &row_span, &column_span);
      grid_ly->addWidget (this, row, column, row_span, column_span);
    }

  }

  delete to_replace;
  to_replace = 0;

  connect (this, SIGNAL (clicked ()), this, SLOT (selected ()));
}

void
SimpleColorButton::set_color (QColor c)
{
  set_color_internal (c);
}

void 
SimpleColorButton::set_color_internal (QColor c)
{
  m_color = c;

  QFontMetrics fm (font (), this);
  QRect rt (fm.boundingRect (QObject::tr ("Auto"))); // dummy text to be compliant with the other color button
  QPixmap pxmp (rt.width () + 24, rt.height ());

  QPainter pxpainter (&pxmp);
  QColor text_color = palette ().color (QPalette::Active, QPalette::Text);
  pxpainter.setPen (QPen (text_color));
  pxpainter.setBrush (QBrush (c.isValid () ? c : QColor (128, 128, 128)));
  QRect r (0, 0, pxmp.width () - 1, pxmp.height () - 1);
  pxpainter.drawRect (r);

  setIconSize (pxmp.size ());
  setIcon (QIcon (pxmp));
}

QColor 
SimpleColorButton::get_color () const
{
  return m_color;
}

void
SimpleColorButton::selected ()
{
  QColor c = QColorDialog::getColor (get_color (), this);
  if (c.isValid ()) {
    set_color (c);
    emit color_changed (m_color);
  }
}

// -------------------------------------------------------------
//  ColorButton implementation

ColorButton::ColorButton (QWidget *parent, const char *name)
  : QPushButton (parent)
{
  setObjectName (QString::fromUtf8 (name));

  setMenu (new QMenu (this));
  connect (menu (), SIGNAL (aboutToShow ()), this, SLOT (menu_about_to_show ()));
}

ColorButton::ColorButton (QPushButton *&to_replace, const char *name)
  : QPushButton (to_replace->parentWidget ())
{
  setObjectName (QString::fromUtf8 (name));

  setMenu (new QMenu (this));
  connect (menu (), SIGNAL (aboutToShow ()), this, SLOT (menu_about_to_show ()));

  //  If the push button was part of a layout, replace it.
  //  This is somewhat tricky because there is no common method of
  //  the layout managers to replace a widget.

  QLayout *ly = to_replace->parentWidget ()->layout ();
  if (ly) {

    QBoxLayout *bx_ly = dynamic_cast <QBoxLayout *> (ly);
    if (bx_ly) {
      int i = ly->indexOf (to_replace);
      bx_ly->insertWidget (i, this);
    }

    QGridLayout *grid_ly = dynamic_cast <QGridLayout *> (ly);
    if (grid_ly) {
      int i = ly->indexOf (to_replace);
      int row = 0, column = 0;
      int row_span = 0, column_span = 0;
      grid_ly->getItemPosition (i, &row, &column, &row_span, &column_span);
      grid_ly->addWidget (this, row, column, row_span, column_span);
    }

  }

  delete to_replace;
  to_replace = 0;
}

const char *color_icon = 
  "xxxxxaaxxxxbbxxxxx"
  "xxxxA00AxxB11Bxxxx"
  "xxxa0000ab1111bxxx"
  "xxxa0000ab1111bxxx"
  "xxxxA00AxxB11Bxxxx"
  "xxffxaaxxxxbbxccxx"
  "xF55FxxxxxxxxC22Cx"
  "f5555fxxxxxxc2222c"
  "f5555fxxxxxxc2222c"
  "xF55FxxxxxxxxC22Cx"
  "xxffxeexxxxddxccxx"
  "xxxxE44ExxD33Dxxxx"
  "xxxe4444ed3333dxxx"
  "xxxe4444ed3333dxxx"
  "xxxxE44ExxD33Dxxxx"
  "xxxxxeexxxxddxxxxx";

const char *color_icon_2x =
  "xxxxxxxxxxaaaaxxxxxxxxbbbbxxxxxxxxxx"
  "xxxxxxxxxA0000AxxxxxxB1111Bxxxxxxxxx"
  "xxxxxxxxA000000AxxxxB111111Bxxxxxxxx"
  "xxxxxxxA00000000AxxB11111111Bxxxxxxx"
  "xxxxxxa0000000000ab1111111111bxxxxxx"
  "xxxxxxa0000000000ab1111111111bxxxxxx"
  "xxxxxxa0000000000ab1111111111bxxxxxx"
  "xxxxxxa0000000000ab1111111111bxxxxxx"
  "xxxxxxxA00000000AxxB11111111Bxxxxxxx"
  "xxxxxxxxA000000AxxxxB111111Bxxxxxxxx"
  "xxxxffffxA0000AxxxxxxB1111Bxccccxxxx"
  "xxxF5555FxaaaaxxxxxxxxbbbbxC2222Cxxx"
  "xxF555555FxxxxxxxxxxxxxxxxC222222Cxx"
  "xF55555555FxxxxxxxxxxxxxxC22222222Cx"
  "f5555555555fxxxxxxxxxxxxc2222222222c"
  "f5555555555fxxxxxxxxxxxxc2222222222c"
  "f5555555555fxxxxxxxxxxxxc2222222222c"
  "f5555555555fxxxxxxxxxxxxc2222222222c"
  "xF55555555FxxxxxxxxxxxxxxC22222222Cx"
  "xxF555555FxxxxxxxxxxxxxxxxC222222Cxx"
  "xxxF5555FxeeeexxxxxxxxddddxC2222Cxxx"
  "xxxxffffxE4444ExxxxxxD3333Dxccccxxxx"
  "xxxxxxxxE444444ExxxxD333333Dxxxxxxxx"
  "xxxxxxxE44444444ExxD33333333Dxxxxxxx"
  "xxxxxxe4444444444ed3333333333dxxxxxx"
  "xxxxxxe4444444444ed3333333333dxxxxxx"
  "xxxxxxe4444444444ed3333333333dxxxxxx"
  "xxxxxxe4444444444ed3333333333dxxxxxx"
  "xxxxxxxE44444444ExxD33333333Dxxxxxxx"
  "xxxxxxxxE444444ExxxxD333333Dxxxxxxxx"
  "xxxxxxxxxE4444ExxxxxxD3333Dxxxxxxxxx"
  "xxxxxxxxxxeeeexxxxxxxxddddxxxxxxxxxx";

void
ColorButton::build_color_menu (QMenu *menu, QObject *receiver, const char *browse_slot, const char *selected_slot)
{
  tl_assert (selected_slot != 0);

  menu->clear ();

  menu->addAction (QObject::tr ("Automatic"), receiver, selected_slot)->setData (QVariant (QColor ()));
  if (browse_slot) {
    menu->addAction (QObject::tr ("Choose ..."), receiver, browse_slot);
  }
  menu->addSeparator ();

  try {

    std::string s;
    if (lay::Dispatcher::instance ()) {
      lay::Dispatcher::instance ()->config_get (cfg_color_palette, s);
    }
    lay::ColorPalette palette = lay::ColorPalette::default_palette ();
    if (! s.empty ()) {
      palette.from_string (s);
    }

    QMenu *submenu = 0;

    //  fill the list of stipple palette items
    for (unsigned int i = 0; i < palette.colors (); ++i) {

      if ((i % 6) == 0) {

        std::map<char, QColor> codes;
        codes.insert (std::make_pair ('x', QColor (0, 0, 0, 0)));
        for (int j = 0; j < 6; ++j) {
          QColor c = palette.color_by_index (i + j);
          codes.insert (std::make_pair ('0' + j, c));
          c.setAlpha (128);
          codes.insert (std::make_pair ('a' + j, c));
          c.setAlpha (192);
          codes.insert (std::make_pair ('A' + j, c));
        }

        int dpr = 1;
#if QT_VERSION >= 0x50000
        dpr = (menu->devicePixelRatio () >= 2.0) ? 2 : 1;
#endif

        QImage icon (18 * dpr, 16 * dpr, QImage::Format_ARGB32);
#if QT_VERSION >= 0x50000
        icon.setDevicePixelRatio (menu->devicePixelRatio ());
#endif
        const char *cp = dpr > 1 ? color_icon_2x : color_icon;
        for (int y = 0; y < icon.height (); ++y) {
          for (int x = 0; x < icon.width (); ++x) {
            icon.setPixel (x, y, codes [*cp].rgba ());
            ++cp;
          }
        }

        submenu = menu->addMenu (QPixmap::fromImage (icon), tl::to_qstring (tl::sprintf ("#%d .. %d", i + 1, std::min (i + 6, palette.colors ()))));

      }

      QColor color = QColor (palette.color_by_index (i));
      std::string name = tl::sprintf ("#%d", i + 1);

      QPixmap icon (16, 16);
#if QT_VERSION >= 0x50000
      icon.setDevicePixelRatio (menu->devicePixelRatio ());
#endif
      icon.fill (color);

      submenu->addAction (QIcon (icon), tl::to_qstring (name), receiver, selected_slot)->setData (QVariant (color));

    }

  } catch (...) { }
}

void 
ColorButton::build_menu ()
{
  build_color_menu (menu (), this, SLOT (browse_selected ()), SLOT (menu_selected ()));
}

void
ColorButton::set_color (QColor c)
{
  set_color_internal (c);
}

void 
ColorButton::set_color_internal (QColor c)
{
  m_color = c;

  QPushButton::setText (QString::fromUtf8 (" "));

  QString text = QString::fromUtf8 ("XXXXXXX");
  QFontMetrics fm (font (), this);
  QRect rt (fm.boundingRect (text)); // dummy text to be compliant with the other color button

  QPushButton::setIconSize (QSize (rt.width (), rt.height ()));

#if QT_VERSION >= 0x050000
  double dpr = devicePixelRatio ();
#else
  double dpr = 1.0;
#endif

  QPixmap pixmap (rt.width () * dpr, rt.height () * dpr);
#if QT_VERSION >= 0x50000
  pixmap.setDevicePixelRatio (dpr);
#endif
  pixmap.fill (QColor (0, 0, 0, 0));

  QColor text_color = palette ().color (QPalette::Active, QPalette::Text);
  QPainter pxpainter (&pixmap);
  pxpainter.setPen (QPen (text_color));

  if (! m_color.isValid ()) {

    pxpainter.setFont (font ());
    QRectF r (0, 0, rt.width () - pxpainter.pen ().widthF (), rt.height () - pxpainter.pen ().widthF ());
    pxpainter.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, QObject::tr ("Auto"));

  } else {

    pxpainter.setBrush (QBrush (c));
    QRectF r (0, 0, rt.width () - pxpainter.pen ().widthF (), rt.height () - pxpainter.pen ().widthF ());
    pxpainter.drawRect (r);

  }

  QPushButton::setIcon (QIcon (pixmap));
}

QColor 
ColorButton::get_color () const
{
  return m_color;
}

void
ColorButton::menu_about_to_show ()
{
  build_menu ();
}

void
ColorButton::menu_selected ()
{
  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {
    set_color (action->data ().value<QColor> ());
    emit color_changed (m_color);
  }
}

void
ColorButton::browse_selected ()
{
  QColor c = QColorDialog::getColor (get_color (), this);
  if (c.isValid ()) {
    set_color (c);
    emit color_changed (m_color);
  }
}

// -------------------------------------------------------------
//  MarginWidget implementation

MarginWidget::MarginWidget (QWidget *parent, const char *name)
  : QFrame (parent), m_margin ()
{
  if (name) {
    setObjectName (name);
  }

  setFrameStyle (QFrame::NoFrame);

  QHBoxLayout *layout = new QHBoxLayout (this);
  layout->setMargin (0);

  QLineEdit *le = new QLineEdit (this);
  mp_abs_edit = le;
  le->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred));
  layout->addWidget (le);

  le = new QLineEdit (this);
  mp_rel_edit = le;
  le->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred));
  layout->addWidget (le);

  QComboBox *mode = new QComboBox (this);
  mode->addItem (tl::to_qstring ("µm"));
  mode->addItem (tl::to_qstring ("%"));
  mp_mode_cb = mode;
  layout->addWidget (mode);

  connect (mode, SIGNAL (currentIndexChanged (int)), this, SLOT (mode_selection_changed ()));

  set_margin (lay::Margin ());
}

lay::Margin
MarginWidget::get_margin () const
{
  bool rel_mode = mp_mode_cb->currentIndex () == 1;
  double rel = 0.0, abs = 0.0;
  tl::from_string (tl::to_string (mp_rel_edit->text ()), rel);
  tl::from_string (tl::to_string (mp_abs_edit->text ()), abs);

  lay::Margin m = m_margin;
  m.set_relative_mode (rel_mode);
  if (rel_mode) {
    m.set_relative_value (rel * 0.01);
  } else {
    m.set_absolute_value (abs);
  }
  return m;
}

void
MarginWidget::set_margin (const lay::Margin &margin)
{
  m_margin = margin;

  mp_abs_edit->setText (tl::to_qstring (tl::to_string (margin.absolute_value ())));
  mp_rel_edit->setText (tl::to_qstring (tl::to_string (margin.relative_value () * 100.0)));
  mp_mode_cb->setCurrentIndex (margin.relative_mode () ? 1 : 0);
  mode_selection_changed ();
}

void
MarginWidget::mode_selection_changed ()
{
  bool rel_mode = mp_mode_cb->currentIndex () == 1;
  //  NOTE: first hiding and then showing avoids layout flicker ..
  mp_rel_edit->hide ();
  mp_abs_edit->hide ();
  if (rel_mode) {
    mp_rel_edit->show ();
  } else {
    mp_abs_edit->show ();
  }
}

// -------------------------------------------------------------
//  DecoratedLineEdit implementation

const int le_frame_width = 4; //  TODO: obtain from style?
const int le_decoration_space = 2; //  additional distance between decoration icons and text

DecoratedLineEdit::DecoratedLineEdit (QWidget *parent)
  : QLineEdit (parent),
    m_clear_button_enabled (false), m_options_button_enabled (false),
    m_escape_signal_enabled (false), m_tab_signal_enabled (false),
    mp_options_menu (0)
{
  mp_options_label = new QLabel (this);
  mp_options_label->hide ();
  mp_options_label->setCursor (Qt::ArrowCursor);
  mp_options_label->setPixmap (QString::fromUtf8 (":/options_edit_16px@2x.png"));

  mp_clear_label = new QLabel (this);
  mp_clear_label->hide ();
  mp_clear_label->setCursor (Qt::ArrowCursor);
  mp_clear_label->setPixmap (QString::fromUtf8 (":/clear_edit_16px@2x.png"));

  QMargins margins = textMargins ();
  m_default_left_margin = margins.left ();
  m_default_right_margin = margins.right ();
}

DecoratedLineEdit::~DecoratedLineEdit ()
{
  //  .. nothing yet ..
}

void DecoratedLineEdit::set_escape_signal_enabled (bool en)
{
  m_escape_signal_enabled = en;
}

void DecoratedLineEdit::set_tab_signal_enabled (bool en)
{
  m_tab_signal_enabled = en;
}

bool DecoratedLineEdit::event (QEvent *event)
{
  //  Handling this event makes the widget receive all keystrokes
  if (event->type () == QEvent::ShortcutOverride) {
    QKeyEvent *ke = static_cast<QKeyEvent *> (event);
    if (ke->key () == Qt::Key_Escape && m_escape_signal_enabled) {
      ke->accept ();
    } else if ((ke->key () == Qt::Key_Tab || ke->key () == Qt::Key_Backtab) && m_tab_signal_enabled) {
      ke->accept ();
    }
  }
  return QLineEdit::event (event);
}

void DecoratedLineEdit::keyPressEvent (QKeyEvent *event)
{
  if (m_escape_signal_enabled && event->key () == Qt::Key_Escape) {
    emit esc_pressed ();
    event->accept ();
  } else if (m_tab_signal_enabled && event->key () == Qt::Key_Tab) {
    emit tab_pressed ();
    event->accept ();
  } else if (m_tab_signal_enabled && event->key () == Qt::Key_Backtab) {
    emit backtab_pressed ();
    event->accept ();
  } else {
    QLineEdit::keyPressEvent (event);
  }
}

bool DecoratedLineEdit::focusNextPrevChild (bool next)
{
  if (m_tab_signal_enabled && isEnabled ()) {
    QKeyEvent event (QEvent::KeyPress, next ? Qt::Key_Tab : Qt::Key_Backtab, Qt::NoModifier);
    keyPressEvent (&event);
    if (event.isAccepted ()) {
      return true;
    }
  }
  return QLineEdit::focusNextPrevChild (next);
}

void DecoratedLineEdit::set_clear_button_enabled (bool en)
{
  if (en != m_clear_button_enabled) {

    m_clear_button_enabled = en;
    mp_clear_label->setVisible (en);

    QMargins margins = textMargins ();
    if (! en) {
      margins.setRight (m_default_right_margin);
    } else {
      margins.setRight (m_default_right_margin + mp_clear_label->sizeHint ().width () + le_decoration_space);
    }
    setTextMargins (margins);

    resizeEvent (0);

  }
}

void DecoratedLineEdit::set_options_button_enabled (bool en)
{
  if (en != m_options_button_enabled) {

    m_options_button_enabled = en;
    mp_options_label->setVisible (en);

    QMargins margins = textMargins ();
    if (! en) {
      margins.setLeft (m_default_left_margin);
    } else {
      margins.setLeft (m_default_left_margin + mp_options_label->sizeHint ().width () + le_decoration_space);
    }
    setTextMargins (margins);

    resizeEvent (0);

  }
}

void DecoratedLineEdit::set_options_menu (QMenu *menu)
{
  mp_options_menu = menu;
}

void DecoratedLineEdit::mouseReleaseEvent (QMouseEvent *event)
{
  if (event->button () == Qt::LeftButton) {

    QWidget *c = childAt (event->pos ());
    if (c == mp_clear_label) {

      clear ();
      emit clear_pressed (); //  might modify the text
      emit textEdited (text ());

    }

  }
}

void DecoratedLineEdit::mousePressEvent (QMouseEvent *event)
{
  if (event->button () == Qt::LeftButton) {

    QWidget *c = childAt (event->pos ());
    if (c == mp_options_label) {

      if (mp_options_menu) {
        mp_options_menu->popup (event->globalPos ());
      } else {
        emit options_button_clicked ();
      }

    }

  }
}

void DecoratedLineEdit::resizeEvent (QResizeEvent * /*event*/)
{
  int fw = hasFrame () ? le_frame_width : 0;

  if (m_clear_button_enabled) {
    QSize label_size = mp_clear_label->sizeHint ();
    QRect r = geometry ();
    mp_clear_label->setGeometry (r.width () - fw - label_size.width (), 0, label_size.width (), r.height ());
  }

  if (m_options_button_enabled) {
    QSize label_size = mp_options_label->sizeHint ();
    QRect r = geometry ();
    mp_options_label->setGeometry (fw, 0, label_size.width (), r.height ());
  }
}

// -------------------------------------------------------------
//  InteractiveListWidget implementation

InteractiveListWidget::InteractiveListWidget (QWidget *parent)
  : QListWidget (parent)
{
  setSelectionMode (QAbstractItemView::ExtendedSelection);
  setDragDropMode (QAbstractItemView::InternalMove);
}

void
InteractiveListWidget::set_values (const std::vector<std::string> &values)
{
  clear ();
  add_values (values);
}

std::vector<std::string>
InteractiveListWidget::get_values ()
{
  std::vector<std::string> v;
  v.reserve ((size_t) count ());
  for (int i = 0; i < count (); ++i) {
    v.push_back (tl::to_string (item (i)->text ()));
  }
  return v;
}

void
InteractiveListWidget::add_value (const std::string &value)
{
  addItem (tl::to_qstring (value));
  refresh_flags ();
  clearSelection ();
  setCurrentItem (item (count () - 1));
}

void
InteractiveListWidget::add_values (const std::vector<std::string> &values)
{
  for (std::vector<std::string>::const_iterator i = values.begin (); i != values.end (); ++i) {
    addItem (tl::to_qstring (*i));
  }
  refresh_flags ();
  clearSelection ();
}

void
InteractiveListWidget::delete_selected_items ()
{
  QStringList items;
  for (int i = 0; i < count (); ++i) {
    if (! item (i)->isSelected ()) {
      items.push_back (item (i)->text ());
    }
  }

  clear ();
  for (QStringList::const_iterator f = items.begin (); f != items.end (); ++f) {
    addItem (*f);
  }
  refresh_flags ();
}

void
InteractiveListWidget::move_selected_items_up ()
{
  std::set<QString> selected;
  for (int i = 0; i < count (); ++i) {
    if (item (i)->isSelected ()) {
      selected.insert (item (i)->text ());
    }
  }

  QStringList items;
  int j = -1;
  for (int i = 0; i < count (); ++i) {
    if (item (i)->isSelected ()) {
      items.push_back (item (i)->text ());
    } else {
      if (j >= 0) {
        items.push_back (item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    items.push_back (item (j)->text ());
  }

  clear ();
  for (QStringList::const_iterator f = items.begin (); f != items.end (); ++f) {
    addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      item (count () - 1)->setSelected (true);
    }
  }
  refresh_flags ();
}

void
InteractiveListWidget::move_selected_items_down ()
{
  std::set<QString> selected;
  for (int i = 0; i < count (); ++i) {
    if (item (i)->isSelected ()) {
      selected.insert (item (i)->text ());
    }
  }

  QStringList items;
  int j = -1;
  for (int i = count (); i > 0; ) {
    --i;
    if (item (i)->isSelected ()) {
      items.push_back (item (i)->text ());
    } else {
      if (j >= 0) {
        items.push_back (item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    items.push_back (item (j)->text ());
  }

  clear ();
  for (QStringList::const_iterator f = items.end (); f != items.begin (); ) {
    --f;
    addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      item (count () - 1)->setSelected (true);
    }
  }
  refresh_flags ();
}

void
InteractiveListWidget::refresh_flags ()
{
  for (int i = 0; i < count (); ++i) {
    item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
  }
}

}

#endif
