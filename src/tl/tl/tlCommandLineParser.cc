
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

#include "tlLog.h"
#include "tlCommandLineParser.h"
#include "tlFileUtils.h"
#include "tlString.h"

namespace tl
{

// ------------------------------------------------------------------------
//  ArgBase implementation

ArgBase::ParsedOption::ParsedOption (const std::string &option)
  : optional (false), inverted (false), advanced (false), non_advanced (false), repeated (false)
{
  tl::Extractor ex (option.c_str ());

  while (! ex.at_end ()) {

    if (ex.test ("#")) {
      advanced = true;
    } else if (ex.test ("/")) {
      non_advanced = true;
    } else if (ex.test ("*")) {
      repeated = true;
    } else if (ex.test ("!")) {
      inverted = true;
    } else if (ex.test ("?")) {
      optional = true;
    } else if (ex.test ("[")) {
      const char *t = ex.get ();
      while (! ex.at_end () && *ex != ']') {
        ++ex;
      }
      group += std::string (t, 0, ex.get () - t);
      ex.test ("]");
    } else {
      break;
    }

  }

  while (! ex.at_end ()) {
    if (ex.test ("--")) {
      optional = true;
      ex.read_word (long_option, "_-");
      if (ex.test ("=")) {
        ex.read_word_or_quoted (name);
      }
    } else if (ex.test ("-")) {
      optional = true;
      ex.read_word (short_option, "");
      if (ex.test ("=")) {
        ex.read_word_or_quoted (name);
      }
    } else {
      ex.read_word_or_quoted (name);
    }
    ex.test("|");
  }
}

ArgBase::ArgBase (const std::string &option, const std::string &brief_doc, const std::string &long_doc)
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

std::string
ArgBase::option_desc () const
{
  std::string res;
  if (! m_option.short_option.empty ()) {
    res += "-" + m_option.short_option;
  }
  if (! m_option.long_option.empty ()) {
    if (! res.empty ()) {
      res += "|";
    }
    res += "--" + m_option.long_option;
  }
  if (! m_option.name.empty ()) {
    if (! res.empty ()) {
      res += "=";
    }
    res += m_option.name;
  }
  return res;
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
    options->produce_help (options->program_name (), false);
    throw tl::CancelException ();
  }
};

