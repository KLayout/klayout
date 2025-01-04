
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

#ifndef HDR_layXORToolDialog
#define HDR_layXORToolDialog

#include <QDialog>

#include "tlObject.h"

namespace Ui
{
  class XORToolDialog;
}

namespace lay
{
  class LayoutViewBase;
}

namespace lay
{

extern std::string cfg_xor_input_mode;
extern std::string cfg_xor_output_mode;
extern std::string cfg_xor_nworkers;
extern std::string cfg_xor_layer_offset;
extern std::string cfg_xor_axorb;
extern std::string cfg_xor_anotb;
extern std::string cfg_xor_bnota;
extern std::string cfg_xor_summarize;
extern std::string cfg_xor_tolerances;
extern std::string cfg_xor_tiling;
extern std::string cfg_xor_region_mode;

class XORToolDialog
  : public QDialog
{
Q_OBJECT 

public:
  XORToolDialog (QWidget *parent);
  ~XORToolDialog ();

  int exec_dialog (lay::LayoutViewBase *view);

protected:
  void accept ();
  void run_xor ();

protected slots:
  void deep_changed ();
  void input_changed (int index);
  void output_changed (int index);

private:
  Ui::XORToolDialog *mp_ui;
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
};

}

#endif

