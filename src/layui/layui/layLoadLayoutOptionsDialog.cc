
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

#include "layLayoutViewBase.h"
#include "layLoadLayoutOptionsDialog.h"
#include "layStream.h"
#include "layFileDialog.h"
#include "laybasicConfig.h"
#include "dbLoadLayoutOptions.h"
#include "dbStream.h"
#include "tlClassRegistry.h"
#include "tlExceptions.h"

#include "ui_LoadLayoutOptionsDialog.h"
#include "ui_SpecificLoadLayoutOptionsDialog.h"

#include <QScrollArea>
#include <QLabel>
#include <QPushButton>

#include <memory>

namespace lay
{

LoadLayoutOptionsDialog::LoadLayoutOptionsDialog (QWidget *parent, const std::string &title)
  : QDialog (parent),
    m_show_always (false), m_technology_index (-1)
{
  setObjectName (QString::fromUtf8 ("load_layout_options_dialog"));

  mp_ui = new Ui::LoadLayoutOptionsDialog ();
  mp_ui->setupUi (this);

  setWindowTitle (tl::to_qstring (title));

  while (mp_ui->options_tab->count () > 0) {
    mp_ui->options_tab->removeTab (0);
  }

  bool any_option = false;

  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {

    StreamReaderOptionsPage *page = 0;

    //  obtain the config page from the plugin which we identify by format name
    const StreamReaderPluginDeclaration *decl = StreamReaderPluginDeclaration::plugin_for_format (fmt->format_name ());

    QScrollArea *page_host = new QScrollArea (mp_ui->options_tab);
    page_host->setFrameStyle (QFrame::NoFrame);
    page_host->setWidgetResizable (true);

    page = decl ? decl->format_specific_options_page (mp_ui->options_tab) : 0;
    if (page) {
      page_host->setWidget (page);
    } else {
#if 0
      //  Show an empty page
      QLabel *empty = new QLabel (mp_ui->options_tab);
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
      mp_ui->options_tab->addTab (page_host, tl::to_qstring (fmt->format_desc ()));
      m_pages.push_back (std::make_pair (page, fmt->format_name ()));
      any_option = true;
    }

  }

  if (! any_option) {
    mp_ui->options_tab->hide ();
  }

  connect (mp_ui->buttonBox, SIGNAL (accepted ()), this, SLOT (ok_button_pressed ()));
  connect (mp_ui->buttonBox, SIGNAL (clicked (QAbstractButton *)), this, SLOT (button_pressed (QAbstractButton *)));
  connect (mp_ui->tech_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (current_tech_changed (int)));
}

LoadLayoutOptionsDialog::~LoadLayoutOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LoadLayoutOptionsDialog::button_pressed (QAbstractButton *button)
{
  if (button == mp_ui->buttonBox->button (QDialogButtonBox::Reset)) {
    reset_button_pressed ();
  }
}

void
LoadLayoutOptionsDialog::current_tech_changed (int index)
{
  if (index != m_technology_index) {
    commit ();
    m_technology_index = index;
    update ();
  }
}

void
LoadLayoutOptionsDialog::reset_button_pressed ()
{
  BEGIN_PROTECTED

  if (m_technology_index >= 0) {
    m_opt_array[m_technology_index] = db::LoadLayoutOptions ();
  }
  update ();

  END_PROTECTED
}

void
LoadLayoutOptionsDialog::ok_button_pressed ()
{
  BEGIN_PROTECTED

  commit ();
  accept ();

  END_PROTECTED
}

void
LoadLayoutOptionsDialog::commit ()
{
  if (m_technology_index < 0) {
    return;
  }

  //  create the particular options for all formats
  for (std::vector< std::pair<StreamReaderOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      db::FormatSpecificReaderOptions *opt = m_opt_array [m_technology_index].get_options (page->second);
      if (!opt) {
        //  Create the options if not already done.
        const lay::StreamReaderPluginDeclaration *decl = StreamReaderPluginDeclaration::plugin_for_format (page->second);
        if (decl) {
          opt = decl->create_specific_options ();
          m_opt_array [m_technology_index].set_options (opt);
        }
      }
      if (opt) {
        page->first->commit (opt, m_tech_array [m_technology_index]);
      }
    }
  }
}

void 
LoadLayoutOptionsDialog::update ()
{
  if (m_technology_index < 0) {
    return;
  }

  const db::Technology *tech = m_tech_array [m_technology_index];
  mp_ui->options_tab->setEnabled (!tech || tech->is_persisted ());

  for (std::vector< std::pair<StreamReaderOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      page->first->setup (m_opt_array [m_technology_index].get_options (page->second), tech);
    }
  }
}

