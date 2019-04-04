
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#ifndef HDR_layBrowserPanel
#define HDR_layBrowserPanel

#include "laybasicCommon.h"
#include "tlDeferredExecution.h"
#include "tlObject.h"
#include "gsiObject.h"

#include <QTextBrowser>

#include <string>
#include <list>
#include <set>

class QTreeWidgetItem;

namespace Ui
{
  class BrowserPanel;
}

namespace lay
{

class BrowserPanel;

/**
 *  @brief Specifies the outline of the document
 *
 *  The outline is a hierarchical tree of items. Each node has a title, a URL to navigate to and
 *  optional child items.
 */
class LAYBASIC_PUBLIC BrowserOutline
{
public:
  typedef std::list<BrowserOutline>::const_iterator const_child_iterator;
  typedef std::list<BrowserOutline>::iterator child_iterator;

  /**
   *  @brief Default constructor: creates an empty browser outline
   */
  BrowserOutline ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Default constructor: creates a single entry with title and URL
   */
  BrowserOutline (const std::string &title, const std::string &url)
    : m_title (title), m_url (url)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the title
   */
  const std::string &title () const
  {
    return m_title;
  }

  /**
   *  @brief Sets the title
   */
  void set_title (const std::string &t)
  {
    m_title = t;
  }

  /**
   *  @brief Gets the URL
   */
  const std::string &url () const
  {
    return m_url;
  }

  /**
   *  @brief Sets the URL
   */
  void set_url (const std::string &u)
  {
    m_url = u;
  }

  /**
   *  @brief Returns the begin iterator for the children
   */
  const_child_iterator begin () const
  {
    return m_children.begin ();
  }

  /**
   *  @brief Returns the end iterator for the children
   */
  const_child_iterator end () const
  {
    return m_children.end ();
  }

  /**
   *  @brief Returns the non-const begin iterator for the children
   */
  child_iterator begin ()
  {
    return m_children.begin ();
  }

  /**
   *  @brief Returns the non-const end iterator for the children
   */
  child_iterator end ()
  {
    return m_children.end ();
  }

  /**
   *  @brief Adds a child entry at the end of the list
   */
  void add_child (const BrowserOutline &ol)
  {
    m_children.push_back (ol);
  }

