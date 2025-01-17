
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

#include "rdbMarkerBrowserPage.h"
#include "rdb.h"

#include "dbLayoutUtils.h"

#include "tlRecipe.h"
#include "layLayoutViewBase.h"
#include "layMarker.h"
#include "tlExceptions.h"

#include "ui_MarkerBrowserSnapshotView.h"

#include <QAbstractItemModel>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QInputDialog>

namespace rdb
{

extern std::string cfg_rdb_show_all;
extern std::string cfg_rdb_list_shapes;

struct FlagDescriptor 
{
  FlagDescriptor (const std::string &i, const std::string &t, const std::string &x)
    : icon (i), text (t), tag (x)
  { }

  std::string icon, text, tag;
};

static FlagDescriptor flag_descriptors[] = 
{
  FlagDescriptor (":no_flag_16px.png", tl::to_string (QObject::tr ("No flag")), ""),
  FlagDescriptor (":red_flag_16px.png", tl::to_string (QObject::tr ("Red flag")), "red"),
  FlagDescriptor (":green_flag_16px.png", tl::to_string (QObject::tr ("Green flag")), "green"),
  FlagDescriptor (":blue_flag_16px.png", tl::to_string (QObject::tr ("Blue flag")), "blue"),
  FlagDescriptor (":yellow_flag_16px.png", tl::to_string (QObject::tr ("Yellow flag")), "yellow")
};

// ----------------------------------------------------------------------------------
//  MarkerBrowserTreeViewModel definition and implementation

class MarkerBrowserTreeViewModelCacheEntry;

struct SortByKeyCompareFunc
{
  SortByKeyCompareFunc (bool ascending, const rdb::Database *rdb)
    : mp_rdb (rdb), m_ascending (ascending)
  {
    // .. nothing yet ..
  }

  bool operator() (MarkerBrowserTreeViewModelCacheEntry *a, MarkerBrowserTreeViewModelCacheEntry *b);

private:
  const rdb::Database *mp_rdb;
  bool m_ascending;
};

struct SortByCountCompareFunc
{
  SortByCountCompareFunc (bool ascending, const rdb::Database *rdb)
    : mp_rdb (rdb), m_ascending (ascending)
  {
    // .. nothing yet ..
  }

  bool operator() (MarkerBrowserTreeViewModelCacheEntry *a, MarkerBrowserTreeViewModelCacheEntry *b);

private:
  const rdb::Database *mp_rdb;
  bool m_ascending;
};

class MarkerBrowserTreeViewModelCacheEntry
{
public:
  MarkerBrowserTreeViewModelCacheEntry ()
    : mp_parent (0), m_id (0), m_row (0), m_count (0), m_waived_count (0)
  {
    // .. nothing yet ..
  }

  MarkerBrowserTreeViewModelCacheEntry (rdb::id_type id, unsigned int branch)
    : mp_parent (0), m_id ((id << 3) + (branch << 1)), m_row (0), m_count (0), m_waived_count (0)
  {
    // .. nothing yet ..
  }

  ~MarkerBrowserTreeViewModelCacheEntry()
  {
    clear ();
  }

  void clear()
  {
    for (std::vector<MarkerBrowserTreeViewModelCacheEntry *>::iterator i = m_ids.begin (); i != m_ids.end (); ++i) {
      delete *i;
    }
    m_ids.clear ();
    m_id = 0;
  }

  void add_child (MarkerBrowserTreeViewModelCacheEntry *child)
  {
    child->m_row = (unsigned int) m_ids.size ();
    m_ids.push_back(child);
    child->mp_parent = this; 
  }

  MarkerBrowserTreeViewModelCacheEntry *parent() const
  {
    return mp_parent;
  }

  void set_cache_valid (bool c)
  {
    m_id = (m_id & ~1l) + c;
  }

  bool cache_valid () const
  {
    return (m_id & 1) != 0;
  }

  void set_branch (unsigned int b)
  {
    m_id = (m_id & ~6l) + b;
  }

  unsigned int branch () const
  {
    return (m_id & 6l) >> 1;
  }

  void set_id (rdb::id_type id)
  {
    m_id = (id << 3) + (m_id & 7l);
  }

  rdb::id_type id () const
  {
    return (m_id >> 3);
  }

  MarkerBrowserTreeViewModelCacheEntry *child (int n) const
  {
    if (n < 0 || n >= int (m_ids.size ())) {
      return 0;
    } else {
      return m_ids [n];
    }
  }

  size_t children () const
  {
    return m_ids.size ();
  }

  unsigned int row () const
  {
    return m_row;
  }

  size_t visited_count (const Database *db) const
  {
    const rdb::Cell *cell = db->cell_by_id (id ());
    const rdb::Category *category = db->category_by_id (id ());

    if (cell) {

      const MarkerBrowserTreeViewModelCacheEntry *node = this;
      while (node && ! category) {
        category = db->category_by_id (node->id ());
        node = node->parent ();
      }

    } else if (category) {

      const MarkerBrowserTreeViewModelCacheEntry *node = this;
      while (node && ! cell) {
        cell = db->cell_by_id (node->id ());
        node = node->parent ();
      }

    }

    if (cell == 0 && category == 0) {
      return db->num_items_visited ();
    } else if (category == 0) {
      return cell->num_items_visited ();
    } else if (cell == 0) {
      return category->num_items_visited ();
    } else {
      return db->num_items_visited (cell->id (), category->id ());
    } 
  }

  size_t count () const
  {
    return m_count;
  }

  void set_count (size_t c)
  {
    m_count = c;
  }

  size_t waived_count () const
  {
    return m_waived_count;
  }

  void set_waived_count (size_t c)
  {
    m_waived_count = c;
  }

  void waive_or_unwaive (bool w)
  {
    if (w) {
      ++m_waived_count;
    } else {
      --m_waived_count;
    }
  }

  void sort_by_key_name (bool ascending, const rdb::Database *database)
  {
    std::sort (m_ids.begin (), m_ids.end (), SortByKeyCompareFunc (ascending, database));
    unsigned int r = 0;
    for (std::vector<MarkerBrowserTreeViewModelCacheEntry *>::iterator c = m_ids.begin (); c != m_ids.end (); ++c) {
      (*c)->m_row = r++;
      (*c)->sort_by_key_name (ascending, database);
    }
  }

  void sort_by_count (bool ascending, const rdb::Database *database)
  {
    std::sort (m_ids.begin (), m_ids.end (), SortByCountCompareFunc (ascending, database));
    unsigned int r = 0;
    for (std::vector<MarkerBrowserTreeViewModelCacheEntry *>::iterator c = m_ids.begin (); c != m_ids.end (); ++c) {
      (*c)->m_row = r++;
      (*c)->sort_by_count (ascending, database);
    }
  }

private:
  MarkerBrowserTreeViewModelCacheEntry *mp_parent;
  rdb::id_type m_id;
  unsigned int m_row;
  size_t m_count, m_waived_count;
  std::vector<MarkerBrowserTreeViewModelCacheEntry *> m_ids;
};

bool 
SortByKeyCompareFunc::operator() (MarkerBrowserTreeViewModelCacheEntry *a, MarkerBrowserTreeViewModelCacheEntry *b)
{
  const rdb::Cell *ca = mp_rdb->cell_by_id (a->id ());
  const rdb::Cell *cb = mp_rdb->cell_by_id (b->id ());
  if (ca && cb) {
    return m_ascending ? ca->qname () < cb->qname () : cb->qname () < ca->qname ();
  }

  const rdb::Category *xa = mp_rdb->category_by_id (a->id ());
  const rdb::Category *xb = mp_rdb->category_by_id (b->id ());
  if (xa && xb) {
    return m_ascending ? xa->name () < xb->name () : xb->name () < xa->name ();
  }

  return a->id () < b->id ();
}

bool 
SortByCountCompareFunc::operator() (MarkerBrowserTreeViewModelCacheEntry *a, MarkerBrowserTreeViewModelCacheEntry *b)
{
  //  Compare only cell vs. cell and category vs. category.
  //  This keeps the top level sorted by id.

  const rdb::Cell *ca = mp_rdb->cell_by_id (a->id ());
  const rdb::Cell *cb = mp_rdb->cell_by_id (b->id ());
  if (ca && cb) {
    return m_ascending ? a->count () < b->count () : b->count () < a->count ();
  }

  const rdb::Category *xa = mp_rdb->category_by_id (a->id ());
  const rdb::Category *xb = mp_rdb->category_by_id (b->id ());
  if (xa && xb) {
    return m_ascending ? a->count () < b->count () : b->count () < a->count ();
  }

  return a->id () < b->id ();
}

/**
 *  @brief Returns true if the given cell matches the given filter
 */
static bool cell_matches_filter (const rdb::Cell *cell, const QString &filter)
{
  return tl::to_qstring (cell->name ()).indexOf (filter, 0, Qt::CaseInsensitive) >= 0;
}

/**
 *  @brief Returns true if the given category or one of the sub-categories matches the given filter
 */
static bool cat_matches_filter (const rdb::Category *cat, const QString &filter, bool recursive) 
{
  if (tl::to_qstring (cat->name ()).indexOf (filter, 0, Qt::CaseInsensitive) >= 0) {
    return true;
  }

  if (recursive) {
    for (rdb::Categories::const_iterator c = cat->sub_categories ().begin (); c != cat->sub_categories ().end (); ++c) {
      if (cat_matches_filter (c.operator-> (), filter, true)) {
        return true;
      }
    }
  }

  return false;
}

class MarkerBrowserTreeViewModel
  : public QAbstractItemModel
{
public:
  enum Sorting {
    ByKeyName,
    ByCount
  };

  MarkerBrowserTreeViewModel ()
    : mp_database (0), m_show_empty_ones (true), m_waived_tag_id (0)
  {
    //  .. nothing yet ..
  }

  void set_database (const rdb::Database *db)
  {
    mp_database = db;
    m_waived_tag_id = mp_database ? mp_database->tags ().tag ("waived").id () : 0;

    invalidate ();
  }

  void set_show_empty_ones (bool show)
  {
    if (m_show_empty_ones != show) {
      m_show_empty_ones = show;
      invalidate ();
    }
  }

  void sort_by (Sorting sorting, bool ascending)
  {
    if (mp_database) {

      QModelIndexList pi = persistentIndexList ();
      std::vector<std::pair <MarkerBrowserTreeViewModelCacheEntry *, int> > ids;
      ids.reserve (pi.size ());
      for (QModelIndexList::const_iterator i = pi.begin(); i != pi.end (); ++i) {
        ids.push_back (std::make_pair ((MarkerBrowserTreeViewModelCacheEntry *) i->internalPointer (), i->column ()));
      }

      if (sorting == ByKeyName) {
        m_cache.sort_by_key_name (ascending, mp_database);
      } else if (sorting == ByCount) {
        m_cache.sort_by_count (ascending, mp_database);
      }

      QModelIndexList new_pi;
      for (std::vector<std::pair <MarkerBrowserTreeViewModelCacheEntry *, int> >::const_iterator i = ids.begin (); i != ids.end (); ++i) {
        new_pi.push_back (createNodeIndex (i->first, i->second));
      }

      changePersistentIndexList (pi, new_pi);

    }
  }

  void waived_changed (const rdb::Item *item, bool waived)
  {
    const rdb::Category *cat = mp_database->category_by_id (item->category_id ());
    while (cat) {
      waive_or_unwaive (0, cat->id (), waived);
      if (item->cell_id () != 0) {
        waive_or_unwaive (item->cell_id (), cat->id (), waived);
      }
      cat = cat->parent ();
    }

    waive_or_unwaive (0, 0, waived);
    if (item->cell_id () != 0) {
      waive_or_unwaive (item->cell_id (), 0, waived);
    }
  }

  int columnCount (const QModelIndex & /*parent*/) const
  {
    return 2;
  }

  QVariant headerData (int section, Qt::Orientation /*orientation*/, int role) const
  {
    if (role == Qt::DisplayRole) {
      if (section == 0) {
        return QVariant (QObject::tr ("Cell / Category"));
      } else if (section == 1) {
        return QVariant (QObject::tr ("Count (Not Visited) - Waived"));
      }
    }

    return QVariant ();
  }

  bool cat_matches (const QModelIndex &index, const QString &filter) const
  {
    MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());
    if (node && mp_database) {

      rdb::id_type id = node->id ();
      const rdb::Category *category = mp_database->category_by_id (id);
      if (category) {
        return cat_matches_filter (category, filter, true /*recursively*/);
      }

    } 

    //  does not apply - return true
    return true;
  }

