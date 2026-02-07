
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


#include "layResourceHelpProvider.h"

#include "tlString.h"
#include "tlClassRegistry.h"

#include <QResource>
#include <QFileInfo>
#include <QUrl>

namespace lay
{

// --------------------------------------------------------------------------------------------
//  Implementation of the resource help provider

static QString resource_url (const QString &u)
{
  QUrl url (u);
  return QString::fromUtf8 (":/help") + url.path ();
}

ResourceHelpProvider::ResourceHelpProvider (const char *folder, const std::string &title)
  : m_folder (folder), m_title (title)
{
  // .. nothing yet ..
}

QDomDocument 
ResourceHelpProvider::get (lay::HelpSource * /*src*/, const std::string &path) const
{
  QString qpath = tl::to_qstring (path);
  QResource res (resource_url (qpath));
  if (res.size () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No data found for resource ")) + tl::to_string (res.fileName ()));
  }

  QByteArray data;
#if QT_VERSION >= 0x60000
  if (res.compressionAlgorithm () == QResource::ZlibCompression) {
#else
  if (res.isCompressed ()) {
#endif
    data = qUncompress ((const unsigned char *)res.data (), (int)res.size ());
  } else {
    data = QByteArray ((const char *)res.data (), (int)res.size ());
  }

  if (tl::verbosity () >= 20) {
    tl::info << "Help provider: create content for " << path;
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine = 0 ;
  if (! doc.setContent (data, true, &errorMsg, &errorLine)) {
    throw tl::Exception (tl::to_string (errorMsg) + ", in line " + tl::to_string (errorLine) + " of " + path);
  }
  
  return doc;
}

//  declare the resource-based help providers
static tl::RegisteredClass<lay::HelpProvider> manual_help_provider (new ResourceHelpProvider ("manual", tl::to_string (QObject::tr ("User Manual"))), 100);
static tl::RegisteredClass<lay::HelpProvider> about_help_provider (new ResourceHelpProvider ("about", tl::to_string (QObject::tr ("Various Topics and Detailed Information About Certain Features"))), 200);
static tl::RegisteredClass<lay::HelpProvider> programming_help_provider (new ResourceHelpProvider ("programming", tl::to_string (QObject::tr ("Programming Ruby Scripts"))), 300);

}

