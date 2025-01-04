
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


#ifndef HDR_gtfUiDialog
#define HDR_gtfUiDialog

#include <QMainWindow>
#include <QFrame>

#include <gtf.h>

class Ui_GtfUiDialog;
class QTreeView;
class QTreeWidgetItem;
class QModelIndex;

namespace gtf
{

class UiDialog 
  : public QMainWindow
{
Q_OBJECT

public:
  UiDialog ();
  ~UiDialog ();

  void open_files (const std::string &fn_au, const std::string &fn_current);

public slots:
  void item_selected (QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
  Ui_GtfUiDialog *mp_ui;
  std::string m_fn_au;
  std::string m_fn_current;
  gtf::EventList m_au_events;
  gtf::EventList m_current_events;
};

class StripedBar 
  : public QFrame
{
Q_OBJECT

public:
  StripedBar (QWidget *parent);

  void paintEvent (QPaintEvent *event);

  void set_treeview (QTreeView *tv);

public slots:
  void force_update (int);
  void force_update (const QModelIndex &);

private:
  QTreeView *mp_tv;
};

}

#endif


