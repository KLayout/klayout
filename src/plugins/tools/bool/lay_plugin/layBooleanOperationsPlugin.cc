
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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



#include "layBooleanOperationsDialogs.h"

#include "layPlugin.h"
#include "layTipDialog.h"
#include "layLayoutViewBase.h"
#include "layMainWindow.h"

#include "dbShapeProcessor.h"

#include <QApplication>

namespace lay
{

static QWidget *parent_widget ()
{
  return QApplication::activeWindow ();
}

class BooleanOperationsPlugin
  : public lay::Plugin
{
public:
  BooleanOperationsPlugin (lay::LayoutViewBase *view)
    : lay::Plugin (view), mp_view (view)
  {
    m_boolean_cva = -1;
    m_boolean_cvb = -1;
    m_boolean_cvr = -1;
    m_boolean_layera = -1;
    m_boolean_layerb = -1;
    m_boolean_layerr = -1;
    m_boolean_hier_mode = 0;
    m_boolean_mode = 0;
    m_boolean_mincoh = true;
    m_boolean_minwc = 0;
    m_boolean_sizex = m_boolean_sizey = 0.0;
    m_boolean_size_mode = 2;
  }

  ~BooleanOperationsPlugin ()
  {
    // ...
  }

  void menu_activated (const std::string &symbol) 
  {
    if (symbol == "lay::boolean") {
      boolean ();
    } else if (symbol == "lay::merge") {
      merge ();
    } else if (symbol == "lay::size") {
      size ();
    }
  }

  void boolean ()
  {
    struct { int *cv; int *layer; } specs [] = {
      { &m_boolean_cva, &m_boolean_layera },
      { &m_boolean_cvb, &m_boolean_layerb },
      { &m_boolean_cvr, &m_boolean_layerr }
    };

    for (unsigned int i = 0; i < sizeof (specs) / sizeof (specs[0]); ++i) {

      int &cv = *(specs[i].cv);
      int &layer = *(specs[i].layer);

      if (cv >= int (mp_view->cellviews ())) {
        cv = -1;
      }

      int index = mp_view->active_cellview_index ();
      if (cv < 0) {
        cv = index;
      }

      if (cv < 0 || ! mp_view->cellview (cv)->layout ().is_valid_layer ((unsigned int) layer)) {
        layer = -1;
      }

    }

    lay::BooleanOptionsDialog dialog (parent_widget ());
    if (dialog.exec_dialog (mp_view, m_boolean_cva, m_boolean_layera, m_boolean_cvb, m_boolean_layerb, m_boolean_cvr, m_boolean_layerr, m_boolean_mode, m_boolean_hier_mode, m_boolean_mincoh)) {

      mp_view->cancel ();

      bool supports_undo = true;

      if (mp_view->manager () && mp_view->manager ()->is_enabled ()) {

        lay::TipDialog td (QApplication::activeWindow (),
                           tl::to_string (QObject::tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")), 
                           "boolean-undo-buffering",
                           lay::TipDialog::yesnocancel_buttons);

        lay::TipDialog::button_type button = lay::TipDialog::null_button;
        td.exec_dialog (button);
        if (button == lay::TipDialog::cancel_button) {
          return;
        }

        supports_undo = (button == lay::TipDialog::yes_button);

      } else {
        supports_undo = false;
      }

      if (mp_view->manager ()) {
        if (! supports_undo) {
          mp_view->manager ()->clear ();
        } else {
          mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Boolean operation"))); 
        }
      }

      try {

        db::BooleanOp::BoolOp op_mode;
        switch (m_boolean_mode) {
        default: 
          op_mode = db::BooleanOp::Or; 
          break;
        case 1: 
          op_mode = db::BooleanOp::And; 
          break;
        case 2: 
          op_mode = db::BooleanOp::ANotB; 
          break;
        case 3: 
          op_mode = db::BooleanOp::BNotA; 
          break;
        case 4: 
          op_mode = db::BooleanOp::Xor; 
          break;
        }

        if (m_boolean_hier_mode == 0) {

          //  flat mode
          db::ShapeProcessor p (true);
          p.boolean (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                     mp_view->cellview (m_boolean_cvb)->layout (), *mp_view->cellview (m_boolean_cvb).cell (), m_boolean_layerb,
                     mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), op_mode, true, true, m_boolean_mincoh);
          
          //  clear the result layer for all called cells in flat mode
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cvr).cell ()->collect_called_cells (called_cells);
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            mp_view->cellview (m_boolean_cvr)->layout ().cell (*c).shapes (m_boolean_layerr).clear ();
          }

        } else if (m_boolean_hier_mode == 1) {

          //  top cell only mode
          db::ShapeProcessor p (true);
          p.boolean (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                     mp_view->cellview (m_boolean_cvb)->layout (), *mp_view->cellview (m_boolean_cvb).cell (), m_boolean_layerb,
                     mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), op_mode, false, true, m_boolean_mincoh);
          
        } else if (m_boolean_hier_mode == 2) {

          //  subcells cell by cell
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cva).cell ()->collect_called_cells (called_cells);
          called_cells.insert (mp_view->cellview (m_boolean_cva).cell_index ());

          db::ShapeProcessor p (true);
          db::Layout &layout = mp_view->cellview (m_boolean_cva)->layout ();
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            db::Cell &cell = layout.cell (*c);
            p.boolean (layout, cell, m_boolean_layera,
                       layout, cell, m_boolean_layerb,
                       cell.shapes (m_boolean_layerr), op_mode, false, true, m_boolean_mincoh);
          }

        }

        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }

      } catch (...) {
        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }
        throw;
      }

    }
  }

  void merge ()
  {
    struct { int *cv; int *layer; } specs [] = {
      { &m_boolean_cva, &m_boolean_layera },
      { &m_boolean_cvr, &m_boolean_layerr }
    };

    for (unsigned int i = 0; i < sizeof (specs) / sizeof (specs[0]); ++i) {

      int &cv = *(specs[i].cv);
      int &layer = *(specs[i].layer);

      if (cv >= int (mp_view->cellviews ())) {
        cv = -1;
      }

      int index = mp_view->active_cellview_index ();
      if (cv < 0) {
        cv = index;
      }

      if (cv < 0 || ! mp_view->cellview (cv)->layout ().is_valid_layer ((unsigned int) layer)) {
        layer = -1;
      }

    }

    lay::MergeOptionsDialog dialog (parent_widget ());
    if (dialog.exec_dialog (mp_view, m_boolean_cva, m_boolean_layera, m_boolean_cvr, m_boolean_layerr, m_boolean_minwc, m_boolean_hier_mode, m_boolean_mincoh)) {

      mp_view->cancel ();

      bool supports_undo = true;

      if (mp_view->manager () && mp_view->manager ()->is_enabled ()) {

        lay::TipDialog td (QApplication::activeWindow (),
                           tl::to_string (QObject::tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")), 
                           "merge-undo-buffering",
                           lay::TipDialog::yesnocancel_buttons);

        lay::TipDialog::button_type button = lay::TipDialog::null_button;
        td.exec_dialog (button);
        if (button == lay::TipDialog::cancel_button) {
          return;
        }

        supports_undo = (button == lay::TipDialog::yes_button);

      } else {
        supports_undo = false;
      }

      if (mp_view->manager ()) {
        if (! supports_undo) {
          mp_view->manager ()->clear ();
        } else {
          mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Merge operation"))); 
        }
      }

      try {

        if (m_boolean_hier_mode == 0) {

          //  flat mode
          db::ShapeProcessor p (true);
          p.merge (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                   mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), true, m_boolean_minwc, true, m_boolean_mincoh);
          
          //  clear the result layer for all called cells in flat mode
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cvr).cell ()->collect_called_cells (called_cells);
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            mp_view->cellview (m_boolean_cvr)->layout ().cell (*c).shapes (m_boolean_layerr).clear ();
          }

        } else if (m_boolean_hier_mode == 1) {

          //  top cell only mode
          db::ShapeProcessor p (true);
          p.merge (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                   mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), false, m_boolean_minwc, true, m_boolean_mincoh);
          
        } else if (m_boolean_hier_mode == 2) {

          //  subcells cell by cell
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cva).cell ()->collect_called_cells (called_cells);
          called_cells.insert (mp_view->cellview (m_boolean_cva).cell_index ());

          db::ShapeProcessor p (true);
          db::Layout &layout = mp_view->cellview (m_boolean_cva)->layout ();
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            db::Cell &cell = layout.cell (*c);
            p.merge (layout, cell, m_boolean_layera,
                     cell.shapes (m_boolean_layerr), false, m_boolean_minwc, true, m_boolean_mincoh);
          }

        }

        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }

      } catch (...) {
        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }
        throw;
      }

    }
  }

  void size ()
  {
    struct { int *cv; int *layer; } specs [] = {
      { &m_boolean_cva, &m_boolean_layera },
      { &m_boolean_cvr, &m_boolean_layerr }
    };

    for (unsigned int i = 0; i < sizeof (specs) / sizeof (specs[0]); ++i) {

      int &cv = *(specs[i].cv);
      int &layer = *(specs[i].layer);

      if (cv >= int (mp_view->cellviews ())) {
        cv = -1;
      }

      int index = mp_view->active_cellview_index ();
      if (cv < 0) {
        cv = index;
      }

      if (cv < 0 || ! mp_view->cellview (cv)->layout ().is_valid_layer ((unsigned int) layer)) {
        layer = -1;
      }

    }

    lay::SizingOptionsDialog dialog (parent_widget ());
    if (dialog.exec_dialog (mp_view, m_boolean_cva, m_boolean_layera, m_boolean_cvr, m_boolean_layerr, m_boolean_sizex, m_boolean_sizey, m_boolean_size_mode, m_boolean_hier_mode, m_boolean_mincoh)) {

      mp_view->cancel ();

      bool supports_undo = true;

      if (mp_view->manager () && mp_view->manager ()->is_enabled ()) {

        lay::TipDialog td (QApplication::activeWindow (),
                           tl::to_string (QObject::tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")), 
                           "size-undo-buffering",
                           lay::TipDialog::yesnocancel_buttons);

        lay::TipDialog::button_type button = lay::TipDialog::null_button;
        td.exec_dialog (button);
        if (button == lay::TipDialog::cancel_button) {
          return;
        }

        supports_undo = (button == lay::TipDialog::yes_button);

      } else {
        supports_undo = false;
      }

      db::Coord dx_int, dy_int;
      dx_int = db::coord_traits<db::Coord>::rounded (m_boolean_sizex / mp_view->cellview (m_boolean_cva)->layout ().dbu ());
      dy_int = db::coord_traits<db::Coord>::rounded (m_boolean_sizey / mp_view->cellview (m_boolean_cva)->layout ().dbu ());

      if (mp_view->manager ()) {
        if (! supports_undo) {
          mp_view->manager ()->clear ();
        } else {
          mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Sizing operation"))); 
        }
      }

      try {

        if (m_boolean_hier_mode == 0) {

          //  flat mode
          db::ShapeProcessor p (true);
          p.size (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                  mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), dx_int, dy_int, m_boolean_size_mode, true, true, m_boolean_mincoh);
          
          //  clear the result layer for all called cells in flat mode
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cvr).cell ()->collect_called_cells (called_cells);
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            mp_view->cellview (m_boolean_cvr)->layout ().cell (*c).shapes (m_boolean_layerr).clear ();
          }

        } else if (m_boolean_hier_mode == 1) {

          //  top cell only mode
          db::ShapeProcessor p (true);
          p.size (mp_view->cellview (m_boolean_cva)->layout (), *mp_view->cellview (m_boolean_cva).cell (), m_boolean_layera,
                  mp_view->cellview (m_boolean_cvr).cell ()->shapes (m_boolean_layerr), dx_int, dy_int, m_boolean_size_mode, false, true, m_boolean_mincoh);
          
        } else if (m_boolean_hier_mode == 2) {

          //  subcells cell by cell
          std::set<db::cell_index_type> called_cells;
          mp_view->cellview (m_boolean_cva).cell ()->collect_called_cells (called_cells);
          called_cells.insert (mp_view->cellview (m_boolean_cva).cell_index ());

          db::ShapeProcessor p (true);
          db::Layout &layout = mp_view->cellview (m_boolean_cva)->layout ();
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            db::Cell &cell = layout.cell (*c);
            p.size (layout, cell, m_boolean_layera,
                    cell.shapes (m_boolean_layerr), dx_int, dy_int, m_boolean_size_mode, false, true, m_boolean_mincoh);
          }

        }

        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }

      } catch (...) {
        if (supports_undo && mp_view->manager ()) {
          mp_view->manager ()->commit ();
        }
        throw;
      }

    }
  }

