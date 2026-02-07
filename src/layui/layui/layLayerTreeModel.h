
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

#ifndef HDR_layLayerTreeModel
#define HDR_layLayerTreeModel

#include "dbBox.h"
#include "layuiCommon.h"
#include "layDitherPattern.h"

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

class LayoutViewBase;
class LayerPropertiesConstIterator;
class LayerPropertiesIterator;

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

class LAYUI_PUBLIC LayerTreeModel
  : public QAbstractItemModel
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   *
   *  The LayoutView reference is required to obtain hidden cell state and current state flags.
   */
  LayerTreeModel (QWidget *parent, lay::LayoutViewBase *view);

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
   *  @brief Sets the expanded state for a given model index
   */
  void set_expanded (const QModelIndex &index, bool ex);

  /**
   *  @brief Gets the expanded state for a given model index
   */
  bool expanded (const QModelIndex &index) const;

  /**
   *  @brief Provides an icon for a given layer style
   */
  static QIcon icon_for_layer (const lay::LayerPropertiesConstIterator &iter, lay::LayoutViewBase *view, unsigned int w, unsigned int h, double dpr, unsigned int di_offset, bool no_state = false);

  /**
   *  @brief Gets the preferred icon size
   */
  QSize icon_size () const;

  /**
   *  @brief Convert a lay::LayerPropertiesConstIterator to a QModelIndex
   */
  QModelIndex index (lay::LayerPropertiesConstIterator iter, int column) const;

  /**
   *  @brief Converts a QModelIndex to an iterator
   */
  lay::LayerPropertiesConstIterator iterator (const QModelIndex &index) const;

  /**
   *  @brief Converts a QModelIndex to an iterator (non-const)
   */
  lay::LayerPropertiesIterator iterator_nc (const QModelIndex &index);

  /**
   *  @brief Gets a flag indicating that an entry is hidden
   */
  bool is_hidden (const QModelIndex &index) const;

  /**
   *  @brief Sets the animation phase
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
   *  @brief Set the font to use for text display (without emitting a signal)
   */
  void set_font_no_signal (const QFont &font);

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
   *  @brief Sets a flag indicating whether to test shapes in view for highlighting non-empty layers
   */
  void set_test_shapes_in_view (bool f);

  /**
   *  @brief Gets a flag indicating whether to test shapes in view for highlighting non-empty layers
   */
  bool get_test_shapes_in_view ()
  {
    return m_test_shapes_in_view;
  }

  /**
   *  @brief Sets the flag indicating whether to hide empty layers
   */
  void set_hide_empty_layers (bool f);

  /**
   *  @brief Gets the flag indicating whether to hide empty layers
   */
  bool get_hide_empty_layers () const
  {
    return m_hide_empty_layers;
  }

  /**
   *  @brief Sets a flag indicating whether selected indexes are filtered or highlighted
   */
  void set_filter_mode (bool f);

  /**
   *  @brief Gets a flag indicating whether selected indexes are filtered or highlighted
   */
  bool get_filter_mode () const
  {
    return m_filter_mode;
  }

  /**
   *  @brief emit a layoutAboutToBeChanged signal
   */
  void signal_begin_layer_changed ();

  /**
   *  @brief emit a layoutChanged signal
   */
  void signal_layers_changed ();

signals:
  /**
   *  @brief This signal is emitted to indicate the hidden flags need update by the client
   *  Note this is neither done by the view nor the model. It needs to be implemented elsewhere.
   */
  void hidden_flags_need_update ();

private: 
  QWidget *mp_parent;
  lay::LayoutViewBase *mp_view;
  bool m_filter_mode;
  size_t m_id_start, m_id_end;
  unsigned int m_phase;
  bool m_test_shapes_in_view;
  bool m_hide_empty_layers;
  QFont m_font;
  QColor m_text_color, m_background_color;
  mutable EmptyWithinViewCache m_test_shapes_cache;
  std::set <size_t> m_selected_ids;
  std::vector <QModelIndex> m_selected_indexes;
  std::vector <QModelIndex>::const_iterator m_current_index;

  /**
   *  @brief Get a flag indicating that a layer is empty
   */
  bool empty_predicate (const QModelIndex &index) const;

  /**
   *  @brief Get a flag indicating that a layer does not have shapes within the shown area
   */
  bool empty_within_view_predicate (const QModelIndex &index) const;

  void search_children (const tl::GlobPattern &pattern, const QModelIndex &parent, bool recurse);
};

}


#endif

#endif  //  defined(HAVE_QT)