  bool cell_matches (const QModelIndex &index, const QString &filter) const
  {
    MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());
    if (node && mp_database) {

      rdb::id_type id = node->id ();
      const rdb::Cell *cell = mp_database->cell_by_id (id);
      if (cell) {
        return cell_matches_filter (cell, filter);
      }

    } 

    //  does not apply - return true
    return true;
  }

  bool no_errors (const QModelIndex &index, bool include_waived = false) const
  {
    MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());
    if (node && mp_database) {

      rdb::id_type id = node->id ();
      bool none = false;
      size_t thr = include_waived ? node->waived_count () : 0;

      const rdb::Cell *cell = mp_database->cell_by_id (id);
      const rdb::Category *category = mp_database->category_by_id (id);

      if (cell) {

        while (node && ! category) {
          category = mp_database->category_by_id (node->id ());
          node = node->parent ();
        }

      } else if (category) {

        while (node && ! cell) {
          cell = mp_database->cell_by_id (node->id ());
          node = node->parent ();
        }

      }

      if (cell == 0 && category == 0) {
        none = (mp_database->num_items () <= thr);
      } else if (category == 0) {
        none = (cell->num_items () <= thr);
      } else if (cell == 0) {
        none = (category->num_items () <= thr);
      } else {
        none = (mp_database->num_items (cell->id (), category->id ()) <= thr);
      } 

      return none;

    } else {
      return false;
    }
  }

  QVariant data (const QModelIndex &index, int role) const
  {
    if (!mp_database || !index.isValid ()) {
      return QVariant ();
    }

    if (role == Qt::DisplayRole) {

      MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());

      if (node) {

        if (index.column () == 1) {

          std::string s;

          if (node->count () > 0) {

            size_t visited = node->visited_count (mp_database);
            size_t waived = node->waived_count ();

            if (visited < node->count ()) {
              s = tl::sprintf (tl::to_string (tr ("%lu (%lu)")), node->count (), node->count () - visited);
            } else {
              s = tl::sprintf (tl::to_string (tr ("%lu")), node->count ());
            }

            if (waived > 0) {
              if (waived == node->count ()) {
                s += tl::to_string (tr (" - all waived"));
              } else {
                s += tl::sprintf (tl::to_string (tr (" - %lu")), waived);
              }
            }

          }

          return QVariant (tl::to_qstring (s));

        } else if (index.column () == 0) {

          rdb::id_type id = node->id ();
          unsigned int b = node->branch ();

          //  On the first level, the id is 0, 1 or 2 reflecting the three top nodes
          if (id == 0) {

            if (b == 0) {
              return QVariant (QObject::tr ("By Cell"));
            } else if (b == 1) {
              return QVariant (QObject::tr ("By Category"));
            } else if (b == 2) {
              return QVariant (QObject::tr ("All"));
            }

          } else {

            const rdb::Cell *cell = mp_database->cell_by_id (id);
            if (cell) {
              //  put cells in square brackets
              if (cell->name ().empty ()) {
                return QObject::tr ("All Cells");
              } else {
                return QVariant (QString::fromUtf8 ("[") + tl::to_qstring (cell->qname ()) + QString::fromUtf8 ("]"));
              }
            }

            const rdb::Category *category = mp_database->category_by_id (id);
            if (category) {
              return QVariant (tl::to_qstring (category->name ()));
            }

          }

        }

      }

    } else if (role == Qt::FontRole) {

      MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());
      if (node) {

        //  Bold font for cells where not all items are visited
        if (node->visited_count (mp_database) != node->count ()) {
          QFont font;
          font.setBold (true);
          return QVariant (font);
        }

      }

    } else if (role == Qt::ForegroundRole) {

      MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *)(index.internalPointer ());
      if (node && node->id () == 0) {
        // blue color for the top level nodes
        return QVariant (QColor (0, 0, 255));
      }

      //  Green color if no errors are present
      if (no_errors (index, true)) {
        return QVariant (QColor (0, 192, 0));
      }

    }

    return QVariant ();
  }

  bool hasChildren (const QModelIndex &parent) const
  {
    return rowCount (parent) != 0;
  }

  void mark_data_changed ()
  {
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, columnCount (QModelIndex ()) - 1, QModelIndex ()));
  }

  QModelIndex index (int row, int column, const QModelIndex &parent) const
  {
    if (! mp_database) {

      return QModelIndex ();

    } else if (! parent.isValid ()) {

      return createNodeIndex (m_cache.child (row), column);

    } else {

      MarkerBrowserTreeViewModelCacheEntry *parent_node = (MarkerBrowserTreeViewModelCacheEntry *) parent.internalPointer ();
      if (parent_node) {
        update_cache (parent_node);
        return createNodeIndex(parent_node->child (row), column);
      } else {
        return QModelIndex ();
      }

    }
  }

  QModelIndex parent (const QModelIndex &index) const
  {
    MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *) index.internalPointer ();
    if (node && node->parent () && node->parent () != &m_cache) {
      return createIndex (node->parent ()->row (), 0, (void *) node->parent ());
    } else {
      return QModelIndex ();
    }
  }

  int rowCount (const QModelIndex &index) const
  {
    if (! mp_database) {
      return 0;
    }

    MarkerBrowserTreeViewModelCacheEntry *node;
    if (! index.isValid ()) {
      node = &m_cache;
    } else {
      node = (MarkerBrowserTreeViewModelCacheEntry *) index.internalPointer ();
    }

    if (node) {
      update_cache (node);
      return int (node->children ());
    } else {
      return 0;
    }
  }

  QModelIndex next_index (QModelIndex current_index, bool up)
  {
    if (!mp_database) {
      return QModelIndex ();
    }

    MarkerBrowserTreeViewModelCacheEntry *node = (MarkerBrowserTreeViewModelCacheEntry *) current_index.internalPointer ();
    
    rdb::id_type id = node->id ();
    unsigned int b = node->branch ();
    bool descend_into_cell = true;
    bool descend_into_category = true;
    bool must_descend_into_cell = false;
    bool must_descend_into_category = false;

    if (b == 0 /*By Cell*/) {
      if (mp_database->cell_by_id (id) != 0) {
        //  stay on cell level in "By cell" branch
        descend_into_category = false;
      } else {
        must_descend_into_category = true;
      }
    } else if (b == 1 /*By Category*/) {
      if (mp_database->category_by_id (id) != 0) {
        //  stay on category level in "By category" branch
        descend_into_cell = false;
      } else {
        must_descend_into_cell = true;
      }
    } else {
      return QModelIndex ();
    }

    while (current_index.isValid ()) {

      //  next sibling
      QModelIndex parent_index = parent (current_index);
      if (! parent_index.isValid ()) {
        //  don't advance on the topmost level
        break;
      }
      
      current_index = index (current_index.row () + (up ? -1 : 1), current_index.column (), parent_index);

      if (current_index.isValid ()) {

        //  try to descend as far as possible
        while (current_index.isValid ()) {

          node = (MarkerBrowserTreeViewModelCacheEntry *) current_index.internalPointer ();
          rdb::id_type id = node->id ();
          if (mp_database->cell_by_id (id) != 0 && !descend_into_cell) {
            break;
          } else if (mp_database->category_by_id (id) != 0 && !descend_into_category) {
            break;
          }

          parent_index = current_index;
          current_index = index (up ? (rowCount (current_index) - 1) : 0, current_index.column (), current_index);

        }

        //  use the last valid one if it fits.

        node = (MarkerBrowserTreeViewModelCacheEntry *) parent_index.internalPointer ();
        rdb::id_type id = node->id ();
        if (mp_database->cell_by_id (id) != 0 && !must_descend_into_category) {
          return parent_index;
        } else if (mp_database->category_by_id (id) != 0 && !must_descend_into_cell) {
          return parent_index;
        }

      }

      //  one level up
      current_index = parent_index;

    }

    return QModelIndex ();
  }

