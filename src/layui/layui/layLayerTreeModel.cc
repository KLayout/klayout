
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

#include "layLayerTreeModel.h"
#include "layLayoutViewBase.h"
#include "dbLayoutUtils.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlGlobPattern.h"

#include <QTreeView>
#include <QModelIndex>
#include <QString>
#include <QFontInfo>

#include <string.h>

#include <algorithm>

namespace lay {

// --------------------------------------------------------------------
//  EmptyWithinViewCache implementation

EmptyWithinViewCache::EmptyWithinViewCache ()
{
  //  .. nothing yet ..
}

void
EmptyWithinViewCache::clear ()
{
  m_cache.clear();
}

bool
EmptyWithinViewCache::is_empty_within_view (const db::Layout *layout, unsigned int cell_index, const db::Box &box, unsigned int layer)
{
  std::pair<cache_t::iterator, bool> c = m_cache.insert (std::make_pair(std::make_pair(std::make_pair(layout, cell_index), box), std::set<unsigned int> ()));
  if (c.second) {

    tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (QObject::tr ("Recomputing layers with shapes in view")));

    const db::Cell &cell (layout->cell (cell_index));

    //  determine layers with shapes on the given layout and within the given box
    std::vector <unsigned int> ll;
    for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {
      if (cell.bbox ((*l).first).empty ()) {
        c.first->second.insert ((*l).first);
      } else if (! cell.shapes ((*l).first).begin_touching (box, db::ShapeIterator::All, 0, false).at_end ()) {
        ;
      } else {
        ll.push_back ((*l).first);
      }
    }

    m_cells_done.resize (layout->cells (), false);
    determine_empty_layers(layout, cell_index, box, ll);
    m_cells_done = std::vector<bool> ();

    c.first->second.insert (ll.begin (), ll.end ());

  }

  return c.first->second.find(layer) != c.first->second.end();
}

void 
EmptyWithinViewCache::determine_empty_layers (const db::Layout *layout, unsigned int cell_index, const db::Box &box, std::vector<unsigned int> &layers)
{
  if (layers.empty ()) {
    return;
  }

  //  Hint: this implementation counts all hierarchy levels - also the ones not shown ...

  db::box_convert<db::CellInst> bc (*layout);

  db::Cell::touching_iterator inst = layout->cell (cell_index).begin_touching (box);
  while (! inst.at_end () && ! layers.empty ()) {

    db::cell_index_type ci = inst->cell_index ();
    if (! m_cells_done [ci]) {

      const db::Cell &cell (layout->cell (ci));

      if (inst->bbox (bc).inside (box)) {

        //  The instance is fully inside the search box: remove the non-empty layers from the 
        //  list and mark the cell as "done".
        std::vector<unsigned int>::iterator lw = layers.begin ();
        for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
          if (cell.bbox (*l).empty ()) {
            *lw++ = *l;
          }
        }
        layers.erase (lw, layers.end ());

        m_cells_done [ci] = true;

      } else {

        //  Remove every layer from the list which has at least one shape on it within the box and
        //  temporarily create a list of layers with only the relevant layers in ll

        std::vector<unsigned int> ll;
        ll.reserve (layers.size ());

        std::vector<unsigned int>::iterator lw = layers.begin ();
        for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
          if (cell.bbox (*l).empty ()) {
            *lw++ = *l;
          } else {
            ll.push_back (*l);
          }
        }

        layers.erase (lw, layers.end ());

        if (! ll.empty ()) {

          db::CellInstArray::iterator inst_array = inst->cell_inst ().begin_touching (box, bc); 
          while (! inst_array.at_end () && ! ll.empty ()) {

            db::Box new_box = db::Box (inst->complex_trans (*inst_array).inverted () * box);

            //  remove all layers which became populated in that instance
            std::vector<unsigned int>::iterator llw = ll.begin ();
            for (std::vector<unsigned int>::const_iterator l = ll.begin (); l != ll.end (); ++l) {
              if (cell.shapes (*l).begin_touching (new_box, db::ShapeIterator::All, 0, false).at_end ()) {
                *llw++ = *l;
              }
            }
            ll.erase (llw, ll.end ());

            determine_empty_layers (layout, inst->cell_index (), new_box, ll);

            ++inst_array;

          }

          //  Join the lists of remaining layers
          layers.insert (layers.end (), ll.begin (), ll.end ());

        }

      }

    }

    ++inst;

  }
}

// --------------------------------------------------------------------
//  LayerTreeModel implementation

