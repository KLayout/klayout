
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "layLayerTreeModel.h"
#include "layLayoutView.h"
#include "layBitmapsToImage.h"
#include "dbLayoutUtils.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlGlobPattern.h"

#include <QTreeView>
#include <QModelIndex>
#include <QString>

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

LayerTreeModel::LayerTreeModel (QWidget *parent, lay::LayoutView *view)
  : QAbstractItemModel (parent), 
    mp_view (view), m_id_start (0), m_id_end (0), m_phase ((unsigned int) -1), m_test_shapes_in_view (false)
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
LayerTreeModel::set_text_color (QColor color)
{
  m_text_color = color;
  signal_data_changed ();
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
LayerTreeModel::signal_layer_changed () 
{
  // establish a new range of valid iterator indices
  m_id_start = m_id_end; 

  //  TODO: is there a more efficient way to compute that?
  size_t max_id = 0;
  for (lay::LayerPropertiesConstIterator iter = mp_view->get_properties(); ! iter.at_end (); ++iter) {
    max_id = std::max (iter.uint (), max_id);
  }
  m_id_end += max_id + 1;

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

/**
 *  @brief A helper function to create an image from a single bitmap
 */
static void
single_bitmap_to_image (const lay::ViewOp &view_op, lay::Bitmap &bitmap,
                        QImage *pimage, const lay::DitherPattern &dither_pattern, const lay::LineStyles &line_styles,
                        unsigned int width, unsigned int height)
{
  std::vector <lay::ViewOp> view_ops;
  view_ops.push_back (view_op);

  std::vector <lay::Bitmap *> pbitmaps;
  pbitmaps.push_back (&bitmap);

  lay::bitmaps_to_image (view_ops, pbitmaps, dither_pattern, line_styles, pimage, width, height, false, 0);
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

QVariant 
LayerTreeModel::data (const QModelIndex &index, int role) const 
{
  if (mp_view->layer_model_updated ()) {

    lay::LayerPropertiesConstIterator iter (iterator (index));
    if (iter.is_null () || iter.at_end ()) {

      return QVariant ();

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
            // inversly blinking
            animate_visible = ((m_phase & 1) != 0);
          }
        }

        unsigned int w = 32;
        unsigned int h = 16;
          
        if (animate_visible) {

          lay::color_t def_color   = 0x808080;
          lay::color_t fill_color  = iter->has_fill_color (true)  ? iter->eff_fill_color (true)  : def_color;
          lay::color_t frame_color = iter->has_frame_color (true) ? iter->eff_frame_color (true) : def_color;

          QImage image (w, h, QImage::Format_RGB32);
          image.fill (m_background_color.rgb ());

          //  TODO: adjust the resolution according to the oversampling mode
          lay::Bitmap fill (w, h, 1.0);
          lay::Bitmap frame (w, h, 1.0);
          lay::Bitmap text (w, h, 1.0);
          lay::Bitmap vertex (w, h, 1.0);

          unsigned int mask_w      = 31;
          unsigned int mask_all    = 0xfffffffe;
          unsigned int mask_left   = 0x80000000;
          unsigned int mask_right  = 0x00000002;
          unsigned int mask_center = 0x00010000;

          if (! iter->visible (true)) {

            mask_w      = 8;
            mask_all    = 0xff800000;
            mask_left   = 0x80000000;
            mask_right  = 0x00800000;
            mask_center = 0x08000000;

            //  Show the arrow if it is invisible also locally.
            if (! iter->visible (false)) {
              text.scanline (4) [0]  = 0x00008000 << 1;
              text.scanline (5) [0]  = 0x00018000 << 1;
              text.scanline (6) [0]  = 0x00038000 << 1;
              text.scanline (7) [0]  = 0x00078000 << 1;
              text.scanline (8) [0]  = 0x00038000 << 1;
              text.scanline (9) [0]  = 0x00018000 << 1;
              text.scanline (10) [0] = 0x00008000 << 1;
            }

          }

          if (mp_view->no_stipples ()) {
            //  Show a partial stipple pattern only for "no stipple" mode
            for (unsigned int i = 1; i < h - 2; ++i) {
              fill.scanline (i) [0] = 0xff800000;
            }
          } else {
            for (unsigned int i = 1; i < h - 2; ++i) {
              fill.scanline (i) [0] = mask_all;
            }
          }

          int lw = iter->width (true);
          if (lw < 0) {
            //  default line width is 0 for parents and 1 for leafs
            lw = iter->has_children () ? 0 : 1;
          }

          int p0 = lw / 2;
          int p1 = (lw - 1) / 2;
          if (p0 < 0) {
            p0 = 0;
          } else if (p0 > 7) {
            p0 = 7;
          }
          if (p1 < 0) {
            p1 = 0;
          } else if (p1 > 7) {
            p1 = 7;
          }

          int p0x = p0, p1x = p1;
          unsigned int ddx = 0;
          unsigned int ddy = h - 2 - p1 - p0;
          if (iter->xfill (true)) {
            ddx = mask_w - p0 - p1 - 1;
          }
          unsigned int d = ddx / 2;

          frame.scanline (p0) [0] = mask_all << p1;
          for (unsigned int i = p0; i < h - 2; ++i) {
            frame.scanline (i) [0] |= (mask_left >> p0) | (mask_right << p1);
            frame.scanline (i) [0] |= (mask_left >> p0x) | (mask_right << p1x);
            while (d < ddx) {
              d += ddy;
              frame.scanline (i) [0] |= (mask_left >> p0x) | (mask_right << p1x);
              ++p0x;
              ++p1x;
            }
            if (d >= ddx) {
              d -= ddx;
            }
          }
          frame.scanline (h - 2 - p1) [0] = mask_all << p1;

          if (! iter->valid (true)) {

            text.scanline (4) [0]  |= 0x00000c60;
            text.scanline (5) [0]  |= 0x00000ee0;
            text.scanline (6) [0]  |= 0x000007c0;
            text.scanline (7) [0]  |= 0x00000380;
            text.scanline (8) [0]  |= 0x000007c0;
            text.scanline (9) [0]  |= 0x00000ee0;
            text.scanline (10) [0] |= 0x00000c60;

            for (unsigned int i = 3; i < 12; ++i) {
              fill.scanline (i) [0] &= ~0x00001ff0;
              frame.scanline (i) [0] &= ~0x00001ff0;
            }

          }

          vertex.scanline (h / 2 - 1) [0] = mask_center;

          lay::ViewOp::Mode mode = lay::ViewOp::Copy;

          //  create fill 
          single_bitmap_to_image (lay::ViewOp (fill_color, mode, 0, iter->eff_dither_pattern (true), di_off), fill, &image, mp_view->dither_pattern (), mp_view->line_styles (), w, h);
          //  create frame 
          if (lw == 0) {
            single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0 /*solid line*/, 2 /*dotted*/, 0), frame, &image, mp_view->dither_pattern (), mp_view->line_styles (), w, h);
          } else {
            single_bitmap_to_image (lay::ViewOp (frame_color, mode, iter->eff_line_style (true), 0, 0, lay::ViewOp::Rect, lw), frame, &image, mp_view->dither_pattern (), mp_view->line_styles (), w, h);
          }
          //  create text 
          single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0, 0, 0), text, &image, mp_view->dither_pattern (), mp_view->line_styles (), w, h);
          //  create vertex 
          single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0, 0, 0, lay::ViewOp::Cross, iter->marked (true) ? 9/*mark size*/ : 0), vertex, &image, mp_view->dither_pattern (),mp_view->line_styles (),  w, h);

          QPixmap pixmap = QPixmap::fromImage (image); // Qt 4.6.0 workaround
          return QVariant (QIcon (pixmap));

        } else {
          return QVariant (QIcon ());
        }

      }

    } else if (role == Qt::BackgroundRole) {

      if (m_selected_ids.find (size_t (index.internalPointer ())) != m_selected_ids.end ()) {
        //  for selected items pick a color between Highlight and Base
        QPalette pl (mp_view->palette ());
        QColor c1 = pl.color (QPalette::Highlight);
        QColor cb = pl.color (QPalette::Base);
        return QVariant (QColor ((c1.red () + cb.red ()) / 2, (c1.green () + cb.green ()) / 2, (c1.blue () + cb.blue ()) / 2));
      } else {
        return QVariant ();
      }

    } else if (role == Qt::TextColorRole || role == Qt::FontRole) {

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
