
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

#include "layCellTreeModel.h"
#include "layLayoutViewBase.h"
#include "layDragDropData.h"
#include "tlGlobPattern.h"
#include "dbPCellHeader.h"
#include "dbPCellVariant.h"
#include "dbLibraryProxy.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"

#include <QTreeView>
#include <QPalette>
#include <QMimeData>

#include <string.h>

#include <algorithm>

namespace lay {

// --------------------------------------------------------------------
//  A compare functor for the cell tree items by area

struct cmp_cell_tree_items_f 
{
  cmp_cell_tree_items_f (CellTreeModel::Sorting s)
    : m_sorting (s)
  { }

  bool operator() (const CellTreeItem *a, const CellTreeItem *b)
  {
    if (m_sorting == CellTreeModel::ByArea) {
      if (a->by_area_equal_than (b)) {
        return a->by_name_less_than (b);
      } else {
        return a->by_area_less_than (b);
      }
    } else if (m_sorting == CellTreeModel::ByAreaReverse) {
      if (a->by_area_equal_than (b)) {
        return a->by_name_less_than (b);
      } else {
        return b->by_area_less_than (a);
      }
    } else {
      return a->by_name_less_than (b);
    }
  }

private:
  CellTreeModel::Sorting m_sorting;
};

// --------------------------------------------------------------------
//  A compare functor for the cell tree items vs. name

struct cmp_cell_tree_item_vs_name_f 
{
  bool operator() (const CellTreeItem *a, const char *name)
  {
    return a->name_less_than (name);
  }
};

// --------------------------------------------------------------------
//  CellTreeItem implementation

CellTreeItem::CellTreeItem (const db::Layout *layout, bool is_pcell, unsigned int cell_or_pcell_index, bool flat, CellTreeModel::Sorting s)
  : mp_layout (layout), mp_parent (0), m_sorting (s), m_is_pcell (is_pcell),
    m_index (0), m_tree_index (0),
    m_children (), m_cell_or_pcell_index (cell_or_pcell_index)
{
  if (! flat && ! is_pcell) {
    m_child_count = int (mp_layout->cell (db::cell_index_type (cell_or_pcell_index)).child_cells ());
  } else {
    m_child_count = 0;
  }
}

CellTreeItem::~CellTreeItem ()
{
  for (std::vector<CellTreeItem *>::iterator c = m_children.begin (); c != m_children.end (); ++c) {
    delete *c;
  }
  m_children.clear ();
}

size_t
CellTreeItem::assign_serial (size_t index, std::map<CellTreeItem *, size_t> &serial)
{
  serial.insert (std::make_pair (this, index++));
  for (std::vector<CellTreeItem *>::iterator c = m_children.begin (); c != m_children.end (); ++c) {
    index = (*c)->assign_serial (index, serial);
  }
  return index;
}

bool
CellTreeItem::is_valid () const
{
  return m_is_pcell || mp_layout->is_valid_cell_index (cell_or_pcell_index ());
}

std::string 
CellTreeItem::display_text () const
{
  if (m_is_pcell) {
    return name ();
  } else if (mp_layout->is_valid_cell_index (cell_or_pcell_index ())) {
    return mp_layout->cell (cell_or_pcell_index ()).get_display_name ();
  } else {
    return std::string ();
  }
}

int 
CellTreeItem::children () const
{
  return m_child_count;
}

int
CellTreeItem::children_in (const std::set<const CellTreeItem *> &sel) const
{
  int count = 0;
  for (std::vector<CellTreeItem *>::const_iterator c = m_children.begin (); c != m_children.end (); ++c) {
    if (sel.find (*c) != sel.end ()) {
      ++count;
    }
  }
  return count;
}

void
CellTreeItem::ensure_children ()
{
  if (! m_is_pcell && m_children.empty ()) {

    //  create a list of child sub-item

    const db::Cell *cell = & mp_layout->cell (cell_or_pcell_index ());

    m_children.reserve (m_child_count);

    for (db::Cell::child_cell_iterator child = cell->begin_child_cells (); ! child.at_end (); ++child) {
      add_child (new CellTreeItem (mp_layout, false, *child, false, m_sorting));
    }

    finish_children ();

  }
}

CellTreeItem *
CellTreeItem::child (int index) 
{
  ensure_children ();
  return m_children [index];
}

CellTreeItem *
CellTreeItem::child_in (const std::set<const CellTreeItem *> &sel, int index)
{
  ensure_children ();

  for (std::vector<CellTreeItem *>::const_iterator c = m_children.begin (); c != m_children.end (); ++c) {
    if (sel.find (*c) != sel.end () && index-- <= 0) {
      return *c;
    }
  }

  return 0;
}

void
CellTreeItem::add_child (CellTreeItem *item)
{
  //  explicitly added
  if (size_t (m_child_count) == m_children.size ()) {
    ++m_child_count;
  }

  item->mp_parent = this;
  m_children.push_back (item);
}

void
CellTreeItem::finish_children ()
{
  std::sort (m_children.begin (), m_children.end (), cmp_cell_tree_items_f (m_sorting));

  for (size_t i = 0; i < m_children.size (); ++i) {
    m_children [i]->set_index (i);
  }
}

db::cell_index_type
CellTreeItem::cell_or_pcell_index () const
{
  return db::cell_index_type (m_cell_or_pcell_index);
}

CellTreeItem *
CellTreeItem::parent () const
{
  return mp_parent;
}

const char *
CellTreeItem::name () const
{
  if (! m_is_pcell) {
    return mp_layout->cell_name (cell_or_pcell_index ());
  } else {
    return mp_layout->pcell_header (m_cell_or_pcell_index)->get_name ().c_str ();
  }
}

bool 
CellTreeItem::by_name_less_than (const CellTreeItem *b) const
{
#if 0 // with name:
  return strcmp (name (), b->name ()) < 0;
#else // with display text:
  return display_text () < b->display_text ();
#endif
}

bool 
CellTreeItem::name_less_than (const char *n) const
{
#if 0 // with name:
  return strcmp (name (), n) < 0;
#else // with display text:
  return display_text () < n;
#endif
}

bool 
CellTreeItem::name_equals (const char *n) const
{
#if 0 // with name:
  return strcmp (name (), n) == 0;
#else // with display text:
  return display_text () == n;
#endif
}

bool 
CellTreeItem::name_matches (const tl::GlobPattern &p) const
{
#if 0 // with name:
  return p.match (name ());
#else // with display text:
  return p.match (display_text ());
#endif
}

bool 
CellTreeItem::by_area_less_than (const CellTreeItem *b) const
{
  if (m_is_pcell || b->is_pcell ()) {
    return m_is_pcell > b->is_pcell ();
  }
  // Hint: since mp_layout == b.mp_layout, not conversion to um^2 is required because of different DBU
  return mp_layout->cell (cell_or_pcell_index ()).bbox ().area () < b->mp_layout->cell (b->cell_or_pcell_index ()).bbox ().area ();
}

bool
CellTreeItem::by_area_equal_than (const CellTreeItem *b) const
{
  if (m_is_pcell != b->is_pcell ()) {
    return false;
  }
  // Hint: since mp_layout == b.mp_layout, not conversion to um^2 is required because of different DBU
  return mp_layout->cell (cell_or_pcell_index ()).bbox ().area () == b->mp_layout->cell (b->cell_or_pcell_index ()).bbox ().area ();
}

// --------------------------------------------------------------------
//  CellTreeModel implementation
//  Hint: it may happen that the cell tree model gets engaged while the layout is not
//  valid ("under construction"). In this case, the model will return defaults or void
//  objects.

CellTreeModel::CellTreeModel (QWidget *parent, lay::LayoutViewBase *view, int cv_index, unsigned int flags, const db::Cell *base, Sorting sorting)
  : QAbstractItemModel (parent), 
    m_flags (flags),
    m_sorting (sorting),
    mp_parent (parent), 
    mp_view (view), 
    m_cv_index (cv_index),
    mp_base (base)
{
  mp_view->cell_visibility_changed_event.add (this, &CellTreeModel::signal_data_changed);
  mp_view->cellview_changed_event.add (this, &CellTreeModel::signal_data_changed_with_int);

  m_flat = ((flags & Flat) != 0) && ((flags & TopCells) == 0);
  m_pad = ((flags & NoPadding) == 0);
  m_filter_mode = false;
  m_is_filtered = false;

  mp_layout = & view->cellview (cv_index)->layout ();
  mp_library = 0;
  tl_assert (! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ()));

