
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

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiEnums.h"
#include "layCursor.h"
#include "layViewObject.h"

namespace gsi
{

class CursorNamespace { };

static int cursor_shape_none () { return int (lay::Cursor::none); }
static int cursor_shape_arrow () { return int (lay::Cursor::arrow); }
static int cursor_shape_up_arrow () { return int (lay::Cursor::up_arrow); }
static int cursor_shape_cross () { return int (lay::Cursor::cross); }
static int cursor_shape_wait () { return int (lay::Cursor::wait); }
static int cursor_shape_i_beam () { return int (lay::Cursor::i_beam); }
static int cursor_shape_size_ver () { return int (lay::Cursor::size_ver); }
static int cursor_shape_size_hor () { return int (lay::Cursor::size_hor); }
static int cursor_shape_size_bdiag () { return int (lay::Cursor::size_bdiag); }
static int cursor_shape_size_fdiag () { return int (lay::Cursor::size_fdiag); }
static int cursor_shape_size_all () { return int (lay::Cursor::size_all); }
static int cursor_shape_blank () { return int (lay::Cursor::blank); }
static int cursor_shape_split_v () { return int (lay::Cursor::split_v); }
static int cursor_shape_split_h () { return int (lay::Cursor::split_h); }
static int cursor_shape_pointing_hand () { return int (lay::Cursor::pointing_hand); }
static int cursor_shape_forbidden () { return int (lay::Cursor::forbidden); }
static int cursor_shape_whats_this () { return int (lay::Cursor::whats_this); }
static int cursor_shape_busy () { return int (lay::Cursor::busy); }
static int cursor_shape_open_hand () { return int (lay::Cursor::open_hand); }
static int cursor_shape_closed_hand () { return int (lay::Cursor::closed_hand); }

Class<gsi::CursorNamespace> decl_Cursor ("lay", "Cursor",
  method ("None", &cursor_shape_none, "@brief 'No cursor (default)' constant for \\Plugin#set_cursor (resets cursor to default)") +
  method ("Arrow", &cursor_shape_arrow, "@brief 'Arrow cursor' constant") +
  method ("UpArrow", &cursor_shape_up_arrow, "@brief 'Upward arrow cursor' constant") +
  method ("Cross", &cursor_shape_cross, "@brief 'Cross cursor' constant") +
  method ("Wait", &cursor_shape_wait, "@brief 'Waiting cursor' constant") +
  method ("IBeam", &cursor_shape_i_beam, "@brief 'I beam (text insert) cursor' constant") +
  method ("SizeVer", &cursor_shape_size_ver, "@brief 'Vertical resize cursor' constant") +
  method ("SizeHor", &cursor_shape_size_hor, "@brief 'Horizontal resize cursor' constant") +
  method ("SizeBDiag", &cursor_shape_size_bdiag, "@brief 'Backward diagonal resize cursor' constant") +
  method ("SizeFDiag", &cursor_shape_size_fdiag, "@brief 'Forward diagonal resize cursor' constant") +
  method ("SizeAll", &cursor_shape_size_all, "@brief 'Size all directions cursor' constant") +
  method ("Blank", &cursor_shape_blank, "@brief 'Blank cursor' constant") +
  method ("SplitV", &cursor_shape_split_v, "@brief 'Split vertical cursor' constant") +
  method ("SplitH", &cursor_shape_split_h, "@brief 'split_horizontal cursor' constant") +
  method ("PointingHand", &cursor_shape_pointing_hand, "@brief 'Pointing hand cursor' constant") +
  method ("Forbidden", &cursor_shape_forbidden, "@brief 'Forbidden area cursor' constant") +
  method ("WhatsThis", &cursor_shape_whats_this, "@brief 'Question mark cursor' constant") +
  method ("Busy", &cursor_shape_busy, "@brief 'Busy state cursor' constant") +
  method ("OpenHand", &cursor_shape_open_hand, "@brief 'Open hand cursor' constant") +
  method ("ClosedHand", &cursor_shape_closed_hand, "@brief 'Closed hand cursor' constant"),
  "@brief The namespace for the cursor constants\n"
  "This class defines the constants for the cursor setting (for example for method \\Plugin#set_cursor)."
  "\n"
  "This class has been introduced in version 0.22.\n"
);

class ButtonStateNamespace { };

static int const_ShiftButton()      { return (int) lay::ShiftButton; }
static int const_ControlButton()    { return (int) lay::ControlButton; }
static int const_AltButton()        { return (int) lay::AltButton; }
static int const_ModifierMask()     { return (int) lay::ModifierMask; }
static int const_LeftButton()       { return (int) lay::LeftButton; }
static int const_MidButton()        { return (int) lay::MidButton; }
static int const_RightButton()      { return (int) lay::RightButton; }
static int const_MouseButtonMask()  { return (int) lay::MouseButtonMask; }

Class<gsi::ButtonStateNamespace> decl_ButtonState ("lay", "ButtonState",
  method ("ModifierMask", &const_ModifierMask, "@brief A bit mask that selects all keyboard modifiers in the button state mask\nThis constant has been introduced in version 0.30.6.") +
  method ("MouseButtonMask", &const_MouseButtonMask, "@brief A bit mask that selects all mouse buttons in the button state mask\nThis constant has been introduced in version 0.30.6.") +
  method ("ShiftKey", &const_ShiftButton, "@brief Indicates that the Shift key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("ControlKey", &const_ControlButton, "@brief Indicates that the Control key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("AltKey", &const_AltButton, "@brief Indicates that the Alt key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("LeftButton", &const_LeftButton, "@brief Indicates that the left mouse button is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("MidButton", &const_MidButton, "@brief Indicates that the middle mouse button is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("RightButton", &const_RightButton, "@brief Indicates that the right mouse button is pressed\nThis constant is combined with other constants within \\ButtonState"),
  "@brief The namespace for the button state flags in the mouse events of the Plugin class.\n"
  "This class defines the constants for the button state. In the event handler, the button state is "
  "indicated by a bitwise combination of these constants. See \\Plugin for further details."
  "\n"
  "This class has been introduced in version 0.22.\n"
);

class KeyCodesNamespace { };

static int const_KeyEscape()        { return (int) lay::KeyEscape; }
static int const_KeyTab()           { return (int) lay::KeyTab; }
static int const_KeyBacktab()       { return (int) lay::KeyBacktab; }
static int const_KeyBackspace()     { return (int) lay::KeyBackspace; }
static int const_KeyReturn()        { return (int) lay::KeyReturn; }
static int const_KeyEnter()         { return (int) lay::KeyEnter; }
static int const_KeyInsert()        { return (int) lay::KeyInsert; }
static int const_KeyDelete()        { return (int) lay::KeyDelete; }
static int const_KeyHome()          { return (int) lay::KeyHome; }
static int const_KeyEnd()           { return (int) lay::KeyEnd; }
static int const_KeyDown()          { return (int) lay::KeyDown; }
static int const_KeyUp()            { return (int) lay::KeyUp; }
static int const_KeyLeft()          { return (int) lay::KeyLeft; }
static int const_KeyRight()         { return (int) lay::KeyRight; }
static int const_KeyPageUp()        { return (int) lay::KeyPageUp; }
static int const_KeyPageDown()      { return (int) lay::KeyPageDown; }

Class<gsi::KeyCodesNamespace> decl_KeyCode ("lay", "KeyCode",
  method ("Escape", &const_KeyEscape, "@brief Indicates the Escape key") +
  method ("Tab", &const_KeyTab, "@brief Indicates the Tab key") +
  method ("Backtab", &const_KeyBacktab, "@brief Indicates the Backtab key") +
  method ("Backspace", &const_KeyBackspace, "@brief Indicates the Backspace key") +
  method ("Return", &const_KeyReturn, "@brief Indicates the Return key") +
  method ("Enter", &const_KeyEnter, "@brief Indicates the Enter key") +
  method ("Insert", &const_KeyInsert, "@brief Indicates the Insert key") +
  method ("Delete", &const_KeyDelete, "@brief Indicates the Delete key") +
  method ("Home", &const_KeyHome, "@brief Indicates the Home key") +
  method ("End", &const_KeyEnd, "@brief Indicates the End key") +
  method ("Down", &const_KeyDown, "@brief Indicates the Down key") +
  method ("Up", &const_KeyUp, "@brief Indicates the Up key") +
  method ("Left", &const_KeyLeft, "@brief Indicates the Left key") +
  method ("Right", &const_KeyRight, "@brief Indicates the Right key") +
  method ("PageUp", &const_KeyPageUp, "@brief Indicates the PageUp key") +
  method ("PageDown", &const_KeyPageDown, "@brief Indicates the PageDown key"),
  "@brief The namespace for the some key codes.\n"
  "This namespace defines some key codes understood by built-in \\LayoutView components. "
  "When compiling with Qt, these codes are compatible with Qt's key codes.\n"
  "The key codes are intended to be used when directly interfacing with \\LayoutView in non-Qt-based environments.\n"
  "\n"
  "This class has been introduced in version 0.28.\n"
);

}
