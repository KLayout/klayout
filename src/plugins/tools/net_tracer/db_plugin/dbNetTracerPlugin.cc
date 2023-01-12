
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

#include "dbNetTracerIO.h"
#include "dbTechnology.h"
#include "tlClassRegistry.h"

namespace tl
{
  /**
   *  @brief A specialization of the XMLConverter that is used to serialize the connection info
   */

  template <>
  struct XMLStdConverter<db::NetTracerConnectionInfo>
  {
    std::string to_string (const db::NetTracerConnectionInfo &v) const
    {
      return v.to_string ();
    }

    void from_string (const std::string &s, db::NetTracerConnectionInfo &v) const
    {
      tl::Extractor ex (s.c_str ());
      v.parse (ex);
    }
  };

  /**
   *  @brief A specialization of the XMLConverter that is used to serialize the symbol info
   */

  template <>
  struct XMLStdConverter<db::NetTracerSymbolInfo>
  {
    std::string to_string (const db::NetTracerSymbolInfo &v) const
    {
      return v.to_string ();
    }

    void from_string (const std::string &s, db::NetTracerSymbolInfo &v) const
    {
      tl::Extractor ex (s.c_str ());
      v.parse (ex);
    }
  };
}

namespace
{

static const db::NetTracerConnectivity *
get_fallback_default (const db::NetTracerTechnologyComponent &tc)
{
  for (auto d = tc.begin (); d != tc.end (); ++d) {
    if (d->is_fallback_default ()) {
      return d.operator-> ();
    }
  }

  return 0;
}

static const db::NetTracerConnectivity *
get_default (const db::NetTracerTechnologyComponent &tc)
{
  for (auto d = tc.begin (); d != tc.end (); ++d) {
    if (d->name ().empty ()) {
      return d.operator-> ();
    }
  }

  if (tc.begin () != tc.end ()) {
    return tc.begin ().operator-> ();
  } else {
    static db::NetTracerConnectivity s_empty;
    return &s_empty;
  }
}

template <class Value>
struct FallbackXMLWriteAdaptor
{
  FallbackXMLWriteAdaptor (void (db::NetTracerConnectivity::*member) (const Value &))
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (db::NetTracerTechnologyComponent &owner, tl::XMLReaderState &reader) const
  {
    db::NetTracerConnectivity *stack = const_cast<db::NetTracerConnectivity *> (get_fallback_default (owner));
    if (! stack && owner.begin () == owner.end ()) {
      owner.push_back (db::NetTracerConnectivity ());
      stack = (owner.end () - 1).operator-> ();
      stack->set_fallback_default (true);
    }

    if (stack) {
      tl::XMLObjTag<Value> tag;
      (stack->*mp_member) (*reader.back (tag));
    }
  }

private:
  void (db::NetTracerConnectivity::*mp_member) (const Value &);
};

template <class Value, class Iter>
struct FallbackXMLReadAdaptor
{
  typedef tl::pass_by_ref_tag tag;

  FallbackXMLReadAdaptor (Iter (db::NetTracerConnectivity::*begin) () const, Iter (db::NetTracerConnectivity::*end) () const)
    : mp_begin (begin), mp_end (end)
  {
    // .. nothing yet ..
  }

  Value operator () () const
  {
    return *m_iter;
  }

  bool at_end () const
  {
    return m_iter == m_end;
  }

  void start (const db::NetTracerTechnologyComponent &parent)
  {
    const db::NetTracerConnectivity *tn = get_default (parent);
    m_iter = (tn->*mp_begin) ();
    m_end = (tn->*mp_end) ();
  }

  void next ()
  {
    ++m_iter;
  }

private:
  Iter (db::NetTracerConnectivity::*mp_begin) () const;
  Iter (db::NetTracerConnectivity::*mp_end) () const;
  Iter m_iter, m_end;
};

}

namespace db
{

class NetTracerTechnologyComponentProvider
  : public db::TechnologyComponentProvider
{
public:
  NetTracerTechnologyComponentProvider ()
    : db::TechnologyComponentProvider ()
  {
    //  .. nothing yet ..
  }

  virtual db::TechnologyComponent *create_component () const
  {
    return new NetTracerTechnologyComponent ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new db::TechnologyComponentXMLElement<NetTracerTechnologyComponent> (net_tracer_component_name (),
      //  0.28 definitions
      tl::make_element ((NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::begin, (NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::end, (void (NetTracerTechnologyComponent::*) (const NetTracerConnectivity &)) &NetTracerTechnologyComponent::push_back, "stack",
        tl::make_member (&NetTracerConnectivity::name, &NetTracerConnectivity::set_name, "name") +
        tl::make_member (&NetTracerConnectivity::description, &NetTracerConnectivity::set_description, "description") +
        tl::make_member ((NetTracerConnectivity::const_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::begin, (NetTracerConnectivity::const_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::end, &NetTracerConnectivity::add, "connection") +
        tl::make_member ((NetTracerConnectivity::const_symbol_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::begin_symbols, (NetTracerConnectivity::const_symbol_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::end_symbols, &NetTracerConnectivity::add_symbol, "symbols")
      ) +
      //  Fallback readers for migrating pre-0.28 setups to 0.28 and backward compatibility
      tl::XMLMember<NetTracerConnectionInfo, NetTracerTechnologyComponent, FallbackXMLReadAdaptor <NetTracerConnectionInfo, NetTracerConnectivity::const_iterator>, FallbackXMLWriteAdaptor <NetTracerConnectionInfo>, tl::XMLStdConverter <NetTracerConnectionInfo> > (
        FallbackXMLReadAdaptor <NetTracerConnectionInfo, NetTracerConnectivity::const_iterator> (&NetTracerConnectivity::begin, &NetTracerConnectivity::end),
        FallbackXMLWriteAdaptor <NetTracerConnectionInfo> (&NetTracerConnectivity::add), "connection") +
      tl::XMLMember<NetTracerSymbolInfo, NetTracerTechnologyComponent, FallbackXMLReadAdaptor <NetTracerSymbolInfo, NetTracerConnectivity::const_symbol_iterator>, FallbackXMLWriteAdaptor <NetTracerSymbolInfo>, tl::XMLStdConverter <NetTracerSymbolInfo> > (
        FallbackXMLReadAdaptor <NetTracerSymbolInfo, NetTracerConnectivity::const_symbol_iterator> (&NetTracerConnectivity::begin_symbols, &NetTracerConnectivity::end_symbols),
        FallbackXMLWriteAdaptor <NetTracerSymbolInfo> (&NetTracerConnectivity::add_symbol), "symbols")
    );
  }
};

static tl::RegisteredClass<db::TechnologyComponentProvider> tc_decl (new NetTracerTechnologyComponentProvider (), 13000, "NetTracerPlugin");

}
