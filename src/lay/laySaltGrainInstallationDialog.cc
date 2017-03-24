
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "laySaltGrainInstallationDialog.h"
#include "laySaltModel.h"
#include "laySaltGrainPropertiesDialog.h"
#include "laySalt.h"
#include "ui_SaltGrainTemplateSelectionDialog.h"
#include "tlString.h"
#include "tlExceptions.h"

#include <QAbstractItemModel>
#include <QAbstractTextDocumentLayout>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>
#include <QDir>
#include <QTextStream>
#include <QBuffer>
#include <QResource>
#include <QMessageBox>

namespace lay
{

// --------------------------------------------------------------------------------------
//  SaltGrainInstallation implementation

SaltGrainInstallationDialog::SaltGrainInstallationDialog (QWidget *parent, lay::Salt *salt)
  : QDialog (parent), mp_salt (salt)
{
  Ui::SaltGrainInstallationDialog::setupUi (this);

  //  TODO: cache package list
  // @@@
  m_salt_mine.load ("/home/matthias/salt.mine");
  // @@@

  SaltModel *model = new SaltModel (this, &m_salt_mine);
  salt_view->setModel (model);
  salt_view->setItemDelegate (new SaltItemDelegate (this));
  salt_view->setCurrentIndex (model->index (0, 0, QModelIndex ()));

  connect (salt_view->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_changed ()));
  connect (mark_button, SIGNAL (clicked ()), this, SLOT (mark ()));
  connect (button_box->button (QDialogButtonBox::Apply), SIGNAL (clicked ()), this, SLOT (apply ()));

  current_changed ();
}

void
SaltGrainInstallationDialog::current_changed ()
{
  SaltGrain *g = current_grain ();
  details_text->set_grain (g);
  details_frame->setEnabled (g != 0);
}

lay::SaltGrain *
SaltGrainInstallationDialog::current_grain ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  return model ? model->grain_from_index (salt_view->currentIndex ()) : 0;
}

void
SaltGrainInstallationDialog::apply ()
{

  // @@@

}

void
SaltGrainInstallationDialog::mark ()
{

  // @@@

}

}