  build_top_level ();

  m_current_index = m_selected_indexes.begin ();
}

CellTreeModel::CellTreeModel (QWidget *parent, db::Layout *layout, unsigned int flags, const db::Cell *base, Sorting sorting)
  : QAbstractItemModel (parent), 
    m_flags (flags),
    m_sorting (sorting),
    mp_parent (parent), 
    mp_view (0), 
    m_cv_index (-1),
    mp_base (base)
{
  m_flat = ((flags & Flat) != 0) && ((flags & TopCells) == 0);
  m_pad = ((flags & NoPadding) == 0);
  m_filter_mode = false;

  mp_layout = layout;
  mp_library = 0;
  tl_assert (! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ()));

  build_top_level ();

  m_current_index = m_selected_indexes.begin ();
}

CellTreeModel::CellTreeModel (QWidget *parent, db::Library *library, unsigned int flags, const db::Cell *base, Sorting sorting)
  : QAbstractItemModel (parent),
    m_flags (flags),
    m_sorting (sorting),
    mp_parent (parent),
    mp_view (0),
    m_cv_index (-1),
    mp_base (base)
{
  m_flat = ((flags & Flat) != 0) && ((flags & TopCells) == 0);
  m_pad = ((flags & NoPadding) == 0);
  m_filter_mode = false;

  mp_layout = &library->layout ();
  mp_library = library;
  tl_assert (! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ()));

  build_top_level ();

  m_current_index = m_selected_indexes.begin ();
}

