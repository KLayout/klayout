
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

#include "layNetlistBrowserTreeModel.h"
#include "layIndexedNetlistModel.h"
#include "layNetlistCrossReferenceModel.h"
#include "layNetlistBrowserModel.h"

#include <QPainter>
#include <QIcon>
#include <QWidget>
#include <QTreeView>

namespace lay
{

// ----------------------------------------------------------------------------------
//  NetlistBrowserTreeModel implementation

const std::string var_sep (" \u21D4 ");

static inline size_t pop (void *&idp, size_t n)
{
  size_t id = reinterpret_cast<size_t> (idp);
  size_t i = id % n;
  id /= n;
  idp = reinterpret_cast<void *> (id);
  return i;
}

static QIcon icon_for_circuit ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_16.png")));
  return icon;
}

static QIcon icon_for_status (db::NetlistCrossReference::Status status)
{
  if (status == db::NetlistCrossReference::NoMatch || status == db::NetlistCrossReference::Mismatch) {
    return QIcon (":/error2_16px.png");
  } else if (status == db::NetlistCrossReference::MatchWithWarning || status == db::NetlistCrossReference::Skipped) {
    return QIcon (":/warn_16px.png");
  } else {
    return QIcon ();
  }
}

template <class Obj>
static std::string str_from_name (const Obj *obj, bool dash_for_empty = false)
{
  if (obj) {
    return obj->name ();
  } else if (dash_for_empty) {
    return std::string ("-");
  } else {
    return std::string ();
  }
}

template <class Obj>
static std::string str_from_names (const std::pair<const Obj *, const Obj *> &objs, bool is_single)
{
  std::string s = str_from_name (objs.first, ! is_single);
  if (! is_single) {
    std::string t = str_from_name (objs.second, ! is_single);
    if (t != s) {
      s += var_sep;
      s += t;
    }
  }
  return s;
}

static std::string combine_search_strings (const std::string &s1, const std::string &s2)
{
  if (s1.empty ()) {
    return s2;
  } else if (s2.empty ()) {
    return s1;
  } else {
    return s1 + "|" + s2;
  }
}

template <class Obj>
static std::string search_string_from_names (const std::pair<const Obj *, const Obj *> &objs)
{
  if (objs.first && objs.second) {
    return combine_search_strings (objs.first->name (), objs.second->name ());
  } else if (objs.first) {
    return objs.first->name ();
  } else if (objs.second) {
    return objs.second->name ();
  } else {
    return std::string ();
  }
}


NetlistBrowserTreeModel::NetlistBrowserTreeModel (QWidget *parent, db::Netlist *netlist)
  : QAbstractItemModel (parent), mp_l2ndb (0), mp_lvsdb (0)
{
  mp_indexer.reset (new SingleIndexedNetlistModel (netlist));

  m_object_column = 0;
  m_status_column = -1;
}

NetlistBrowserTreeModel::NetlistBrowserTreeModel (QWidget *parent, db::LayoutToNetlist *l2ndb)
  : QAbstractItemModel (parent), mp_l2ndb (l2ndb), mp_lvsdb (0)
{
  mp_indexer.reset (new SingleIndexedNetlistModel (l2ndb->netlist ()));

  m_object_column = 0;
  m_status_column = -1;
}

NetlistBrowserTreeModel::NetlistBrowserTreeModel (QWidget *parent, db::LayoutVsSchematic *lvsdb)
  : QAbstractItemModel (parent), mp_l2ndb (0), mp_lvsdb (lvsdb)
{
  mp_indexer.reset (new NetlistCrossReferenceModel (lvsdb->cross_ref ()));

  m_object_column = 0;
  m_status_column = 1;
}

NetlistBrowserTreeModel::~NetlistBrowserTreeModel ()
{
  //  .. nothing yet ..
}

int
NetlistBrowserTreeModel::columnCount (const QModelIndex & /*parent*/) const
{
  //  Text and status for twoway indexer
  return mp_indexer->is_single () ? 1 : 2;
}

