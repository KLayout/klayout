
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


#include "dbLayerProperties.h"

namespace db
{

// -----------------------------------------------------------------
//  Implementation of the LayerProperties class

LayerProperties::LayerProperties () 
  : name (), layer (-1), datatype (-1)
{ }

LayerProperties::LayerProperties (int l, int d) 
  : name (), layer (l), datatype (d)
{ }

LayerProperties::LayerProperties (const std::string &n) 
  : name (n), layer (-1), datatype (-1)
{ }

LayerProperties::LayerProperties (int l, int d, const std::string &n) 
  : name (n), layer (l), datatype (d)
{ }

bool 
LayerProperties::is_named () const
{
  return (layer < 0 || datatype < 0) && ! name.empty ();
}

bool 
LayerProperties::log_equal (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return false;
  }
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype;
  }
}

bool 
LayerProperties::log_less (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return is_null () < b.is_null ();
  }
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (is_named ()) {
    return name < b.name;
  } else {
    if (layer != b.layer) { return layer < b.layer; }
    return datatype < b.datatype;
  }
}

bool 
LayerProperties::operator== (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return false;
  }
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype && name == b.name;
  }
}

bool 
LayerProperties::operator!= (const LayerProperties &b) const
{
  return ! operator== (b);
}

bool 
LayerProperties::operator< (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return is_null () < b.is_null ();
  }
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (! is_named ()) {
    if (layer != b.layer) { return layer < b.layer; }
    if (datatype != b.datatype) { return datatype < b.datatype; }
  }
  return name < b.name;
}

std::string 
LayerProperties::to_string () const
{
  std::string r;
  if (! name.empty ()) {
    if (is_named ()) {
      r = tl::to_word_or_quoted_string (name);
    } else {
      r = tl::to_word_or_quoted_string (name) + tl::sprintf (" (%d/%d)", layer, datatype);
    }
  } else if (! is_null ()) {
    r = tl::sprintf ("%d/%d", layer, datatype);
  }
  return r;
}

void
LayerProperties::read (tl::Extractor &ex)
{
  layer = -1;
  datatype = -1;
  name.clear ();

  int l = 0, d = 0;
  if (ex.try_read (l)) {

    if (ex.test ("/")) {
      ex.read (d);
    }

    layer = l;
    datatype = d;

  } else if (ex.try_read_word_or_quoted (name)) {

    if (ex.test ("(")) {

      ex.read (l);
      if (ex.test ("/")) {
        ex.read (d);
      }
      ex.expect (")");

      layer = l;
      datatype = d;

    }

  }
}

// -----------------------------------------------------------------
//  Implementation of the LayerOffset class

LayerOffset::LayerOffset () 
  : name (), layer (-1), datatype (-1)
{ }

LayerOffset::LayerOffset (int l, int d) 
  : name (), layer (l), datatype (d)
{ }

LayerOffset::LayerOffset (const std::string &n) 
  : name (n), layer (-1), datatype (-1)
{ }

LayerOffset::LayerOffset (int l, int d, const std::string &n) 
  : name (n), layer (l), datatype (d)
{ }

bool 
LayerOffset::operator== (const LayerOffset &b) const
{
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype && name == b.name;
  }
}

bool 
LayerOffset::operator!= (const LayerOffset &b) const
{
  return ! operator== (b);
}

bool 
LayerOffset::operator< (const LayerOffset &b) const
{
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (! is_named ()) {
    if (layer != b.layer) { return layer < b.layer; }
    if (datatype != b.datatype) { return datatype < b.datatype; }
  } 
  return name < b.name;
}

bool 
LayerOffset::is_named () const
{
  return layer < 0 || datatype < 0;
}

std::string 
LayerOffset::to_string () const
{
  std::string r;
  if (! name.empty ()) {
    if (is_named ()) {
      r = tl::to_word_or_quoted_string (name, "_.$\\*");
    } else {
      r = tl::to_word_or_quoted_string (name, "_.$\\*") + tl::sprintf (" (%d/%d)", layer, datatype);
    }
  } else if (! is_named ()) {
    r = tl::sprintf ("%d/%d", layer, datatype);
  }
  return r;
}

void
LayerOffset::read (tl::Extractor &ex)
{
  layer = -1;
  datatype = -1;
  name.clear ();

  int l = 0, d = 0;
  if (ex.try_read (l)) {

    if (ex.test ("/")) {
      ex.read (d);
    }

    layer = l;
    datatype = d;

  } else if (ex.try_read_word_or_quoted (name, "_.$\\*")) {

    if (ex.test ("(")) {

      ex.read (l);
      if (ex.test ("/")) {
        ex.read (d);
      }
      ex.expect (")");

      layer = l;
      datatype = d;

    }

  }
}

LayerProperties 
LayerOffset::apply (const LayerProperties &props) const
{
  LayerProperties p (props);
  if (layer > 0 && p.layer >= 0) {
    p.layer += layer;
  }
  if (datatype > 0 && p.datatype >= 0) {
    p.datatype += datatype;
  }

  if (is_named () && p.is_named ()) {
    std::string new_name;
    for (const char *cp = name.c_str (); *cp; ++cp) {
      if (*cp == '\\' && cp[1]) {
        new_name += cp[1];
        ++cp;
      } else if (*cp == '*') {
        new_name += p.name;
      } 
    }
    p.name = new_name;
  }

  return p;
}

}

namespace tl
{

template<> bool test_extractor_impl (tl::Extractor &ex, db::LayerProperties &e)
{
  e.read (ex);
  return true; // TODO: never returns false!
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::LayerOffset &e)
{
  e.read (ex);
  return true; // TODO: never returns false!
}

template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::LayerProperties &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (tr ("Expected a layer specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::LayerOffset &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (tr ("Expected a layer offset specification")));
  }
}

}

