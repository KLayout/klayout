
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

#if defined(HAVE_QT)

#include <QFileDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>
#if QT_VERSION>=0x050000
#  include <QStandardPaths>
#endif

#include "layFileDialog.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlFileUtils.h"

namespace lay
{

FileDialog::FileDialog (QWidget *parent, const std::string &title, const std::string &filters, const std::string &def_suffix)
  : QObject (parent)
{
#ifdef _WIN32
# if QT_VERSION>=0x050000
  m_dir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
# else
  //  On Windows the current directory does not have any meaning - it's usually the installation 
  //  location.
  m_dir = QDir (QDesktopServices::storageLocation (QDesktopServices::DocumentsLocation));
# endif
#else
  m_dir = QDir::current ();
#endif
  m_title = tl::to_qstring (title);
  m_filters = tl::to_qstring (filters);
  m_def_suffix = tl::to_qstring (def_suffix);
}

FileDialog::~FileDialog ()
{
  //  .. nothing yet ..
}

int
FileDialog::find_selected_filter (const QString &fs, const QString &selected_filter)
{
  QStringList filters = fs.split (tl::to_qstring (";;"));

  for (auto f = filters.begin (); f != filters.end (); ++f) {
    if (*f == selected_filter) {
      return int (f - filters.begin ());
    }
  }

  return -1;
}

std::string
FileDialog::add_default_extension (const std::string &path, const QString &selected_filter)
{
  if (tl::extension (path).empty ()) {

    std::string sf = tl::to_string (selected_filter);

    auto star = sf.find ("*.");
    if (star != std::string::npos) {

      tl::Extractor ex (sf.c_str () + star + 2);
      std::string ext;

      if (ex.try_read_word (ext)) {
        return path + "." + ext;
      }

    }

  }

  return path;
}

int
FileDialog::selected_filter () const
{
  return find_selected_filter (m_filters, m_sel_filter);
}

bool 
FileDialog::get_open (std::string &fp, const std::string &title) 
{
  //  Use the standard (system) dialogs:

  QString file_name;
  if (! fp.empty ()) {
    QFileInfo fi (tl::to_qstring (fp));
    m_dir = fi.absoluteDir ();
    file_name = tl::to_qstring (fp);
  } else {
    file_name = m_dir.absolutePath ();
  }

  QString f = QFileDialog::getOpenFileName (QApplication::activeWindow (), (title.empty () ? m_title : tl::to_qstring (title)), file_name, m_filters, &m_sel_filter);

  if (! f.isEmpty ()) {
    fp = tl::to_string (f);
    QFileInfo fi (f);
    m_dir = fi.absoluteDir ();
    return true;
  } else {
    return false;
  }
}

bool 
FileDialog::get_open (std::vector<std::string> &fp, const std::string &dir, const std::string &title) 
{
  //  Use the standard (system) dialogs:

  if (! dir.empty ()) {
    QDir fi (tl::to_qstring (dir));
    m_dir = fi.absolutePath ();
  }

  QStringList files = QFileDialog::getOpenFileNames (QApplication::activeWindow (), (title.empty () ? m_title : tl::to_qstring (title)), m_dir.absolutePath (), m_filters, &m_sel_filter);

  if (! files.isEmpty ()) {
    fp.clear ();
    for (QStringList::iterator f = files.begin (); f != files.end (); ++f) {
      fp.push_back (tl::to_string (*f));
      QFileInfo fi (*f);
      m_dir = fi.absoluteDir ();
    }
    return true;
  } else {
    return false;
  }
}

bool 
FileDialog::get_save (std::string &fp, const std::string &title) 
{
  //  Use the standard (system) dialogs:

  QString file_name;
  if (! fp.empty ()) {
    QFileInfo fi (tl::to_qstring (fp));
    m_dir = fi.absoluteDir ();
    file_name = tl::to_qstring (fp);
  } else {
    file_name = m_dir.absolutePath ();
  }

  QString f = QFileDialog::getSaveFileName (QApplication::activeWindow (), (title.empty () ? m_title : tl::to_qstring (title)), file_name, m_filters, &m_sel_filter);

  if (! f.isEmpty ()) {

    fp = add_default_extension (tl::to_string (f), m_sel_filter);

    QFileInfo fi (f);
    m_dir = fi.absoluteDir ();
    return true;

  } else {
    return false;
  }
}

} // namespace lay

#endif
