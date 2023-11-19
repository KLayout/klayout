
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
#include "gsiDeclBasic.h"
#include "gsiInterpreter.h"
#include "gsiEnums.h"
#include "lymMacroInterpreter.h"
#include "lymMacro.h"
#include "lymMacroCollection.h"
#include "rba.h"

#include "tlClassRegistry.h"
#include "tlFileUtils.h"
#include "tlInclude.h"

#include <cstdio>

namespace gsi
{

class MacroExecutionContext
{
public:
  static void set_debugger_scope (const std::string &filename)
  {
    if (rba::RubyInterpreter::instance ()) {
      rba::RubyInterpreter::instance ()->set_debugger_scope (filename);
    }
  }

  static void remove_debugger_scope ()
  {
    if (rba::RubyInterpreter::instance ()) {
      rba::RubyInterpreter::instance ()->remove_debugger_scope ();
    }
  }

  static void ignore_next_exception ()
  {
    if (rba::RubyInterpreter::instance ()) {
      rba::RubyInterpreter::instance ()->ignore_next_exception ();
    }
  }
};

Class<gsi::MacroExecutionContext> decl_MacroExecutionContext ("lay", "MacroExecutionContext",
  gsi::method ("set_debugger_scope", &gsi::MacroExecutionContext::set_debugger_scope, gsi::arg ("filename"),
    "@brief Sets a debugger scope (file level which shall appear in the debugger)\n"
    "If a debugger scope is set, back traces will be produced starting from that scope. "
    "Setting a scope is useful for implementing DSL interpreters and giving a proper hint about "
    "the original location of an error."
  ) +
  gsi::method ("remove_debugger_scope", &gsi::MacroExecutionContext::remove_debugger_scope,
    "@brief Removes a debugger scope previously set with \\set_debugger_scope\n"
  ) +
  gsi::method ("ignore_next_exception", &gsi::MacroExecutionContext::ignore_next_exception,
    "@brief Ignores the next exception in the debugger\n"
    "The next exception thrown will be ignored in the debugger. That feature is useful when "
    "re-raising exceptions if those new exception shall not appear in the debugger."
  ),
  "@brief Support for various debugger features\n"
  "\n"
  "This class implements some features that allow customization of the debugger behavior, specifically "
  "the generation of back traces and the handling of exception. These functions are particular useful "
  "for implementing DSL interpreters and providing proper error locations in the back traces or to "
  "suppress exceptions when re-raising them."
);

class MacroInterpreterImpl
  : public lym::MacroInterpreter
{
public:
  MacroInterpreterImpl ()
    : lym::MacroInterpreter (), 
      mp_registration (0), m_supports_include_expansion (true)
  {
    m_suffix = lym::MacroInterpreter::suffix ();
    m_description = lym::MacroInterpreter::description ();
    m_storage_scheme = lym::MacroInterpreter::storage_scheme ();
    m_syntax_scheme = lym::MacroInterpreter::syntax_scheme ();
    m_debugger_scheme = lym::MacroInterpreter::debugger_scheme ();
  }

  ~MacroInterpreterImpl ()
  {
    delete mp_registration;
    mp_registration = 0;
    for (std::vector<lym::Macro *>::const_iterator t = m_templates.begin (); t != m_templates.end (); ++t) {
      delete *t;
    }
    m_templates.clear ();
  }

  std::pair<std::string, std::string> include_expansion (const lym::Macro *macro)
  {
    if (m_supports_include_expansion) {
      return lym::MacroInterpreter::include_expansion (macro);
    } else {
      return std::pair<std::string, std::string> (macro->path (), macro->text ());
    }
  }

  void register_gsi (const char *name)
  {
    m_name = name;

    //  do not register an interpreter again (this is important as registration code
    //  may be executed again and we do not want to have two interpreters for the same thing)
    for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
      if (cls.current_name () == m_name) {
        return;
      }
    }

    //  makes the object owned by the C++ side
    keep ();

    //  cancel any previous registration and register (again)
    delete mp_registration;
    mp_registration = new tl::RegisteredClass<lym::MacroInterpreter> (this, 0 /*position*/, name, false /*does not own object*/);
  }

