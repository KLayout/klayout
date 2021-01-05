
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include <QFileDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>

#include "layFileDialog.h"
#include "tlInternational.h"

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

//  not used if standard dialogs are used (disabled to avoid compiler warning):
#if 0
static void
split_filters (const QString &filters, QStringList &slist)
{
  QString s;
  for (const char *cp = filters.ascii (); *cp; ++cp) {
    if (cp[0] == ';' && cp[1] == ';') {
      slist << s;
      ++cp;
      s = "";
    } else {
      s += *cp;
    }
  }

  if (s != "") {
    slist << s;
  }
}
#endif

bool 
FileDialog::get_open (std::string &fp, const std::string &title) 
{
#if 1
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
#else
  QString file_name;
  if (! fp.empty ()) {
    QFileInfo fi (fp.c_str ());
    m_dir = fi.absoluteDir ();
    file_name = fi.fileName ();
  }

  QFileDialog fdia (QApplication::activeWindow ());

  fdia.setDirectory (m_dir);
  if (m_def_suffix != "") {
    fdia.setDefaultSuffix (m_def_suffix);
  }

  QStringList types;
  split_filters (m_filters, types);
  fdia.setFilters (types);

  fdia.setReadOnly (true);
  fdia.setViewMode (QFileDialog::Detail);
  fdia.setFileMode (QFileDialog::ExistingFile);
  fdia.setAcceptMode (QFileDialog::AcceptOpen);
  fdia.setConfirmOverwrite (true);
  fdia.setCaption (QString (tl::to_string (QObject::tr ("Open ")).c_str ()) + (title.empty () ? m_title : tl::to_qstring (title)));
  if (! file_name.isEmpty ()) {
    fdia.selectFile (file_name);
  }

  QStringList files;
  if (fdia.exec ()) {
    files = fdia.selectedFiles();
    if (files.size () > 0) {
      fp = tl::to_string (files [0]);
      QFileInfo fi (files [0]);
      m_dir = fi.absoluteDir ();
      return true;
    }
  }

  return false;
#endif
}

bool 
FileDialog::get_open (std::vector<std::string> &fp, const std::string &dir, const std::string &title) 
{
#if 1
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
#else
  if (! dir.empty ()) {
    QDir fi (dir.c_str ());
    m_dir = fi.absolutePath ();
  }

  QStringList file_names;
  for (std::vector<std::string>::const_iterator f = fp.begin (); f != fp.end (); ++f) {
    QFileInfo fi (f->c_str ());
    m_dir = fi.absoluteDir ();
    file_names << QString (f->c_str ());
  }

  QFileDialog fdia (QApplication::activeWindow ());

  fdia.setDirectory (m_dir);
  if (m_def_suffix != "") {
    fdia.setDefaultSuffix (m_def_suffix);
  }

  QStringList types;
  split_filters (m_filters, types);
  fdia.setFilters (types);

  fdia.setReadOnly (true);
  fdia.setViewMode (QFileDialog::Detail);
  fdia.setFileMode (QFileDialog::ExistingFiles);
  fdia.setAcceptMode (QFileDialog::AcceptOpen);
  fdia.setConfirmOverwrite (true);
  fdia.setCaption (QString (tl::to_string (QObject::tr ("Open ")).c_str ()) + (title.empty () ? m_title : tl::to_qstring (title)));

  for (QStringList::iterator f = file_names.begin (); f != file_names.end (); ++f) {
    fdia.selectFile (*f);
  }

  QStringList files;
  if (fdia.exec ()) {

    files = fdia.selectedFiles();
    if (! files.isEmpty ()) {

      fp.clear ();
      for (QStringList::iterator f = files.begin (); f != files.end (); ++f) {
        fp.push_back (tl::to_string (*f));
        QFileInfo fi (*f);
        m_dir = fi.absoluteDir ();
      }

      return true;

    }

  }

  return false;
#endif
}

bool 
FileDialog::get_save (std::string &fp, const std::string &title) 
{
#if 1
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
    fp = tl::to_string (f);
    QFileInfo fi (f);
    m_dir = fi.absoluteDir ();
    return true;
  } else {
    return false;
  }
#else
  QString file_name;
  if (! fp.empty ()) {
    QFileInfo fi (fp.c_str ());
    m_dir = fi.absoluteDir ();
    file_name = fi.fileName ();
  }

  QFileDialog fdia (QApplication::activeWindow ());

  fdia.setDirectory (m_dir);
  if (m_def_suffix != "") {
    fdia.setDefaultSuffix (m_def_suffix);
  }

  QStringList types;
  split_filters (m_filters, types);
  fdia.setFilters (types);

  fdia.setReadOnly (false);
  fdia.setViewMode (QFileDialog::Detail);
  fdia.setFileMode (QFileDialog::AnyFile);
  fdia.setAcceptMode (QFileDialog::AcceptSave);
  fdia.setConfirmOverwrite (true);
  fdia.setCaption (QString (tl::to_string (QObject::tr ("Save ")).c_str ()) + (title.empty () ? m_title : tl::to_qstring (title)));
  if (! file_name.isEmpty ()) {
    fdia.selectFile (file_name);
  }

  QStringList files;
  if (fdia.exec ()) {
    files = fdia.selectedFiles();
    if (files.size () > 0) {
      fp = tl::to_string (files [0]);
      QFileInfo fi (files [0]);
      m_dir = fi.absoluteDir ();
      return true;
    }
  }

  return false;
#endif
}

} // namespace lay

