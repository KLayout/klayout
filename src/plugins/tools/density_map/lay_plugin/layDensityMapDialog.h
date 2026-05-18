
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


#ifndef HDR_layDensityMapDialog
#define HDR_layDensityMapDialog

#include "ui_DensityMapDialog.h"

#include "layLayoutView.h"
#include "layBrowser.h"
#include "layMarker.h"

namespace img
{
  class Object;
}

namespace lay
{

class DensityMapDialog
  : public lay::Browser,
    private Ui::DensityMapDialog
{
Q_OBJECT 

public:
  DensityMapDialog (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~DensityMapDialog ();

public slots:
  void layer_mode_changed (int);
  void region_mode_changed (int);
  void accept ();
  void apply ();

private:
  struct DensityMapParameters
  {
    DensityMapParameters ()
      : pixel_size (100.0), threads (1)
    { }

    //  Collects all cv_index/layer index pairs used for input
    std::vector<std::pair<unsigned int, unsigned int> > input_layers;

    //  The region to compute the density map from
    db::DBox region;

    //  The pixel size
    double pixel_size;

    //  The window size or zero for "no window"
    double window_size;

    //  The boundary mode
    std::string boundary_mode;

    //  The number of threads to use
    int threads;
  };

  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

  void make_density_map ();
  void compute_density_map (const DensityMapParameters &par);
  void average_window (img::Object &img_object, const std::string boundary_mode, int nw);

};

}

#endif

