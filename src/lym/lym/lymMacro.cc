
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "lymMacro.h"
#include "lymMacroInterpreter.h"
#include "tlExceptions.h"
#include "gsiDecl.h"
#include "gsiInterpreter.h"

#include "tlString.h"
#include "tlStableVector.h"
#include "tlClassRegistry.h"
#include "tlLog.h"
#include "tlXMLParser.h"
#include "tlGlobPattern.h"
#include "tlInclude.h"
#include "tlProgress.h"

#include "rba.h"
#include "pya.h"

#include <QFile>
#include <QDir>
#include <QUrl>
#include <QResource>

#include <fstream>
#include <memory>

namespace lym
{

// ----------------------------------------------------------------------

Macro::Macro ()
  : m_modified (true), m_readonly (false), m_autorun (false), m_autorun_default (false), m_autorun_early (false), m_show_in_menu (false), m_is_file (false), mp_parent (0), m_interpreter (None), m_format (Macro::NoFormat)
{
  // .. nothing yet ..
}

void Macro::on_menu_needs_update ()
{
  //  forward the signal to the root collection - the main window will attach to this
  MacroCollection::root ().on_menu_needs_update ();
}

void Macro::on_changed ()
{
  emit changed ();
  if (mp_parent) {
    mp_parent->on_macro_changed (this);
  }
}

void Macro::assign (const lym::Macro &other)
{
  m_description = other.m_description;
  m_version = other.m_version;
  m_prolog = other.m_prolog;
  m_category = other.m_category;
  m_epilog = other.m_epilog;
  m_text = other.m_text;
  m_doc = other.m_doc;
  m_version = other.m_version;
  m_modified = other.m_modified;
  m_readonly = other.m_readonly;
  m_autorun = other.m_autorun;
  m_autorun_default = other.m_autorun_default;
  m_autorun_early = other.m_autorun_early;
  m_show_in_menu = other.m_show_in_menu;
  m_shortcut = other.m_shortcut;
  m_format = other.m_format;
  m_group_name = other.m_group_name;
  m_menu_path = other.m_menu_path;
  m_format = other.m_format;
  m_interpreter = other.m_interpreter;
  m_dsl_interpreter = other.m_dsl_interpreter;
  m_is_file = other.m_is_file;
  m_file_path = other.m_file_path;
  on_changed ();
}

bool Macro::operator== (const Macro &other) const
{
  return 
    m_description == other.m_description &&
    m_version == other.m_version &&
    m_epilog == other.m_epilog &&
    m_prolog == other.m_prolog &&
    m_category == other.m_category &&
    m_text == other.m_text &&
    m_autorun == other.m_autorun &&
    m_autorun_early == other.m_autorun_early &&
    m_show_in_menu == other.m_show_in_menu &&
    m_shortcut == other.m_shortcut &&
    m_interpreter == other.m_interpreter &&
    m_dsl_interpreter == other.m_dsl_interpreter &&
    m_format == other.m_format;
}

void Macro::save ()
{
  save_to (path ());
}

bool Macro::del ()
{
  if (m_is_file) {
    if (tl::verbosity () >= 20) {
      tl::log << "Deleting macro " << path ();
    }
    QFile f (tl::to_qstring (path ()));
    return f.remove ();
  } else {
    return true;
  }
}

struct Interpreter2s
{
  std::string to_string (Macro::Interpreter i) const
  {
    switch (i) {
      case Macro::Ruby:
        return "ruby";
      case Macro::Python:
        return "python";
      case Macro::Text:
        return "text";
      case Macro::DSLInterpreter:
        return "dsl";
      default:
        return "none";
    }
  }

