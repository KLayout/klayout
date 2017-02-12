
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#ifndef HDR_layCrashMessage
#define HDR_layCrashMessage

#include <QDialog>

#include "layCommon.h"
#include "ui_CrashMessage.h"

namespace lay
{

/**
 *  @brief A window showing a crash message
 */
class LAY_PUBLIC CrashMessage  
  : public QDialog, private Ui::CrashMessage
{
public:
  /**
   *  @brief Instantiate a dialog
   *
   *  @param can_resume If true, an "Ok" button is provided
   *  @param stack_trace The stack trace message shown in the window
   */
  CrashMessage (QWidget *parent, bool can_resume, const QString &stack_trace);
};

}

#endif