private:
  const rdb::Database *mp_database;
  mutable MarkerBrowserTreeViewModelCacheEntry m_cache;
  mutable std::multimap<std::pair<rdb::id_type, rdb::id_type>, MarkerBrowserTreeViewModelCacheEntry *> m_cache_by_ids;
  bool m_show_empty_ones;
  id_type m_waived_tag_id;

  void waive_or_unwaive (rdb::id_type cell_id, rdb::id_type cat_id, bool waived)
  {
    auto k = std::make_pair (cell_id, cat_id);
    auto c = m_cache_by_ids.find (k);
    while (c != m_cache_by_ids.end () && c->first == k) {
      c->second->waive_or_unwaive (waived);
      ++c;
    }
  }

  size_t num_waived () const
  {
    size_t n = 0;
    for (auto i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
      if (i->has_tag (m_waived_tag_id)) {
        ++n;
      }
    }
    return n;
  }

  size_t num_waived_per_cat (id_type cat_id) const
  {
    size_t n = 0;

    auto ii = mp_database->items_by_category (cat_id);
    for (auto i = ii.first; i != ii.second; ++i) {
      if ((*i)->has_tag (m_waived_tag_id)) {
        ++n;
      }
    }

    //  include sub-categories
    const rdb::Category *cat = mp_database->category_by_id (cat_id);
    tl_assert (cat != 0);
    for (auto c = cat->sub_categories ().begin (); c != cat->sub_categories ().end (); ++c) {
      n += num_waived_per_cat (c->id ());
    }

    return n;
  }

  size_t num_waived_per_cell_and_cat (id_type cell_id, id_type cat_id) const
  {
    size_t n = 0;

    auto ii = mp_database->items_by_cell_and_category (cell_id, cat_id);
    for (auto i = ii.first; i != ii.second; ++i) {
      if ((*i)->has_tag (m_waived_tag_id)) {
        ++n;
      }
    }

    //  include sub-categories
    const rdb::Category *cat = mp_database->category_by_id (cat_id);
    tl_assert (cat != 0);
    for (auto c = cat->sub_categories ().begin (); c != cat->sub_categories ().end (); ++c) {
      n += num_waived_per_cell_and_cat (cell_id, c->id ());
    }

    return n;
  }

  size_t num_waived_per_cell (id_type cell_id) const
  {
    auto ii = mp_database->items_by_cell (cell_id);
    size_t n = 0;
    for (auto i = ii.first; i != ii.second; ++i) {
      if ((*i)->has_tag (m_waived_tag_id)) {
        ++n;
      }
    }
    return n;
  }

  void invalidate ()
  {
    beginResetModel ();

    m_cache.clear ();
    m_cache_by_ids.clear ();
    
    MarkerBrowserTreeViewModelCacheEntry *by_cell_node = new MarkerBrowserTreeViewModelCacheEntry(0, 0);
    m_cache.add_child (by_cell_node);
    m_cache_by_ids.insert (std::make_pair (std::make_pair (rdb::id_type (0), rdb::id_type (0)), by_cell_node));

    MarkerBrowserTreeViewModelCacheEntry *by_category_node = new MarkerBrowserTreeViewModelCacheEntry(0, 1);
    m_cache.add_child (by_category_node);
    m_cache_by_ids.insert (std::make_pair (std::make_pair (rdb::id_type (0), rdb::id_type (0)), by_category_node));

    MarkerBrowserTreeViewModelCacheEntry *all_node = new MarkerBrowserTreeViewModelCacheEntry(0, 2);
    m_cache.add_child (all_node);
    m_cache_by_ids.insert (std::make_pair (std::make_pair (rdb::id_type (0), rdb::id_type (0)), all_node));

    m_cache.set_cache_valid (true);

    endResetModel ();
  }

  QModelIndex createNodeIndex (MarkerBrowserTreeViewModelCacheEntry *node, int column) const
  {
    if (node) {
      return createIndex (node->row (), column, (void *) node);
    } else {
      return QModelIndex ();
    }
  }

  void add_sub_categories (MarkerBrowserTreeViewModelCacheEntry *node) const
  {
    const rdb::Category *category = mp_database->category_by_id (node->id ());
    if (category) {

      for (rdb::Categories::const_iterator c = category->sub_categories ().begin (); c != category->sub_categories ().end (); ++c) {

        node->set_cache_valid (true);

        MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (c->id (), node->branch ());
        m_cache_by_ids.insert (std::make_pair (std::make_pair (rdb::id_type (0), c->id ()), child));
        node->add_child (child);

        child->set_count (mp_database->category_by_id (c->id ())->num_items ());
        child->set_waived_count (num_waived_per_cat (c->id ()));

        add_sub_categories (child);

      }

    }
  }

  void add_sub_categories (id_type cell_id, MarkerBrowserTreeViewModelCacheEntry *node, std::set <rdb::id_type> &partial_tree) const
  {
    node->set_cache_valid (true);

    const rdb::Category *category = mp_database->category_by_id (node->id ());
    if (category) {

      for (rdb::Categories::const_iterator c = category->sub_categories ().begin (); c != category->sub_categories ().end (); ++c) {

        if (partial_tree.find (c->id ()) != partial_tree.end ()) {

          MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (c->id (), node->branch ());
          m_cache_by_ids.insert (std::make_pair (std::make_pair (cell_id, c->id ()), child));
          node->add_child (child);

          child->set_count (mp_database->num_items (cell_id, c->id ()));
          child->set_waived_count (num_waived_per_cell_and_cat (cell_id, c->id ()));

          add_sub_categories (cell_id, child, partial_tree);

        }

      }

    }
  }

  void update_cache (MarkerBrowserTreeViewModelCacheEntry *node) const
  {
    if (node->cache_valid ()) {
      return;
    }

    node->set_cache_valid (true);

    rdb::id_type id = node->id ();
    unsigned int branch = node->branch ();

    if (id == 0) {

      if (branch == 0) {

        for (rdb::Database::const_cell_iterator c = mp_database->cells ().begin (); c != mp_database->cells ().end (); ++c) {

          if (mp_database->cell_by_id (c->id ()) && (m_show_empty_ones || mp_database->cell_by_id (c->id ())->num_items () != 0)) {

            MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (c->id (), branch);
            m_cache_by_ids.insert (std::make_pair (std::make_pair (c->id (), rdb::id_type (0)), child));

            child->set_count (mp_database->cell_by_id (c->id ())->num_items ());
            child->set_waived_count (num_waived_per_cell (c->id ()));

            node->add_child (child);

          }

        }

      } else if (branch == 1) {

        for (rdb::Categories::const_iterator c = mp_database->categories ().begin (); c != mp_database->categories ().end (); ++c) {

          if (mp_database->category_by_id (c->id ()) && (m_show_empty_ones || mp_database->category_by_id (c->id ())->num_items () != 0)) {

            MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (c->id (), branch);
            m_cache_by_ids.insert (std::make_pair (std::make_pair (rdb::id_type (0), c->id ()), child));

            child->set_count (mp_database->category_by_id (c->id ())->num_items ());
            child->set_waived_count (num_waived_per_cat (c->id ()));

            node->add_child (child);
            add_sub_categories (child);

          }

        }

      }

      node->set_count (mp_database->num_items ());
      node->set_waived_count (num_waived ());

    } else if (branch == 0) {

      const rdb::Cell *cell = mp_database->cell_by_id (id);
      if (cell) {

        //  look up all categories used inside this cell and determine top-level categories to insert into the cell node.

        std::set <rdb::id_type> category_ids;
        std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> be = mp_database->items_by_cell (id);
        for (rdb::Database::const_item_ref_iterator c = be.first; c != be.second; ++c) {

          rdb::id_type id = (*c)->category_id ();
          if (category_ids.find (id) == category_ids.end ()) {

            const Category *cat = mp_database->category_by_id (id);
            if (cat) {

              do {
                category_ids.insert (cat->id ());
                cat = cat->parent ();
              } while (cat && category_ids.find (cat->id ()) == category_ids.end ());

            }

          }

        }

        for (rdb::Categories::const_iterator c = mp_database->categories ().begin (); c != mp_database->categories ().end (); ++c) {
          if (category_ids.find (c->id ()) != category_ids.end ()) {

            MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (c->id (), branch);

            size_t n = mp_database->num_items (id, c->id ());

            if (m_show_empty_ones || n != 0) {

              m_cache_by_ids.insert (std::make_pair (std::make_pair (id, c->id ()), child));

              child->set_count (n);
              child->set_waived_count (num_waived_per_cell_and_cat (id, c->id ()));

              node->add_child (child);

              add_sub_categories (id, child, category_ids);

            } else {
              delete child;
            }

          }
        }

      }

    } else if (branch == 1) {

      const rdb::Category *category = mp_database->category_by_id (id);
      if (category) {

        std::set <rdb::id_type> cell_ids;
        std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> be = mp_database->items_by_category (id);
        for (rdb::Database::const_item_ref_iterator c = be.first; c != be.second; ++c) {
          cell_ids.insert ((*c)->cell_id ());
        }

        for (std::set <rdb::id_type>::const_iterator c = cell_ids.begin (); c != cell_ids.end (); ++c) {

          MarkerBrowserTreeViewModelCacheEntry *child = new MarkerBrowserTreeViewModelCacheEntry (*c, branch);

          size_t n = mp_database->num_items (*c, id);

          if (m_show_empty_ones || n != 0) {

            m_cache_by_ids.insert (std::make_pair (std::make_pair (*c, id), child));

            child->set_count (n);
            child->set_waived_count (num_waived_per_cell_and_cat (*c, id));

            node->add_child (child);

          } else {
            delete child;
          }

        }

      }

    }

  }
};

// ----------------------------------------------------------------------------------
//  MarkerBrowserListViewModel definition and implementation

static const rdb::Item &access (const rdb::Item &item) { return item; }
static const rdb::Item &access (const rdb::ItemRef &ref) { return *ref; }

template <class Iter>
struct ValueIterSorter
{
  ValueIterSorter (rdb::id_type tag_id) 
    : m_tag_id (tag_id)
  { 
  }

  bool operator() (Iter a, Iter b) 
  {
    const rdb::Item &ia = access (*a);
    const rdb::Item &ib = access (*b);

    const rdb::ValueBase *va = 0, *vb = 0;

    for (rdb::Values::const_iterator i = ia.values ().begin (); i != ia.values ().end () && !va; ++i) {
      if (i->tag_id () == m_tag_id) {
        va = i->get ();
      }
    }

    for (rdb::Values::const_iterator i = ib.values ().begin (); i != ib.values ().end () && !vb; ++i) {
      if (i->tag_id () == m_tag_id) {
        vb = i->get ();
      }
    }

    if ((va == 0) != (vb == 0)) {
      return ((va == 0) < (vb == 0));
    } else if (va == 0 && vb == 0) {
      return false;
    } else {
      return rdb::ValueBase::compare (va, vb);
    }
  }

private:
  rdb::id_type m_tag_id;
};