CellTreeModel::~CellTreeModel ()
{
  clear_top_level ();
}

void
CellTreeModel::configure (lay::LayoutViewBase *view, int cv_index, unsigned int flags, const db::Cell *base, Sorting sorting)
{
  db::Layout *layout = & view->cellview (cv_index)->layout ();
  do_configure (layout, 0, view, cv_index, flags, base, sorting);
}

void
CellTreeModel::configure (db::Layout *layout, unsigned int flags, const db::Cell *base, Sorting sorting)
{
  do_configure (layout, 0, 0, -1, flags, base, sorting);
}

void
CellTreeModel::configure (db::Library *library, unsigned int flags, const db::Cell *base, Sorting sorting)
{
  do_configure (& library->layout (), library, 0, -1, flags, base, sorting);
}

void
CellTreeModel::do_configure (db::Layout *layout, db::Library *library, lay::LayoutViewBase *view, int cv_index, unsigned int flags, const db::Cell *base, Sorting sorting)
{
  bool flat = ((flags & Flat) != 0) && ((flags & TopCells) == 0);

  bool need_reset = false;
  if (flat != m_flat || layout != mp_layout || view != mp_view) {

    need_reset = true;
    beginResetModel ();

  }

  std::vector<lay::CellTreeItem *> old_toplevel_items;
  old_toplevel_items.swap (m_toplevel);

  if (view != mp_view) {

    if (mp_view) {
      mp_view->cell_visibility_changed_event.remove (this, &CellTreeModel::signal_data_changed);
      mp_view->cellview_changed_event.remove (this, &CellTreeModel::signal_data_changed_with_int);
    }

    mp_view = view;

    if (mp_view) {
      mp_view->cell_visibility_changed_event.add (this, &CellTreeModel::signal_data_changed);
      mp_view->cellview_changed_event.add (this, &CellTreeModel::signal_data_changed_with_int);
    }

  }

  m_cv_index = cv_index;
  m_flags = flags;
  mp_base = base;
  m_selected_indexes.clear ();
  m_current_index = m_selected_indexes.begin ();

  m_sorting = sorting;
  m_flat = flat;
  m_pad = ((flags & NoPadding) == 0);

  mp_layout = layout;
  mp_library = library;
  tl_assert (! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ()));

  build_top_level ();

  if (need_reset) {

    endResetModel ();

  } else {

    emit layoutAboutToBeChanged ();

    //  Translate persistent indexes: translation happens according to the path given by
    //  a sequence of cell indexes.

    QModelIndexList indexes = persistentIndexList ();
    QModelIndexList new_indexes;

    for (QModelIndexList::const_iterator index = indexes.begin (); index != indexes.end (); ++index) {

      std::vector<std::pair<bool, db::cell_index_type> > path;
      CellTreeItem *item = (CellTreeItem *) index->internalPointer ();
      while (item) {
        path.push_back (std::make_pair (item->is_pcell (), item->cell_or_pcell_index ()));
        item = item->parent ();
      }

      CellTreeItem *parent = 0;
      int row = index->row ();

      if (! path.empty ()) {

        //  because we push_back'd on our way up:
        std::reverse (path.begin (), path.end ());

        for (std::vector<std::pair<bool, db::cell_index_type> >::const_iterator ci = path.begin (); ci != path.end (); ++ci) {

          CellTreeItem *new_parent = 0;

          if ((! ci->first && ! layout->is_valid_cell_index (ci->second)) || (ci->first && ! layout->pcell_declaration (ci->second))) {
            //  can't translate this index
          } else if (parent == 0) {
            for (int i = 0; i < int (m_toplevel.size ()) && !new_parent; ++i) {
              if (m_toplevel [i]->cell_or_pcell_index () == ci->second && m_toplevel [i]->is_pcell () == ci->first) {
                new_parent = m_toplevel [i];
                row = i;
              }
            }
          } else {
            for (int i = 0; i < parent->children () && !new_parent; ++i) {
              if (parent->child (i)->cell_or_pcell_index () == ci->second && parent->child (i)->is_pcell () == ci->first) {
                new_parent = parent->child (i);
                row = i;
              }
            }
          }

          parent = new_parent;

        }

      }

      if (parent) {
        new_indexes << createIndex (row, index->column (), (void *) parent);
      } else {
        new_indexes << QModelIndex ();
      }

    }

    changePersistentIndexList (indexes, new_indexes);

    emit layoutChanged ();

  }

  //  TODO: harden against exceptions
  for (std::vector<lay::CellTreeItem *>::iterator t = old_toplevel_items.begin (); t != old_toplevel_items.end (); ++t) {
    delete *t;
  }
}

