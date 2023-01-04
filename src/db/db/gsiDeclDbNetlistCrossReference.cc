
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

#include "gsiDecl.h"
#include "gsiEnums.h"
#include "dbNetlistCrossReference.h"

namespace gsi
{

namespace
{

struct CircuitPairData
{
  typedef db::Circuit object_type;

  CircuitPairData (const db::Circuit *a, const db::Circuit *b, db::NetlistCrossReference::Status s) : pair (a, b), status (s) { }
  CircuitPairData () : pair ((const db::Circuit *)0, (const db::Circuit *)0), status (db::NetlistCrossReference::None) { }

  std::pair<const db::Circuit *, const db::Circuit *> pair;
  db::NetlistCrossReference::Status status;
};

template <class PairData>
static const typename PairData::object_type *first (const PairData *data)
{
  return data->pair.first;
}

template <class PairData>
static const typename PairData::object_type *second (const PairData *data)
{
  return data->pair.second;
}

template <class PairData>
static db::NetlistCrossReference::Status status (const PairData *data)
{
  return data->status;
}

template <class PairData>
class PairDataClass
  : public ChildClass<db::NetlistCrossReference, PairData>
{
public:
  PairDataClass (const std::string &module, const std::string &name, const std::string &doc)
    : gsi::ChildClass<db::NetlistCrossReference, PairData> (module, name,
      gsi::method_ext ("first", &first<PairData>,
        "@brief Gets the first object of the relation pair.\n"
        "The first object is usually the one obtained from the layout-derived netlist. "
        "This member can be nil if the pair is describing a non-matching reference object. "
        "In this case, the \\second member is the reference object for which no match was found."
      ) +
      gsi::method_ext ("second", &second<PairData>,
        "@brief Gets the second object of the relation pair.\n"
        "The first object is usually the one obtained from the reference netlist. "
        "This member can be nil if the pair is describing a non-matching layout object. "
        "In this case, the \\first member is the layout-derived object for which no match was found."
      ) +
      gsi::method_ext ("status", &status<PairData>,
        "@brief Gets the status of the relation.\n"
        "This enum described the match status of the relation pair. "
      ),
      doc +
      "\n"
      "Upon successful match, the \\first and \\second members are the matching objects and \\status is 'Match'."
      "\n"
      "This object is also used to describe non-matches or match errors. In this case, \\first or \\second may be nil and "
      "\\status further describes the case."
    )
  { }
};

template <class Obj>
static const Obj *first_of_pair (const std::pair<const Obj *, const Obj *> *pair)
{
  return pair->first;
}

template <class Obj>
static const Obj *second_of_pair (const std::pair<const Obj *, const Obj *> *pair)
{
  return pair->second;
}

template <class Obj>
class NetObjectPairClass
  : public ChildClass<db::NetlistCrossReference, std::pair<const Obj *, const Obj *> >
{
public:
  NetObjectPairClass (const std::string &module, const std::string &name, const std::string &doc)
    : gsi::ChildClass<db::NetlistCrossReference, std::pair<const Obj *, const Obj *> > (module, name,
      gsi::method_ext ("first", &first_of_pair<Obj>,
        "@brief Gets the first object of the relation pair.\n"
        "The first object is usually the one obtained from the layout-derived netlist. "
        "This member can be nil if the pair is describing a non-matching reference object. "
        "In this case, the \\second member is the reference object for which no match was found."
      ) +
      gsi::method_ext ("second", &second_of_pair<Obj>,
        "@brief Gets the second object of the relation pair.\n"
        "The first object is usually the one obtained from the reference netlist. "
        "This member can be nil if the pair is describing a non-matching layout object. "
        "In this case, the \\first member is the layout-derived object for which no match was found."
      ),
      doc +
      "\n"
      "Upon successful match, the \\first and \\second members are the matching net objects."
      "Otherwise, either \\first or \\second is nil and the other member is the object for "
      "which no match was found."
    )
  { }
};

}

PairDataClass<db::NetlistCrossReference::NetPairData> decl_dbNetlistCrossReference_NetPairData ("db", "NetPairData",
  "@brief A net match entry.\n"
  "This object is used to describe the relationship of two nets in a netlist match.\n"
);

PairDataClass<db::NetlistCrossReference::DevicePairData> decl_dbNetlistCrossReference_DevicePairData ("db", "DevicePairData",
  "@brief A device match entry.\n"
  "This object is used to describe the relationship of two devices in a netlist match.\n"
);

PairDataClass<db::NetlistCrossReference::PinPairData> decl_dbNetlistCrossReference_PinPairData ("db", "PinPairData",
  "@brief A pin match entry.\n"
  "This object is used to describe the relationship of two circuit pins in a netlist match.\n"
);

PairDataClass<db::NetlistCrossReference::SubCircuitPairData> decl_dbNetlistCrossReference_SubCircuitPairData ("db", "SubCircuitPairData",
  "@brief A subcircuit match entry.\n"
  "This object is used to describe the relationship of two subcircuits in a netlist match.\n"
);

PairDataClass<CircuitPairData> decl_dbNetlistCrossReference_CircuitPairData ("db", "CircuitPairData",
  "@brief A circuit match entry.\n"
  "This object is used to describe the relationship of two circuits in a netlist match.\n"
);

NetObjectPairClass<db::NetTerminalRef> decl_dbNetlistCrossReference_NetTerminalRefPair ("db", "NetTerminalRefPair",
  "@brief A match entry for a net terminal pair.\n"
  "This object is used to describe the matching terminal pairs or non-matching terminals on a net.\n"
);

NetObjectPairClass<db::NetPinRef> decl_dbNetlistCrossReference_NetPinRefPair ("db", "NetPinRefPair",
  "@brief A match entry for a net pin pair.\n"
  "This object is used to describe the matching pin pairs or non-matching pins on a net.\n"
);

NetObjectPairClass<db::NetSubcircuitPinRef> decl_dbNetlistCrossReference_NetSubcircuitPinRefPair ("db", "NetSubcircuitPinRefPair",
  "@brief A match entry for a net subcircuit pin pair.\n"
  "This object is used to describe the matching subcircuit pin pairs or non-matching subcircuit pins on a net.\n"
);

extern Class<db::NetlistCompareLogger> decl_dbNetlistCompareLogger;

namespace {

class CircuitPairIterator
{
public:
  //  makes STL happy:
  typedef std::forward_iterator_tag iterator_category;
  typedef CircuitPairData value_type;
  typedef size_t difference_type;
  typedef const CircuitPairData *pointer;
  typedef const CircuitPairData &reference;

