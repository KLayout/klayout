
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


#ifndef HDR_layTextProgressDelegate
#define HDR_layTextProgressDelegate

#include "layCommon.h"

#include "layTextProgress.h"

namespace lay
{

class MainWindow;

class TextProgressDelegate
  : public lay::TextProgress
{
public:
  TextProgressDelegate (MainWindow *mw, int verbosity);

  virtual void update_progress (tl::Progress *progress);
  virtual void show_progress_bar (bool show);
  virtual bool progress_wants_widget () const;
  virtual void progress_add_widget (QWidget *widget);
  virtual QWidget *progress_get_widget () const;
  virtual void progress_remove_widget ();

private:
  MainWindow *mp_mw;
};

}

#endif