  virtual tl::Executable *executable (const lym::Macro *macro) const
  {
    if (f_executable.can_issue ()) {
      return f_executable.issue<MacroInterpreter, tl::Executable *, const lym::Macro *> (&MacroInterpreter::executable, macro);
    } else {
      return 0;
    }
  }

  void set_supports_include_expansion (bool f)
  {
    m_supports_include_expansion = f;
  }

  virtual bool supports_include_expansion () const
  {
    return m_supports_include_expansion;
  }

  void set_storage_scheme (lym::Macro::Format scheme)
  {
    m_storage_scheme = scheme;
  }

  virtual lym::Macro::Format storage_scheme () const
  {
    return m_storage_scheme;
  }

  void set_debugger_scheme (lym::Macro::Interpreter scheme)
  {
    m_debugger_scheme = scheme;
  }

  virtual lym::Macro::Interpreter debugger_scheme () const
  {
    return m_debugger_scheme;
  }

  void set_syntax_scheme (const std::string &s)
  {
    m_syntax_scheme = s;
  }

  virtual std::string syntax_scheme () const
  {
    return m_syntax_scheme;
  }

  void set_description (const std::string &s)
  {
    m_description = s;
  }

  virtual std::string description () const
  {
    return m_description;
  }

  void set_suffix (const std::string &s)
  {
    m_suffix = s;
  }

  virtual std::string suffix () const
  {
    return m_suffix;
  }

  lym::Macro *create_template (const std::string &url)
  {
    if (m_name.empty ()) {
      throw std::runtime_error (tl::to_string (tr ("MacroInterpreter::create_template must be called after register")));
    }

    lym::Macro *m = new lym::Macro ();
    if (! url.empty ()) {
      m->load_from (url);
    }

    m->rename (tl::basename (url));

    m->set_readonly (true);
    m->set_dsl_interpreter (m_name);
    m->set_interpreter (lym::Macro::DSLInterpreter);
    m->set_format (storage_scheme ());

    //  avoid registering the same template twice
    for (auto t = m_templates.begin (); t != m_templates.end (); ++t) {
      if ((*t)->path () == m->path ()) {
        delete *t;
        *t = m;
        return m;
      }
    }

    //  not present yet - install at end
    m_templates.push_back (m);
    return m;
  }

  virtual void get_templates (std::vector<lym::Macro *> &tt) const
  {
    for (std::vector<lym::Macro *>::const_iterator t = m_templates.begin  (); t != m_templates.end (); ++t) {
      tt.push_back (new lym::Macro ());
      tt.back ()->rename ((*t)->name ());
      tt.back ()->assign (**t);
    }
  }

  gsi::Callback f_executable;

private:
  tl::RegisteredClass <lym::MacroInterpreter> *mp_registration;
  std::string m_name;
  std::vector<lym::Macro *> m_templates;
  std::string m_syntax_scheme;
  lym::Macro::Format m_storage_scheme;
  lym::Macro::Interpreter m_debugger_scheme;
  std::string m_suffix;
  std::string m_description;
  bool m_supports_include_expansion;
};

static std::vector<std::string>
include_expansion (MacroInterpreterImpl *interp, lym::Macro *macro)
{
  std::vector<std::string> res;

  auto sp = interp->include_expansion (macro);
  res.push_back (sp.first);
  res.push_back (sp.second);

  return res;
}

gsi::EnumIn<lym::Macro, lym::Macro::Format> decl_FormatEnum ("lay", "Format",
  gsi::enum_const ("PlainTextFormat", lym::Macro::PlainTextFormat,
    "@brief The macro has plain text format"
  ) +
  gsi::enum_const ("PlainTextWithHashAnnotationsFormat", lym::Macro::PlainTextWithHashAnnotationsFormat,
    "@brief The macro has plain text format with special pseudo-comment annotations"
  ) +
  gsi::enum_const ("MacroFormat", lym::Macro::MacroFormat,
    "@brief The macro has macro (XML) format"
  ),
  "@brief Specifies the format of a macro\n"
  "This enum has been introduced in version 0.27.5."
);

