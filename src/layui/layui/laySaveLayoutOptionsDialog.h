
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

#if defined(HAVE_QT)

#ifndef HDR_laySaveLayoutOptionsDialog
#define HDR_laySaveLayoutOptionsDialog

#include "layuiCommon.h"
#include "dbStream.h"
#include "dbSaveLayoutOptions.h"
#include "layStream.h"
#include "tlStream.h"

#include <string>
#include <QDialog>

class QScrollArea;
class QAbstractButton;
class QWidget;

namespace db
{
  class SaveLayoutOptions;
  class Technologies;
}

namespace Ui
{
  class SaveLayoutAsOptionsDialog;
  class SaveLayoutOptionsDialog;
}

namespace lay
{

class LayoutViewBase;

class LAYUI_PUBLIC SaveLayoutAsOptionsDialog
  : public QDialog
{
  Q_OBJECT 

public:
  SaveLayoutAsOptionsDialog (QWidget *parent, const std::string &title);
  ~SaveLayoutAsOptionsDialog ();

  bool get_options (lay::LayoutViewBase *view, unsigned int cv_index, const std::string &fn, tl::OutputStream::OutputStreamMode &compression, db::SaveLayoutOptions &options);

public slots:
  void ok_button_pressed ();
  void fmt_cbx_changed (int);

private:
  Ui::SaveLayoutAsOptionsDialog *mp_ui;
  std::vector< std::pair<StreamWriterOptionsPage *, std::string> > m_pages;
  std::vector<int> m_tab_positions;
  std::string m_filename;
  db::SaveLayoutOptions m_options;
  const db::Technology *mp_tech;
};

class LAYUI_PUBLIC SaveLayoutOptionsDialog
  : public QDialog
{
  Q_OBJECT 

public:
  SaveLayoutOptionsDialog (QWidget *parent, const std::string &title);
  ~SaveLayoutOptionsDialog ();

  bool edit_global_options (lay::Dispatcher *dispatcher, db::Technologies *technologies);
  bool get_options (db::SaveLayoutOptions &options);

public slots:
  void ok_button_pressed ();
  void reset_button_pressed ();
  void button_pressed (QAbstractButton *button);
  void current_tech_changed (int index);

private:
  Ui::SaveLayoutOptionsDialog *mp_ui;
  std::vector< std::pair<StreamWriterOptionsPage *, std::string> > m_pages;
  int m_technology_index;
  std::vector<db::SaveLayoutOptions> m_opt_array;
  std::vector<const db::Technology *> m_tech_array;

  void commit ();
  void update ();
  bool get_options_internal ();
};

}

#endif

#endif  //  defined(HAVE_QT)
