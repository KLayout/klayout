
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


#include "layHelpSource.h"
#include "layApplication.h"
#include "layVersion.h"

#include "tlLog.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "tlString.h"
#include "tlXMLParser.h"

#include "gsiDecl.h"

#include <set>
#include <string>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <fstream>

#include <QColor>
#include <QRegExp>
#include <QPalette>
#include <QApplication>
#include <QDir>
#include <QResource>
#include <QDomElement>
#include <QXmlStreamWriter>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

namespace lay
{

static const std::string index_url ("/index.xml");
static const std::string search_url ("/search.xml");

// --------------------------------------------------------------------------------------
//  Some utilities

static QString class_doc_path (const QString &c)
{
  QRegExp cm_re (QString::fromUtf8 ("^(.*)#(.*)$"));
  if (cm_re.indexIn (c) == 0) {
    QString cn = cm_re.cap (1);
    cn.replace (QString::fromUtf8 ("::"), QString::fromUtf8 ("_"));
    return QString::fromUtf8 ("/code/class_") + cn +  QString::fromUtf8 (".xml#m_") + cm_re.cap (2);
  } else {
    QString cn = c;
    cn.replace (QString::fromUtf8 ("::"), QString::fromUtf8 ("_"));
    return QString::fromUtf8 ("/code/class_") + cn + QString::fromUtf8 (".xml");
  }
}

static QString class_title (const QString &c)
{
  return c;
}

std::string escape_xml (const std::string &s)
{
  std::string r;
  r.reserve (s.size ());
  for (const char *sc = s.c_str (); *sc; ++sc) {
    if (*sc == '<') {
      r += "&lt;";
    } else if (*sc == '>') {
      r += "&gt;";
    } else if (*sc == '&') {
      r += "&amp;";
    } else {
      r += *sc;
    }
  }
  return r;
}

/**
 *  @brief A modified Levenshtein distance for determining in a fuzzy way whether a string is contained in another
 */
static int 
search_pattern_distance (const std::string &a, const std::string &b)
{
  std::vector<int> row0, row1;
  row0.resize (a.size () + 1, 0);
  row1.resize (a.size () + 1, 0);

  for (int i = 0; i <= int (a.size ()); ++i) {
    row0[i] = 0;
  }

  for (int i = 0; i < int (b.size ()); ++i) {

    row1[0] = i + 1;

    for (int j = 0; j < int (a.size ()); ++j) {
      int cost = (b[i] == a[j] ? 0 : 1);
      row1[j + 1] = std::min (row0[j] + cost, std::min (row0[j + 1], row1[j]) + 1);
    }

    row0.swap (row1);

  }

  int res = row0 [0];
  for (int i = 1; i <= int (a.size ()); ++i) {
    res = std::min (res, row0 [i]);
  }
  return res;
}

struct EditDistanceSorter 
{
  EditDistanceSorter (const std::string &subject, const std::vector<IndexEntry> &index)
    : m_subject (subject), m_index (index)
  {
    // .. nothing yet ..
  }

  bool operator() (int a, int b) const
  {
    int d1 = search_pattern_distance (m_index[a].normalized_key, m_subject);
    int d2 = search_pattern_distance (m_index[b].normalized_key, m_subject);
    if (d1 == d2) {
      return m_index[a].normalized_key.size () < m_index[b].normalized_key.size ();
    } else {
      return d1 < d2;
    }
  }

private:
  const std::string &m_subject;
  const std::vector<IndexEntry> &m_index;
};

struct StringLengthSorter 
{
  StringLengthSorter (const std::vector<IndexEntry> &index)
    : m_index (index)
  {
    // .. nothing yet ..
  }

