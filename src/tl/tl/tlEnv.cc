
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


#include "tlEnv.h"
#include "tlString.h"
#include "tlThreads.h"

#include <string>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#elif __APPLE__
#  include <libproc.h>
#  include <unistd.h>
#else
#  include <unistd.h>
#endif

namespace tl
{

static tl::Mutex *s_env_lock = 0;
static std::map<std::string, std::string> s_env_map;

std::string get_env (const std::string &name, const std::string &def_value)
{
  if (! s_env_lock) {
    s_env_lock = new tl::Mutex ();
  }

  tl::MutexLocker env_locker (s_env_lock);

#ifdef _WIN32
  std::wstring wname = tl::to_wstring (name);
  wchar_t *env = _wgetenv (wname.c_str ());
  if (env) {
    return tl::to_string (std::wstring (env));
  } else {
    return def_value;
  }
#else
  char *env = getenv (name.c_str ());
  if (env) {
    return tl::system_to_string (env);
  } else {
    return def_value;
  }
#endif
}

void set_env (const std::string &name, const std::string &value)
{
  if (! s_env_lock) {
    s_env_lock = new tl::Mutex ();
  }

  tl::MutexLocker env_locker (s_env_lock);

  s_env_map [name] = name + "=" + value;
  const std::string &s = s_env_map [name];

#if defined(_WIN32)
  _putenv (const_cast<char *> (s.c_str ()));
#else
  putenv (const_cast<char *> (s.c_str ()));
#endif
}

void unset_env (const std::string &name)
{
  if (! s_env_lock) {
    s_env_lock = new tl::Mutex ();
  }

  tl::MutexLocker env_locker (s_env_lock);

#if defined(_WIN32)
  s_env_map [name] = name + "=";
#else
  s_env_map [name] = name;
#endif
  const std::string &s = s_env_map [name];

#if defined(_WIN32)
  _putenv (const_cast<char *> (s.c_str ()));
#else
  putenv (const_cast<char *> (s.c_str ()));
#endif
}

bool has_env (const std::string &name)
{
  if (! s_env_lock) {
    s_env_lock = new tl::Mutex ();
  }

  tl::MutexLocker env_locker (s_env_lock);

#ifdef _WIN32
  std::wstring wname = tl::to_wstring (name);
  wchar_t *env = _wgetenv (wname.c_str ());
  return env != 0;
#else
  char *env = getenv (name.c_str ());
  return env != 0;
#endif
}

bool app_flag (const std::string &name)
{
  std::string env_name = std::string ("KLAYOUT_") + tl::replaced (tl::to_upper_case (name), "-", "_");

  int v = 0;
  std::string vs = get_env (env_name);
  tl::Extractor ex (vs.c_str ());
  return ex.try_read (v) && v != 0;
}

} // namespace tl