private:
  lay::LayoutViewBase *mp_view;
  int m_boolean_cva, m_boolean_cvb, m_boolean_cvr;
  int m_boolean_layera, m_boolean_layerb, m_boolean_layerr;
  int m_boolean_hier_mode, m_boolean_mode;
  bool m_boolean_mincoh;
  unsigned int m_boolean_minwc;
  double m_boolean_sizex, m_boolean_sizey;
  unsigned int m_boolean_size_mode;
};

class BooleanOperationsPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  BooleanOperationsPluginDeclaration ()
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
  {
    //  .. nothing yet ..
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::separator ("ops_group", "edit_menu.layer_menu.end"));
    menu_entries.push_back (lay::menu_item ("lay::boolean", "boolean:edit:edit_mode", "edit_menu.layer_menu.end", tl::to_string (QObject::tr ("Boolean Operations"))));
    menu_entries.push_back (lay::menu_item ("lay::merge", "merge:edit:edit_mode", "edit_menu.layer_menu.end", tl::to_string (QObject::tr ("Merge"))));
    menu_entries.push_back (lay::menu_item ("lay::size", "size:edit:edit_mode", "edit_menu.layer_menu.end", tl::to_string (QObject::tr ("Size"))));
  }

  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  virtual void config_finalize ()
  {
    // .. nothing yet ..
  }

  lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *view) const
  {
    return new BooleanOperationsPlugin (view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::BooleanOperationsPluginDeclaration (), 3010, "lay::BooleanOperationsPlugin");

}

