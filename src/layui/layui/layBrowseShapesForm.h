
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

#ifndef HDR_layBrowseShapesForm
#define HDR_layBrowseShapesForm

#include "ui_BrowseShapesForm.h"
#include "ui_BrowseShapesConfigPage.h"

#include "layLayoutViewBase.h"
#include "layMargin.h"
#include "layPluginConfigPage.h"
#include "layBrowser.h"
#include "layMarker.h"

class QTreeWidgetItem;

namespace lay
{

class BrowseShapesConfigPage
  : public lay::ConfigPage,
    private Ui::BrowseShapesConfigPage
{
  Q_OBJECT 

public:
  BrowseShapesConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void context_changed (int);
  void window_changed (int);
};

class BrowseShapesForm
  : public lay::Browser,
    private Ui::BrowseShapesForm
{
  Q_OBJECT 

public:
  enum mode_type { ToCellView = 0, AnyTop, Local };
  enum window_type { DontChange = 0, FitCell, FitMarker, Center, CenterSize };

  BrowseShapesForm (lay::Dispatcher *root, LayoutViewBase *view);
  ~BrowseShapesForm ();

  bool eventFilter (QObject *watched, QEvent *event);

public slots:
  void cell_changed(QTreeWidgetItem *, QTreeWidgetItem *);
  void shape_inst_changed();
  void cell_inst_changed(QTreeWidgetItem *, QTreeWidgetItem *);
  void next_cell ();
  void prev_cell ();
  void next_shape ();
  void prev_shape ();
  void next_inst ();
  void prev_inst ();
  void configure ();
  
private:
  lay::CellView m_cellview;
  int m_cv_index;
  std::vector <lay::LayerPropertiesConstIterator> m_lprops;
  std::vector <std::string> m_layer_names;

  bool m_cell_changed_enabled;
  bool m_view_changed;
  bool m_cell_inst_changed_enabled;
  bool m_shape_inst_changed_enabled;
  bool m_ef_enabled;

  std::vector<lay::ShapeMarker *> mp_markers;

  mode_type m_mode;
  window_type m_window;

  lay::Margin m_window_dim;
  std::string m_context_cell;

  unsigned int m_max_inst_count;
  unsigned int m_max_shape_count;

  lay::DisplayState m_display_state;

  void update ();
  void update_cell_list ();

  bool fill_cell_instances (const db::ICplxTrans &t, const db::Layout &layout, const db::Cell *from, const db::Cell *to, bool to_parent, const std::string &path, QList<QTreeWidgetItem *> &items, unsigned int &count);

  void highlight_current ();
  void remove_marker ();

  bool adv_cell (bool up);
  bool adv_shape (bool up);
  bool adv_cell_inst (bool up);

  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Browser interface
  virtual void activated ();
  virtual void deactivated ();

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

};

}

#endif

#endif  //  defined(HAVE_QT)
