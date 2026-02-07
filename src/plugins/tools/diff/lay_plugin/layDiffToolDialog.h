
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




#ifndef HDR_layDiffToolDialog
#define HDR_layDiffToolDialog

#include <QDialog>
#include <string>

namespace Ui
{
  class DiffToolDialog;
}

namespace lay
{
  class LayoutViewBase;
}

namespace lay
{

extern std::string cfg_diff_run_xor;
extern std::string cfg_diff_detailed;
extern std::string cfg_diff_summarize;
extern std::string cfg_diff_expand_cell_arrays;
extern std::string cfg_diff_exact;

class DiffToolDialog
  : public QDialog
{
Q_OBJECT 

public:
  DiffToolDialog (QWidget *parent);
  ~DiffToolDialog ();

  int exec_dialog (lay::LayoutViewBase *view);

protected slots:
  void xor_changed () { update (); }

protected:
  void accept ();
  void run_diff ();
  void update ();

private:
  Ui::DiffToolDialog *mp_ui;
  lay::LayoutViewBase *mp_view;
};

}

#endif

