
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


#include "layLayoutView.h"
#include "laySaveLayoutOptionsDialog.h"
#include "layStream.h"
#include "laybasicConfig.h"
#include "tlExceptions.h"
#include "dbSaveLayoutOptions.h"
#include "dbStream.h"
#include "tlClassRegistry.h"

#include <QScrollArea>
#include <QPushButton>

#include <memory>

namespace lay
{

static const StreamWriterPluginDeclaration *plugin_for_format (const std::string &format_name)
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    const StreamWriterPluginDeclaration *decl = dynamic_cast <const StreamWriterPluginDeclaration *> (&*cls);
    if (decl && decl->format_name () == format_name) {
      return decl;
    }
  }
  return 0;
}

static unsigned int om_to_index (tl::OutputStream::OutputStreamMode om)
{
  if (om == tl::OutputStream::OM_Plain) {
    return 1;
  } else if (om == tl::OutputStream::OM_Zlib) {
    return 2; 
  } else {
    return 0;
  }
}

static tl::OutputStream::OutputStreamMode index_to_om (unsigned int i)
{
  if (i == 1) {
    return tl::OutputStream::OM_Plain;
  } else if (i == 2) {
    return tl::OutputStream::OM_Zlib;
  } else {
    return tl::OutputStream::OM_Auto;
  }
}

// -----------------------------------------------------------------
//  SaveLayoutOptionsDialog implementation

SaveLayoutOptionsDialog::SaveLayoutOptionsDialog (QWidget *parent, const std::string &title)
  : QDialog (parent), Ui::SaveLayoutOptionsDialog (),
    m_technology_index (-1)
{
  setObjectName (QString::fromUtf8 ("save_layout_options_dialog"));

  Ui::SaveLayoutOptionsDialog::setupUi (this);

  setWindowTitle (tl::to_qstring (title));

  while (options_tab->count () > 0) {
    options_tab->removeTab (0);
  }

  bool any_option = false;

  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {

    StreamWriterOptionsPage *page = 0;

    //  obtain the config page from the plugin which we identify by format name
    const StreamWriterPluginDeclaration *decl = StreamWriterPluginDeclaration::plugin_for_format (fmt->format_name ());

    QScrollArea *page_host = new QScrollArea (options_tab);
    page_host->setFrameStyle (QFrame::NoFrame);
    page_host->setWidgetResizable (true);

    page = decl ? decl->format_specific_options_page (options_tab) : 0;
    if (page) {
      page_host->setWidget (page);
    } else {
#if 0
      //  Show an empty page
      QLabel *empty = new QLabel (options_tab);
      empty->setAlignment (Qt::AlignCenter);
      empty->setText (QObject::tr ("No specific options available for this format"));
      page_host->setWidget (empty);
#else
      //  Drop empty pages
      delete page_host;
      page_host = 0;
#endif
    }

    if (page_host) {
      options_tab->addTab (page_host, tl::to_qstring (fmt->format_desc ()));
      m_pages.push_back (std::make_pair (page, fmt->format_name ()));
      any_option = true;
    }

  }

  if (! any_option) {
    options_tab->hide ();
  }

  connect (buttonBox, SIGNAL (accepted ()), this, SLOT (ok_button_pressed ()));
  connect (buttonBox, SIGNAL (clicked (QAbstractButton *)), this, SLOT (button_pressed (QAbstractButton *)));
  connect (tech_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (current_tech_changed (int)));
}

SaveLayoutOptionsDialog::~SaveLayoutOptionsDialog ()
{
  // .. nothing yet ..
}

void
SaveLayoutOptionsDialog::button_pressed (QAbstractButton *button)
{
  if (button == buttonBox->button (QDialogButtonBox::Reset)) {
    reset_button_pressed ();
  }
}

void
SaveLayoutOptionsDialog::current_tech_changed (int index)
{
  if (index != m_technology_index) {
    commit ();
    m_technology_index = index;
    update ();
  }
}

void
SaveLayoutOptionsDialog::reset_button_pressed ()
{
  BEGIN_PROTECTED

  if (m_technology_index >= 0) {
    m_opt_array[m_technology_index] = db::SaveLayoutOptions ();
  }
  update ();

  END_PROTECTED
}

void
SaveLayoutOptionsDialog::ok_button_pressed ()
{
  BEGIN_PROTECTED

  commit ();
  accept ();

  END_PROTECTED
}

void
SaveLayoutOptionsDialog::commit ()
{
  if (m_technology_index < 0) {
    return;
  }

  //  create the particular options for all formats
  for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {

    if (page->first) {

      db::FormatSpecificWriterOptions *specific_options = m_opt_array [m_technology_index].get_options (page->second);
      if (! specific_options) {
        //  Create a container for the options unless there is one already
        specific_options = StreamWriterPluginDeclaration::plugin_for_format (page->second)->create_specific_options ();
        m_opt_array [m_technology_index].set_options (specific_options);
      }

      page->first->commit (specific_options, m_tech_array [m_technology_index], false);

    }

  }
}

