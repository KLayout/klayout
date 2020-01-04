
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


#ifndef HDR_layLayerTreeModel
#define HDR_layLayerTreeModel

#include "dbBox.h"

#include <vector>
#include <set>

#include <QAbstractItemModel>
#include <QFont>
#include <QColor>

namespace db
{
  class Layout;
}

namespace tl
{
  class GlobPattern;
}

namespace lay
{

class LayoutView;
class LayerPropertiesConstIterator;

/**
 *  @brief A helper class implementing a cache for the "test shapes in view" feature
 */

class EmptyWithinViewCache
{
public:
  EmptyWithinViewCache();

  void clear();
  bool is_empty_within_view(const db::Layout *layout, unsigned int cell_index, const db::Box &box, unsigned int layer);

private:
  typedef std::pair<std::pair<const db::Layout *, unsigned int>, db::Box> cache_key_t;
  typedef std::map<cache_key_t, std::set<unsigned int> > cache_t;
  cache_t m_cache;
  std::vector<bool> m_cells_done;

  void determine_empty_layers(const db::Layout *layout, unsigned int cell_index, const db::Box &box, std::vector<unsigned int> &layers);
};

/**
 *  @brief The layer tree model
 *
 *  This model delivers data of the cell tree forming either a flat
 *  representation or a hierarchical one.
 */

class LayerTreeModel 
  : public QAbstractItemModel
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   *
   *  The LayoutView reference is required to obtain hidden cell state and current state flags.
   */
  LayerTreeModel (QWidget *parent, lay::LayoutView *view);

  /**
   *  @brief Dtor
   */
  ~LayerTreeModel ();

  //  Implementation of the QAbstractItemModel interface 
  virtual Qt::ItemFlags flags (const QModelIndex &index) const;
  virtual int columnCount (const QModelIndex &) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual QVariant headerData (int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const;
  virtual int rowCount (const QModelIndex &parent) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &index) const;

  /**
   *  @brief Convert a lay::LayerPropertiesConstIterator to a QModelIndex
   */
  QModelIndex index (lay::LayerPropertiesConstIterator iter, int column) const;

  /**
   *  @brief Convert a QModelIndex to an iterator
   */
  lay::LayerPropertiesConstIterator iterator (const QModelIndex &index) const;

  /**
   *  @brief Get a flag indicating that a layer is empty 
   */
  bool empty_predicate (const QModelIndex &index) const;

  /**
   *  @brief Get a flag indicating that a layer does not have shapes within the shown area
   */
  bool empty_within_view_predicate (const QModelIndex &index) const;

  /**
   *  @brief Set the non-empty layers (the "uint" for the layer iterators) for the "test shapes is view" mode
   *
   *  @return True, if a change has been made.
   */
  bool set_non_empty_layers (const std::set <size_t> &non_empty_layers);

  /**
   *  @brief Set the animation phase
   */
  void set_phase (unsigned int ph);

  /**
   *  @brief Obtain the upperLeft index
   */
  QModelIndex upperLeft () const;

  /**
   *  @brief Obtain the lowerRight index
   */
  QModelIndex bottomRight () const;

  /**
   *  @brief Set the font to use for text display
   */
  void set_font (const QFont &font);

  /**
   *  @brief Set the text color to use for text display
   */
  void set_text_color (QColor color);

  /**
   *  @brief Set the background color to use for text display
   */
  void set_background_color (QColor background);

  /**
   *  @brief emit a dataChanged signal
   */
  void signal_data_changed ();

  /**
   *  @brief Locate an index by name (at least closest)
   *
   *  If top_only is set, only top-level items are searched. An invalid model index is returned if
   *  no corresponding item could be found.
   */
  QModelIndex locate (const char *name, bool glob_pattern = false, bool case_sensitive = true, bool top_only = true);

  /**
   *  @brief Locate the next index (after the first locate)
   */
  QModelIndex locate_next ();

  /**
   *  @brief Locate the previous index (after the first locate)
   */
  QModelIndex locate_prev ();

  /**
   *  @brief Clears the locate flags
   */
  void clear_locate ();

  /**
   *  @brief Set the test_shapes_in_view flag
   *
   *  This method does not issue a data changed signal. This has to be done somewhere else.
   */
  void set_test_shapes_in_view (bool f)
  {
    m_test_shapes_in_view = f;
  }

  /**
   *  @brief emit a layoutAboutToBeChanged signal
   */
  void signal_begin_layer_changed ();

  /**
   *  @brief emit a layoutChanged signal
   */
  void signal_layer_changed ();

private: 
  lay::LayoutView *mp_view;
  size_t m_id_start, m_id_end;
  unsigned int m_phase;
  bool m_test_shapes_in_view;
  QFont m_font;
  QColor m_text_color, m_background_color;
  mutable EmptyWithinViewCache m_test_shapes_cache;
  std::set <size_t> m_selected_ids;
  std::vector <QModelIndex> m_selected_indexes;
  std::vector <QModelIndex>::const_iterator m_current_index;

  void search_children (const tl::GlobPattern &pattern, const QModelIndex &parent, bool recurse);
};

}


#endif

