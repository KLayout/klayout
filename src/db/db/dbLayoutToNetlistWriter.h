
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

#ifndef HDR_dbLayoutToNetlistWriter
#define HDR_dbLayoutToNetlistWriter

#include "dbCommon.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "dbPolygon.h"
#include "dbHierNetworkProcessor.h"
#include "tlStream.h"
#include "tlProgress.h"

namespace db
{

class Circuit;
class SubCircuit;
class Device;
class DeviceAbstract;
class Net;
class Netlist;
class LayoutToNetlist;

namespace l2n_std_format
{

template <class Keys>
class std_writer_impl
  : private db::CircuitCallback
{
public:
  std_writer_impl (tl::OutputStream &stream, double dbu, const std::string &progress_description = std::string ());

  void write (const db::LayoutToNetlist *l2n);
  void write (const db::Netlist *netlist, const db::LayoutToNetlist *l2n, bool nested, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit);

protected:
  tl::OutputStream &stream ()
  {
    return *mp_stream;
  }

private:
  tl::OutputStream *mp_stream;
  db::Point m_ref;
  double m_dbu;
  const db::Netlist *mp_netlist;
  const db::LayoutToNetlist *mp_l2n;
  tl::AbsoluteProgress m_progress;

  void write (bool nested, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit);
  void write (const db::Circuit &circuit, const std::string &indent, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit);
  void write (const db::Net &net, unsigned int id, const std::string &indent);
  void write (const db::SubCircuit &subcircuit, std::map<const Net *, unsigned int> &net2id, const std::string &indent);
  void write (const db::Device &device, std::map<const Net *, unsigned int> &net2id, const std::string &indent);
  void write (const db::DeviceAbstract &device_abstract, const std::string &indent);
  void write (const db::PolygonRef *s, const db::ICplxTrans &tr, const std::string &lname, bool relative);
  void write (const db::DCplxTrans &trans);
  void reset_geometry_ref ();

  //  implementation of CircuitCallback
  bool new_cell (cell_index_type ci) const;
};

}

class LayoutToNetlist;

/**
 *  @brief The base class for a LayoutToNetlist writer
 */
class DB_PUBLIC LayoutToNetlistWriterBase
{
public:
  LayoutToNetlistWriterBase ();
  virtual ~LayoutToNetlistWriterBase ();

  void write (const db::LayoutToNetlist *l2n);

protected:
  virtual void do_write (const db::LayoutToNetlist *l2n) = 0;

private:
  std::string m_filename;
};

/**
 *  @brief The standard writer
 */
class DB_PUBLIC LayoutToNetlistStandardWriter
  : public LayoutToNetlistWriterBase
{
public:
  LayoutToNetlistStandardWriter (tl::OutputStream &stream, bool short_version);

protected:
  void do_write (const db::LayoutToNetlist *l2n);

private:
  tl::OutputStream *mp_stream;
  bool m_short_version;
};

}

#endif
