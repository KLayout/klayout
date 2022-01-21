
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

#include "dbD25TechnologyComponent.h"
#include "tlClassRegistry.h"
#include "tlString.h"

namespace db
{

std::string d25_component_name ()
{
  return std::string ("d25");
}

std::string d25_description ()
{
  return tl::to_string (tr ("Z stack (2.5d)"));
}

// -----------------------------------------------------------------------------------------
//  D25LayerInfo implementation

D25LayerInfo::D25LayerInfo ()
  : m_layer (), m_zstart (0.0), m_zstop (0.0)
{
  //  .. nothing yet ..
}

D25LayerInfo::~D25LayerInfo ()
{
  //  .. nothing yet ..
}

D25LayerInfo::D25LayerInfo (const D25LayerInfo &other)
{
  operator= (other);
}

D25LayerInfo &
D25LayerInfo::operator= (const D25LayerInfo &other)
{
  if (this != &other) {
    m_layer = other.m_layer;
    m_zstart = other.m_zstart;
    m_zstop = other.m_zstop;
  }
  return *this;
}

bool
D25LayerInfo::operator== (const D25LayerInfo &other) const
{
  return fabs (m_zstart - other.m_zstart) < db::epsilon && fabs (m_zstop - other.m_zstop) < db::epsilon;
}

void
D25LayerInfo::set_layer (const db::LayerProperties &l)
{
  m_layer = l;
}

void
D25LayerInfo::set_layer_from_string (const std::string &l)
{
  db::LayerProperties lp;
  tl::Extractor ex (l.c_str ());
  try {
    lp.read (ex);
  } catch (tl::Exception &) {
    //  ignore errors for now.
  }
  m_layer = lp;
}

std::string
D25LayerInfo::layer_as_string () const
{
  return m_layer.to_string ();
}

void
D25LayerInfo::set_zstart (double z0)
{
  m_zstart = z0;
}

void
D25LayerInfo::set_zstop (double z1)
{
  m_zstop = z1;
}

// -----------------------------------------------------------------------------------
//  D25TechnologyComponent implementation

D25TechnologyComponent::D25TechnologyComponent ()
  : db::TechnologyComponent (d25_component_name (), d25_description ())
{
  //  provide some explanation for the initialization
  m_src =
    "# Provide z stack information here\n"
    "#\n"
    "# Each line is one layer. The specification consists of a layer specification, a colon and arguments.\n"
    "# The arguments are named (like \"x=...\") or in serial. Parameters are separated by comma or blanks.\n"
    "# Named arguments are:\n"
    "#\n"
    "#   zstart   The lower z position of the extruded layer in µm\n"
    "#   zstop    The upper z position of the extruded layer in µm\n"
    "#   height   The height of the extruded layer in µm\n"
    "#\n"
    "# 'height', 'zstart' and 'zstop' can be used in any combination. If no value is given for 'zstart',\n"
    "# the upper level of the previous layer will be used.\n"
    "#\n"
    "# If a single unnamed parameter is given, it corresponds to 'height'. Two parameters correspond to\n"
    "# 'zstart' and 'zstop'.\n"
    "#\n"
    "# Examples:\n"
    "#   1: 0.5 1.5                    # extrude layer 1/0 from 0.5 to 1.5 vertically\n"
    "#   1/0: 0.5 1.5                  # same with explicit datatype\n"
    "#   1: zstop=1.5, zstart=0.5      # same with named parameters\n"
    "#   1: height=1.0, zstop=1.5      # same with z stop minus height\n"
    "#   1: 1.0 zstop=1.5              # same with height as unnamed parameter\n"
    "#\n"
    "# VARIABLES\n"
    "#\n"
    "# You can declare variables with:\n"
    "#   var name = value\n"
    "#\n"
    "# You can use variables inside numeric expressions.\n"
    "# Example:\n"
    "#   var hmetal = 0.48\n"
    "#   7/0: 0.5 0.5+hmetal*2        # 2x thick metal\n"
    "#\n"
    "# You cannot use variables inside layer specifications currently.\n"
    "#\n"
    "# CONDITIONALS\n"
    "#\n"
    "# You can enable or disable branches of the table using 'if', 'else', 'elseif' and 'end':\n"
    "# Example:\n"
    "#   var thick_m1 = true\n"
    "#   if thickm1\n"
    "#     1: 0.5 1.5\n"
    "#   else\n"
    "#     1: 0.5 1.2\n"
    "#   end\n"
    "\n"
  ;
}

D25TechnologyComponent::D25TechnologyComponent (const D25TechnologyComponent &d)
  : db::TechnologyComponent (d25_component_name (), d25_description ())
{
  m_src = d.m_src;
}

D25TechnologyComponent::layers_type
D25TechnologyComponent::compile_from_source () const
{
  layers_type layers;

  int current_line = 0;

  try {

    tl::Eval eval;
    std::vector<bool> conditional_stack;

    std::vector<std::string> lines = tl::split (m_src, "\n");
    for (std::vector<std::string>::const_iterator l = lines.begin (); l != lines.end (); ++l) {

      ++current_line;

      tl::Extractor ex (l->c_str ());

      if (ex.test ("#")) {
        //  ignore comments
      } else if (ex.at_end ()) {
        //  ignore empty lines

      } else if (ex.test ("if")) {

        tl::Expression x;
        eval.parse (x, ex);
        conditional_stack.push_back (x.execute ().to_bool ());

        ex.expect_end ();

      } else if (ex.test ("else")) {

        if (conditional_stack.empty ()) {
          throw tl::Exception (tl::to_string (tr ("'else' without 'if'")));
        }

        conditional_stack.back () = ! conditional_stack.back ();

        ex.expect_end ();

      } else if (ex.test ("end")) {

        if (conditional_stack.empty ()) {
          throw tl::Exception (tl::to_string (tr ("'end' without 'if'")));
        }

        conditional_stack.pop_back ();

        ex.expect_end ();

      } else if (ex.test ("elsif")) {

        if (conditional_stack.empty ()) {
          throw tl::Exception (tl::to_string (tr ("'elsif' without 'if'")));
        }

        tl::Expression x;
        eval.parse (x, ex);
        conditional_stack.back () = x.execute ().to_bool ();

        ex.expect_end ();

      } else if (! conditional_stack.empty () && ! conditional_stack.back ()) {

        continue;

      } else if (ex.test ("var")) {

        std::string n;
        ex.read_name (n);

        ex.expect ("=");

        tl::Expression x;
        eval.parse (x, ex);
        eval.set_var (n, x.execute ());

        ex.expect_end ();

      } else if (ex.test ("print")) {

        tl::Expression x;
        eval.parse (x, ex);
        ex.expect_end ();

        tl::info << x.execute ().to_string ();

      } else if (ex.test ("error")) {

        tl::Expression x;
        eval.parse (x, ex);
        ex.expect_end ();

        throw tl::Exception (x.execute ().to_string ());

      } else {

        db::D25LayerInfo info;
        if (! layers.empty ()) {
          info.set_zstart (layers.back ().zstop ());
          info.set_zstop (layers.back ().zstop ());
        }

        tl::Variant z0, z1, h;
        std::vector<double> args;

        db::LayerProperties lp;
        lp.read (ex);
        info.set_layer (lp);

        ex.expect (":");

        while (! ex.at_end ()) {

          if (ex.test ("#")) {
            break;
          }

          tl::Expression pvx;

          std::string pn;
          if (ex.try_read_name (pn)) {
            ex.expect ("=");
            eval.parse (pvx, ex);
          } else {
            eval.parse (pvx, ex);
          }

          double pv = pvx.execute ().to_double ();

          ex.test (",");

          if (pn.empty ()) {
            args.push_back (pv);
          } else if (pn == "zstart") {
            z0 = pv;
          } else if (pn == "zstop") {
            z1 = pv;
          } else if (pn == "height") {
            h = pv;
          } else {
            throw tl::Exception (tl::to_string (tr ("Invalid parameter name: ")) + pn);
          }

        }

        if (args.size () == 0) {
          if (z0.is_nil () && z1.is_nil ()) {
            if (! h.is_nil ()) {
              info.set_zstop (info.zstop () + h.to_double ());
            }
          } else if (z0.is_nil ()) {
            info.set_zstop (z1.to_double ());
            if (! h.is_nil ()) {
              info.set_zstart (info.zstop () - h.to_double ());
            }
          } else if (z1.is_nil ()) {
            info.set_zstart (z0.to_double ());
            if (! h.is_nil ()) {
              info.set_zstop (info.zstart () + h.to_double ());
            }
          } else {
            info.set_zstart (z0.to_double ());
            info.set_zstop (z1.to_double ());
          }
        } else if (args.size () == 1) {
          if (! h.is_nil ()) {
            if (! z0.is_nil ()) {
              throw tl::Exception (tl::to_string (tr ("Redundant parameters: zstart already given")));
            }
            if (! z1.is_nil ()) {
              throw tl::Exception (tl::to_string (tr ("Redundant parameters: zstop implicitly given")));
            }
            info.set_zstart (args[0]);
            info.set_zstop (args[0] + h.to_double ());
          } else {
            if (! z1.is_nil ()) {
              throw tl::Exception (tl::to_string (tr ("Redundant parameters: zstop implicitly given")));
            }
            info.set_zstop ((! z0.is_nil () ? z0.to_double () : info.zstart ()) + args[0]);
          }
        } else if (args.size () == 2) {
          if (! z0.is_nil ()) {
            throw tl::Exception (tl::to_string (tr ("Redundant parameters: zstart already given")));
          }
          if (! z1.is_nil ()) {
            throw tl::Exception (tl::to_string (tr ("Redundant parameters: zstop already given")));
          }
          if (! h.is_nil ()) {
            throw tl::Exception (tl::to_string (tr ("Redundant parameters: height implicitly given")));
          }
          info.set_zstart (args[0]);
          info.set_zstop (args[1]);
        } else {
          throw tl::Exception (tl::to_string (tr ("Too many parameters (max 2)")));
        }

        layers.push_back (info);

      }

    }

    if (! conditional_stack.empty ()) {
      throw tl::Exception (tl::to_string (tr ("'if', 'else' or 'elsif' without matching 'end'")));
    }

  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::sprintf (tl::to_string (tr (" in line %d")), current_line));
  }

  return layers;
}

std::string
D25TechnologyComponent::to_string () const
{
  layers_type layers = compile_from_source ();
  std::string res;

  for (layers_type::const_iterator i = layers.begin (); i != layers.end (); ++i) {
    if (! res.empty ()) {
      res += "\n";
    }
    res += i->layer ().to_string () + ": zstart=" + tl::to_string (i->zstart ()) + ", zstop=" + tl::to_string (i->zstop ());
  }

  return res;
}

// -----------------------------------------------------------------------------------
//  D25TechnologyComponent technology component registration

class D25TechnologyComponentProvider
  : public db::TechnologyComponentProvider
{
public:
  D25TechnologyComponentProvider ()
    : db::TechnologyComponentProvider ()
  {
    //  .. nothing yet ..
  }

  virtual db::TechnologyComponent *create_component () const
  {
    return new D25TechnologyComponent ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new db::TechnologyComponentXMLElement<D25TechnologyComponent> (d25_component_name (),
      tl::make_member (&D25TechnologyComponent::src, &D25TechnologyComponent::set_src, "src")
    );
  }
};

static tl::RegisteredClass<db::TechnologyComponentProvider> tc_decl (new D25TechnologyComponentProvider (), 3100, d25_component_name ().c_str ());

}