LayerTreeModel::LayerTreeModel (QWidget *parent, lay::LayoutViewBase *view)
  : QAbstractItemModel (parent), 
    mp_parent (parent), mp_view (view), m_filter_mode (false), m_id_start (0), m_id_end (0),
    m_phase ((unsigned int) -1),
    m_test_shapes_in_view (false), m_hide_empty_layers (false)
{
  // .. nothing yet ..
}

LayerTreeModel::~LayerTreeModel ()
{
  // .. nothing yet ..
}

void
LayerTreeModel::set_phase (unsigned int ph)
{
  m_phase = ph;
}

void 
LayerTreeModel::set_font (const QFont &font)
{
  m_font = font;
  signal_data_changed ();
}

void
LayerTreeModel::set_font_no_signal (const QFont &font)
{
  m_font = font;
}

void
LayerTreeModel::set_text_color (QColor color)
{
  m_text_color = color;
  signal_data_changed ();
}

void
LayerTreeModel::set_test_shapes_in_view (bool f)
{
  if (m_test_shapes_in_view != f) {
    m_test_shapes_in_view = f;
    if (m_hide_empty_layers) {
      emit hidden_flags_need_update ();
    }
    signal_data_changed ();
  }
}

void
LayerTreeModel::set_hide_empty_layers (bool f)
{
  if (m_hide_empty_layers != f) {
    m_hide_empty_layers = f;
    //  we actually can't do this ourselves.
    emit hidden_flags_need_update ();
  }
}

void
LayerTreeModel::set_filter_mode (bool f)
{
  if (f != m_filter_mode) {
    m_filter_mode = f;
    emit hidden_flags_need_update ();
  }
}

void 
LayerTreeModel::set_background_color (QColor background)
{
  m_background_color = background;
  signal_data_changed ();
}

Qt::ItemFlags 
LayerTreeModel::flags (const QModelIndex &index) const
{
  return QAbstractItemModel::flags (index);
}

int 
LayerTreeModel::columnCount (const QModelIndex &) const 
{
  return 2;
}

QVariant 
LayerTreeModel::headerData (int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
  return QVariant ();
}

int 
LayerTreeModel::rowCount (const QModelIndex &parent) const 
{
  if (mp_view->layer_model_updated ()) {

    if (parent.isValid ()) {
      lay::LayerPropertiesConstIterator iter (iterator (parent));
      if (iter.is_null () || iter.at_end ()) {
        return 0;
      } else {
        return iter->end_children () - iter->begin_children ();
      }
    } else {
      int n = int (mp_view->get_properties ().end_const () - mp_view->get_properties ().begin_const ());
      return n;
    }

  } else {
    return 0;
  }
}

QModelIndex 
LayerTreeModel::index (int row, int column, const QModelIndex &parent) const 
{
  if (row >= 0 && row < rowCount (parent)) {
    if (parent.isValid ()) {
      lay::LayerPropertiesConstIterator iter (iterator (parent));
      if (iter.is_null () || iter.at_end ()) {
        return QModelIndex ();
      } else {
        iter.down_first_child ();
        iter.next_sibling (row);
        return createIndex (row, column, (void *) (iter.uint () + m_id_start));
      }
    } else {
      lay::LayerPropertiesConstIterator iter (mp_view->begin_layers ());
      iter.next_sibling (row);
      return createIndex (row, column, (void *) (iter.uint () + m_id_start));
    }
  } else {
    return QModelIndex ();
  }
}

void
LayerTreeModel::clear_locate ()
{
  m_selected_indexes.clear ();
  m_current_index = m_selected_indexes.begin ();
  m_selected_ids.clear ();

  signal_data_changed ();

  if (m_filter_mode) {
    emit hidden_flags_need_update ();
  }
}

QModelIndex
LayerTreeModel::locate_next ()
{
  if (m_current_index == m_selected_indexes.end ()) {
    return QModelIndex ();
  } else {
    ++m_current_index;
    if (m_current_index == m_selected_indexes.end ()) {
      m_current_index = m_selected_indexes.begin ();
    }
    return *m_current_index;
  }
}

QModelIndex
LayerTreeModel::locate_prev ()
{
  if (m_current_index == m_selected_indexes.end ()) {
    return QModelIndex ();
  } else {
    if (m_current_index == m_selected_indexes.begin ()) {
      m_current_index = m_selected_indexes.end ();
    }
    --m_current_index;
    return *m_current_index;
  }
}