  void from_string (const std::string &s, Macro::Interpreter &i) const
  {
    if (s == "ruby") {
      i = Macro::Ruby;
    } else if (s == "python") {
      i = Macro::Python;
    } else if (s == "dsl") {
      i = Macro::DSLInterpreter;
    } else if (s == "text") {
      i = Macro::Text;
    } else {
      i = Macro::None;
    }
  }
};

/**
 *  @brief Declaration of the XML structure of a macro
 */
static tl::XMLStruct<lym::Macro> xml_struct ("klayout-macro", 
  tl::make_member (&Macro::description, &Macro::set_description, "description") +
  tl::make_member (&Macro::version, &Macro::set_version, "version") +
  tl::make_member (&Macro::category, &Macro::set_category, "category") +
  tl::make_member (&Macro::prolog, &Macro::set_prolog, "prolog") +
  tl::make_member (&Macro::epilog, &Macro::set_epilog, "epilog") +
  tl::make_member (&Macro::doc, &Macro::set_doc, "doc") +
  tl::make_member (&Macro::is_autorun, &Macro::set_autorun, "autorun") +
  tl::make_member (&Macro::is_autorun_early, &Macro::set_autorun_early, "autorun-early") +
  tl::make_member (&Macro::shortcut, &Macro::set_shortcut, "shortcut") +
  tl::make_member (&Macro::show_in_menu, &Macro::set_show_in_menu, "show-in-menu") +
  tl::make_member (&Macro::group_name, &Macro::set_group_name, "group-name") +
  tl::make_member (&Macro::menu_path, &Macro::set_menu_path, "menu-path") +
  tl::make_member (&Macro::interpreter, &Macro::set_interpreter, "interpreter", Interpreter2s ()) +
  tl::make_member (&Macro::dsl_interpreter, &Macro::set_dsl_interpreter, "dsl-interpreter-name") +
  tl::make_member (&Macro::text, &Macro::set_text, "text") +
  tl::make_member<Macro> ("format")  //  for backward compatibility
);

void Macro::save_to (const std::string &path)
{
  if (tl::verbosity () >= 20) {
    tl::log << "Saving macro to " << path;
  }

  tl::OutputStream os (path, tl::OutputStream::OM_Plain, true /*as text*/);

  if (m_format == MacroFormat) {
    xml_struct.write (os, *this);
  } else if (m_format == PlainTextWithHashAnnotationsFormat) {
    sync_text_with_properties ();
    os << text ();
  } else if (m_format == PlainTextFormat) {
    os << text ();
  }

  if (m_modified || ! m_is_file) {
    m_modified = false;
    m_is_file = true;
    on_changed ();
  }
}

void Macro::load_from (const std::string &fn)
{
  m_format = NoFormat;

  std::pair<bool, std::string> f = format_from_filename (fn, m_interpreter, m_dsl_interpreter, m_autorun_default, m_format);
  if (f.first) {

    const std::string &path = f.second;

    if (tl::verbosity () >= 20) {
      tl::log << "Loading macro from " << path;
    }

    m_autorun = m_autorun_default;

    if (m_format == MacroFormat) {

      //  default interpreter for .lym files is Ruby - but should be mentioned in the file anyway
      m_interpreter = Ruby;

      tl::XMLFileSource source (path);
      xml_struct.parse (source, *this);

    } else if (m_format == PlainTextFormat || m_format == PlainTextWithHashAnnotationsFormat) {

      tl::InputStream stream (path);
      tl::TextInputStream text_stream (stream);
      m_text = text_stream.read_all ();
      sync_properties_with_text ();

    }

  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to determine format for file from suffix or format spec ")) + fn);
  }

  m_modified = true;
  m_is_file = true;
  on_changed ();
}

void Macro::load_from_string (const std::string &text, const std::string &url)
{
  m_format = NoFormat;

  if (tl::verbosity () >= 20) {
    tl::log << "Loading macro from " << url;
  }

  if (format_from_suffix (tl::to_string (QUrl (tl::to_qstring (url)).path ()), m_interpreter, m_dsl_interpreter, m_autorun_default, m_format)) {

    m_autorun = m_autorun_default;

    if (m_format == MacroFormat) {

      tl::XMLStringSource source (text);
      xml_struct.parse (source, *this);

    } else if (m_format == PlainTextWithHashAnnotationsFormat) {

      m_text = text;
      sync_properties_with_text ();

    } else if (m_format == PlainTextFormat) {

      m_text = text;

    }

  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to determine format for file from suffix ")) + url);
  }

  m_modified = true;
  on_changed ();
}

void Macro::load ()
{
  load_from (path ());
}

bool 
Macro::format_from_suffix (const std::string &fn, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format)
{
  std::string suffix = tl::to_string (QFileInfo (tl::to_qstring (fn)).suffix ());
  return format_from_suffix_string (suffix, interpreter, dsl_name, autorun_pref, format);
}

std::pair<bool, std::string>
Macro::format_from_filename (const std::string &fn, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format)
{
  tl::GlobPattern pat ("(*)\\[(*)\\]");
  std::vector<std::string> pat_parts;
  if (pat.match (fn, pat_parts) && pat_parts.size () == 2) {
    return std::make_pair (format_from_suffix_string (pat_parts[1], interpreter, dsl_name, autorun_pref, format), pat_parts[0]);
  } else {
    return std::make_pair (format_from_suffix (fn, interpreter, dsl_name, autorun_pref, format), fn);
  }
}

bool
Macro::format_from_suffix_string (const std::string &suffix, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format)
{
  interpreter = None;
  dsl_name = std::string ();
  format = NoFormat;
  autorun_pref = false;

  //  know suffixes
  if (suffix == "rb" || suffix == "rbm") {

    autorun_pref = (suffix == "rbm");
    interpreter = Ruby;
    format = PlainTextWithHashAnnotationsFormat;
    return true;

  } else if (suffix == "py" || suffix == "pym") {

    autorun_pref = (suffix == "pym");
    interpreter = Python;
    format = PlainTextWithHashAnnotationsFormat;
    return true;

  } else if (suffix == "txt") {

    format = PlainTextFormat;
    return true;

  } else if (suffix == "lym") {

    format = MacroFormat;
    return true;

  } else if (!suffix.empty ()) {

    //  locate the suffix in the DSL interpreter declarations
    for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {

      if (cls->suffix () == suffix) {

        interpreter = DSLInterpreter; // by default - may be overridden by content of file
        dsl_name = cls.current_name ();
        format = cls->storage_scheme ();

        return true;

      }

    }

  } 

  return false;
}

std::string 
Macro::suffix_for_format (Macro::Interpreter interpreter, const std::string &dsl_name, Macro::Format format)
{
  std::string s;
  if (interpreter == DSLInterpreter) {
    s = MacroInterpreter::suffix (dsl_name);
  } else if (format == MacroFormat) {
    s = "lym";
  } else if (interpreter == Ruby) {
    s = "rb";
  } else if (interpreter == Python) {
    s = "py";
  } else {
    s = "txt";
  }
  if (! s.empty ()) {
    return "." + s;
  } else {
    return ".lym";
  }
}

std::string Macro::interpreter_name () const
{
  if (interpreter () == Ruby) {
    return "Ruby";
  } else if (interpreter () == Python) {
    return "Python";
  } else if (interpreter () == DSLInterpreter) {
    return MacroInterpreter::description (dsl_interpreter ());
  } else {
    return std::string ();
  }
}

std::string Macro::summary () const
{
  return std::string ("<html><body><b>") + interpreter_name () + "</b> " + path () + "</body></html>";
}

std::string Macro::path () const
{
  if (! m_file_path.empty ()) {
    return m_file_path;
  }

  std::string suffix = suffix_for_format (m_interpreter, m_dsl_interpreter, m_format);
  if (mp_parent) {
    return tl::to_string (QFileInfo (QDir (tl::to_qstring (mp_parent->path ())), tl::to_qstring (m_name + suffix)).filePath ());
  } else {
    return m_name + suffix;
  }
}

void Macro::set_file_path (const std::string &fp)
{
  m_file_path = fp;
}

void Macro::set_is_file ()
{
  if (! m_is_file) {
    m_is_file = true;
    on_changed ();
  }
}

void Macro::reset_modified ()
{
  if (m_modified) {
    m_modified = false;
    on_changed ();
  }
}

bool Macro::rename (const std::string &n)
{
  if (m_is_file && parent ()) {
    std::string suffix = suffix_for_format (m_interpreter, m_dsl_interpreter, m_format);
    if (tl::verbosity () >= 20) {
      tl::log << "Renaming macro " << path () << " to " << n;
    }
    QFile f (tl::to_qstring (path ()));
    if (! f.rename (QFileInfo (QDir (tl::to_qstring (parent ()->path ())), tl::to_qstring (n + suffix)).filePath ())) {
      return false;
    }
  }

  if (parent ()) {
    parent ()->rename_macro (this, n);
  }

  m_name = n;
  on_changed ();
  return true;
}

std::string Macro::dir () const
{
  if (mp_parent) {
    return mp_parent->path ();
  } else {
    return std::string ();
  }
}

std::string Macro::display_string () const
{
  std::string r = name ();
  if (! m_description.empty ()) {
    r += " - " + m_description;
  }
  if (! m_shortcut.empty ()) {
    r += " (" + m_shortcut + ")";
  }
  return r;
}

void Macro::set_doc (const std::string &d)
{
  if (m_doc != d) {
    m_modified = true;
    m_doc = d;
    on_changed ();
  }
}

void Macro::set_description (const std::string &d)
{
  if (m_description != d) {
    m_modified = true;
    m_description = d;
    if (m_show_in_menu) {
      on_menu_needs_update ();
    }
    on_changed ();
  }
}

void Macro::set_epilog (const std::string &s)
{
  if (m_epilog != s) {
    m_modified = true;
    m_epilog = s;
    on_changed ();
  }
}

void Macro::set_prolog (const std::string &s)
{
  if (m_prolog != s) {
    m_modified = true;
    m_prolog = s;
    on_changed ();
  }
}

void Macro::set_version (const std::string &s)
{
  if (m_version != s) {
    m_modified = true;
    m_version = s;
    on_changed ();
  }
}

const std::string &Macro::text () const
{
  return m_text;
}

struct PropertyField
{
  const char *name;
  const std::string &(lym::Macro::*string_getter) () const;
  void (lym::Macro::*string_setter) (const std::string &);
  bool (lym::Macro::*bool_getter) () const;
  void (lym::Macro::*bool_setter) (bool);
};

static PropertyField property_fields[] = {
  { "description",    &lym::Macro::description, &lym::Macro::set_description,   0, 0 }, 
  { "prolog",         &lym::Macro::prolog, &lym::Macro::set_prolog,             0, 0 }, 
  { "epilog",         &lym::Macro::epilog, &lym::Macro::set_epilog,             0, 0 }, 
  { "version",        &lym::Macro::version, &lym::Macro::set_version,           0, 0 }, 
  { "autorun",        0, 0,                                                     &lym::Macro::is_autorun, &lym::Macro::set_autorun },
  { "autorun-early",  0, 0,                                                     &lym::Macro::is_autorun_early, &lym::Macro::set_autorun_early},
  { "show-in-menu",   0, 0,                                                     &lym::Macro::show_in_menu, &lym::Macro::set_show_in_menu },
  { "group-name",     &lym::Macro::group_name, &lym::Macro::set_group_name,     0, 0 }, 
  { "menu-path",      &lym::Macro::menu_path, &lym::Macro::set_menu_path,       0, 0 }, 
  { "shortcut",       &lym::Macro::shortcut, &lym::Macro::set_shortcut,         0, 0 } 
};

static std::string escape_pta_string (const char *cp) 
{
  std::string res;
  while (*cp) {
    if (*cp == '\n') {
      res += "\\n";
    } else if ((unsigned char)*cp < 0x20) {
      res += " ";
    } else if (*cp == '\\') {
      res += "\\\\";
    } else {
      res += *cp;
    }
    ++cp;
  }
  return res;
}

static std::string unescape_pta_string (const char *cp) 
{
  std::string res;
  while (*cp) {
    if (*cp == '\\' && cp[1]) {
      ++cp;
      if (*cp == 'n') {
        res += "\n";
      } else {
        res += *cp;
      }
    } else {
      res += *cp;
    }
    ++cp;
  }
  return res;
}

void Macro::sync_text_with_properties ()
{
  if (m_format != PlainTextWithHashAnnotationsFormat) {
    return;
  }

  std::vector<std::string> lines = tl::split (m_text, "\n");

  std::vector<std::string> new_lines;
  for (size_t i = 0; i < sizeof (property_fields) / sizeof (property_fields[0]); ++i) {
    const PropertyField *pf = property_fields + i;
    if (pf->string_getter) {
      std::string v = (this->*(pf->string_getter)) ();
      if (! v.empty ()) {
        new_lines.push_back (std::string ("# $") + pf->name + ": " + escape_pta_string (v.c_str ()));
      }
    } else if (pf->bool_getter) {
      bool v = (this->*(pf->bool_getter)) ();
      if (v) {
        new_lines.push_back (std::string ("# $") + pf->name);
      }
    }
  }

  bool stop_fishing = false;

  for (std::vector<std::string>::const_iterator l = lines.begin (); l != lines.end (); ++l) {

    tl::Extractor ex (l->c_str ());

    bool taken = false;
    if (stop_fishing) {
      //  done - no more lines are removed
    } else if (ex.test ("#") && ex.test ("$")) {
      for (size_t i = 0; i < sizeof (property_fields) / sizeof (property_fields[0]) && !taken; ++i) {
        taken = ex.test (property_fields [i].name);
      }
    } else if (! ex.at_end ()) {
      stop_fishing = true;
    }

    if (! taken) {
      new_lines.push_back (*l);
    }

  }

  std::string new_text = tl::join (new_lines, "\n");
  if (new_text != m_text) {
    m_text = new_text;
    m_modified = true;
    on_changed ();
  }
}

void Macro::sync_properties_with_text ()
{
  if (m_format != PlainTextWithHashAnnotationsFormat) {
    return;
  }

  //  reset the properties first
  for (size_t i = 0; i < sizeof (property_fields) / sizeof (property_fields[0]); ++i) {
    const PropertyField *pf = property_fields + i;
    if (pf->string_setter) {
      (this->*(pf->string_setter)) (std::string ());
    } else if (pf->bool_setter) {
      (this->*(pf->bool_setter)) (false);
    }
  }

  m_autorun = m_autorun_default;

  std::vector<std::string> lines = tl::split (m_text, "\n");

  for (std::vector<std::string>::const_iterator l = lines.begin (); l != lines.end (); ++l) {

    tl::Extractor ex (l->c_str ());

    if (ex.test ("#") && ex.test ("$")) {

      for (size_t i = 0; i < sizeof (property_fields) / sizeof (property_fields[0]); ++i) {

        tl::Extractor pex = ex;

        const PropertyField *pf = property_fields + i;
        if (pex.test (pf->name) && (pex.at_end () || pex.test (":"))) {

          if (pf->string_setter) {
            (this->*(pf->string_setter)) (unescape_pta_string (pex.skip ()));
          } else if (pf->bool_setter) {
            (this->*(pf->bool_setter)) (true);
          }

          break;

        }

      }

    } else if (! ex.at_end ()) {
      //  stop fishing
      break;
    }

  }
}

void Macro::set_text (const std::string &t)
{
  if (text () != t) {
    m_text = t;
    m_modified = true;
    sync_properties_with_text ();
    on_changed ();
  }
}

void Macro::set_format (Format f)
{
  if (f != m_format) {
    m_modified = true;
    m_format = f;
    on_changed ();
  }
}

void Macro::set_dsl_interpreter (const std::string &n)
{
  if (n != m_dsl_interpreter) {
    m_modified = true;
    m_dsl_interpreter = n;
    on_changed ();
  }
}

void Macro::set_interpreter (Interpreter i)
{
  if (i != m_interpreter) {
    m_modified = true;
    m_interpreter = i;
    on_changed ();
  }
}

void Macro::set_autorun_early (bool f)
{
  if (f != m_autorun_early) {
    m_modified = true;
    m_autorun_early = f;
    on_changed ();
  }
}

void Macro::set_autorun (bool f)
{
  if (f != m_autorun) {
    m_modified = true;
    m_autorun = f;
    on_changed ();
  }
}

void Macro::set_show_in_menu (bool f)
{
  if (f != m_show_in_menu) {
    m_modified = true;
    m_show_in_menu = f;
    on_menu_needs_update ();
    on_changed ();
  }
}

void Macro::set_menu_path (const std::string &mp)
{
  if (m_menu_path != mp) {
    m_modified = true;
    m_menu_path = mp;
    on_menu_needs_update ();
    on_changed ();
  }
}

void Macro::set_group_name (const std::string &g)
{
  if (m_group_name != g) {
    m_modified = true;
    m_group_name = g;
    on_changed ();
  }
}

void Macro::set_shortcut (const std::string &s)
{
  if (s != m_shortcut) {
    m_modified = true;
    m_shortcut = s;
    on_menu_needs_update ();
    on_changed ();
  }
}

void Macro::set_readonly (bool f)
{
  if (m_readonly != f) {
    m_readonly = f;
    on_changed ();
  }
}

class ExternalMethod
  : public gsi::MethodBase
{
public:
  ExternalMethod (const std::string &name, const std::string &doc, bool c, bool s)
    : gsi::MethodBase (name, doc, c, s)
  {
    //  no return type
    gsi::ArgType a;
    a.set_type (gsi::BasicType (-1));
    set_return (a);
  }

  virtual MethodBase *clone () const 
  {
    return new ExternalMethod (*this);
  }

  //  this class is not intended to go functional. It's just a hook for the documentation
  virtual void call(void*, gsi::SerialArgs&, gsi::SerialArgs&) const
  {
    tl_assert (false); 
  }
};

/** 
 *  @brief A descriptor for an external class (scripted)
 *
 *  This declaration is not intended to go functional. It's just a hook for the documentation.
 */
class ExternalClass 
  : public gsi::ClassBase
{
public:
  ExternalClass (const std::string &module, const std::string &name, const std::string &category, const gsi::ClassBase *base, const std::string &doc, const gsi::Methods &mm)
    : gsi::ClassBase (doc, mm), m_category (category)
  {
    set_module (module);
    set_name (name);
    set_base (base);
  }

  const std::string &category () const
  {
    return m_category;
  }

  virtual bool consolidate () const
  {
    return true;
  }

  virtual bool is_external () const
  {
    return true;
  }

  virtual bool can_upcast (const void *) const
  {
    //  It does not make sense to upcast-check vs. an external class
    //  An external class is only provided as a stub.
    return false;
  }

private:
  std::string m_category;
};

void Macro::install_doc () const
{
  std::vector<std::string> lines = tl::split (tl::trim (doc ()), "\n");
  if (! lines.empty () && tl::trim (lines [0]).find ("@class") == 0) {

    //  this macro provides documentation for the GSI namespace
    gsi::ClassBase *cls = 0;

    for (size_t i = 0; i < lines.size (); ++i) {

      bool st = false;
      tl::Extractor ex (lines [i].c_str ());
      if (ex.test ("@class")) {

        std::string module;
        if (ex.test ("[")) {
          ex.read_word_or_quoted (module);
          ex.test ("]");
        }

        std::string cls_name, super_cls_name;
        ex.read_word_or_quoted (cls_name);
        if (ex.test ("<")) {
          ex.read_word_or_quoted (super_cls_name);
        }

        std::string doc;
        while (++i < lines.size ()) {
          std::string l = tl::trim (lines [i]);
          if (l.find ("@method") == 0 || l.find ("@static_method") == 0) {
            break;
          }
          if (! doc.empty ()) {
            doc += "\n";
          }
          doc += lines [i];
        }
        --i;

        if (cls) {
          tl::error << tl::to_string (QObject::tr ("Reading class doc from ")) << path () << ": " << tl::to_string (QObject::tr ("Duplicate @class"));
          return;
        }

        for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
          if (c->name () == cls_name) {
            const ExternalClass *ec = dynamic_cast<const ExternalClass *> (&*c);
            if (!ec || ec->category () == category ()) {
              cls = const_cast <gsi::ClassBase *> (&*c);
            }
          }
        }

        const gsi::ClassBase *super_cls = 0;
        if (! super_cls_name.empty ()) {
          for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
            if (c->name () == super_cls_name) {
              super_cls = &*c;
              break;
            }
          }
          if (! super_cls) {
            tl::error << tl::to_string (QObject::tr ("Reading class doc from ")) << path () << ": " << tl::to_string (QObject::tr ("Cannot find super class: ")) << super_cls_name;
            return;
          }
        }

        if (! cls) {
          //  create a new class declaration
          static tl::stable_vector<ExternalClass> ext_classes;
          ExternalClass *ext_cls = new ExternalClass (module, cls_name, category (), super_cls, doc, gsi::Methods ());
          ext_classes.push_back (ext_cls);
          cls = ext_cls;
        }

      } else if (ex.test ("@method") || (st = ex.test ("@static_method")) == true) {

        if (cls == 0) {
          tl::error << tl::to_string (QObject::tr ("Reading class doc from ")) << path () << ": " << tl::to_string (QObject::tr ("@method without preceding @class"));
        } else {

          std::string n;
          ex.read_word_or_quoted (n);

          std::string doc;
          while (++i < lines.size ()) {
            std::string l = tl::trim (lines [i]);
            if (l.find ("@method") == 0 || l.find ("@static_method") == 0) {
              break;
            }
            if (! doc.empty ()) {
              doc += "\n";
            }
            doc += lines [i];
          }
          --i;

          ExternalMethod *meth = new ExternalMethod (n, doc, false, st);
          cls->add_method (meth);

        }

      }

    }

  }
}