gsi::EnumIn<lym::Macro, lym::Macro::Interpreter> decl_InterpreterEnum ("lay", "Interpreter",
  gsi::enum_const ("Ruby", lym::Macro::Ruby,
    "@brief The interpreter is Ruby"
  ) +
  gsi::enum_const ("Python", lym::Macro::Python,
    "@brief The interpreter is Python"
  ) +
  gsi::enum_const ("Text", lym::Macro::Text,
    "@brief Plain text"
  ) +
  gsi::enum_const ("DSLInterpreter", lym::Macro::DSLInterpreter,
    "@brief A domain-specific interpreter (DSL)"
  ) +
  gsi::enum_const ("None", lym::Macro::None,
    "@brief No specific interpreter"
  ),
  "@brief Specifies the interpreter used for executing a macro\n"
  "This enum has been introduced in version 0.27.5."
);

lym::Macro::Interpreter const_RubyDebugger ()
{
  return lym::Macro::Ruby;
}

lym::Macro::Interpreter const_NoDebugger ()
{
  return lym::Macro::None;
}

Class<MacroInterpreterImpl> decl_MacroInterpreter ("lay", "MacroInterpreter",
  gsi::method ("RubyDebugger", &const_RubyDebugger,
    "@brief Indicates Ruby debugger for \\debugger_scheme\n"
  ) +
  gsi::method ("NoDebugger", &const_NoDebugger,
    "@brief Indicates no debugging for \\debugger_scheme\n"
  ) +
  gsi::method_ext ("include_expansion", &include_expansion, gsi::arg ("macro"),
    "@brief Provides include expansion as defined by the interpreter\n"
    "The return value will be a two-element array with the encoded file path "
    "and the include-expanded text.\n"
    "\n"
    "This method has been introduced in version 0.28.12."
  ) +
  gsi::method ("register", &MacroInterpreterImpl::register_gsi, gsi::arg ("name"),
    "@brief Registers the macro interpreter\n"
    "@param name The interpreter name. This is an arbitrary string which should be unique.\n"
    "\n"
    "Registration of the interpreter makes the object known to the system. After registration, macros whose interpreter "
    "is set to 'dsl' can use this object to run the script. For executing a script, the system will "
    "call the interpreter's \\execute method.\n"
  ) + 
  gsi::method ("create_template", &MacroInterpreterImpl::create_template, gsi::arg ("url"),
    "@brief Creates a new macro template\n"
    "@param url The template will be initialized from that URL.\n"
    "\n"
    "This method will create a register a new macro template. It returns a \\Macro object which "
    "can be modified in order to adjust the template (for example to set description, add a content, "
    "menu binding, autorun flags etc.)\n"
    "\n"
    "This method must be called after \\register has called.\n"
  ) + 
  gsi::method ("supports_include_expansion=", &MacroInterpreterImpl::set_supports_include_expansion, gsi::arg ("flag"),
    "@brief Sets a value indicating whether this interpreter supports the default include file expansion scheme.\n"
    "If this value is set to true (the default), lines like '# %include ...' will be substituted by the "
    "content of the file following the '%include' keyword.\n"
    "Set this value to false if you don't want to support this feature.\n"
    "\n"
    "This attribute has been introduced in version 0.27.\n"
  ) +
  gsi::method ("syntax_scheme=", &MacroInterpreterImpl::set_syntax_scheme, gsi::arg ("scheme"),
    "@brief Sets a string indicating the syntax highlighter scheme\n"
    "\n"
    "The scheme string can be empty (indicating no syntax highlighting), \"ruby\" for the Ruby syntax "
    "highlighter or another string. In that case, the highlighter will look for a syntax definition "
    "under the resource path \":/syntax/<scheme>.xml\".\n"
    "\n"
    "Use this attribute setter in the initializer before registering the interpreter.\n"
    "\n"
    "Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for "
    "performance reasons in version 0.25.\n"
  ) +
  gsi::method ("debugger_scheme=", &MacroInterpreterImpl::set_debugger_scheme, gsi::arg ("scheme"),
    "@brief Sets the debugger scheme (which debugger to use for the DSL macro)\n"
    "\n"
    "The value can be one of the constants \\RubyDebugger or \\NoDebugger.\n"
    "\n"
    "Use this attribute setter in the initializer before registering the interpreter.\n"
    "\n"
    "Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for "
    "performance reasons in version 0.25.\n"
  ) +
  gsi::method ("storage_scheme=", &MacroInterpreterImpl::set_storage_scheme, gsi::arg ("scheme"),
    "@brief Sets the storage scheme (the format as which the macro is stored)\n"
    "\n"
    "This value indicates how files for this DSL macro type shall be stored. "
    "The value can be one of the constants \\PlainTextFormat, \\PlainTextWithHashAnnotationsFormat and \\MacroFormat.\n"
    "\n"
    "Use this attribute setter in the initializer before registering the interpreter.\n"
    "\n"
    "Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for "
    "performance reasons in version 0.25.\n"
  ) +
  gsi::method ("description=", &MacroInterpreterImpl::set_description, gsi::arg ("description"),
    "@brief Sets a description string\n"
    "\n"
    "This string is used for showing the type of DSL macro in the file selection box together with the "
    "suffix for example. "
    "\n"
    "Use this attribute setter in the initializer before registering the interpreter.\n"
    "\n"
    "Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for "
    "performance reasons in version 0.25.\n"
  ) +
  gsi::method ("suffix=", &MacroInterpreterImpl::set_suffix, gsi::arg ("suffix"),
    "@brief Sets the file suffix\n"
    "\n"
    "This string defines which file suffix to associate with the DSL macro. If an empty string is given (the default) "
    "no particular suffix is assciated with that macro type and \"lym\" is assumed. "
    "\n"
    "Use this attribute setter in the initializer before registering the interpreter.\n"
    "\n"
    "Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for "
    "performance reasons in version 0.25.\n"
  ) +
  gsi::callback ("executable", &MacroInterpreterImpl::executable, &MacroInterpreterImpl::f_executable, gsi::arg ("macro"),
    "@brief Returns the executable object which implements the macro execution\n"
    "This method must be reimplemented to return an \\Executable object for the actual implementation. "
    "The system will use this function to execute the script when a macro with interpreter type 'dsl' and the "
    "name of this interpreter is run.\n"
    "\n"
    "@param macro The macro to execute\n"
    "\n"
    "This method has been introduced in version 0.27 and replaces the 'execute' method.\n"
  ),
  "@brief A custom interpreter for a DSL (domain specific language)\n"
  "\n"
  "DSL interpreters are a way to provide macros written in a language specific for the "
  "application. One example are DRC scripts which are written in some special language "
  "optimized for DRC ruledecks. Interpreters for such languages "
  "can be built using scripts itself by providing the interpreter implementation through "
  "this object.\n"
  "\n"
  "An interpreter implementation involves at least these steps:\n"
  "\n"
  "@ul\n"
  "@li Derive a new object from RBA::MacroInterpreter @/li\n"
  "@li Reimplement the \\execute method for the actual execution of the code @/li\n"
  "@li In the initialize method configure the object using the attribute setters like \\suffix= and register the object as DSL interpreter (in that order) @/li\n"
  "@li Create at least one template macro in the initialize method @/li\n"
  "@/ul\n"
  "\n"
  "Template macros provide a way for the macro editor to present macros for the new interpreter in the "
  "list of templates. Template macros can provide menu bindings, shortcuts and some initial text for example\n"
  "\n"
  "The simple implementation can be enhanced by providing more information, i.e. syntax highlighter "
  "information, the debugger to use etc. This involves reimplementing further methods, i.e. \"syntax_scheme\".\n"
  "\n"
  "This is a simple example for an interpreter in Ruby. Is is registered under the name 'simple-dsl' and "
  "just evaluates the script text:\n"
  "\n"
  "@code\n"
  "class SimpleExecutable < RBA::Excutable\n"
  "\n"
  "  # Constructor\n"
  "  def initialize(macro)\n"
  "    \\@macro = macro\n"
  "  end\n"
  "  \n"
  "  # Implements the execute method\n"
  "  def execute\n"
  "    eval(\\@macro.text, nil, \\@macro.path)\n"
  "    nil\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "class SimpleInterpreter < RBA::MacroInterpreter\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.description = \"A test interpreter\"\n"
  "    # Registers the new interpreter\n"
  "    register(\"simple-dsl\")\n"
  "    # create a template for the macro editor:\n"
  "    # Name is \"new_simple\", the description will be \"Simple interpreter macro\"\n"
  "    # in the \"Special\" group.\n"
  "    mt = create_template(\"new_simple\")\n"
  "    mt.description = \"Special;;Simple interpreter macro\"\n" 
  "  end\n"
  "  \n"
  "  # Creates the executable delegate\n"
  "  def executable(macro)\n"
  "    SimpleExecutable::new(macro)\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# Register the new interpreter\n"
  "SimpleInterpreter::new\n"
  "\n"
  "@/code\n"
  "\n"
  "Please note that such an implementation is dangerous because the evaluation of the script "
  "happens in the context of the interpreter object. In this implementation the script could redefine "
  "the execute method for example. This implementation is provided as an example only.\n"
  "A real implementation should add execution of prolog and epilog code inside the execute method "
  "and proper error handling.\n"
  "\n"
  "In order to make the above code effective, store the code in an macro, set \"early auto-run\" and restart KLayout.\n"
  "\n"
  "This class has been introduced in version 0.23 and modified in 0.27.\n"
);

