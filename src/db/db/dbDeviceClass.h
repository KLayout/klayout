
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

#ifndef _HDR_dbDeviceClass
#define _HDR_dbDeviceClass

#include "dbCommon.h"
#include "dbMemStatistics.h"

#include "gsiObject.h"
#include "tlObject.h"
#include "tlUniqueId.h"

#include <string>
#include <vector>

namespace db
{

class Netlist;
class Device;

/**
 *  @brief A device terminal definition
 */
class DB_PUBLIC DeviceTerminalDefinition
{
public:
  /**
   *  @brief Creates an empty device terminal definition
   */
  DeviceTerminalDefinition ()
    : m_name (), m_description (), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a device terminal definition with the given name and description
   */
  DeviceTerminalDefinition (const std::string &name, const std::string &description)
    : m_name (name), m_description (description), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the terminal name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the terminal name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the terminal description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the terminal description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the terminal ID
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_description, true, (void *) this);
  }

private:
  friend class DeviceClass;

  std::string m_name, m_description;
  size_t m_id;

  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief Memory statistics for DeviceTerminalDefinition
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const DeviceTerminalDefinition &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A device parameter definition
 */
class DB_PUBLIC DeviceParameterDefinition
{
public:
  /**
   *  @brief Creates an empty device parameter definition
   */
  DeviceParameterDefinition ()
    : m_name (), m_description (), m_default_value (0.0), m_id (0), m_is_primary (true), m_si_scaling (1.0), m_geo_scaling (0.0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a device parameter definition with the given name and description
   */
  DeviceParameterDefinition (const std::string &name, const std::string &description, double default_value = 0.0, bool is_primary = true, double si_scaling = 1.0, double geo_scaling = 0.0)
    : m_name (name), m_description (description), m_default_value (default_value), m_id (0), m_is_primary (is_primary), m_si_scaling (si_scaling), m_geo_scaling (geo_scaling)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the parameter name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the parameter name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the parameter description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the parameter description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the SI unit scaling factor
   *
   *  Some parameters are given in micrometers - for example W and L of MOS devices. This
   *  scaling factor gives the translation to SI units (1e-6 for micrometers).
   */
  double si_scaling () const
  {
    return m_si_scaling;
  }

  /**
   *  @brief Set the SI unit scaling factor
   */
  void set_si_scaling (double s)
  {
    m_si_scaling = s;
  }

  /**
   *  @brief Gets the geometry scaling exponent
   *
   *  The geometry scaling exponent is used for example when applying .option scale
   *  in Spice reading. It is 0 for "no scaling", 1 for linear scaling and 2 for
   *  quadratic scaling.
   */
  double geo_scaling_exponent () const
  {
    return m_geo_scaling;
  }

  /**
   *  @brief Sets the geometry scaling exponent
   */
  void set_geo_scaling_exponent (double e)
  {
    m_geo_scaling = e;
  }

  /**
   *  @brief Gets the parameter default value
   */
  double default_value () const
  {
    return m_default_value;
  }

  /**
   *  @brief Sets the parameter default value
   */
  void set_default_value (double d)
  {
    m_default_value = d;
  }

  /**
   *  @brief Sets a value indicating whether the parameter is a primary parameter
   *
   *  If this flag is set to true (the default), the parameter is considered a primary parameter.
   *  Only primary parameters are compared by default.
   */
  void set_is_primary (bool p)
  {
    m_is_primary = p;
  }

  /**
   *  @brief Gets a value indicating whether the parameter is a primary parameter
   */
  bool is_primary () const
  {
    return m_is_primary;
  }

  /**
   *  @brief Gets the parameter ID
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_description, true, (void *) this);
  }

private:
  friend class DeviceClass;

  std::string m_name, m_description;
  double m_default_value;
  size_t m_id;
  bool m_is_primary;
  double m_si_scaling;
  double m_geo_scaling;

  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief Memory statistics for DeviceParameterDefinition
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const DeviceParameterDefinition &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A device parameter compare delegate
 *
 *  Device parameter compare delegates are used to establish
 *  device equivalence in the context of netlist comparison.
 */
class DB_PUBLIC DeviceParameterCompareDelegate
  : public gsi::ObjectBase, public tl::Object
{
public:
  DeviceParameterCompareDelegate () { }
  virtual ~DeviceParameterCompareDelegate () { }

  virtual bool less (const db::Device &a, const db::Device &b) const = 0;
};

/**
 *  @brief A parameter compare delegate that compares several parameters either relative or absolute (or both)
 *
 *  The reasoning behind this class is to supply a chainable compare delegate: ab = a + b
 *  where a and b are compare delegates for two different parameters and ab is the combined compare delegate.
 */
class DB_PUBLIC EqualDeviceParameters
  : public DeviceParameterCompareDelegate
{
public:
  EqualDeviceParameters ();
  EqualDeviceParameters (size_t parameter_id, bool ignore = false);
  EqualDeviceParameters (size_t parameter_id, double absolute, double relative);

  virtual bool less (const db::Device &a, const db::Device &b) const;

  //  for test purposes
  std::string to_string () const;

  EqualDeviceParameters &operator+= (const EqualDeviceParameters &other);

  EqualDeviceParameters operator+ (const EqualDeviceParameters &other) const
  {
    EqualDeviceParameters pc (*this);
    pc += other;
    return pc;
  }

private:
  std::vector<std::pair<size_t, std::pair<double, double> > > m_compare_set;
};

/**
 *  @brief A parameter compare delegate that compares all parameters in a relative fashion
 */
class DB_PUBLIC AllDeviceParametersAreEqual
  : public DeviceParameterCompareDelegate
{
public:
  AllDeviceParametersAreEqual (double relative);

  virtual bool less (const db::Device &a, const db::Device &b) const;

private:
  double m_relative;
};

/**
 *  @brief A device combiner
 *
 *  The device combiner is a delegate that combines devices
 */
class DB_PUBLIC DeviceCombiner
  : public gsi::ObjectBase, public tl::Object
{
public:
  DeviceCombiner () { }
  virtual ~DeviceCombiner () { }

  /**
   *  @brief Combines two devices
   *
   *  This method shall test, whether the two devices can be combined. Both devices
   *  are guaranteed to share the same device class.
   *  If they cannot be combined, this method shall do nothing and return false.
   *  If they can be combined, this method shall reconnect the nets of the first
   *  device and entirely disconnect the nets of the second device.
   *  The second device will be deleted afterwards.
   */
  virtual bool combine_devices (db::Device *a, db::Device *b) const = 0;
};

/**
 *  @brief A device class
 *
 *  A device class describes a type of device.
 */
class DB_PUBLIC DeviceClass
  : public gsi::ObjectBase, public tl::Object, public tl::UniqueId
{
public:
  typedef size_t terminal_id_type;

  /**
   *  @brief Constructor
   *
   *  Creates an empty circuit.
   */
  DeviceClass ();

  /**
   *  @brief Copy constructor
   *  NOTE: do not use this copy constructor as the device class
   *  is intended for subclassing.
   */
  DeviceClass (const DeviceClass &other);

  /**
   *  @brief Assignment
   *  NOTE: do not use this copy constructor as the device class
   *  is intended for subclassing.
   */
  DeviceClass &operator= (const DeviceClass &other);

  /**
   *  @brief Gets the netlist the device class lives in
   */
  db::Netlist *netlist ()
  {
    return mp_netlist;
  }

  /**
   *  @brief Gets the netlist the device class lives in (const version)
   */
  const db::Netlist *netlist () const
  {
    return mp_netlist;
  }

  /**
   *  @brief Sets a value indicating whether this class performs strict terminal mapping
   *
   *  Classes with this flag set don't allow terminal swapping, independently of the
   *  "normalize_terminal_id" implementation. If two classes are involved in a compare,
   *  both classes are treated strict if one of them operates in strict mode.
   */
  void set_strict (bool s)
  {
    m_strict = s;
  }

  /**
   *  @brief Gets a value indicating whether this class performs strict terminal mapping
   */
  bool is_strict () const
  {
    return m_strict;
  }

  /**
   *  @brief Gets the name of the device class
   *
   *  The name is a formal name which identifies the class.
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the device name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the description text for the device class
   *
   *  The description text is a human-readable text that
   *  identifies the device class.
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the description text
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the terminal definitions
   *
   *  The terminal definitions indicate what terminals the device offers.
   *  The number of terminals is constant per class. The index of the terminal
   *  is used as an ID of the terminal, hence the order must be static.
   */
  const std::vector<DeviceTerminalDefinition> &terminal_definitions () const
  {
    return m_terminal_definitions;
  }

  /**
   *  @brief Adds a terminal definition
   */
  const DeviceTerminalDefinition &add_terminal_definition (const DeviceTerminalDefinition &pd);

  /**
   *  @brief Clears the terminal definition
   */
  void clear_terminal_definitions ();

  /**
   *  @brief Gets the terminal definition from the ID
   */
  const DeviceTerminalDefinition *terminal_definition (size_t id) const;

  /**
   *  @brief Gets the parameter definitions
   */
  const std::vector<DeviceParameterDefinition> &parameter_definitions () const
  {
    return m_parameter_definitions;
  }

  /**
   *  @brief Gets the parameter definitions
   */
  std::vector<DeviceParameterDefinition> &parameter_definitions_non_const ()
  {
    return m_parameter_definitions;
  }

  /**
   *  @brief Adds a parameter definition
   */
  const DeviceParameterDefinition &add_parameter_definition (const DeviceParameterDefinition &pd);

  /**
   *  @brief Clears the parameter definition
   */
  void clear_parameter_definitions ();

  /**
   *  @brief Gets the parameter definition from the ID
   */
  const DeviceParameterDefinition *parameter_definition (size_t id) const;

  /**
   *  @brief Gets the parameter definition from the ID (non-const version)
   */
  DeviceParameterDefinition *parameter_definition_non_const (size_t id);

  /**
   *  @brief Returns true, if the device has a parameter with the given name
   */
  bool has_parameter_with_name (const std::string &name) const;

  /**
   *  @brief Returns the parameter ID for the parameter with the given name
   *  If the name is invalid, an exception is thrown.
   */
  size_t parameter_id_for_name (const std::string &name) const;

  /**
   *  @brief Returns true, if the device has a terminal with the given name
   */
  bool has_terminal_with_name (const std::string &name) const;

  /**
   *  @brief Returns the parameter ID for the terminal with the given name
   *  If the name is invalid, an exception is thrown.
   */
  size_t terminal_id_for_name (const std::string &name) const;

  /**
   *  @brief Clones the device class
   */
  virtual DeviceClass *clone () const
  {
    return new DeviceClass (*this);
  }

  /**
   *  @brief Combines two devices
   *
   *  This method shall test, whether the two devices can be combined. Both devices
   *  are guaranteed to share the same device class (this).
   *  If they cannot be combined, this method shall do nothing and return false.
   *  If they can be combined, this method shall reconnect the nets of the first
   *  device and entirely disconnect the nets of the second device.
   *  The second device will be deleted afterwards.
   */
  bool combine_devices (db::Device *a, db::Device *b) const
  {
    return mp_device_combiner.get () ? mp_device_combiner->combine_devices (a, b) : false;
  }

  /**
   *  @brief Returns true if the device class supports device combination in parallel mode
   */
  bool supports_parallel_combination () const
  {
    return m_supports_parallel_combination;
  }

  /**
   *  @brief Returns true if the device class supports device combination in serial mode
   */
  bool supports_serial_combination () const
  {
    return m_supports_serial_combination;
  }

  /**
   *  @brief Sets a value indicating that the class supports device combination in parallel mode
   */
  void set_supports_parallel_combination (bool f)
  {
    m_supports_parallel_combination = f;
  }

  /**
   *  @brief Sets a value indicating that the class supports device combination in serial mode
   */
  void set_supports_serial_combination (bool f)
  {
    m_supports_serial_combination = f;
  }

  /**
   *  @brief Marks two terminals as equivalent (swappable)
   */
  void equivalent_terminal_id (size_t tid, size_t equiv_tid)
  {
    m_equivalent_terminal_ids.insert (std::make_pair (tid, equiv_tid));
  }

  /**
   *  @brief Clears all equivalent terminal ids
   */
  void clear_equivalent_terminal_ids ()
  {
    m_equivalent_terminal_ids.clear ();
  }

  /**
   *  @brief Normalizes the terminal IDs to indicate terminal swapping
   *
   *  This method returns a "normalized" terminal ID. For example, for MOS
   *  transistors where S and D can be exchanged, D will be mapped to S.
   */
  size_t normalize_terminal_id (size_t tid) const
  {
    std::map<size_t, size_t>::const_iterator ntid = m_equivalent_terminal_ids.find (tid);
    if (ntid != m_equivalent_terminal_ids.end ()) {
      return ntid->second;
    } else {
      return tid;
    }
  }

  /**
   *  @brief Compares the parameters of the devices a and b
   *
   *  a and b are expected to originate from this or an equivalent device class having
   *  the same parameters.
   *  This is the "less" operation. If a parameter compare delegate is registered, this
   *  compare request will be forwarded to the delegate.
   *
   *  If two devices with different device classes are compared and only one of
   *  the classes features a delegate, the one with the delegate is employed.
   */
  static bool less (const db::Device &a, const db::Device &b);

  /**
   *  @brief Compares the parameters of the devices a and b
   *
   *  a and b are expected to originate from this or an equivalent device class having
   *  the same parameters.
   *  This is the "equal" operation. If a parameter compare delegate is registered, this
   *  compare request will be forwarded to the delegate.
   *
   *  If two devices with different device classes are compared and only one of
   *  the classes features a delegate, the one with the delegate is employed.
   */
  static bool equal (const db::Device &a, const db::Device &b);

  /**
   *  @brief Registers a compare delegate
   *
   *  The reasoning behind choosing a delegate is that a delegate is efficient
   *  also in scripts if one of the standard delegates is taken.
   *
   *  The device class takes ownership of the delegate.
   */
  void set_parameter_compare_delegate (db::DeviceParameterCompareDelegate *delegate)
  {
    if (delegate) {
      delegate->keep ();  //  assume transfer of ownership for scripts
    }
    mp_pc_delegate.reset (delegate);
  }

  /**
   *  @brief Gets the parameter compare delegate or null if no such delegate is registered
   */
  const db::DeviceParameterCompareDelegate *parameter_compare_delegate () const
  {
    return mp_pc_delegate.get ();
  }

  /**
   *  @brief Gets the parameter compare delegate or null if no such delegate is registered (non-const version)
   */
  db::DeviceParameterCompareDelegate *parameter_compare_delegate ()
  {
    return mp_pc_delegate.get ();
  }

  /**
   *  @brief Registers a device combiner
   *
   *  The device class takes ownership of the combiner.
   */
  void set_device_combiner (db::DeviceCombiner *combiner)
  {
    if (combiner) {
      combiner->keep ();  //  assume transfer of ownership for scripts
    }
    mp_device_combiner.reset (combiner);
  }

  /**
   *  @brief Gets the device combiner or null if no such delegate is registered
   */
  const db::DeviceCombiner *device_combiner () const
  {
    return mp_device_combiner.get ();
  }

  /**
   *  @brief Gets the device combiner or null if no such delegate is registered (non-const version)
   */
  db::DeviceCombiner *device_combiner ()
  {
    return mp_device_combiner.get ();
  }

  /**
   *  @brief Internally used by the netlist comparer to temporarily attach a device class pointing to the primary one
   */
  void set_primary_class (const db::DeviceClass *primary) const
  {
    mp_primary_class = primary;
  }

  /**
   *  @brief Internally used by the netlist comparer to temporarily attach a device class pointing to the primary one
   */
  const db::DeviceClass *primary_class () const
  {
    return mp_primary_class;
  }

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_description, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_terminal_definitions, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_parameter_definitions, true, (void *) this);
  }

private:
  friend class Netlist;

