
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "bdReaderOptions.h"
#include "bdWriterOptions.h"
#include "gsiInterpreter.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "tlLog.h"
#include "tlCommandLineParser.h"
#include "tlFileUtils.h"
#include "rba.h"
#include "pya.h"
#include "gsi.h"
#include "gsiExpression.h"
#include "libForceLink.h"
#include "rdbForceLink.h"
#include "lymMacro.h"
#include "lymMacroCollection.h"

struct RunnerData
{
  std::string script;
  std::vector<std::pair<std::string, std::string> > vars;

  void add_var (const std::string &def)
  {
    std::string var_name, var_value;
    tl::Extractor ex (def.c_str ());
    ex.read_word (var_name);
    if (ex.test ("=")) {
      var_value = ex.get ();
    }

    vars.push_back (std::make_pair (var_name, var_value));
  }
};

BD_PUBLIC int strmrun (int argc, char *argv[])
{
  tl::CommandLineOptions cmd;
  RunnerData data;

  cmd << tl::arg ("script",                     &data.script, "The script to execute",
                  "This script will be executed by the script interpreter. "
                  "The script can be either Ruby (\".rb\") or Python (\".py\")."
                 )
      << tl::arg ("*-v|--var=\"name=value\"", &data, &RunnerData::add_var, "Defines a variable",
                  "When using this option, a global variable with name \"var\" will be defined with the string value \"value\"."
                 )
    ;

  cmd.brief ("This program runs Ruby or Python scripts with a subset of KLayout's API.");

  cmd.parse (argc, argv);

  //  create the ruby and python interpreter instances now.
  //  Hint: we do this after load_plugin, because that way the plugins can register GSI classes and methods.
  //  TODO: do this through some auto-registration
  rba::RubyInterpreter ruby;
  pya::PythonInterpreter python;

  for (std::vector< std::pair<std::string, std::string> >::const_iterator v = data.vars.begin (); v != data.vars.end (); ++v) {
    ruby.define_variable (v->first, v->second);
    python.define_variable (v->first, v->second);
  }

  //  install the built-in macros so we can run DRC and LVS scripts
  lym::MacroCollection &lym_root = lym::MacroCollection::root ();
  lym_root.add_folder (tl::to_string (tr ("Built-In")), ":/built-in-macros", "macros", true);
  lym_root.add_folder (tl::to_string (tr ("Built-In")), ":/built-in-pymacros", "pymacros", true);

  lym_root.autorun_early ();
  lym_root.autorun ();

  std::string script = tl::absolute_file_path (data.script);

  lym::Macro macro;
  macro.load_from (script);
  macro.set_file_path (script);
  return macro.run ();
}
