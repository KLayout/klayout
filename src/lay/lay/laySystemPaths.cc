
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "laySystemPaths.h"
#include "tlFileUtils.h"
#include "tlString.h"
#include "tlEnv.h"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace lay
{

std::string 
get_appdata_path ()
{
  if (tl::has_env ("KLAYOUT_HOME")) {
    return tl::get_env ("KLAYOUT_HOME");
  }

  QDir appdata_dir = QDir::homePath ();
  QString appdata_folder;
#ifdef _WIN32
  appdata_folder = QString::fromUtf8 ("KLayout");
#else
  appdata_folder = QString::fromUtf8 (".klayout");
#endif

  return tl::to_string (appdata_dir.absoluteFilePath (appdata_folder));
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

    std::string env = tl::get_env ("KLAYOUT_PATH");
    if (! env.empty ()) {
      split_path (env, klayout_path);
    } else {
      get_other_system_paths (klayout_path);
      klayout_path.push_back (tl::get_inst_path ());
    }

    return klayout_path;

  }
}

std::string
salt_mine_url ()
{
  return tl::get_env ("KLAYOUT_SALT_MINE", "http://sami.klayout.org/repository.xml");
}

}

