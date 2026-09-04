
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbLayoutConnectivity
#define HDR_dbLayoutConnectivity

#include "dbCommon.h"
#include "dbPropertiesRepository.h"
#include "dbShape.h"
#include "dbInstances.h"
#include "tlString.h"
#include "tlSList.h"

namespace db
{

/**
 *  @brief Encodes pin information
 *
 *  The value is a LayoutPin object.
 */
extern DB_PUBLIC db::property_names_id_type pin_property_name_id;

/**
 *  @brief Encodes instance connectivity information
 *
 *  The value is a LayoutInstanceConnections object.
 */
extern DB_PUBLIC db::property_names_id_type instance_connections_property_name_id;

/**
 *  @brief Encodes shape net information
 *
 *  The value is the net name
 */
extern DB_PUBLIC db::property_names_id_type shape_net_property_name_id;

/**
 *  @brief Describes a layout pin
 *
 *  Layout pins are shapes that are declared "pins".
 *  Technically this happens by adding a property
 *  with a key named with pin_property_name_id and
 *  adding a LayoutPin object as a value.
 *
 *  A pin has:
 *  * A name: This is a basic name followed by an optional
 *    disambiguator in form of $n, where n is an integer number.
 *  * A flag indicating whether all pins of the same basic
 *    name need to be connected ("must_connect")
 */
class DB_PUBLIC LayoutPin
{
public:
  LayoutPin ();
  LayoutPin (const std::string &name, bool must_connect = false);

  /**
   *  @brief Sets the name of the pin
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name of the pin
   */
  const char *name () const;

  /**
   *  @brief Gets the basic name of the pin
   */
  std::string basic_name () const;

  /**
   *  @brief Gets the pin name ID (in property name ID space)
   */
  db::property_names_id_type name_id () const
  {
    return m_name;
  }

  /**
   *  @brief Sets a value indicating that all pins with the same name need to connect
   */
  void set_must_connect (bool mc);

  /**
   *  @brief Gets a value indicating that all pins with the same name need to connect
   */
  bool must_connect () const
  {
    return m_must_connect;
  }

  /**
   *  @brief Converts the pin information to a string for serialization
   */
  std::string to_string () const;

  /**
   *  @brief Reads the pin information from a string for de-serialization
   */
  bool parse (tl::Extractor &ex);

  /**
   *  @brief Equality
   */
  bool operator== (const LayoutPin &other) const
  {
    return m_must_connect == other.m_must_connect && m_name == other.m_name;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const LayoutPin &other) const
  {
    if (m_must_connect != other.m_must_connect) {
      return m_must_connect < other.m_must_connect;
    }

    db::ComparePropertiesNameIds comp;
    return comp (m_name, other.m_name);
  }

private:
  db::property_names_id_type m_name;
  bool m_must_connect;
};

/**
 *  @brief Describes instance connectivity
 *
 *  Instance connectivity connects pin names to nets.
 *  This object is attached to an instance through a
 *  property with name connectivity_property_name_id
 *  to an instance with a LayoutInstanceConnections
 *  object as a value.
 *
 *  The object keeps all connections for an instance.
 *  Connections are pin-to-net relations. Unconnected
 *  pins are not listed.
 *
 *  Instances can be cirtuits for devices. In case of devices,
 *  the pin name is the terminal name.
 */
class DB_PUBLIC LayoutInstanceConnections
{
public:
  typedef std::map<db::property_names_id_type, db::property_names_id_type, db::ComparePropertiesNameIds> connections_map;
  typedef connections_map::const_iterator connections_iterator;

  LayoutInstanceConnections ();

  /**
   *  @brief Clears all connections
   */
  void clear ();

  /**
   *  @brief Creates a connection
   *  @param pin_name The pin to connect
   *  @param net The name of the net to connect
   */
  void add_connection (const std::string &pin_name, const std::string &net);

  /**
   *  @brief Creates a connection
   *  @param pin_name_id The pin to connect
   *  @param net_id The name of the net to connect
   */
  void add_connection (db::property_names_id_type pin_name_id, db::property_names_id_type net_id);

  /**
   *  @brief Sets all connections from an iterator
   */
  template <class Iter>
  void set_connections (Iter b, Iter e)
  {
    clear ();
    for (auto i = b; i != e; ++i) {
      add_connection (i->first, i->second);
    }
  }