class MarkerBrowserListViewModel
  : public QAbstractItemModel
{
public:
  MarkerBrowserListViewModel ()
    : mp_database (0), m_sorting (-1), m_sorting_order (false)
  {
    for (size_t i = 0; i < sizeof (m_flag_tag_ids) / sizeof (m_flag_tag_ids [0]); ++i) {
      m_flag_tag_ids [i] = 0;
    }
    m_waived_tag_id = 0;
    m_important_tag_id = 0;
  }

  QModelIndex index_of_row (int row)
  {
    return createIndex (row, 0, (void *)0);
  }

  void clear ()
  {
    beginResetModel ();
    m_item_list.clear ();
    endResetModel ();
  }

  void set_sorting (int sorting, bool sorting_order)
  {
    m_sorting = sorting;
    m_sorting_order = sorting_order;
  }

  int sorting () const
  {
    return m_sorting;
  }

  bool sorting_order () const
  {
    return m_sorting_order;
  }

  template <class Iter>
  bool set_items (const std::vector <std::pair<Iter, Iter> > &be_vector, size_t max_marker_count)
  {
    beginResetModel ();

    typedef Iter iterator_type;
    typedef std::pair<Iter, Iter> iterator_pair_type;
    typedef typename std::vector<iterator_pair_type>::const_iterator ipv_iterator_type;

    size_t n = 0;
    bool clipped = false;

    for (ipv_iterator_type be = be_vector.begin (); be != be_vector.end () && n < max_marker_count; ++be) {
      iterator_type i = be->first; 
      for ( ; n < max_marker_count && i != be->second; ++n) {
        ++i;
      }
      if (i != be->second) {
        clipped = true;
      }
    }

    m_item_list.clear ();
    m_item_list.reserve (n + 1);
  
    if (m_sorting == 0 || m_sorting == 1 || m_sorting == 2) {

      id_type tags_in_order [sizeof (flag_descriptors) / sizeof (flag_descriptors [0])];
      size_t n_tags = 0;

      if (m_sorting == 0) {

        n_tags = sizeof (flag_descriptors) / sizeof (flag_descriptors [0]);

        for (unsigned int j = 1; j < n_tags; ++j) {
          tags_in_order [j - 1] = m_flag_tag_ids [j];
        }

        tags_in_order [n_tags - 1] = 0;

      } else if (m_sorting == 1) {

        n_tags = 2;
        tags_in_order [0] = m_important_tag_id;
        tags_in_order [1] = 0;

      } else if (m_sorting == 2) {

        n_tags = 2;
        tags_in_order [0] = m_waived_tag_id;
        tags_in_order [1] = 0;

      }

      if (m_sorting_order) {
        std::reverse (tags_in_order, tags_in_order + n_tags);
      }

      ipv_iterator_type be = be_vector.begin ();
      while (be != be_vector.end () && be->first == be->second) {
        ++be;
      }

      if (be != be_vector.end ()) {

        iterator_type i = be->first;

        //  Select markers in the order which the tag selection defines.
        //  A tag selection of "0" means "none of the other tags".
        unsigned int itag = 0;
        for (n = 0; n <= max_marker_count && itag != n_tags; ) {

          if (tags_in_order [itag] == 0) {

            bool has_tag = false;
            for (unsigned int t = 0; t < n_tags && !has_tag; ++t) {
              if (t != itag) {
                has_tag = access (*i).has_tag (tags_in_order [t]);
              }
            }

            if (! has_tag) {
              if (n == max_marker_count) {
                m_item_list.push_back (0);
              } else {
                m_item_list.push_back (&access (*i));
              }
              ++n;
            }

          } else if (access (*i).has_tag (tags_in_order [itag])) {

            if (n == max_marker_count) {
              m_item_list.push_back (0);
            } else {
              m_item_list.push_back (&access (*i));
            }
            ++n;

          }

          ++i;
          if (i == be->second) {

            do {
              ++be;
              if (be == be_vector.end ()) {
                be = be_vector.begin ();
                ++itag;
              }
            } while (be->first == be->second);

            i = be->first;

          }

        }

      }

    } else if (m_sorting > 3 && m_sorting - 4 < int (m_user_tags.size ())) {

      ipv_iterator_type be;

      size_t n = 0;
      for (be = be_vector.begin (); be != be_vector.end (); ++be) {
        for (iterator_type i = be->first; i != be->second; ++i) {
          ++n;
        }
      }

      std::vector<Iter> ii;
      ii.reserve (n);

      for (be = be_vector.begin (); be != be_vector.end (); ++be) {
        for (iterator_type i = be->first; i != be->second; ++i) {
          ii.push_back (i);
        }
      }

      tl::sort (ii.begin (), ii.end (), ValueIterSorter<Iter> (m_user_tags [m_sorting - 4].second));

      if (! m_sorting_order) {
        std::reverse (ii.begin (), ii.end ());
      }

      n = 0;

      typename std::vector<Iter>::const_iterator j = ii.begin ();
      for (j = ii.begin (); j != ii.end (); ++n, ++j) {
        if (n == max_marker_count) {
          //  "..." placeholder for further items
          m_item_list.push_back (0);
          break;
        } else {
          m_item_list.push_back (&access (**j));
        }
      }

    } else {

      n = 0;

      ipv_iterator_type be;
      for (be = be_vector.begin (); be != be_vector.end () && n < max_marker_count; ++be) {
        for (iterator_type i = be->first; i != be->second; ++n, ++i) {
          if (n == max_marker_count) {
            //  "..." placeholder for further items
            m_item_list.push_back (0);
            break;
          } else {
            m_item_list.push_back (&access (*i));
          }
        }
      }

    }

    endResetModel ();

    return clipped;
  }

  const rdb::Item *item (int row) const
  {
    if (row >= 0 && row < int (m_item_list.size ())) {
      return m_item_list [row];
    } else {
      return 0;
    }
  }

  void set_database (const rdb::Database *db)
  {
    mp_database = db;

    if (mp_database) {

      m_user_tags.clear ();
      for (rdb::Tags::const_iterator t = mp_database->tags ().begin_tags (); t != mp_database->tags ().end_tags (); ++t) {
        if (t->is_user_tag ()) {
          m_user_tags.push_back (std::make_pair (t->name (), t->id ()));
        }
      }

      for (unsigned int j = 1; j < sizeof (flag_descriptors) / sizeof (flag_descriptors [0]); ++j) {
        m_flag_tag_ids [j] = mp_database->tags ().tag (flag_descriptors [j].tag).id ();
      }

      m_waived_tag_id = mp_database->tags ().tag ("waived").id ();
      m_important_tag_id = mp_database->tags ().tag ("important").id ();

    }

    clear ();
  }

  void mark_data_changed ()
  {
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, columnCount (QModelIndex ()) - 1, QModelIndex ()));
  }

  int columnCount (const QModelIndex & /*parent*/) const
  {
    return 4 + int (m_user_tags.size ());
  }

  QVariant headerData (int section, Qt::Orientation /*orientation*/, int role) const
  {
    if (role == Qt::DisplayRole) {
      if (section == 0) {
        return QVariant (QString::fromUtf8 ("F"));
      } else if (section == 1) {
        return QVariant (QString::fromUtf8 ("I"));
      } else if (section == 2) {
        return QVariant (QString::fromUtf8 ("W"));
      } else if (section == 3) {
        return QVariant (QObject::tr ("Marker"));
      } else if (section > 3 && section - 4 < int (m_user_tags.size ())) {
        return QVariant (QString::fromUtf8 (m_user_tags [section - 4].first.c_str ()));
      }
    }

    return QVariant ();
  }

  QVariant data (const QModelIndex &index, int role) const
  {
    if (!mp_database || !index.isValid ()) {
      return QVariant ();
    }

    if (role == Qt::DecorationRole) {

      if (index.column () == 0) {

        const rdb::Item *i = item (index.row ());
        if (i != 0) {

          for (unsigned int j = 1; j < sizeof (flag_descriptors) / sizeof (flag_descriptors [0]); ++j) {
            if (i->has_tag (m_flag_tag_ids [j])) {
              return QVariant (QIcon (tl::to_qstring (flag_descriptors [j].icon)));
            }
          }

          return QVariant (QIcon (tl::to_qstring (flag_descriptors [0].icon)));

        }

      } else if (index.column () == 1) {

        const rdb::Item *i = item (index.row ());
        if (i != 0 && i->has_tag (m_important_tag_id)) {
          return QVariant (QIcon (QString::fromUtf8 (":important_16px.png")));
        }

      } else if (index.column () == 2) {

        const rdb::Item *i = item (index.row ());
        if (i != 0 && i->has_tag (m_waived_tag_id)) {
          return QVariant (QIcon (QString::fromUtf8 (":waived_16px.png")));
        }

      }

    } else if (role == Qt::DisplayRole) {

      if (index.column () > 3 && index.column () - 4 < int (m_user_tags.size ())) {

        const rdb::Item *i = item (index.row ());
        if (i != 0) {

          rdb::id_type tag_id = m_user_tags [index.column () - 4].second;

          for (rdb::Values::const_iterator v = i->values ().begin (); v != i->values ().end (); ++v) {

            if (v->get () && v->tag_id () == tag_id) {

              std::string value_string = v->get ()->to_display_string ();

              size_t max_length = 100;
              if (value_string.size () > max_length) {
                value_string = std::string (value_string.begin (), value_string.begin () + max_length) + "...";
              }

              return QVariant (QString::fromUtf8 (value_string.c_str ()));

            }

          }

        } 

      } else if (index.column () == 3) {

        const rdb::Item *i = item (index.row ());
        if (i == 0) {
          return QVariant (QString::fromUtf8 ("..."));
        } else {

          const rdb::Cell *cell = mp_database->cell_by_id (i->cell_id ());
          const rdb::Category *cat = mp_database->category_by_id (i->category_id ());

          std::string r;

          if (cat) {
            r += cat->path ();
          }

          if (cell && !cell->name ().empty ()) {
            if (! r.empty ()) {
              r += " ";
            }
            r += "[";
            r += cell->name ();
            r += "]";
          }

          std::string value; 

          for (rdb::Values::const_iterator v = i->values ().begin (); v != i->values ().end (); ++v) {
            if (v->tag_id () == 0 && v->get () && (v->get ()->type_index () == rdb::type_index_of<std::string> () ||
                                                   v->get ()->type_index () == rdb::type_index_of<double> ())) {
              if (! value.empty ()) {
                value += ", ";
              }
              value += v->get ()->to_display_string ();
            }
          }

          if (! value.empty ()) {
            r += " - ";
            r += value;
          }

          return QVariant (tl::to_qstring (r));

        }

      }

    } else if (role == Qt::FontRole) {

      const rdb::Item *i = item (index.row ());
      if (i) {

        QFont font;

        if (! i->visited ()) {
          //  Bold font for non-visited items
          font.setBold (true);
        }

        if (i->has_tag (m_waived_tag_id)) {
          //  Strikeout for waived items
          font.setStrikeOut (true);
        }

        return QVariant (font);

      } 

    }

    return QVariant ();
  }

  bool hasChildren (const QModelIndex &parent) const
  {
    return rowCount (parent) != 0;
  }

  QModelIndex index (int row, int column, const QModelIndex &parent) const
  {
    if (mp_database && ! parent.isValid () && row >= 0 && row < int (m_item_list.size ())) {
      return createIndex (row, column);
    } else {
      return QModelIndex ();
    }
  }

  QModelIndex parent (const QModelIndex & /*index*/) const
  {
    return QModelIndex ();
  }

  int rowCount (const QModelIndex &index) const
  {
    if (mp_database && ! index.isValid ()) {
      return int (m_item_list.size ());
    } else {
      return 0;
    }
  }

private:
  std::vector<const rdb::Item *> m_item_list;
  const rdb::Database *mp_database;
  std::vector<std::pair<std::string, rdb::id_type> > m_user_tags;
  id_type m_flag_tag_ids [sizeof (flag_descriptors) / sizeof (flag_descriptors [0])];
  id_type m_waived_tag_id;
  id_type m_important_tag_id;
  int m_sorting;
  bool m_sorting_order;
};

// ----------------------------------------------------------------------------------
//  MarkerBrowserSnapshowView definition and implementation

class MarkerBrowserSnapshotView
  : public QDialog, 
    public Ui::MarkerBrowserSnapshotView 
{
public:
  MarkerBrowserSnapshotView (QWidget *parent, const QImage &image)
    : QDialog (parent)
  {
    Ui::MarkerBrowserSnapshotView::setupUi (this);

    content->set_image (image);
    content->setHtml (QString::fromUtf8 ("<img src=\"item.image\"/>"));
  }
};

// ----------------------------------------------------------------------------------
//  MarkerBrowserPage implementation