static gsi::Interpreter *script_interpreter (lym::Macro::Interpreter lang)
{
  gsi::Interpreter *ip = 0;

  //  This
  if (lang == lym::Macro::Ruby) {
    ip = rba::RubyInterpreter::instance ();
  } else if (lang == lym::Macro::Python) {
    ip = pya::PythonInterpreter::instance ();
  }

  return (ip && ip->available() ? ip : 0);
}

bool Macro::can_run () const
{
  gsi::Interpreter *ip = script_interpreter (interpreter ());
  if (ip) {
    return true;
  } else if (interpreter () == lym::Macro::DSLInterpreter) {
    return lym::MacroInterpreter::can_run (this);
  } else {
    return false;
  }
}

int Macro::run () const
{
  if (tl::verbosity () >= 20) {
    tl::log << tl::to_string (tr ("Running macro ")) << path ();
  }

  try {

    tl::ProgressGarbageCollector progress_gc;

    gsi::Interpreter *ip = script_interpreter (interpreter ());
    if (ip) {

      static lym::MacroInterpreter def_interpreter;

      if (! prolog ().empty ()) {
        ip->eval_string (prolog ().c_str ());
      }

      std::pair<std::string, std::string> ep = def_interpreter.include_expansion (this);
      ip->eval_string (ep.second.c_str (), ep.first.c_str (), 1);

      if (! epilog ().empty ()) {
        ip->eval_string (epilog ().c_str ());
      }

    } else if (interpreter () == lym::Macro::DSLInterpreter) {
      lym::MacroInterpreter::execute_macro (this);
    } else {
      throw tl::Exception (tl::to_string (tr ("Can't run macro (no interpreter): ")) + path ());
    }

  } catch (tl::ExitException &ex) {
    return ex.status ();
  }

  return 0;
}

