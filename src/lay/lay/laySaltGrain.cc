
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

#include "laySaltGrain.h"
#include "laySaltController.h"
#include "laySaltParsedURL.h"
#include "tlString.h"
#include "tlXMLParser.h"
#include "tlHttpStream.h"
#include "tlFileUtils.h"
#include "tlWebDAV.h"
#if defined(HAVE_GIT2)
#  include "tlGit.h"
#endif

#include <memory>
#include <QDir>
#include <QFileInfo>
#include <QBuffer>
#include <QResource>
#include <QUrl>

namespace lay
{

static const std::string grain_filename = "grain.xml";

SaltGrain::SaltGrain ()
  : m_hidden (false)
{
  //  .. nothing yet ..
}

bool
SaltGrain::operator== (const SaltGrain &other) const
{
  return m_name == other.m_name &&
         m_path == other.m_path &&
         m_version == other.m_version &&
         m_api_version == other.m_api_version &&
         m_url == other.m_url &&
         m_title == other.m_title &&
         m_doc == other.m_doc &&
         m_doc_url == other.m_doc_url &&
         m_icon == other.m_icon &&
         m_screenshot == other.m_screenshot &&
         m_dependencies == other.m_dependencies &&
         m_author == other.m_author &&
         m_author_contact == other.m_author_contact &&
         m_license == other.m_license &&
         m_hidden == other.m_hidden &&
         m_authored_time == other.m_authored_time &&
         m_installed_time == other.m_installed_time
      ;
}

void
SaltGrain::set_name (const std::string &n)
{
  m_name = n;
}

void
SaltGrain::set_token (const std::string &t)
{
  m_token = t;
}

void
SaltGrain::set_hidden (bool f)
{
  m_hidden = f;
}

void
SaltGrain::set_version (const std::string &v)
{
  m_version = v;
}

void
SaltGrain::set_api_version (const std::string &v)
{
  m_api_version = v;
}

void
SaltGrain::set_path (const std::string &p)
{
  m_path = p;
}

void
SaltGrain::set_url (const std::string &u)
{
  m_url = u;
}

void
SaltGrain::set_title (const std::string &t)
{
  m_title = t;
}

void
SaltGrain::set_doc (const std::string &t)
{
  m_doc = t;
}

void
SaltGrain::set_doc_url (const std::string &u)
{
  m_doc_url = u;
}

std::string
SaltGrain::eff_doc_url () const
{
  if (m_doc_url.empty ()) {
    return std::string ();
  }

  QUrl url (tl::to_qstring (m_doc_url));
  if (! url.scheme ().isEmpty ()) {
    return m_doc_url;
  }

  //  force this to become a "file" (for Qt5)
  url.setScheme (QString::fromUtf8 ("file"));

  QString p = tl::to_qstring (path ());
  if (! p.isEmpty ()) {

    //  if the URL is a relative URL, make it absolute relative to the grain's installation directory
    QFileInfo fi (url.toLocalFile ());
    if (! fi.isAbsolute ()) {
      fi = QFileInfo (QDir (p).absoluteFilePath (fi.filePath ()));
    }

    //  if the resulting path is inside the downloaded package, use this path
    QString dp = fi.canonicalFilePath ();
    if (!dp.isEmpty () && tl::is_parent_path (tl::to_string (p), tl::to_string (dp))) {
      url = QUrl::fromLocalFile (dp);
      url.setScheme (tl::to_qstring ("file"));
      return tl::to_string (url.toString ());
    }

  }

  //  base the documentation URL on the download URL
  QUrl eff_url = QUrl (tl::to_qstring (m_url));
  eff_url.setPath (eff_url.path () + QString::fromUtf8 ("/") + url.path ());
  return tl::to_string (eff_url.toString ());
}

void
SaltGrain::set_author (const std::string &a)
{
  m_author = a;
}

void
SaltGrain::set_author_contact (const std::string &a)
{
  m_author_contact = a;
}

void
SaltGrain::set_license (const std::string &l)
{
  m_license = l;
}

void
SaltGrain::set_authored_time (const QDateTime &t)
{
  m_authored_time = t;
}

void
SaltGrain::set_installed_time (const QDateTime &t)
{
  m_installed_time = t;
}

void
SaltGrain::set_screenshot (const QImage &i)
{
  m_screenshot = i;
}

void
SaltGrain::set_icon (const QImage &i)
{
  m_icon = i;
}

int
SaltGrain::compare_versions (const std::string &v1, const std::string &v2)
{
  tl::Extractor ex1 (v1.c_str ());
  tl::Extractor ex2 (v2.c_str ());

  while (true) {

    if (ex1.at_end () && ex2.at_end ()) {
      return 0;
    }

    int n1 = 0, n2 = 0;
    if (! ex1.at_end ()) {
      ex1.try_read (n1);
    }
    if (! ex2.at_end ()) {
      ex2.try_read (n2);
    }

    if (n1 != n2) {
      return n1 < n2 ? -1 : 1;
    }

    while (! ex1.at_end ()) {
      char c = *ex1;
      ++ex1;
      if (c == '.') {
        break;
      }
    }

    while (! ex2.at_end ()) {
      char c = *ex2;
      ++ex2;
      if (c == '.') {
        break;
      }
    }

  }
}

const std::string &
SaltGrain::spec_file ()
{
  return grain_filename;
}

bool
SaltGrain::valid_name (const std::string &n)
{
  std::string res;

  tl::Extractor ex (n);

  //  a package name must not start with a dot.
  if (ex.test (".")) {
    return false;
  }

  std::string s;
  if (! ex.try_read_word (s, "_.-")) {
    return false;
  }
  res += s;

  while (! ex.at_end ()) {
    if (! ex.test ("/")) {
      return false;
    }
    //  a prefix must not start with a dot.
    if (ex.test (".")) {
      return false;
    }
    if (! ex.try_read_word (s, "_.-")) {
      return false;
    }
    res += "/";
    res += s;
  }

  //  this captures the cases where the extractor skips blanks
  //  TODO: the extractor should have a "non-blank-skipping" mode
  return res == n;
}

bool
SaltGrain::valid_api_version (const std::string &v)
{
  tl::Extractor ex (v.c_str ());

  while (! ex.at_end ()) {

    std::string feature;
    ex.try_read_name (feature);

    bool first = true;
    while (! ex.at_end () && ! ex.test (";")) {
      int n = 0;
      if (! first && ! ex.test (".")) {
        return false;
      }
      if (! ex.try_read (n)) {
        return false;
      }
      first = false;
    }

  }

  return true;
}

bool
SaltGrain::valid_version (const std::string &v)
{
  tl::Extractor ex (v.c_str ());

  while (! ex.at_end ()) {
    int n = 0;
    if (! ex.try_read (n)) {
      return false;
    }
    if (! ex.at_end ()) {
      if (*ex != '.') {
        return false;
      } else {
        ++ex;
      }
    }
  }

  return true;
}

struct TimeConverter
{
  std::string to_string (const QDateTime &time) const
  {
    if (time.isNull ()) {
      return std::string ();
    } else {
      return tl::to_string (time.toString (Qt::ISODate));
    }
  }

