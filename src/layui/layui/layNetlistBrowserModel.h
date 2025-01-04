
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

#ifndef HDR_layNetlistBrowserModel
#define HDR_layNetlistBrowserModel

#include "layColorPalette.h"
#include "tlColor.h"
#include "layuiCommon.h"

#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"
#include "layNetColorizer.h"

#include "tlList.h"
#include "tlTypeTraits.h"

#include <QAbstractItemModel>

#include <map>
#include <memory>

class QTreeView;

namespace lay
{

class IndexedNetlistModel;

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel definition

class NetlistBrowserModel;
class NetlistModelItemData;
class RootItemData;
class CircuitItemData;
class CircuitNetItemData;
class CircuitDeviceItemData;
class CircuitSubCircuitItemData;

/**
 *  @brief A base class for the item data object
 */
class NetlistModelItemData
  : public tl::list_node<NetlistModelItemData>
{
public:
  typedef tl::list<NetlistModelItemData, false>::iterator iterator;

  NetlistModelItemData ();
  NetlistModelItemData (NetlistModelItemData *parent);

  virtual ~NetlistModelItemData ();

  virtual NetlistModelItemData *parent () { return mp_parent; }

  virtual QIcon icon (NetlistBrowserModel *model) = 0;
  virtual QString text (int column, NetlistBrowserModel *model) = 0;
  virtual QString search_text () = 0;
  virtual std::string tooltip (NetlistBrowserModel *model) = 0;
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model) = 0;
  virtual bool has_children (NetlistBrowserModel *model) = 0;

  void ensure_children (NetlistBrowserModel *model);

  void push_back (NetlistModelItemData *child);

  iterator begin () { return m_children.begin (); }
  iterator end ()   { return m_children.end (); }

  size_t child_count () { return m_children_per_index.size (); }
  size_t index () { return m_index; }

  NetlistModelItemData *child (size_t n);

  virtual std::pair<const db::Circuit *, const db::Circuit *> circuits_of_this ();
  std::pair<const db::Circuit *, const db::Circuit *> circuits ();
  bool derived_from_circuits (const std::pair<const db::Circuit *, const db::Circuit *> &sp);

  virtual std::pair<const db::Device *, const db::Device *> devices_of_this ();
  std::pair<const db::Device *, const db::Device *> devices ();
  bool derived_from_devices (const std::pair<const db::Device *, const db::Device *> &sp);

  virtual std::pair<const db::Pin *, const db::Pin *> pins_of_this ();
  std::pair<const db::Pin *, const db::Pin *> pins ();
  bool derived_from_pins (const std::pair<const db::Pin *, const db::Pin *> &sp);

  virtual std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuits_of_this ();
  std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuits ();
  bool derived_from_subcircuits (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &sp);

  virtual std::pair<const db::Net *, const db::Net *> nets_of_this ();
  std::pair<const db::Net *, const db::Net *> nets ();
  bool derived_from_nets (const std::pair<const db::Net *, const db::Net *> &np);

private:
  NetlistModelItemData (const NetlistModelItemData &);
  NetlistModelItemData &operator= (const NetlistModelItemData &);

  NetlistModelItemData *mp_parent;
  tl::list<NetlistModelItemData, false> m_children;
  std::vector<NetlistModelItemData *> m_children_per_index;
  bool m_children_made;
  size_t m_index;

  void set_index (size_t index) { m_index = index; }

  virtual void do_ensure_children (NetlistBrowserModel *model) = 0;
};

/**
 *  @brief An object describing the instantiation path of a net, a device or a (sub)circuit pair
 *
 *  This object applies to pairs of these objects. A class providing a path for a single
 *  object is NetlistObjectPath
 */
class LAYUI_PUBLIC NetlistObjectPath
{
public:
  typedef std::list<const db::SubCircuit *> path_type;
  typedef path_type::const_iterator path_iterator;

  NetlistObjectPath () : root (0), net (0), device (0) { }

  bool is_null () const
  {
    return ! root;
  }

  bool operator== (const NetlistObjectPath &other) const
  {
    return root == other.root && path == other.path && net == other.net && device == other.device;
  }

  bool operator!= (const NetlistObjectPath &other) const
  {
    return ! operator== (other);
  }

  const db::Circuit *root;
  std::list<const db::SubCircuit *> path;
  const db::Net *net;
  const db::Device *device;
};

/**
 *  @brief An object describing the instantiation path of a net, a device or a (sub)circuit pair
 *
 *  This object applies to pairs of these objects. A class providing a path for a single
 *  object is NetlistObjectPath
 */
class LAYUI_PUBLIC NetlistObjectsPath
{
public:
  typedef std::list<std::pair<const db::SubCircuit *, const db::SubCircuit *> > path_type;
  typedef path_type::const_iterator path_iterator;

  NetlistObjectsPath () { }

  bool is_null () const
  {
    return ! root.first && ! root.second;
  }

  static NetlistObjectsPath from_first (const NetlistObjectPath &p);
  static NetlistObjectsPath from_second (const NetlistObjectPath &p);
  static bool translate (NetlistObjectsPath &p, const db::NetlistCrossReference &xref);

  NetlistObjectPath first () const;
  NetlistObjectPath second () const;

  bool operator== (const NetlistObjectsPath &other) const
  {
    return root == other.root && path == other.path && net == other.net && device == other.device;
  }

  bool operator!= (const NetlistObjectsPath &other) const
  {
    return ! operator== (other);
  }

