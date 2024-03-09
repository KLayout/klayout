
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

#ifndef HDR_layBrowser
#define HDR_layBrowser

#include "layuiCommon.h"

#include "layPlugin.h"

#include <QDialog>

class QCloseEvent;

namespace lay
{

class LayoutViewBase;
class Dispatcher;

class LAYUI_PUBLIC Browser
  : public QDialog, 
    public lay::Plugin
{
public:
  /**
   *  @brief Constructor 
   */
#if QT_VERSION >= 0x050000
  Browser (lay::Dispatcher *root, lay::LayoutViewBase *view, const char *name = "", Qt::WindowFlags fl = Qt::Window /*adds minimize button for example*/);
#else
  Browser (lay::Dispatcher *root, lay::LayoutViewBase *view, const char *name = "", Qt::WFlags fl = Qt::Window /*adds minimize button for example*/);
#endif

  /**
   *  @brief Destructor
   */
  virtual ~Browser ();

  /**
   *  @brief Activation event
   *
   *  This method can be overloaded by derived classes to provide actions
   *  for activation of the dialog, like setup of the controls etc.
   *  This handler is called immediately before the dialog becomes visible.
   *  The 'active' method returns a value indicating whether the dialog is active.
   */
  virtual void activated () 
  {
    //  the default implementation does nothing.
  }

  /**
   *  @brief Deactivation event
   *
   *  The handler is called if the dialog becomes deactivated, either by
   *  request of the view or by closing the dialog. This method is supposed
   *  to release all resources related to the browsing, i.e. view objects etc.
   */
  virtual void deactivated () 
  {
    //  the default implementation does nothing.
  }
  
  /**
   *  @brief Tell if the dialog is active
   */
  bool active () const
  {
    return m_active;
  }

  /** 
   *  @brief Return the pointer to the layout view 
   */
  lay::LayoutViewBase *view ()
  {
    return mp_view;
  }

  /**
   *  @brief Returns the pointer to the root configuration object
   */
  lay::Dispatcher *root ()
  {
    return mp_root;
  }

  /**
   *  @brief Activate the dialog
   *
   *  Calls activated() before the dialog is shown. show() can be used too
   *  but will not activate the dialog.
   */
  void activate ();

  /**
   *  @brief Deactivate the dialog
   *
   *  Calls deactivated() after the dialog is hidden. hide() can be used too
   *  but will not deactivate the dialog.
   */
  void deactivate ();

  /**
   *  @brief implementation of the lay::Plugin interface: obtain a pointer to the lay::Browser interface
   */
  lay::Browser *browser_interface ()
  {
    return this;
  }

private:
  bool m_active;
  lay::LayoutViewBase *mp_view;
  lay::Dispatcher *mp_root;

  void closeEvent (QCloseEvent *);
  void accept ();
  void reject ();
};

}

#endif

#endif  //  defined(HAVE_QT)
