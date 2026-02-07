
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

#ifndef HDR_layD25View
#define HDR_layD25View

#include <QDialog>
#include <QListWidgetItem>

#include "tlObject.h"
#include "layBrowser.h"
#include "layViewOp.h"

namespace Ui
{
  class D25View;
}

namespace lay
{
  class LayoutViewBase;
}

namespace db
{
  class Region;
  class Edges;
  class EdgePairs;
  struct LayerProperties;
}

namespace lay
{

class D25View
  : public lay::Browser
{
Q_OBJECT 

public:
  D25View (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~D25View ();

  virtual bool configure (const std::string &name, const std::string &value);
  virtual void menu_activated (const std::string &symbol);
  virtual void deactivated ();
  virtual void activated ();

  static D25View *open (lay::LayoutViewBase *view);
  void close ();
  void clear ();
  void begin (const std::string &generator);
  void open_display (const tl::color_t *frame_color, const tl::color_t *fill_color, const db::LayerProperties *like, const std::string *name);
  void close_display ();
  void entry (const db::Region &data, double dbu, double zstart, double zstop);
  void entry_edge (const db::Edges &data, double dbu, double zstart, double zstop);
  void entry_edge_pair (const db::EdgePairs &data, double dbu, double zstart, double zstop);
  void finish ();

protected:
  void accept ();
  void reject ();

private slots:
  void fit_button_clicked ();
  void scale_factor_changed (double f);
  void scale_slider_changed (int value);
  void scale_value_edited ();
  void vscale_factor_changed (double f);
  void vscale_slider_changed (int value);
  void vscale_value_edited ();
  void init_failed ();
  void rerun_button_pressed ();
  void material_item_changed (QListWidgetItem *);
  void hide_all_triggered ();
  void hide_selected_triggered ();
  void show_all_triggered ();
  void show_selected_triggered ();
  void visibility_follows_selection_changed (bool checked);
  void update_visibility ();

private:
  Ui::D25View *mp_ui;
  tl::DeferredMethod<D25View> dm_rerun_macro;
  tl::DeferredMethod<D25View> dm_fit;
  std::string m_generator;
  bool m_visibility_follows_selection;

  void cellviews_changed ();
  void layer_properties_changed (int);
  void rerun_macro ();
  void fit ();
};

}

#endif

