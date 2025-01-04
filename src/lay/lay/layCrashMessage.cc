
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


#include "layCrashMessage.h"
#include "layQtTools.h"

#include <QPushButton>

namespace lay
{

CrashMessage::CrashMessage (QWidget *parent, bool can_resume, const QString &t)
  : QDialog (parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  setupUi (this);
  m_cancel_pressed = false;

  text->setFont (monospace_font ());
  text->setPlainText (t);
  set_can_resume (can_resume);

  connect (buttonBox->button (QDialogButtonBox::Cancel), SIGNAL (pressed ()), this, SLOT (cancel_pressed ()));
}

CrashMessage::~CrashMessage ()
{
  //  .. nothing yet ..
}

void
CrashMessage::set_can_resume (bool f)
{
  buttonBox->button (QDialogButtonBox::Ok)->setVisible (f);
}

void
CrashMessage::set_text (const QString &t)
{
  text->setPlainText (t);
}

void
CrashMessage::cancel_pressed ()
{
  m_cancel_pressed = true;
}

}