void
CellTreeModel::set_filter_mode (bool f)
{
  if (f != m_filter_mode) {
    m_filter_mode = f;
    signal_data_changed ();
  }
}

void 
CellTreeModel::set_sorting (Sorting s)
{
  if (s != m_sorting) {
    do_configure (mp_layout, mp_library, mp_view, m_cv_index, m_flags, mp_base, s);
  }
}

void
CellTreeModel::signal_data_changed ()
{
  emit layoutAboutToBeChanged ();
  emit layoutChanged ();
}

void 
CellTreeModel::clear_top_level ()
{
  for (std::vector<CellTreeItem *>::iterator c = m_toplevel.begin (); c != m_toplevel.end (); ++c) {
    delete *c;
  }
  m_toplevel.clear ();
}

void 
CellTreeModel::build_top_level ()
{
  if ((m_flags & Children) != 0) {

    m_flat = true; //  no "hierarchical children" yet.

    if (mp_base) {
      m_toplevel.reserve (mp_base->child_cells ());
      for (db::Cell::child_cell_iterator child = mp_base->begin_child_cells (); ! child.at_end (); ++child) {
        if (name_selected (mp_layout->cell_name (*child))) {
          CellTreeItem *item = new CellTreeItem (mp_layout, false, *child, true, m_sorting);
          m_toplevel.push_back (item);
        }
      }
    }

  } else if ((m_flags & Parents) != 0) {

    m_flat = true; //  no "hierarchical parents" yet.

    if (mp_base) {
      m_toplevel.reserve (mp_base->parent_cells ());
      for (db::Cell::parent_cell_iterator parent = mp_base->begin_parent_cells (); parent != mp_base->end_parent_cells (); ++parent) {
        if (name_selected (mp_layout->cell_name (*parent))) {
          CellTreeItem *item = new CellTreeItem (mp_layout, false, *parent, true, m_sorting);
          m_toplevel.push_back (item);
        }
      }
    }

  } else {

    if (m_flat) {
      m_toplevel.reserve (mp_layout->cells ());
    }

    db::Layout::top_down_const_iterator top = mp_layout->begin_top_down ();
    while (top != mp_layout->end_top_down ()) {

      if (! name_selected (mp_layout->cell_name (*top))) {
        //  ignore cell
      } else if (m_flat) {
        if ((m_flags & BasicCells) == 0 || ! mp_layout->cell (*top).is_proxy ()) {
          CellTreeItem *item = new CellTreeItem (mp_layout, false, *top, true, m_sorting);
          m_toplevel.push_back (item);
        }
      } else if (mp_layout->cell (*top).is_top ()) {
        if ((m_flags & BasicCells) == 0 || ! mp_layout->cell (*top).is_proxy ()) {
          CellTreeItem *item = new CellTreeItem (mp_layout, false, *top, (m_flags & TopCells) != 0, m_sorting);
          m_toplevel.push_back (item);
        }
      } else {
        break;
      }

      ++top;

    }

    if ((m_flags & BasicCells) != 0) {

      for (db::Layout::pcell_iterator pc = mp_layout->begin_pcells (); pc != mp_layout->end_pcells (); ++pc) {

        const auto *pcell_decl = mp_layout->pcell_declaration (pc->second);
        if (name_selected (pcell_decl->name ())) {

          CellTreeItem *item = new CellTreeItem (mp_layout, true, pc->second, true, m_sorting);
          m_toplevel.push_back (item);

          if ((m_flags & WithVariants) != 0) {

            const auto *pcell_header = mp_layout->pcell_header (pc->second);

            for (db::PCellHeader::variant_iterator v = pcell_header->begin (); v != pcell_header->end (); ++v) {
              if (mp_library && mp_library->is_retired (v->second->cell_index ())) {
                //  skip retired cells - this means we won't show variants which are just kept
                //  as shadow variants for the transactions.
              } else {
                item->add_child (new CellTreeItem (mp_layout, false, v->second->cell_index (), true, m_sorting));
              }
            }

            item->finish_children ();

          }

        }

      }

    }

  }

  std::sort (m_toplevel.begin (), m_toplevel.end (), cmp_cell_tree_items_f (m_sorting));

  for (size_t i = 0; i < m_toplevel.size (); ++i) {
    m_toplevel [i]->set_index (i);
  }
}