// ----------------------------------------------------------------------

static MacroCollection ms_root;

MacroCollection::MacroCollection ()
  : mp_parent (0), m_virtual_mode (ProjectFolder), m_readonly (false)
{
  // .. nothing yet ..
}

MacroCollection::~MacroCollection ()
{
  for (iterator m = begin (); m != end (); ++m) {
    delete m->second;
  }
  m_macros.clear ();

  for (child_iterator mm = begin_children (); mm != end_children (); ++mm) {
    delete mm->second;
  }
  m_folders.clear ();
}

void MacroCollection::begin_changes ()
{
  //  Note: it is very important that each on_changed occurs after exactly one begin_changes.
  //  (See #459 for example)
  if (mp_parent) {
    mp_parent->begin_changes ();
  } else {
    emit about_to_change ();
  }
}

void MacroCollection::on_menu_needs_update ()
{
  emit menu_needs_update ();
}

void MacroCollection::on_changed ()
{
  //  Note: it is very important that each on_changed occurs after exactly one begin_changes.
  //  (See #459 for example)
  emit changed ();
  on_macro_collection_changed (this);
}

void MacroCollection::on_macro_collection_changed (MacroCollection *mc)
{
  if (mp_parent) {
    mp_parent->on_macro_collection_changed (mc);
  } else {
    emit macro_collection_changed (mc);
  }
}