  CircuitPairIterator (db::NetlistCrossReference *xref)
    : m_xref (xref), m_iter (xref->begin_circuits ()), m_end_iter (xref->end_circuits ())
  { }

  bool at_end () const
  {
    return m_xref.get () == 0 || m_iter == m_end_iter;
  }

  CircuitPairIterator &operator++ ()
  {
    ++m_iter;
    return *this;
  }

  pointer operator-> () const
  {
    m_data.pair = *m_iter;
    const db::NetlistCrossReference::PerCircuitData *data = m_xref->per_circuit_data_for (*m_iter);
    tl_assert (data != 0);
    m_data.status = data->status;
    return &m_data;
  }

  reference operator* () const
  {
    return *operator-> ();
  }

  tl::weak_ptr<db::NetlistCrossReference> m_xref;
  mutable CircuitPairData m_data;
  db::NetlistCrossReference::circuits_iterator m_iter, m_end_iter;
};

template <class PairData, class PairDataIter>
class pair_data_iterator
{
public:
  //  makes STL happy:
  typedef std::forward_iterator_tag iterator_category;
  typedef PairData value_type;
  typedef size_t difference_type;
  typedef const PairData *pointer;
  typedef const PairData &reference;

  pair_data_iterator ()
    : m_xref (), m_iter (), m_end_iter ()
  { }

  pair_data_iterator (db::NetlistCrossReference *xref, const PairDataIter &iter, const PairDataIter &end_iter)
    : m_xref (xref), m_iter (iter), m_end_iter (end_iter)
  { }

