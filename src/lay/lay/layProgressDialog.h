
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


#ifndef HDR_layProgressDialog
#define HDR_layProgressDialog

#include "layCommon.h"

#include "tlObject.h"
#include "tlProgress.h"

#include <QDialog>

namespace lay
{

class ProgressReporter;
class ProgressWidget;

class ProgressDialog
  : public QDialog,
    public tl::Object
{
public:
  ProgressDialog (QWidget *parent, lay::ProgressReporter *pr);

  void closeEvent (QCloseEvent * /*event*/);

  void set_progress (tl::Progress *progress);
  void add_widget (QWidget *widget);
  void remove_widget ();
  QWidget *get_widget () const;

private:
  lay::ProgressWidget *mp_progress_widget;
  lay::ProgressReporter *mp_pr;
};

}

#endif