bool
CellTreeModel::name_selected (const std::string &name) const
{
  return ((m_flags & HidePrivate) == 0 || (! name.empty () && *name.begin () != '_'));
}

Qt::ItemFlags 
CellTreeModel::flags (const QModelIndex &index) const
{
  return Qt::ItemIsDragEnabled | QAbstractItemModel::flags (index);
}

QStringList 
CellTreeModel::mimeTypes () const
{
  QStringList types;
  types << QString::fromUtf8 (lay::drag_drop_mime_type ());
  return types;
}

QMimeData *
CellTreeModel::mimeData(const QModelIndexList &indexes) const
{
  for (QModelIndexList::const_iterator i = indexes.begin (); i != indexes.end (); ++i) {

    if (! i->isValid()) {
      continue;
    }

    if (is_pcell (*i)) {

      lay::CellDragDropData data (mp_layout, mp_library, pcell_id (*i), true);
      return data.to_mime_data ();

    } else {

      const db::Cell *c = cell (*i);
      if (c) {

        //  resolve library proxies
        const db::Layout *layout = mp_layout;
        const db::Library *library = mp_library;

        const db::LibraryProxy *lib_proxy;
        while (layout != 0 && (lib_proxy = dynamic_cast <const db::LibraryProxy *> (c)) != 0) {

          const db::Library *lib = db::LibraryManager::instance ().lib (lib_proxy->lib_id ());
          if (! lib) {
            break;
          }

          library = lib;
          layout = &lib->layout ();

          if (layout->is_valid_cell_index (lib_proxy->library_cell_index ())) {
            c = &layout->cell (lib_proxy->library_cell_index ());
          } else {
            c = 0;
          }

        }

        //  identify pcell variants and turn them into PCell drag targets
        const db::PCellVariant *pcell_var = dynamic_cast<const db::PCellVariant *> (c);
        if (pcell_var) {
          lay::CellDragDropData data (layout, library, pcell_var->pcell_id (), true, pcell_var->parameters ());
          return data.to_mime_data ();
        } else if (c) {
          lay::CellDragDropData data (layout, library, c->cell_index (), false);
          return data.to_mime_data ();
        }

      }

    }

  }

  return 0;
}

int 
CellTreeModel::columnCount (const QModelIndex &) const 
{
  return 1;
}

