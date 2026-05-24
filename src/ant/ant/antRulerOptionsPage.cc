
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

#if defined(HAVE_QT)

#include "antRulerOptionsPage.h"
#include "antConfig.h"
#include "laySnap.h"
#include "layConverters.h"
#include "layDispatcher.h"
#include "tlString.h"
#include "tlClassRegistry.h"

#include "ui_RulerOptions.h"

namespace ant
{

// ------------------------------------------------------------------
//  RulerOptionsPage implementation

RulerOptionsPage::RulerOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPageWidget (view, dispatcher)
{
  mp_ui = new Ui::RulerOptions ();
  mp_ui->setupUi (this);

  connect (mp_ui->angle_cb, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->snap_to_grid_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->snap_to_objects_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
}

RulerOptionsPage::~RulerOptionsPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string
RulerOptionsPage::title () const
{
  return tl::to_string (QObject::tr ("Ruler Options"));
}

//  Must match order in angle_cb
static std::vector<lay::angle_constraint_type> s_ac_options =
{
  lay::AC_Any,
  lay::AC_Diagonal,
  lay::AC_DiagonalOnly,
  lay::AC_Ortho,
  lay::AC_Horizontal,
  lay::AC_Vertical
};

void
RulerOptionsPage::apply (lay::Dispatcher *root)
{
  lay::angle_constraint_type ac = lay::AC_Any;
  int ai = mp_ui->angle_cb->currentIndex ();
  if (ai >= 0 && ai < int (s_ac_options.size ())) {
    ac = s_ac_options [ai];
  }

  lay::ACConverter acc;
  root->config_set (cfg_ruler_snap_mode, acc.to_string (ac));

  root->config_set (cfg_ruler_obj_snap, tl::to_string (mp_ui->snap_to_objects_cbx->isChecked ()));
  root->config_set (cfg_ruler_grid_snap, tl::to_string (mp_ui->snap_to_grid_cbx->isChecked ()));
}

void
RulerOptionsPage::setup (lay::Dispatcher *root)
{
  lay::ACConverter acc;
  lay::angle_constraint_type ac;

  ac = lay::AC_Any;
  root->config_get (cfg_ruler_snap_mode, ac, acc);
  for (int ai = 0; ai < int (s_ac_options.size ()); ++ai) {
    if (s_ac_options [ai] == ac) {
      mp_ui->angle_cb->setCurrentIndex (ai);
    }
  }

  bool snap_to_grid = false;
  root->config_get (cfg_ruler_grid_snap, snap_to_grid);
  mp_ui->snap_to_grid_cbx->setChecked (snap_to_grid);

  bool snap_to_objects = false;
  root->config_get (cfg_ruler_obj_snap, snap_to_objects);
  mp_ui->snap_to_objects_cbx->setChecked (snap_to_objects);
}

static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_ruler_options (new lay::EditorOptionsPageFactory<RulerOptionsPage> ("ant::RulerOptions"), 0);

}

#endif