MarkerBrowserPage::MarkerBrowserPage (QWidget * /*parent*/)
  : m_enable_updates (true),
    m_update_needed (false),
    mp_database (0), 
    m_show_all (true),
    m_list_shapes (true),
    mp_view (0), 
    m_cv_index (0),
    m_num_items (0), 
    m_view_changed (false),
    m_recursion_sentinel (false),
    m_in_directory_selection_change (false),
    m_context (rdb::DatabaseTop), 
    m_window (rdb::FitMarker), 
    m_window_dim (0.0), 
    m_max_marker_count (1000),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_current_flag (0),
    m_marker_list_sorted_section (-1),
    m_marker_list_sort_order (Qt::DescendingOrder),
    m_directory_tree_sorted_section (-1),
    m_directory_tree_sort_order (Qt::DescendingOrder),
    mp_plugin_root (0),
    dm_rerun_macro (this, &MarkerBrowserPage::rerun_macro)
{
  Ui::MarkerBrowserPage::setupUi (this);

  directory_tree->installEventFilter (this);
  markers_list->installEventFilter (this);
  warn_label->hide ();

  QMenu *flags_menu = new QMenu (this);
  for (unsigned int i = 0; i < sizeof (flag_descriptors) / sizeof (flag_descriptors[0]); ++i) {
    QAction *action = flags_menu->addAction (QIcon (tl::to_qstring (flag_descriptors [i].icon)), tl::to_qstring (flag_descriptors [i].text), this, SLOT (flag_menu_selected ()));
    action->setData (QVariant (int (i)));
  }

  flags_pb->setMenu (flags_menu);
  flags_pb->setIcon (QIcon (tl::to_qstring (flag_descriptors [0].icon)));

#if QT_VERSION >= 0x040300
  connect (directory_tree->header (), SIGNAL (sortIndicatorChanged (int, Qt::SortOrder)), this, SLOT (directory_sorting_changed (int, Qt::SortOrder)));
  connect (markers_list->header (), SIGNAL (sortIndicatorChanged (int, Qt::SortOrder)), this, SLOT (markers_sorting_changed (int, Qt::SortOrder)));
#else
  connect (directory_tree->header (), SIGNAL (sectionClicked (int)), this, SLOT (directory_header_clicked (int)));
  connect (markers_list->header (), SIGNAL (sectionClicked (int)), this, SLOT (markers_header_clicked (int)));
#endif

  directory_tree->header ()->setStretchLastSection (true);
  directory_tree->header ()->setSortIndicatorShown (true);
  
  markers_list->header ()->setStretchLastSection (true);
#if QT_VERSION >= 0x050000
  markers_list->header ()->setSectionResizeMode (QHeaderView::Interactive);
#else
  markers_list->header ()->setResizeMode (QHeaderView::Interactive);
#endif
  markers_list->header ()->setSortIndicatorShown (true);
  markers_list->header ()->setMinimumSectionSize (24);

  QAction *select_all_info_action = new QAction (this);
  select_all_info_action->setText (tr ("Select All"));
  connect (select_all_info_action, SIGNAL (triggered ()), info_text, SLOT (selectAll ()));

  QAction *copy_info_action = new QAction (this);
  copy_info_action->setText (tr ("Copy"));
  connect (copy_info_action, SIGNAL (triggered ()), info_text, SLOT (copy ()));

  info_text->addAction (select_all_info_action);
  info_text->addAction (copy_info_action);
  info_text->setContextMenuPolicy (Qt::ActionsContextMenu);

  list_shapes_cb->setChecked (m_list_shapes);

  connect (markers_list, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (marker_double_clicked (const QModelIndex &)));

  connect (dir_up_pb, SIGNAL (clicked ()), this, SLOT (dir_up_clicked ()));
  connect (dir_down_pb, SIGNAL (clicked ()), this, SLOT (dir_down_clicked ()));
  connect (list_up_pb, SIGNAL (clicked ()), this, SLOT (list_up_clicked ()));
  connect (list_down_pb, SIGNAL (clicked ()), this, SLOT (list_down_clicked ()));
  connect (flags_pb, SIGNAL (clicked ()), this, SLOT (flag_button_clicked ()));
  connect (important_pb, SIGNAL (clicked ()), this, SLOT (important_button_clicked ()));
  connect (edit_pb, SIGNAL (clicked ()), this, SLOT (edit_button_clicked ()));
  connect (waive_pb, SIGNAL (clicked ()), this, SLOT (waived_button_clicked ()));
  connect (photo_pb, SIGNAL (clicked ()), this, SLOT (snapshot_button_clicked ()));
  connect (nophoto_pb, SIGNAL (clicked ()), this, SLOT (remove_snapshot_button_clicked ()));
  connect (info_text, SIGNAL (anchorClicked (const QUrl &)), this, SLOT (info_anchor_clicked (const QUrl &)));
  connect (cat_filter, SIGNAL (textEdited (const QString &)), this, SLOT (filter_changed ()));
  connect (cell_filter, SIGNAL (textEdited (const QString &)), this, SLOT (filter_changed ()));
  connect (rerun_button, SIGNAL (pressed ()), this, SLOT (rerun_button_pressed ()));
  connect (list_shapes_cb, SIGNAL (clicked ()), this, SLOT (list_shapes_clicked ()));

  m_show_all_action = new QAction (QObject::tr ("Show All"), this);
  m_show_all_action->setCheckable (true);
  m_show_all_action->setChecked (m_show_all);
  connect (m_show_all_action, SIGNAL (triggered ()), this, SLOT (show_all_clicked ()));

  QAction *revisit_non_waived_action = new QAction (QObject::tr ("Revisit Non-Waived Markers"), this);
  connect (revisit_non_waived_action, SIGNAL (triggered ()), this, SLOT (revisit_non_waived ()));
  QAction *revisit_important_action = new QAction (QObject::tr ("Revisit Important Markers"), this);
  connect (revisit_important_action, SIGNAL (triggered ()), this, SLOT (revisit_important ()));
  QAction *revisit_all_action = new QAction (QObject::tr ("Revisit All"), this);
  connect (revisit_all_action, SIGNAL (triggered ()), this, SLOT (revisit_all ()));
  QAction *unwaive_all_action = new QAction (QObject::tr ("Unwaive All"), this);
  connect (unwaive_all_action, SIGNAL (triggered ()), this, SLOT (unwaive_all ()));

  QAction *mark_important_action = new QAction (QObject::tr ("Mark Important"), this);
  connect (mark_important_action, SIGNAL (triggered ()), this, SLOT (mark_important ()));
  QAction *mark_unimportant_action = new QAction (QObject::tr ("Mark Unimportant"), this);
  connect (mark_unimportant_action, SIGNAL (triggered ()), this, SLOT (mark_unimportant ()));
  QAction *mark_visited_action = new QAction (QObject::tr ("Mark Visited"), this);
  connect (mark_visited_action, SIGNAL (triggered ()), this, SLOT (mark_visited ()));
  QAction *mark_notvisited_action = new QAction (QObject::tr ("Mark Not Visited"), this);
  connect (mark_notvisited_action, SIGNAL (triggered ()), this, SLOT (mark_notvisited ()));
  QAction *waive_action = new QAction (QObject::tr ("Waive"), this);
  connect (waive_action, SIGNAL (triggered ()), this, SLOT (waive ()));
  QAction *unwaive_action = new QAction (QObject::tr ("Unwaive"), this);
  connect (unwaive_action, SIGNAL (triggered ()), this, SLOT (unwaive ()));

  QAction *separator;

  directory_tree->addAction (m_show_all_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  directory_tree->addAction (separator);
  directory_tree->addAction (revisit_non_waived_action);
  directory_tree->addAction (revisit_important_action);
  directory_tree->addAction (revisit_all_action);
  directory_tree->addAction (unwaive_all_action);

  markers_list->addAction (mark_important_action);
  markers_list->addAction (mark_unimportant_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  markers_list->addAction (separator);
  markers_list->addAction (mark_visited_action);
  markers_list->addAction (mark_notvisited_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  markers_list->addAction (separator);
  markers_list->addAction (waive_action);
  markers_list->addAction (unwaive_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  markers_list->addAction (separator);
  markers_list->addAction (revisit_non_waived_action);
  markers_list->addAction (revisit_important_action);
  markers_list->addAction (revisit_all_action);
  markers_list->addAction (unwaive_all_action);

#if QT_VERSION >= 0x040700
  cell_filter->setPlaceholderText (tr ("Cell"));
  cell_filter_label->hide ();
  cat_filter->setPlaceholderText (tr ("Category"));
  cat_filter_label->hide ();
#endif
}

MarkerBrowserPage::~MarkerBrowserPage ()
{
  release_markers ();

  QAbstractItemModel *tree_model = directory_tree->model ();
  if (tree_model) {
    directory_tree->setModel (0);
    delete tree_model;
  }

  QAbstractItemModel *list_model = markers_list->model ();
  if (list_model) {
    markers_list->setModel (0);
    delete list_model;
  }  
}

void
MarkerBrowserPage::set_dispatcher (lay::Dispatcher *pr)
{
  mp_plugin_root = pr;
}

void 
MarkerBrowserPage::set_marker_style (tl::Color color, int line_width, int vertex_size, int halo, int dither_pattern)
{
  m_marker_color = color;
  m_marker_line_width = line_width;
  m_marker_vertex_size = vertex_size;
  m_marker_halo = halo;
  m_marker_dither_pattern = dither_pattern;
  update_markers ();
}

void 
MarkerBrowserPage::set_view (lay::LayoutViewBase *view, unsigned int cv_index)
{
  mp_view = view;
  m_cv_index = cv_index;
  update_markers ();
  update_info_text ();
}

static void 
set_hidden_rec (MarkerBrowserTreeViewModel *model, QTreeView *tree_view, const QModelIndex &parent, bool show_all, const QString &cat_filter, const QString &cell_filter)
{
  int rows = model->rowCount (parent);
  for (int r = 0; r < rows; ++r) {

    QModelIndex index = model->index (r, 0, parent);
    bool hidden = (!show_all && model->no_errors (index)) || 
                  (!cat_filter.isEmpty() && !model->cat_matches (index, cat_filter)) ||
                  (!cell_filter.isEmpty() && !model->cell_matches (index, cell_filter));

    tree_view->setRowHidden (r, parent, hidden);

    set_hidden_rec (model, tree_view, index, show_all, cat_filter, cell_filter);

  }
}

void 
MarkerBrowserPage::filter_changed ()
{
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
  }

  update_marker_list (2 /*select all*/);
}

void 
MarkerBrowserPage::show_all (bool f)
{
  if (f != m_show_all) {

    m_show_all = f;
    m_show_all_action->setChecked (f);

    MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
    if (tree_model) {
      set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
    }

  }
}

void
MarkerBrowserPage::list_shapes (bool f)
{
  if (f != m_list_shapes) {

    m_list_shapes = f;
    list_shapes_cb->setChecked (f);

    update_info_text ();

  }
}

void
MarkerBrowserPage::set_rdb (rdb::Database *database)
{
  if (database != mp_database) {

    release_markers ();

    mp_database = database;

    rerun_button->setEnabled (mp_database && ! mp_database->generator ().empty ());
    if (rerun_button->isEnabled ()) {
      QString shortcut;
      if (! rerun_button->shortcut ().isEmpty ()) {
        shortcut = QString::fromUtf8 (" (%1)").arg (rerun_button->shortcut ().toString ());
      }
      rerun_button->setToolTip (tl::to_qstring (tl::to_string (tr ("Run ")) + mp_database->generator ()) + shortcut);
    } else {
      rerun_button->setToolTip (QString ());
    }

    //  mark items visited that carry the waived flag
    if (mp_database) {
      id_type waived_tag_id = mp_database->tags ().tag ("waived").id ();
      for (auto i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
        if (i->has_tag (waived_tag_id)) {
          mp_database->set_item_visited (i.operator-> (), true);
        }
      }
    }

    QAbstractItemModel *tree_model = directory_tree->model ();

    MarkerBrowserTreeViewModel *new_model = new MarkerBrowserTreeViewModel ();
    new_model->set_show_empty_ones (true);
    new_model->set_database (database);
    directory_tree->setModel (new_model);
    connect (directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (directory_selection_changed (const QItemSelection &, const QItemSelection &)));

    directory_tree->header ()->setSortIndicatorShown (true);

    cat_filter->setText (QString ());
    cell_filter->setText (QString ());
    set_hidden_rec (new_model, directory_tree, QModelIndex (), m_show_all, QString (), QString ());

    if (tree_model) {
      delete tree_model;
    }

    QAbstractItemModel *list_model = markers_list->model ();

    MarkerBrowserListViewModel *new_list_model = new MarkerBrowserListViewModel ();
    //  default sorting is by waived flag
    new_list_model->set_sorting (2, true);
    markers_list->header ()->setSortIndicator (new_list_model->sorting (), new_list_model->sorting_order () ? Qt::AscendingOrder : Qt::DescendingOrder);
    new_list_model->set_database (database);
    markers_list->setModel (new_list_model);
    connect (markers_list->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (markers_selection_changed (const QItemSelection &, const QItemSelection &)));
    connect (markers_list->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (markers_current_changed (const QModelIndex &, const QModelIndex &)));

    if (list_model) {
      delete list_model;
    }

  }
}

static std::string top_item_by_index (int i)
{
  if (i == 0) {
    return std::string ("by-cell");
  } else if (i == 1) {
    return std::string ("by-category");
  } else {
    return std::string ();
  }
}

static int top_index_from_item (const std::string &s)
{
  if (s == "by-cell") {
    return 0;
  } else if (s == "by-category") {
    return 1;
  } else {
    return -1;
  }
}

std::string
MarkerBrowserPage::get_tree_state ()
{
  std::string res;

  QAbstractItemModel *tree_model = directory_tree->model ();
  if (! tree_model) {
    return res;
  }

  int rows = tree_model->rowCount (QModelIndex ());
  for (int i = 0; i < rows; ++i) {
    bool expanded = directory_tree->isExpanded (tree_model->index (i, 0, QModelIndex ()));
    std::string item = top_item_by_index (i);
    if (! item.empty ()) {
      if (! res.empty ()) {
        res += ",";
      }
      res += expanded ? "+" : "-";
      res += item;
    }
  }

  return res;
}

void
MarkerBrowserPage::set_tree_state (const std::string &state)
{
  QAbstractItemModel *tree_model = directory_tree->model ();
  if (! tree_model) {
    return;
  }

  tl::Extractor ex (state.c_str ());
  while (! ex.at_end ()) {
    bool expanded = false;
    if (ex.test ("+")) {
      expanded = true;
    } else {
      ex.test ("-");
    }
    std::string item;
    if (! ex.try_read_word (item, "-_")) {
      break;
    }
    int index = top_index_from_item (item);
    if (index >= 0) {
      directory_tree->setExpanded (tree_model->index (index, 0, QModelIndex ()), expanded);
    }
    ex.test (",");
  }
}

void
MarkerBrowserPage::update_content ()
{

  // ...

}

void
MarkerBrowserPage::markers_header_clicked (int section)
{
  Qt::SortOrder so = m_marker_list_sort_order;

  if (so == Qt::AscendingOrder) {
    so = Qt::DescendingOrder; 
  } else {
    so = Qt::AscendingOrder; 
  }

  m_marker_list_sort_order = so;
  m_marker_list_sorted_section = section;

  markers_list->header ()->setSortIndicator (section, so);
  markers_list->header ()->setSortIndicatorShown (true);

  markers_sorting_changed (section, so);
}

void
MarkerBrowserPage::markers_sorting_changed (int section, Qt::SortOrder order)
{
  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (list_model) {
    list_model->set_sorting (section >= 0 ? section : -1, order == Qt::AscendingOrder);
    update_marker_list (1 /*select first*/);
  }
}

void
MarkerBrowserPage::directory_header_clicked (int section)
{
  Qt::SortOrder so = m_directory_tree_sort_order;

  if (so == Qt::AscendingOrder) {
    so = Qt::DescendingOrder; 
  } else {
    so = Qt::AscendingOrder; 
  }

  m_directory_tree_sort_order = so;
  m_directory_tree_sorted_section = section;

  directory_tree->header ()->setSortIndicator (section, so);
  directory_tree->header ()->setSortIndicatorShown (true);

  directory_sorting_changed (section, so);
}

void
MarkerBrowserPage::directory_sorting_changed (int section, Qt::SortOrder order)
{
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {

    if (section == 0) {
      tree_model->sort_by (MarkerBrowserTreeViewModel::ByKeyName, order == Qt::AscendingOrder);
    } else if (section == 1) {
      tree_model->sort_by (MarkerBrowserTreeViewModel::ByCount, order == Qt::AscendingOrder);
    }

    //  reset the item's visibility
    set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());

  }

}

void 
MarkerBrowserPage::markers_selection_changed (const QItemSelection &, const QItemSelection &)
{
  update_markers ();
  update_info_text ();
}

void
MarkerBrowserPage::marker_double_clicked (const QModelIndex &)
{
  if (! m_markers_bbox.empty () && mp_view) {
    mp_view->zoom_box (m_markers_bbox.enlarged (db::DVector (m_markers_bbox.width () * 0.1, m_markers_bbox.height () * 0.1)));
  }
}

void 
MarkerBrowserPage::set_window (rdb::window_type window, const lay::Margin &window_dim, rdb::context_mode_type context)
{
  if (window != m_window || window_dim != m_window_dim || context != m_context) {
    m_window = window;
    m_window_dim = window_dim;
    m_context = context;
    update_markers ();
    update_info_text ();
  }
}

void
MarkerBrowserPage::set_max_marker_count (size_t max_marker_count)
{
  if (m_max_marker_count != max_marker_count) {
    m_max_marker_count = max_marker_count;
    update_marker_list (1 /*select first*/);
  }
}

void 
MarkerBrowserPage::enable_updates (bool f)
{
  if (f != m_enable_updates) {

    m_enable_updates = f;

    if (f && m_update_needed) {
      update_markers ();
      update_info_text ();
    }

    m_update_needed = false;

  }
}

void
MarkerBrowserPage::update_markers ()
{
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  if (m_recursion_sentinel) {
    return;
  }

  m_recursion_sentinel = true;
  try {
    do_update_markers ();
  } catch (...) {
    m_recursion_sentinel = false;
    throw;
  }
  m_recursion_sentinel = false;
}

void
MarkerBrowserPage::update_info_text ()
{
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  if (! mp_database) {
    info_text->setHtml (QString ());
    markers_label->setText (QString ());
    return;
  }

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  if (selected.isEmpty ()) {
    info_text->setHtml (QString ());
    markers_label->setText (QString ());
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (list_model) {

    const rdb::Cell *cell = 0;
    size_t n_cell = 0;
    const rdb::Category *category = 0;
    size_t n_category = 0;
    const rdb::Item *item = 0;
    size_t n_item = 0;
    std::string comment;
    size_t n_comment = 0;

    for (QModelIndexList::const_iterator selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {

      if (selected_item->column () == 0) {

        const rdb::Item *i = list_model->item (selected_item->row ());
        if (i) {

          item = i;
          ++n_item;

          if (! item->comment ().empty () && item->comment () != comment) {
            comment = item->comment ();
            ++n_comment;
          }

          const rdb::Cell *c = mp_database->cell_by_id (item->cell_id ());
          if (c && c != cell) {
            cell = c;
            ++n_cell;
          } 

          const rdb::Category *x = mp_database->category_by_id (item->category_id ());
          if (x && x != category) {
            category = x;
            ++n_category;
          } 

        }

      }

    }

    //  Produce the info text ...

    std::string info;
    info.reserve (8192);

    info += "<h3>";

    if (category && n_category == 1) {
      tl::escape_to_html (info, category->name ());
    }

    if (cell && n_cell == 1 && ! cell->name ().empty ()) {
      tl::escape_to_html (info, std::string (" [") + cell->name () + "]");
    }

    info += "</h3>";

    if (category && n_category == 1 && ! category->description ().empty ()) {
      info += "<p style=\"color:blue; font-weight: bold\">";
      tl::escape_to_html (info, category->description ());
      info += "</p>";
    }

    if (! m_error_text.empty ()) {
      info += "<p style=\"color:red; font-weight: bold\">";
      tl::escape_to_html (info, m_error_text);
      info += "</p>";
    }

    if (! comment.empty () && n_comment == 1) {
      info += "<p style=\"color:gray\">";
      tl::escape_to_html (info, comment);
      info += "</p>";
    }

    info += "<p/>";

    if (item && n_item == 1) {

      info += "<pre>";

      for (rdb::Values::const_iterator v = item->values ().begin (); v != item->values ().end (); ++v) {

        if (v->get () != 0 && (m_list_shapes || ! v->get ()->is_shape ())) {

          if (v->tag_id () != 0) {
            const rdb::Tag &tag = mp_database->tags ().tag (v->tag_id ());
            info += "<b>";
            tl::escape_to_html (info, tag.name ());
            info += ":</br> ";
          }

          std::string value_string = v->get ()->to_display_string ();

          size_t max_length = 200;
          if (value_string.size () > max_length) {
            value_string = std::string (value_string.begin (), value_string.begin () + max_length) + "...";
          }

          tl::escape_to_html (info, value_string);

          info += "<br/>";

        }

      }

      info += "</pre>";

      QImage image = item->image ();
      if (! image.isNull ()) {
        info += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\" style=\"border-color:blue; border-style:solid\"><tr><td><p>Snapshot image<br/>(click to enlarge)</p><p><a href=\"show-snapshot\"><img src=\"item.overview-image\"/></a></p></td></tr></table>";
        info_text->set_image (image);
      }

    }

    info_text->setHtml (tl::to_qstring (info));

  }
}

void
MarkerBrowserPage::do_update_markers ()
{
  release_markers ();

  if (! mp_database) {
    markers_label->setText (QString ());
    return;
  }

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  if (selected.isEmpty ()) {
    markers_label->setText (QString ());
    return;
  }

  size_t item_index = 0;

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (list_model) {

    const rdb::Cell *cell = 0;
    size_t n_cell = 0;
    const rdb::Category *category = 0;
    size_t n_category = 0;
    const rdb::Item *item = 0;
    size_t n_item = 0;

    m_markers_bbox = db::DBox ();

    for (QModelIndexList::const_iterator selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {

      if (selected_item->column () == 0) {

        const rdb::Item *i = list_model->item (selected_item->row ());
        if (i) {

          item = i;
          item_index = size_t (selected_item->row ());
          ++n_item;

          const rdb::Cell *c = mp_database->cell_by_id (item->cell_id ());
          if (c && c != cell) {
            cell = c;
            ++n_cell;
          } 

          const rdb::Category *x = mp_database->category_by_id (item->category_id ());
          if (x && x != category) {
            category = x;
            ++n_category;
          } 

        }

      }

    }

    m_error_text.clear ();

    //  Switch to the context cell if possible and required
    if (mp_view) {

      const rdb::Cell *current_cell = 0;

      if (m_context == rdb::AnyCell) {

        current_cell = mp_database->cell_by_qname (mp_database->top_cell_name ());

      } else if (m_context == rdb::DatabaseTop) {

        const lay::CellView &cv = mp_view->cellview (m_cv_index);
        if (cv.is_valid ()) {

          std::pair<bool, db::cell_index_type> cc = cv->layout ().cell_by_name (mp_database->top_cell_name ().c_str ());
          if (cc.first && cc.second != cv.cell_index ()) {
            mp_view->select_cell (cc.second, m_cv_index);
            current_cell = mp_database->cell_by_qname (mp_database->top_cell_name ());
            m_view_changed = true;
          }

        }

      } else if (m_context == rdb::Local && cell != 0 && n_cell == 1) {

        const lay::CellView &cv = mp_view->cellview (m_cv_index);
        if (cv.is_valid ()) {

          std::pair<bool, db::cell_index_type> cc = cv->layout ().cell_by_name (cell->name ().c_str ());
          if (cc.first && cc.second != cv.cell_index ()) {
            mp_view->select_cell (cc.second, m_cv_index);
            current_cell = cell;
            m_view_changed = true;
          }

        }

      }

      lay::CellView cv = mp_view->cellview (m_cv_index);
      if (! current_cell && cv.is_valid ()) {
        current_cell = mp_database->cell_by_qname (cv->layout ().cell_name (cv.ctx_cell_index ()));
      }

      std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);
      if (tv.empty ()) {
        tv.push_back (db::DCplxTrans ());
      }

      for (QModelIndexList::const_iterator selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {

        if (selected_item->column () > 0) {
          continue;
        }

        const rdb::Item *i = list_model->item (selected_item->row ());
        if (! i) {
          continue;
        }

        const rdb::Cell *c = mp_database->cell_by_id (i->cell_id ());
        if (! c || c->name ().empty ()) {
          continue;
        }

        //  Determine the context transformation
        std::pair <bool, db::DCplxTrans> context (false, db::DCplxTrans ());
        if (current_cell) {
          context = c->path_to (current_cell->id (), mp_database);
        }

        if (! context.first && cv.is_valid ()) {
          //  If we could not find a transformation in the RDB, try to find one in the layout DB:
          std::pair<bool, db::cell_index_type> cc = cv->layout ().cell_by_name (c->name ().c_str ());
          if (cc.first) {
            std::pair <bool, db::ICplxTrans> ic = db::find_layout_context (cv->layout (), cc.second, cv.ctx_cell_index ());
            if (ic.first) {
              context.first = true;
              context.second = db::DCplxTrans (cv->layout ().dbu ()) * db::DCplxTrans (ic.second) * db::DCplxTrans (1.0 / cv->layout ().dbu ());
            }
          }
        }

        if (! context.first && cv.is_valid ()) {

          if (m_context == rdb::AnyCell || m_context == rdb::CurrentOrAny) {
            //  Ultimate fallback in "any cell" mode is to take whatever cell we have ..
            context = std::pair <bool, db::DCplxTrans> (true, db::DCplxTrans ());
          } else if (! current_cell) {
            m_error_text = tl::sprintf (tl::to_string (QObject::tr ("Current layout cell '%s' not found in marker database and no path found from marker's cell '%s' to current cell in the layout database.")),
                                        cv->layout ().cell_name (cv.ctx_cell_index ()), c->name ());
          } else {
            m_error_text = tl::sprintf (tl::to_string (QObject::tr ("No example instantiation given in marker database for marker's cell '%s' to current cell '%s' and no such path in the layout database either.")),
                                        c->name (), current_cell->name ());
          }

        }

        //  If a suitable context could be found ..
        if (context.first) {

          db::DCplxTrans trans = tv[0] * context.second;

          //  Produce the markers ...
          for (rdb::Values::const_iterator v = i->values ().begin (); v != i->values ().end (); ++v) {

            const rdb::Value<db::DPolygon> *polygon_value = dynamic_cast <const rdb::Value<db::DPolygon> *> (v->get ());
            const rdb::Value<db::DBox> *box_value = dynamic_cast <const rdb::Value<db::DBox> *> (v->get ());
            const rdb::Value<db::DEdge> *edge_value = dynamic_cast <const rdb::Value<db::DEdge> *> (v->get ());
            const rdb::Value<db::DEdgePair> *edge_pair_value = dynamic_cast <const rdb::Value<db::DEdgePair> *> (v->get ());
            const rdb::Value<db::DPath> *path_value = dynamic_cast <const rdb::Value<db::DPath> *> (v->get ());
            const rdb::Value<db::DText> *text_value = dynamic_cast <const rdb::Value<db::DText> *> (v->get ());

            if (polygon_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * polygon_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * polygon_value->value ().box ();

            } else if (edge_pair_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * edge_pair_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * db::DBox (edge_pair_value->value ().bbox ());

            } else if (edge_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * edge_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * db::DBox (edge_value->value ().bbox ());

            } else if (box_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * box_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * box_value->value ();

            } else if (text_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * text_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * text_value->value ().box ();

            } else if (path_value) {

              mp_markers.push_back (new lay::DMarker (mp_view));
              mp_markers.back ()->set (trans * path_value->value ());
              mp_markers.back ()->set_dismissable (true);
              m_markers_bbox += trans * path_value->value ().box ();

            }

          }

        }

      }

      for (std::vector<lay::DMarker *>::const_iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
        (*m)->set_color (m_marker_color);
        (*m)->set_line_width (m_marker_line_width);
        (*m)->set_vertex_size (m_marker_vertex_size);
        (*m)->set_halo (m_marker_halo);
        (*m)->set_dither_pattern (m_marker_dither_pattern);
      }

    }

    //  Produce a marker label info text ..

    std::string marker_info_text;
    if (n_item == 1) {
      marker_info_text = tl::sprintf ("%d / %d", item_index + 1, m_num_items);
    }
    markers_label->setText (tl::to_qstring (marker_info_text));

    //  Reposition the window ..

    if (mp_view && ! m_markers_bbox.empty ()) {

      double wdim = m_window_dim.get (m_markers_bbox);

      if (m_window == FitCell) {
        mp_view->zoom_fit ();
      } else if (m_window == FitMarker) {
        mp_view->zoom_box (m_markers_bbox.enlarged (db::DVector (wdim, wdim)));
      } else if (m_window == Center) {
        mp_view->pan_center (m_markers_bbox.p1 () + (m_markers_bbox.p2 () - m_markers_bbox.p1 ()) * 0.5);
      } else if (m_window == CenterSize) {
        double w = std::max (m_markers_bbox.width (), wdim);
        double h = std::max (m_markers_bbox.height (), wdim);
        db::DPoint center (m_markers_bbox.p1 () + (m_markers_bbox.p2 () - m_markers_bbox.p1 ()) * 0.5);
        db::DVector d (w * 0.5, h * 0.5);
        mp_view->zoom_box (db::DBox (center - d, center + d));
      }

    }

    //  Set the visited flag on the current item
    const rdb::Item *current_item = list_model->item (markers_list->selectionModel ()->currentIndex ().row ());
    if (current_item && ! current_item->visited ()) {

      mp_database->set_item_visited (current_item, true);

      list_model->mark_data_changed ();

      MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
      if (tree_model) {
        tree_model->mark_data_changed ();
      }

    }
  }
}

void 
MarkerBrowserPage::markers_current_changed (const QModelIndex & /*current*/, const QModelIndex & /*previous*/)
{
  //  The current item changed event cannot be used for setting the visited flag, since it appears to 
  //  occur too often - i.e. when the widgets gets the focus.
}

void
MarkerBrowserPage::release_markers ()
{
  for (std::vector<lay::DMarker *>::iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }
  mp_markers.clear ();
}

void 
MarkerBrowserPage::directory_selection_changed (const QItemSelection &, const QItemSelection &)
{
  if (! m_in_directory_selection_change) {
    update_marker_list (2 /*select all*/);
  } else {
    update_marker_list (0 /*select none*/);
  }
}

static void collect_items_of_category (const rdb::Database *rdb, rdb::id_type cat_id, const QString &cat_f, std::vector< std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> > &be_vector) 
{
  QString cat_f_sub;
  const Category *cat = rdb->category_by_id (cat_id);
  
  if (cat_matches_filter (cat, cat_f, false /*locally*/)) {
    be_vector.push_back (rdb->items_by_category (cat_id));
  } else {
    //  inherit filter for sub-categories
    cat_f_sub = cat_f;
  }

  for (rdb::Categories::const_iterator subcat = cat->sub_categories ().begin (); subcat != cat->sub_categories ().end (); ++subcat) {
    collect_items_of_category (rdb, subcat->id (), cat_f_sub, be_vector);
  }
}

static void collect_items_of_cell_and_category (const rdb::Database *rdb, rdb::id_type cell_id, rdb::id_type cat_id, const QString &cat_f, std::vector< std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> > &be_vector) 
{
  QString cat_f_sub;
  const Category *cat = rdb->category_by_id (cat_id);

  if (cat_matches_filter (cat, cat_f, false /*locally*/)) {
    be_vector.push_back (rdb->items_by_cell_and_category (cell_id, cat_id));
  } else {
    //  inherit filter for sub-categories
    cat_f_sub = cat_f;
  }

  for (rdb::Categories::const_iterator subcat = cat->sub_categories ().begin (); subcat != cat->sub_categories ().end (); ++subcat) {
    collect_items_of_cell_and_category (rdb, cell_id, subcat->id (), cat_f_sub, be_vector);
  }
}

void 
MarkerBrowserPage::update_marker_list (int /*selection_mode*/)
{
  if (! mp_database) {
    return;
  }

  //  ensure that the directory tree is initialized (that was reason for 0.20.2)
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (! tree_model) {
    return;
  }

  std::vector< std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> > be_vector;
  std::vector< std::pair<rdb::Database::const_item_iterator, rdb::Database::const_item_iterator> > be_vector_all;
  m_num_items = 0;

  QModelIndexList selected = directory_tree->selectionModel ()->selectedIndexes ();
  std::set<QModelIndex> selected_set;
  selected_set.insert (selected.begin (), selected.end ());

  QString cat_f = cat_filter->text ();
  QString cell_f = cell_filter->text ();

  for (QModelIndexList::const_iterator s = selected.begin (); s != selected.end (); ++s) {

    QModelIndex selected_item = *s;

    if (selected_item.column () != 0) {
      continue;
    }

    //  ignore selected items whose parent is selected too - the parent will include the selections output.
    if (selected_set.find (tree_model->parent (selected_item)) != selected_set.end ()) {
      continue;
    }
    
    const rdb::Cell *cell = 0;
    for (MarkerBrowserTreeViewModelCacheEntry *entry = (MarkerBrowserTreeViewModelCacheEntry *) selected_item.internalPointer (); entry && !cell; entry = entry->parent ()) {
      cell = mp_database->cell_by_id (entry->id ());
    }

    const rdb::Category *cat = 0;
    for (MarkerBrowserTreeViewModelCacheEntry *entry = (MarkerBrowserTreeViewModelCacheEntry *) selected_item.internalPointer (); entry && !cat; entry = entry->parent ()) {
      cat = mp_database->category_by_id (entry->id ());
    }
     
    if (cell == 0 && cat == 0) {

      be_vector.clear ();
      be_vector_all.clear ();

      be_vector_all.push_back (std::make_pair (mp_database->items ().begin (), mp_database->items ().end ()));

      m_num_items = mp_database->num_items ();

    } else if (be_vector_all.empty ()) {

      if (cell != 0 && cat == 0 && cat_f.isEmpty ()) {

        if (cell_f.isEmpty () || cell_matches_filter (cell, cell_f)) {
          be_vector.push_back (mp_database->items_by_cell (cell->id ()));
        }

      } else if (cell != 0 && cat == 0) {

        if (cell_f.isEmpty () || cell_matches_filter (cell, cell_f)) {
          for (rdb::Categories::const_iterator x = mp_database->categories ().begin (); x != mp_database->categories ().end (); ++x) {
            collect_items_of_cell_and_category (mp_database, cell->id (), x->id (), cat_f, be_vector);
          }
        }

      } else if (cell == 0 && cat != 0 && cell_f.isEmpty ()) {

        collect_items_of_category (mp_database, cat->id (), cat_f, be_vector);

      } else if (cell == 0 && cat != 0) {

        for (rdb::Database::const_cell_iterator c = mp_database->cells ().begin (); c != mp_database->cells ().end (); ++c) {
          if (cell_f.isEmpty () || cell_matches_filter (c.operator-> (), cell_f)) {
            collect_items_of_cell_and_category (mp_database, c->id (), cat->id (), cat_f, be_vector);
          }
        }

      } else {

        if (cell_f.isEmpty () || cell_matches_filter (cell, cell_f)) {
          collect_items_of_cell_and_category (mp_database, cell->id (), cat->id (), cat_f, be_vector);
        }

      }

      for (std::vector< std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> >::const_iterator be = be_vector.begin (); be != be_vector.end (); ++be) {
        for (rdb::Database::const_item_ref_iterator i = be->first; i != be->second; ++i) {
          ++m_num_items;
        }
      }

    }

  }

  //  in case of given filter, the "all" categories are reduced to the filtered ones
  if (! be_vector_all.empty () && (! cat_f.isEmpty () || ! cell_f.isEmpty ())) {

    be_vector_all.clear ();

    if (cat_f.isEmpty ()) {

      //  filter by cell
      for (rdb::Database::const_cell_iterator c = mp_database->cells ().begin (); c != mp_database->cells ().end (); ++c) {
        if (cell_matches_filter (c.operator-> (), cell_f)) {
          be_vector.push_back (mp_database->items_by_cell (c->id ()));
        }
      }

    } else if (cell_f.isEmpty ()) {

      //  filter by category
      for (rdb::Categories::const_iterator c = mp_database->categories ().begin (); c != mp_database->categories ().end (); ++c) {
        collect_items_of_category (mp_database, c->id (), cat_f, be_vector);
      }

    } else {

      //  filter by cell and category
      for (rdb::Database::const_cell_iterator c = mp_database->cells ().begin (); c != mp_database->cells ().end (); ++c) {
        if (cell_matches_filter (c.operator-> (), cell_f)) {
          for (rdb::Categories::const_iterator x = mp_database->categories ().begin (); x != mp_database->categories ().end (); ++x) {
            collect_items_of_cell_and_category (mp_database, c->id (), x->id (), cat_f, be_vector);
          }
        }
      }

    }

    //  recompute the number of filtered items
    m_num_items = 0;
    for (std::vector< std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> >::const_iterator be = be_vector.begin (); be != be_vector.end (); ++be) {
      for (rdb::Database::const_item_ref_iterator i = be->first; i != be->second; ++i) {
        ++m_num_items;
      }
    }

  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (list_model) {

    bool clipped = false;
    if (! be_vector_all.empty ()) {
      clipped = list_model->set_items (be_vector_all, m_max_marker_count);
    } else {
      clipped = list_model->set_items (be_vector, m_max_marker_count);
    }

    warn_label->setVisible (clipped);

    if (m_num_items > 0) {
#if 0
      if (selection_mode == 0) {
        //  select none
        markers_list->selectionModel ()->setCurrentIndex (QModelIndex (), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
      } else if (selection_mode == 1 || m_num_items == 1) {
        //  set the current to the first entry
        markers_list->selectionModel ()->setCurrentIndex (list_model->index_of_row (0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
      } else {
        //  select all
        markers_list->selectAll ();
      }
#else
      if (m_num_items == 1) {
        //  set the current to the first entry
        markers_list->selectionModel ()->setCurrentIndex (list_model->index_of_row (0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
      } else {
        //  select all
        markers_list->selectAll ();
      }
#endif
    } else {
      update_markers ();
      update_info_text ();
    }

  }
}

bool 
MarkerBrowserPage::adv_tree (bool up)
{
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (! tree_model) {
    return false;
  }

  QModelIndex index = directory_tree->selectionModel ()->currentIndex ();
  while (index.isValid ()) {
    index = tree_model->next_index (index, up);
    if (index.isValid () && ! directory_tree->isRowHidden (index.row (), tree_model->parent (index))) {
      break;
    }
  }

  if (index.isValid ()) {
    directory_tree->selectionModel ()->setCurrentIndex (index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    return true;
  } else {
    return false;
  }

}

bool 
MarkerBrowserPage::adv_list (bool up)
{
  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return false;
  }

  bool ret = false;

  //  don't consider selection changed events since they might interfere with the markers list that we deal with currently.
  m_in_directory_selection_change = true;

  //  for an empty list forward the advance request to the tree
  QModelIndex index = markers_list->selectionModel ()->currentIndex ();
  if (index.isValid ()) {
    index = list_model->index (index.row () + (up ? -1 : 1), index.column (), QModelIndex ());
  }

  if (! index.isValid () && adv_tree (up)) {
    index = list_model->index (up ? (list_model->rowCount (QModelIndex ()) - 1) : 0, 0, QModelIndex ());
  }

  if (index.isValid ()) {
    markers_list->selectionModel ()->setCurrentIndex (index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    ret = true;
  }

  m_in_directory_selection_change = false;

  return ret;
}

void 
MarkerBrowserPage::dir_up_clicked ()
{
  adv_tree (true);
}

void 
MarkerBrowserPage::dir_down_clicked ()
{
  adv_tree (false);
}

void 
MarkerBrowserPage::list_up_clicked ()
{
  adv_list (true);
}

void 
MarkerBrowserPage::list_down_clicked ()
{
  adv_list (false);
}

void 
MarkerBrowserPage::flag_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type flag_tag_ids [sizeof (flag_descriptors) / sizeof (flag_descriptors [0])];
  flag_tag_ids [0] = 0;
  for (unsigned int j = 1; j < sizeof (flag_descriptors) / sizeof (flag_descriptors [0]); ++j) {
    flag_tag_ids [j] = mp_database->tags ().tag (flag_descriptors [j].tag).id ();
  }

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        for (unsigned int j = 1; j < sizeof (flag_descriptors) / sizeof (flag_descriptors [0]); ++j) {
          mp_database->remove_item_tag (i, flag_tag_ids [j]);
        }
        if (m_current_flag > 0) {
          mp_database->add_item_tag (i, flag_tag_ids [m_current_flag]);
        }
      }
    }
  }

  list_model->mark_data_changed ();
}

void
MarkerBrowserPage::rerun_button_pressed ()
{
  //  NOTE: we use deferred execution, because otherwise the button won't get repainted properly
  dm_rerun_macro ();
}

void
MarkerBrowserPage::rerun_macro ()
{
BEGIN_PROTECTED

  if (! mp_database->generator ().empty ()) {

    std::map<std::string, tl::Variant> add_pars;

    for (unsigned int i = 0; i < mp_view->num_rdbs (); ++i) {
      if (mp_view->get_rdb (i) == mp_database) {
        add_pars["rdb_index"] = tl::Variant (int (i));
        break;
      }
    }

    tl::Recipe::make (mp_database->generator (), add_pars);

  }

END_PROTECTED
}

void
MarkerBrowserPage::flag_menu_selected ()
{
  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {
    int flag_index = action->data ().toInt ();
    if (flag_index >= 0 && flag_index < int (sizeof (flag_descriptors) / sizeof (flag_descriptors [0]))) {
      flags_pb->setIcon (QIcon (tl::to_qstring (flag_descriptors [flag_index].icon)));
      m_current_flag = flag_index;
      flag_button_clicked ();
    }
  }
}

void
MarkerBrowserPage::edit_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  std::string str;

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (! i->comment ().empty ()) {
        if (str.empty ()) {
          str = i->comment ();
        } else if (str != i->comment ()) {
          str.clear ();
          break;
        }
      }
    }
  }

  bool ok = false;

#if QT_VERSION >= 0x50200
  QString new_text = QInputDialog::getMultiLineText (this, QObject::tr ("Edit Marker Comment"), QObject::tr ("Comment"), tl::to_qstring (str), &ok);
  str = tl::to_string (new_text);
#else
  QString new_text = QInputDialog::getText (this, QObject::tr ("Edit Marker Comment"), QObject::tr ("Comment"), QLineEdit::Normal, tl::to_qstring (tl::escape_string (str)), &ok);
  str = tl::unescape_string (tl::to_string (new_text));
#endif

  if (ok) {

    QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
    for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
      if (selected_item->column () == 0) {
        const rdb::Item *i = list_model->item (selected_item->row ());
        mp_database->set_item_comment (i, str);
      }
    }

    update_info_text ();

  }
}

void
MarkerBrowserPage::waived_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type waived_tag_id = mp_database->tags ().tag ("waived").id ();

  size_t nyes = 0;
  size_t nno = 0;

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        if (i->has_tag (waived_tag_id)) {
          ++nyes;
        } else {
          ++nno;
        }
      }
    }
  }

  if (nyes < nno) {
    waive ();
  } else {
    unwaive ();
  }
}