void MacroCollection::on_child_deleted (MacroCollection *mc)
{
  emit child_deleted (mc);
  on_macro_collection_deleted (mc);
}

void MacroCollection::on_macro_collection_deleted (MacroCollection *mc)
{
  if (mp_parent) {
    mp_parent->on_macro_collection_deleted (mc);
  } else {
    emit macro_collection_deleted (mc);
  }
}

void MacroCollection::on_macro_deleted_here (Macro *macro)
{
  emit macro_deleted_here (macro);
  on_macro_deleted (macro);
}

void MacroCollection::on_macro_deleted (Macro *macro)
{
  if (mp_parent) {
    mp_parent->on_macro_deleted (macro);
  } else {
    emit macro_deleted (macro);
  }
}

void MacroCollection::on_macro_changed (Macro *macro)
{
  if (mp_parent) {
    mp_parent->on_macro_changed (macro);
  } else {
    emit macro_changed (macro);
  }
}

void MacroCollection::collect_used_nodes (std::set <Macro *> &macros, std::set <MacroCollection *> &macro_collections)
{
  for (MacroCollection::child_iterator c = begin_children (); c != end_children (); ++c) {
    macro_collections.insert (c->second);
    c->second->collect_used_nodes (macros, macro_collections);
  }
  for (MacroCollection::iterator c = begin (); c != end (); ++c) {
    macros.insert (c->second);
  }
}

Macro *MacroCollection::macro_by_name (const std::string &name, Macro::Format format)
{
  std::multimap <std::string, Macro *>::iterator i = m_macros.find (name);
  while (i != m_macros.end () && i->first == name) {
    if (format == Macro::NoFormat || i->second->format () == format) {
      return i->second;
    }
    ++i;
  } 
  return 0;
}

const Macro *MacroCollection::macro_by_name (const std::string &name, Macro::Format format) const
{
  std::multimap <std::string, Macro *>::const_iterator i = m_macros.find (name);
  while (i != m_macros.end () && i->first == name) {
    if (format == Macro::NoFormat || i->second->format () == format) {
      return i->second;
    }
    ++i;
  } 
  return 0;
}