QVariant 
CellTreeModel::data (const QModelIndex &index, int role) const 
{
  CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
  if (! item || mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QVariant ();
  }

  if (role == Qt::DisplayRole || role == Qt::EditRole) {

    if (m_pad) {
      return QVariant (tl::to_qstring (" " + item->display_text () + " "));
    } else {
      return QVariant (tl::to_qstring (item->display_text ()));
    }

  } else if (role == Qt::FontRole) {

    if (! mp_view) {

      return QVariant ();

    } else {

      QFont f (mp_parent->font ());

      const lay::CellView::unspecific_cell_path_type &path = mp_view->cellview (m_cv_index).unspecific_path ();
      const lay::CellView::specific_cell_path_type &ctx_path = mp_view->cellview (m_cv_index).specific_path ();

      if (! path.empty ()) {
        if (item->cell_or_pcell_index () == path.back ()) {
          if (m_flat) {
            f.setBold (true);
          } else {
            CellTreeItem *it = item;
            lay::CellView::unspecific_cell_path_type::const_iterator p = path.end ();
            while (it && p != path.begin ()) {
              --p;
              if (it->cell_or_pcell_index () != *p) {
                break;
              }
              it = it->parent ();
            }
            if (! it && p == path.begin ()) {
              f.setBold (true);
            }
          }
        } else if (! ctx_path.empty () && item->cell_or_pcell_index () == ctx_path.back ().inst_ptr.cell_index ()) {
          if (m_flat) {
            f.setUnderline (true);
          } else {
            CellTreeItem *it = item;
            lay::CellView::specific_cell_path_type::const_iterator cp = ctx_path.end ();
            while (it && cp != ctx_path.begin ()) {
              --cp;
              if (it->cell_or_pcell_index () != cp->inst_ptr.cell_index ()) {
                break;
              }
              it = it->parent ();
            }
            if (cp == ctx_path.begin ()) {
              lay::CellView::unspecific_cell_path_type::const_iterator p = path.end ();
              while (it && p != path.begin ()) {
                --p;
                if (it->cell_or_pcell_index () != *p) {
                  break;
                }
                it = it->parent ();
              }
              if (! it && p == path.begin ()) {
                f.setUnderline (true);
              }
            }
          }
        }
      }

      if (mp_view->is_cell_hidden (item->cell_or_pcell_index (), m_cv_index)) {
        f.setStrikeOut (true);
      }

      return QVariant (f);

    }

  } else if (role == Qt::BackgroundRole) {

    if (m_selected_indexes_set.find (index.internalPointer ()) != m_selected_indexes_set.end ()) {
      //  for selected items pick a color between Highlight and Base
      QPalette pl (mp_parent->palette ());
      QColor c1 = pl.color (QPalette::Highlight);
      QColor cb = pl.color (QPalette::Base);
      return QVariant (QColor ((c1.red () + cb.red ()) / 2, (c1.green () + cb.green ()) / 2, (c1.blue () + cb.blue ()) / 2));
    } else {
      return QVariant ();
    }

  } else if (role == Qt::ForegroundRole) {

#if 0 // do strikeout rather than making the color darker
    if (! mp_view) {
      return QVariant ();
    } else {
      QPalette pl (mp_parent->palette ());
      if (mp_view->is_cell_hidden (item->cell_index (), m_cv_index)) {
        QColor c1 = pl.color (QPalette::Text);
        QColor cb = pl.color (QPalette::Base);
        return QVariant (QColor ((c1.red () + cb.red ()) / 2, (c1.green () + cb.green ()) / 2, (c1.blue () + cb.blue ()) / 2));
      } else {
        return QVariant (pl.color (QPalette::Text));
      }
    }
#else
    return QVariant ();
#endif

  } else if (role == Qt::DecorationRole && (m_flags & WithIcons) != 0) {

    //  TODO: icons for normal cells too?
    if (item->is_pcell ()) {
      QIcon icon (":/setup.png");
      return QVariant (icon);
    } else {
      QIcon icon (":/instance.png");
      return QVariant (icon);
    }

  } else {

    return QVariant ();

  }
}

QVariant 
CellTreeModel::headerData (int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
  return QVariant ();
}

int 
CellTreeModel::rowCount (const QModelIndex &parent) const 
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return 0;
  } else if (parent.isValid ()) {
    CellTreeItem *item = (CellTreeItem *) parent.internalPointer ();
    if (! item) {
      return 0;
    } else if (! item->is_valid ()) {
      //  for safety we return 0 children for invalid cells
      return 0;
    } else if (m_filter_mode && m_is_filtered) {
      return int (item->children_in (m_visible_cell_set));
    } else {
      return int (item->children ());
    }
  } else {
    if (m_filter_mode && m_is_filtered) {
      int n = 0;
      for (std::vector <CellTreeItem *>::const_iterator i = m_toplevel.begin (); i != m_toplevel.end (); ++i) {
        if (m_visible_cell_set.find (*i) != m_visible_cell_set.end ()) {
          ++n;
        }
      }
      return n;
    } else {
      return int (m_toplevel.size ());
    }
  }
}

QModelIndex 
CellTreeModel::index (int row, int column, const QModelIndex &parent) const 
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  } else if (parent.isValid ()) {
    CellTreeItem *item = (CellTreeItem *) parent.internalPointer ();
    if (! item) {
      return QModelIndex ();
    } else if (! item->is_valid ()) {
      //  for safety we don't return valid child indexes for invalid cells
      return QModelIndex ();
    } else if (m_filter_mode && m_is_filtered) {
      return createIndex (row, column, item->child_in (m_visible_cell_set, row));
    } else {
      return createIndex (row, column, item->child (row));
    }
  } else if (row >= 0 && row < int (m_toplevel.size ())) {
    if (m_filter_mode && m_is_filtered) {
      int n = row;
      for (std::vector <CellTreeItem *>::const_iterator i = m_toplevel.begin (); i != m_toplevel.end (); ++i) {
        if (m_visible_cell_set.find (*i) != m_visible_cell_set.end ()) {
          if (n-- == 0) {
            return createIndex (row, column, *i);
          }
        }
      }
      return QModelIndex ();
    } else {
      return createIndex (row, column, m_toplevel [row]);
    }
  } else {
    return QModelIndex ();
  }
}

