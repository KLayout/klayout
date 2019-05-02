
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "layNetExportDialog.h"
#include "layPlugin.h"

#include "ui_NetExportDialog.h"

#include <sstream>

namespace lay
{

extern std::string cfg_l2ndb_net_cell_prefix;
extern std::string cfg_l2ndb_circuit_cell_prefix;
extern std::string cfg_l2ndb_produce_circuit_cells;
extern std::string cfg_l2ndb_device_cell_prefix;
extern std::string cfg_l2ndb_produce_device_cells;


NetExportDialog::NetExportDialog (QWidget *parent)
  : QDialog (parent)
{
  ui = new Ui::NetExportDialog ();
  ui->setupUi (this);
}

NetExportDialog::~NetExportDialog ()
{
  delete ui;
  ui = 0;
}

void
NetExportDialog::set_net_prefix (const std::string &net_prefix)
{
  ui->net_cell_prefix->setText (tl::to_qstring (net_prefix));
}

std::string
NetExportDialog::net_prefix ()
{
  return tl::to_string (ui->net_cell_prefix->text ());
}

void
NetExportDialog::set_produce_circuit_cells (bool f)
{
  ui->circuit_cell_prefix->setEnabled (f);
  ui->produce_circuit_cells_cb->setChecked (f);
}

bool
NetExportDialog::produce_circuit_cells ()
{
  return ui->produce_circuit_cells_cb->isChecked ();
}

void
NetExportDialog::set_circuit_cell_prefix (const std::string &cell_prefix)
{
  ui->circuit_cell_prefix->setText (tl::to_qstring (cell_prefix));
}

std::string
NetExportDialog::circuit_cell_prefix ()
{
  return tl::to_string (ui->circuit_cell_prefix->text ());
}

void
NetExportDialog::set_produce_device_cells (bool f)
{
  ui->device_cell_prefix->setEnabled (f);
  ui->produce_device_cells_cb->setChecked (f);
}

bool
NetExportDialog::produce_device_cells ()
{
  return ui->produce_device_cells_cb->isChecked ();
}

void
NetExportDialog::set_device_cell_prefix (const std::string &cell_prefix)
{
  ui->device_cell_prefix->setText (tl::to_qstring (cell_prefix));
}

std::string
NetExportDialog::device_cell_prefix ()
{
  return tl::to_string (ui->device_cell_prefix->text ());
}

int
NetExportDialog::exec (lay::PluginRoot *plugin_root)
{
  std::string v;
  plugin_root->config_get (cfg_l2ndb_net_cell_prefix, v);
  set_net_prefix (v);

  bool f = false;
  plugin_root->config_get (cfg_l2ndb_produce_circuit_cells, f);
  set_produce_circuit_cells (f);

  v.clear ();
  plugin_root->config_get (cfg_l2ndb_circuit_cell_prefix, v);
  set_circuit_cell_prefix (v);

  f = false;
  plugin_root->config_get (cfg_l2ndb_produce_device_cells, f);
  set_produce_device_cells (f);

  v.clear ();
  plugin_root->config_get (cfg_l2ndb_device_cell_prefix, v);
  set_device_cell_prefix (v);

  int ret = QDialog::exec ();
  if (ret) {

    plugin_root->config_set (cfg_l2ndb_net_cell_prefix, net_prefix ());
    plugin_root->config_set (cfg_l2ndb_produce_circuit_cells, tl::to_string (produce_circuit_cells ()));
    plugin_root->config_set (cfg_l2ndb_circuit_cell_prefix, circuit_cell_prefix ());
    plugin_root->config_set (cfg_l2ndb_produce_device_cells, tl::to_string (produce_device_cells ()));
    plugin_root->config_set (cfg_l2ndb_device_cell_prefix, device_cell_prefix ());

  }

  return ret;
}

}