void
LayerTreeModel::search_children (const tl::GlobPattern &pattern, const QModelIndex &parent, bool recurse)
{
  int children = rowCount (parent);
  for (int i = 0; i < children; ++i) {

    QModelIndex child = index (i, 0, parent);

    lay::LayerPropertiesConstIterator iter (iterator (child));
    if (!iter.is_null () && !iter.at_end () &&
        pattern.match (iter->display_string (mp_view, true /*real*/))) {
      m_selected_indexes.push_back (child);
    }

    if (recurse && iter->has_children ()) {
      search_children (pattern, child, recurse);
    }

  }
}

QModelIndex
LayerTreeModel::locate (const char *name, bool glob_pattern, bool case_sensitive, bool top_only)
{
  m_selected_indexes.clear ();

  tl::GlobPattern p = tl::GlobPattern (std::string (name));
  p.set_case_sensitive (case_sensitive);
  p.set_exact (!glob_pattern);
  p.set_header_match (true);

  search_children (p, QModelIndex (), !top_only);

  m_selected_ids.clear ();
  for (std::vector<QModelIndex>::const_iterator i = m_selected_indexes.begin (); i != m_selected_indexes.end (); ++i) {
    m_selected_ids.insert (size_t (i->internalPointer ()));
  }

  signal_data_changed ();

  if (m_filter_mode) {
    emit hidden_flags_need_update ();
  }

  m_current_index = m_selected_indexes.begin ();
  if (m_current_index == m_selected_indexes.end ()) {
    return QModelIndex ();
  } else {
    return *m_current_index;
  }
}

void 
LayerTreeModel::signal_data_changed () 
{
  m_test_shapes_cache.clear ();
  emit dataChanged (upperLeft (), bottomRight ());
}

void 
LayerTreeModel::signal_begin_layer_changed () 
{
  m_id_start = m_id_end; // means: model is invalid
  m_test_shapes_cache.clear ();
  emit layoutAboutToBeChanged ();
}

void 
LayerTreeModel::signal_layers_changed ()
{
  //  establish a new range of valid iterator indices
  m_id_start = m_id_end; 

  //  TODO: is there a more efficient way to compute that?
  size_t max_id = 0;
  for (lay::LayerPropertiesConstIterator iter = mp_view->get_properties(); ! iter.at_end (); ++iter) {
    max_id = std::max (iter.uint (), max_id);
  }
  m_id_end += max_id + 1;

  //  update the persistent indexes

  QModelIndexList indexes = persistentIndexList ();
  QModelIndexList new_indexes;
  for (QModelIndexList::const_iterator i = indexes.begin (); i != indexes.end (); ++i) {
    lay::LayerPropertiesConstIterator li = iterator (*i);
    if (! li.at_end ()) {
      new_indexes.push_back (createIndex (int (li.child_index ()), i->column (), (void *) (li.uint () + m_id_start)));
    } else {
      new_indexes.push_back (QModelIndex ());
    }
  }

  changePersistentIndexList (indexes, new_indexes);

  m_test_shapes_cache.clear ();
  emit layoutChanged ();
}

QModelIndex
LayerTreeModel::upperLeft () const
{
  if (mp_view->layer_model_updated ()) {
    lay::LayerPropertiesConstIterator iter (mp_view->begin_layers ());
    iter.next_sibling (0);
    return createIndex (0, 0, (void *) (iter.uint () + m_id_start));
  } else {
    return QModelIndex ();
  }
}

QModelIndex
LayerTreeModel::bottomRight () const
{
  if (mp_view->layer_model_updated ()) {

    //  navigate to the last top-level item
    lay::LayerPropertiesConstIterator iter (mp_view->begin_layers ());
    int n = int (mp_view->get_properties ().end_const () - mp_view->get_properties ().begin_const ()) - 1;
    iter.next_sibling (n);

    //  navigate to the last child 
    QModelIndex p = createIndex (n, 1, (void *) (iter.uint () + m_id_start));
    int nr = 0;
    while (p.isValid () && (nr = rowCount (p)) > 0) {
      p = index (nr - 1, 0, p);
    }
    return p;

  } else {
    return QModelIndex ();
  }
}

