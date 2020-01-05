
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


#ifndef HDR_layPropertiesDialog
#define HDR_layPropertiesDialog

#include <vector>

#include <QMutex>
#include <QDialog>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>

#include "ui_PropertiesDialog.h"

namespace db
{
  class Manager;
}

class QStackedLayout;

namespace lay
{

class Editable;
class Editables;
class PropertiesPage;
class MainWindow;

/**
 *  @brief The properties dialog
 *
 *  This is the implementation of the properties dialog that
 *  is opened to edit or view the properties of a set of selected
 *  objects from a set of editables.
 */

class PropertiesDialog
  : public QDialog, private Ui::PropertiesDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  PropertiesDialog (QWidget *parent, db::Manager *manager, lay::Editables *editables);

  /**
   *  @brief The Destructor
   */
  ~PropertiesDialog ();

private:
  std::vector <lay::PropertiesPage *> mp_properties_pages;
  db::Manager *mp_manager;
  lay::Editables *mp_editables;
  int m_index;
  QStackedLayout *mp_stack;
  lay::MainWindow *mp_mw;
  size_t m_objects, m_current_object;

  void disconnect ();
  bool any_prev () const;
  bool any_next () const;
  void update_title ();

public slots:
  void next_pressed ();
  void prev_pressed ();
  void apply_pressed ();
  void apply_to_all_pressed ();
  void cancel_pressed ();
  void ok_pressed ();

protected:
  void reject ();
};

}

#endif

