
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

#if defined(HAVE_QT)

#ifndef HDR_layFileDialog
#define HDR_layFileDialog

#include "layuiCommon.h"

#include <QDir>
#include <QObject>

#include <string>
#include <vector>

namespace lay
{

/**
 *  @brief Generic file dialog 
 *
 *  This dialog is provided to encapsulate the Qt file dialog.
 *  This implementation allows staying within a directory even
 *  if the static convenience functions are used. Under Windows,
 *  we need to use the static functions since these use the 
 *  system dialogs.
 */
class LAYUI_PUBLIC FileDialog 
  : public QObject 
{
public:
  /**
   *  @brief Instantiate the file dialog 
   *
   *  @param parent The parent object that owns the dialog
   *  @param title The title string
   *  @param filters The filters as provided for QFileDialog::getOpenFileName 
   *  @param def_suffix The default suffix
   */
  FileDialog (QWidget *parent, const std::string &title, const std::string &filters, const std::string &def_suffix = std::string ());

  /**
   *  @brief The destructor
   */
  ~FileDialog ();

  /**
   *  @brief Gets a file name to read
   */
  bool get_open (std::string &file_name, const std::string &title = std::string ());

  /**
   *  @brief Reads multiple files names
   */
  bool get_open (std::vector<std::string> &file_names, const std::string &dir = std::string (), const std::string &title = std::string ());

  /**
   *  @brief Gets a file name to save
   */
  bool get_save (std::string &file_name, const std::string &title = std::string ());

  /**
   *  @brief Gets the selected filter or -1 if no specific filter was selected
   *
   *  This value is only set after get_open or get_save returned true
   */
  int selected_filter () const;

  /**
   *  @brief Make the file names use UTF8 encoding 
   *
   *  TODO: this is a quick hack - basically all 8bit strings should be UTF8 and 
   *  file names should be handled properly by QString and tl::Stream.
   */
  static void set_utf8 (bool utf);

  /**
   *  @brief Gets the index of the selected filter from the filter list
   */
  static int find_selected_filter (const QString &filters, const QString &selected_filter);

  /**
   *  @brief Adds the default extension unless there is one already
   */
  static std::string add_default_extension (const std::string &path, const QString &selected_filter);

private:
  QDir m_dir;
  QString m_title;
  QString m_filters;
  QString m_sel_filter;
  QString m_def_suffix;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
