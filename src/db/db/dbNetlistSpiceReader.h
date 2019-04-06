
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

#ifndef HDR_dbNetlistSpiceReader
#define HDR_dbNetlistSpiceReader

#include "dbCommon.h"
#include "dbNetlistReader.h"
#include "tlStream.h"

#include <string>
#include <memory>

namespace db
{

class Netlist;
class Net;
class Circuit;
class DeviceClass;

/**
 *  @brief A SPICE format reader for netlists
 */
class DB_PUBLIC NetlistSpiceReader
  : public NetlistReader
{
public:
  NetlistSpiceReader ();
  virtual ~NetlistSpiceReader ();

  virtual void read (tl::InputStream &stream, db::Netlist &netlist);

private:
  db::Netlist *mp_netlist;
  db::Circuit *mp_circuit;
  std::auto_ptr<tl::TextInputStream> mp_stream;
  std::vector<std::pair<tl::InputStream *, tl::TextInputStream *> > m_streams;
  std::auto_ptr<std::map<std::string, db::Net *> > mp_nets_by_name;
  std::string m_stored_line;

  void push_stream (const std::string &path);
  void pop_stream ();
  bool at_end ();
  void read_subcircuit (tl::Extractor &ex);
  void read_circuit (tl::Extractor &ex);
  void read_device (db::DeviceClass *dev_cls, size_t param_id, tl::Extractor &ex);
  void read_mos4_device (tl::Extractor &ex);
  bool read_element ();
  double read_value (tl::Extractor &ex);
  double read_atomic_value (tl::Extractor &ex);
  double read_dot_expr (tl::Extractor &ex);
  double read_bar_expr (tl::Extractor &ex);
  std::string get_line ();
  void unget_line (const std::string &l);
  void error (const std::string &msg);
  void warn (const std::string &msg);
  void finish ();
  db::Net *make_net (const std::string &name);
  void ensure_circuit ();
};

}

namespace tl
{

template <>
struct type_traits<db::NetlistSpiceReader>
  : public tl::type_traits<void>
{
  typedef tl::false_tag has_default_constructor;
  typedef tl::false_tag has_copy_constructor;
};

}

#endif
