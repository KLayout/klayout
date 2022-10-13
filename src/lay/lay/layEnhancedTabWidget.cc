
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


#include "layEnhancedTabWidget.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QString>
#include <QToolButton>

namespace lay
{

// ---------------------------------------------------------------------------------------------
//  EnhancedTabWidget implementation

EnhancedTabWidget::EnhancedTabWidget (QWidget *parent)
  : QTabWidget (parent)
{
  mp_list_tool_button = new QToolButton (this);
  mp_list_tool_button->setAutoRaise (true);
  mp_list_tool_button->setIcon (QIcon (QString::fromUtf8 (":/find_16px.png")));
  mp_list_tool_button->setIconSize (QSize (20, 20));
  mp_list_tool_button->setMenu (new QMenu (this));
  mp_list_tool_button->setPopupMode (QToolButton::InstantPopup);
  mp_list_tool_button->setToolButtonStyle (Qt::ToolButtonIconOnly);
  mp_list_tool_button->setToolTip ( tr ("List of all opened views"));
  setCornerWidget (mp_list_tool_button, Qt::TopRightCorner);

  connect (mp_list_tool_button->menu (), SIGNAL (aboutToShow()),
           this, SLOT (list_tool_button_menu_about_to_show()));

  mp_list_action_group = new QActionGroup (this);
  mp_list_action_group->setExclusive (true);

  connect (mp_list_action_group, SIGNAL (triggered(QAction *)),
           this, SLOT (list_action_group_triggered(QAction *)));
}

EnhancedTabWidget::~EnhancedTabWidget () = default;

void EnhancedTabWidget::tabInserted (int index)
{
  QTabWidget::tabInserted (index);
  update_list_button_visibility ();
}

void EnhancedTabWidget::tabRemoved (int index)
{
  QTabWidget::tabRemoved (index);
  update_list_button_visibility ();
}

void EnhancedTabWidget::list_action_group_triggered (QAction *action)
{
  setCurrentIndex (action->data ().toInt ());
}

void EnhancedTabWidget::list_tool_button_menu_about_to_show ()
{
  mp_list_tool_button->menu ()->clear ();
  if (count () > 1) {
    for (int i = 0; i < count (); ++i) {
      QAction *action = mp_list_tool_button->menu ()->addAction (tabText (i));
      action->setCheckable (true);
      action->setData (QVariant (i));
      mp_list_action_group->addAction (action);
    }
    mp_list_action_group->actions ().at (currentIndex ())->setChecked (true);
  }
}

void EnhancedTabWidget::update_list_button_visibility()
{
  if (cornerWidget (Qt::TopRightCorner) != nullptr) {
    cornerWidget (Qt::TopRightCorner)->setVisible (count () > 1);
  }
}

}