QModelIndex 
LayerTreeModel::parent (const QModelIndex &index) const 
{
  try {

    if (mp_view->layer_model_updated ()) {
      lay::LayerPropertiesConstIterator iter (iterator (index));
      if (iter.is_null () || iter.at_end ()) {
        return QModelIndex ();
      } else {
        iter.up ();
        if (iter.is_null ()) {
          return QModelIndex ();
        } else {
          //  It is important that the column index of the parent is 0.
          //  Otherwise the tree view will not behave as expected.
          return createIndex (int (iter.child_index ()), 0, (void *) (iter.uint () + m_id_start));
        }
      }
    } else {
      return QModelIndex ();
    }

  } catch (tl::Exception &ex) {
    //  this can happen because some internal indices might not be in place properly - in particular the 
    //  element the mouse was over (hover index)
    tl::warn << ex.msg ();
    return QModelIndex ();
  }
}

bool
LayerTreeModel::is_hidden (const QModelIndex &index) const
{
  if (m_filter_mode && ! m_selected_ids.empty () && m_selected_ids.find (size_t (index.internalPointer ())) == m_selected_ids.end ()) {
    return true;
  }

  if (! m_hide_empty_layers) {
    return false;
  } else if (m_test_shapes_in_view) {
    return empty_within_view_predicate (index);
  } else {
    return empty_predicate (index);
  }
}

bool 
LayerTreeModel::empty_predicate (const QModelIndex &index) const
{
  lay::LayerPropertiesConstIterator iter (iterator (index));
  if (iter.is_null () || iter.at_end ()) {
    return true;
  } else if (iter->is_cell_box_layer () || iter->is_standard_layer ()) {
    return iter->bbox ().empty ();
  } else {
    //  special purpose layers are always visible
    return false;
  }
}

bool 
LayerTreeModel::empty_within_view_predicate (const QModelIndex &index) const
{
  lay::LayerPropertiesConstIterator iter (iterator (index));
  if (iter.is_null () || iter.at_end ()) {
    return false;
  } else if (iter->is_standard_layer ()) {

    int cv_index = iter->cellview_index ();
    if (! mp_view->cellview (cv_index).is_valid ()) {
      return true;
    }

    const lay::CellView &cv = mp_view->cellview (cv_index);

    const db::Layout &layout = cv->layout ();

    int layer_id = iter->layer_index ();
    if (! layout.is_valid_layer (layer_id)) {
      return true;
    }

    db::cell_index_type ci = cv.cell_index ();
    const db::Cell &cell = layout.cell (ci);
    db::ICplxTrans ctx_trans = cv.context_trans ();

    std::vector<db::DCplxTrans> trans = iter->trans ();

    const lay::Viewport &vp = mp_view->viewport ();
    db::DCplxTrans vp_trans = vp.trans ();
    int width = vp.width ();
    int height = vp.height ();

    for (std::vector<db::DCplxTrans>::const_iterator t = trans.begin (); t != trans.end (); ++t) {

      db::CplxTrans ct = vp_trans * *t * db::CplxTrans(layout.dbu ()) * ctx_trans;

      // the following scheme to compute the region avoids problems with accessing designs through very large viewports:
      db::Coord lim = std::numeric_limits<db::Coord>::max ();
      db::DBox world (ct * db::Box (db::Point (-lim, -lim), db::Point (lim, lim)));
      db::Box region = ct.inverted () * (world & db::DBox (db::DPoint (0.0, 0.0), db::DPoint (width, height)));
      region &= cell.bbox (); 

      if (! m_test_shapes_cache.is_empty_within_view (&layout, ci, region, layer_id)) {
        return false;
      }

    }

    return true;

  } else if (iter->is_cell_box_layer ()) {

    //  There is no "within view" method for cell frame layers currently.
    return iter->bbox ().empty ();

  } else {

    //  Other special purpose layers are always visible
    return false;

  }
}

QIcon
LayerTreeModel::icon_for_layer (const lay::LayerPropertiesConstIterator &iter, lay::LayoutViewBase *view, unsigned int w, unsigned int h, double dpr, unsigned int di_off, bool no_state)
{
  QImage img = view->icon_for_layer (iter, w, h, dpr, di_off, no_state).to_image_copy ();
  QPixmap pixmap = QPixmap::fromImage (std::move (img));
#if QT_VERSION >= 0x050000
  pixmap.setDevicePixelRatio (dpr);
#endif
  return QIcon (pixmap);
}

QSize
LayerTreeModel::icon_size () const
{
  unsigned int is = ((QFontInfo (m_font).pixelSize () + 15) / 16) * 16;
  return QSize (is * 2, is);
}

