
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

#include "tlFileUtils.h"
#include "tlStream.h"
#include "tlLog.h"
#include "tlInternational.h"
#include "tlEnv.h"

#include <cctype>
#include <fstream>

// Use this define to print debug output
// #define FILE_UTILS_VERBOSE

#if defined(_MSC_VER)

#  include <sys/types.h>
#  include <sys/stat.h>
#  include <io.h>
#  include <Windows.h>

#elif defined(_WIN32)

#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <dir.h>
#  include <Windows.h>

#elif defined(__APPLE__)

#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <libproc.h>
#  include <dlfcn.h>
#  include <pwd.h>

#else

#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <dlfcn.h>
#  include <pwd.h>

#endif

#if defined(__FreeBSD__)

#include <sys/types.h>
#include <sys/sysctl.h>

#endif

namespace tl
{

enum { OS_Auto, OS_Windows, OS_Linux } s_mode = OS_Auto;

static bool is_win ()
{
  if (s_mode == OS_Windows) {
    return true;
  } else if (s_mode == OS_Linux) {
    return false;
  } else {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
  }
}

//  Secret mode switchers for testing
TL_PUBLIC void file_utils_force_windows () { s_mode = OS_Windows; }
TL_PUBLIC void file_utils_force_linux () { s_mode = OS_Linux; }
TL_PUBLIC void file_utils_force_reset () { s_mode = OS_Auto; }

const char *line_separator ()
{
  return is_win () ? "\r\n" : "\n";
}

static bool is_drive (const std::string &part)
{
  return is_win () && (part.size () == 2 && isalpha (part[0]) && part[1] == ':');
}

static std::string normalized_part (const std::string &part)
{
  if (! is_win ()) {
    return part;
  }

  std::string p;
  p.reserve (part.size ());
  const char *cp = part.c_str ();
  while (*cp == '\\' || *cp == '/') {
    p += '\\';
    ++cp;
  }
  p += cp;
  return p;
}

static std::string trimmed_part (const std::string &part)
{
  const char *cp = part.c_str ();

  if (is_win ()) {
    while (*cp == '\\' || *cp == '/') {
      ++cp;
    }
  } else {
    while (*cp == '/') {
      ++cp;
    }
  }

  return std::string (cp);
}

static bool is_part_with_separator (const std::string &part)
{
  const char *cp = part.c_str ();
  if (is_win ()) {
    return (*cp == '\\' || *cp == '/');
  } else {
    return (*cp == '/');
  }
}

std::vector<std::string> split_path (const std::string &p, bool keep_last)
{
  std::vector<std::string> parts;

  bool first = true;

  if (is_win ()) {

    const char *cp = p.c_str ();
    if (*cp && isalpha (*cp) && cp[1] == ':') {

      //  drive name
      parts.push_back (std::string ());
      parts.back () += toupper (*cp);
      parts.back () += ":";

      cp += 2;

    } else if ((*cp == '\\' && cp[1] == '\\') || (*cp == '/' && cp[1] == '/')) {

      //  UNC server name
      const char *cp0 = cp;
      cp += 2;
      while (*cp && *cp != '\\' && *cp != '/') {
        ++cp;
      }
      parts.push_back (tl::normalized_part (std::string (cp0, 0, cp - cp0)));

    } else if ((*cp == '\\' || *cp == '/') && cp[1] && isalpha (cp[1]) && cp[2] == ':') {

      //  drive name in the form "/c:" or "\c:"
      parts.push_back (std::string ());
      parts.back () += toupper (cp[1]);
      parts.back () += ":";

      cp += 3;

    }

    while (*cp) {

      const char *cp0 = cp;
      bool any = false;
      while (*cp && (!any || (*cp != '\\' && *cp != '/'))) {
        if (*cp != '\\' && *cp != '/') {
          any = true;
        } else {
          cp0 = cp;
        }
        ++cp;
      }

      if (any || first || keep_last) {
        first = false;
        parts.push_back (tl::normalized_part (std::string (cp0, 0, cp - cp0)));
      }

    }

  } else {

    const char *cp = p.c_str ();
    while (*cp) {

      const char *cp0 = cp;
      bool any = false;
      while (*cp && (!any || *cp != '/')) {
        if (*cp != '/') {
          any = true;
        } else {
          cp0 = cp;
        }
        //  backslash escape
        if (*cp == '\\' && cp[1]) {
          ++cp;
        }
        ++cp;
      }

      if (any || first || keep_last) {
        first = false;
        parts.push_back (std::string (cp0, 0, cp - cp0));
      }

    }

  }

  return parts;
}

static std::vector<std::string> split_filename (const std::string &fn)
{
  std::vector<std::string> parts;

  const char *cp = fn.c_str ();

  while (*cp) {

    const char *cp0 = cp;
    ++cp;
    while (*cp && *cp != '.') {
      //  backslash escaping (ineffective on Windows because that is a path separator)
      if (*cp == '\\' && cp[1]) {
        ++cp;
      }
      ++cp;
    }

    parts.push_back (std::string (cp0, 0, cp - cp0));
    if (*cp) {
      ++cp;
    }

  }

  return parts;
}

std::string normalize_path (const std::string &s)
{
  return tl::join (tl::split_path (s), "");
}

std::string combine_path (const std::string &p1, const std::string &p2, bool always_join)
{
  if (! always_join && p2.empty ()) {
    return p1;
  } else if (is_win ()) {
    return p1 + "\\" + p2;
  } else {
    return p1 + "/" + p2;
  }
}

std::string dirname (const std::string &s)
{
  std::vector<std::string> parts = split_path (s, true /*keep last part*/);
  if (parts.size () > 0) {
    parts.pop_back ();
  }

  if (parts.empty ()) {
    return is_part_with_separator (s) ? "" : ".";
  } else {
    return tl::join (parts, "");
  }
}

std::string filename (const std::string &s)
{
  std::vector<std::string> parts = split_path (s, true /*keep last part*/);
  if (parts.size () > 0) {
    return trimmed_part (parts.back ());
  } else {
    return std::string ();
  }
}

std::string basename (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    return fnp.front ();
  } else {
    return std::string ();
  }
}