bool 
LoadLayoutOptionsDialog::edit_global_options (lay::Dispatcher *config_root, db::Technologies *technologies)
{
  m_opt_array.clear ();
  m_tech_array.clear ();

  std::string technology;
  config_root->config_get (cfg_initial_technology, technology);

  try {
    config_root->config_get (cfg_reader_options_show_always, m_show_always);
  } catch (...) {
    m_show_always = false;
  }
  mp_ui->always_cbx->setChecked (m_show_always);
  mp_ui->always_cbx->show ();

  mp_ui->tech_cbx->blockSignals (true);
  mp_ui->tech_cbx->clear ();

  unsigned int i = 0;
  m_technology_index = -1;

  for (db::Technologies::const_iterator t = technologies->begin (); t != technologies->end (); ++t, ++i) {

    std::string d = t->name ();
    if (! d.empty () && ! t->description ().empty ()) {
      d += " - ";
    }
    d += t->description ();

    m_opt_array.push_back (t->load_layout_options ());
    m_tech_array.push_back (t.operator-> ());

    mp_ui->tech_cbx->addItem (tl::to_qstring (d));
    if (t->name () == technology) {
      mp_ui->tech_cbx->setCurrentIndex (i);
      m_technology_index = i;
    }

  }

  mp_ui->tech_cbx->blockSignals (false);
  mp_ui->tech_cbx->show ();
  mp_ui->tech_frame->show ();

  if (get_options_internal ()) {

    //  get the selected technology name and store in the config
    if (m_technology_index >= 0 && m_technology_index < (int) technologies->technologies ()) {
      technology = technologies->begin () [m_technology_index].name ();
    } else {
      technology = std::string ();
    }
    config_root->config_set (cfg_initial_technology, technology);

    m_show_always = mp_ui->always_cbx->isChecked ();
    config_root->config_set (cfg_reader_options_show_always, tl::to_string (m_show_always));

    i = 0;
    technologies->begin_updates ();
    for (db::Technologies::iterator t = technologies->begin (); t != technologies->end () && i < m_opt_array.size (); ++t, ++i) {
      technologies->begin ()[i].set_load_layout_options (m_opt_array [i]);
    }
    technologies->end_updates ();

    return true;

  } else {
    return false;
  }
}

bool 
LoadLayoutOptionsDialog::get_options (db::LoadLayoutOptions &options)
{
  mp_ui->tech_frame->hide ();
  mp_ui->always_cbx->hide ();

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
LoadLayoutOptionsDialog::get_options_internal ()
{
  update ();
  if (exec ()) {
    commit ();
    return true;
  } else {
    return false;
  }
}

// ----------------------------------------------------------------
//  TechComponentSetupDialog implementation

SpecificLoadLayoutOptionsDialog::SpecificLoadLayoutOptionsDialog (QWidget *parent, db::LoadLayoutOptions *options, const std::string &format_name)
  : QDialog (parent),
    m_format_name (format_name), mp_options (options), mp_specific_options (0), mp_editor (0)
{
  setObjectName (QString::fromUtf8 ("specific_load_layout_options_dialog"));

  mp_ui = new Ui::SpecificLoadLayoutOptionsDialog ();
  mp_ui->setupUi (this);

  setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Edit Reader Options")) + " - " + format_name));

  db::FormatSpecificReaderOptions *specific_options = mp_options->get_options (format_name);
  const lay::StreamReaderPluginDeclaration *decl = StreamReaderPluginDeclaration::plugin_for_format (format_name);
  if (decl && specific_options) {

    mp_specific_options = specific_options->clone ();

    mp_editor = decl->format_specific_options_page (mp_ui->content_frame);
    if (mp_editor) {

      QVBoxLayout *layout = new QVBoxLayout (mp_ui->content_frame);
      layout->addWidget (mp_editor);
      layout->setContentsMargins (0, 0, 0, 0);
      mp_ui->content_frame->setLayout (layout);

      mp_editor->show ();
      mp_editor->setup (specific_options, 0);

    }
  }
}

SpecificLoadLayoutOptionsDialog::~SpecificLoadLayoutOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;

  delete mp_specific_options;
  mp_specific_options = 0;
}

void
SpecificLoadLayoutOptionsDialog::accept ()
{
BEGIN_PROTECTED
  if (mp_editor && mp_options && mp_specific_options) {
    mp_editor->commit (mp_specific_options, 0);
    mp_options->set_options (mp_specific_options);
    mp_specific_options = 0;
  }

  QDialog::accept ();
END_PROTECTED
}

}

#endif
