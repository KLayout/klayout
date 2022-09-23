
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#ifndef HDR_layEnhancedTabWidget
#define HDR_layEnhancedTabWidget

#include <QTabWidget>

class QActionGroup;
class QToolButton;

namespace lay
{

class EnhancedTabWidget
  : public QTabWidget
{
Q_OBJECT

public:
  EnhancedTabWidget (QWidget *parent);
  ~EnhancedTabWidget () override;

protected:
  void tabInserted (int index) override;
  void tabRemoved (int index) override;

private slots:
  void list_action_group_triggered (QAction* action);
  void list_tool_button_menu_about_to_show ();

private:
  QActionGroup *mp_list_action_group;
  QToolButton *mp_list_tool_button;

  void update_list_button_visibility ();
};

}

#endif