std::string complete_basename (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    fnp.pop_back ();
    return tl::join (fnp, ".");
  } else {
    return std::string ();
  }
}

std::string extension (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    fnp.erase (fnp.begin ());
  }
  return tl::join (fnp, ".");
}

std::string extension_last (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 1) {
    return fnp.back ();
  } else {
    return std::string ();
  }
}

bool
is_parent_path (const std::string &parent, const std::string &path)
{
  if (! file_exists (parent)) {
    //  If the parent path does not exist, we always return false. This cannot be a parent.
    return false;
  }

  std::vector<std::string> parts = split_path (absolute_file_path (path));

  while (! parts.empty () && ! (parts.size () == 1 && is_drive (parts[0]))) {
    if (is_same_file (parent, tl::join (parts, ""))) {
      return true;
    } else {
      parts.pop_back ();
    }
  }

  //  We did not find a match - now maybe the parent is root
  return (is_same_file (parent, tl::combine_path (tl::join (parts, ""), "", true /*always add slash*/)));
}

std::vector<std::string> dir_entries (const std::string &s, bool with_files, bool with_dirs, bool without_dotfiles)
{
  std::vector<std::string> ee;

#if defined(_WIN32)

  struct _wfinddata_t fileinfo;

  intptr_t h = _wfindfirst (tl::to_wstring (s + "\\*").c_str (), &fileinfo);
  if (h != -1) {

    do {

      std::string e = tl::to_string (std::wstring (fileinfo.name));
      if (e.empty () || e == "." || e == "..") {
        continue;
      }

      bool is_dir = ((fileinfo.attrib & _A_SUBDIR) != 0);
      if ((e[0] != '.' || !without_dotfiles) && ((is_dir && with_dirs) || (!is_dir && with_files))) {
        ee.push_back (e);
      }

    } while (_wfindnext (h, &fileinfo) == 0);

  }

  _findclose (h);

#else

  DIR *h = opendir (tl::to_local (s).c_str ());
  if (h) {

    struct dirent *d;
    while ((d = readdir (h)) != NULL) {

      std::string e = tl::to_string_from_local (d->d_name);
      if (e.empty () || e == "." || e == "..") {
        continue;
      }

      bool is_dir = (d->d_type == DT_DIR);
      if ((e[0] != '.' || !without_dotfiles) && ((is_dir && with_dirs) || (!is_dir && with_files))) {
        ee.push_back (e);
      }

    }

    closedir (h);

  }

#endif

  return ee;
}

