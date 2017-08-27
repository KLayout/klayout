
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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



#ifndef HDR_layMainConfigPages
#define HDR_layMainConfigPages

#include <QObject>

#include "layPlugin.h"

#include <map>

namespace Ui {
  class MainConfigPage;
  class MainConfigPage2;
  class MainConfigPage3;
  class MainConfigPage4;
  class MainConfigPage5;
  class MainConfigPage6;
  class MainConfigPage7;
  class KeyBindingsConfigPage;
}

class QTreeWidgetItem;

namespace lay
{

/** 
 *  @brief A utility function to convert the packed key binding in the cfg_key_bindings string to a vector
 */
std::vector<std::pair<std::string, std::string> > unpack_key_binding (const std::string &packed);

/** 
 *  @brief A utility function to convert the key binding (as path/shortcut pair vector) to a packed string for cfg_key_bindings
 */
std::string pack_key_binding (const std::vector<std::pair<std::string, std::string> > &unpacked);

class ColorButton;

class MainConfigPage 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage (QWidget *parent);
  ~MainConfigPage ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage *mp_ui;
};

class MainConfigPage2 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage2 (QWidget *parent);
  ~MainConfigPage2 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage2 *mp_ui;
};

class MainConfigPage3 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage3 (QWidget *parent);
  ~MainConfigPage3 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage3 *mp_ui;
};

class MainConfigPage4 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage4 (QWidget *parent);
  ~MainConfigPage4 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage4 *mp_ui;
};

class MainConfigPage5 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage5 (QWidget *parent);
  ~MainConfigPage5 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage5 *mp_ui;
};

class MainConfigPage6 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage6 (QWidget *parent);
  ~MainConfigPage6 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage6 *mp_ui;
};

class MainConfigPage7
  : public lay::ConfigPage
{
Q_OBJECT

public:
  MainConfigPage7 (QWidget *parent);
  ~MainConfigPage7 ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

private:
  Ui::MainConfigPage7 *mp_ui;
};

class KeyBindingsConfigPage
  : public lay::ConfigPage
{
Q_OBJECT

public:
  KeyBindingsConfigPage (QWidget *parent);
  ~KeyBindingsConfigPage ();

  virtual void setup (lay::PluginRoot *root);
  virtual void commit (lay::PluginRoot *root);

  static void set_default ();

public slots:
  void current_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void reset_clicked ();

private:
  Ui::KeyBindingsConfigPage *mp_ui;
  std::map<std::string, std::string> m_current_bindings;
  bool m_enable_event;
  static std::vector<std::pair<std::string, std::string> > m_default_bindings;

  void apply (const std::vector<std::pair<std::string, std::string> > &bindings);
};

}

#endif