  /**
   *  @brief Gets a value indicating whether the given pin is attached to a net
   */
  bool has_pin (const std::string &pin_name) const;

  /**
   *  @brief Gets a value indicating whether the given pin (by name ID) is attached to a net
   */
  bool has_pin (db::property_names_id_type pin_name_id) const;

  /**
   *  @brief Gets the net name for a pin
   *
   *  If no net is connected to the given pin, a null pointer is returned.
   */
  const char *net_for_pin (const std::string &pin_name) const;

  /**
   *  @brief Gets the net name ID for a pin
   */
  db::property_names_id_type net_id_for_pin (const std::string &pin_name) const;

  /**
   *  @brief Gets the net name ID for a pin name ID
   */
  db::property_names_id_type net_id_for_pin (db::property_names_id_type pin_name_id) const;

  /**
   *  @brief Connections iterator (begin)
   *  The value is a pair of pin name ID and net name ID
   */
  connections_iterator begin () const
  {
    return m_connections.begin ();
  }

  /**
   *  @brief Connections iterator (end)
   */
  connections_iterator end () const
  {
    return m_connections.end ();
  }

  /**
   *  @brief Converts the instance connectivity information to a string for serialization
   */
  std::string to_string () const;

  /**
   *  @brief Reads the instance connectivity information from a string for de-serialization
   */
  bool parse (tl::Extractor &ex);

  /**
   *  @brief Equality
   */
  bool operator== (const LayoutInstanceConnections &other) const
  {
    return m_connections == other.m_connections;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const LayoutInstanceConnections &other) const
  {
    return m_connections < other.m_connections;
  }

private:
  std::map<db::property_names_id_type, db::property_names_id_type, db::ComparePropertiesNameIds> m_connections;
};

/**
 *  @brief Represents a single pin in a cell
 *
 *  A pin can have multiple shapes.
 */
class DB_PUBLIC LayoutConnectivityPin
{
public:
  /**
   *  Default constructor
   */
  LayoutConnectivityPin ()
    : must_connect (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief A flag indicating whether the pins must be connected
   *  Pins which are not connected inside the child cell need to
   *  have this flag to indicate that they need to be connected
   *  further up in the hierarchy.
   */
  bool must_connect;

  /**
   *  @brief The shapes (layer, shape reference) making the pins
   */
  tl::slist<std::pair<unsigned int, db::Shape> > pin_shapes;
};

/**
 *  @brief Provides the net information for the per-cell connectivity information
 *
 *  This object is used inside the layout connectivity index to represent a net.
 */
class DB_PUBLIC LayoutConnectivityNet
{
public:
  /**
   *  @brief Represents a pin connecting an instance to a net
   */
  struct InstancePin
  {
  public:
    /**
     *  @brief The instance connected by the pin
     */
    db::Instance instance;

    /**
     *  @brief Name of the pin the referenced cell
     */
    db::property_names_id_type pin_name;
  };

  /**
   *  @brief Represents a pin shape
   */
  struct PinShape
  {
  public:
    /**
     *  @brief The layer the pin is on
     */
    unsigned int layer;

    /**
     *  @brief The shape representing the pin
     */
    db::Shape shape;

    /**
     *  @brief Name of the pin the referenced cell
     */
    db::property_names_id_type pin_name;
  };

  /**
   *  @brief Represents a net shape
   */
  struct NetShape
  {
  public:
    /**
     *  @brief The layer the pin is on
     */
    unsigned int layer;

    /**
     *  @brief The shape representing the pin
     */
    db::Shape shape;
  };

  /**
   *  Default constructor
   */
  LayoutConnectivityNet ()
    : net_name_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Keeps the net name
   */
  db::property_names_id_type net_name_id;

  /**
   *  @brief The pin shapes on that net
   *  The values are pairs of layer index and shape reference.
   *  Pins are ordinary shapes with LayoutPin information attached through properties.
   *  These are the pins leading upwards to parent cells. Pins leading to subcells
   *  are represented by "instance_pins".
   */
  tl::slist<PinShape> pins;

  /**
   *  @brief The ordinary shapes on the net
   *  The values are pairs of layer index and shape reference.
   */
  tl::slist<NetShape> shapes;

