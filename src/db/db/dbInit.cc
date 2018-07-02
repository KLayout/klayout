
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbInit.h"
#include "dbPlugin.h"
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

#include <set>

namespace db
{

static std::list<db::PluginDescriptor> s_plugins;

const std::list<db::PluginDescriptor> &plugins ()
{
  return s_plugins;
}

static PluginDescriptor load_plugin (const std::string &pp)
{
  PluginDescriptor desc;
  desc.path = pp;

  dbp_init_func_t init_func = 0;
  static const char *init_func_name = "dbp_init";

#if defined(_WIN32)

  //  there is no "dlopen" on mingw, so we need to emulate it.
  HINSTANCE handle = LoadLibraryW ((const wchar_t *) tl::to_qstring (pp).constData ());
  if (! handle) {
    throw tl::Exception (tl::to_string (tr ("Unable to load plugin: %s with error message: %s ")), pp, GetLastError ());
    return desc;
  }
  init_func = reinterpret_cast<dbp_init_func_t> (GetProcAddress (handle, init_func_name));

#else

  void *handle;
  handle = dlopen (tl::string_to_system (pp).c_str (), RTLD_LAZY);
  if (! handle) {
    throw tl::Exception (tl::to_string (tr ("Unable to load plugin: %s")), pp);
  }
  init_func = reinterpret_cast<dbp_init_func_t> (dlsym (handle, init_func_name));

#endif

  //  If present, call the initialization function to fetch some details from the plugin
  if (init_func) {
    const char *version = 0;
    const char *description = 0;
    (*init_func) (&version, &description);
    if (version) {
      desc.version = version;
    }
    if (description) {
      desc.description = description;
    }
  }

  tl::log << tl::sprintf (tl::to_string (tr ("Loaded plugin: %s")), pp);
  return desc;
}

/**
 *  @brief Gets the path to the current module
 */
static std::string
get_module_path ()
{
#if defined(_WIN32)

  HMODULE h_module = NULL;
  if (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR) &init, &h_module)) {

    wchar_t buffer[MAX_PATH];
    int len;
    if ((len = GetModuleFileName(h_module, buffer, MAX_PATH)) > 0) {
      return tl::absolute_file_path (tl::to_string (std::wstring (buffer, 0, len)));
    }

  }

  //  no way to get module file path
  return std::string ();

#else

  Dl_info info = { };
  if (dladdr ((void *) &init, &info)) {
    return tl::absolute_file_path (tl::to_string_from_local (info.dli_fname));
  } else {
    return std::string ();
  }

#endif
}

void init (const std::vector<std::string> &_paths)
{
  std::vector<std::string> paths = _paths;
  if (paths.empty ()) {
    std::string module_path = get_module_path ();
    if (! module_path.empty ()) {
      paths.push_back (module_path);
    }
  }

  if (paths.empty ()) {
    //  nothing to do
    return;
  }

  std::set<std::string> modules;

  for (std::vector<std::string>::const_iterator p = paths.begin (); p != paths.end (); ++p) {

    //  look next to the db library, but in "db_plugins" directory
    const char *db_plugin_dir = "db_plugins";
    std::string pp = tl::combine_path (*p, db_plugin_dir);

    std::vector<std::string> ee = tl::dir_entries (pp, true, false);

    tl::GlobPattern pattern;
#if defined(_WIN32)
    pattern.set_case_sensitive (false);
    pattern = std::string ("*.dll");
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
          s_plugins.push_back (load_plugin (imp));
          modules.insert (*im);
        } catch (tl::Exception (&ex)) {
          tl::error << ex.msg ();
        }
      }

    }

  }
}

}