void  
MarkerBrowserPage::important_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type important_tag_id = mp_database->tags ().tag ("important").id ();

  size_t nyes = 0;
  size_t nno = 0;

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        if (i->has_tag (important_tag_id)) {
          ++nyes;
        } else {
          ++nno;
        }
      }
    }
  }

  if (nyes < nno) {
    mark_important ();
  } else {
    mark_unimportant ();
  }
}

void  
MarkerBrowserPage::remove_snapshot_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  QMessageBox msgbox (QMessageBox::Question, 
                      QObject::tr ("Remove All Snapshots"),
                      QObject::tr ("Are you sure to remove the snapshot from all markers?"),
                      QMessageBox::Yes | QMessageBox::No);
  if (msgbox.exec () == QMessageBox::Yes) {

    QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
    for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
      if (selected_item->column () == 0) {
        const rdb::Item *i = list_model->item (selected_item->row ());
        if (i) {
          mp_database->set_item_image (i, QImage ());
        }
      }
    }

    update_info_text ();

  }
}

void  
MarkerBrowserPage::snapshot_button_clicked ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {

    const rdb::Item *i = list_model->item (selected_item->row ());
    if (i) {

      mp_database->set_item_image (i, QImage (mp_view->get_screenshot ()));
      markers_list->selectionModel ()->setCurrentIndex (*selected_item, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
      update_info_text ();

      // Currently, don't add the snapshot to all selected items - this would create some overhead since snapshots are not shared currently.
      break;

    }

  }
}