  std::pair<const db::Circuit *, const db::Circuit *> root;
  std::list<std::pair<const db::SubCircuit *, const db::SubCircuit *> > path;
  std::pair<const db::Net *, const db::Net *> net;
  std::pair<const db::Device *, const db::Device *> device;
};

/**
 *  @brief The NetlistBrowserModel
 *
 *  The model hierarchy is the following
 *  - circuits
 *    - 0..#pins: pins
 *      - net (1x)
 *    - #pins..#pins+#nets: nets
 *      - 0..#devices: terminals
 *        - other terminals and nets
 *      - #devices..#devices+#pins: pins
 *      - #devices+#pins..: subcircuit pins
 *        - other pins and nets
 *    - #pins+#nets..#pins+#nets+#subcircuits: subcircuits
 *      - pins and nets
 *    - #pins+#nets+#subcircuits..: devices
 *      - terminals and nets
 */
class LAYUI_PUBLIC NetlistBrowserModel
  : public QAbstractItemModel, public tl::Object
{
Q_OBJECT

public:
  NetlistBrowserModel (QWidget *parent, db::Netlist *netlist, NetColorizer *colorizer);
  NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb, NetColorizer *colorizer);
  NetlistBrowserModel (QWidget *parent, db::LayoutVsSchematic *lvsdb, NetColorizer *colorizer);
  ~NetlistBrowserModel ();

  virtual int columnCount (const QModelIndex &parent) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual Qt::ItemFlags flags (const QModelIndex &index) const;
  virtual bool hasChildren (const QModelIndex &parent) const;
  virtual QVariant headerData (int section, Qt::Orientation orientation, int role) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &index) const;
  virtual int rowCount (const QModelIndex &parent) const;

  int status_column () const
  {
    return m_status_column;
  }

  int object_column () const
  {
    return m_object_column;
  }

  int first_column () const
  {
    return m_first_column;
  }

  int second_column () const
  {
    return m_second_column;
  }

  IndexedNetlistModel *indexer ()
  {
    return mp_indexer.get ();
  }

  std::pair<const db::Net *, const db::Net *> net_from_index (const QModelIndex &index, bool include_parents = true) const;
  QModelIndex index_from_net (const std::pair<const db::Net *, const db::Net *> &net) const;
  QModelIndex index_from_net (const db::Net *net) const;
  std::pair<const db::Circuit *, const db::Circuit *> circuit_from_index (const QModelIndex &index, bool include_parents = true) const;
  QModelIndex index_from_circuit (const std::pair<const db::Circuit *, const db::Circuit *> &circuit) const;
  QModelIndex index_from_circuit (const db::Circuit *circuit) const;
  QModelIndex index_from_subcircuit (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &subcircuits) const;

  std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuit_from_index (const QModelIndex &index, bool include_parents = true) const;

  std::pair<const db::Device *, const db::Device *> device_from_index (const QModelIndex &index, bool include_parents = true) const;

  void set_item_visibility (QTreeView *view, bool show_all, bool with_warnings);

  QString make_link_to (const std::pair<const db::Net *, const db::Net *> &nets, int column = 0) const;
  QString make_link_to (const std::pair<const db::Device *, const db::Device *> &devices, int column = 0) const;
  QString make_link_to (const std::pair<const db::Pin *, const db::Pin *> &pins, const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column = 0) const;
  QString make_link_to (const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column = 0) const;
  QString make_link_to (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &sub_circuits, int column = 0) const;

  bool is_valid_net_pair (const std::pair<const db::Net *, const db::Net *> &net) const;

  QIcon icon_for_nets (const std::pair<const db::Net *, const db::Net *> &net) const;
  QIcon icon_for_connection (const std::pair<const db::Net *, const db::Net *> &net) const;

  QModelIndex index_from_url (const QString &url) const;

  NetlistObjectsPath path_from_index (const QModelIndex &index) const;
  NetlistObjectPath spath_from_index (const QModelIndex &index) const
  {
    return path_from_index (index).first ();
  }

  QModelIndex index_from_path (const NetlistObjectsPath &path);
  QModelIndex index_from_path (const NetlistObjectPath &path)
  {
    return index_from_path (NetlistObjectsPath::from_first (path));
  }

private slots:
  void colors_changed ();

private:
  NetlistBrowserModel (const NetlistBrowserModel &);
  NetlistBrowserModel &operator= (const NetlistBrowserModel &);

  QString text (const QModelIndex &index) const;
  QVariant tooltip (const QModelIndex &index) const;
  QString search_text (const QModelIndex &index) const;
  db::NetlistCrossReference::Status status (const QModelIndex &index) const;
  QIcon icon (const QModelIndex &index) const;
  QString build_url (const QModelIndex &index, const std::string &title) const;

  std::pair<const db::Netlist *, const db::Netlist *> netlists () const
  {
    return std::pair<const db::Netlist *, const db::Netlist *> (mp_l2ndb->netlist (), (const db::Netlist *)0);
  }

  void show_or_hide_items (QTreeView *view, const QModelIndex &parent, bool show_all, bool with_warnings, int levels);

  db::LayoutToNetlist *mp_l2ndb;
  db::LayoutVsSchematic *mp_lvsdb;
  NetColorizer *mp_colorizer;
  std::unique_ptr<IndexedNetlistModel> mp_indexer;
  mutable std::map<tl::color_t, QIcon> m_net_icon_per_color;
  mutable std::map<tl::color_t, QIcon> m_connection_icon_per_color;
  int m_object_column;
  int m_status_column;
  int m_first_column;
  int m_second_column;
  std::unique_ptr<NetlistModelItemData> mp_root;

  RootItemData *root () const;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
