
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

#if defined(HAVE_QT)

#ifndef HDR_layQtTools
#define HDR_layQtTools

#include "layuiCommon.h"

#include <string>
#include <QFont>

class QLabel;
class QWidget;
class QObject;
class QSignalBlocker;

namespace tl
{
  class Exception;
}

namespace lay
{

/**
 *  @brief Save the given widget's (dialog) state to the string
 *
 *  The state can be recovered from the string using restore_dialog_state;
 */
LAYUI_PUBLIC std::string save_dialog_state (QWidget *dialog, bool with_section_sizes = true);

/**
 *  @brief Restore the dialog's state from the given string
 */
LAYUI_PUBLIC void restore_dialog_state (QWidget *dialog, const std::string &s, bool with_section_sizes = true);

/**
 *  @brief A utility function connecting a label's linkActivated event with the help browser
 */
LAYUI_PUBLIC void activate_help_links (QLabel *label);

/**
 *  @brief A utility function connecting a label's linkActivated event with the help browser (modal help dialogs)
 */
LAYUI_PUBLIC void activate_modal_help_links (QLabel *label);

/**
 *  @brief Register the help handler (object and slots for non-modal and modal help dialogs)
 */
LAYUI_PUBLIC void register_help_handler (QObject *object, const char *slot, const char *modal_slot);

/**
 *  @brief Configures a QLineEdit or other widget to indicate an error
 *
 *  When a non-null ex pointer is passed, the background will be turned red
 *  and the exception's text will be used as tooltip. Use this function with
 *  a null ex pointer to clear the error condition.
 */
LAYUI_PUBLIC void indicate_error (QWidget *le, const tl::Exception *ex);

/**
 *  @brief Configures a QLineEdit or other widget to indicate an error
 */
LAYUI_PUBLIC void indicate_error (QWidget *le, bool error);

/**
 *  @brief Gets the system monospace font
 */
LAYUI_PUBLIC QFont monospace_font ();


#if QT_VERSION < 0x050000

//  Provide missing QSignalBlocker for Qt4

class LAYUI_PUBLIC SignalBlocker
{
public:
  SignalBlocker (QWidget *w);
  ~SignalBlocker ();

private:
  QWidget *mp_widget;
  bool m_state;
};

#else

typedef QSignalBlocker SignalBlocker;

#endif

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
