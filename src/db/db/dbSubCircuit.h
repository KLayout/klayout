
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

#ifndef _HDR_dbSubCircuit
#define _HDR_dbSubCircuit

#include "dbCommon.h"
#include "dbTrans.h"
#include "dbNet.h"
#include "dbMemStatistics.h"

#include "tlObject.h"

#include <string>
#include <vector>

namespace db
{

class Circuit;

/**
 *  @brief A subcircuit of a circuit
 *
 *  This class essentially is a reference to another circuit
 */
class DB_PUBLIC SubCircuit
  : public db::NetlistObject
{
public:
  typedef tl::vector<const Net *> connected_net_list;

  /**
   *  @brief Default constructor
   */
  SubCircuit ();

  /**
   *  @brief Copy constructor
   */
  SubCircuit (const SubCircuit &other);

  /**
   *  @brief Assignment
   */
  SubCircuit &operator= (const SubCircuit &other);

  /**
   *  @brief Creates a subcircuit reference to the given circuit
   */
  SubCircuit (Circuit *circuit_ref, const std::string &name = std::string ());

  /**
   *  @brief Destructor
   */
  ~SubCircuit ();

  /**
   *  @brief Gets the subcircuit ID
   *  The ID is a unique integer which identifies the subcircuit.
   *  It can be used to retrieve the subcircuit from the circuit using Circuit::subcircuit_by_id.
   *  When assigned, the subcircuit ID is not 0.
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the circuit the subcircuit lives in (const version)
   *  This pointer is 0 if the subcircuit isn't added to a circuit
   */
  const Circuit *circuit () const
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the circuit the subcircuit lives in (non-const version)
   *  This pointer is 0 if the subcircuit isn't added to a circuit
   */
  Circuit *circuit ()
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the circuit the reference points to (const version)
   */
  const Circuit *circuit_ref () const
  {
    return m_circuit_ref.get ();
  }

  /**
   *  @brief Gets the circuit the reference points to (non-const version)
   */
  Circuit *circuit_ref ()
  {
    return m_circuit_ref.get ();
  }

  /**
   *  @brief Sets the name of the subcircuit
   *
   *  The name is one way to identify the subcircuit. The transformation is
   *  another one.
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name of the subcircuit
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
   *  @brief Sets the transformation describing the subcircuit
   *
   *  The transformation is a natural description of a subcircuit
   *  (in contrast to the name) when deriving it from a layout.
   */
  void set_trans (const db::DCplxTrans &trans);

  /**
   *  @brief Gets the transformation describing the subcircuit
   */
  const db::DCplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Gets the net attached to a specific pin
   *  Returns 0 if no net is attached.
   */
  const Net *net_for_pin (size_t pin_id) const;

  /**
   *  @brief Gets the net attached to a specific pin (non-const version)
   *  Returns 0 if no net is attached.
   */
  Net *net_for_pin (size_t pin_id)
  {
    return const_cast<Net *> (((const SubCircuit *) this)->net_for_pin (pin_id));
  }

  /**
   *  @brief Gets the net attached to a specific pin as a subcircuit pin ref object
   *  Returns 0 if no net is attached.
   */
  const NetSubcircuitPinRef *netref_for_pin (size_t pin_id) const;

  /**
   *  @brief Gets the net attached to a specific pin as a subcircuit pin ref object (non-const version)
   *  Returns 0 if no net is attached.
   */
  NetSubcircuitPinRef *netref_for_pin (size_t pin_id)
  {
    return const_cast<NetSubcircuitPinRef *> (((const SubCircuit *) this)->netref_for_pin (pin_id));
  }

  /**
   *  @brief Connects the given pin to the given net
   *  If the net is 0 the pin is disconnected.
   *  If non-null, a NetPinRef object will be inserted into the
   *  net and connected with the given pin.
   */
  void connect_pin (size_t pin_id, Net *net);

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_trans, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_pin_refs, true, (void *) this);
  }

private:
  friend class Circuit;
  friend class Net;

  tl::weak_ptr<Circuit> m_circuit_ref;
  std::string m_name;
  db::DCplxTrans m_trans;
  std::vector<Net::subcircuit_pin_iterator> m_pin_refs;
  size_t m_id;
  Circuit *mp_circuit;

  /**
   *  @brief Sets the pin reference for a specific pin
   */
  void set_pin_ref_for_pin (size_t ppin_id, Net::subcircuit_pin_iterator iter);

  /**
   *  @brief Sets the circuit reference
   */
  void set_circuit_ref (Circuit *c);

  /**
   *  @brief Erases the given pin reference
   */
  void erase_pin (size_t pin_id);

  /**
   *  @brief Sets the circuit the subcircuit belongs to
   */
  void set_circuit (Circuit *c)
  {
    mp_circuit = c;
  }

  /**
   *  @brief Sets the device ID
   */
  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief Memory statistics for SubCircuit
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const SubCircuit &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