MacroCollection *MacroCollection::folder_by_name (const std::string &name)
{
  std::map <std::string, MacroCollection *>::iterator i = m_folders.find (name);
  if (i != m_folders.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const MacroCollection *MacroCollection::folder_by_name (const std::string &name) const
{
  std::map <std::string, MacroCollection *>::const_iterator i = m_folders.find (name);
  if (i != m_folders.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

std::string MacroCollection::path () const
{
  if (m_virtual_mode) {
    return m_path;
  } else if (mp_parent) {
    return tl::to_string (QFileInfo (QDir (tl::to_qstring (mp_parent->path ())), tl::to_qstring (m_path)).filePath ());
  } else {
    return m_path;
  }
}

std::string MacroCollection::display_string () const
{
  if (m_virtual_mode) {
    return "[" + m_description + "]";
  } else {
    std::string r = name ();
    if (! m_description.empty ()) {
      r += " - " + m_description;
    }
    return r;
  }
}

void
MacroCollection::make_readonly (bool f)
{
  if (m_readonly != f) {
    begin_changes ();
    m_readonly = f;
    on_changed ();
  }
}

MacroCollection *
MacroCollection::add_folder (const std::string &description, const std::string &path, const std::string &cat, bool readonly, bool force_create)
{
  if (! path.empty () && path[0] == ':') {
    readonly = true;
  } else {

    QFileInfo file_info (tl::to_qstring (path));

    if (! file_info.exists ()) {

      //  Try to create the folder since it does not exist yet or skip that one
      if (! force_create) {

        if (tl::verbosity () >= 20) {
          tl::log << "Folder does not exist - skipping: " << path;
        }
        return 0;

      } else {

        if (tl::verbosity () >= 20) {
          tl::log << "Folder does not exist yet - trying to create it: " << path;
        }
        if (! QDir::root ().mkpath (file_info.absoluteFilePath ())) {
          if (tl::verbosity () >= 10) {
            tl::error << "Unable to create folder path: " << path;
          }
          return 0;
        }
      }

      file_info.refresh ();

    }

    if (! file_info.isDir ()) {
      if (tl::verbosity () >= 10) {
        tl::error << "Folder is not a directory: " << path;
      }
      return 0;
    }

    QString cp = file_info.canonicalFilePath ();
    if (cp.isEmpty ()) {
      return 0;
    }

    for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
      //  skip, if that folder is in the collection already
      if (QFileInfo (tl::to_qstring (f->first)).canonicalFilePath () == cp) {
        return 0;
      }
    }

    if (! readonly && ! file_info.isWritable ()) {
      readonly = true;
      if (tl::verbosity () >= 20) {
        tl::log << "Folder is read-only: " << path;
      }
    }

  }

  begin_changes ();

  MacroCollection *mc = m_folders.insert (std::make_pair (path, new MacroCollection ())).first->second;
  mc->set_name (path);
  mc->set_description (description);
  mc->set_category (cat);
  mc->set_readonly (readonly);
  mc->scan (path);
  mc->set_parent (this);

  on_changed ();
  on_macro_changed (0);

  return mc;
}

void MacroCollection::rescan ()
{
  for (std::map <std::string, MacroCollection *>::const_iterator m = m_folders.begin (); m != m_folders.end (); ++m) {
    m->second->scan (m->first);
  }
}

namespace {

  /**
   *  @brief A QResource variant that allows access to the children
   */
  class ResourceWithChildren
    : public QResource
  {
  public:
    ResourceWithChildren (const QString &path) : QResource (path) { }
    using QResource::children;
  };

}

void MacroCollection::scan (const std::string &path)
{
  if (tl::verbosity () >= 20) {
    tl::info << "Scanning macro path " << path << " (readonly=" << m_readonly << ")";
  }

  if (! path.empty () && path[0] == ':') {

    ResourceWithChildren res (tl::to_qstring (path));
    QStringList children = res.children ();
    children.sort ();

    for (QStringList::const_iterator c = children.begin (); c != children.end (); ++c) {

      std::string url = path + "/" + tl::to_string (*c);
      QResource res (tl::to_qstring (url));
      if (res.size () > 0) {

        QByteArray data;
        if (res.isCompressed ()) {
          data = qUncompress ((const unsigned char *)res.data (), (int)res.size ());
        } else {
          data = QByteArray ((const char *)res.data (), (int)res.size ());
        }

        try {

          Macro::Format format = Macro::NoFormat;
          Macro::Interpreter interpreter = Macro::None;
          std::string dsl_name;
          bool autorun = false;

          if (Macro::format_from_suffix (tl::to_string (*c), interpreter, dsl_name, autorun, format)) {

            std::string n = tl::to_string (QFileInfo (*c).baseName ());

            iterator mm = m_macros.find (n);
            bool found = false;
            while (mm != m_macros.end () && mm->first == n && ! found) {
              if ((interpreter == Macro::None || mm->second->interpreter () == interpreter) &&
                  (dsl_name.empty () || mm->second->dsl_interpreter () == dsl_name) &&
                  mm->second->format () == format) {
                found = true;
              }
              ++mm;
            }
            if (! found) {
              Macro *m = m_macros.insert (std::make_pair (n, new Macro ()))->second;
              m->set_interpreter (interpreter);
              m->set_autorun_default (autorun);
              m->set_autorun (autorun);
              m->set_dsl_interpreter (dsl_name);
              m->set_format (format);
              m->set_name (n);
              m->load_from_string (std::string (data.constData (), data.size ()), url);
              m->set_readonly (m_readonly);
              m->reset_modified ();
              m->set_is_file ();
              m->set_parent (this);
            }

          }

        } catch (tl::Exception &ex) {
          tl::error << "Reading " << url << ": " << ex.msg ();
        }

      }

    }

  } else {

    QDir dir (tl::to_qstring (path));
    QStringList filters;
    filters << QString::fromUtf8 ("*.lym");
    filters << QString::fromUtf8 ("*.txt");
    //  TODO: should be either *.rb or *.python, depending on the category.
    //  Right now we rely on the folders not containing foreign files.
    filters << QString::fromUtf8 ("*.rb");
    filters << QString::fromUtf8 ("*.py");

    //  add the suffixes in the DSL interpreter declarations
    for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
      if (! cls->suffix ().empty ()) {
        filters << tl::to_qstring ("*." + cls->suffix ());
      }
    }

    QStringList files = dir.entryList (filters, QDir::Files);
    for (QStringList::ConstIterator f = files.begin (); f != files.end (); ++f) {

      std::unique_ptr<lym::Macro> new_macro;

      try {

        std::string n = tl::to_string (QFileInfo (*f).completeBaseName ());
        std::string mp = tl::to_string (dir.absoluteFilePath (*f));

        Macro::Format format = Macro::NoFormat;
        Macro::Interpreter interpreter = Macro::None;
        std::string dsl_name;
        bool autorun = false;

        if (Macro::format_from_suffix (tl::to_string (*f), interpreter, dsl_name, autorun, format)) {

          iterator mm = m_macros.find (n);
          bool found = false;
          while (mm != m_macros.end () && mm->first == n && ! found) {
            if ((interpreter == Macro::None || mm->second->interpreter () == interpreter) &&
                (dsl_name.empty () || mm->second->dsl_interpreter () == dsl_name) && 
                mm->second->format () == format) {
              found = true;
            }
            ++mm;
          }
          if (! found) {
            Macro *m = new Macro ();
            new_macro.reset (m);
            m->set_format (format);
            m->set_autorun_default (autorun);
            m->set_autorun (autorun);
            m->set_interpreter (interpreter);
            m->set_dsl_interpreter (dsl_name);
            m->set_name (n);
            m->load_from (mp);
            m->reset_modified ();
            m->set_readonly (m_readonly);
            m->set_parent (this);
          }

        }

        if (new_macro.get ()) {
          m_macros.insert (std::make_pair (n, new_macro.release ()));
        }

      } catch (tl::Exception &ex) {
        tl::error << "Reading " << tl::to_string (*f) << " in " << path << ": " << ex.msg ();
      }

    }

    QStringList folders = dir.entryList (QDir::Dirs | QDir::NoDotAndDotDot);
    for (QStringList::ConstIterator f = folders.begin (); f != folders.end (); ++f) {

      try {

        std::string n = tl::to_string (*f);
        MacroCollection *&mc = m_folders.insert (std::make_pair (n, (MacroCollection *) 0)).first->second;
        if (! mc) {
          mc = new MacroCollection ();
          mc->set_name (n);
          mc->set_virtual_mode (NotVirtual);
          bool ro = (m_readonly || ! QFileInfo (dir.filePath (*f)).isWritable ());
          mc->set_readonly (ro);
          mc->scan (tl::to_string (dir.filePath (*f)));
          mc->set_parent (this);
        }

      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      }

    }

  }
}

void MacroCollection::erase (lym::Macro *mp)
{
  for (iterator m = m_macros.begin (); m != m_macros.end (); ++m) {
    if (m->second == mp) {
      begin_changes ();
      on_macro_deleted_here (mp);
      m_macros.erase (m);
      delete mp;
      on_changed ();
      return;
    }
  }
}

void MacroCollection::erase (lym::MacroCollection *mp)
{
  for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
    if (f->second == mp) {
      begin_changes ();
      on_child_deleted (mp);
      m_folders.erase (f);
      delete mp;
      on_changed ();
      return;
    }
  }
}

void MacroCollection::erase (iterator i) 
{
  begin_changes ();
  on_macro_deleted_here (i->second);
  delete i->second;
  m_macros.erase (i);
  on_changed ();
}

void MacroCollection::erase (child_iterator i)
{
  begin_changes ();
  on_child_deleted (i->second);
  delete i->second;
  m_folders.erase (i);
  on_changed ();
}

void MacroCollection::save ()
{
  for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
    f->second->save ();
  }

  for (iterator m = m_macros.begin (); m != m_macros.end (); ++m) {
    if (m->second->is_modified () && ! m->second->is_readonly () && ! m->second->path ().empty ()) {
      try {
        m->second->save ();
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      }
    }
  }
}

