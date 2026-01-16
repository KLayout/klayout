
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

#ifndef HDR_edtEditorOptionsPages
#define HDR_edtEditorOptionsPages

#include "layEditorOptionsPage.h"
#include "layEditorOptionsPageWidget.h"

#include <tlVariant.h>

#include <QFrame>
#include <vector>
#include <string>

class QTabWidget;
class QLabel;
class QHBoxLayout;

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
  class LayoutViewBase;
  class Plugin;
  class DecoratedLineEdit;
}

namespace edt
{

class PCellParametersPage;

/**
 *  @brief The generic properties page
 */
class EditorOptionsGeneric
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT

public:
  EditorOptionsGeneric (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  ~EditorOptionsGeneric ();

  virtual std::string title () const;
  virtual int order () const { return 0; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

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
  : public lay::EditorOptionsPageWidget
{
public:
  EditorOptionsText (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  ~EditorOptionsText ();

  virtual std::string title () const;
  virtual int order () const { return 10; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

private:
  Ui::EditorOptionsText *mp_ui;
};

/**
 *  @brief The path properties page
 */
class EditorOptionsPath
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT 

public:
  EditorOptionsPath (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  ~EditorOptionsPath ();

  virtual std::string title () const;
  virtual int order () const { return 30; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

public slots:
  void type_changed (int);

private:
  Ui::EditorOptionsPath *mp_ui;
};

/**
 *  @brief The instance properties page
 */
class EditorOptionsInst
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT 

public:
  EditorOptionsInst (lay::LayoutViewBase *view, lay::Dispatcher *root);
  ~EditorOptionsInst ();

  virtual std::string title () const;
  virtual int order () const { return 20; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

private slots:
  void array_changed ();
  void browse_cell ();
  void library_changed ();
  void update_cell_edits ();

private:
  Ui::EditorOptionsInst *mp_ui;
  edt::PCellParametersPage *mp_pcell_parameters;
  int m_cv_index;

  virtual void technology_changed (const std::string &);
  virtual void active_cellview_changed ();
};

/**
 *  @brief The instance properties page (PCell parameters)
 */
class EditorOptionsInstPCellParam
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT

public:
  EditorOptionsInstPCellParam (lay::LayoutViewBase *view, lay::Dispatcher *root);
  ~EditorOptionsInstPCellParam ();

  virtual std::string title () const;
  virtual int order () const { return 21; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

private slots:
  void update_pcell_parameters ();

private:
  Ui::EditorOptionsInstPCellParam *mp_ui;
  edt::PCellParametersPage *mp_pcell_parameters;
  QLabel *mp_placeholder_label;
  int m_cv_index;
  std::string m_lib_name, m_cell_name;

  void update_pcell_parameters (const std::vector <tl::Variant> &parameters);
  virtual void technology_changed (const std::string &);
};

/**
 *  @brief The toolbox widget for boxes
 */
class BoxToolkitWidget
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT

public:
  BoxToolkitWidget (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  ~BoxToolkitWidget ();

  virtual std::string title () const;
  virtual int order () const { return 0; }
  virtual void configure (const std::string &name, const std::string &value);
  virtual void commit (lay::Dispatcher *root);
  virtual void deactivated ();

private:
  QHBoxLayout *mp_layout;
  lay::DecoratedLineEdit *mp_x_le, *mp_y_le;
};

}

#endif

#endif
