
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


#include "layRuntimeErrorForm.h"
#include "layQtTools.h"
#include "tlScriptError.h"

#include <QMessageBox>

namespace lay
{

// ------------------------------------------------------------

RuntimeErrorForm::RuntimeErrorForm (QWidget *parent, const char *name, const tl::ScriptError *error)
  : QDialog (parent), Ui::RuntimeErrorForm ()
{
  setObjectName (QString::fromUtf8 (name));

  Ui::RuntimeErrorForm::setupUi (this);

  msg_label->setText (tl::to_qstring (error->basic_msg ()));
  details_text->setText (tl::to_qstring (error->msg ()));
  details_text->setFont (lay::monospace_font ());
  details_frame->hide ();

  //  "borrow" the error pixmap from the message box
  QMessageBox *mb = new QMessageBox (QMessageBox::Critical, QString (), QString ());
  QPixmap error_icon = mb->iconPixmap ();
  delete mb;
  icon_label->setPixmap (error_icon);

  connect (details_pb, SIGNAL (clicked ()), this, SLOT (show_details ()));

  resize (size ().width (), 50);
}

void 
RuntimeErrorForm::show_details ()
{
  QString t (details_pb->text ());
  if (details_frame->isVisible ()) {
    details_frame->hide ();
    t.replace (QString::fromUtf8 ("<<"), QString::fromUtf8 (">>"));
    //  It looks like the minimum size is set to a too large value internally. 
    //  Resetting it helps to keep a small-as-possible dialog size.
    setMinimumSize (QSize (0, 0));
    resize (size ().width (), 0);
  } else {
    details_frame->show ();
    t.replace (QString::fromUtf8 (">>"), QString::fromUtf8 ("<<"));
    resize (size ().width (), sizeHint ().height ());
  }
  details_pb->setText (t);
}

}