  bool operator() (int a, int b) const
  {
    return (m_index[a].normalized_key.size () < m_index[b].normalized_key.size ());
  }

private:
  const std::vector<IndexEntry> &m_index;
};

QString relative_url (const std::string &doc, const QString &target)
{
  QUrl udoc = QUrl::fromEncoded (doc.c_str ());
  QUrl utarget = QUrl::fromEncoded (target.toUtf8 ());
  QString pdoc = udoc.path ();
  QString ptarget = utarget.path ();

  utarget.setPath (QFileInfo (pdoc).dir ().relativeFilePath (ptarget));
  return utarget.toString ();
}

// --------------------------------------------------------------------------------------
//  IndexEntry implementation

IndexEntry::IndexEntry (const std::string &_key, const std::string &_title, const std::string &_path)
  : key (_key), title (_title), path (_path)
{
  normalized_key = tl::to_string (tl::to_qstring (_key).toLower ());
}

// --------------------------------------------------------------------------------------
//  Implementation of HelpSource

static QString class_doc_element = QString::fromUtf8 ("class_doc");
static QString doc_element = QString::fromUtf8 ("doc");
static QString h2_element = QString::fromUtf8 ("h2");
static QString h2_index_element = QString::fromUtf8 ("h2-index");
static QString h3_element = QString::fromUtf8 ("h3");
static QString href_attribute = QString::fromUtf8 ("href");
static QString name_attribute = QString::fromUtf8 ("name");
static QString title_attribute = QString::fromUtf8 ("title");
static QString img_element = QString::fromUtf8 ("img");
static QString a_element = QString::fromUtf8 ("a");
static QString inline_keyword_element = QString::fromUtf8 ("k");
static QString keyword_element = QString::fromUtf8 ("keyword");
static QString link_element = QString::fromUtf8 ("link");
static QString menu_element = QString::fromUtf8 ("menu");
static QString mi_element = QString::fromUtf8 ("mi");
static QString src_attribute = QString::fromUtf8 ("src");
static QString title_element = QString::fromUtf8 ("title");
static QString topic_ref_element = QString::fromUtf8 ("topic-ref");
static QString topic_element = QString::fromUtf8 ("topic");
static QString topics_element = QString::fromUtf8 ("topics");

/**
 *  @brief A specialization of tl::make_element that is capable of taking a std::map::const_iterator 
 *
 *  The original tl::make_element gives a compiler warning (taking address of temporary)
 *  TODO: consolidate this version and tl::make_element.
 */
template <class Value, class Iter, class Parent>
tl::XMLElement<Value, Parent, tl::XMLMemberIterReadAdaptor <Value, Iter, Parent>, tl::XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element_iter (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, const tl::XMLElementList &children)
{
  return tl::XMLElement<Value, Parent, tl::XMLMemberIterReadAdaptor <Value, Iter, Parent>, tl::XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          tl::XMLMemberIterReadAdaptor <Value, Iter, Parent> (begin, end), 
          tl::XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

static const tl::XMLStruct <HelpSource>
help_index_structure ("help-index", 
  tl::make_member<std::string, HelpSource> (&HelpSource::klayout_version, &HelpSource::set_klayout_version, "program-version") +
  tl::make_element<IndexEntry, std::vector<IndexEntry>::const_iterator, HelpSource> (&HelpSource::begin_index, &HelpSource::end_index, &HelpSource::push_index, "index",
    tl::make_member<std::string, IndexEntry> (&IndexEntry::key, "literal-key") +
    tl::make_member<std::string, IndexEntry> (&IndexEntry::normalized_key, "key") +
    tl::make_member<std::string, IndexEntry> (&IndexEntry::title, "title") +
    tl::make_member<std::string, IndexEntry> (&IndexEntry::path, "path") 
  ) +
  make_element_iter<std::pair<std::string, std::string>, std::map<std::string, std::string>::const_iterator, HelpSource> (&HelpSource::begin_parents, &HelpSource::end_parents, &HelpSource::push_parent, "parent",
    tl::make_member<std::string, std::pair<std::string, std::string> > (&std::pair<std::string, std::string>::first, "path") +
    tl::make_member<std::string, std::pair<std::string, std::string> > (&std::pair<std::string, std::string>::second, "parent") 
  ) +
  make_element_iter<std::pair<std::string, std::string>, std::vector<std::pair<std::string, std::string> >::const_iterator, HelpSource> (&HelpSource::begin_titles, &HelpSource::end_titles, &HelpSource::push_title, "title",
    tl::make_member<std::string, std::pair<std::string, std::string> > (&std::pair<std::string, std::string>::first, "path") +
    tl::make_member<std::string, std::pair<std::string, std::string> > (&std::pair<std::string, std::string>::second, "title") 
  ) 
);

HelpSource::HelpSource ()
  : m_kindex (0)
{
  initialize_index ();
}

HelpSource::HelpSource (bool make_index)
  : m_kindex (0)
{
  if (make_index) {
    initialize_index ();
  }
}

void
HelpSource::initialize_index ()
{
  try {

    tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (QObject::tr ("Initializing help index")));

    bool ok = false;

    int qt = int (QT_VERSION >> 16);
    QString help_index_cache_file = tl::to_qstring (tl::sprintf ("help-index-%s-qt%d.xml.gz", lay::Version::version (), qt));

    std::string per_user_cache_file;
    if (! lay::ApplicationBase::instance ()->appdata_path ().empty ()) {
      per_user_cache_file = tl::to_string (QDir (tl::to_qstring (lay::ApplicationBase::instance ()->appdata_path ())).absoluteFilePath (help_index_cache_file));
    }

    //  Try to obtain the help index from the installation or application path

    std::vector<std::string> cache_files;
    cache_files.push_back (tl::to_string (QDir (tl::to_qstring (lay::ApplicationBase::instance ()->inst_path ())).absoluteFilePath (help_index_cache_file)));
    if (! per_user_cache_file.empty ()) {
      cache_files.push_back (per_user_cache_file);
    }

    for (std::vector<std::string>::const_iterator c = cache_files.begin (); ! ok && c != cache_files.end (); ++c) {

      try {
        tl::XMLFileSource in (*c);
        help_index_structure.parse (in, *this);
        if (m_klayout_version == lay::ApplicationBase::version ()) {
          if (tl::verbosity () >= 10) {
            tl::info << tl::to_string (tr ("Help index initialized from ")) << *c;
          }
          ok = true;
        } else {
          if (tl::verbosity () >= 10) {
            tl::warn << tl::to_string (tr ("Help index ignored (wrong version) from ")) << *c;
          }
        }
      } catch (tl::Exception &ex) {
        tl::warn << ex.msg ();
      } catch (std::runtime_error &ex) {
        tl::warn << ex.what ();
      } catch (...) {
        tl::warn << "unknown error.";
      }

    }

    if (! ok && ! per_user_cache_file.empty ()) {
      //  If no index is found, create one in "per_user_cache_file"
      produce_index_file (per_user_cache_file);
    }

  } catch (tl::Exception &ex) {

    m_index.clear ();
    m_titles.clear ();
    m_title_map.clear ();
    m_parent_of.clear ();

    tl::error << ex.msg ();

  }
}

HelpSource::~HelpSource()
{
  //  .. nothing yet ..
}

void
HelpSource::produce_index_file (const std::string &path)
{
  scan ();

  try {

    tl::OutputStream os (path, tl::OutputStream::OM_Zlib);
    help_index_structure.write (os, *this);

  } catch (tl::Exception &ex) {
    tl::warn << ex.msg ();
  } catch (std::runtime_error &ex) {
    tl::warn << ex.what ();
  } catch (...) {
    tl::warn << "unknown error.";
  }
}

void
HelpSource::create_index_file (const std::string &path)
{
  HelpSource source (false);
  source.produce_index_file (path);
}

void
HelpSource::scan ()
{
  m_index.clear ();
  m_titles.clear ();
  m_title_map.clear ();
  m_parent_of.clear ();

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Initializing help index")), 1);
  scan (index_url, progress);
}

std::string
HelpSource::klayout_version () const
{
  return lay::ApplicationBase::version ();
}

void
HelpSource::scan (const std::string &path, tl::AbsoluteProgress &progress)
{
  if (tl::verbosity () >= 20) {
    tl::info << "Help provider: scanning contents for " << path;
  }

  ++progress;

  m_kindex = 0;

  QDomDocument doc = get_dom (path);

  std::vector<std::string> subtopics;
  std::string title, section;
  scan (doc.documentElement (), path, subtopics, title, section);

  if (! title.empty ()) {
    m_titles.push_back (std::make_pair (path, title));
  }

  for (std::vector<std::string>::const_iterator st = subtopics.begin (); st != subtopics.end (); ++st) {
    scan (*st, progress);
  }
}

void
HelpSource::scan_child_nodes (const QDomElement &element, const std::string &path, std::vector<std::string> &subtopics, std::string &title, std::string &section)
{
  if (element.isNull ()) {
    return;
  }
  for (QDomNode n = element.firstChild (); ! n.isNull (); n = n.nextSibling ()) {
    if (n.isElement ()) {
      scan (n.toElement (), path, subtopics, title, section);
    }
  }
}

void
HelpSource::scan (const QDomElement &element, const std::string &path, std::vector<std::string> &subtopics, std::string &title, std::string &section)
{
  if (element.localName () == topic_ref_element) {

    //  remember topic
    std::string href = tl::to_string (element.attribute (href_attribute, QString ()));
    m_parent_of.insert (std::make_pair (href, path));
    subtopics.push_back (href);

  } else if (element.localName () == topic_element) {

    //  remember topic
    std::string href = tl::to_string (element.attribute (href_attribute, QString ()));
    m_parent_of.insert (std::make_pair (href, path));
    subtopics.push_back (href);

  } else if (element.localName () == keyword_element) {

    //  remember that location 
    ++m_kindex;
    QString name = element.attribute (name_attribute, QString ());
    QString title_attr = element.attribute (title_attribute, QString ());
    std::string t;
    if (! title_attr.isEmpty ()) {
      t = tl::to_string (title_attr);
    } else {
      t = title;
      if (! section.empty ()) {
        t += " - " + section;
      }
    }
    m_index.push_back (IndexEntry (tl::to_string (name), t, path + "#k_" + tl::to_string (m_kindex)));

  } else if (element.localName () == inline_keyword_element) {

    //  remember that location 
    ++m_kindex;
    std::string t = title;
    if (! section.empty ()) {
      t += " - " + section;
    }
    m_index.push_back (IndexEntry (tl::to_string (element.text ()), t, path + "#k_" + tl::to_string (m_kindex)));

  } else if (element.localName () == title_element) {

    //  remember title for later reference
    title = tl::to_string (element.text ());

  } else if (element.localName () == h2_element) {

    //  remember title for later reference
    section = tl::to_string (element.text ());

  } else {
    scan_child_nodes (element, path, subtopics, title, section);
  }
}

QDomDocument
HelpSource::get_dom (const std::string &u)
{
  QUrl url = QUrl::fromEncoded (u.c_str ());
  QString path = url.path ();

  for (tl::Registrar<lay::HelpProvider>::iterator cls = tl::Registrar<lay::HelpProvider>::begin (); cls != tl::Registrar<lay::HelpProvider>::end (); ++cls) {
    if (path.startsWith (tl::to_qstring ("/" + cls->folder (this) + "/"))) {
      if (tl::verbosity () >= 20) {
        tl::info << "Help provider: create content for " << u;
      }
      return cls->get (this, u);
    }
  }

  if (path == tl::to_qstring (search_url)) {
#if QT_VERSION >= 0x050000
    return produce_search (tl::to_string (QUrlQuery (url.query ()).queryItemValue (QString::fromUtf8 ("string")).toLower ()));
#else
    return produce_search (tl::to_string (url.queryItemValue (QString::fromUtf8 ("string")).toLower ()));
#endif
  } else if (path == tl::to_qstring (index_url)) {
    if (tl::verbosity () >= 20) {
      tl::info << "Help provider: create content for " << u;
    }
    return produce_main_index ();
  } else {
    tl::error << "Help provider: no content for " << u;
    return QDomDocument ();
  }
}

static QString resource_url (const QString &u)
{
  QUrl url (u);
  return QString::fromUtf8 (":/help") + url.path ();
}

QImage
HelpSource::get_image (const std::string &u)
{
  QResource res (resource_url (QUrl::fromEncoded (u.c_str ()).path ()));
  if (res.size () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No data found for resource ")) + u);
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

  return QImage::fromData (data);
}

std::string 
HelpSource::get_css (const std::string &u)
{
  std::ifstream t (tl::to_string (QDir (tl::to_qstring (lay::ApplicationBase::instance()->inst_path ())).absoluteFilePath (QString::fromUtf8 ("help_format.css"))).c_str ());
  if (t.good ()) {
    std::string c;
    while (t.good ()) {
      std::string l;
      std::getline (t, l);
      c += l + "\n";
    }
    return c;
  }

  QResource res (resource_url (QUrl::fromEncoded (u.c_str ()).path ()));
  if (res.size () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No data found for resource ")) + u);
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

  return std::string (data.constData (), data.size ());
}

std::string 
HelpSource::get (const std::string &u)
{
  BrowserOutline ol;
  return process (get_dom (u), u, ol);
}

BrowserOutline
HelpSource::get_outline (const std::string &u)
{
  BrowserOutline ol;
  process (get_dom (u), u, ol);
  return ol;
}

void
HelpSource::search_completers (const std::string &string, std::list<std::string> &completers)
{
  size_t n = 0;
  const size_t max_completers = 100;

  //  first produce all hits with match
  for (std::vector <IndexEntry>::const_iterator i = m_index.begin (); i < m_index.end () && n < max_completers; ++i) {
    if (i->normalized_key.find (string) != std::string::npos) {
      completers.push_back (i->key);
      ++n;
    }
  }
}

std::string
HelpSource::next_topic (const std::string &url)
{
  std::string u = tl::to_string (QUrl::fromEncoded (url.c_str ()).path ());
  for (size_t t = m_titles.size (); t > 0; ) {
    --t;
    if (m_titles [t].first == u) {
      if (t + 1 >= m_titles.size ()) {
        return std::string ();
      } else {
        return "int:" + m_titles [t + 1].first;
      }
    }
  }
  return std::string ();
}

std::string  
HelpSource::prev_topic (const std::string &url)
{
  std::string u = tl::to_string (QUrl::fromEncoded (url.c_str ()).path ());
  for (size_t t = 1; t < m_titles.size (); ++t) {
    if (m_titles [t].first == u) {
      return "int:" + m_titles [t - 1].first;
    }
  }
  return std::string ();
}

QDomDocument
HelpSource::produce_main_index ()
{
  std::ostringstream os;

  os << "<doc><title>" << tl::to_string (QObject::tr ("Main Index")) << "</title>" << std::endl;

  os << "<p>" << tl::to_string (QObject::tr ("Welcome to KLayout's documentation")) << "</p>" << std::endl;
  os << "<p>" << tl::to_string (QObject::tr (
    "The documentation is organized in chapters.\n"
    "For a brief introduction read the User Manual. 'Various Topics' is a collection of brief articles about specific topics.\n"
    "For Ruby programming see the 'Programming Ruby Scripts' chapter and for a complete Ruby class reference see the 'Class Index'.\n"
  ));
  os << "</p>" << std::endl;

  os << "<topics>" << std::endl;
  for (tl::Registrar<lay::HelpProvider>::iterator cls = tl::Registrar<lay::HelpProvider>::begin (); cls != tl::Registrar<lay::HelpProvider>::end (); ++cls) {
    os << "<topic href=\"" << cls->index (this) << "\"/>" << std::endl;
  }
  os << "</topics>" << std::endl;

  os << "</doc>" << std::endl;

  std::string text = os.str ();

  QDomDocument doc;
  QString errorMsg;
  int errorLine = 0 ;
  if (! doc.setContent (QByteArray (text.c_str (), int (text.size ())), true, &errorMsg, &errorLine)) {
    throw tl::Exception (tl::to_string (errorMsg) + ", in line " + tl::to_string (errorLine) + " of main index");
  }
  
  return doc;
}

QDomDocument
HelpSource::produce_search (const std::string &string)
{
  std::ostringstream os;

  os << "<doc><title>" << tl::to_string (QObject::tr ("Search results for")) << " \"" << escape_xml (string) << "\"</title>" << std::endl;

  os << "<h2>" << tl::to_string (QObject::tr ("Search results for")) << " \"" << escape_xml (string) << "\"</h2><p/>" << std::endl;

  std::vector <int> exact_hit_indices;

  int n = 0;
  //  first produce all hits with match
  for (std::vector <IndexEntry>::const_iterator i = m_index.begin (); i < m_index.end (); ++i, ++n) {
    if (i->normalized_key.find (string) != std::string::npos) {
      exact_hit_indices.push_back (n);
    }
  }

  if (! exact_hit_indices.empty ()) {

    if (exact_hit_indices.size () > 1) {
      os << "<p>" << exact_hit_indices.size () << " " << tl::to_string (QObject::tr ("exact hits found")) << "</p>" << std::endl;
    } else {
      os << "<p>" << tl::to_string (QObject::tr ("One exact hit found")) << "</p>" << std::endl;
    }

    os << "<table>" << std::endl;

    std::sort (exact_hit_indices.begin (), exact_hit_indices.end (), StringLengthSorter (m_index));

    int max_n = 100;

    n = 0;
    for (std::vector <int>::const_iterator i = exact_hit_indices.begin (); i < exact_hit_indices.end () && n < max_n; ++i, ++n) {

      const IndexEntry &ie = m_index[*i];
      size_t f = ie.normalized_key.find (string);

      os << "<tr>" << std::endl;
      os << "<td>" << escape_xml (std::string (ie.key, 0, f)) << "<b>" << escape_xml (std::string (ie.key, f, string.size ())) << "</b>" << escape_xml (std::string (ie.key, f + string.size ())) << "</td>" << std::endl;
      os << "<td><a href=\"" << ie.path << "\"><nobr>" << escape_xml (ie.title) << "</nobr></a></td>" << std::endl;
      os << "</tr>" << std::endl;

    }

    if (int (exact_hit_indices.size ()) >= max_n) {
      os << "<tr><td>...</td></tr>" << std::endl;
    }

    os << "</table>" << std::endl;

  } else {

    std::vector <int> indices;
    indices.reserve (m_index.size ());
    for (int i = 0; i < int (m_index.size ()); ++i) {
      indices.push_back (i);
    }

    int max_n = 20;

    if (int (indices.size ()) > max_n) {
      std::partial_sort (indices.begin (), indices.begin () + max_n, indices.end (), EditDistanceSorter (string, m_index));
    } else {
      std::sort (indices.begin (), indices.end (), EditDistanceSorter (string, m_index));
    }

    //  Then produce all similar hits if no exact match was found
    n = 0;
    for (std::vector <int>::const_iterator i = indices.begin (); i < indices.end () && n < max_n; ++i) {

      const IndexEntry &ie = m_index[*i];
      size_t f = ie.normalized_key.find (string);

      if (f == std::string::npos) {
        if (n == 0) {
          os << "<table>" << std::endl;
        }
        os << "<tr>" << std::endl;
        os << "<td>" << escape_xml (ie.key) << "</td>" << std::endl;
        os << "<td><a href=\"" << ie.path << "\"><nobr>" << escape_xml (ie.title) << "</nobr></a></td>" << std::endl;
        os << "</tr>" << std::endl;
        ++n;
      }

    }

    if (n > 0) {
      os << "</table>" << std::endl;
    }

  }

  os << "</doc>" << std::endl;

  std::string text = os.str ();

  QDomDocument doc;
  QString errorMsg;
  int errorLine = 0 ;
  if (! doc.setContent (QByteArray (text.c_str (), int (text.size ())), true, &errorMsg, &errorLine)) {
    throw tl::Exception (tl::to_string (errorMsg) + ", in line " + tl::to_string (errorLine) + " of main index");
  }
  
  return doc;
}

const std::string & 
HelpSource::parent_of (const std::string &path) 
{
  std::map<std::string, std::string>::const_iterator t = m_parent_of.find (path);
  if (t != m_parent_of.end ()) {
    return t->second;
  } else {
    static std::string empty;
    return empty;
  }
}

std::string  
HelpSource::process (const QDomDocument &doc, const std::string &path, BrowserOutline &ol)
{
  QBuffer output;
  output.open (QIODevice::WriteOnly);

  m_kindex = 0;

  QXmlStreamWriter writer (&output);
  writer.writeStartDocument (QString::fromUtf8 ("1.0"));
  process (doc.documentElement (), path, writer, ol);
  writer.writeEndDocument ();

  output.close ();

  return std::string (output.data ().constData (), output.data ().size ());
}

void
HelpSource::process_child_nodes (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &ol)
{
  if (element.isNull ()) {
    return;
  }

  for (QDomNode n = element.firstChild (); ! n.isNull (); n = n.nextSibling ()) {
    if (n.isElement ()) {
      process (n.toElement (), path, writer, ol);
    } else if (n.isComment ()) {
      //  ignore
    } else if (n.isCDATASection ()) {
      writer.writeCDATA (n.toCDATASection ().data ());
    } else if (n.isCharacterData ()) {
      writer.writeCharacters (n.toCharacterData ().data ());
    }
  }
}

void
HelpSource::writeElement (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &ol)
{
  //  simply pass all other elements
  writer.writeStartElement (element.nodeName ());

  if (element.hasAttributes ()) {
    //  Hint: attribute nodes are not children of the elements ..
    QDomNamedNodeMap attributes = element.attributes ();
    for (int i = 0; i < attributes.count (); ++i) {
      QDomAttr a = attributes.item (i).toAttr ();
      if (! a.isNull ()) {
        writer.writeAttribute (a.nodeName (), a.value ());
      }
    }
  }

  process_child_nodes (element, path, writer, ol);

  writer.writeEndElement ();
}

static void
add_outline_at_level (int i, BrowserOutline &ol, const BrowserOutline &child)
{
  if (i == 0) {
    ol.add_child (child);
  } else if (ol.begin () != ol.end ()) {
    add_outline_at_level (i - 1, *(--ol.end ()), child);
  } else {
    ol.add_child (BrowserOutline (tl::to_string (QObject::tr ("(empty")), std::string ()));
    add_outline_at_level (i - 1, *(--ol.end ()), child);
  }
}

void
HelpSource::process (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &ol)
{
  if (element.localName () == keyword_element) {

    //  insert an anchor (must align with implementation of scan_index):
    ++m_kindex;
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("name"), QString::fromUtf8 ("k_") + QString::number (m_kindex));
    writer.writeEndElement ();

    //  drop <keyword>..</keyword>

  } else if (element.localName () == inline_keyword_element) {

    //  insert an anchor (must align with implementation of scan_index):
    ++m_kindex;
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("name"), QString::fromUtf8 ("k_") + QString::number (m_kindex));
    writer.writeEndElement ();

    //  replace <k>..</k> by content
    process_child_nodes (element, path, writer, ol);

  } else if (element.localName () == h2_index_element) {

    //  replace "h2-index" by "<ul><li>...</li></ul>" with an index
    writer.writeStartElement (QString::fromUtf8 ("ul"));
    QDomNodeList index = element.ownerDocument ().documentElement ().elementsByTagName (h2_element);
    for (int i = 0; i != index.count (); ++i) {
      writer.writeStartElement (QString::fromUtf8 ("li"));
      writer.writeStartElement (QString::fromUtf8 ("a"));
      writer.writeAttribute (QString::fromUtf8 ("href"), QString::fromUtf8 ("#") + index.at (i).localName () + QString::fromUtf8 ("-") + QString::number (index.at (i).lineNumber ()));
      writer.writeCharacters (index.at (i).toElement ().text ());
      writer.writeEndElement ();
      writer.writeEndElement ();
    }
    writer.writeEndElement ();

  } else if (element.localName () == h2_element || element.localName () == h3_element) {

    int level = (element.localName () == h2_element ? 1 : 2);

    QString name = element.localName () + QString::fromUtf8 ("-") + QString::number (element.lineNumber ());
    QString title = element.text ();

    std::string path_wo_anchor = path;
    std::string::size_type n;
    if ((n = path.rfind ("#")) != std::string::npos) {
      path_wo_anchor = std::string (path, 0, n);
    }

    add_outline_at_level (level, ol, BrowserOutline (tl::to_string (title), path_wo_anchor + "#" + tl::to_string (name)));

    //  replace "h2"/"h3" by "<a name='hx-line-no'/><h2>"
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("name"), name);
    writer.writeEndElement ();
    writer.writeStartElement (element.localName ());
    process_child_nodes (element, path, writer, ol);
    writer.writeEndElement ();

  } else if (element.localName () == title_element) {

    add_outline_at_level (0, ol, BrowserOutline (tl::to_string (element.text ()), path));

    //  replace "title" by "h1"
    writer.writeStartElement (QString::fromUtf8 ("h1"));
    process_child_nodes (element, path, writer, ol);
    writer.writeEndElement ();

  } else if (element.localName () == doc_element) {

    //  replace "doc" by "<html><head><link rel='stylesheet' type='text/css' href='/css/help_format.css'><title>(document title</title></head><body>(content)</body></html>"
    //  and the navigator bar.
    QDomNodeList title_elements = element.elementsByTagName (title_element);
    QString title;
    if (! title_elements.isEmpty ()) {
      title = title_elements.item (0).toElement ().text ();
    }

    std::vector<std::pair<std::string, std::string> > pp;
    pp.push_back (std::make_pair (std::string (), tl::to_string (title)));

    std::string pu = parent_of (tl::to_string (QUrl::fromEncoded (path.c_str ()).path ()));
    while (! pu.empty ()) {
      pp.push_back (std::make_pair (pu, title_for (pu)));
      pu = parent_of (pu);
    }
    std::reverse (pp.begin (), pp.end ());

    writer.writeDTD (QString::fromUtf8 ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"));
    writer.writeDefaultNamespace (QString::fromUtf8 ("http://www.w3.org/1999/xhtml"));
    writer.writeStartElement (QString::fromUtf8 ("html"));
    writer.writeStartElement (QString::fromUtf8 ("head"));
    writer.writeStartElement (QString::fromUtf8 ("link"));
    writer.writeAttribute (QString::fromUtf8 ("rel"), QString::fromUtf8 ("stylesheet"));
    writer.writeAttribute (QString::fromUtf8 ("type"), QString::fromUtf8 ("text/css"));
    writer.writeAttribute (QString::fromUtf8 ("href"), QString::fromUtf8 ("/css/help_format.css"));
    writer.writeEndElement ();
    writer.writeTextElement (QString::fromUtf8 ("title"), title);
    writer.writeEndElement ();
    writer.writeStartElement (QString::fromUtf8 ("body"));
    writer.writeStartElement (QString::fromUtf8 ("p"));
    writer.writeAttribute (QString::fromUtf8 ("class"), QString::fromUtf8 ("navigator"));
    for (std::vector<std::pair<std::string, std::string> >::const_iterator p = pp.begin (); p != pp.end (); ++p) {
      if (p != pp.begin ()) {
        writer.writeCharacters (QString::fromUtf8 (" ") + QString (QChar (187)) + QString::fromUtf8 (" ")); // &raquo;
      }
      if (p->first.empty ()) {
        writer.writeCharacters (tl::to_qstring (p->second));
      } else {
        writer.writeStartElement (QString::fromUtf8 ("a"));
        writer.writeAttribute (QString::fromUtf8 ("href"), relative_url (path, tl::to_qstring (p->first)));
        writer.writeCharacters (tl::to_qstring (p->second));
        writer.writeEndElement ();
      }
    }
    writer.writeEndElement ();
    process_child_nodes (element, path, writer, ol);
    writer.writeEndElement ();
    writer.writeEndElement ();

  } else if (element.localName () == topics_element) {

    //  replace "topics" by "ul"
    writer.writeStartElement (QString::fromUtf8 ("ul"));
    process_child_nodes (element, path, writer, ol);
    writer.writeEndElement ();

  } else if (element.localName () == topic_ref_element) {

    //  drop "ref" element (hidden topic)

  } else if (element.localName () == topic_element) {

    //  replace "topic" by "li"
    std::string title;
    QString href = element.attribute (href_attribute, QString ());
    if (! href.isEmpty ()) {
      if (! element.text ().isEmpty ()) {
        title = tl::to_string (element.text ());
      } else {
        title = title_for (tl::to_string (href));
      }
    }

    //  replace "<topic href='...'>" by "<li><a href="(path)">(topic title></a></li>"
    writer.writeStartElement (QString::fromUtf8 ("li"));
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("href"), relative_url (path, href));
    writer.writeCharacters (tl::to_qstring (title));
    writer.writeEndElement ();
    writer.writeEndElement ();

  } else if (element.localName () == a_element) {

    QDomElement new_el = element;
    if (new_el.hasAttribute (href_attribute)) {
      new_el.setAttribute (href_attribute, relative_url (path, new_el.attribute (href_attribute)));
    }
    writeElement (new_el, path, writer, ol);

  } else if (element.localName () == img_element) {

    QDomElement new_el = element;
    if (new_el.hasAttribute (src_attribute)) {
      new_el.setAttribute (src_attribute, relative_url (path, new_el.attribute (src_attribute)));
    }
    writeElement (new_el, path, writer, ol);

  } else if (element.localName () == class_doc_element) {

    QString href = element.attribute (href_attribute, QString ());

    //  replace "<link href='...'>" by "<a href="(path)">class name></a>"
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("href"), relative_url (path, class_doc_path (href)));
    writer.writeCharacters (class_title (href));
    writer.writeEndElement ();

  } else if (element.localName () == link_element) {

    std::string title;
    QString href = element.attribute (href_attribute, QString ());
    if (! href.isEmpty ()) {
      if (! element.text ().isEmpty ()) {
        title = tl::to_string (element.text ());
      } else {
        title = title_for (tl::to_string (href));
      }
    }

    //  replace "<link href='...'>" by "<a href="(path)">(topic title></a>"
    writer.writeStartElement (QString::fromUtf8 ("a"));
    writer.writeAttribute (QString::fromUtf8 ("href"), relative_url (path, href));
    writer.writeCharacters (tl::to_qstring (title));
    writer.writeEndElement ();

  } else {

    //  simply pass all other elements
    writeElement (element, path, writer, ol);

  }
}

std::string HelpSource::title_for (const std::string &path)
{
  if (m_title_map.empty ()) {
    for (std::vector<std::pair<std::string, std::string> >::const_iterator t = m_titles.begin (); t != m_titles.end (); ++t) {
      m_title_map.insert (*t);
    }
  }

  std::map<std::string, std::string>::const_iterator t = m_title_map.find (path);
  if (t != m_title_map.end ()) {
    return t->second;
  } else {
    return std::string ();
  }
}

std::vector<std::string> 
HelpSource::urls ()
{
  std::vector<std::string> u;
  u.push_back (index_url);
  for (std::map<std::string, std::string>::const_iterator p = m_parent_of.begin (); p != m_parent_of.end (); ++p) {
    u.push_back (p->first);
  }
  return u;
}

void
HelpSource::set_option (const std::string &key, const tl::Variant &value)
{
  s_global_options[key] = value;
}

const tl::Variant &
HelpSource::get_option (const std::string &key) const
{
  auto i = s_global_options.find (key);
  if (i != s_global_options.end ()) {
    return i->second;
  } else {
    static tl::Variant nil;
    return nil;
  }
}

}

