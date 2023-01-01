
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

#include <Python.h>

#include "gsiDecl.h"
#include "pyaInternal.h"
#include "pya.h"

namespace gsi
{

static const pya::MethodTableEntry *getter (std::pair<const pya::MethodTableEntry *, const pya::MethodTableEntry *> *p)
{
  return p->second;
}

static const pya::MethodTableEntry *setter (std::pair<const pya::MethodTableEntry *, const pya::MethodTableEntry *> *p)
{
  return p->first;
}

gsi::Class<std::pair<const pya::MethodTableEntry *, const pya::MethodTableEntry *> > decl_PythonGetterSetterPair ("tl", "PythonGetterSetterPair",
  gsi::method_ext ("getter", &getter, "@brief Gets the getter function") +
  gsi::method_ext ("setter", &setter, "@brief Gets the setter function"),
  "@hide"
);

gsi::Class<pya::MethodTableEntry> decl_PythonFunction ("tl", "PythonFunction",
  gsi::method ("methods", &pya::MethodTableEntry::methods, "@brief Gets the list of methods bound to this Python function") +
  gsi::method ("name", &pya::MethodTableEntry::name, "@brief Gets the name of this Python function") +
  gsi::method ("is_static", &pya::MethodTableEntry::is_static, "@brief Gets the value indicating whether this Python function is 'static' (class function)") +
  gsi::method ("is_protected", &pya::MethodTableEntry::is_protected, "@brief Gets a value indicating whether this function is protected"),
  "@hide"
);

static std::vector<const pya::MethodTableEntry *> get_python_methods (const gsi::ClassBase *cls, bool st)
{
  const pya::MethodTable *mt = pya::MethodTable::method_table_by_class (cls);

  std::vector<const pya::MethodTableEntry *> methods;

  if (mt != 0) {
    for (auto m = mt->method_table ().begin (); m != mt->method_table ().end (); ++m) {
      if (m->is_enabled () && m->is_static () == st) {
        methods.push_back (m.operator-> ());
      }
    }
  }

  return methods;
}

static std::vector<std::pair<const pya::MethodTableEntry *, const pya::MethodTableEntry *> > get_python_properties (const gsi::ClassBase *cls, bool st)
{
  const pya::MethodTable *mt = pya::MethodTable::method_table_by_class (cls);

  std::vector<std::pair<const pya::MethodTableEntry *, const pya::MethodTableEntry *> > methods;

  if (mt != 0) {
    for (auto m = mt->property_table ().begin (); m != mt->property_table ().end (); ++m) {
      if (m->first.is_enabled () && m->first.is_static () == st) {
        methods.push_back (std::make_pair (&m->first, &m->second));
      }
    }
  }

  return methods;
}

static
gsi::ClassExt<gsi::ClassBase> class_base_ext (
  gsi::method_ext ("python_methods", &get_python_methods, gsi::arg ("static"), "@brief Gets the Python methods (static or non-static)") +
  gsi::method_ext ("python_properties", &get_python_properties, gsi::arg ("static"), "@brief Gets the Python properties (static or non-static) as a list of getter/setter pairs\nNote that if a getter or setter is not available the list of Python functions for this part is empty."),
  "@hide"
);

static
gsi::ClassExt<gsi::MethodBase> method_base_ext (
  gsi::method_ext ("python_methods", &pya::PythonInterpreter::python_doc, "@brief Gets the Python specific documentation"),
  "@hide"
);

}
