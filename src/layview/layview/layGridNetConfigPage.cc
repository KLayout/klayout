
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "laybasicConfig.h"
#include "layGridNetConfigPage.h"
#include "layConverters.h"
#include "layDispatcher.h"
#include "layGridNet.h"
#include "layWidgets.h"
#include "ui_GridNetConfigPage.h"

namespace lay
{

// ------------------------------------------------------------
//  Implementation of the configuration page

GridNetConfigPage::GridNetConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::GridNetConfigPage ();
  mp_ui->setupUi (this);

  mp_grid_color_cbtn = new lay::ColorButton (mp_ui->grid_net_color_pb);
  mp_grid_grid_color_cbtn = new lay::ColorButton (mp_ui->grid_grid_color_pb);
  mp_grid_axis_color_cbtn = new lay::ColorButton (mp_ui->grid_axis_color_pb);
  mp_grid_ruler_color_cbtn = new lay::ColorButton (mp_ui->grid_ruler_color_pb);
}

GridNetConfigPage::~GridNetConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
GridNetConfigPage::setup (lay::Dispatcher *root)
{
  std::string value;

  //  Grid visibility
  bool visible = false;
  root->config_get (cfg_grid_visible, visible);
  mp_ui->grid_group->setChecked (visible);

  bool show_ruler = false;
  root->config_get (cfg_grid_show_ruler, show_ruler);
  mp_ui->show_ruler->setChecked (show_ruler);

  QColor color;
  root->config_get (cfg_grid_color, color, ColorConverter ());
  mp_grid_color_cbtn->set_color (color);

  root->config_get (cfg_grid_grid_color, color, ColorConverter ());
  mp_grid_grid_color_cbtn->set_color (color);

  root->config_get (cfg_grid_axis_color, color, ColorConverter ());
  mp_grid_axis_color_cbtn->set_color (color);

  root->config_get (cfg_grid_ruler_color, color, ColorConverter ());
  mp_grid_ruler_color_cbtn->set_color (color);

  lay::GridNet::GridStyle style;

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style0, style, GridNetStyleConverter ());
  mp_ui->style0_cbx->setCurrentIndex (int (style));

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style1, style, GridNetStyleConverter ());
  mp_ui->style1_cbx->setCurrentIndex (int (style));

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style2, style, GridNetStyleConverter ());
  mp_ui->style2_cbx->setCurrentIndex (int (style));
}

void
GridNetConfigPage::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_grid_visible, mp_ui->grid_group->isChecked ());
  root->config_set (cfg_grid_show_ruler, mp_ui->show_ruler->isChecked ());
  root->config_set (cfg_grid_color, mp_grid_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_grid_color, mp_grid_grid_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_axis_color, mp_grid_axis_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_ruler_color, mp_grid_ruler_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_style0, lay::GridNet::GridStyle (mp_ui->style0_cbx->currentIndex ()), GridNetStyleConverter ());
  root->config_set (cfg_grid_style1, lay::GridNet::GridStyle (mp_ui->style1_cbx->currentIndex ()), GridNetStyleConverter ());
  root->config_set (cfg_grid_style2, lay::GridNet::GridStyle (mp_ui->style2_cbx->currentIndex ()), GridNetStyleConverter ());
}

} // namespace lay

#endif