QVariant 
LayerTreeModel::data (const QModelIndex &index, int role) const 
{
  if (mp_view->layer_model_updated ()) {

    lay::LayerPropertiesConstIterator iter (iterator (index));
    if (iter.is_null () || iter.at_end ()) {

      return QVariant ();

    } else if (role == Qt::SizeHintRole) {

      if (index.column () == 0) {
        //  NOTE: for some reason, the widget clips the icon when inside a tree and needs a some what bigger width ..
        QSize is = icon_size ();
        return QVariant (is + QSize (is.width () / 4, 0));
      } else {
        return QVariant ();
      }

    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {

      if (index.column () == 1) {
        return QVariant (tl::to_qstring (iter->display_string (mp_view, true /*real*/)));
      }

    } else if (role == Qt::DecorationRole) {

      if (index.column () == 0) {

        bool animate_visible = true;
        unsigned int di_off = 0;
        if (iter->animation (true)) {
          if (iter->animation (true) == 1) {
            // scrolling 
            di_off += m_phase;
          } else if (iter->animation (true) == 2) {
            // blinking
            animate_visible = ((m_phase & 1) == 0);
          } else {
            // inversely blinking
            animate_visible = ((m_phase & 1) != 0);
          }
        }

        QSize is = icon_size ();

        if (animate_visible) {
#if QT_VERSION >= 0x050000
          double dpr = mp_parent ? mp_parent->devicePixelRatio () : 1.0;
#else
          double dpr = 1.0;
#endif
          return QVariant (icon_for_layer (iter, mp_view, is.width (), is.height (), dpr, di_off));
        } else {
          return QVariant (QIcon ());
        }

      }

    } else if (role == Qt::BackgroundRole) {

      if (mp_parent && m_selected_ids.find (size_t (index.internalPointer ())) != m_selected_ids.end ()) {
        //  for selected items pick a color between Highlight and Base
        QPalette pl (mp_parent->palette ());
        QColor c1 = pl.color (QPalette::Highlight);
        QColor cb = pl.color (QPalette::Base);
        return QVariant (QColor ((c1.red () + cb.red ()) / 2, (c1.green () + cb.green ()) / 2, (c1.blue () + cb.blue ()) / 2));
      } else {
        return QVariant ();
      }

    } else if (role == Qt::ForegroundRole || role == Qt::FontRole) {

      if (index.column () == 1) {

        QColor c1 = m_text_color;
        QColor cb = m_background_color;
        QColor c0 = QColor ((c1.red () + cb.red ()) / 2, (c1.green () + cb.green ()) / 2, (c1.blue () + cb.blue ()) / 2);

        bool empty;
        if (m_test_shapes_in_view) {
          empty = empty_within_view_predicate (index);
        } else {
          empty = iter->bbox ().empty ();
        }

        //  set the color to one with less contrast if there is nothing on this layer here
        if (role == Qt::FontRole) {
          QFont f = m_font;
          f.setBold (! empty);
          return QVariant (f);
        } else {
          return QVariant (empty ? c0 : c1);
        }

      }

    } 

  }

  return QVariant ();
}

lay::LayerPropertiesConstIterator 
LayerTreeModel::iterator (const QModelIndex &index) const
{
  if (index.isValid ()) {
    size_t iter_index = size_t (index.internalPointer ());
    if (mp_view->layer_lists () > 0 && iter_index >= m_id_start && iter_index < m_id_end) {
      return lay::LayerPropertiesConstIterator (mp_view->get_properties (), iter_index - m_id_start);
    }
  }

  return lay::LayerPropertiesConstIterator ();
}

lay::LayerPropertiesIterator
LayerTreeModel::iterator_nc (const QModelIndex &index)
{
  if (index.isValid ()) {
    size_t iter_index = size_t (index.internalPointer ());
    if (mp_view->layer_lists () > 0 && iter_index >= m_id_start && iter_index < m_id_end) {
      return lay::LayerPropertiesIterator (mp_view->get_properties (), iter_index - m_id_start);
    }
  }

  return lay::LayerPropertiesIterator ();
}

QModelIndex
LayerTreeModel::index (lay::LayerPropertiesConstIterator iter, int column) const
{
  try {

    std::vector<int> rows;
    
    while (! iter.is_null ()) {
      rows.push_back (int (iter.child_index ()));
      iter = iter.parent ();
    }

    QModelIndex idx;

    for (std::vector<int>::reverse_iterator r = rows.rbegin (); r != rows.rend (); ++r) {
      idx = index (*r, column, idx);
    }

    return idx;

  } catch (tl::Exception &ex) {
    tl::warn << ex.msg ();
    return QModelIndex ();
  }
}

} // namespace lay

#endif