  std::string m_name, m_description;
  std::vector<DeviceTerminalDefinition> m_terminal_definitions;
  std::vector<DeviceParameterDefinition> m_parameter_definitions;
  bool m_strict;
  db::Netlist *mp_netlist;
  tl::shared_ptr<db::DeviceParameterCompareDelegate> mp_pc_delegate;
  tl::shared_ptr<db::DeviceCombiner> mp_device_combiner;
  bool m_supports_parallel_combination;
  bool m_supports_serial_combination;
  std::map<size_t, size_t> m_equivalent_terminal_ids;
  mutable const db::DeviceClass *mp_primary_class;

  void set_netlist (db::Netlist *nl)
  {
    mp_netlist = nl;
  }
};

/**
 *  @brief Memory statistics for DeviceClass
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const DeviceClass &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A device class template
 *
 *  This is a registered class which provides a device class template.
 *  The built-in classes serve as templates and registering a template
 *  allows regenerating the class from an abstract description (template
 *  name).
 *
 *  NOTE: device classes derived from one of the built-in classes
 *  cannot be distinguished from pure built-in classes. Entirely
 *  customized classes are treated as "non-template based" (i.e.
 *  "is_a" returns 0).
 */
class DB_PUBLIC DeviceClassTemplateBase
{
public:
  DeviceClassTemplateBase (const std::string &name)
    : m_name (name)
  {
    //  .. nothing yet ..
  }

  virtual ~DeviceClassTemplateBase () { }

  const std::string &name () const
  {
    return m_name;
  }

  virtual bool is_of (const db::DeviceClass *) const = 0;
  virtual DeviceClass *create () const = 0;

  static DeviceClassTemplateBase *template_by_name (const std::string &name);
  static DeviceClassTemplateBase *is_a (const db::DeviceClass *dc);

private:
  std::string m_name;

  DeviceClassTemplateBase (const DeviceClassTemplateBase &);
  DeviceClassTemplateBase &operator= (const DeviceClassTemplateBase &);
};

template <class T>
class DB_PUBLIC_TEMPLATE device_class_template
  : public DeviceClassTemplateBase
{
public:
  device_class_template (const std::string &name)
    : DeviceClassTemplateBase (name)
  {
    //  .. nothing yet ..
  }

  virtual bool is_of (const db::DeviceClass *dc) const
  {
    return dynamic_cast<const T *> (dc) != 0;
  }

  virtual DeviceClass *create () const
  {
    return new T ();
  }

private:
  device_class_template (const device_class_template &);
  device_class_template &operator= (const device_class_template &);
};

}

#endif