QVariant
NetlistBrowserTreeModel::data (const QModelIndex &index, int role) const
{
  if (! index.isValid ()) {
    return QVariant ();
  }

  if (role == Qt::DecorationRole && index.column () == m_object_column) {
    return QVariant (icon_for_circuit ());
  } else if (role == Qt::DecorationRole && index.column () == m_status_column) {
    return QVariant (icon_for_status (status (index)));
  } else if (role == Qt::DisplayRole) {
    return QVariant (text (index));
  } else if (role == Qt::ToolTipRole) {
    if (index.column () == m_status_column) {
      return tooltip (index);
    } else {
      return QVariant (text (index));
    }
  } else if (role == Qt::UserRole) {
    return QVariant (search_text (index));
  } else if (role == Qt::FontRole) {
    db::NetlistCrossReference::Status st = status (index);
    if (st == db::NetlistCrossReference::NoMatch || st == db::NetlistCrossReference::Mismatch || st == db::NetlistCrossReference::Skipped) {
      QFont font;
      font.setWeight (QFont::Bold);
      return QVariant (font);
    }
  } else if (role == Qt::ForegroundRole) {
    db::NetlistCrossReference::Status st = status (index);
    if (st == db::NetlistCrossReference::Match || st == db::NetlistCrossReference::MatchWithWarning) {
      //  taken from marker browser:
      return QVariant (QColor (0, 192, 0));
    }
  }
  return QVariant ();
}

QString
NetlistBrowserTreeModel::text (const QModelIndex &index) const
{
  std::pair<const db::Circuit *, const db::Circuit *> circuits = circuits_from_index (index);

  if (index.column () == m_object_column) {
    return tl::to_qstring (str_from_names (circuits, mp_indexer->is_single ()));
  } else {
    return QString ();
  }
}

QString
NetlistBrowserTreeModel::search_text (const QModelIndex &index) const
{
  std::pair<const db::Circuit *, const db::Circuit *> circuits = circuits_from_index (index);
  return tl::to_qstring (search_string_from_names (circuits));
}

std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> >
NetlistBrowserTreeModel::cp_status_from_index (const QModelIndex &index, size_t &nprod, size_t &nlast, size_t &nnlast) const
{
  typedef std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> > cp_status;

  void *id = index.internalPointer ();
  tl_assert (id != 0);

  nprod = 1;
  nnlast = 1;

  nlast = mp_indexer->top_circuit_count () + 1;
  size_t i = pop (id, nlast);
  nprod *= nlast;
  cp_status cps = mp_indexer->top_circuit_from_index (i - 1);

  while (id != 0) {
    nnlast = nlast;
    nlast = mp_indexer->child_circuit_count (cps.first) + 1;
    i = pop (id, nlast);
    nprod *= nlast;
    cps = mp_indexer->child_circuit_from_index (cps.first, i - 1);
  }

  return cps;
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistBrowserTreeModel::circuits_from_index (const QModelIndex &index) const
{
  size_t nprod = 0, nlast = 0, nnlast = 0;
  return cp_status_from_index (index, nprod, nlast, nnlast).first;
}

void
NetlistBrowserTreeModel::build_circuits_to_index (size_t nprod, const std::pair<const db::Circuit *, const db::Circuit *> &circuits, IndexedNetlistModel *model, const QModelIndex &index, std::map<std::pair<const db::Circuit *, const db::Circuit *>, QModelIndex> &map) const
{
  if (map.find (circuits) != map.end ()) {
    return;
  }

  map.insert (std::make_pair (circuits, index));

  size_t count = mp_indexer->child_circuit_count (circuits);
  size_t child_nprod = nprod * (count + 1);

  for (size_t n = count; n > 0; ) {
    std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> > cp = mp_indexer->child_circuit_from_index (circuits, n - 1);
    QModelIndex child_index = createIndex (int (n - 1), 0, reinterpret_cast<void *> (size_t (index.internalPointer ()) + nprod * n));
    build_circuits_to_index (child_nprod, cp.first, model, child_index, map);
    --n;
  }
}

static bool is_compatible (const std::pair<const db::Circuit *, const db::Circuit *> &a, const std::pair<const db::Circuit *, const db::Circuit *> &b)
{
  if (a.first && b.first && a.first == b.first) {
    return true;
  } else if (a.second && b.second && a.second == b.second) {
    return true;
  } else {
    return false;
  }
}

QModelIndex
NetlistBrowserTreeModel::index_from_netpath (const NetlistObjectsPath &path) const
{
  QModelIndex idx;

  idx = index_from_circuits (path.root);

  for (NetlistObjectsPath::path_iterator p = path.path.begin (); p != path.path.end () && idx.isValid (); ++p) {

    std::pair<const db::Circuit *, const db::Circuit *> sc (p->first ? p->first->circuit_ref () : 0, p->second ? p->second->circuit_ref (): 0);
    std::pair<const db::Circuit *, const db::Circuit *> circuit = circuits_from_index (idx);

    size_t count = mp_indexer->child_circuit_count (circuit);
    for (size_t n = count; n > 0; ) {
      --n;
      std::pair<const db::Circuit *, const db::Circuit *> cc = mp_indexer->child_circuit_from_index (circuit, n).first;
      if (is_compatible (sc, cc)) {
        circuit = cc;
        idx = index (int (n), 0, idx);
        break;
      }
    }

  }

  return idx;
}

QModelIndex
NetlistBrowserTreeModel::index_from_circuits (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const
{
  if (m_circuits_to_index.empty ()) {

    size_t count = mp_indexer->top_circuit_count ();
    for (size_t n = count; n > 0; ) {
      std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> > cp = mp_indexer->top_circuit_from_index (n - 1);
      build_circuits_to_index (count + 1, cp.first, mp_indexer.get (), createIndex (int (n - 1), 0, reinterpret_cast<void *> (n)), m_circuits_to_index);
      --n;
    }

  }

  std::map<std::pair<const db::Circuit *, const db::Circuit *>, QModelIndex>::const_iterator m = m_circuits_to_index.find (circuits);
  if (m != m_circuits_to_index.end ()) {
    return m->second;
  } else {
    return QModelIndex ();
  }
}


db::NetlistCrossReference::Status
NetlistBrowserTreeModel::status (const QModelIndex &index) const
{
  size_t nprod = 0, nlast = 0, nnlast = 0;
  return cp_status_from_index (index, nprod, nlast, nnlast).second.first;
}

QVariant
NetlistBrowserTreeModel::tooltip (const QModelIndex &index) const
{
  typedef std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> > cp_status;
  size_t nlast = 0;
  std::string hint;

  void *id = index.internalPointer ();
  tl_assert (id != 0);

  nlast = mp_indexer->top_circuit_count () + 1;
  size_t i = pop (id, nlast);
  hint = mp_indexer->top_circuit_status_hint (i - 1);
  cp_status cps = mp_indexer->top_circuit_from_index (i - 1);

  while (id != 0) {
    nlast = mp_indexer->child_circuit_count (cps.first) + 1;
    i = pop (id, nlast);
    hint = mp_indexer->child_circuit_status_hint (cps.first, i - 1);
    cps = mp_indexer->child_circuit_from_index (cps.first, i - 1);
  }

  if (! hint.empty ()) {
    return QVariant (tl::to_qstring (hint));
  } else {
    return QVariant ();
  }
}

Qt::ItemFlags
NetlistBrowserTreeModel::flags (const QModelIndex & /*index*/) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool
NetlistBrowserTreeModel::hasChildren (const QModelIndex &parent) const
{
  return rowCount (parent) > 0;
}

QVariant
NetlistBrowserTreeModel::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole && section == m_object_column) {
    if (mp_indexer->is_single ()) {
      return tr ("Circuit");
    } else {
      return tr ("Circuits");
    }
  } else if (role == Qt::DecorationRole && section == m_status_column) {
    return QIcon (":/info_16px.png");
  }
  return QVariant ();
}

