
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

#include "tlLog.h"
#include "tlCommandLineParser.h"

#include <QFileInfo>

namespace tl
{

// ------------------------------------------------------------------------
//  ArgBase implementation

ArgBase::ParsedOption::ParsedOption (const std::string &option)
  : optional (false), inverted (false)
{
  tl::Extractor ex (option.c_str ());

  if (ex.test ("!")) {
    inverted = true;
  }

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

ArgBase::ArgBase (const char *option, const char *brief_doc, const char *long_doc)
  : m_option (option), m_brief_doc (brief_doc), m_long_doc (long_doc)
{
  //  .. nothing yet ..
}

ArgBase::~ArgBase ()
{
  //  .. nothing yet ..
}

bool
ArgBase::is_option () const
{
  return !m_option.short_option.empty () || !m_option.long_option.empty ();
}

// ------------------------------------------------------------------------
//  Internal argument classes to implement info arguments

class HelpArg
  : public ArgBase
{
public:
  HelpArg ()
    : ArgBase ("-h|--help", "Shows the usage and exits", "")
  {
    //  .. nothing yet ..
  }

  ArgBase *clone () const
  {
    return new HelpArg ();
  }

  void action (CommandLineOptions *options) const
  {
    options->produce_help (options->program_name ());
    throw tl::CancelException ();
  }
};

class LicenseArg
  : public ArgBase
{
public:
  LicenseArg ()
    : ArgBase ("--license", "Shows the license and exits", "")
  {
    //  .. nothing yet ..
  }

  ArgBase *clone () const
  {
    return new LicenseArg ();
  }

  void action (CommandLineOptions *options) const
  {
    options->produce_license ();
    throw tl::CancelException ();
  }
};

class VersionArg
  : public ArgBase
{
public:
  VersionArg ()
    : ArgBase ("--version", "Produces the version and exits", "")
  {
    //  .. nothing yet ..
  }

  ArgBase *clone () const
  {
    return new VersionArg ();
  }

  void action (CommandLineOptions *options) const
  {
    options->produce_version ();
    throw tl::CancelException ();
  }
};

// ------------------------------------------------------------------------
//  CommandLineOptions implementation

std::string CommandLineOptions::m_version;
std::string CommandLineOptions::m_license;

CommandLineOptions::CommandLineOptions ()
{
  //  Populate with the built-in options
  *this << HelpArg () << VersionArg () << LicenseArg ();
}

CommandLineOptions::~CommandLineOptions ()
{
  for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end (); ++a) {
    delete *a;
  }
  m_args.clear ();
}

CommandLineOptions &
CommandLineOptions::operator<< (const ArgBase &a)
{
  m_args.push_back (a.clone ());
  return *this;
}

static void
print_string_formatted (const std::string &indent, unsigned int columns, const std::string &text)
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
        c = (unsigned int) (t - tt);
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

static std::string
pad_string (unsigned int columns, const std::string &text)
{
  std::string s = text;
  while (s.size () < size_t (columns)) {
    s += " ";
  }
  return s;
}

struct NameCompare
{
  bool operator() (ArgBase *a, ArgBase *b)
  {
    if (a->is_option () != b->is_option ()) {
      return a->is_option () < b->is_option ();
    }
    if (! a->is_option ()) {
      return false;
    }
    if (a->option ().short_option.empty () != b->option ().short_option.empty ()) {
      return a->option ().short_option.empty () < b->option ().short_option.empty ();
    }
    if (a->option ().short_option != b->option ().short_option) {
      return a->option ().short_option < b->option ().short_option;
    }
    return a->option ().long_option < b->option ().long_option;
  }
};

void
CommandLineOptions::produce_help (const std::string &program_name)
{
  int columns = 80;

  tl::info << "Usage:" << tl::endl;
  tl::info << "  "  << program_name << "  [options]" << tl::noendl;

  std::vector<ArgBase *> sorted_args = m_args;
  std::stable_sort (sorted_args.begin (), sorted_args.end (), NameCompare ());

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    if (! (*a)->is_option ()) {
      if ((*a)->option ().optional) {
        tl::info << "  [<" << (*a)->option ().name << ">]" << tl::noendl;
      } else {
        tl::info << "  <" << (*a)->option ().name << ">" << tl::noendl;
      }
    }
  }

  tl::info << tl::endl;
  print_string_formatted ("    ", columns, m_brief);
  tl::info << tl::endl;

