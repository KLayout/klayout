
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_layNavigator
#define HDR_layNavigator

#include <QFrame>

#include "tlDeferredExecution.h"
#include "layLayerProperties.h"

#include <map>

class QCloseEvent;
class QShowEvent;
class QFrame;
class QLabel;

namespace lay
{

class MainWindow;
class LayoutView;
class LayoutViewWidget;
class AbstractMenu;
class DMarker;
class NavigatorService;

/**
 *  @brief A structure containing all the frozen view information
 */
struct NavigatorFrozenViewInfo
{
  NavigatorFrozenViewInfo (const LayerPropertiesList &lp, std::pair<int, int> hier)
    : layer_properties (lp), hierarchy_levels (hier)
  {
    // .. nothing yet ..
  }

  NavigatorFrozenViewInfo ()
    : layer_properties (), hierarchy_levels (0, 0)
  {
    // .. nothing yet ..
  }

  LayerPropertiesList layer_properties;
  std::pair<int, int> hierarchy_levels;
};

/**
 *  @brief The navigator window 
 */
class Navigator 
  : public QFrame,
    public tl::Object
{
Q_OBJECT

public:
  Navigator (MainWindow *main_window);
  ~Navigator ();

  void update ();
  void freeze_clicked (); 
  void all_hier_levels (bool f);
  void show_images (bool f);

protected:
  virtual void closeEvent (QCloseEvent *event);
  virtual void showEvent (QShowEvent *event);
  virtual void resizeEvent (QResizeEvent *event);

private slots:
  void menu_changed ();

private:
  bool m_show_all_hier_levels;
  bool m_show_images;
  bool m_update_layers_needed;
  bool m_update_needed;
  MainWindow *mp_main_window;  
  LayoutViewWidget *mp_view;
  QLabel *mp_placeholder_label;  
  QFrame *mp_menu_bar;
  LayoutView *mp_source_view;
  NavigatorService *mp_service;
  tl::DeferredMethod<Navigator> m_do_view_changed;
  tl::DeferredMethod<Navigator> m_do_layers_changed;
  tl::DeferredMethod<Navigator> m_do_content_changed;
  tl::DeferredMethod<Navigator> m_do_update_menu_dm;
  std::map <lay::LayoutView *, NavigatorFrozenViewInfo> m_frozen_list;

  void attach_view (LayoutView *);
  void attach_view ();
  void view_closed (int);
  void update_layers ();
  void do_update_menu ();
  void view_changed ();
  void content_changed ();
  void layers_changed (int);
  void viewport_changed ();
  void hier_levels_changed ();
  void update_background_color ();

  void content_changed_with_int (int)
  {
    content_changed ();
  }
};

}

#endif

