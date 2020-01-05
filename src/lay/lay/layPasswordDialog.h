
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

#ifndef HDR_layPasswordDialog
#define HDR_layPasswordDialog

#include "tlHttpStream.h"

#include "ui_PasswordDialog.h"
#include <QDialog>

namespace lay
{

/**
 * @brief A password dialog for registration with tl::HttpStream
 */
class PasswordDialog
  : public QDialog, public tl::HttpCredentialProvider, private Ui::PasswordDialog
{
public:
  PasswordDialog (QWidget *parent);

  bool user_password (const std::string &url, const std::string &realm, bool proxy, int attempt, std::string &user, std::string &passwd);
};



}

#endif