void
SaveLayoutOptionsDialog::update ()
{
  if (m_technology_index < 0) {
    return;
  }

  for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      db::FormatSpecificWriterOptions *specific_options = m_opt_array [m_technology_index].get_options (page->second);
      if (! specific_options) {
        //  Create a container for the options unless there is one already
        std::auto_ptr<db::FormatSpecificWriterOptions> new_options (StreamWriterPluginDeclaration::plugin_for_format (page->second)->create_specific_options ());
        page->first->setup (new_options.get (), m_tech_array [m_technology_index]);
      } else {
        page->first->setup (specific_options, m_tech_array [m_technology_index]);
      }
    }
  }
}

bool
SaveLayoutOptionsDialog::edit_global_options (lay::Dispatcher *config_root, db::Technologies *technologies)
{
  m_opt_array.clear ();
  m_tech_array.clear ();

  std::string technology;
  config_root->config_get (cfg_initial_technology, technology);

  tech_cbx->blockSignals (true);
  tech_cbx->clear ();

  unsigned int i = 0;
  m_technology_index = -1;

  for (db::Technologies::const_iterator t = technologies->begin (); t != technologies->end (); ++t, ++i) {

    std::string d = t->name ();
    if (! d.empty () && ! t->description ().empty ()) {
      d += " - ";
    }
    d += t->description ();

    m_opt_array.push_back (t->save_layout_options ());
    m_tech_array.push_back (t.operator-> ());

    tech_cbx->addItem (tl::to_qstring (d));
    if (t->name () == technology) {
      tech_cbx->setCurrentIndex (i);
      m_technology_index = i;
    }

  }

  tech_cbx->blockSignals (false);
  tech_cbx->show ();

  if (get_options_internal ()) {

    //  get the selected technology name and store in the config
    unsigned int i = 0;
    for (db::Technologies::iterator t = technologies->begin (); t != technologies->end () && i < m_opt_array.size (); ++t, ++i) {
      technologies->begin ()[i].set_save_layout_options (m_opt_array [i]);
    }

    //  TODO: this call is required currently because otherwise the technology
    //  management subsystem does not notice the changes of technologies.
    technologies->notify_technologies_changed ();

    return true;

  } else {
    return false;
  }
}

bool
SaveLayoutOptionsDialog::get_options (db::SaveLayoutOptions &options)
{
  tech_cbx->hide ();

  m_opt_array.clear ();
  m_opt_array.push_back (options);
  m_tech_array.clear ();
  m_tech_array.push_back (0);
  m_technology_index = 0;

  if (get_options_internal ()) {
    options = m_opt_array.front ();
    return true;
  } else {
    return false;
  }
}

bool
SaveLayoutOptionsDialog::get_options_internal ()
{
  update ();
  if (exec ()) {
    commit ();
    return true;
  } else {
    return false;
  }
}


// -----------------------------------------------------------------
//  SaveLayoutAsOptionsDialog implementation

SaveLayoutAsOptionsDialog::SaveLayoutAsOptionsDialog (QWidget *parent, const std::string &title)
  : QDialog (parent), Ui::SaveLayoutAsOptionsDialog (), mp_tech (0)
{
  setObjectName (QString::fromUtf8 ("save_layout_options_dialog"));

  Ui::SaveLayoutAsOptionsDialog::setupUi (this);

  setWindowTitle (tl::to_qstring (title));

  QWidget *empty_widget = new QWidget (options_stack);
  int empty_widget_index = options_stack->addWidget (empty_widget);

  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {

    if (fmt->can_write ()) {

      fmt_cbx->addItem (tl::to_qstring (fmt->format_title ()));

      StreamWriterOptionsPage *page = 0;

      //  obtain the config page from the plugin which we identify by format name
      const StreamWriterPluginDeclaration *decl = plugin_for_format (fmt->format_name ());
      if (decl) {
        page = decl->format_specific_options_page (options_stack);
      }

      m_pages.push_back (std::make_pair (page, fmt->format_name ()));
      m_tab_positions.push_back (page ? options_stack->addWidget (page) : empty_widget_index);

    }

  }

  connect (buttonBox, SIGNAL (accepted ()), this, SLOT (ok_button_pressed ()));
  connect (fmt_cbx, SIGNAL (activated (int)), this, SLOT (fmt_cbx_changed (int)));
}

SaveLayoutAsOptionsDialog::~SaveLayoutAsOptionsDialog ()
{
  //  .. nothing yet ..
}

void
SaveLayoutAsOptionsDialog::ok_button_pressed ()
{
  BEGIN_PROTECTED

  //  get the name of the currently selected format 
  int index = fmt_cbx->currentIndex ();
  std::string fmt_name;
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end () && index >= 0; ++fmt) {
    if (fmt->can_write ()) {
      if (index-- == 0) {
        fmt_name = fmt->format_name ();
        break;
      }
    }
  }

  //  test-commit the page for the current format
  const StreamWriterPluginDeclaration *decl = plugin_for_format (fmt_name);
  if (decl) {
    for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
      if (page->second == fmt_name) {
        if (page->first) {
          std::auto_ptr<db::FormatSpecificWriterOptions> options (decl->create_specific_options ());
          if (options.get ()) {
            page->first->commit (options.get (), mp_tech, tl::OutputStream::output_mode_from_filename (m_filename, index_to_om (compression->currentIndex ())) != tl::OutputStream::OM_Plain);
          }
        }
        break;
      }
    }
  }

  double x = 0.0;
  tl::from_string (tl::to_string (dbu_le->text ()), x);
  tl::from_string (tl::to_string (sf_le->text ()), x);

  accept ();

  END_PROTECTED
}