  bool at_end () const
  {
    return m_xref.get () == 0 || m_iter == m_end_iter;
  }

  pair_data_iterator &operator++ ()
  {
    ++m_iter;
    return *this;
  }

  pointer operator-> () const
  {
    return m_iter.operator-> ();
  }

  reference operator* () const
  {
    return *operator-> ();
  }

  tl::weak_ptr<db::NetlistCrossReference> m_xref;
  PairDataIter m_iter, m_end_iter;
};

}

static CircuitPairIterator each_circuit_pair (db::NetlistCrossReference *xref)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  return CircuitPairIterator (xref);
}

static pair_data_iterator<db::NetlistCrossReference::NetPairData, db::NetlistCrossReference::PerCircuitData::net_pairs_const_iterator> each_net_pair (db::NetlistCrossReference *xref, const CircuitPairData &circuit_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<db::NetlistCrossReference::NetPairData, db::NetlistCrossReference::PerCircuitData::net_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerCircuitData *data = xref->per_circuit_data_for (circuit_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->nets.begin (), data->nets.end ());
  }
}

static pair_data_iterator<db::NetlistCrossReference::DevicePairData, db::NetlistCrossReference::PerCircuitData::device_pairs_const_iterator> each_device_pair (db::NetlistCrossReference *xref, const CircuitPairData &circuit_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<db::NetlistCrossReference::DevicePairData, db::NetlistCrossReference::PerCircuitData::device_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerCircuitData *data = xref->per_circuit_data_for (circuit_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->devices.begin (), data->devices.end ());
  }
}

static pair_data_iterator<db::NetlistCrossReference::PinPairData, db::NetlistCrossReference::PerCircuitData::pin_pairs_const_iterator> each_pin_pair (db::NetlistCrossReference *xref, const CircuitPairData &circuit_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<db::NetlistCrossReference::PinPairData, db::NetlistCrossReference::PerCircuitData::pin_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerCircuitData *data = xref->per_circuit_data_for (circuit_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->pins.begin (), data->pins.end ());
  }
}

static pair_data_iterator<db::NetlistCrossReference::SubCircuitPairData, db::NetlistCrossReference::PerCircuitData::subcircuit_pairs_const_iterator> each_subcircuit_pair (db::NetlistCrossReference *xref, const CircuitPairData &circuit_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<db::NetlistCrossReference::SubCircuitPairData, db::NetlistCrossReference::PerCircuitData::subcircuit_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerCircuitData *data = xref->per_circuit_data_for (circuit_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->subcircuits.begin (), data->subcircuits.end ());
  }
}

static pair_data_iterator<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *>, db::NetlistCrossReference::PerNetData::terminal_pairs_const_iterator> each_net_terminal_pair (db::NetlistCrossReference *xref, const db::NetlistCrossReference::NetPairData &net_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *>, db::NetlistCrossReference::PerNetData::terminal_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerNetData *data = xref->per_net_data_for (net_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->terminals.begin (), data->terminals.end ());
  }
}

static pair_data_iterator<std::pair<const db::NetPinRef *, const db::NetPinRef *>, db::NetlistCrossReference::PerNetData::pin_pairs_const_iterator> each_net_pin_pair (db::NetlistCrossReference *xref, const db::NetlistCrossReference::NetPairData &net_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<std::pair<const db::NetPinRef *, const db::NetPinRef *>, db::NetlistCrossReference::PerNetData::pin_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerNetData *data = xref->per_net_data_for (net_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->pins.begin (), data->pins.end ());
  }
}

static pair_data_iterator<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *>, db::NetlistCrossReference::PerNetData::subcircuit_pin_pairs_const_iterator> each_net_subcircuit_pin_pair (db::NetlistCrossReference *xref, const db::NetlistCrossReference::NetPairData &net_pair)
{
  tl_assert (xref->netlist_a () != 0 && xref->netlist_b () != 0);
  typedef pair_data_iterator<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *>, db::NetlistCrossReference::PerNetData::subcircuit_pin_pairs_const_iterator> iter_type;

  const db::NetlistCrossReference::PerNetData *data = xref->per_net_data_for (net_pair.pair);
  if (! data) {
    return iter_type ();
  } else {
    return iter_type (xref, data->subcircuit_pins.begin (), data->subcircuit_pins.end ());
  }
}

