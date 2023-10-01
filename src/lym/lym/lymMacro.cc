
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


#include "lymMacro.h"
#include "lymMacroCollection.h"
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

#include "tlFileUtils.h"
#include "tlUri.h"

#include <fstream>
#include <memory>
#include <string>
#include <set>


namespace lym
{

// ----------------------------------------------------------------------

Macro::Macro ()
  : m_modified (true), m_readonly (false),
    m_autorun (false), m_autorun_default (false), m_autorun_early (false), m_was_autorun (false),
    m_priority (0), m_show_in_menu (false), m_is_file (false),
    m_interpreter (None), m_format (Macro::NoFormat)
{
  mp_parent = 0;
}

void Macro::on_menu_needs_update ()
{
#if defined(HAVE_QT)
  //  forward the signal to the root collection - the main window will attach to this
  MacroCollection::root ().on_menu_needs_update ();
#endif
}

void Macro::on_changed ()
{
  m_was_autorun = false;

#if defined(HAVE_QT)
  emit changed ();
  if (mp_parent) {
    mp_parent->on_macro_changed (this);
  }
#endif
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
  m_priority = other.m_priority;
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
    m_priority == other.m_priority &&
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
    return tl::rm_file (path ());
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
  tl::make_member (&Macro::priority, &Macro::set_priority, "priority") +
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
  m_interpreter = None;

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

      if (m_format == PlainTextWithHashAnnotationsFormat) {
        sync_properties_with_text ();
      }

    }

  } else {

    if (tl::verbosity () >= 20) {
      tl::log << "Loading macro from " << fn;
    }

    tl::InputStream stream (fn);
    tl::TextInputStream text_stream (stream);
    m_text = text_stream.read_all ();

  }

  m_modified = true;
  m_is_file = true;
  on_changed ();
}

void Macro::load_from_string (const std::string &text, const std::string &url)
{
  m_format = NoFormat;
  m_interpreter = None;

  if (tl::verbosity () >= 20) {
    tl::log << "Loading macro from " << url;
  }

  if (format_from_suffix (tl::URI (url).path (), m_interpreter, m_dsl_interpreter, m_autorun_default, m_format)) {

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
    m_text = text;
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
  return format_from_suffix_string (tl::extension_last (fn), interpreter, dsl_name, autorun_pref, format);
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
    return tl::combine_path (mp_parent->path (), m_name + suffix);
  }
  return m_name + suffix;
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
    if (! tl::rename_file (path (), tl::combine_path (parent ()->path (), n + suffix))) {
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
  }
  return tl::dirname (path ());
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
  int (lym::Macro::*int_getter) () const;
  void (lym::Macro::*int_setter) (int);
};

static PropertyField property_fields[] = {
  { "description",    &lym::Macro::description, &lym::Macro::set_description,   0, 0,                                                            0, 0 },
  { "prolog",         &lym::Macro::prolog, &lym::Macro::set_prolog,             0, 0,                                                            0, 0 },
  { "epilog",         &lym::Macro::epilog, &lym::Macro::set_epilog,             0, 0,                                                            0, 0 },
  { "version",        &lym::Macro::version, &lym::Macro::set_version,           0, 0,                                                            0, 0 },
  { "autorun",        0, 0,                                                     &lym::Macro::is_autorun, &lym::Macro::set_autorun,               0, 0 },
  { "autorun-early",  0, 0,                                                     &lym::Macro::is_autorun_early, &lym::Macro::set_autorun_early,   0, 0 },
  { "show-in-menu",   0, 0,                                                     &lym::Macro::show_in_menu, &lym::Macro::set_show_in_menu,        0, 0 },
  { "group-name",     &lym::Macro::group_name, &lym::Macro::set_group_name,     0, 0,                                                            0, 0 },
  { "menu-path",      &lym::Macro::menu_path, &lym::Macro::set_menu_path,       0, 0,                                                            0, 0 },
  { "shortcut",       &lym::Macro::shortcut, &lym::Macro::set_shortcut,         0, 0,                                                            0, 0 },
  { "priority",       0, 0,                                                     0, 0,                                                            &lym::Macro::priority, &lym::Macro::set_priority }
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
    } else if (pf->int_getter) {
      int v = (this->*(pf->int_getter)) ();
      if (v) {
        new_lines.push_back (std::string ("# $") + pf->name + ": " + tl::to_string (v));
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
    } else if (pf->int_setter) {
      (this->*(pf->int_setter)) (0);
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
          } else if (pf->int_setter) {
            int v = 0;
            tl::from_string (pex.skip (), v);
            (this->*(pf->int_setter)) (v);
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

void Macro::set_was_autorun (bool f)
{
  m_was_autorun = f;
}

void Macro::set_priority (int p)
{
  if (p != m_priority) {
    m_modified = true;
    m_priority = p;
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
          tl::error << tl::to_string (tr ("Reading class doc from ")) << path () << ": " << tl::to_string (tr ("Duplicate @class"));
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
            tl::error << tl::to_string (tr ("Reading class doc from ")) << path () << ": " << tl::to_string (tr ("Cannot find super class: ")) << super_cls_name;
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
          tl::error << tl::to_string (tr ("Reading class doc from ")) << path () << ": " << tl::to_string (tr ("@method without preceding @class"));
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

}