bool MacroCollection::rename (const std::string &n)
{
  if (tl::verbosity () >= 20) {
    tl::info << "Renaming macro folder " << path () << " to " << n;
  }
  QFile f (tl::to_qstring (path ()));
  begin_changes ();
  if (! f.rename (QFileInfo (QDir (tl::to_qstring (mp_parent->path ())), tl::to_qstring (n)).filePath ())) {
    on_changed ();
    return false;
  } else {
    m_path = n;
    on_changed ();
    return true;
  }
}

lym::MacroCollection *MacroCollection::create_folder (const char *prefix, bool mkdir)
{
  std::string name;
  int n = 0;
  do {
    name = (prefix ? prefix : "new_folder");
    if (n > 0) {
      name += "_" + tl::to_string (n);
    }
    if (m_folders.find (name) == m_folders.end ()) {
      break;
    }
    ++n;
  } while (true);

  if (mkdir && ! QDir (tl::to_qstring (path ())).mkdir (tl::to_qstring (name))) {
    return 0;
  }

  begin_changes ();

  lym::MacroCollection *m = m_folders.insert (std::make_pair (name, new lym::MacroCollection ())).first->second; 
  m->set_virtual_mode (NotVirtual);
  m->set_name (name);
  m->set_parent (this);

  on_changed ();

  return m;
}

lym::Macro *MacroCollection::create (const char *prefix, Macro::Format format)
{
  std::string name;
  int n = 0;
  do {
    name = (prefix ? prefix : "new_macro");
    if (n > 0) {
      name += "_" + tl::to_string (n);
    }
    if (! macro_by_name (name, format)) {
      break;
    }
    ++n;
  } while (true);

  begin_changes ();

  lym::Macro *m = m_macros.insert (std::make_pair (name, new lym::Macro ()))->second; 
  m->set_name (name);
  m->set_parent (this);

  on_changed ();

  return m;
}

void MacroCollection::add_unspecific (lym::Macro *m)
{
  begin_changes ();
  m_macros.insert (std::make_pair (m->name (), m));
  m->set_parent (this);
  on_changed ();
}

bool MacroCollection::add (lym::Macro *m)
{
  QDir d (tl::to_qstring (path ()));
  QDir dd = QFileInfo (tl::to_qstring (m->path ())).dir ();

  if (d == dd) {

    begin_changes ();
    m_macros.insert (std::make_pair (m->name (), m));
    m->set_parent (this);
    on_changed ();
    return true;

  } else {

    for (child_iterator c = begin_children (); c != end_children (); ++c) {
      if (c->second->add (m)) {
        return true;
      }
    }

    //  try to detect new child folders. If that is the case, create that folder and add
    //  the macro there.
    QDir dm (tl::to_qstring (m->dir ()));
    while (true) {

      std::string folder_name = tl::to_string (dm.dirName ());
      if (! dm.cdUp ()) {
        break;
      }

      if (dm == d) {
        begin_changes ();
        lym::MacroCollection *mc = m_folders.insert (std::make_pair (folder_name, new MacroCollection ())).first->second;
        mc->set_virtual_mode (NotVirtual);
        mc->set_parent (this);
        on_changed ();
        return mc->add (m);
      }

    }

  }

  return false;
}

