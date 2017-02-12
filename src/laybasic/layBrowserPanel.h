
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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
#include <set>

namespace Ui
{
  class BrowserPanel;
}

namespace lay
{

class BrowserPanel;

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
   *  a new location. This allows to implement any functionality behind such links.
   */
  virtual std::string get (const std::string &url);

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
 *  @brief A specialisation of QTextBrowser that allows to load a specific resource through BrowserPanel
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
 *  @brief A specialisation of QWidget around a TextBrowser that allows to load a specific resource
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
  Ui::BrowserPanel *mp_ui;
  bool m_schedule_back;
  tl::DeferredMethod<BrowserPanel> m_back_dm;
  std::string m_search_url, m_search_query_item;
  QString m_current_title;

  void init ();
};

}

#endif