QModelIndex 
CellTreeModel::parent (const QModelIndex &index) const 
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  } 
  if (! index.isValid ()) {
    return index;
  }
  CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
  if (! item) {
    return QModelIndex ();
  }

  CellTreeItem *pitem = item->parent ();
  if (pitem) {
    if (m_filter_mode && m_is_filtered) {
      if (pitem->tree_index () == std::numeric_limits<size_t>::max ()) {
        //  WARNING: invisible item!
        return QModelIndex ();
      } else {
        return createIndex (int (pitem->tree_index ()), index.column (), pitem);
      }
    } else {
      return createIndex (int (pitem->index ()), index.column (), pitem);
    }
  } else {
    return QModelIndex ();
  }
}

int 
CellTreeModel::toplevel_items () const
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return 0;
  } else {
    return int (m_toplevel.size ());
  }
}

CellTreeItem *
CellTreeModel::toplevel_item (int index) 
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return 0;
  } else {
    return m_toplevel [index];
  }
}

QModelIndex 
CellTreeModel::model_index (CellTreeItem *item) const
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  } else if (m_filter_mode && m_is_filtered) {
    if (item->tree_index () == std::numeric_limits<size_t>::max ()) {
      //  WARNING: invisible item!
      return QModelIndex ();
    } else {
      return createIndex (int (item->tree_index ()), 0, item);
    }
  } else {
    return createIndex (int (item->index ()), 0, item);
  }
}

bool
CellTreeModel::is_pcell (const QModelIndex &index) const
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return false;
  } else {
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    return item->is_pcell ();
  }
}

db::pcell_id_type
CellTreeModel::pcell_id (const QModelIndex &index) const
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return 0;
  } else {
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    return item->cell_or_pcell_index ();
  }
}

db::cell_index_type 
CellTreeModel::cell_index (const QModelIndex &index) const
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return 0;
  } else {
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    return item->cell_or_pcell_index ();
  }
}

const db::Cell *
CellTreeModel::cell (const QModelIndex &index) const
{
  if (index.isValid () && ! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    return & mp_layout->cell (item->cell_or_pcell_index ());
  } else {
    return 0;
  }
}

const char *
CellTreeModel::cell_name (const QModelIndex &index) const
{
  if (index.isValid () && ! mp_layout->under_construction () && ! (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    if (item->is_pcell ()) {
      return mp_layout->pcell_header (item->cell_or_pcell_index ())->get_name ().c_str ();
    } else {
      return mp_layout->cell_name (item->cell_or_pcell_index ());
    }
  } else {
    return 0;
  }
}

void
CellTreeModel::clear_locate ()
{
  m_selected_indexes.clear ();
  m_visible_cell_set.clear ();
  m_is_filtered = false;
  m_current_index = m_selected_indexes.begin ();
  m_selected_indexes_set.clear ();

  emit layoutAboutToBeChanged ();

  if (m_filter_mode) {

    QModelIndexList indexes = persistentIndexList ();

    QModelIndexList new_indexes;
    for (QModelIndexList::iterator i = indexes.begin (); i != indexes.end (); ++i) {
      new_indexes.push_back (model_index ((CellTreeItem *) i->internalPointer ()));
    }

    changePersistentIndexList (indexes, new_indexes);

  }

  emit layoutChanged ();
}

QModelIndex
CellTreeModel::locate_next (const QModelIndex &index)
{
  if (m_current_index == m_selected_indexes.end ()) {
    return QModelIndex ();
  } else if (! index.isValid ()) {
    return locate_next ();
  }

  //  easy case: the requested index is a selected one

  for (std::vector <QModelIndex>::const_iterator i = m_selected_indexes.begin (); i != m_selected_indexes.end (); ++i) {
    if (i->internalPointer () == index.internalPointer ()) {
      m_current_index = i;
      if (++m_current_index == m_selected_indexes.end ()) {
        m_current_index = m_selected_indexes.begin ();
      }
      return *m_current_index;
    }
  }

  //  otherwise: search by sequential order

  m_current_index = m_selected_indexes.begin ();

  std::map<CellTreeItem *, size_t> serial_index;
  size_t seq = 0;
  for (size_t i = 0; i < m_toplevel.size (); ++i) {
    seq = m_toplevel [i]->assign_serial (seq, serial_index);
  }

  size_t serial = serial_index [(CellTreeItem *) index.internalPointer ()];
  size_t next = 0;

  for (std::vector <QModelIndex>::const_iterator i = m_selected_indexes.begin (); i != m_selected_indexes.end (); ++i) {
    size_t s = serial_index [(CellTreeItem *) i->internalPointer ()];
    if (s > serial && (next == 0 || s < next)) {
      next = s;
      m_current_index = i;
    }
  }

  return *m_current_index;
}

QModelIndex 
CellTreeModel::locate_next ()
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  }

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
CellTreeModel::locate_prev ()
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  }

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

