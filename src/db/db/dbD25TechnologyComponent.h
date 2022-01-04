
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

#ifndef HDR_dbD25TechnologyComponent
#define HDR_dbD25TechnologyComponent

#include "dbTechnology.h"
#include "dbLayerProperties.h"

namespace db
{

class DB_PUBLIC D25LayerInfo
{
public:
  D25LayerInfo ();
  ~D25LayerInfo ();
  D25LayerInfo (const D25LayerInfo &other);
  D25LayerInfo &operator= (const D25LayerInfo &other);

  bool operator== (const D25LayerInfo &other) const;

  const db::LayerProperties &layer () const
  {
    return m_layer;
  }

  void set_layer_from_string (const std::string &l);
  std::string layer_as_string () const;

  void set_layer (const db::LayerProperties &l);

  double zstart () const
  {
    return m_zstart;
  }

  void set_zstart (double z0);

  double zstop () const
  {
    return m_zstop;
  }

  void set_zstop (double z1);

private:
  db::LayerProperties m_layer;
  double m_zstart, m_zstop;
};

class DB_PUBLIC D25TechnologyComponent
  : public db::TechnologyComponent
{
public:
  D25TechnologyComponent ();
  D25TechnologyComponent (const D25TechnologyComponent &d);

  typedef std::list<D25LayerInfo> layers_type;

  layers_type compile_from_source () const;

  const std::string &src () const
  {
    return m_src;
  }

  //  for persistency only, use "compile_from_source" to read from a source string
  void set_src (const std::string &s)
  {
    m_src = s;
  }

  std::string to_string () const;

  db::TechnologyComponent *clone () const
  {
    return new D25TechnologyComponent (*this);
  }

private:
  std::string m_src;
};

}

#endif
