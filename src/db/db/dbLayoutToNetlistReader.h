
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

#ifndef HDR_dbLayoutToNetlistReader
#define HDR_dbLayoutToNetlistReader

#include "dbCommon.h"
#include "dbPolygon.h"
#include "dbCell.h"
#include "dbLayoutToNetlist.h"
#include "tlStream.h"
#include "tlProgress.h"

namespace db {

class LayoutToNetlistStandardReader;

namespace l2n_std_reader {

  class Brace
  {
  public:
    Brace (db::LayoutToNetlistStandardReader *reader);

    operator bool ();
    void done ();

    bool has_brace () const
    {
      return m_has_brace;
    }

  private:
    db::LayoutToNetlistStandardReader *mp_reader;
    bool m_checked;
    bool m_has_brace;
  };

}

class LayoutToNetlist;
class Circuit;
class Cell;
class DeviceAbstract;
class DeviceClass;
class Net;
class Region;

/**
 *  @brief The base class for a LayoutToNetlist writer
 */
class DB_PUBLIC LayoutToNetlistReaderBase
{
public:
  LayoutToNetlistReaderBase () { }
  virtual ~LayoutToNetlistReaderBase () { }

  void read (db::LayoutToNetlist *l2n)
  {
    do_read (l2n);
  }

private:
  virtual void do_read (db::LayoutToNetlist *l2n) = 0;
};

/**
 *  @brief The standard writer
 */
class DB_PUBLIC LayoutToNetlistStandardReader
  : public LayoutToNetlistReaderBase
{
public:

  struct ObjectMap
  {
    std::map<unsigned int, db::Net *> id2net;
    std::map<unsigned int, db::Device *> id2device;
    std::map<unsigned int, db::SubCircuit *> id2subcircuit;
  };

  LayoutToNetlistStandardReader (tl::InputStream &stream);

  void do_read (db::LayoutToNetlist *l2n);

protected:
  friend class l2n_std_reader::Brace;
  typedef l2n_std_reader::Brace Brace;

  void read_netlist (Netlist *netlist, db::LayoutToNetlist *l2n, Brace *nested = 0, std::map<const db::Circuit *, ObjectMap> *map_per_circuit = 0);
  static size_t terminal_id (const db::DeviceClass *device_class, const std::string &tname);
  static std::pair<db::DeviceAbstract *, const db::DeviceClass *> device_model_by_name (db::Netlist *netlist, const std::string &dmname);
  bool read_message (std::string &msg);
  bool read_severity (Severity &severity);

  const std::string &path () const
  {
    return m_path;
  }

  tl::TextInputStream &stream ()
  {
    return m_stream;
  }

  struct Connections
  {
    Connections (size_t _from_cluster, size_t _to_cluster)
      : from_cluster (_from_cluster), to_cluster (_to_cluster)
    { }

    size_t from_cluster, to_cluster;
  };

  bool test (const std::string &token);
  void expect (const std::string &token);
  void read_word_or_quoted (std::string &s);
  int read_int ();
  bool try_read_int (int &i);
  db::Coord read_coord ();
  double read_double ();
  bool at_end ();
  void skip ();
  void skip_element ();

private:
  tl::TextInputStream m_stream;
  std::string m_path;
  std::string m_line;
  double m_dbu;
  tl::Extractor m_ex;
  db::Point m_ref;
  tl::AbsoluteProgress m_progress;

  void read_net (Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map);
  void read_pin (Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map);
  void read_device (Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map, std::map<db::CellInstArray, std::list<Connections> > &connections);
  void read_subcircuit (Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map, std::map<db::CellInstArray, std::list<Connections> > &connections);
  bool read_trans_part (db::DCplxTrans &tr);
  void read_abstract_terminal (db::LayoutToNetlist *l2n, db::DeviceAbstract *dm, db::DeviceClass *dc);
  void read_property (db::NetlistObject *obj);
  db::Polygon read_polygon ();
  db::Box read_rect ();
  void read_geometries (db::NetlistObject *obj, Brace &br, db::LayoutToNetlist *l2n, db::local_cluster<NetShape> &lc, db::Cell &cell);
  db::Point read_point ();
  void read_message_entry (db::LogEntryData &data);
  bool read_message_cell (std::string &cell_name);
  bool read_message_geometry (db::DPolygon &polygon);
  bool read_message_cat (std::string &category_name, std::string &category_description);
};

}

#endif