bool mkdir (const std::string &path)
{
#if defined(_WIN32)
  return _wmkdir (tl::to_wstring (path).c_str ()) == 0;
#else
  return ::mkdir (tl::to_local (path).c_str (), 0777) == 0;
#endif
}

bool mkpath (const std::string &p)
{
  std::vector<std::string> parts = split_path (absolute_file_path (p));

  size_t i = 0;
  std::string front;
  if (! parts.empty () && is_drive (parts.front ())) {
    front = parts.front ();
    ++i;
  }

  while (i < parts.size ()) {
    front += parts[i++];
    if (! file_exists (front)) {
      if (! mkdir (front)) {
#if defined(FILE_UTILS_VERBOSE)
        tl::error << tr ("Unable to create directory: ") << front;
#endif
        return false;
      }
    }
  }

  return true;
}

bool rename_file (const std::string &path, const std::string &new_name)
{
  //  resolve relative names in new_name
  std::string new_path = new_name;
  if (! tl::is_absolute (new_path)) {
    new_path = tl::combine_path (tl::dirname (path), new_name);
  }

#if defined(_WIN32)
  return _wrename (tl::to_wstring (path).c_str (), tl::to_wstring (new_path).c_str ()) == 0;
#else
  return rename (tl::to_local (path).c_str (), tl::to_local (new_path).c_str ()) == 0;
#endif
}

bool rm_file (const std::string &path)
{
#if defined(_WIN32)
  std::wstring wpath = tl::to_wstring (path);
  _wchmod (wpath.c_str (), _S_IREAD | _S_IWRITE);
  return _wunlink (wpath.c_str ()) == 0;
#else
  return unlink (tl::to_local (path).c_str ()) == 0;
#endif
}

bool rm_dir (const std::string &path)
{
#if defined(_WIN32)
  return _wrmdir (tl::to_wstring (path).c_str ()) == 0;
#else
  return rmdir (tl::to_local (path).c_str ()) == 0;
#endif
}

