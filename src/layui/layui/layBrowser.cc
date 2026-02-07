
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

#include <QDialog>
#include <QApplication>
#include <QCloseEvent>

#include "layBrowser.h"
#include "layLayoutViewBase.h"

#include <vector>

namespace lay
{

#if QT_VERSION >= 0x050000
Browser::Browser (lay::Dispatcher *root, lay::LayoutViewBase *view, const char *name, Qt::WindowFlags fl)
#else
Browser::Browser (lay::Dispatcher *root, lay::LayoutViewBase *view, const char *name, Qt::WFlags fl)
#endif
    //  TODO: clarify whether to keep the browsers as separate (potentially hidden) windows
  : QDialog (0 /*view*/, fl),
    lay::Plugin (view),
    m_active (false),
    mp_view (view),
    mp_root (root)
{
  QObject::setObjectName (QString::fromUtf8 (name));
}

Browser::~Browser ()
{
  if (active ()) {
    deactivated ();
  }
}

void 
Browser::activate ()
{
  if (! active ()) {
    m_active = true;
    activated ();
    QDialog::show ();
  }
}

void 
Browser::deactivate ()
{
  if (active ()) {
    m_active = false;
    deactivated ();
    QDialog::hide ();
  }
}

void 
Browser::closeEvent (QCloseEvent *event)
{
  if (active ()) {
    m_active = false;
    deactivated ();
    event->accept ();
  }
}

void
Browser::accept ()
{
  if (active ()) {
    m_active = false;
    deactivated ();
    QDialog::accept ();
  }
}

void
Browser::reject ()
{
  if (active ()) {
    m_active = false;
    deactivated ();
    QDialog::reject ();
  }
}

}

#endif

