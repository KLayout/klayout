
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


#include "tlSystemPaths.h"
#include "tlString.h"

#include <QDir>
#include <QCoreApplication>

#ifdef _WIN32
#  include <windows.h>
#endif

#include <cstdlib>

namespace tl
{

std::string 
get_appdata_path ()
{
  QDir appdata_dir = QDir::homePath ();
  QString appdata_folder;
#ifdef _WIN32
  appdata_folder = QString::fromUtf8 ("KLayout");
#else
  appdata_folder = QString::fromUtf8 (".klayout");
#endif

  //  create the basic folder hierarchy
  if (! appdata_dir.exists (appdata_folder)) {
    appdata_dir.mkdir (appdata_folder);
  }

  QString appdata_klayout_path = appdata_dir.absoluteFilePath (appdata_folder);
  QDir appdata_klayout_dir (appdata_klayout_path);

  const char *folders[] = { "macros", "drc", "libraries", "tech" };
  for (size_t i = 0; i < sizeof (folders) / sizeof (folders [0]); ++i) {
    QString folder = QString::fromUtf8 (folders [i]);
    if (! appdata_klayout_dir.exists (folder)) {
      appdata_klayout_dir.mkdir (folder);
    }
  }

  return tl::to_string (appdata_klayout_path);
}

std::string 
get_inst_path ()
{
  return tl::to_string (QCoreApplication::applicationDirPath ());
}

#ifdef _WIN32
static void 
get_other_system_paths (std::vector <std::string> &p)
{
  //  Use "Application Data" if it exists on Windows
  const wchar_t *appdata_env = 0;
  if ((appdata_env = _wgetenv (L"APPDATA")) != 0) {
    QDir appdata_dir = QString ((const QChar *) appdata_env);
    QString appdata_folder = QString::fromUtf8 ("KLayout");
    if (appdata_dir.exists () && appdata_dir.exists (appdata_folder)) {
      p.push_back (tl::to_string (appdata_dir.absoluteFilePath (appdata_folder)));
    }
  }
}
#else
static void 
get_other_system_paths (std::vector <std::string> &)
{
  //  .. nothing yet ..
}
#endif

static void
split_path (const std::string &path, std::vector <std::string> &pc)
{
  QString sep;
#ifdef _WIN32
  sep = QString::fromUtf8 (";");
#else
  sep = QString::fromUtf8 (":");
#endif

  QStringList pp = tl::to_qstring (path).split (sep, QString::SkipEmptyParts);
  for (QStringList::ConstIterator p = pp.begin (); p != pp.end (); ++p) {
    pc.push_back (tl::to_string (*p));
  }
}


static std::vector<std::string> s_klayout_path;
static bool s_klayout_path_set = false;

void
set_klayout_path (const std::vector<std::string> &path)
{
  s_klayout_path = path;
  s_klayout_path_set = true;
}

void
reset_klayout_path ()
{
  s_klayout_path.clear ();
  s_klayout_path_set = false;
}

std::vector<std::string>
get_klayout_path ()
{
  if (s_klayout_path_set) {

    return s_klayout_path;

  } else {

    std::vector<std::string> klayout_path;

    //  generate the klayout path: the first component is always the appdata path
    klayout_path.push_back (get_appdata_path ());
#ifdef _WIN32
    wchar_t *env = _wgetenv (L"KLAYOUT_PATH");
    if (env) {
      split_path (tl::to_string (QString ((const QChar *) env)), klayout_path);
    } else {
      get_other_system_paths (klayout_path);
      klayout_path.push_back (get_inst_path ());
    }
#else
    char *env = getenv ("KLAYOUT_PATH");
    if (env) {
      split_path (tl::system_to_string (env), klayout_path);
    } else {
      get_other_system_paths (klayout_path);
      klayout_path.push_back (get_inst_path ());
    }
#endif

    return klayout_path;

  }
}

}

