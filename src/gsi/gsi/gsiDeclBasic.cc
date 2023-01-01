
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


#include "gsiDeclBasic.h"
#include "gsiInterpreter.h"
#include "gsiDecl.h"
#include "tlTypeTraits.h"

namespace gsi
{

// ---------------------------------------------------------------------------------
//  A generic value wrapper that allows wrapping a plain data type into an object

static gsi::Value *new_vv (const tl::Variant &v) 
{
  return new gsi::Value (v);
}

static gsi::Value *new_vv0 () 
{
  return new gsi::Value ();
}

Class<Value> decl_Value ("tl", "Value",
  gsi::constructor ("new", &new_vv0, 
    "@brief Constructs a nil object.\n"
  ) +
  gsi::constructor ("new", &new_vv, gsi::arg ("value"),
    "@brief Constructs a non-nil object with the given value.\n"
    "This constructor has been introduced in version 0.22.\n"
  ) +
  gsi::method ("to_s", &Value::to_string,
    "@brief Convert this object to a string\n"
  ) +
  gsi::method ("value=", &Value::set_value, gsi::arg ("value"),
    "@brief Set the actual value.\n"
  ) +
  gsi::method ("value", (const tl::Variant &(Value::*)() const) &Value::value,
    "@brief Gets the actual value.\n"
  ),
  "@brief Encapsulates a value (preferably a plain data type) in an object\n"
  "This class is provided to 'box' a value (encapsulate the value in an object). This class is required to interface "
  "to pointer or reference types in a method call. By using that class, the method can alter the value and thus implement "
  "'out parameter' semantics. The value may be 'nil' which acts as a null pointer in pointer type arguments."
  "\n"
  "This class has been introduced in version 0.22."
);

static void eval_string_impl (Interpreter *ip, const char *string, const char *filename, int line)
{
  ip->eval_string (string, filename, line);
}

static tl::Variant eval_expr_impl (Interpreter *ip, const char *string, const char *filename, int line)
{
  return ip->eval_expr (string, filename, line);
}

static void define_variable_impl (Interpreter *ip, const std::string &name, const tl::Variant &value)
{
  ip->define_variable (name, value);
}

static gsi::Interpreter *interpreter_by_name (const std::string &name)
{
  for (tl::Registrar<gsi::Interpreter>::iterator i = gsi::interpreters.begin (); i != gsi::interpreters.end (); ++i) {
    if (i.current_name () == name) {
      return i->available () ? i.operator-> () : 0;
    }
  }
  return 0;
}

static gsi::Interpreter *python_interpreter ()
{
  return interpreter_by_name ("pya");
}

static gsi::Interpreter *ruby_interpreter ()
{
  return interpreter_by_name ("rba");
}

Class<Interpreter> decl_Interpreter ("tl", "Interpreter",
  gsi::method ("load_file", &Interpreter::load_file, gsi::arg ("path"),
    "@brief Loads the given file into the interpreter\n"
    "This will execute the code inside the file.\n"
  ) +
  gsi::method_ext ("eval_string", &eval_string_impl, gsi::arg ("string"), gsi::arg ("filename", (const char *) 0, "nil"), gsi::arg ("line", 1),
    "@brief Executes the code inside the given string\n"
    "Use 'filename' and 'line' to indicate the original source for the error messages.\n"
  ) +
  gsi::method_ext ("eval_expr", &eval_expr_impl, gsi::arg ("string"), gsi::arg ("filename", (const char *) 0, "nil"), gsi::arg ("line", 1),
    "@brief Executes the expression inside the given string and returns the result value\n"
    "Use 'filename' and 'line' to indicate the original source for the error messages.\n"
  ) +
  gsi::method_ext ("define_variable", &define_variable_impl, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Defines a (global) variable with the given name and value\n"
    "You can use the \\Value class to provide 'out' or 'inout' parameters which can be modified by code executed inside the interpreter and read back by the caller."
  ) +
  gsi::method ("python_interpreter", &python_interpreter,
    "@brief Gets the instance of the Python interpreter\n"
  ) +
  gsi::method ("ruby_interpreter", &ruby_interpreter,
    "@brief Gets the instance of the Ruby interpreter\n"
  ),
  "@brief A generalization of script interpreters\n"
  "The main purpose of this class is to provide cross-language call options. "
  "Using the Python interpreter, it is possible to execute Python code from Ruby for example.\n"
  "\n"
  "The following example shows how to use the interpreter class to execute Python code from Ruby "
  "and how to pass values from Ruby to Python and back using the \\Value wrapper object:\n"
  "\n"
  "@code\n"
  "pya = RBA::Interpreter::python_interpreter\n"
  "out_param = RBA::Value::new(17)\n"
  "pya.define_variable(\"out_param\", out_param)\n"
  "pya.eval_string(<<END)\n"
  "print(\"This is Python now!\")\n"
  "out_param.value = out_param.value + 25\n"
  "END\n"
  "puts out_param.value  # gives '42'"
  "@/code\n"
  "\n"
  "This class was introduced in version 0.27.5.\n"
);

}
