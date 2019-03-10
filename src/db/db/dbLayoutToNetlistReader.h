
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "tlStream.h"

namespace db {

namespace l2n_std_reader {
  class Layers;
  class Brace;
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

  virtual void read (db::LayoutToNetlist *l2n) = 0;
};

/**
 *  @brief The standard writer
 */
class DB_PUBLIC LayoutToNetlistStandardReader
  : public LayoutToNetlistReaderBase
{
public:
  LayoutToNetlistStandardReader (tl::InputStream &stream);

  void read (db::LayoutToNetlist *l2n);

private:
  friend class l2n_std_reader::Brace;
  typedef l2n_std_reader::Brace Brace;
  typedef l2n_std_reader::Layers Layers;

  struct Connections
  {
    Connections (size_t _from_cluster, size_t _to_cluster)
      : from_cluster (_from_cluster), to_cluster (_to_cluster)
    { }

    size_t from_cluster, to_cluster;
  };

  tl::TextInputStream m_stream;
  std::string m_path;
  std::string m_line;
  tl::Extractor m_ex;

  void do_read (db::LayoutToNetlist *l2n);

  bool test (const std::string &token);
  void expect (const std::string &token);
  void read_word_or_quoted(std::string &s);
  int read_int ();
  db::Coord read_coord ();
  double read_double ();
  bool at_end ();
  void skip ();

  void read_net (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::map<unsigned int, db::Net *> &id2net);
  void read_pin (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::map<unsigned int, db::Net *> &id2net);
  db::CellInstArray read_device (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::list<Connections> &refs, std::map<unsigned int, db::Net *> &id2net);
  db::CellInstArray read_subcircuit (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::list<Connections> &refs, std::map<unsigned int, db::Net *> &id2net);
  void read_abstract_terminal (db::LayoutToNetlist *l2n, db::DeviceAbstract *dm, db::DeviceClass *dc);
  std::pair<unsigned int, db::PolygonRef> read_geometry (db::LayoutToNetlist *l2n);
};

}

#endif

