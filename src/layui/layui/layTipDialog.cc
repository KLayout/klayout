
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

#include "layTipDialog.h"
#include "laybasicConfig.h"
#include "layPlugin.h"
#include "layDispatcher.h"
#include "layDialogs.h"
#include "layQtTools.h"

#include "ui_TipDialog.h"

namespace lay
{

// ------------------------------------------------------------
//  Implementation of the "tip" dialog

TipDialog::TipDialog (QWidget *parent, const std::string &text, const std::string &key, buttons_type buttons)
  : QDialog (parent), m_key (key)
{
  init (text, buttons);
} 

void
TipDialog::init (const std::string &text, buttons_type buttons)
{
  mp_ui = new Ui::TipDialog ();
  mp_ui->setupUi (this);

  mp_ui->dont_show_cbx->setChecked (false);
  mp_ui->tip_text->setText (tl::to_qstring (text));

  mp_ui->ok_button->hide ();
  mp_ui->cancel_button->hide ();
  mp_ui->close_button->hide ();
  mp_ui->yes_button->hide ();
  mp_ui->no_button->hide ();

  connect (mp_ui->ok_button, SIGNAL (clicked ()), this, SLOT (ok_pressed ()));
  connect (mp_ui->close_button, SIGNAL (clicked ()), this, SLOT (close_pressed ()));
  connect (mp_ui->cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_pressed ()));
  connect (mp_ui->yes_button, SIGNAL (clicked ()), this, SLOT (yes_pressed ()));
  connect (mp_ui->no_button, SIGNAL (clicked ()), this, SLOT (no_pressed ()));

  activate_help_links (mp_ui->tip_text);

  if (buttons == close_buttons) {
    mp_ui->close_button->show ();
  } else if (buttons == okcancel_buttons) {
    mp_ui->ok_button->show ();
    mp_ui->cancel_button->show ();
  } else if (buttons == yesno_buttons) {
    mp_ui->yes_button->show ();
    mp_ui->no_button->show ();
  } else if (buttons == yesnocancel_buttons) {
    mp_ui->yes_button->show ();
    mp_ui->no_button->show ();
    mp_ui->cancel_button->show ();
  }
}

bool
TipDialog::exec_dialog (button_type &button)
{
  return do_exec_dialog (&button);
}

bool
TipDialog::exec_dialog ()
{
  button_type b = no_button;
  return do_exec_dialog (&b);
}

void 
TipDialog::close_pressed ()
{
  *mp_res = close_button;
  accept ();
}

void 
TipDialog::ok_pressed ()
{
  *mp_res = ok_button;
  accept ();
}

void 
TipDialog::cancel_pressed ()
{
  *mp_res = cancel_button;
  reject (); // don't set "show never again"
}

void 
TipDialog::yes_pressed ()
{
  *mp_res = yes_button;
  accept ();
}

void 
TipDialog::no_pressed ()
{
  *mp_res = no_button;
  accept ();
}

static std::pair<bool, int>
tip_dialog_status (const std::string &key)
{
  std::string th;
  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_get (cfg_tip_window_hidden, th);
  }

  //  test if we need to show this window
  tl::Extractor ex (th.c_str ());
  while (! ex.at_end ()) {
    std::string k;
    if (! ex.try_read_word (k, "_-.")) {
      break;
    }
    int r = -1;
    ex.test ("=") && ex.try_read (r);
    if (k == key) {
      return std::make_pair (false, r);
    }
    ex.test (",");
  }

  return std::make_pair (true, -1);
}

bool
TipDialog::will_be_shown ()
{
  return tip_dialog_status (m_key).first;
}

bool
TipDialog::do_exec_dialog (button_type *button)
{
  mp_res = button;

  std::string th;
  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_get (cfg_tip_window_hidden, th);
  }

  std::pair<bool, int> td_status = tip_dialog_status (m_key);
  if (td_status.first) {
    exec ();
    return true;
  } else {
    if (td_status.second >= 0) {
      *mp_res = button_type (td_status.second);
    }
    return false;
  }
}

void
TipDialog::accept ()
{
  //  register this dialog as dont-show on next startup
  if (mp_ui->dont_show_cbx->isChecked ()) {

    std::string th;
    if (lay::Dispatcher::instance ()) {
      lay::Dispatcher::instance ()->config_get (cfg_tip_window_hidden, th);
    }

    if (! th.empty ()) {
      th += ",";
    } 
    th += m_key;
    th += "=";
    th += tl::to_string (int (*mp_res));

    if (lay::Dispatcher::instance ()) {
      lay::Dispatcher::instance ()->config_set (cfg_tip_window_hidden, th);
    }

  }

  QDialog::accept ();
}

TipDialog::~TipDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

}

#endif
