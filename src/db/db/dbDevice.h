
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

#ifndef _HDR_dbDevice
#define _HDR_dbDevice

#include "dbCommon.h"
#include "dbNet.h"
#include "dbPoint.h"

#include "tlObject.h"

#include <vector>

namespace db
{

class Circuit;
class DeviceClass;
class DeviceAbstract;

/**
 *  @brief An actual device
 *
 *  This class represents the incarnation of a specific device.
 *  The device has a class which specifies a type. This class
 *  is intended for subclassing.
 *  A specific device subclass is supposed to correspond to
 *  a specific device class.
 */
class DB_PUBLIC Device
  : public tl::Object
{
public:
  typedef std::vector<std::pair<size_t, size_t> > global_connections;
  typedef global_connections::const_iterator global_connections_iterator;

  /**
   *  @brief Default constructor
   */
  Device ();

  /**
   *  @brief The constructor
   */
  Device (DeviceClass *device_class, const std::string &name = std::string ());

  /**
   *  @brief The constructor
   */
  Device (DeviceClass *device_class, DeviceAbstract *device_abstract, const std::string &name = std::string ());

  /**
   *  @brief Copy constructor
   */
  Device (const Device &other);

  /**
   *  @brief Assignment
   */
  Device &operator= (const Device &other);

  /**
   *  @brief Destructor
   */
  ~Device ();

  /**
   *  @brief Gets the device class
   */
  const DeviceClass *device_class () const
  {
    return mp_device_class;
  }

  /**
   *  @brief Sets the device class
   */
  void set_device_class (DeviceClass *dc)
  {
    mp_device_class = dc;
  }

  /**
   *  @brief Gets the device abstract
   */
  const DeviceAbstract *device_abstract () const
  {
    return mp_device_abstract;
  }

  /**
   *  @brief Sets the device abstract
   */
  void set_device_abstract (DeviceAbstract *dm)
  {
    mp_device_abstract = dm;
  }

  /**
   *  @brief Gets the device ID
   *  The ID is a unique integer which identifies the device.
   *  It can be used to retrieve the device from the circuit using Circuit::device_by_id.
   *  When assigned, the device ID is not 0.
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the circuit the device lives in (const version)
   *  This pointer is 0 if the device isn't added to a circuit
   */
  const Circuit *circuit () const
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the circuit the device lives in (non-const version)
   *  This pointer is 0 if the device isn't added to a circuit
   */
  Circuit *circuit ()
  {
    return mp_circuit;
  }

  /**
   *  @brief Sets the name
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets a name which always non-empty
   *  This method will pick a name like "$<id>" if the explicit name is empty.
   */
  std::string expanded_name () const;

  /**
   *  @brief Sets the device position
   *  The device position should be the center of the recognition shape or something similar.
   *  Giving the device a position allows combining multiple devices with the same
   *  relative geometry into a single cell.
   *  The position has to be given in micrometer units.
   */
  void set_position (const db::DPoint &pos);

  /**
   *  @brief Gets the device position
   */
  const db::DPoint &position () const
  {
    return m_position;
  }

  /**
   *  @brief Gets the net attached to a specific terminal
   *  Returns 0 if no net is attached.
   */
  const Net *net_for_terminal (size_t terminal_id) const;

  /**
   *  @brief Gets the net attached to a specific terminal (non-const version)
   *  Returns 0 if no net is attached.
   */
  Net *net_for_terminal (size_t terminal_id)
  {
    return const_cast<Net *> (((const Device *) this)->net_for_terminal (terminal_id));
  }

  /**
   *  @brief Connects the given terminal to the given net
   *  If the net is 0 the terminal is disconnected.
   *  If non-null, a NetTerminalRef object will be inserted into the
   *  net and connected with the given terminal.
   *  If the terminal is connected to a global net, it will be
   *  disconnected from there.
   */
  void connect_terminal (size_t terminal_id, Net *net);

  /**
   *  @brief Gets the value for the parameter with the given ID
   */
  double parameter_value (size_t param_id) const;

  /**
   *  @brief Sets the value for the parameter with the given ID
   */
  void set_parameter_value (size_t param_id, double v);

  /**
   *  @brief Gets the value for the parameter with the given name
   *  If the name is not valid, an exception is thrown.
   */
  double parameter_value (const std::string &name) const;

  /**
   *  @brief Sets the value for the parameter with the given name
   *  If the name is not valid, an exception is thrown.
   */
  void set_parameter_value (const std::string &name, double v);

private:
  friend class Circuit;
  friend class Net;

  DeviceClass *mp_device_class;
  DeviceAbstract *mp_device_abstract;
  std::string m_name;
  db::DPoint m_position;
  std::vector<Net::terminal_iterator> m_terminal_refs;
  std::vector<double> m_parameters;
  size_t m_id;
  Circuit *mp_circuit;

  /**
   *  @brief Sets the terminal reference for a specific terminal
   */
  void set_terminal_ref_for_terminal (size_t terminal_id, Net::terminal_iterator iter);

  /**
   *  @brief Sets the device ID
   */
  void set_id (size_t id)
  {
    m_id = id;
  }

  /**
   *  @brief Sets the circuit
   */
  void set_circuit (Circuit *circuit);
};

}

#endif