//  Inject the Macro::Format declarations into MacroInterpreter:
gsi::ClassExt<MacroInterpreterImpl> inject_Format_in_parent (decl_FormatEnum.defs ());

static lym::Macro *macro_by_path (const std::string &path)
{
#if defined(HAVE_QT)
  return lym::MacroCollection::root ().find_macro (path);
#else
  return 0;
#endif
}

static std::string real_path (const std::string &path, int line)
{
  if (! path.empty () && path[0] == '@') {
    return tl::IncludeExpander::from_string (path).translate_to_original (line).first;
  } else {
    return path;
  }
}

static int real_line (const std::string &path, int line)
{
  if (! path.empty () && path[0] == '@') {
    return tl::IncludeExpander::from_string (path).translate_to_original (line).second;
  } else {
    return line;
  }
}

lym::Macro *new_from_path (const std::string &path)
{
  std::unique_ptr<lym::Macro> m (new lym::Macro ());
  m->set_is_file ();
  m->set_file_path (path);
  m->load_from (path);
  return m.release ();
}

Class<lym::Macro> decl_Macro ("lay", "Macro",
  gsi::constructor ("new", &new_from_path, gsi::arg ("path"),
    "@brief Loads the macro from the given file path\n"
    "\n"
    "This constructor has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("run", &lym::Macro::run,
    "@brief Executes the macro\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("save_to", &lym::Macro::save_to, gsi::arg ("path"),
    "@brief Saves the macro to the given file\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("version", &lym::Macro::version,
    "@brief Gets the macro's version\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("version=", &lym::Macro::set_version, gsi::arg ("version"),
    "@brief Sets the macro's version\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("doc", &lym::Macro::doc,
    "@brief Gets the macro's documentation string\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("doc=", &lym::Macro::set_doc, gsi::arg ("doc"),
    "@brief Sets the macro's documentation string\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("shortcut", &lym::Macro::shortcut,
    "@brief Gets the macro's keyboard shortcut\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("shortcut=", &lym::Macro::set_shortcut, gsi::arg ("shortcut"),
    "@brief Sets the macro's keyboard shortcut\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("is_autorun?", &lym::Macro::is_autorun,
    "@brief Gets a flag indicating whether the macro is automatically executed on startup\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("is_autorun=", &lym::Macro::set_autorun, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the macro is automatically executed on startup\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("is_autorun_early?", &lym::Macro::is_autorun_early,
    "@brief Gets a flag indicating whether the macro is automatically executed early on startup\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("is_autorun_early=", &lym::Macro::set_autorun_early, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the macro is automatically executed early on startup\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("format", &lym::Macro::format,
    "@brief Gets the macro's storage format\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("format=", &lym::Macro::set_format, gsi::arg ("format"),
    "@brief Sets the macro's storage format\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("interpreter", &lym::Macro::interpreter,
    "@brief Gets the macro's interpreter\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("interpreter=", &lym::Macro::set_interpreter, gsi::arg ("interpreter"),
    "@brief Sets the macro's interpreter\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("interpreter_name", &lym::Macro::interpreter_name,
    "@brief Gets the macro interpreter name\n"
    "This is the string version of \\interpreter.\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("dsl_interpreter", &lym::Macro::dsl_interpreter,
    "@brief Gets the macro's DSL interpreter name (if interpreter is DSLInterpreter)\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("dsl_interpreter=", &lym::Macro::set_dsl_interpreter, gsi::arg ("dsl_interpreter"),
    "@brief Sets the macro's DSL interpreter name (if interpreter is DSLInterpreter)\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("sync_text_with_properties", &lym::Macro::sync_text_with_properties,
    "@brief Synchronizes the macro text with the properties\n"
    "\n"
    "This method applies to PlainTextWithHashAnnotationsFormat format. The macro text will "
    "be enhanced with pseudo-comments reflecting the macro properties. This way, the macro "
    "properties can be stored in plain files.\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("sync_properties_with_text", &lym::Macro::sync_properties_with_text,
    "@brief Synchronizes the macro properties with the text\n"
    "\n"
    "This method performs the reverse process of \\sync_text_with_properties.\n"
    "\n"
    "This method has been introduced in version 0.27.5.\n"
  ) +
  gsi::method ("path", &lym::Macro::path,
    "@brief Gets the path of the macro\n"
    "\n"
    "The path is the path where the macro is stored, starting with an abstract group identifier. "
    "The path is used to identify the macro in the debugger for example."
  ) + 
  gsi::method ("macro_by_path", &macro_by_path, gsi::arg ("path"),
    "@brief Finds the macro by installation path\n"
    "\n"
    "Returns nil if no macro with this path can be found.\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("name", &lym::Macro::name,
    "@brief Gets the name of the macro\n"
    "\n"
    "This attribute has been added in version 0.25."
  ) +
  gsi::method ("description", &lym::Macro::description,
    "@brief Gets the description text\n"
    "\n"
    "The description text of a macro will appear in the macro list. If used as a macro template, "
    "the description text can have the format \"Group;;Description\". In that case, the macro "
    "will appear in a group with title \"Group\"."
  ) + 
  gsi::method ("description=", &lym::Macro::set_description, gsi::arg ("description"),
    "@brief Sets the description text\n"
    "@param description The description text.\n"
    "See \\description for details.\n"
  ) +
  gsi::method ("prolog", &lym::Macro::prolog,
    "@brief Gets the prolog code\n"
    "\n"
    "The prolog is executed before the actual code is executed. Interpretation depends on the "
    "implementation of the DSL interpreter for DSL macros."
  ) + 
  gsi::method ("prolog=", &lym::Macro::set_prolog, gsi::arg ("string"),
    "@brief Sets the prolog\n"
    "See \\prolog for details.\n"
  ) +
  gsi::method ("epilog", &lym::Macro::epilog,
    "@brief Gets the epilog code\n"
    "\n"
    "The epilog is executed after the actual code is executed. Interpretation depends on the "
    "implementation of the DSL interpreter for DSL macros."
  ) + 
  gsi::method ("epilog=", &lym::Macro::set_epilog, gsi::arg ("string"),
    "@brief Sets the epilog\n"
    "See \\epilog for details.\n"
  ) +
  gsi::method ("category", &lym::Macro::category,
    "@brief Gets the category tags\n"
    "\n"
    "The category tags string indicates to which categories a macro will belong to. This string "
    "is only used for templates currently and is a comma-separated list of category names."
  ) + 
  gsi::method ("category=", &lym::Macro::set_category, gsi::arg ("string"),
    "@brief Sets the category tags string\n"
    "See \\category for details.\n"
  ) +
  gsi::method ("text", &lym::Macro::text,
    "@brief Gets the macro text\n"
    "\n"
    "The text is the code executed by the macro interpreter. "
    "Depending on the DSL interpreter, the text can be any kind of code."
  ) + 
  gsi::method ("text=", &lym::Macro::set_text, gsi::arg ("string"),
    "@brief Sets the macro text\n"
    "See \\text for details.\n"
  ) +
  gsi::method ("show_in_menu?", &lym::Macro::show_in_menu,
    "@brief Gets a value indicating whether the macro shall be shown in the menu\n"
  ) + 
  gsi::method ("show_in_menu=", &lym::Macro::set_show_in_menu, gsi::arg ("flag"),
    "@brief Sets a value indicating whether the macro shall be shown in the menu\n"
  ) +
  gsi::method ("group_name", &lym::Macro::group_name,
    "@brief Gets the menu group name\n"
    "\n"
    "If a group name is specified and \\show_in_menu? is true, the macro will appear in "
    "a separate group (separated by a separator) together with other macros sharing the same group."
  ) + 
  gsi::method ("group_name=", &lym::Macro::set_group_name, gsi::arg ("string"),
    "@brief Sets the menu group name\n"
    "See \\group_name for details.\n"
  ) +
  gsi::method ("menu_path", &lym::Macro::menu_path,
    "@brief Gets the menu path\n"
    "\n"
    "If a menu path is specified and \\show_in_menu? is true, the macro will appear in "
    "the menu at the specified position."
  ) + 
  gsi::method ("menu_path=", &lym::Macro::set_menu_path, gsi::arg ("string"),
    "@brief Sets the menu path\n"
    "See \\menu_path for details.\n"
  ) +
  gsi::method ("real_path", &real_path, gsi::arg ("path"), gsi::arg ("line"),
    "@brief Gets the real path for an include-encoded path and line number\n"
    "\n"
    "When using KLayout's include scheme based on '# %include ...', __FILE__ and __LINE__ (Ruby) will "
    "not have the proper values but encoded file names. This method allows retrieving the real file by using\n"
    "\n"
    "@code\n"
    "# Ruby\n"
    "real_file = RBA::Macro::real_path(__FILE__, __LINE__)\n"
    "@/code\n"
    "\n"
    "This substitution is not required for top-level macros as KLayout's interpreter will automatically use this "
    "function instead of __FILE__. Call this function when you need __FILE__ from files "
    "included through the languages mechanisms such as 'require' or 'load' where this substitution does not happen.\n"
    "\n"
    "For Python there is no equivalent for __LINE__, so you always have to use:\n"
    "\n"
    "@code\n"
    "# Python"
    "import inspect\n"
    "real_file = pya.Macro.real_path(__file__, inspect.currentframe().f_back.f_lineno)\n"
    "@/code\n"
    "\n"
    "This feature has been introduced in version 0.27."
  ) +
  gsi::method ("real_line", &real_line, gsi::arg ("path"), gsi::arg ("line"),
    "@brief Gets the real line number for an include-encoded path and line number\n"
    "\n"
    "When using KLayout's include scheme based on '# %include ...', __FILE__ and __LINE__ (Ruby) will "
    "not have the proper values but encoded file names. This method allows retrieving the real line number by using\n"
    "\n"
    "@code\n"
    "# Ruby\n"
    "real_line = RBA::Macro::real_line(__FILE__, __LINE__)\n"
    "\n"
    "# Python\n"
    "real_line = pya::Macro::real_line(__file__, __line__)\n"
    "@/code\n"
    "\n"
    "This substitution is not required for top-level macros as KLayout's interpreter will automatically use this "
    "function instead of __FILE__. Call this function when you need __FILE__ from files "
    "included through the languages mechanisms such as 'require' or 'load' where this substitution does not happen.\n"
    "\n"
    "For Python there is no equivalent for __LINE__, so you always have to use:\n"
    "\n"
    "@code\n"
    "# Python"
    "import inspect\n"
    "real_line = pya.Macro.real_line(__file__, inspect.currentframe().f_back.f_lineno)\n"
    "@/code\n"
    "\n"
    "This feature has been introduced in version 0.27."
  ),
  "@brief A macro class\n"
  "\n"
  "This class is provided mainly to support generation of template macros in the "
  "DSL interpreter framework provided by \\MacroInterpreter. The implementation may be "
  "enhanced in future versions and provide access to macros stored inside KLayout's macro repository."
  "\n"
  "But it can be used to execute macro code in a consistent way:\n"
  "\n"
  "@code\n"
  "path = \"path-to-macro.lym\"\n"
  "RBA::Macro::new(path).run()\n"
  "@/code\n"
  "\n"
  "Using the Macro class with \\run for executing code will chose the right interpreter and is "
  "able to execute DRC and LVS scripts in the proper environment. This also provides an option to "
  "execute Ruby code from Python and vice versa.\n"
  "\n"
  "In this scenario you can pass values to the script using \\Interpreter#define_variable. "
  "The interpreter to choose for DRC and LVS scripts is \\Interpreter#ruby_interpreter. "
  "For passing values back from the script, wrap the variable value into a \\Value object "
  "which can be modified by the called script and read back by the caller."
);

//  Inject the Macro::Format declarations into MacroInterpreter:
gsi::ClassExt<lym::Macro> inject_Format_in_macro (decl_FormatEnum.defs ());
gsi::ClassExt<lym::Macro> inject_Interpreter_in_macro (decl_InterpreterEnum.defs ());

}

