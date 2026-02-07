
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


#ifndef HDR_FillDialog
#define HDR_FillDialog

#include "ui_FillDialog.h"

#include "layLayoutView.h"
#include "layPlugin.h"
#include "layMarker.h"
#include "layCommon.h"

#include "dbRegion.h"

#include <QDialog>

namespace lay
{

struct LAY_PUBLIC FillParameters
{
  FillParameters ()
    : exclude_all_layers (true), fill_region_mode (WholeCell), enhanced_fill (false)
  { }

  enum FillRegionMode {
    WholeCell,
    Region,
    Layer
  };

  bool exclude_all_layers;
  std::vector<db::LayerProperties> exclude_layers;
  FillRegionMode fill_region_mode;
  db::Region fill_region;
  db::LayerProperties fill_region_layer;
  db::DVector exclude_distance;
  db::DVector border_distance;
  bool enhanced_fill;
  std::string fill_cell_name;
  db::DVector fill_cell_margin;
  db::DVector row_step, column_step;
  db::DBox fc_bbox;
  std::string fill_cell_name2;
  db::DVector fill_cell_margin2;
  db::DVector row_step2, column_step2;
  db::DBox fc_bbox2;
};

class LAY_PUBLIC FillDialog
  : public QDialog,
    public lay::Plugin,
    private Ui::FillDialog
{
Q_OBJECT 

public:
  FillDialog (QWidget *parent, lay::LayoutViewBase *view);
  ~FillDialog ();

public slots:
  void fill_area_changed (int);
  void ok_pressed ();
  void choose_fc ();
  void choose_fc_2nd ();

private:
  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

  void generate_fill (const FillParameters &fp);
  FillParameters get_fill_parameters ();

  lay::LayoutViewBase *mp_view;
};

}

#endif

