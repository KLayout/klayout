
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

#include "layProgressDialog.h"
#include "layProgressWidget.h"

#include <QVBoxLayout>

namespace lay
{

ProgressDialog::ProgressDialog (QWidget *parent, ProgressReporter *pr)
  : QDialog (parent), mp_pr (pr)
{
  QVBoxLayout *vbl = new QVBoxLayout (this);
  vbl->setContentsMargins (0, 0, 0, 0);
  vbl->setSpacing (0);

  mp_progress_widget = new ProgressWidget (pr, this, true);
  mp_progress_widget->setObjectName (QString::fromUtf8 ("progress"));
  vbl->addWidget (mp_progress_widget);

  setWindowTitle (QObject::tr ("Progress"));
  setWindowModality (Qt::WindowModal);
}

void ProgressDialog::closeEvent (QCloseEvent *)
{
  if (mp_pr) {
    //  NOTE: We don't kill on close for now. This creates a too easy way to scrap results.
    //    mp_pr->signal_break ();
    //  TODO: there should be a warning saying some jobs are pending.
  }
}

void ProgressDialog::set_progress (tl::Progress *progress)
{
  mp_progress_widget->set_progress (progress);
}

void ProgressDialog::add_widget (QWidget *widget)
{
  mp_progress_widget->add_widget (widget);
}

void ProgressDialog::remove_widget ()
{
  mp_progress_widget->remove_widget ();
}

QWidget *ProgressDialog::get_widget () const
{
  return mp_progress_widget->get_widget ();
}

}
