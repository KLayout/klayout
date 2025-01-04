
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

#ifndef HDR_layBrowseInstancesForm
#define HDR_layBrowseInstancesForm

#include "ui_BrowseInstancesForm.h"
#include "ui_BrowseInstancesConfigPage.h"

#include "layLayoutViewBase.h"
#include "layMargin.h"
#include "layPluginConfigPage.h"
#include "layBrowser.h"
#include "layMarker.h"

class QTreeWidget;
class QTreeWidgetItem;

namespace lay
{

class BrowseInstancesConfigPage
  : public lay::ConfigPage,
    private Ui::BrowseInstancesConfigPage
{
  Q_OBJECT 

public:
  BrowseInstancesConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void context_changed (int);
  void window_changed (int);
};

class BrowseInstancesForm
  : public lay::Browser,
    private Ui::BrowseInstancesForm
{
  Q_OBJECT 

public:
  enum mode_type { ToCellView = 0, AnyTop, Parent };
  enum window_type { DontChange = 0, FitCell, FitMarker, Center, CenterSize };

  BrowseInstancesForm (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~BrowseInstancesForm ();

  bool eventFilter (QObject *watched, QEvent *event);

public slots:
  void cell_changed (QTreeWidgetItem *, QTreeWidgetItem *);
  void cell_inst_changed ();
  void choose_cell_pressed ();
  void next_cell ();
  void prev_cell ();
  void next_inst ();
  void prev_inst ();
  void configure ();
  
private:
  unsigned int m_cv_index;
  std::vector<db::DCplxTrans> m_global_trans;
  lay::CellView::cell_index_type m_cell_index;
  bool m_cell_changed_enabled;
  bool m_view_changed;
  bool m_cell_inst_changed_enabled;
  bool m_ef_enabled;
  QList<QTreeWidgetItem *> m_items;
  
  std::vector<lay::Marker *> mp_markers;

  lay::CellView m_context_cv;

  mode_type m_mode;
  window_type m_window;

  lay::Margin m_window_dim;
  std::string m_context_cell;

  unsigned int m_max_inst_count;
  unsigned int m_current_count;

  lay::DisplayState m_display_state;

  bool fill_cell_instances (const db::ICplxTrans &t, const db::Layout &layout, const db::Cell *parent_sel, const db::Cell *from, const db::Cell *to, bool to_parent, const std::string &path, QList<QTreeWidgetItem *> &items);

  void highlight_current ();
  void remove_marker ();

  bool adv_cell_inst (bool up);
  bool adv_cell (bool up);

  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Browser interface
  virtual void activated ();
  virtual void deactivated ();

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

  //  change to the given cell in the given cellview
  void change_cell (db::cell_index_type cell, int cv_index);

};

}

#endif

#endif  //  defined(HAVE_QT)