void
MarkerBrowserPage::show_all_clicked ()
{
  if (mp_plugin_root) {
    mp_plugin_root->config_set (cfg_rdb_show_all, tl::to_string (m_show_all_action->isChecked ()));
  }
}

void
MarkerBrowserPage::list_shapes_clicked ()
{
  if (mp_plugin_root) {
    mp_plugin_root->config_set (cfg_rdb_list_shapes, tl::to_string (list_shapes_cb->isChecked ()));
  }
}

void
MarkerBrowserPage::unwaive_all ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (! tree_model) {
    return;
  }

  QMessageBox msgbox (QMessageBox::Question,
                      QObject::tr ("Remove All Waived"),
                      QObject::tr ("Are you sure to remove the waived flags from all markers?"),
                      QMessageBox::Yes | QMessageBox::No);
  if (msgbox.exec () == QMessageBox::Yes) {

    id_type waived_tag_id = mp_database->tags ().tag ("waived").id ();

    for (Items::const_iterator i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
      if (i->has_tag (waived_tag_id)) {
        mp_database->remove_item_tag (i.operator-> (), waived_tag_id);
        tree_model->waived_changed (i.operator-> (), false);
      }
    }

    list_model->mark_data_changed ();

  }
}

void  
MarkerBrowserPage::revisit_all ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  for (Items::const_iterator i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
    mp_database->set_item_visited (i.operator-> (), false);
  }

  list_model->mark_data_changed ();

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    tree_model->mark_data_changed ();
  }
}

