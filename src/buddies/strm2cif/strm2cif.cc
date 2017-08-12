
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "dbLayout.h"
#include "dbReader.h"
#include "dbCIFWriter.h"
#include "tlLog.h"

#include <QFileInfo>

namespace bd
{

class ArgBase
{
public:
  ArgBase (const char *option, const char *brief_doc, const char *long_doc)
    : m_option (option), m_brief_doc (brief_doc), m_long_doc (long_doc)
  {
    //  .. nothing yet ..
  }

  virtual ~ArgBase ()
  {
    //  .. nothing yet ..
  }

  const std::string &option () const
  {
    return m_option;
  }

  const std::string &brief_doc () const
  {
    return m_brief_doc;
  }

  const std::string &long_doc () const
  {
    return m_long_doc;
  }

  virtual void take_value (tl::Extractor & /*ex*/)
  {
    //  .. nothing yet ..
  }

  virtual ArgBase *clone () const
  {
    return new ArgBase (*this);
  }

  virtual bool wants_value () const
  {
    return false;
  }

private:
  std::string m_option;
  std::string m_brief_doc;
  std::string m_long_doc;
};

template <class T>
void extract (tl::Extractor &ex, T &t, bool /*for_list*/ = false)
{
  ex.read (t);
}

void extract (tl::Extractor &ex, std::string &t, bool for_list = false)
{
  if (*ex == '"' || *ex == '\'') {
    ex.read_quoted (t);
  } else if (for_list) {
    ex.read (t, ",");
  } else {
    t = ex.get ();
  }
}

template <class T>
void extract (tl::Extractor &ex, std::vector<T> &t, bool /*for_list*/ = false)
{
  while (! ex.at_end ()) {
    t.push_back (T ());
    extract (ex, t.back (), true);
    ex.test (",");
  }
}

template <class T>
struct type_without_const_ref
{
  typedef T inner_type;
};

template <class T>
struct type_without_const_ref<const T &>
{
  typedef T inner_type;
};

template <class T>
struct wants_value_traits
{
  bool operator() () const { return true; }
};

template <>
struct wants_value_traits<bool>
{
  bool operator() () const { return false; }
};

template <class T>
class arg_direct_setter
  : public ArgBase
{
public:
  arg_direct_setter (const char *option, T *value, const char *brief_doc, const char *long_doc)
    : ArgBase (option, brief_doc, long_doc), mp_value (value)
  {
    //  .. nothing yet ..
  }

  virtual void take_value (tl::Extractor &ex)
  {
    extract (ex, *mp_value);
  }

  virtual ArgBase *clone () const
  {
    return new arg_direct_setter<T> (*this);
  }

  virtual bool wants_value () const
  {
    return wants_value_traits<T> () ();
  }

private:
  T *mp_value;
};

template <class C, class T>
class arg_method_setter
  : public ArgBase
{
public:
  arg_method_setter (const char *option, C *object, void (C::*setter)(T), const char *brief_doc, const char *long_doc)
    : ArgBase (option, brief_doc, long_doc), mp_object (object), mp_setter (setter)
  {
    //  .. nothing yet ..
  }

  virtual void take_value (tl::Extractor &ex)
  {
    typename type_without_const_ref<T>::innter_type t = T ();
    extract (ex, t);
    (mp_object->*mp_setter) (t);
  }

  virtual ArgBase *clone () const
  {
    return new arg_method_setter<C, T> (*this);
  }

  virtual bool wants_value () const
  {
    return wants_value_traits<T> () ();
  }

private:
  C *mp_object;
  void (C::*mp_setter)(T);
};

template <class C, class T>
arg_method_setter<C, T> arg (const char *option, C *object, void (C::*setter)(T), const char *brief_doc, const char *long_doc = "")
{
  return arg_method_setter<C, T> (option, object, setter, brief_doc, long_doc);
}

template <class T>
arg_direct_setter<T> arg (const char *option, T *value, const char *brief_doc, const char *long_doc = "")
{
  return arg_direct_setter<T> (option, value, brief_doc, long_doc);
}

struct ParsedOption
{
  ParsedOption (const std::string &option)
    : optional (false)
  {
    tl::Extractor ex (option.c_str ());
    while (! ex.at_end ()) {
      if (ex.test ("--")) {
        optional = true;
        ex.read_word (long_option, "_-");
        if (ex.test ("=")) {
          ex.read_word (name);
        }
      } else if (ex.test ("-")) {
        optional = true;
        ex.read_word (short_option, "");
        if (ex.test ("=")) {
          ex.read_word (name);
        }
      } else {
        optional = ex.test ("?");
        ex.read_word (name);
      }
      ex.test("|");
    }
  }

  bool optional;
  std::string long_option, short_option, name;
};

class CommandLineOptions
{
public:
  CommandLineOptions ()
  {
    //  Populate with the built-in options
    *this << ArgBase ("-h|--help", "Shows the usage", "");
  }

  ~CommandLineOptions ()
  {
    for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
      delete *a;
    }
    m_args.clear ();
  }

  CommandLineOptions &operator<< (const ArgBase &a)
  {
    m_args.push_back (a.clone ());
    return *this;
  }

  void brief (const std::string &text)
  {
    m_brief = text;
  }

  void parse (int argc, char *argv[])
  {
    for (int i = 0; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-h" || arg == "--help") {
        produce_help (tl::to_string (QFileInfo (tl::to_qstring (argv[0])).fileName ()));
        throw tl::CancelException ();
      }
    }