  /**
   *  @brief Clears the child list of the node
   */
  void clear_children ()
  {
    m_children.clear ();
  }

private:
  std::string m_title;
  std::string m_url;
  std::list<BrowserOutline> m_children;
};

/**
 *  @brief The source for BrowserDialog's "int" URL's
 */
class LAYBASIC_PUBLIC BrowserSource
  : public gsi::ObjectBase, public tl::Object
{
public:
  /**
   *  @brief Default constructor
   */
  BrowserSource ();

  /**
   *  @brief construct a BrowserSource object with a default HTML string
   *
   *  The default HTML string is sent when no specific implementation is provided.
   */
  BrowserSource (const std::string &html);

  /**
   *  @brief Destructor
   */
  virtual ~BrowserSource ();

  /**
   *  @brief Get the HTML code for a given "int" URL.
   *
   *  If this method returns an empty string, the browser will not be set to 
   *  a new location. This allows implementing any functionality behind such links.
   */
  virtual std::string get (const std::string &url);

  /**
   *  @brief Gets the outline object if the source provides one
   *
   *  The outline is a dictionary of item and subitems, each with a title and a
   *  URL to navigate to if selected.
   *
   *  If an empty outline is returned, no outline is shown.
   */
  virtual BrowserOutline get_outline (const std::string &url);

  /**
   *  @brief Get the image for a given "int" URL in an image
   */
  virtual QImage get_image (const std::string &url);

  /**
   *  @brief Get the CSS resource for a given "int" URL 
   */
  virtual std::string get_css (const std::string &url);

  /**
   *  @brief Returns the next URL to a given URL
   *
   *  Returning an empty string indicates that there is no "next" URL.
   */
  virtual std::string next_topic (const std::string & /*url*/)
  {
    return std::string ();
  }

  /**
   *  @brief Returns the previous URL to a given URL
   *
   *  Returning an empty string indicates that there is no "previous" URL.
   */
  virtual std::string prev_topic (const std::string & /*url*/)
  {
    return std::string ();
  }

  /**
   *  @brief Attach to a BrowserPanel
   */
  void attach (lay::BrowserPanel *d);

  /**
   *  @brief Detach to a BrowserPanel
   */
  void detach (lay::BrowserPanel *d);

private:
  std::set<BrowserPanel *> mp_owners;
  std::string m_default_html;
};

/**
 *  @brief A specialisation of QTextBrowser that allows loading a specific resource through BrowserPanel
 */
class LAYBASIC_PUBLIC BrowserTextWidget
  : public QTextBrowser
{
public:
  BrowserTextWidget (QWidget *w)
    : QTextBrowser (w), mp_panel (0)
  {
    //  .. nothing yet ..
  }
  
  void set_panel (BrowserPanel *panel)
  {
    mp_panel = panel;
  }

  virtual QVariant loadResource (int type, const QUrl &url);

private:
  BrowserPanel *mp_panel;
};

/**
 *  @brief A specialisation of QWidget around a TextBrowser that allows loading a specific resource
 */
class LAYBASIC_PUBLIC BrowserPanel
  : public QWidget
{
  friend class BrowserTextWidget;

Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  BrowserPanel (QWidget *p); 

  /**
   *  @brief Dtor
   */
  ~BrowserPanel ();

  /**
   *  @brief Connect to a source object
   *  If "
   */
  void set_source (BrowserSource *source);

  /**
   *  @brief Load a certain URL
   */
  void load (const std::string &s);

  /**
   *  @brief Gets the URL currently shown
   */
  std::string url () const;

  /**
   *  @brief Gets the title of the page currently 
   */
  std::string title () const;

  /**
   *  @brief Set the home URL
   *
   *  Set the browser to the given URL. This will also be the URL that is navigated to
   *  when the "home" button is clicked.
   */
  void set_home (const std::string &url);

  /**
   *  @brief Reload the current page
   */
  void reload ();

  /**
   *  @brief Set the label text
   */
  void set_label (const std::string &text);

  /**
   *  @brief Sets the outline
   */
  void set_outline (const BrowserOutline &ol);

  /**
   *  @brief Enables the search bx and sets the Url and query item name for the search
   */
  void set_search_url (const std::string &url, const std::string &query_item);

  /**
   *  @brief Navigates to the search entry with that subject
   */
  void search (const std::string &s);

signals:
  /**
   *  @brief This signal is emitted when the document's title has changed
   */
  void title_changed (const QString &t);

  /**
   *  @brief This signal is emitted when the URL has changed
   */
  void url_changed (const QString &t);

public slots:
  /**
   *  @brief Navigate backward
   */
  void back ();

  /**
   *  @brief Navigate forward
   */
  void forward ();
  
  /**
   *  @brief Navigate to previous topic
   */
  void prev ();

  /**
   *  @brief Navigate to next topic
   */
  void next ();
  
  /**
   *  @brief Navigate to home and force reload if required
   */
  void home ();

protected slots:
  void search_edited ();
  void text_changed ();
  void outline_item_clicked (QTreeWidgetItem *item);

protected:
  virtual QVariant loadResource (int type, const QUrl &url);
  virtual QSize sizeHint () const;

private:
  bool m_enable_load, m_enable_reject;
  tl::weak_ptr<BrowserSource> mp_source;
  std::string m_home;
  std::string m_cached_url;
  std::string m_cached_text;
  std::string m_cached_next_url;
  std::string m_cached_prev_url;
  BrowserOutline m_cached_outline;
  Ui::BrowserPanel *mp_ui;
  bool m_schedule_back;
  tl::DeferredMethod<BrowserPanel> m_back_dm;
  std::string m_search_url, m_search_query_item;
  QString m_current_title;

  void init ();
};

}

#endif

