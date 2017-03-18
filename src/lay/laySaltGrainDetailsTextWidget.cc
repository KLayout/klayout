
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

#include "laySaltGrainDetailsTextWidget.h"
#include "laySaltGrain.h"
#include "tlString.h"

#include <QTextStream>
#include <QBuffer>
#include <QIcon>

namespace lay
{

SaltGrainDetailsTextWidget::SaltGrainDetailsTextWidget (QWidget *w)
  : QTextBrowser (w), mp_grain (0)
{
  //  .. nothing yet ..
}

void SaltGrainDetailsTextWidget::set_grain (SaltGrain *g)
{
  if (mp_grain != g) {
    mp_grain = g;
    setHtml (details_text ());
  }
}

QVariant
SaltGrainDetailsTextWidget::loadResource (int type, const QUrl &url)
{
  if (url.path () == QString::fromUtf8 ("/icon")) {
    //  @@@
    return QImage (":/salt_icon.png");
    //  @@@
  } else {
    return QTextBrowser::loadResource (type, url);
  }
}

QString
SaltGrainDetailsTextWidget::details_text ()
{
  SaltGrain *g = mp_grain;
  if (! g) {
    return QString ();
  }

  QBuffer buffer;
  buffer.open (QIODevice::WriteOnly);
  QTextStream stream (&buffer);
  stream.setCodec ("UTF-8");

  stream << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head><body>";

  stream << "<table><tr>";
  stream << "<td><img src=\":/icon\"/></td>";
  stream << "<td>";
  stream << "<h1>";
  stream << tl::to_qstring (tl::escaped_to_html (g->name ())) << " " << tl::to_qstring (tl::escaped_to_html (g->version ()));
  stream << "</h1>";
  if (! g->title ().empty()) {
    stream << "<h3>" << tl::to_qstring (tl::escaped_to_html (g->title ())) << "</h3>";
  }

  if (g->version ().empty ()) {
    stream << "<p><i><font color='gray'>";
    stream << QObject::tr ("This package does not have a version. "
                           "Use the &lt;version&gt; element of the specification file or edit the package properties to provide a version.");
    stream << "</font></i></p>";
  }

  if (g->title ().empty ()) {
    stream << "<p><i><font color='gray'>";
    stream << QObject::tr ("This package does not have a title. "
                           "Use the &lt;title&gt; element of the specification file or edit the package properties to provide a title.");
    stream << "</font></i></p>";
  }

  stream << "</td></tr></table>";

  stream << "<p><br/>";
  if (! g->doc ().empty ()) {
    stream << tl::to_qstring (tl::escaped_to_html (g->doc ()));
  } else {
    stream << "<i><font color='gray'>";
    stream << QObject::tr ("This package does not have a description. "
                           "Use the &lt;doc&gt; element of the specification file or edit the package properties to provide a description.");
    stream << "</font></i>";
  }
  stream << "</p>";

  stream << "<p>";
  if (! g->author ().empty ()) {
    stream << "<b>" << QObject::tr ("Author") << ":</b> " << tl::to_qstring (tl::escaped_to_html (g->author ())) << " ";
    if (! g->author_contact ().empty ()) {
      stream << "(" << tl::to_qstring (tl::escaped_to_html (g->author_contact ())) << ")";
    }
    if (!g->authored_time ().isNull ()) {
      stream << "<br/>";
      stream << "<b>" << QObject::tr ("Released") << ":</b> " << g->authored_time ().date ().toString (Qt::ISODate);
    }
  } else {
    stream << "<i><font color='gray'>";
    stream << QObject::tr ("This package does not have a author information. "
                           "Use the &lt;author&gt;, &lt;authored-time&gt; and &lt;author-contact&gt; elements of the specification file or edit the package properties to provide authoring information.");
    stream << "</font></i>";
  }
  stream << "</p>";

  stream << "<p>";
  if (! g->url ().empty ()) {
    stream << "<b>" << QObject::tr ("Documentation link") << ":</b> <a href=\"" << tl::to_qstring (g->url ()) << "\">" << tl::to_qstring (tl::escaped_to_html (g->url ())) << "</a>";
  } else {
    stream << "<i><font color='gray'>";
    stream << QObject::tr ("This package does not have a documentation link. "
                           "Use the &lt;url&gt; element of the specification file or edit the package properties to provide a link.");
    stream << "</font></i>";
  }
  stream << "</p>";

  stream << "</body></html>";

  stream.flush ();

  return QString::fromUtf8 (buffer.buffer());
}

}
