
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

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <set>
#include <QFileInfo>
#include <QStringList>
#include <QDir>

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
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin: %s with error message: %s ")), pp, GetLastError ());
    return desc;
  }
  init_func = reinterpret_cast<dbp_init_func_t> (GetProcAddress (handle, init_func_name));

#else

  void *handle;
  handle = dlopen (tl::string_to_system (pp).c_str (), RTLD_LAZY);
  if (! handle) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin: %s")), pp);
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

  tl::log << tl::sprintf (tl::to_string (QObject::tr ("Loaded plugin: %s")), pp);
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
      QFileInfo fi (QString::fromUtf16 ((const ushort *) buffer, len));
      return tl::to_string (fi.absolutePath ());
    }

  }

  //  no way to get module file path
  return std::string ();

#else

  Dl_info info = { };
  if (dladdr ((void *) &init, &info)) {
    QFileInfo fi (QString::fromLocal8Bit (info.dli_fname));
    return tl::to_string (fi.absolutePath ());
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
    std::string pp = tl::to_string (QDir (tl::to_qstring (*p)).filePath (QString::fromUtf8 (db_plugin_dir)));

    QStringList name_filters;
#if defined(_WIN32)
    name_filters << QString::fromUtf8 ("*.dll");
#else
    name_filters << QString::fromUtf8 ("*.so");
#endif

    QStringList inst_modules = QDir (tl::to_qstring (pp)).entryList (name_filters);
    inst_modules.sort ();

    for (QStringList::const_iterator im = inst_modules.begin (); im != inst_modules.end (); ++im) {

      QFileInfo dbp_file (tl::to_qstring (pp), *im);
      if (dbp_file.exists () && dbp_file.isReadable ()) {

        std::string mn = tl::to_string (dbp_file.fileName ());
        if (modules.find (mn) == modules.end ()) {

          std::string m = tl::to_string (dbp_file.absoluteFilePath ());
          try {
            s_plugins.push_back (load_plugin (m));
            modules.insert (mn);
          } catch (tl::Exception (&ex)) {
            tl::error << ex.msg ();
          }

        }

      }

    }

  }
}

}