bool 
SaveLayoutAsOptionsDialog::get_options (lay::LayoutView *view, unsigned int cv_index, const std::string &fn, tl::OutputStream::OutputStreamMode &om, db::SaveLayoutOptions &options)
{
  const lay::CellView &cv = view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return false;
  }

  mp_tech = cv->technology ();

  const db::Layout &layout = cv->layout ();

  m_filename = fn;
  filename_lbl->setText (tl::to_qstring (fn));
  compression->setCurrentIndex (om_to_index (om));

  dbu_le->setText (tl::to_qstring (tl::to_string (options.dbu ())));

  fmt_cbx->setCurrentIndex (0);
  fmt_cbx_changed (0);

  unsigned int i = 0;
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {
    if (fmt->can_write ()) {
      if (fmt->format_name () == options.format ()) {
        fmt_cbx->setCurrentIndex (i);
        fmt_cbx_changed (i);
        break;
      }
      ++i;
    }
  }

  for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {

    const StreamWriterPluginDeclaration *decl = plugin_for_format (page->second);
    if (decl) {

      std::auto_ptr<db::FormatSpecificWriterOptions> specific_options;
      if (options.get_options (page->second)) {
        specific_options.reset (options.get_options (page->second)->clone ());
      } else {
        specific_options.reset (decl->create_specific_options ());
      }

      decl->initialize_options_from_layout_handle (specific_options.get (), *cv.handle ());

      if (page->first) {
        page->first->setup (specific_options.get (), mp_tech);
      }

    }

  }

  bool ret = false;

  if (exec ()) {

    om = index_to_om (compression->currentIndex ());

    int index = fmt_cbx->currentIndex ();
    for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end () && index >= 0; ++fmt) {
      if (fmt->can_write ()) {
        if (index-- == 0) {
          options.set_format (fmt->format_name ());
        }
      }
    }

    double dbu = 0.0;
    tl::from_string (tl::to_string (dbu_le->text ()), dbu);

    double sf = 1.0;
    tl::from_string (tl::to_string (sf_le->text ()), sf);

    options.set_dbu (dbu);
    options.set_scale_factor (sf);

    options.set_dont_write_empty_cells (no_empty_cells_cb->isChecked ());
    options.set_keep_instances (keep_instances_cb->isChecked ());
    options.set_write_context_info (store_context_cb->isChecked ());

    if (no_hidden_cells_cb->isChecked ()) {
      options.clear_cells ();
      for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
        if (! view->is_cell_hidden (c->cell_index (), cv_index)) {
          options.add_this_cell (c->cell_index ());
        }
      }
    } else {
      options.select_all_cells ();
    }

    if (layersel_cbx->currentIndex () == 0 /*all*/) {
      options.select_all_layers ();
    } else if (layersel_cbx->currentIndex () == 1 /*shown layers*/) {
      options.deselect_all_layers ();
      for (LayerPropertiesConstIterator layer = view->begin_layers (); layer != view->end_layers (); ++layer) {
        if (layer->cellview_index () == int (cv_index)) {
          options.add_layer (layer->layer_index ());
        }
      }
    } else if (layersel_cbx->currentIndex () == 2 /*visible layers*/) {
      options.deselect_all_layers ();
      for (LayerPropertiesConstIterator layer = view->begin_layers (); layer != view->end_layers (); ++layer) {
        if (layer->cellview_index () == int (cv_index) && layer->visible (true)) {
          options.add_layer (layer->layer_index ());
        }
      }
    }

    //  identify the plugin in charge of creating the particular options and commit only those options
    const StreamWriterPluginDeclaration *decl = plugin_for_format (options.format ());
    if (decl) {
      for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
        if (page->second == options.format ()) {

          std::auto_ptr<db::FormatSpecificWriterOptions> specific_options;
          specific_options.reset (decl->create_specific_options ());

          if (specific_options.get ()) {
            if (page->first) {
              page->first->commit (specific_options.get (), mp_tech, tl::OutputStream::output_mode_from_filename (m_filename, index_to_om (compression->currentIndex ())) != tl::OutputStream::OM_Plain);
            }
            options.set_options (specific_options.release ());
          }

        }
      }
    }

    ret = true;

  }

  return ret;
}

void 
SaveLayoutAsOptionsDialog::fmt_cbx_changed (int index)
{
  if (index >= 0 && index < int (m_tab_positions.size ())) {
    options_stack->setCurrentIndex (m_tab_positions[index]);
  }
}

}

