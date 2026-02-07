
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

#include "layPasswordDialog.h"

namespace lay
{

PasswordDialog::PasswordDialog (QWidget *parent)
  : QDialog (parent)
{
  setupUi (this);
}

bool
PasswordDialog::user_password (const std::string &url, const std::string &realm, bool proxy, int attempt, std::string &user, std::string &passwd)
{
  realm_label->setText (tr ("<b>Realm:</b> ") + tl::to_qstring (realm));
  if (proxy) {
    where_label->setText (tr ("<b>Proxy:</b> ") + tl::to_qstring (url));
  } else {
    where_label->setText (tr ("<b>URL:</b> ") + tl::to_qstring (url));
  }

  if (attempt > 1) {
    attempt_label->setText (tr ("Authentication failed - please try again"));
    attempt_label->show ();
  } else {
    attempt_label->hide ();
  }

  if (QDialog::exec ()) {
    passwd = tl::to_string (password_le->text ());
    user = tl::to_string (user_le->text ());
    return true;
  } else {
    return false;
  }

}

}