bool
CellTreeModel::search_children (const tl::GlobPattern &pattern, CellTreeItem *item)
{
  bool any = false;
  size_t ti = 0;

  int children = item->children ();
  for (int i = 0; i < children; ++i) {

    CellTreeItem *c = item->child (i);
    if (c) {

      bool visible = false;

      c->set_tree_index (std::numeric_limits<size_t>::max ());
      if (c->name_matches (pattern)) {
        c->set_tree_index (ti);
        m_selected_indexes.push_back (model_index (c));
        visible = true;
      }
      if (search_children (pattern, c)) {
        c->set_tree_index (ti);
        visible = true;
      }

      if (visible) {
        ++ti;
        m_visible_cell_set.insert (c);
        any = true;
      }

    }

  }

  return any;
}

QModelIndex 
CellTreeModel::locate (const char *name, bool glob_pattern, bool case_sensitive, bool top_only)
{
  if (mp_layout->under_construction () || (mp_layout->manager () && mp_layout->manager ()->transacting ())) {
    return QModelIndex ();
  }

  emit layoutAboutToBeChanged ();

  QModelIndexList indexes = persistentIndexList ();
  std::vector<CellTreeItem *> persistent_index_cells;
  persistent_index_cells.reserve (indexes.size ());

  for (QModelIndexList::iterator i = indexes.begin (); i != indexes.end (); ++i) {
    persistent_index_cells.push_back ((CellTreeItem *) i->internalPointer ());
  }

  m_selected_indexes.clear ();
  m_visible_cell_set.clear ();
  m_is_filtered = true;

  tl::GlobPattern p = tl::GlobPattern (std::string (name));
  p.set_case_sensitive (case_sensitive);
  p.set_exact (!glob_pattern);
  p.set_header_match (true);

  size_t ti = 0;

  for (std::vector <CellTreeItem *>::const_iterator lc = m_toplevel.begin (); lc != m_toplevel.end (); ++lc) {

    bool visible = false;

    (*lc)->set_tree_index (std::numeric_limits<size_t>::max ());
    if ((*lc)->name_matches (p)) {
      (*lc)->set_tree_index (ti);
      m_selected_indexes.push_back (model_index (*lc));
      visible = true;
    }
    if (! top_only && ! m_flat && search_children (p, *lc)) {
      (*lc)->set_tree_index (ti);
      visible = true;
    }

    if (visible) {
      ++ti;
      m_visible_cell_set.insert (*lc);
    }

  }

  m_selected_indexes_set.clear ();
  for (std::vector <QModelIndex>::const_iterator i = m_selected_indexes.begin (); i != m_selected_indexes.end (); ++i) {
    m_selected_indexes_set.insert (i->internalPointer ());
  }

  //  re-layout the items

  if (m_filter_mode) {

    QModelIndexList new_indexes;

    for (std::vector<CellTreeItem *>::const_iterator item = persistent_index_cells.begin (); item != persistent_index_cells.end (); ++item) {
      if (m_visible_cell_set.find (*item) != m_visible_cell_set.end ()) {
        new_indexes.push_back (model_index (*item));
      } else {
        new_indexes.push_back (QModelIndex ());
      }
    }

    changePersistentIndexList (indexes, new_indexes);

  }

  emit layoutChanged ();

  //  make the first selected one current

  m_current_index = m_selected_indexes.begin ();
  if (m_current_index == m_selected_indexes.end ()) {
    return QModelIndex (); 
  } else {
    return *m_current_index;
  }
}

} // namespace lay

#endif