bool MacroCollection::del ()
{
  if (tl::verbosity () >= 20) {
    tl::info << "Deleting macro folder " << path ();
  }
  return QDir ().rmdir (tl::to_qstring (path ()));
}

void MacroCollection::rename_macro (Macro *macro, const std::string &new_name)
{
  iterator m = m_macros.find (macro->name ());
  while (m != m_macros.end () && m->first == macro->name ()) {
    if (m->second == macro) {
      m_macros.erase (m);
      m_macros.insert (std::make_pair (new_name, macro));
      return;
    }
    ++m;
  }
}

lym::Macro *MacroCollection::find_macro (const std::string &path)
{
  for (iterator m = begin (); m != end (); ++m) {
    if (m->second->path () == path) {
      return m->second;
    }
  }

  for (child_iterator mc = begin_children (); mc != end_children (); ++mc) {
    lym::Macro *macro = mc->second->find_macro (path);
    if (macro) {
      return macro;
    }
  }

  return 0;
}

MacroCollection &MacroCollection::root ()
{
  return ms_root;
}

static bool sync_macros (lym::MacroCollection *current, lym::MacroCollection *actual, bool safe)
{
  bool ret = false;

  if (actual) {
    current->make_readonly (actual->is_readonly ());
  }

  std::vector<lym::MacroCollection *> folders_to_delete;

  for (lym::MacroCollection::child_iterator m = current->begin_children (); m != current->end_children (); ++m) {
    lym::MacroCollection *cm = actual ? actual->folder_by_name (m->first) : 0;
    if (! cm) {
      folders_to_delete.push_back (m->second);
    }
  }

  if (actual) {
    for (lym::MacroCollection::child_iterator m = actual->begin_children (); m != actual->end_children (); ++m) {
      lym::MacroCollection *cm = current->folder_by_name (m->first);
      if (! cm) {
        cm = current->create_folder (m->first.c_str (), false);
        ret = true;
      }
      if (sync_macros(cm, m->second, safe)) {
        ret = true;
      }
    }
  }

  //  delete folders which do no longer exist
  for (std::vector<lym::MacroCollection *>::iterator m = folders_to_delete.begin (); m != folders_to_delete.end (); ++m) {
    ret = true;
    sync_macros (*m, 0, safe);
    current->erase (*m);
  }

  std::vector<lym::Macro *> macros_to_delete;

  for (lym::MacroCollection::iterator m = current->begin (); m != current->end (); ++m) {
    lym::Macro *cm = actual ? actual->macro_by_name (m->first, m->second->format ()) : 0;
    if (! cm) {
      macros_to_delete.push_back (m->second);
    }
  }

  if (actual) {
    for (lym::MacroCollection::iterator m = actual->begin (); m != actual->end (); ++m) {
      lym::Macro *cm = current->macro_by_name (m->first, m->second->format ());
      if (cm) {
        if (*cm != *m->second && (! safe || ! cm->is_modified ())) {
          cm->assign (*m->second);
        }
        cm->set_readonly (m->second->is_readonly ());
      } else {
        cm = current->create (m->first.c_str (), m->second->format ());
        cm->assign (*m->second);
        cm->set_readonly (m->second->is_readonly ());
        ret = true;
      }
    }
  }

  //  erase macros from collection which are no longer used
  for (std::vector<lym::Macro *>::const_iterator m = macros_to_delete.begin (); m != macros_to_delete.end (); ++m) {
    current->erase (*m);
    ret = true;
  }

  return ret;
}

void MacroCollection::reload (bool safe)
{
  //  create a new collection and synchronize

  lym::MacroCollection new_collection;
  for (lym::MacroCollection::child_iterator c = begin_children (); c != end_children (); ++c) {
    new_collection.add_folder (c->second->description (), c->second->path (), c->second->category (), c->second->is_readonly (), false /* don't force to create */);
  }

  //  and synchronize current with the actual one
  sync_macros (this, &new_collection, safe);
}

static bool has_autorun_for (const lym::MacroCollection &collection, bool early)
{
  for (lym::MacroCollection::const_child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    if (has_autorun_for (*c->second, early)) {
      return true;
    }
  }

  for (lym::MacroCollection::const_iterator c = collection.begin (); c != collection.end (); ++c) {
    if ((early && c->second->is_autorun_early ()) || (!early && c->second->is_autorun () && !c->second->is_autorun_early ())) {
      return true;
    }
  }

  return false;
}

bool MacroCollection::has_autorun () const
{
  return has_autorun_for (*this, false);
}

bool MacroCollection::has_autorun_early () const
{
  return has_autorun_for (*this, true);
}

static void autorun_for (lym::MacroCollection &collection, bool early)
{
  for (lym::MacroCollection::child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    autorun_for (*c->second, early);
  }

  for (lym::MacroCollection::iterator c = collection.begin (); c != collection.end (); ++c) {
    if (c->second->can_run () && ((early && c->second->is_autorun_early ()) || (!early && c->second->is_autorun () && !c->second->is_autorun_early ()))) {
      BEGIN_PROTECTED_SILENT
        c->second->run ();
        c->second->install_doc ();
      END_PROTECTED_SILENT
    }
  }
}

void MacroCollection::autorun ()
{
  autorun_for (*this, false);
}

void MacroCollection::autorun_early ()
{
  autorun_for (*this, true);
}

void MacroCollection::dump (int l)
{
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("----\n");
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection: %s\n", name ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection-path: %s\n", path ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection-description: %s\n", description ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf("Collection-readonly: %d\n", is_readonly ());
  printf ("\n");

  for (iterator m = begin (); m != end (); ++m) {
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("Name: %s%s\n", m->second->name ().c_str (), m->second->is_modified() ? "*" : "");
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Path: %s\n", m->second->path ().c_str ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Readonly: %d\n", m->second->is_readonly ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Autorun: %d\n", m->second->is_autorun ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Autorun-early: %d\n", m->second->is_autorun_early ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Description: %s\n", m->second->description ().c_str ());
  }

  for (child_iterator m = begin_children (); m != end_children (); ++m) {
    m->second->dump (l + 1);
  }
}

}