bool rm_dir_recursive (const std::string &p)
{
  std::vector<std::string> entries;
  std::string path = tl::absolute_file_path (p);

  if (! tl::file_exists (path)) {
    //  already gone.
    return true;
  }

  entries = dir_entries (path, false /*without_files*/, true /*with_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    if (! rm_dir_recursive (tl::combine_path (path, *e))) {
      return false;
    }
  }

  entries = dir_entries (path, true /*with_files*/, false /*without_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    std::string tc = tl::combine_path (path, *e);
    if (! rm_file (tc)) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to remove file: ") << tc;
#endif
      return false;
    }
  }

  if (! rm_dir (path)) {
#if defined(FILE_UTILS_VERBOSE)
    tl::error << tr ("Unable to remove directory: ") << path;
#endif
    return false;
  }

  return true;
}

bool
cp_dir_recursive (const std::string &source, const std::string &target)
{
  std::vector<std::string> entries;
  std::string path = tl::absolute_file_path (source);
  std::string path_to = tl::absolute_file_path (target);

  entries = dir_entries (path, false /*without_files*/, true /*with_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    std::string tc = tl::combine_path (path_to, *e);
    if (! mkpath (tc)) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to create target directory: ") << tc;
#endif
      return false;
    }
    if (! cp_dir_recursive (tl::combine_path (path, *e), tc)) {
      return false;
    }
  }

  entries = dir_entries (path, true /*with_files*/, false /*without_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    //  TODO: leave symlinks symlinks? How to copy symlinks with Qt?

    //  copy the files
    try {

      tl::OutputFile os_file (tl::combine_path (path_to, *e));
      tl::OutputStream os (os_file);
      tl::InputFile is_file (tl::combine_path (path, *e));
      tl::InputStream is (is_file);
      is.copy_to (os);

    } catch (tl::Exception &ex) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to copy file ") << tl::combine_path (path_to, *e) << tr (" to ") << tl::combine_path (path, *e)
                << tr ("(Error ") << ex.msg () << ")";
#endif
      return false;
    }

  }

  return true;
}

bool
mv_dir_recursive (const std::string &source, const std::string &target)
{
  std::vector<std::string> entries;
  std::string path = tl::absolute_file_path (source);
  std::string path_to = tl::absolute_file_path (target);

  bool error = false;

  entries = dir_entries (path, false /*without_files*/, true /*with_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    std::string tc = tl::combine_path (path_to, *e);
    if (! mkpath (tc)) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to create target directory: ") << tc;
#endif
      error = true;
    } else if (! mv_dir_recursive (tl::combine_path (path, *e), tc)) {
      error = true;
    }
  }

  entries = dir_entries (path, true /*with_files*/, false /*without_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    if (! tl::rename_file (tl::combine_path (path, *e), tl::combine_path (path_to, *e))) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to move file from ") << tl::combine_path (path, *e) << tr (" to ") << tl::combine_path (path_to, *e);
#endif
      error = true;
    }
  }

  if (! tl::rm_dir (path)) {
#if defined(FILE_UTILS_VERBOSE)
      tl::error << tr ("Unable to remove folder ") << path;
#endif
    error = true;
  }

  return ! error;
}

std::string absolute_path (const std::string &s)
{
  std::vector<std::string> parts = split_path (absolute_file_path (s));
  if (parts.size () > 0) {
    parts.pop_back ();
  }

  return tl::join (parts, "");
}

std::string current_dir ()
{
#if defined(_WIN32)

  wchar_t *cwd = _wgetcwd (NULL, 0);
  if (cwd == NULL) {
    return std::string ();
  } else {
    std::string cwds (tl::to_string (std::wstring (cwd)));
    free (cwd);
    return cwds;
  }

#else

  char *cwd = getcwd (NULL, 0);
  if (cwd == NULL) {
    return std::string ();
  } else {
    std::string cwds (tl::to_string_from_local (cwd));
    free (cwd);
    return cwds;
  }

#endif
}

bool chdir (const std::string &path)
{
#if defined(_WIN32)
  return _wchdir (tl::to_wstring (path).c_str ()) == 0;
#else
  return ::chdir (tl::to_local (path).c_str ()) == 0;
#endif
}

static std::pair<std::string, bool> absolute_path_of_existing (const std::string &s)
{
#if defined(_WIN32)

  wchar_t *fp = _wfullpath (NULL, tl::to_wstring (s).c_str (), 0);
  if (fp == NULL) {
    return std::make_pair (std::string (), false);
  } else {
    std::string fps (tl::to_string (std::wstring (fp)));
    free (fp);
    return std::make_pair (fps, true);
  }

#else

  char *fp;
  fp = realpath (tl::to_local (s).c_str (), NULL);
  if (fp == NULL) {
    return std::make_pair (std::string (), false);
  } else {
    std::string fps (tl::to_string_from_local (fp));
    free (fp);
    return std::make_pair (fps, true);
  }

#endif
}

bool is_absolute (const std::string &s)
{
  //  ~ paths are always absolute, because the home directory is
  if (s.size () > 0 && s[0] == '~') {
    return true;
  }

  std::vector<std::string> parts = split_path (s);
  if (parts.size () > 1 && is_drive (parts [0])) {
    return is_part_with_separator (parts [1]);
  } else if (! parts.empty ()) {
    return is_part_with_separator (parts.front ());
  } else {
    return false;
  }
}

std::string absolute_file_path (const std::string &s)
{
  //  ~ paths are always absolute, because the home directory is
  if (s.size () > 0 && s[0] == '~') {
    return get_home_path () + std::string (s, 1);
  }

  std::vector<std::string> parts = split_path (s);
  if (parts.empty ()) {
    return current_dir ();
  }

  std::pair<std::string, bool> known_part;
  std::vector<std::string> unknown_parts;

  while (! parts.empty () && ! (parts.size () == 1 && is_drive (parts[0]))) {
    known_part = absolute_path_of_existing (tl::join (parts, ""));
    if (! known_part.second) {
      unknown_parts.push_back (parts.back ());
      parts.pop_back ();
    } else {
      break;
    }
  }

  std::reverse (unknown_parts.begin (), unknown_parts.end ());

  if (! known_part.second) {

    //  the top-level component is unknown. This can mean:
    //  1.) the path is already absolute, but the top-level entry does not exist
    //  2.) the path is relative, but the entry does not exist

    tl_assert (! unknown_parts.empty ());
    if (is_part_with_separator (unknown_parts.front ())) {

      //  case 1: return the full path as absolute
      return s;

    } else if (parts.size () == 1 && is_drive (parts[0])) {

      //  case 2 (for Windows): try to root on drive's working dir
      known_part = absolute_path_of_existing (parts[0]);
      if (! known_part.second) {
        //  drive is not known ... return the original path as fallback
        return s;
      } else {
        return combine_path (known_part.first, tl::join (unknown_parts, ""));
      }

    } else {

      //  case 2 (for *nix): try to root on current working dir
      return combine_path (current_dir (), tl::join (unknown_parts, ""));

    }

  } else {
    return combine_path (known_part.first, tl::join (unknown_parts, ""));
  }
}

#if defined(_WIN32)

typedef struct _stat stat_struct;

static int stat_func (const std::string &s, stat_struct &st)
{
  return _wstat (tl::to_wstring (s).c_str (), &st);
}

#else

typedef struct stat stat_struct;

static int stat_func (const std::string &s, stat_struct &st)
{
  return stat (tl::to_local (s).c_str (), &st);
}

#endif

bool file_exists (const std::string &p)
{
  stat_struct st;
  return stat_func (p, st) == 0;
}

bool is_writable (const std::string &p)
{
  stat_struct st;
#if defined(_MSC_VER)
  return stat_func (p, st) == 0 && (st.st_mode & _S_IWRITE) != 0;
#else
  return stat_func (p, st) == 0 && (st.st_mode & S_IWUSR) != 0;
#endif
}

bool is_readable (const std::string &p)
{
  stat_struct st;
#if defined(_MSC_VER)
  return stat_func (p, st) == 0 && (st.st_mode & _S_IREAD) != 0;
#else
  return stat_func (p, st) == 0 && (st.st_mode & S_IRUSR) != 0;
#endif
}

bool is_dir (const std::string &p)
{
  stat_struct st;
  if (stat_func (p, st) != 0) {
    return false;
  } else {
#if defined(_MSC_VER)
    return !(st.st_mode & _S_IFREG);
#else
    return !S_ISREG (st.st_mode);
#endif
  }
}

std::string relative_path (const std::string &base, const std::string &p)
{
  std::vector<std::string> rem;
  std::vector<std::string> parts = split_path (p);

  while (! parts.empty ()) {

    if (is_same_file (base, tl::join (parts, ""))) {

      //  combine the remaining path
      std::reverse (rem.begin (), rem.end ());
      if (! rem.empty ()) {
        rem[0] = tl::trimmed_part (rem.front ());
      }
      return tl::join (rem, "");

    }

    rem.push_back (parts.back ());
    parts.pop_back ();

  }

  return p;
}

bool is_same_file (const std::string &a, const std::string &b)
{
  if (tl::normalize_path (a) == tl::normalize_path (b)) {
    return true;
  }

#if defined(_WIN32)

  HANDLE h1 = ::CreateFileW (tl::to_wstring (a).c_str (), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  HANDLE h2 = ::CreateFileW (tl::to_wstring (b).c_str (), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  bool result = false;

  if (h1 != INVALID_HANDLE_VALUE && h2 != INVALID_HANDLE_VALUE) {
    BY_HANDLE_FILE_INFORMATION fi1, fi2;
    if (::GetFileInformationByHandle(h1, &fi1) && ::GetFileInformationByHandle(h2, &fi2)) {
      result = fi1.dwVolumeSerialNumber == fi2.dwVolumeSerialNumber &&
               fi1.nFileIndexHigh == fi2.nFileIndexHigh &&
               fi1.nFileIndexLow == fi2.nFileIndexLow;
   }
  }

  if (h1 != INVALID_HANDLE_VALUE) {
    ::CloseHandle(h1);
  }

  if (h2 != INVALID_HANDLE_VALUE) {
    ::CloseHandle(h2);
  }

  return result;

#else

  stat_struct sta, stb;
  if (stat_func (a, sta) != 0 || stat_func (b, stb) != 0) {
    return false;
  }

  return sta.st_dev == stb.st_dev && sta.st_ino == stb.st_ino;

#endif
}

std::string
get_home_path ()
{
#if !defined(_WIN32)
  if (tl::has_env ("HOME")) {
    return tl::get_env ("HOME");
  } else {
    struct passwd *pwd = getpwuid (getuid ());
    if (pwd) {
      return std::string (pwd->pw_dir);
    }
  }
  tl::warn << tl::to_string (tr ("Unable to get home directory (set HOME environment variable)"));
#else
  if (tl::has_env ("HOMEDRIVE") && tl::has_env ("HOMEPATH")) {
    return tl::get_env ("HOMEDRIVE") + tl::get_env ("HOMEPATH");
  } else if (tl::has_env ("HOMESHARE") && tl::has_env ("HOMEPATH")) {
    return tl::get_env ("HOMESHARE") + tl::get_env ("HOMEPATH");
  } else if (tl::has_env ("USERPROFILE")) {
    return tl::get_env ("USERPROFILE");
  }
  tl::warn << tl::to_string (tr ("Unable to get home directory (no HOMEDRIVE/HOMEPATH, HOMESHARE/HOMEPATH or USERPROFILE environment variables)"));
#endif
  return std::string (".");
}

static std::string
get_app_path_internal ()
{
#if defined(_WIN32)

  wchar_t buffer[MAX_PATH];
  int len;
  if ((len = GetModuleFileNameW (NULL, buffer, MAX_PATH)) > 0) {
    return tl::to_string (std::wstring (buffer));
  }

#elif __APPLE__

  char buffer[PROC_PIDPATHINFO_MAXSIZE];
  int ret = proc_pidpath (getpid (), buffer, sizeof (buffer));
  if (ret > 0) {
    //  TODO: does this correctly translate paths? (MacOS uses UTF-8 encoding with D-like normalization)
    return buffer;
  }

#elif defined (__FreeBSD__)

  char path[PATH_MAX];
  size_t len = PATH_MAX;
  const int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  if (sysctl(&mib[0], 4, &path, &len, NULL, 0) == 0) {
    return path;
  }
  return "";

#else

  std::string pf = tl::sprintf ("/proc/%d/exe", getpid ());
  if (tl::file_exists (pf)) {
    return pf;
  }

#endif
  tl_assert (false);
}

std::string
get_inst_path ()
{
  static std::string s_inst_path;
  if (s_inst_path.empty ()) {
    s_inst_path = tl::absolute_path (get_app_path_internal ());
  }
  return s_inst_path;
}

std::string
get_app_path ()
{
  static std::string s_app_path;
  if (s_app_path.empty ()) {
    s_app_path = get_app_path_internal ();
  }
  return s_app_path;
}

std::string
get_module_path (void *addr)
{
#if defined(_WIN32)

  HMODULE h_module = NULL;
  if (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR) addr, &h_module)) {

    wchar_t buffer[MAX_PATH];
    int len;
    if ((len = GetModuleFileNameW (h_module, buffer, MAX_PATH)) > 0) {
      return tl::absolute_file_path (tl::to_string (std::wstring (buffer, 0, len)));
    }

  }

  //  no way to get module file path
  return std::string ();

#else

  Dl_info info = { };
  if (dladdr (addr, &info)) {
    return tl::absolute_file_path (tl::to_string_from_local (info.dli_fname));
  } else {
    tl::warn << tl::to_string (tr ("Unable to get path of db library (as basis for loading db_plugins)"));
    return std::string ();
  }

#endif
}

std::string
tmpfile (const std::string &domain)
{
  std::string tmp = tl::get_env ("TMPDIR");
  if (tmp.empty ()) {
    tmp = tl::get_env ("TMP");
  }
  if (tmp.empty ()) {
#if defined(_WIN32)
    throw tl::Exception (tl::to_string (tr ("TMP and TMPDIR not set - cannot create temporary file")));
#else
    tmp = "/tmp";
#endif
  }

  std::string templ = tl::combine_path (tmp, domain + "XXXXXX");
  char *tmpstr = strdup (templ.c_str ());

#if defined(_WIN32)
  if (_mktemp_s (tmpstr, templ.size () + 1) != 0) {
    free (tmpstr);
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder name in %s")), tmp);
  }

  //  for compatibility with Linux, create the file as an empty one
  std::ofstream os (tmpstr);
  if (os.bad ()) {
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder in %s")), tmp);
  }
  os.close ();
#else
  int fd = mkstemp (tmpstr);
  if (fd < 0) {
    free (tmpstr);
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder in %s")), tmp);
  }
  close (fd);
#endif

  std::string res = tmpstr;
  free (tmpstr);
  return res;
}

TemporaryFile::TemporaryFile (const std::string &domain)
{
  m_path = tmpfile (domain);
}

TemporaryFile::~TemporaryFile ()
{
  tl::rm_file (m_path);
}

std::string
tmpdir (const std::string &domain)
{
  std::string tmp = tl::get_env ("TMPDIR");
  if (tmp.empty ()) {
    tmp = tl::get_env ("TMP");
  }
  if (tmp.empty ()) {
#if defined(_WIN32)
    throw tl::Exception (tl::to_string (tr ("TMP and TMPDIR not set - cannot create temporary file")));
#else
    tmp = "/tmp";
#endif
  }

  std::string templ = tl::combine_path (tmp, domain + "XXXXXX");
  char *tmpstr = strdup (templ.c_str ());

#if defined(_WIN32)
  if (_mktemp_s (tmpstr, templ.size () + 1) != 0) {
    free (tmpstr);
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder name in %s")), tmp);
  }
  if (! tl::mkdir (tmpstr)) {
    free (tmpstr);
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder in %s")), tmp);
  }
#else
  if (mkdtemp (tmpstr) == NULL) {
    free (tmpstr);
    throw tl::Exception (tl::to_string (tr ("Unable to create temporary folder in %s")), tmp);
  }
#endif

  std::string res = tmpstr;
  free (tmpstr);
  return res;
}

TemporaryDirectory::TemporaryDirectory (const std::string &domain)
{
  m_path = tmpdir (domain);
}

TemporaryDirectory::~TemporaryDirectory ()
{
  tl::rm_dir_recursive (m_path);
}


}