  unsigned int short_option_width = 0;
  unsigned int long_option_width = 0;
  unsigned int name_width = 0;

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    name_width = std::max (name_width, (unsigned int) (*a)->option ().name.size ());
    short_option_width = std::max (short_option_width, (unsigned int) (*a)->option ().short_option.size ());
    long_option_width = std::max (long_option_width, (unsigned int) (*a)->option ().long_option.size ());
  }

  tl::info << "Arguments:" << tl::endl;

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    if ((*a)->is_option ()) {
      continue;
    }
    std::string n = "<" + (*a)->option ().name + ">";
    if ((*a)->option ().optional) {
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

  print_string_formatted ("  ", columns,
                          "Options can be specified in a short (with one dash) or a long form "
                          "(with two dashes). If a value is required, it can be specified either "
                          "as the following argument or added to the option with an equal sign (=).");

  tl::info << "  List of options:" << tl::endl;

  tl::info << "    "
           << pad_string (short_option_width + 5, "Short") << " "
           << pad_string (long_option_width + 5, "Long") << " "
           << pad_string (name_width + 3, "Value") << " " << "Description" << tl::endl;

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    if (! (*a)->is_option ()) {
      continue;
    }
    std::string name;
    if ((*a)->wants_value ()) {
      name = (*a)->option ().name;
      if (name.empty ()) {
        name = "value";
      }
    }
    tl::info << "    "
             << pad_string (short_option_width + 5, (*a)->option ().short_option.empty () ? "" : "-" + (*a)->option ().short_option) << " "
             << pad_string (long_option_width + 5, (*a)->option ().long_option.empty () ? "" : "--" + (*a)->option ().long_option) << " "
             << pad_string (name_width + 3, name) << " "
             << (*a)->brief_doc ();
    tl::info << "";

    if (! (*a)->long_doc ().empty ()) {
      print_string_formatted ("      ", columns, (*a)->long_doc ());
      tl::info << "";
    }
  }
}

void
CommandLineOptions::produce_license ()
{
  tl::info << m_license;
}

void
CommandLineOptions::produce_version ()
{
  tl::info << m_version;
}

void
CommandLineOptions::parse (int argc, char *argv[])
{
  m_program_name = tl::to_string (QFileInfo (QString::fromLocal8Bit (argv [0])).fileName ());

  std::vector<ArgBase *>::const_iterator next_plain_arg = m_args.begin ();
  while (next_plain_arg != m_args.end () && (*next_plain_arg)->is_option ()) {
    ++next_plain_arg;
  }

  for (int i = 1; i < argc; ++i) {

    ArgBase *arg = 0;

    std::string arg_as_utf8 = tl::to_string (QString::fromLocal8Bit (argv [i]));
    tl::Extractor ex (arg_as_utf8.c_str ());

    if (ex.test ("--")) {

      std::string n;
      ex.read_word (n);
      for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end () && !arg; ++a) {
        if ((*a)->option ().long_option == n) {
          arg = *a;
        }
      }

      if (!arg) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unknown command line option --%1 (use -h for help)").arg (tl::to_qstring (n))));
      }

    } else if (ex.test ("-")) {

      std::string n;
      ex.read_word (n);
      for (std::vector<ArgBase *>::const_iterator a = m_args.begin (); a != m_args.end () && !arg; ++a) {
        if ((*a)->option ().short_option == n) {
          arg = *a;
        }
      }

      if (!arg) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unknown command line option -%1 (use -h for help)").arg (tl::to_qstring (n))));
      }

    } else {

      if (next_plain_arg == m_args.end ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unknown command line component %1 - no further plain argument expected (use -h for help)").arg (tl::to_qstring (arg_as_utf8))));
      }

      arg = *next_plain_arg++;

      while (next_plain_arg != m_args.end () && (*next_plain_arg)->is_option ()) {
        ++next_plain_arg;
      }

    }

    if (arg->wants_value ()) {

      if (! arg->is_option () || ex.test ("=")) {
        arg->take_value (ex);
      } else {

        if (! ex.at_end ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Syntax error in argument at \"..%1\" (use -h for help)").arg (tl::to_qstring (ex.get ()))));
        }
        ++i;
        if (i == argc) {
          throw tl::Exception (tl::to_string (QObject::tr ("Value missing for last argument (use -h for help)")));
        }

        std::string arg_as_utf8 = tl::to_string (QString::fromLocal8Bit (argv [i]));
        tl::Extractor ex_value (arg_as_utf8);
        arg->take_value (ex_value);

      }

    } else {
      if (ex.test ("=")) {
        arg->take_value (ex);
      } else {
        arg->mark_present (arg->option ().inverted);
      }
    }

    //  Exection the action if there is one
    arg->action (this);

  }

  if (next_plain_arg != m_args.end () && !(*next_plain_arg)->option ().optional) {
    throw tl::Exception (tl::to_string (QObject::tr ("Additional arguments required (use -h for help)")));
  }
}

}
