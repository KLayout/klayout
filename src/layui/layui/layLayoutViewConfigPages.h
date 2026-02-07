
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

#ifndef HDR_layLayoutViewConfigPages
#define HDR_layLayoutViewConfigPages

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layLineStylePalette.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "dbObject.h"

namespace Ui {
  class LayoutViewConfigPage;
  class LayoutViewConfigPage1;
  class LayoutViewConfigPage2a;
  class LayoutViewConfigPage2b;
  class LayoutViewConfigPage2c;
  class LayoutViewConfigPage2d;
  class LayoutViewConfigPage3a;
  class LayoutViewConfigPage3b;
  class LayoutViewConfigPage3c;
  class LayoutViewConfigPage3f;
  class LayoutViewConfigPage4;
  class LayoutViewConfigPage5;
  class LayoutViewConfigPage6;
  class LayoutViewConfigPage6a;
  class LayoutViewConfigPage7;
  class LayoutViewConfigPage8;
}

namespace lay
{

class ColorButton;

// -------------------------------------------------------------
//  This file provides the main declarations for the configuration
//  dialogs for the layout view.

class LayoutViewConfigPage 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage (QWidget *parent);
  ~LayoutViewConfigPage ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage *mp_ui;
};

class LayoutViewConfigPage1 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage1 (QWidget *parent);
  ~LayoutViewConfigPage1 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage1 *mp_ui;
};

class LayoutViewConfigPage2a
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage2a (QWidget *parent);
  ~LayoutViewConfigPage2a ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage2a *mp_ui;
};

class LayoutViewConfigPage2b
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage2b (QWidget *parent);
  ~LayoutViewConfigPage2b ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage2b *mp_ui;
};

class LayoutViewConfigPage2c 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage2c (QWidget *parent);
  ~LayoutViewConfigPage2c ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage2c *mp_ui;
};

class LayoutViewConfigPage2d
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage2d (QWidget *parent);
  ~LayoutViewConfigPage2d ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage2d *mp_ui;
};

class LayoutViewConfigPage3a
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage3a (QWidget *parent);
  ~LayoutViewConfigPage3a ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage3a *mp_ui;
};

class LayoutViewConfigPage3b 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage3b (QWidget *parent);
  ~LayoutViewConfigPage3b ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage3b *mp_ui;
};

class LayoutViewConfigPage3c 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage3c (QWidget *parent);
  ~LayoutViewConfigPage3c ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage3c *mp_ui;
};

class LayoutViewConfigPage3f 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage3f (QWidget *parent);
  ~LayoutViewConfigPage3f ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage3f *mp_ui;
};

class LayoutViewConfigPage4 
  : public lay::ConfigPage, 
    public db::Object
{
Q_OBJECT

public:
  LayoutViewConfigPage4 (QWidget *parent);
  ~LayoutViewConfigPage4 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

  virtual void undo (db::Op *op);
  virtual void redo (db::Op *op);

public slots:
  void color_button_clicked ();
  void undo_button_clicked ();
  void redo_button_clicked ();
  void reset_button_clicked ();
  void edit_order_changed (int s);
  
private:
  Ui::LayoutViewConfigPage4 *mp_ui;
  lay::ColorPalette m_palette;
  db::Manager m_manager;
  bool m_edit_order_changed_disabled;

  void update ();
  void set_edit_order (bool edit_order);
};

class LayoutViewConfigPage5 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage5 (QWidget *parent);
  ~LayoutViewConfigPage5 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void browse_clicked ();

private:
  Ui::LayoutViewConfigPage5 *mp_ui;
};

class LayoutViewConfigPage6 
  : public lay::ConfigPage, 
    public db::Object
{
Q_OBJECT

public:
  LayoutViewConfigPage6 (QWidget *parent);
  ~LayoutViewConfigPage6 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

  virtual void undo (db::Op *op);
  virtual void redo (db::Op *op);

public slots:
  void stipple_button_clicked ();
  void undo_button_clicked ();
  void redo_button_clicked ();
  void reset_button_clicked ();
  void edit_order_changed (int s);
  
private:
  Ui::LayoutViewConfigPage6 *mp_ui;
  lay::StipplePalette m_palette;
  db::Manager m_manager;
  bool m_edit_order_changed_disabled;
  lay::DitherPattern m_pattern;

  void update ();
  void set_edit_order (bool edit_order);
};

class LayoutViewConfigPage6a
  : public lay::ConfigPage,
    public db::Object
{
Q_OBJECT

public:
  LayoutViewConfigPage6a (QWidget *parent);
  ~LayoutViewConfigPage6a ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

  virtual void undo (db::Op *op);
  virtual void redo (db::Op *op);

public slots:
  void line_style_button_clicked ();
  void undo_button_clicked ();
  void redo_button_clicked ();
  void reset_button_clicked ();

private:
  Ui::LayoutViewConfigPage6a *mp_ui;
  lay::LineStylePalette m_palette;
  db::Manager m_manager;
  lay::LineStyles m_style;

  void update ();
  void set_edit_order (bool edit_order);
};

class LayoutViewConfigPage7
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage7 (QWidget *parent);
  ~LayoutViewConfigPage7 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage7 *mp_ui;
};

class LayoutViewConfigPage8
  : public lay::ConfigPage
{
Q_OBJECT

public:
  LayoutViewConfigPage8 (QWidget *parent);
  ~LayoutViewConfigPage8 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::LayoutViewConfigPage8 *mp_ui;
};

}

#endif

#endif  //  defined(HAVE_QT)