class AdvancedHelpArg
  : public ArgBase
{
public:
  AdvancedHelpArg ()
    : ArgBase ("/--help-all", "Shows all options (including advanced) and exits", "")
  {
    //  .. nothing yet ..
  }

  ArgBase *clone () const
  {
    return new AdvancedHelpArg ();
  }

  void action (CommandLineOptions *options) const
  {
    options->produce_help (options->program_name (), true);
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
    : ArgBase ("--version", "Shows the version and exits", "")
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

class VerbosityArg
  : public ArgBase
{
public:
  VerbosityArg ()
    : ArgBase ("-d|--debug-level", "Sets the verbosity level",
               "The verbosity level is an integer. Typical values are:\n"
               "* 0: silent\n"
               "* 10: somewhat verbose\n"
               "* 11: somewhat verbose plus timing information\n"
               "* 20: verbose\n"
               "* 21: verbose plus timing information\n"
               "..."
              )
  {
    //  .. nothing yet ..
  }

  ArgBase *clone () const
  {
    return new VerbosityArg ();
  }

  bool wants_value () const
  {
    return true;
  }

  void take_value (tl::Extractor &ex)
  {
    int d = 0;
    ex.read (d);
    tl::verbosity (d);
  }
};

// ------------------------------------------------------------------------
//  CommandLineOptions implementation

std::string CommandLineOptions::m_version;
std::string CommandLineOptions::m_license;

CommandLineOptions::CommandLineOptions ()
{
  //  Populate with the built-in options
  *this << HelpArg () << AdvancedHelpArg () << VersionArg () << LicenseArg () << VerbosityArg ();
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
    if (a->option ().group != b->option ().group) {
      return a->option ().group < b->option ().group;
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
CommandLineOptions::produce_help (const std::string &program_name, bool advanced)
{
  int columns = 70;

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

  unsigned int arg_width = 0;
  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    arg_width = std::max (arg_width, (unsigned int) (*a)->option_desc ().size ());
  }

  tl::info << "Arguments:" << tl::endl;

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {
    if ((*a)->is_option ()) {
      continue;
    }
    std::string n = "<" + (*a)->option_desc () + ">";
    if ((*a)->option ().optional) {
      n += " (optional)";
    }
    tl::info << "  " << pad_string_right (arg_width + 4, n) << (*a)->brief_doc ();
    tl::info << "";

    if (! (*a)->long_doc ().empty ()) {
      print_string_formatted ("        ", columns, (*a)->long_doc ());
      tl::info << "";
    }
  }

  tl::info << "";
  tl::info << "Options:" << tl::endl;

  print_string_formatted ("  ", columns,
                          "Options can be specified in a short (with one dash) or a long form "
                          "(with two dashes). If a value is required, it can be specified either "
                          "as the following argument or added to the option with an equal sign (=).");

  tl::info << tl::endl << "  List of options:" << tl::endl;

  std::string prev_group;
  bool hidden = false;

  for (std::vector<ArgBase *>::const_iterator a = sorted_args.begin (); a != sorted_args.end (); ++a) {

    if (! (*a)->is_option ()) {
      continue;
    } else if ((*a)->option ().advanced && !advanced) {
      hidden = true;
      continue;
    } else if ((*a)->option ().non_advanced && advanced) {
      continue;
    }

    if ((*a)->option ().group != prev_group) {
      prev_group = (*a)->option ().group;
      tl::info << tl::endl << "  " << prev_group << ":" << tl::endl;
    }

    std::string name;
    if ((*a)->wants_value ()) {
      name = (*a)->option ().name;
      if (name.empty ()) {
        name = "value";
      }
    }

    tl::info << "    "
             << pad_string_right (arg_width + 4, (*a)->option_desc ())
             << (*a)->brief_doc ();
    tl::info << "";

    if (! (*a)->long_doc ().empty ()) {
      print_string_formatted ("          ", columns, (*a)->long_doc ());
      tl::info << "";
    }

  }

  if (hidden) {
    tl::info << tl::endl << "  See --help-all for more options." << tl::endl;
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
  m_program_name = tl::filename (tl::to_string_from_local (argv [0]));

  std::vector<ArgBase *> plain_args;
  std::map<std::string, ArgBase *> arg_by_short_option, arg_by_long_option;

  for (std::vector<ArgBase *>::const_iterator i = m_args.begin (); i != m_args.end (); ++i) {
    if ((*i)->is_option ()) {
      if (! (*i)->option ().short_option.empty ()) {
        if (arg_by_short_option.find ((*i)->option ().short_option) != arg_by_short_option.end ()) {
          throw tl::Exception ("Command line parser setup: duplicate option -" + (*i)->option ().short_option);
        }
        arg_by_short_option.insert (std::make_pair ((*i)->option ().short_option, *i));
      }
      if (! (*i)->option ().long_option.empty ()) {
        if (arg_by_long_option.find ((*i)->option ().long_option) != arg_by_long_option.end ()) {
          throw tl::Exception ("Command line parser setup: duplicate option --" + (*i)->option ().long_option);
        }
        arg_by_long_option.insert (std::make_pair ((*i)->option ().long_option, *i));
      }
    } else {
      plain_args.push_back (*i);
    }
  }

  std::vector<ArgBase *>::const_iterator next_plain_arg = plain_args.begin ();

  for (int i = 1; i < argc; ++i) {

    ArgBase *arg = 0;

    std::string arg_as_utf8 = tl::to_string_from_local (argv [i]);
    tl::Extractor ex (arg_as_utf8.c_str ());

    if (ex.test ("--")) {

      std::string n;
      ex.read_word (n, "_-");
      std::map<std::string, ArgBase *>::const_iterator a = arg_by_long_option.find (n);
      if (a == arg_by_long_option.end ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unknown command line option --%s (use -h for help)")), n));
      }
      arg = a->second;

    } else if (ex.test ("-")) {

      std::string n;
      ex.read_word (n);
      std::map<std::string, ArgBase *>::const_iterator a = arg_by_short_option.find (n);
      if (a == arg_by_short_option.end ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unknown command line option -%s (use -h for help)")), n));
      }
      arg = a->second;

    } else {

      if (next_plain_arg == plain_args.end ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unknown command line component %s - no further plain argument expected (use -h for help)")), arg_as_utf8));
      }

      arg = *next_plain_arg;
      if (! arg->option ().repeated) {
        ++next_plain_arg;
      }

    }

    try {

      if (! arg->is_option ()) {

        arg->take_value (ex);

      } else if (arg->wants_value ()) {

        if (ex.test ("=")) {
          arg->take_value (ex);
        } else {

          ex.expect_end ();
          ++i;
          if (i == argc) {
            throw tl::Exception (tl::to_string (tr ("Value missing")));
          }

          std::string arg_as_utf8 = tl::to_string_from_local (argv [i]);
          tl::Extractor ex_value (arg_as_utf8);
          arg->take_value (ex_value);

        }

      } else {

        if (ex.test ("=")) {
          arg->take_value (ex);
        } else {
          arg->mark_present ();
        }
        if (arg->option ().inverted) {
          arg->invert_present ();
        }

      }

      //  Execute the action if there is one
      arg->action (this);

    } catch (tl::CancelException &) {
      throw;
    } catch (tl::Exception &ex) {

      std::string msg = "Error ";
      if (i == argc) {
        msg += "at end of argument list";
      } else {
        msg += "at argument #" + tl::to_string (i);
      }
      if (arg->is_option ()) {
        msg += " (option " + arg->option_desc () + ")";
      }
      msg += ": ";
      msg += ex.msg ();

      throw tl::Exception (msg);

    }

  }

  if (next_plain_arg != plain_args.end () && !(*next_plain_arg)->option ().optional) {
    throw tl::Exception (tl::to_string (tr ("Additional arguments required (use -h for help)")));
  }
}

}