    //  @@@ implement parsing
  }

private:
  std::string m_brief;
  std::vector<ArgBase *> m_args;

  void print_string_formatted (const std::string &indent, unsigned int columns, const std::string &text)
  {
    tl::info << indent << tl::noendl;

    unsigned int c = 0;
    const char *t = text.c_str ();
    while (*t) {

      const char *tt = t;
      bool at_beginning = (c == 0);
      while (*t && *t != ' ' && *t != '\n') {
        ++t;
        ++c;
        if (c == columns && !at_beginning) {
          tl::info << "";
          tl::info << indent << tl::noendl;
          c = 0;
        }
      }

      tl::info << std::string (tt, 0, t - tt) << tl::noendl;
      while (*t == ' ') {
        ++t;
      }
      if (*t == '\n') {
        ++t;
        tl::info << tl::endl << indent << tl::noendl;
        c = 0;
      } else {
        if (c + 1 == columns) {
          tl::info << tl::endl << indent << tl::noendl;
          c = 0;
        } else {
          tl::info << " " << tl::noendl;
          c += 1;
        }
      }
      while (*t == ' ') {
        ++t;
      }

    }

    tl::info << "";
  }

  std::string pad_string (unsigned int columns, const std::string &text)
  {
    std::string s = text;
    while (s.size () < size_t (columns)) {
      s += " ";
    }
    return s;
  }

  void produce_help (const std::string &program_name)
  {
    int columns = 60;

    tl::info << "Usage:" << tl::endl;
    tl::info << "  "  << program_name << "  [options]" << tl::noendl;

    for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
      ParsedOption option ((*a)->option ());
      if (! option.name.empty ()) {
        if (option.optional) {
          tl::info << "  [<" << option.name << ">]" << tl::noendl;
        } else {
          tl::info << "  <" << option.name << ">" << tl::noendl;
        }
      }
    }

    tl::info << tl::endl;
    print_string_formatted ("    ", columns, m_brief);
    tl::info << tl::endl;

    unsigned int short_option_width = 0;
    unsigned int long_option_width = 0;
    unsigned int name_width = 0;

    for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
      ParsedOption option ((*a)->option ());
      name_width = std::max (name_width, (unsigned int) option.name.size ());
      short_option_width = std::max (short_option_width, (unsigned int) option.short_option.size ());
      long_option_width = std::max (long_option_width, (unsigned int) option.long_option.size ());
    }

    tl::info << "Arguments:" << tl::endl;

    for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
      ParsedOption option ((*a)->option ());
      if (! option.short_option.empty () || ! option.long_option.empty ()) {
        continue;
      }
      std::string n = "<" + option.name + ">";
      if (option.optional) {
        n += " (optional)";
      }
      tl::info << "  " << pad_string (name_width + 13, n) << (*a)->brief_doc ();
      tl::info << "";

      if (! (*a)->long_doc ().empty ()) {
        print_string_formatted ("    ", columns, (*a)->long_doc ());
        tl::info << "";
      }
    }

    tl::info << "";
    tl::info << "Options:" << tl::endl;

    for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
      ParsedOption option ((*a)->option ());
      if (option.short_option.empty () && option.long_option.empty ()) {
        continue;
      }
      std::string name;
      if ((*a)->wants_value ()) {
        name = option.name;
        if (name.empty ()) {
          name = "value";
        }
      }
      tl::info << "  "
               << pad_string (short_option_width + 5, option.short_option.empty () ? "" : "-" + option.short_option) << " "
               << pad_string (long_option_width + 5, option.long_option.empty () ? "" : "--" + option.long_option) << " "
               << pad_string (name_width + 3, name) << " "
               << (*a)->brief_doc ();
      tl::info << "";

      if (! (*a)->long_doc ().empty ()) {
        print_string_formatted ("    ", columns, (*a)->long_doc ());
        tl::info << "";
      }
    }
  }
};

}

int 
main (int argc, char *argv [])
{
  db::CIFWriterOptions cif_options;
  std::string infile, outfile;

  bd::CommandLineOptions cmd;

  cmd << bd::arg("-od|--dummy-calls",         &cif_options.dummy_calls,       "Produces dummy calls",
                "If this option is given, the writer will produce dummy cell calls on global level for all top cells"
              )
      << bd::arg("-ob|--blank-separator",     &cif_options.blank_separator,   "Uses blanks as x/y separators",
                "If this option is given, blank characters will be used to separate x and y values. "
                "Otherwise comma characters will be used.\n"
                "Use this option if your CIF consumer cannot read comma characters as x/y separators."
              )
      << bd::arg("input",                     &infile,                        "The input file (any format, may be gzip compressed)")
      << bd::arg("output",                    &outfile,                       "The output file")
    ;

  cmd.brief ("This program will convert the given file to a CIF file");

  try {

    cmd.parse (argc, argv);

    db::Manager m;
    db::Layout layout (&m);
    db::LayerMap map;

    {
      tl::InputStream stream (infile);
      db::Reader reader (stream);
      map = reader.read (layout);
    }

    {
      tl::OutputStream stream (outfile);
      db::CIFWriter writer;
      db::SaveLayoutOptions save_options;
      save_options.set_options (cif_options);
      writer.write (layout, stream, save_options);
    }

  } catch (tl::CancelException &ex) {
    return 1;
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "ERROR: unspecific error";
  }

  return 0;
}