Class<db::NetlistCrossReference> decl_dbNetlistCrossReference (decl_dbNetlistCompareLogger, "db", "NetlistCrossReference",
  gsi::iterator_ext ("each_circuit_pair", &each_circuit_pair,
    "@brief Delivers the circuit pairs and their status.\n"
    "See the class description for details."
  ) +
  gsi::iterator_ext ("each_net_pair", &each_net_pair, gsi::arg ("circuit_pair"),
    "@brief Delivers the net pairs and their status for the given circuit pair.\n"
    "See the class description for details."
  ) +
  gsi::iterator_ext ("each_device_pair", &each_device_pair, gsi::arg ("circuit_pair"),
    "@brief Delivers the device pairs and their status for the given circuit pair.\n"
    "See the class description for details."
  ) +
  gsi::iterator_ext ("each_pin_pair", &each_pin_pair, gsi::arg ("circuit_pair"),
    "@brief Delivers the pin pairs and their status for the given circuit pair.\n"
    "See the class description for details."
  ) +
  gsi::iterator_ext ("each_subcircuit_pair", &each_subcircuit_pair, gsi::arg ("circuit_pair"),
    "@brief Delivers the subcircuit pairs and their status for the given circuit pair.\n"
    "See the class description for details."
  ) +
  gsi::iterator_ext ("each_net_terminal_pair", &each_net_terminal_pair, gsi::arg ("net_pair"),
    "@brief Delivers the device terminal pairs for the given net pair.\n"
    "For the net pair, lists the device terminal pairs identified on this net."
  ) +
  gsi::iterator_ext ("each_net_pin_pair", &each_net_pin_pair, gsi::arg ("net_pair"),
    "@brief Delivers the pin pairs for the given net pair.\n"
    "For the net pair, lists the pin pairs identified on this net."
  ) +
  gsi::iterator_ext ("each_net_subcircuit_pin_pair", &each_net_subcircuit_pin_pair, gsi::arg ("net_pair"),
    "@brief Delivers the subcircuit pin pairs for the given net pair.\n"
    "For the net pair, lists the subcircuit pin pairs identified on this net."
  ) +
  gsi::method ("other_net_for", &db::NetlistCrossReference::other_net_for, gsi::arg ("net"),
    "@brief Gets the matching other net for a given primary net.\n"
    "The return value will be nil if no match is found. "
    "Otherwise it is the 'b' net for nets from the 'a' netlist and vice versa."
  ) +
  gsi::method ("other_circuit_for", &db::NetlistCrossReference::other_circuit_for, gsi::arg ("circuit"),
    "@brief Gets the matching other circuit for a given primary circuit.\n"
    "The return value will be nil if no match is found. "
    "Otherwise it is the 'b' circuit for circuits from the 'a' netlist and vice versa."
    "\n\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("other_device_for", &db::NetlistCrossReference::other_device_for, gsi::arg ("device"),
    "@brief Gets the matching other device for a given primary device.\n"
    "The return value will be nil if no match is found. "
    "Otherwise it is the 'b' device for devices from the 'a' netlist and vice versa."
    "\n\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("other_pin_for", &db::NetlistCrossReference::other_pin_for, gsi::arg ("pin"),
    "@brief Gets the matching other pin for a given primary pin.\n"
    "The return value will be nil if no match is found. "
    "Otherwise it is the 'b' pin for pins from the 'a' netlist and vice versa."
    "\n\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("other_subcircuit_for", &db::NetlistCrossReference::other_subcircuit_for, gsi::arg ("subcircuit"),
    "@brief Gets the matching other subcircuit for a given primary subcircuit.\n"
    "The return value will be nil if no match is found. "
    "Otherwise it is the 'b' subcircuit for subcircuits from the 'a' netlist and vice versa."
    "\n\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("clear", &db::NetlistCrossReference::clear,
    "@hide\n"
  ) +
  gsi::method ("circuit_count", &db::NetlistCrossReference::circuit_count,
    "@brief Gets the number of circuit pairs in the cross-reference object."
  ) +
  gsi::method ("netlist_a", &db::NetlistCrossReference::netlist_a,
    "@brief Gets the first netlist which participated in the compare.\n"
    "This member may be nil, if the respective netlist is no longer valid. "
    "In this case, the netlist cross-reference object cannot be used."
  ) +
  gsi::method ("netlist_b", &db::NetlistCrossReference::netlist_b,
    "@brief Gets the second netlist which participated in the compare.\n"
    "This member may be nil, if the respective netlist is no longer valid."
    "In this case, the netlist cross-reference object cannot be used."
  ),
  "@brief Represents the identity mapping between the objects of two netlists.\n"
  "\n"
  "The NetlistCrossReference object is a container for the results of a netlist comparison. "
  "It implemented the \\NetlistCompareLogger interface, hence can be used as output for "
  "a netlist compare operation (\\NetlistComparer#compare). It's purpose is to store the "
  "results of the compare. It is used in this sense inside the \\LayoutVsSchematic framework.\n"
  "\n"
  "The basic idea of the cross reference object is pairing: the netlist comparer will try "
  "to identify matching items and store them as pairs inside the cross reference object. "
  "If no match is found, a single-sided pair is generated: one item is nil in this case.\n"
  "Beside the items, a status is kept which gives more details about success or failure of the "
  "match operation.\n"
  "\n"
  "Item pairing happens on different levels, reflecting the hierarchy of the netlists. "
  "On the top level there are circuits. Inside circuits nets, devices, subcircuits and pins "
  "are paired. Nets further contribute their connected items through terminals (for devices), "
  "pins (outgoing) and subcircuit pins.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

gsi::EnumIn<db::NetlistCrossReference, db::NetlistCrossReference::Status> decl_dbNetlistCrossReference_Status ("db", "Status",
  gsi::enum_const ("None", db::NetlistCrossReference::None,
    "@brief Enum constant NetlistCrossReference::None\n"
    "No specific status is implied if this code is present."
  ) +
  gsi::enum_const ("Match", db::NetlistCrossReference::Match,
    "@brief Enum constant NetlistCrossReference::Match\n"
    "An exact match exists if this code is present.\n"
  ) +
  gsi::enum_const ("NoMatch", db::NetlistCrossReference::NoMatch,
    "@brief Enum constant NetlistCrossReference::NoMatch\n"
    "If this code is present, no match could be found.\n"
    "There is also 'Mismatch' which means there is a candidate, but exact "
    "identity could not be confirmed."
  ) +
  gsi::enum_const ("Skipped", db::NetlistCrossReference::Skipped,
    "@brief Enum constant NetlistCrossReference::Skipped\n"
    "On circuits this code means that a match has not been attempted because "
    "subcircuits of this circuits were not matched. As circuit matching happens "
    "bottom-up, all subcircuits must match at least with respect to their pins "
    "to allow any parent circuit to be matched."
  ) +
  gsi::enum_const ("MatchWithWarning", db::NetlistCrossReference::MatchWithWarning,
    "@brief Enum constant NetlistCrossReference::MatchWithWarning\n"
    "If this code is present, a match was found but a warning is issued. For nets, this "
    "means that the choice is ambiguous and one, unspecific candidate has been chosen. "
    "For devices, this means a device match was established, but parameters or the device class "
    "are not matching exactly."
  ) +
  gsi::enum_const ("Mismatch", db::NetlistCrossReference::Mismatch,
    "@brief Enum constant NetlistCrossReference::Mismatch\n"
    "This code means there is a match candidate, but exact identity could not be confirmed."
  ),
  "@brief This class represents the NetlistCrossReference::Status enum"
);

//  Inject the NetlistCrossReference::Status declarations into NetlistCrossReference:
gsi::ClassExt<db::NetlistCrossReference> inject_NetlistCrossReference_Status_in_parent (decl_dbNetlistCrossReference_Status.defs ());

}
