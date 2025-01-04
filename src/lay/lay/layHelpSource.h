
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


#ifndef HDR_layHelpSource
#define HDR_layHelpSource

#include "layBrowserPanel.h"
#include "layHelpProvider.h"
#include "layCommon.h"
#include "tlProgress.h"
#include "tlClassRegistry.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <QBuffer>
#include <QUrl>
#include <QDomDocument>

class QXmlStreamWriter;

namespace lay
{

/**
 *  @brief Defines a entry in the index list
 *
 *  The Index entry consists of the key, a title and the URL.
 *
 *  The normalized key is the string that can be used for comparing.
 */
struct IndexEntry
{
  IndexEntry () { }
  IndexEntry (const std::string &_key, const std::string &_title, const std::string &_path);

  std::string key;
  std::string normalized_key;
  std::string title;
  std::string path;
};

/**
 *  @brief A utility function that escapes a HTML string
 */
std::string escape_xml (const std::string &s);

/**
 *  @brief A specialization of BrowserSource for delivering the generated documentation
 */
class LAY_PUBLIC HelpSource
  : public BrowserSource
{
public:
  HelpSource ();
  HelpSource (bool make_index);

  ~HelpSource();

  virtual std::string get (const std::string &url);
  virtual BrowserOutline get_outline (const std::string &url);
  virtual QImage get_image (const std::string &url);
  virtual std::string get_css (const std::string &url);

  virtual void search_completers(const std::string &search_string, std::list<std::string> &completers);

  virtual std::string next_topic (const std::string &url);
  virtual std::string prev_topic (const std::string &url);

  QDomDocument get_dom (const std::string &u);

  std::vector<IndexEntry>::const_iterator begin_index () const
  {
    return m_index.begin ();
  }

  std::vector<IndexEntry>::const_iterator end_index () const
  {
    return m_index.end ();
  }

  void push_index (const IndexEntry &index)
  {
    m_index.push_back (index);
  }

  std::map<std::string, std::string>::const_iterator begin_parents () const
  {
    return m_parent_of.begin ();
  }

  std::map<std::string, std::string>::const_iterator end_parents () const
  {
    return m_parent_of.end ();
  }

  void push_parent (const std::pair<std::string, std::string> &p) 
  {
    m_parent_of.insert (p);
  }

  std::vector<std::pair <std::string, std::string> >::const_iterator begin_titles () const
  {
    return m_titles.begin ();
  }

  std::vector<std::pair <std::string, std::string> >::const_iterator end_titles () const
  {
    return m_titles.end ();
  }

  void push_title (const std::pair<std::string, std::string> &p) 
  {
    m_titles.push_back (p);
  }

  std::string klayout_version () const;

  void set_klayout_version (const std::string &v) 
  {
    m_klayout_version = v;
  }

  const std::string &parent_of (const std::string &path);

  std::string title_for (const std::string &path);

  std::vector<std::string> urls ();

  /**
   *  @brief Creates a help index file at the given path
   */
  static void create_index_file (const std::string &path);

  /**
   *  @brief Scans the help providers and produce the index
   */
  void scan ();

  /**
   *  @brief Sets a global options for tailoring the help output
   */
  void set_option (const std::string &key, const tl::Variant &value);

  /**
   *  @brief Sets a global options for tailoring the help output
   *  A null variant is returned if the option is not present.
   */
  const tl::Variant &get_option (const std::string &key) const;

private:
  std::vector<IndexEntry> m_index;
  std::map<std::string, std::string> m_parent_of;
  std::vector<std::pair<std::string, std::string> > m_titles;
  std::map<std::string, std::string> m_title_map;
  std::string m_klayout_version;
  int m_kindex;
  std::map<std::string, tl::Variant> s_global_options;

  QDomDocument produce_search (const std::string &index);
  QDomDocument produce_main_index ();
  void produce_index_file (const std::string &path);
  void initialize_index ();
  std::string process (const QDomDocument &doc, const std::string &path, BrowserOutline &ol);
  void process_child_nodes (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &old);
  void process (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &ol);
  std::string read (const std::string &u);
  void writeElement (const QDomElement &element, const std::string &path, QXmlStreamWriter &writer, BrowserOutline &ol);
  void scan (const std::string &path, tl::AbsoluteProgress &progress);
  void scan_child_nodes (const QDomElement &element, const std::string &path, std::vector<std::string> &subtopics, std::string &title, std::string &section);
  void scan (const QDomElement &element, const std::string &path, std::vector<std::string> &subtopics, std::string &title, std::string &section);
};

}

namespace tl
{
  //  make registration available to external DLL's
  template class LAY_PUBLIC tl::RegisteredClass<lay::HelpProvider>;
}

#endif



