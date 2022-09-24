
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

template <class Value>
struct FallbackXMLWriteAdapator
{
  FallbackXMLWriteAdapator (void (db::NetTracerConnectivity::*member) (const Value &))
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (db::NetTracerTechnologyComponent &owner, tl::XMLReaderState &reader) const
  {
    if (owner.size () == 0) {
      owner.push_back (db::NetTracerConnectivity ());
    }
    tl::XMLObjTag<Value> tag;
    ((*owner.begin ()).*mp_member) (*reader.back (tag));
  }

private:
  void (db::NetTracerConnectivity::*mp_member) (const Value &);
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
      //  Fallback readers for migrating pre-0.28 setups to 0.28
      tl::XMLMember<NetTracerConnectionInfo, NetTracerTechnologyComponent, tl::XMLMemberDummyReadAdaptor <NetTracerConnectionInfo, NetTracerTechnologyComponent>, FallbackXMLWriteAdapator <NetTracerConnectionInfo>, tl::XMLStdConverter <NetTracerConnectionInfo> > (
        tl::XMLMemberDummyReadAdaptor <NetTracerConnectionInfo, NetTracerTechnologyComponent> (),
        FallbackXMLWriteAdapator <NetTracerConnectionInfo> (&NetTracerConnectivity::add), "connection") +
      tl::XMLMember<NetTracerSymbolInfo, NetTracerTechnologyComponent, tl::XMLMemberDummyReadAdaptor <NetTracerSymbolInfo, NetTracerTechnologyComponent>, FallbackXMLWriteAdapator <NetTracerSymbolInfo>, tl::XMLStdConverter <NetTracerSymbolInfo> > (
        tl::XMLMemberDummyReadAdaptor <NetTracerSymbolInfo, NetTracerTechnologyComponent> (),
        FallbackXMLWriteAdapator <NetTracerSymbolInfo> (&NetTracerConnectivity::add_symbol), "symbols") +
      //  0.28 definitions
      tl::make_element ((NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::begin, (NetTracerTechnologyComponent::const_iterator (NetTracerTechnologyComponent::*) () const) &NetTracerTechnologyComponent::end, (void (NetTracerTechnologyComponent::*) (const NetTracerConnectivity &)) &NetTracerTechnologyComponent::push_back, "connectivity",
        tl::make_member ((NetTracerConnectivity::const_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::begin, (NetTracerConnectivity::const_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::end, &NetTracerConnectivity::add, "connection") +
        tl::make_member ((NetTracerConnectivity::const_symbol_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::begin_symbols, (NetTracerConnectivity::const_symbol_iterator (NetTracerConnectivity::*) () const) &NetTracerConnectivity::end_symbols, &NetTracerConnectivity::add_symbol, "symbols")
      )
    );
  }
};

static tl::RegisteredClass<db::TechnologyComponentProvider> tc_decl (new NetTracerTechnologyComponentProvider (), 13000, "NetTracerPlugin");

}
