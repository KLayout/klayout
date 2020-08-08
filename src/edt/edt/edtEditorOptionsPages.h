
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_edtEditorOptionsPages
#define HDR_edtEditorOptionsPages

#include <tlVariant.h>

#include <QFrame>
#include <vector>
#include <string>

class QTabWidget;

namespace Ui
{
  class EditorOptionsDialog;
  class EditorOptionsGeneric;
  class EditorOptionsPolygon;
  class EditorOptionsBox;
  class EditorOptionsPath;
  class EditorOptionsText;
  class EditorOptionsInst;
  class EditorOptionsInstPCellParam;
}

namespace lay
{
  class PluginDeclaration;
  class Dispatcher;
  class Plugin;
}

namespace edt
{

class PCellParametersPage;

class EditorOptionsPages;

/**
 *  @brief The base class for a object properties page 
 */
class EditorOptionsPage
{
public:
  EditorOptionsPage ();
  virtual ~EditorOptionsPage ();

  virtual QWidget *q_frame () = 0;
  virtual std::string title () const = 0;
  virtual int order () const = 0;
  virtual void apply (lay::Plugin *root) = 0;
  virtual void setup (lay::Plugin *root) = 0;

  bool active () const { return m_active; }
  void activate (bool active);
  void set_owner (EditorOptionsPages *owner);

  const lay::PluginDeclaration *plugin_declaration () const { return mp_plugin_declaration; }
  void set_plugin_declaration (const lay::PluginDeclaration *pd) { mp_plugin_declaration = pd; }

private:
  EditorOptionsPages *mp_owner;
  bool m_active;
  const lay::PluginDeclaration *mp_plugin_declaration;
};

/**
 *  @brief The object properties dialog
 */
class EditorOptionsPages
  : public QFrame
{
Q_OBJECT

public:
  EditorOptionsPages (QWidget *parent, const std::vector<edt::EditorOptionsPage *> &pages, lay::Dispatcher *root);
  ~EditorOptionsPages ();

  void unregister_page (edt::EditorOptionsPage *page);
  void activate_page (edt::EditorOptionsPage *page);

  const std::vector <edt::EditorOptionsPage *> &pages () const
  {
    return m_pages;
  }

public slots:
  void apply ();
  void setup ();

private:
  std::vector <edt::EditorOptionsPage *> m_pages;
  lay::Dispatcher *mp_dispatcher;
  QTabWidget *mp_pages;

  void update (edt::EditorOptionsPage *page);
  void do_apply ();
};

/**
 *  @brief The generic properties page
 */
class EditorOptionsGeneric
  : public QWidget, public EditorOptionsPage
{
Q_OBJECT

public:
  EditorOptionsGeneric ();
  ~EditorOptionsGeneric ();

  virtual QWidget *q_frame () { return this; }

  virtual std::string title () const;
  virtual int order () const { return 0; }
  void apply (lay::Plugin *root);
  void setup (lay::Plugin *root);

public slots:
  void grid_changed (int);
  void show_shapes_changed ();

private:
  Ui::EditorOptionsGeneric *mp_ui;
};

/**
 *  @brief The text properties page
 */
class EditorOptionsText
  : public QWidget, public EditorOptionsPage 
{
public:
  EditorOptionsText ();
  ~EditorOptionsText ();

  virtual QWidget *q_frame () { return this; }

  virtual std::string title () const;
  virtual int order () const { return 10; }
  void apply (lay::Plugin *root);
  void setup (lay::Plugin *root);

private:
  Ui::EditorOptionsText *mp_ui;
};

/**
 *  @brief The path properties page
 */
class EditorOptionsPath
  : public QWidget, public EditorOptionsPage 
{
Q_OBJECT 

public:
  EditorOptionsPath ();
  ~EditorOptionsPath ();

  virtual QWidget *q_frame () { return this; }

  virtual std::string title () const;
  virtual int order () const { return 30; }
  void apply (lay::Plugin *root);
  void setup (lay::Plugin *root);

public slots:
  void type_changed (int);

private:
  Ui::EditorOptionsPath *mp_ui;
};

/**
 *  @brief The instance properties page
 */
class EditorOptionsInst
  : public QWidget, public EditorOptionsPage 
{
Q_OBJECT 

public:
  EditorOptionsInst (lay::Dispatcher *root);
  ~EditorOptionsInst ();

  virtual QWidget *q_frame () { return this; }

  virtual std::string title () const;
  virtual int order () const { return 20; }
  void apply (lay::Plugin *root);
  void setup (lay::Plugin *root);

public slots:
  void array_changed ();
  void browse_cell ();
  void update_pcell_parameters ();
  void library_changed (int index);
  void cell_name_changed (const QString &s);
  void update_cell_edits ();

private:
  Ui::EditorOptionsInst *mp_ui;
  lay::Dispatcher *mp_root;
  edt::PCellParametersPage *mp_pcell_parameters;
  int m_cv_index;

  void update_pcell_parameters (const std::vector <tl::Variant> &parameters);
};

/**
 *  @brief The instance properties page (PCell parameters)
 */
class EditorOptionsInstPCellParam
  : public QWidget, public EditorOptionsPage
{
Q_OBJECT

public:
  EditorOptionsInstPCellParam (lay::Dispatcher *root);
  ~EditorOptionsInstPCellParam ();

  virtual QWidget *q_frame () { return this; }

  virtual std::string title () const;
  virtual int order () const { return 21; }
  void apply (lay::Plugin *root);
  void setup (lay::Plugin *root);

public slots:
  void update_pcell_parameters ();

private:
  Ui::EditorOptionsInstPCellParam *mp_ui;
  lay::Dispatcher *mp_root;
  edt::PCellParametersPage *mp_pcell_parameters;
  int m_cv_index;
  std::string m_lib_name, m_cell_name;

  void update_pcell_parameters (const std::vector <tl::Variant> &parameters);
};

}

#endif