void  
MarkerBrowserPage::revisit_non_waived ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type waived_tag_id = mp_database->tags ().tag ("waived").id ();

  for (Items::const_iterator i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
    if (! i->has_tag (waived_tag_id)) {
      mp_database->set_item_visited (i.operator-> (), false);
    }
  }

  list_model->mark_data_changed ();

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    tree_model->mark_data_changed ();
  }
}

void  
MarkerBrowserPage::revisit_important ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type important_tag_id = mp_database->tags ().tag ("important").id ();

  for (Items::const_iterator i = mp_database->items ().begin (); i != mp_database->items ().end (); ++i) {
    if (i->has_tag (important_tag_id)) {
      mp_database->set_item_visited (i.operator-> (), false);
    }
  }

  list_model->mark_data_changed ();

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    tree_model->mark_data_changed ();
  }
}

void  
MarkerBrowserPage::mark_important ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type important_tag_id = mp_database->tags ().tag ("important").id ();

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        mp_database->add_item_tag (i, important_tag_id);
      }
    }
  }

  list_model->mark_data_changed ();
}

void  
MarkerBrowserPage::mark_unimportant ()
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  id_type important_tag_id = mp_database->tags ().tag ("important").id ();

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        mp_database->remove_item_tag (i, important_tag_id);
      }
    }
  }

  list_model->mark_data_changed ();
}

void  
MarkerBrowserPage::mark_visited ()
{
  mark_visited (true);
}

void  
MarkerBrowserPage::mark_notvisited ()
{
  mark_visited (false);
}

void  
MarkerBrowserPage::mark_visited (bool f)
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        mp_database->set_item_visited (i, f);
      }
    }
  }

  list_model->mark_data_changed ();

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    tree_model->mark_data_changed ();
  }
}

void  
MarkerBrowserPage::waive ()
{
  waive_or_unwaive (true);
}

void
MarkerBrowserPage::unwaive ()
{
  waive_or_unwaive (false);
}

void
MarkerBrowserPage::waive_or_unwaive (bool w)
{
  if (! mp_database) {
    return;
  }

  MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
  if (! list_model) {
    return;
  }

  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (! tree_model) {
    return;
  }

  id_type waived_tag_id = mp_database->tags ().tag ("waived").id ();

  QModelIndexList selected = markers_list->selectionModel ()->selectedIndexes ();
  for (auto selected_item = selected.begin (); selected_item != selected.end (); ++selected_item) {
    if (selected_item->column () == 0) {
      const rdb::Item *i = list_model->item (selected_item->row ());
      if (i) {
        bool was_waived = i->has_tag (waived_tag_id);
        if (w != was_waived) {
          if (w) {
            mp_database->add_item_tag (i, waived_tag_id);
          } else {
            mp_database->remove_item_tag (i, waived_tag_id);
          }
          if (w) {
            //  waiving an item makes it visited (rationale: once waived, an item is no
            //  longer of interest)
            mp_database->set_item_visited (i, true);
          }
          tree_model->waived_changed (i, w);
        }
      }
    }
  }

  list_model->mark_data_changed ();
  tree_model->mark_data_changed ();
}

void
MarkerBrowserPage::info_anchor_clicked (const QUrl &link)
{
  if (link.isRelative () && link.path () == QString::fromUtf8 ("show-snapshot")) {

    if (! mp_database) {
      return;
    }

    MarkerBrowserListViewModel *list_model = dynamic_cast<MarkerBrowserListViewModel *> (markers_list->model ());
    if (! list_model) {
      return;
    }

    QModelIndex current = markers_list->selectionModel ()->currentIndex ();
    const rdb::Item *i = list_model->item (current.row ());
    if (i && i->has_image ()) {
      MarkerBrowserSnapshotView *snapshot_view = new MarkerBrowserSnapshotView (this, i->image ());
      snapshot_view->exec ();
      delete snapshot_view;
#if QT_VERSION < 0x040300
      update_info_text ();
#endif
    }

  } 
}

bool 
MarkerBrowserPage::eventFilter (QObject *watched, QEvent *event)
{
  if (event->type () == QEvent::KeyPress) {

    QKeyEvent *ke = dynamic_cast <QKeyEvent *> (event);
    if (ke && (ke->key () == Qt::Key_Up || ke->key () == Qt::Key_Down)) {

      bool up = ke->key () == Qt::Key_Up;

      if (watched == directory_tree) {
        adv_tree (up);
      } else if (watched == markers_list) {
        adv_list (up);
      }

      return true;

    }

  } 

  return QFrame::eventFilter (watched, event);
}

}

#endif
