
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

#include "laySaltGrainDetailsTextWidget.h"
#include "laySaltGrain.h"
#include "tlString.h"

#include <QTextStream>
#include <QBuffer>
#include <QIcon>
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

namespace lay
{

SaltGrainDetailsTextWidget::SaltGrainDetailsTextWidget (QWidget *w)
  : QTextBrowser (w), mp_grain (), m_detailed_view (false)
{
  setOpenLinks (false);
  setOpenExternalLinks (false);
  connect (this, SIGNAL (anchorClicked (const QUrl &)), this, SLOT (open_link (const QUrl &)));
}

void
SaltGrainDetailsTextWidget::set_grain (const SaltGrain *g)
{
  if (g) {
    mp_grain.reset (new SaltGrain (*g));
  } else {
    mp_grain.reset (0);
  }
  setHtml (details_text ());
}

void
SaltGrainDetailsTextWidget::show_detailed_view (bool f)
{
  if (m_detailed_view != f) {
    m_detailed_view = f;
    setHtml (details_text ());
  }
}

void
SaltGrainDetailsTextWidget::open_link (const QUrl &url)
{
  QDesktopServices::openUrl (url);
}

QVariant
SaltGrainDetailsTextWidget::loadResource (int type, const QUrl &url)
{
  if (url.path () == QString::fromUtf8 ("/icon")) {

    int icon_dim = 64;

    if (!mp_grain.get () || mp_grain->icon ().isNull ()) {

      return QImage (":/salt_icon.png");

    } else {

      QImage img = mp_grain->icon ();
      if (img.width () != icon_dim || img.height () != icon_dim) {

        img = img.scaled (QSize (icon_dim, icon_dim), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QImage final_img (icon_dim, icon_dim, QImage::Format_ARGB32);
#if QT_VERSION >= 0x40700
        final_img.fill (QColor (0, 0, 0, 0));
#else
        final_img.fill (0);
#endif
        QPainter painter (&final_img);
        painter.drawImage ((icon_dim - img.width ()) / 2, (icon_dim - img.height ()) / 2, img);

        return final_img;

      } else {
        return img;
      }

    }

  } else if (url.path () == QString::fromUtf8 ("/screenshot")) {

    QImage s = mp_grain->screenshot ().convertToFormat (QImage::Format_ARGB32_Premultiplied);

    QImage smask (s.size (), QImage::Format_ARGB32_Premultiplied);
#if QT_VERSION >= 0x40700
    smask.fill (QColor (0, 0, 0, 0));
#else
    smask.fill (0);
#endif
    {
      int border = 0;
      int radius = 6;

      QPainter painter (&smask);

      painter.setRenderHint (QPainter::Antialiasing);

      painter.setCompositionMode (QPainter::CompositionMode_Source);
      for (int b = border; b > 0; --b) {
        QPen pen (QColor (255, 255, 255, ((border - b + 1) * 255) / border));
        pen.setWidth (b * 2 + 1);
        painter.setBrush (Qt::NoBrush);
        painter.setPen (pen);
        painter.drawRoundedRect (QRectF (border, border, s.width () - 2 * border, s.height () - 2 * border), radius, radius, Qt::AbsoluteSize);
      }

      painter.setPen (Qt::white);
      painter.setBrush (Qt::white);
      painter.drawRoundedRect (QRectF (border, border, s.width () - 2 * border, s.height () - 2 * border), radius, radius, Qt::AbsoluteSize);
    }

    {
      QPainter painter (&s);
      painter.setCompositionMode (QPainter::CompositionMode_DestinationIn);
      painter.drawImage (0, 0, smask);
    }

    return s;

  } else {
    return QTextBrowser::loadResource (type, url);
  }
}

static void produce_listing (QTextStream &stream, QDir dir, int level)
{
  for (int i = 0; i < level + 1; ++i) {
    stream << "<img src=\":/empty_12px.png\"/>&nbsp;&nbsp;";
  }
  stream << "<img src=\":/folder_12px.png\"/>&nbsp;&nbsp;<i>";
  if (level > 0) {
    stream << tl::escaped_to_html (tl::to_string (dir.dirName ())).c_str ();
  } else {
    stream << tl::escaped_to_html (tl::to_string (dir.absolutePath ())).c_str ();
  }
  stream << "</i><br/>\n";

  level += 1;

  QStringList entries = dir.entryList (QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    QFileInfo fi (dir.filePath (*e));
    if (fi.isDir ()) {
      produce_listing (stream, QDir (fi.filePath ()), level);
    } else {
      for (int i = 0; i < level + 1; ++i) {
        stream << "<img src=\":/empty_12px.png\"/>&nbsp;&nbsp;";
      }
      stream << "<img src=\":/file_12px.png\"/>&nbsp;&nbsp;" << tl::escaped_to_html (tl::to_string (*e)).c_str () << "<br/>\n";
    }

  }
}

QString
SaltGrainDetailsTextWidget::details_text ()
{
  SaltGrain *g = mp_grain.get ();
  if (! g) {
    return QString ();
  }

  QBuffer buffer;
  buffer.open (QIODevice::WriteOnly);
  QTextStream stream (&buffer);
#if QT_VERSION >= 0x60000
  stream.setEncoding (QStringConverter::Utf8);
#else
  stream.setCodec ("UTF-8");
#endif

  stream << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head><body>";

  stream << "<table cellpadding=\"6\"><tr>";
  stream << "<td><img src=\":/icon\" width=\"64\" height=\"64\"/></td>";

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
  if (! g->license ().empty ()) {
    stream << "<b>" << QObject::tr ("License") << ":</b> " << tl::to_qstring (tl::escaped_to_html (g->license ())) << " ";
  } else {
    stream << "<i><font color='gray'>";
    stream << QObject::tr ("This package does not have license information. "
                           "Use the &lt;license&gt; elements of the specification file or edit the package properties to provide license information.");
    stream << "</font></i>";
  }
  stream << "</p>";

  stream << "<p>";
  if (! g->api_version ().empty ()) {
    stream << "<b>" << QObject::tr ("API version and features") << ":</b> " << tl::to_qstring (tl::escaped_to_html (g->api_version ())) << " ";
  }
  stream << "</p>";

  stream << "<p>";
  if (! g->doc_url ().empty ()) {
    stream << "<b>" << QObject::tr ("Documentation link") << ":</b> <a href=\"" << tl::to_qstring (g->eff_doc_url ()) << "\">" << tl::to_qstring (tl::escaped_to_html (g->eff_doc_url ())) << "</a>";
  } else {
    stream << "<i><font color='gray'>";
    stream << QObject::tr ("This package does not have a documentation link. "
                           "Use the &lt;doc-url&gt; element of the specification file or edit the package properties to provide a link.");
    stream << "</font></i>";
  }
  stream << "</p>";

  if (! g->screenshot ().isNull ()) {
    stream << "<br/>";
    stream << "<h3>" << QObject::tr ("Screenshot") << "</h3><p><img src=\":/screenshot\"/></p>";
  }

  if (m_detailed_view) {

    stream << "<br/>";
    stream << "<h3>" << QObject::tr ("Installation") << "</h3>";

    if (! g->url ().empty ()) {
      stream << "<p><b>" << QObject::tr ("Download URL: ") << "</b>" << tl::to_qstring (tl::escaped_to_html (g->url ())) << "</p>";
    }
    if (! g->path ().empty () && ! g->installed_time ().isNull ()) {
      stream << "<p><b>" << QObject::tr ("Installed: ") << "</b>" << g->installed_time ().toString () << "</p>";
    }
    if (! g->dependencies ().empty ()) {
      stream << "<p><b>" << QObject::tr ("Depends on: ") << "</b><br/>";
      for (std::vector<lay::SaltGrainDependency>::const_iterator d = g->dependencies ().begin (); d != g->dependencies ().end (); ++d) {
        stream << "&nbsp;&nbsp;&nbsp;&nbsp;" << tl::to_qstring (tl::escaped_to_html (d->name)) << " ";
        stream << tl::to_qstring (tl::escaped_to_html (d->version));
        if (! d->url.empty ()) {
          stream << " - ";
          stream << "[" << tl::to_qstring (tl::escaped_to_html (d->url)) << "]<br/>";
        }
      }
      stream << "</p>";
    }

    if (! g->path ().empty ()) {
      stream << "<p><b>" << QObject::tr ("Installed files: ") << "</b></p><p>";
      produce_listing (stream, QDir (tl::to_qstring (g->path ())), 0);
      stream << "</p>";
    }

  }

  stream << "</td></tr></table>";

  stream << "</body></html>";

  stream.flush ();

  return QString::fromUtf8 (buffer.buffer());
}

}
