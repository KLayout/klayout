
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

#include "tlFileUtils.h"
#include "tlLog.h"
#include "tlInternational.h"

#include <cctype>

#include <sys/stat.h>
#include <unistd.h>

namespace tl
{

#if defined(_WIN32)
static bool is_drive (const std::string &part)
{
  return (part.size () == 2 && isletter (part[0]) && part[1] == ':');
}
#else
static bool is_drive (const std::string &)
{
  return false;
}
#endif

static std::string trimmed_part (const std::string &part)
{
  const char *cp = part.c_str ();

#if defined(_WIN32)
  while (*cp && *cp != '\\' && *cp != '/') {
    ++cp;
  }
#else
  while (*cp && *cp != '/') {
    ++cp;
  }
#endif

  return std::string (part, 0, cp - part.c_str ());
}

static bool is_part_with_separator (const std::string &part)
{
  const char *cp = part.c_str ();
#if defined(_WIN32)
  return (*cp == '\\' || *cp == '/');
#else
  return (*cp == '/');
#endif
}

std::vector<std::string> split_path (const std::string &p)
{
  std::vector<std::string> parts;

#if defined(_WIN32)

  const char *cp = p.c_str ();
  if (*cp && isletter (*cp) && cp[1] == ':') {

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
    parts.push_back (std::string (cp0, 0, cp - cp0));

  }

  while (*cp) {

    const char *cp0 = cp;
    bool any = false;
    while (*cp && (!any || (*cp != '\\' && *cp != '/'))) {
      if (*cp != '\\' && *cp != '/') {
        any = true;
      }
      ++cp;
    }

    parts.push_back (std::string (cp0, 0, cp - cp0));

  }

#else

  const char *cp = p.c_str ();
  while (*cp) {

    const char *cp0 = cp;
    bool any = false;
    while (*cp && (!any || *cp != '/')) {
      if (*cp != '/') {
        any = true;
      }
      //  backslash escape
      if (*cp == '\\' && cp[1]) {
        ++cp;
      }
      ++cp;
    }

    parts.push_back (std::string (cp0, 0, cp - cp0));

  }

#endif

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
  return (is_same_file (parent, tl::combine_path (tl::join (parts, ""), "")));
}

bool rm_dir_recursive (const std::string &path)
{
#if 0  // @@@
  QDir dir (path);

  QStringList entries = dir.entryList (QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    QFileInfo fi (dir.absoluteFilePath (*e));
    if (fi.isDir ()) {
      if (! rm_dir_recursive (fi.filePath ())) {
        return false;
      }
    } else if (fi.isFile ()) {
      if (! dir.remove (*e)) {
        tl::error << tr ("Unable to remove file: %1").arg (dir.absoluteFilePath (*e));
        return false;
      }
    }
  }

  QString name = dir.dirName ();
  if (dir.cdUp ()) {
    if (! dir.rmdir (name)) {
      tl::error << tr ("Unable to remove directory: %1").arg (dir.absoluteFilePath (name));
      return false;
    }
  }

#endif

  return true;
}

bool
cp_dir_recursive (const std::string &source, const std::string &target)
{
#if 0  // @@@
  QDir dir (source);
  QDir dir_target (target);

  QStringList entries = dir.entryList (QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    QFileInfo fi (dir.absoluteFilePath (*e));
    QFileInfo fi_target (dir_target.absoluteFilePath (*e));

    if (fi.isDir ()) {

      //  Copy subdirectory
      if (! fi_target.exists ()) {
        if (! dir_target.mkdir (*e)) {
          tl::error << tr ("Unable to create target directory: %1").arg (dir_target.absoluteFilePath (*e));
          return false;
        }
      } else if (! fi_target.isDir ()) {
        tl::error << tr ("Unable to create target directory (is a file already): %1").arg (dir_target.absoluteFilePath (*e));
        return false;
      }
      if (! cp_dir_recursive (fi.filePath (), fi_target.filePath ())) {
        return false;
      }

    //  TODO: leave symlinks symlinks? How to copy symlinks with Qt?
    } else if (fi.isFile ()) {

      QFile file (fi.filePath ());
      QFile file_target (fi_target.filePath ());

      if (! file.open (QIODevice::ReadOnly)) {
        tl::error << tr ("Unable to open source file for reading: %1").arg (fi.filePath ());
        return false;
      }
      if (! file_target.open (QIODevice::WriteOnly)) {
        tl::error << tr ("Unable to open target file for writing: %1").arg (fi_target.filePath ());
        return false;
      }

      size_t chunk_size = 64 * 1024;

      while (! file.atEnd ()) {
        QByteArray data = file.read (chunk_size);
        file_target.write (data);
      }

      file.close ();
      file_target.close ();

    }

  }

#endif

  return true;
}

bool mkpath (const std::string &path)
{
  //  @@@
  return false;
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
  char *cwd;
#if defined(_WIN32)
  cwd = _getcwd (NULL, 0);
#else
  cwd = getcwd (NULL, 0);
#endif

  if (cwd == NULL) {
    return std::string ();
  } else {
    std::string cwds (cwd);
    free (cwd);
    return cwds;
  }
}


static std::pair<std::string, bool> absolute_path_of_existing (const std::string &s)
{
  char *fp;
#if defined (_WIN32)
  fp = _fullpath (NULL, s.c_str (), 0);
#else
  fp = realpath (s.c_str (), NULL);
#endif

  if (fp == NULL) {
    return std::make_pair (std::string (), false);
  } else {
    std::string fps (fp);
    free (fp);
    return std::make_pair (fps, true);
  }
}

std::string absolute_file_path (const std::string &s)
{
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

std::string dirname (const std::string &s)
{
  std::vector<std::string> parts = split_path (s);
  if (parts.size () > 0) {
    parts.pop_back ();
  }

  return tl::join (parts, "");
}

std::string filename (const std::string &s)
{
  std::vector<std::string> parts = split_path (s);
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

std::string extension (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    fnp.erase (fnp.begin ());
  }
  return tl::join (fnp, ".");
}

bool file_exists (const std::string &p)
{
  struct stat st;
  return stat (p.c_str (), &st) == 0;
}

bool is_dir (const std::string &p)
{
  struct stat st;
  if (stat (p.c_str (), &st) != 0) {
    return false;
  } else {
    return !S_ISREG (st.st_mode);
  }
}

std::string combine_path (const std::string &p1, const std::string &p2)
{
#if defined(_WIN32)
  return p1 + "\\" + p2;
#else
  return p1 + "/" + p2;
#endif
}

bool is_same_file (const std::string &a, const std::string &b)
{

#if defined(_WIN32)

  HANDLE h1 = ::CreateFile (a.c_str (), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  HANDLE h2 = ::CreateFile (b.c_str (), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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

  struct stat sta, stb;
  if (stat (a.c_str (), &sta) != 0) {
    return false;
  }
  if (stat (b.c_str (), &stb) != 0) {
    return false;
  }

  return sta.st_dev == stb.st_dev && sta.st_ino == stb.st_ino;

#endif
}

}
