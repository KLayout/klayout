
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#ifndef HDR_antConfigPage
#define HDR_antConfigPage

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "antTemplate.h"

class QListWidgetItem;

namespace Ui
{
  class RulerConfigPage;
  class RulerConfigPage2;
  class RulerConfigPage3;
  class RulerConfigPage4;
}

namespace ant 
{

class ConfigPage
  : public lay::ConfigPage
{
  Q_OBJECT 

public:
  ConfigPage (QWidget *parent);
  ~ConfigPage ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::RulerConfigPage *mp_ui;
  
  void show ();
  void commit ();
};

class ConfigPage2
  : public lay::ConfigPage
{
  Q_OBJECT 

public:
  ConfigPage2 (QWidget *parent);
  ~ConfigPage2 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::RulerConfigPage2 *mp_ui;
  
  void show ();
  void commit ();
};

class ConfigPage3
  : public lay::ConfigPage
{
  Q_OBJECT 

public:
  ConfigPage3 (QWidget *parent);
  ~ConfigPage3 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::RulerConfigPage3 *mp_ui;
  
  void show ();
  void commit ();
};

class ConfigPage4
  : public lay::ConfigPage
{
  Q_OBJECT 

public:
  ConfigPage4 (QWidget *parent);
  ~ConfigPage4 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void add_clicked ();
  void del_clicked ();
  void up_clicked ();
  void down_clicked ();
  void current_template_changed (int index);
  void double_clicked (QListWidgetItem *);
  
private:
  Ui::RulerConfigPage4 *mp_ui;
  std::vector<ant::Template> m_ruler_templates;
  int m_current_template;
  bool m_current_changed_enabled;
  
  void show ();
  void commit ();
  void update_list ();
};

} // namespace ant

#endif

#endif

