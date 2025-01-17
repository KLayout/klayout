
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

#if defined(HAVE_QT)

#ifndef HDR_layConfigurationDialog
#define HDR_layConfigurationDialog

#include "layWidgets.h"
#include "tlException.h"
#include "dbTypes.h"

#include <QDialog>

namespace Ui
{
  class ConfigurationDialog;
}

namespace lay
{

class Dispatcher;
class ConfigPage;
class PluginDeclaration;

class LAYUI_PUBLIC ConfigurationDialog
  : public QDialog
{
  Q_OBJECT

public:
  ConfigurationDialog (QWidget *parent, lay::Dispatcher *root, lay::PluginDeclaration *decl, const char *name = "");
  ConfigurationDialog (QWidget *parent, lay::Dispatcher *root, const std::string &plugin_name, const char *name = "");
  ~ConfigurationDialog ();
  
  void commit ();

public slots:
  virtual void ok_clicked ();

private:
  lay::Dispatcher *mp_root;
  std::vector <lay::ConfigPage *> m_config_pages;
  Ui::ConfigurationDialog *mp_ui;

  void init (const lay::PluginDeclaration *decl);
};

}

#endif

#endif  //  defined(HAVE_QT)