  /**
   *  @brief The instances (subcircuits or devices) on the net
   */
  tl::slist<InstancePin> instance_pins;
};

/**
 *  @brief Provides the per-cell connectivity information
 *
 *  This object is attached to a cell and keeps the connectivity index.
 *  The connectivity index is net-centric and keeps all shapes and instances
 *  connected to a net.
 *
 *  In addition to the nets, the connectivity index keeps the pins.
 *
 *  A connectivity index has three possible states:
 *  * not existing - no index is built
 *  * pins - the pins are listed
 *  * nets - the pins and nets are listed
 *
 *  "ensure_pins" will make sure the pin list is made.
 *  "ensure_nets" will make sure, pin and net list is made.
 *  These methods are typically called automatically.
 */
class DB_PUBLIC LayoutConnectivityIndex
{
public:
  typedef std::map<db::property_names_id_type, db::LayoutConnectivityNet, db::ComparePropertiesNameIds> nets_container;
  typedef std::map<db::property_names_id_type, db::LayoutConnectivityPin, db::ComparePropertiesNameIds> pins_container;

  typedef nets_container::const_iterator net_iterator;
  typedef pins_container::const_iterator pin_iterator;

  /**
   *  @brief Creates a connectivity index for the given cell
   *  The index will not register itself on the cell.
   */
  LayoutConnectivityIndex (const db::Cell *cell);

  /**
   *  @brief Gets the cell the index is associated with
   */
  const db::Cell *cell () const
  {
    return mp_cell;
  }

  /**
   *  @brief Clears the connectivity index
   */
  void clear ();

  /**
   *  @brief Ensures the pins are listed
   *  This method is called automatically when pin information is requested.
   */
  void ensure_pins () const;

  /**
   *  @brief Ensure that pins and nets are listed
   *  If not available yet, this information is collected from the
   *  shape and instance properties.
   *  This method is called automatically when net information is requested.
   */
  void ensure_nets () const;

  /**
   *  @brief Gets the net information for a net with the given name
   *  This method returns a null pointer if no net information is available for
   *  a net with the given name.
   */
  const LayoutConnectivityNet *net_by_name (const std::string &name) const;

  /**
   *  @brief Gets the net information for a net with the given name (by properties name ID)
   *  This method returns a null pointer if no net information is available for
   *  a net with the given name.
   */
  const LayoutConnectivityNet *net_by_name (db::property_names_id_type name_id) const;

  /**
   *  @brief Net iterator (begin)
   *  The iterator delivers a pair of net name ID and LayoutConnectivityNet information
   */
  net_iterator begin_nets () const;

  /**
   *  @brief Net iterator (end)
   */
  net_iterator end_nets () const;

  /**
   *  @brief Gets the pins for a given pin name
   *  This method returns a null pointer if there is no pin with the given name.
   */
  const LayoutConnectivityPin *pin_by_name (const std::string &name) const;

  /**
   *  @brief Gets the pins for a given pin name (by ID)
   *  This method returns a null pointer if there is no pin with the given name.
   */
  const LayoutConnectivityPin *pin_by_name (db::property_names_id_type name_id) const;

  /**
   *  @brief Pin iterator (begin)
   *  The iterator delivers a pair of pin name ID and LayoutConnectivityPin information
   */
  pin_iterator begin_pins () const;

  /**
   *  @brief Pin iterator (end)
   */
  pin_iterator end_pins () const;

private:
  mutable nets_container m_nets;
  mutable pins_container m_pins;
  const db::Cell *mp_cell;
  mutable bool m_pins_available;
  mutable bool m_nets_available;
};

}

/**
 *  @brief Special extractors for the objects
 */

namespace tl
{

  template<> inline bool test_extractor_impl (tl::Extractor &ex, db::LayoutPin &lp)
  {
    return lp.parse (ex);
  }

  template<> inline void extractor_impl (tl::Extractor &ex, db::LayoutPin &lp)
  {
    if (! test_extractor_impl (ex, lp)) {
      ex.error (tl::to_string (tr ("Expected a LayoutPin specification")));
    }
  }

  template<> inline bool test_extractor_impl (tl::Extractor &ex, db::LayoutInstanceConnections &lp)
  {
    return lp.parse (ex);
  }

  template<> inline void extractor_impl (tl::Extractor &ex, db::LayoutInstanceConnections &lp)
  {
    if (! test_extractor_impl (ex, lp)) {
      ex.error (tl::to_string (tr ("Expected a LayoutInstanceConnections specification")));
    }
  }

} // namespace tl

#endif
