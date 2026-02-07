
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "layInit.h"
#include "layPlugin.h"
#include "tlException.h"
#include "tlLog.h"
#include "tlString.h"
#include "tlFileUtils.h"
#include "tlGlobPattern.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <QFileInfo>
#include <QStringList>
#include <QDir>

namespace lay
{

static std::list<lay::PluginDescriptor> s_plugins;

const std::list<lay::PluginDescriptor> &plugins ()
{
  return s_plugins;
}

static PluginDescriptor do_load_plugin (const std::string &pp)
{
  PluginDescriptor desc;
  desc.path = pp;

  klp_init_func_t init_func = 0;
  static const char *init_func_name = "klp_init";

  //  NOTE: since we are using a different suffix ("*.klp"), we can't use QLibrary.
#ifdef _WIN32
  //  there is no "dlopen" on mingw, so we need to emulate it.
  HINSTANCE handle = LoadLibraryW ((const wchar_t *) tl::to_qstring (pp).constData ());
  if (! handle) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin: %s with error message: %s ")), pp, GetLastError ());
  }
  init_func = reinterpret_cast<klp_init_func_t> (GetProcAddress (handle, init_func_name));
#else
  void *handle;
  handle = dlopen (tl::string_to_system (pp).c_str (), RTLD_LAZY);
  if (! handle) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin: %s")), pp);
  }
  init_func = reinterpret_cast<klp_init_func_t> (dlsym (handle, init_func_name));
#endif

  //  If present, call the initialization function to fetch some details from the plugin
  if (init_func) {
    const char *version = 0;
    const char *description = 0;
    (*init_func) (&desc.autorun, &desc.autorun_early, &version, &description);
    if (version) {
      desc.version = version;
    }
    if (description) {
      desc.description = description;
    }
  }

  tl::log << "Loaded plugin '" << pp << "'";

  return desc;
}

void load_plugin (const std::string &pp)
{
  s_plugins.push_back (do_load_plugin (pp));
}

void init (const std::vector<std::string> &_paths)
{
  std::vector<std::string> paths = _paths;

  //  add the module path so we also look beside the "db" library
  std::string module_path = tl::get_module_path ((void *) &init);
  if (! module_path.empty ()) {
    paths.push_back (tl::absolute_path (module_path));
  }

  if (paths.empty ()) {
    //  nothing to do
    tl::log << tl::to_string (tr ("No lay_plugins loaded - no path given"));
    return;
  }

  std::set<std::string> modules;

  for (std::vector<std::string>::const_iterator p = paths.begin (); p != paths.end (); ++p) {

    //  look next to the lay library, but in "lay_plugins" directory
    const char *lay_plugin_dir = "lay_plugins";
    std::string pp = tl::combine_path (*p, lay_plugin_dir);

    if (tl::verbosity () >= 20) {
      tl::info << "Scanning for lay plugins: " << pp;
    }

    std::vector<std::string> ee = tl::dir_entries (pp, true, false);

    tl::GlobPattern pattern;
#if defined(_WIN32)
    pattern.set_case_sensitive (false);
    pattern = std::string ("*.dll");
#elif defined(__APPLE__)
    pattern = std::string ("*.dylib");
#else
    pattern = std::string ("*.so");
#endif

    std::vector<std::string> inst_modules;
    for (std::vector<std::string>::const_iterator e = ee.begin (); e != ee.end (); ++e) {
      if (pattern.match (*e)) {
        inst_modules.push_back (*e);
      }
    }

    std::sort (inst_modules.begin (), inst_modules.end ());

    for (std::vector<std::string>::const_iterator im = inst_modules.begin (); im != inst_modules.end (); ++im) {

      std::string imp = tl::combine_path (pp, *im);
      if (modules.find (*im) == modules.end ()) {
        try {
          s_plugins.push_back (do_load_plugin (imp));
          modules.insert (*im);
        } catch (tl::Exception &ex) {
          tl::error << ex.msg ();
        }
      }

    }

  }

}

}
