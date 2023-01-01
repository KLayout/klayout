
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


#include "dbLayerProperties.h"
#include "dbStreamLayers.h"

namespace db
{

// -----------------------------------------------------------------
//  Implementation of the LayerProperties class

LayerProperties::LayerProperties () 
  : name (), layer (db::any_ld ()), datatype (db::any_ld ())
{ }

LayerProperties::LayerProperties (int l, int d) 
  : name (), layer (l), datatype (d)
{ }

LayerProperties::LayerProperties (const std::string &n) 
  : name (n), layer (db::any_ld ()), datatype (db::any_ld ())
{ }

LayerProperties::LayerProperties (int l, int d, const std::string &n) 
  : name (n), layer (l), datatype (d)
{ }

bool 
LayerProperties::is_named () const
{
  return db::is_any_ld (layer) && db::is_any_ld (datatype) && ! name.empty ();
}

bool
LayerProperties::is_null () const
{
  return db::is_any_ld (layer) && db::is_any_ld (datatype) && name.empty ();
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

static std::string format_ld (db::ld_type ld)
{
  if (db::is_static_ld (ld)) {
    return tl::to_string (ld);
  } else if (db::is_any_ld (ld)) {
    return "*";
  } else if (db::is_relative_ld (ld)) {
    db::ld_type offset = db::ld_offset (ld);
    if (offset < 0) {
      return "*-" + tl::to_string (-offset);
    } else {
      return "*+" + tl::to_string (offset);
    }
  } else {
    return tl::to_string (ld);
  }
}

std::string 
LayerProperties::to_string (bool as_target) const
{
  std::string r;
  if (! name.empty ()) {
    if (is_named ()) {
      r = tl::to_word_or_quoted_string (name);
    } else {
      r = tl::to_word_or_quoted_string (name) + " (" + format_ld (layer) + "/" + format_ld (datatype) + ")";
    }
  } else if (! is_null () || as_target) {
    r = format_ld (layer) + "/" + format_ld (datatype);
  }
  return r;
}

static bool read_ld (tl::Extractor &ex, ld_type &l, bool with_relative)
{
  if (ex.test ("*")) {

    int offset = 0;

    tl::Extractor eex = ex;
    if (with_relative && eex.test ("+") && eex.try_read (offset)) {
      l = db::relative_ld (offset);
      ex = eex;
    } else {
      eex = ex;
      if (with_relative && eex.test ("-") && eex.try_read (offset)) {
        l = db::relative_ld (-offset);
        ex = eex;
      } else {
        l = db::any_ld ();
      }
    }

    return true;

  } else {
    return ex.try_read (l);
  }
}

void
LayerProperties::read (tl::Extractor &ex, bool as_target)
{
  layer = db::any_ld ();
  datatype = db::any_ld ();
  name.clear ();

  int l = 0, d = 0;
  if (read_ld (ex, l, as_target)) {

    if (ex.test ("/")) {
      read_ld (ex, d, as_target);
    }

    layer = l;
    datatype = d;

  } else if (ex.try_read_word_or_quoted (name)) {

    if (ex.test ("(")) {

      read_ld (ex, l, as_target);
      if (ex.test ("/")) {
        read_ld (ex, d, as_target);
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

