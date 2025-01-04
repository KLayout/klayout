
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


#include "layReaderErrorForm.h"
#include "layQtTools.h"
#include "dbReader.h"

#include <QMessageBox>

namespace lay
{

// ------------------------------------------------------------

static bool is_text (const std::string &s)
{
  for (std::string::const_iterator i = s.begin (); i != s.end (); ++i) {
    unsigned char uc = (unsigned char) *i;
    if (uc < 32 && uc != '\t' && uc != '\r' && uc != '\n') {
      return false;
    }
  }
  return true;
}

static std::string format_hex_dump (const std::string &s)
{
  const int bytes_per_line = 16;

  std::string hex_dump;
  //  Some rough estimate of the capacity
  hex_dump.reserve ((s.size () / bytes_per_line + 1) * (8 + bytes_per_line * 4) + 100);

  const char *ce = s.c_str () + s.size ();
  for (const char *c = s.c_str (); c + bytes_per_line <= ce; c += bytes_per_line) {

    hex_dump += tl::sprintf ("%04x  ", c - s.c_str ());
    for (int i = 0; i < bytes_per_line; ++i) {
      hex_dump += tl::sprintf ("%02x ", (unsigned char) c [i]);
    }
    hex_dump += " ";
    for (int i = 0; i < bytes_per_line; ++i) {
      unsigned char uc = (unsigned char) c[i];
      hex_dump += (uc < 32 || uc >= 128) ? '.' : c[i];
    }

    hex_dump += "\n";

  }

  return hex_dump;
}

ReaderErrorForm::ReaderErrorForm (QWidget *parent, const char *name, const db::ReaderUnknownFormatException *error)
  : QDialog (parent), Ui::ReaderErrorForm ()
{
  setObjectName (QString::fromUtf8 (name));

  Ui::ReaderErrorForm::setupUi (this);

  msg_label->setText (tl::to_qstring (error->basic_msg ()));

  if (is_text (error->data ())) {
    details_text->setText (tl::to_qstring (error->msg () + "\n\n" + error->data () + (error->has_more () ? "..." : "")));
  } else {
    details_text->setText (tl::to_qstring (error->msg () + "\n\n" + format_hex_dump (error->data ()) + (error->has_more () ? "..." : "")));
  }

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
ReaderErrorForm::show_details ()
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