QModelIndex
NetlistBrowserTreeModel::index (int row, int column, const QModelIndex &parent) const
{
  if (! parent.isValid ()) {

    return createIndex (row, column, reinterpret_cast<void *> (size_t (row + 1)));

  } else {

    size_t nprod = 0, nlast = 0, nnlast = 0;
    cp_status_from_index (parent, nprod, nlast, nnlast);

    void *id = parent.internalPointer ();
    return createIndex (row, column, reinterpret_cast<void *> (reinterpret_cast<size_t> (id) + size_t (row + 1) * nprod));

  }
}

QModelIndex
NetlistBrowserTreeModel::parent (const QModelIndex &index) const
{
  if (index.isValid ()) {

    size_t nprod = 0, nlast = 0, nnlast = 0;
    cp_status_from_index (index, nprod, nlast, nnlast);

    tl_assert (nlast != 0);

    if (nprod > nlast) {

      nprod /= nlast;

      void *id = index.internalPointer ();
      size_t ids = reinterpret_cast<size_t> (id);
      tl_assert (ids >= nprod);
      ids %= nprod;

      nprod /= nnlast;

      return createIndex (int (ids / nprod - 1), 0, reinterpret_cast<void *> (ids));

    }

  }

  return QModelIndex ();
}

int
NetlistBrowserTreeModel::rowCount (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return int (mp_indexer->top_circuit_count ());
  } else {
    std::pair<const db::Circuit *, const db::Circuit *> circuits = circuits_from_index (parent);
    return int (mp_indexer->child_circuit_count (circuits));
  }
}

}

#endif