  void from_string (const std::string &time, QDateTime &res) const
  {
    if (time.empty ()) {
      res = QDateTime ();
    } else {
      res = QDateTime::fromString (tl::to_qstring (time), Qt::ISODate);
    }
  }
};

struct ImageConverter
{
  std::string to_string (const QImage &image) const
  {
    if (image.isNull ()) {
      return std::string ();
    } else {
      QBuffer buffer;
      buffer.open (QIODevice::WriteOnly);
      image.save (&buffer, "PNG");
      buffer.close ();
      return buffer.buffer ().toBase64 ().constData ();
    }
  }

  void from_string (const std::string &image, QImage &res) const
  {
    if (image.empty ()) {
      res = QImage ();
    } else {
      res = QImage::fromData (QByteArray::fromBase64 (QByteArray (image.c_str (), int (image.size ()))));
    }
  }
};

static tl::XMLElementList *sp_xml_elements = 0;

tl::XMLElementList &
SaltGrain::xml_elements ()
{
  if (! sp_xml_elements) {
    sp_xml_elements = new tl::XMLElementList (
      tl::make_member (&SaltGrain::name, &SaltGrain::set_name, "name") +
      tl::make_member (&SaltGrain::token, &SaltGrain::set_token, "token") +
      tl::make_member (&SaltGrain::is_hidden, &SaltGrain::set_hidden, "hidden") +
      tl::make_member (&SaltGrain::version, &SaltGrain::set_version, "version") +
      tl::make_member (&SaltGrain::api_version, &SaltGrain::set_api_version, "api-version") +
      tl::make_member (&SaltGrain::title, &SaltGrain::set_title, "title") +
      tl::make_member (&SaltGrain::doc, &SaltGrain::set_doc, "doc") +
      tl::make_member (&SaltGrain::doc_url, &SaltGrain::set_doc_url, "doc-url") +
      tl::make_member (&SaltGrain::url, &SaltGrain::set_url, "url") +
      tl::make_member (&SaltGrain::license, &SaltGrain::set_license, "license") +
      tl::make_member (&SaltGrain::author, &SaltGrain::set_author, "author") +
      tl::make_member (&SaltGrain::author_contact, &SaltGrain::set_author_contact, "author-contact") +
      tl::make_member (&SaltGrain::authored_time, &SaltGrain::set_authored_time, "authored-time", TimeConverter ()) +
      tl::make_member (&SaltGrain::installed_time, &SaltGrain::set_installed_time, "installed-time", TimeConverter ()) +
      tl::make_member (&SaltGrain::icon, &SaltGrain::set_icon, "icon", ImageConverter ()) +
      tl::make_member (&SaltGrain::screenshot, &SaltGrain::set_screenshot, "screenshot", ImageConverter ()) +
      tl::make_element (&SaltGrain::begin_dependencies, &SaltGrain::end_dependencies, &SaltGrain::add_dependency, "depends",
        tl::make_member (&SaltGrainDependency::name, "name") +
        tl::make_member (&SaltGrainDependency::url, "url") +
        tl::make_member (&SaltGrainDependency::version, "version")
      )
    );
  }

  return *sp_xml_elements;
}

static
tl::XMLStruct<lay::SaltGrain>
xml_struct ()
{
  return tl::XMLStruct<lay::SaltGrain> ("salt-grain", SaltGrain::xml_elements ());
}

bool
SaltGrain::is_readonly () const
{
  //  A grain is readonly if the directory is not writable or there is a download URL
  //  (this means the grain has been installed from an URL).
  return !QFileInfo (tl::to_qstring (path ())).isWritable () || !m_url.empty ();
}

void
SaltGrain::load (const std::string &p)
{
  tl_assert (!p.empty ());

  if (p[0] != ':') {

    tl::XMLFileSource source (p);
    xml_struct ().parse (source, *this);

  } else {

    QResource res (tl::to_qstring (p));
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

    std::string str_data (data.constData (), data.size ());
    tl::XMLStringSource source (str_data);
    xml_struct ().parse (source, *this);

  }
}

void
SaltGrain::load (tl::InputStream &p)
{
  tl::XMLStreamSource source (p);
  xml_struct ().parse (source, *this);
}

void
SaltGrain::save () const
{
  save (tl::to_string (QDir (tl::to_qstring (path ())).filePath (tl::to_qstring (SaltGrain::spec_file ()))));
}

void
SaltGrain::save (const std::string &p) const
{
  tl::OutputStream os (p, tl::OutputStream::OM_Plain);
  xml_struct ().write (os, *this);
}

SaltGrain
SaltGrain::from_path (const std::string &path)
{
  QDir dir (tl::to_qstring (path));

  SaltGrain g;
  g.load (tl::to_string (dir.filePath (tl::to_qstring (SaltGrain::spec_file ()))));
  g.set_path (tl::to_string (dir.absolutePath ()));
  return g;
}

tl::InputStream *
SaltGrain::stream_from_url (std::string &generic_url, double timeout, tl::InputHttpStreamCallback *callback)
{
  if (generic_url.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No download link available")));
  }

  if (tl::verbosity () >= 20) {
    tl::info << tr ("Downloading package info from ") << generic_url;
  }

  lay::SaltParsedURL purl (generic_url);
  const std::string &url = purl.url ();

  //  base relative URL's on the salt mine URL
  if (purl.protocol () == lay::DefaultProtocol && url.find ("http:") != 0 && url.find ("https:") != 0 && url.find ("file:") != 0 && !url.empty() && url[0] != '/' && url[0] != '\\' && lay::SaltController::instance ()) {

    //  replace the last component ("repository.xml") by the given path
    QUrl sami_url (tl::to_qstring (lay::SaltController::instance ()->salt_mine_url ()));
    QStringList path_comp = sami_url.path ().split (QString::fromUtf8 ("/"));
    if (!path_comp.isEmpty ()) {
      path_comp.back () = tl::to_qstring (url);
    }
    sami_url.setPath (path_comp.join (QString::fromUtf8 ("/")));

    //  return the full path as a file path, not an URL
    generic_url = tl::to_string (sami_url.toString ());

  }

  if (url.find ("http:") == 0 || url.find ("https:") == 0) {

    if (purl.protocol () == lay::Git) {
#if defined(HAVE_GIT2)
      return tl::GitObject::download_item (url, SaltGrain::spec_file (), purl.subfolder (), purl.branch (), timeout, callback);
#else
      throw tl::Exception (tl::to_string (QObject::tr ("Cannot download from Git - Git support not compiled in")));
#endif
    } else {
      return tl::WebDAVObject::download_item (url + "/" + SaltGrain::spec_file (), timeout, callback);
    }

  } else {

    return new tl::InputStream (url + "/" + SaltGrain::spec_file ());

  }
}

SaltGrain
SaltGrain::from_url (const std::string &url_in, double timeout, tl::InputHttpStreamCallback *callback)
{
  std::string url = url_in;
  std::unique_ptr<tl::InputStream> stream (stream_from_url (url, timeout, callback));

  SaltGrain g;
  g.load (*stream);
  g.set_url (url);
  return g;
}

bool
SaltGrain::is_grain (const std::string &path)
{
  tl_assert (! path.empty ());

  if (path[0] != ':') {
    QDir dir (tl::to_qstring (path));
    QString gf = dir.filePath (tl::to_qstring (SaltGrain::spec_file ()));
    return QFileInfo (gf).exists ();
  } else {
    return QResource (tl::to_qstring (path + "/" + SaltGrain::spec_file ())).isValid ();
  }
}

}
