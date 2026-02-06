
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

#ifndef HDR_layCellTreeModel
#define HDR_layCellTreeModel

#include <vector>

#include "dbLayout.h"

#include <QAbstractItemModel>

class QTreeView;

namespace tl
{
  class GlobPattern;
}

namespace db
{
  class Library;
}

namespace lay
{

class LayoutViewBase;
class CellTreeItem;

/**
 *  @brief The cell tree model
 *
 *  This model delivers data of the cell tree forming either a flat
 *  representation or a hierarchical one.
 */

class CellTreeModel 
  : public QAbstractItemModel,
    public tl::Object
{
public:
  enum Flags {
    Flat = 1,           //  flat list (rather than hierarchy)
    Children = 2,       //  direct children of cell "base"
    Parents = 4,        //  direct parents of cell "base"
    TopCells = 8,       //  show top cells only
    BasicCells = 16,    //  show basic cells (PCells included, no proxies)
    WithVariants = 32,  //  show PCell variants below PCells
    WithIcons = 64,     //  show icons for the top level cell type
    NoPadding = 128,    //  disable padding of display string with a blank at the beginning and end
    HidePrivate = 256   //  hide cells whose name starts with an underscore
  };

  enum Sorting {
    ByName,             //  sort by name
    ByArea,             //  sort by cell area (small to large)
    ByAreaReverse       //  sort by cell area (large to small)
  };

  /**
   *  @brief Constructor
   *
   *  The LayoutView reference is required to obtain hidden cell state and current state flags.
   *  The flags member is a combination of the flags given by 
   *  If flags "Children" or "Parents" are given, "base" must be set to the cell of which
   *  the children or parents should be derived.
   */
  CellTreeModel (QWidget *parent, lay::LayoutViewBase *view, int cv_index, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Constructor
   *
   *  This constructor does not take a view but rather a layout. It does not display hidden status or similar.
   */
  CellTreeModel (QWidget *parent, db::Layout *layout, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Constructor
   *
   *  This constructor does not take a view but rather a layout from a library. It does not display hidden status or similar.
   */
  CellTreeModel (QWidget *parent, db::Library *library, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Dtor
   */
  ~CellTreeModel ();

  //  Implementation of the QAbstractItemModel interface 
  virtual Qt::ItemFlags flags (const QModelIndex &index) const;
  virtual int columnCount (const QModelIndex &) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual QVariant headerData (int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const;
  virtual int rowCount (const QModelIndex &parent) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &index) const;
  virtual QStringList mimeTypes () const;
  virtual QMimeData *mimeData (const QModelIndexList &indexes) const;

  /**
   *  @brief Reconfigures the model with a LayoutView
   */
  void configure (LayoutViewBase *view, int cv_index, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Reconfigures the model with a pure Layout
   */
  void configure (db::Layout *layout, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Reconfigures the model with a pure Layout from a library
   */
  void configure (db::Library *library, unsigned int flags = 0, const db::Cell *base = 0, Sorting sorting = ByName);

  /**
   *  @brief Gets the layout this model is connected to
   */
  const db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Return the number of top level items
   */
  int toplevel_items () const;

  /**
   *  @brief Return the top level item
   */
  CellTreeItem *toplevel_item (int index);

  /**
   *  @brief Transform a CellTreeItem * to a QModelIndex
   */
  QModelIndex model_index (CellTreeItem *item) const;

  /**
   *  @brief Returns true, if the given item is a PCell entry
   */
  bool is_pcell (const QModelIndex &index) const;

  /**
   *  @brief Returns the PCell id if the given item is PCell entry
   */
  db::pcell_id_type pcell_id (const QModelIndex &index) const;

  /**
   *  @brief Convert a QModelIndex to a cell_index
   */
  db::cell_index_type cell_index (const QModelIndex &index) const; 

  /**
   *  @brief Convert a QModelIndex to a db::Cell pointer
   *
   *  This method returns 0 if the model index is not valid.
   */
  const db::Cell *cell (const QModelIndex &index) const; 

  /**
   *  @brief Convert a QModelIndex to a cell name
   *
   *  This method returns 0 if the model index is not valid.
   */
  const char *cell_name (const QModelIndex &index) const; 

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
   *  @brief Resets the search pointer to the one next to the given index
   */
  QModelIndex locate_next (const QModelIndex &index);

  /**
   *  @brief Clears the locate flags
   */
  void clear_locate ();

  /**
   *  @brief Set the sorting 
   */
  void set_sorting (Sorting s);

  /**
   *  @brief Get the sorting
   */
  Sorting sorting () const
  {
    return m_sorting;
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
   *  @brief Signal to the owner of the model that the data has changed
   */
  void signal_data_changed ();

  /**
   *  @brief Signal to the owner of the model that the data has changed (with an int parameter)
   */
  void signal_data_changed_with_int (int)
  {
    signal_data_changed ();
  }

private:
  bool m_flat, m_pad;
  bool m_filter_mode, m_is_filtered;
  unsigned int m_flags;
  Sorting m_sorting;
  QWidget *mp_parent;
  lay::LayoutViewBase *mp_view;
  db::Layout *mp_layout;
  db::Library *mp_library;
  int m_cv_index;
  const db::Cell *mp_base;
  std::vector <CellTreeItem *> m_toplevel;
  std::set <void *> m_selected_indexes_set;
  std::set<const CellTreeItem *> m_visible_cell_set;
  std::vector <QModelIndex> m_selected_indexes;
  std::vector <QModelIndex>::const_iterator m_current_index;

  void build_top_level ();
  void clear_top_level ();
  bool search_children (const tl::GlobPattern &pattern, CellTreeItem *item);
  void do_configure (db::Layout *layout, db::Library *library, LayoutViewBase *view, int cv_index, unsigned int flags, const db::Cell *base, Sorting sorting);
  bool name_selected (const std::string &name) const;
};

/**
 *  @brief The cell tree item object 
 *
 *  This object is used to represent a cell in the tree model.
 */

class CellTreeItem
{
public:
  CellTreeItem (const db::Layout *layout, bool is_pcell, unsigned int cell_or_pcell_index, bool flat, CellTreeModel::Sorting sorting);
  ~CellTreeItem ();

  int children () const;
  int children_in (const std::set<const CellTreeItem *> &sel) const;
  CellTreeItem *child (int index);
  CellTreeItem *child_in (const std::set<const CellTreeItem *> &sel, int index);
  db::cell_index_type cell_or_pcell_index () const;
  CellTreeItem *parent () const;
  bool by_name_less_than (const CellTreeItem *b) const;
  bool by_area_less_than (const CellTreeItem *b) const;
  bool by_area_equal_than (const CellTreeItem *b) const;
  bool name_less_than (const char *name) const;
  bool name_equals (const char *name) const;
  bool name_matches (const tl::GlobPattern &p) const;
  std::string display_text () const;
  void add_child (CellTreeItem *item);
  void finish_children ();
  bool is_valid () const;

  bool is_pcell () const
  {
    return m_is_pcell;
  }

  size_t index () const
  {
    return m_index;
  }

  void set_index (size_t index) 
  {
    m_index = index;
  }

  size_t tree_index () const
  {
    return m_tree_index;
  }

  void set_tree_index (size_t index)
  {
    m_tree_index = index;
  }

  size_t assign_serial (size_t index, std::map<CellTreeItem *, size_t> &serial);

private:
  const db::Layout *mp_layout;
  CellTreeItem *mp_parent;
  CellTreeModel::Sorting m_sorting;
  bool m_is_pcell;
  size_t m_index, m_tree_index;
  std::vector<CellTreeItem *> m_children;
  int m_child_count;
  unsigned int m_cell_or_pcell_index;

  const char *name () const;
  void ensure_children ();
};

}


#endif

#endif  //  defined(HAVE_QT)
