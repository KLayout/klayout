
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


#ifndef HDR_laySettingsForm
#define HDR_laySettingsForm

#include <QDialog>

#include "ui_SettingsForm.h"

#include "layWidgets.h"
#include "tlException.h"
#include "dbTypes.h"

#include <vector>

namespace lay
{

class Dispatcher;
class ConfigPage;

class SettingsForm
  : public QDialog, private Ui::SettingsForm 
{
  Q_OBJECT

public:
  SettingsForm (QWidget *parent, lay::Dispatcher *dispatcher, const char *name);
  
  void setup ();
  void commit ();

public slots:
  virtual void ok_clicked ();
  virtual void apply_clicked ();
  void reset_clicked ();
  void item_changed (QTreeWidgetItem *, QTreeWidgetItem *);

private:
  lay::Dispatcher *mp_dispatcher;
  std::vector <lay::ConfigPage *> m_config_pages;
  bool m_finalize_recursion;
};

}

#endif

