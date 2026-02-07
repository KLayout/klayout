
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

#if defined(HAVE_QT)

#ifndef HDR_layBrowserDialog
#define HDR_layBrowserDialog

#include "tlTypeTraits.h"

#include "ui_BrowserDialog.h"

#include "gsi.h"

namespace lay
{

/**
 *  @brief The HTML browser dialog
 *
 *  The HTML browser displays HTML code in a browser panel. It receives the code
 *  by retrieving it from a given URL. 
 *  URL's with the special scheme "int" are retrieved from a BrowserSource object.
 *  This will act as a kind of "server" for these URL's.
 */
class LAYUI_PUBLIC BrowserDialog
  : public QDialog, 
    private Ui::BrowserDialog
{
  Q_OBJECT 

public:
  /**
   *  @brief Default constructor
   */
  BrowserDialog ();

  /**
   *  @brief Constructor with a parent
   */
  BrowserDialog (QWidget *parent);

  /**
   *  @brief Constructor with a static HTML page
   */
  BrowserDialog (const std::string &html);

  /**
   *  @brief Constructor with a static HTML page and a parent
   */
  BrowserDialog (QWidget *parent, const std::string &html);

  /**
   *  @brief Destructor
   */
  virtual ~BrowserDialog ();

  /**
   *  @brief Connect to a source object
   */
  void set_source (BrowserSource *source);

  /**
   *  @brief Load a certain URL
   */
  void load (const std::string &s);

  /**
   *  @brief Set the home URL
   *
   *  Set the browser to the given URL. This will also be the URL that is navigated to
   *  when the "home" button is clicked.
   */
  void set_home (const std::string &url);

  /**
   *  @brief Sets the label text
   */
  void set_label (const std::string &label);

  /**
   *  @brief Sets the search URL
   *  Enables the search bx and sets the Url and query item name for the search
   */
  void set_search_url (const std::string &url, const std::string &query_item);

  /**
   *  @brief Navigates to the search entry with that subject
   */
  void search (const std::string &s);

  /**
   *  @brief Reload the current page
   */
  void reload ();

  /**
   *  @brief Callback when the dialog is closed
   *
   *  Reimplement this callback to implement on-close functionality such as cleanup.
   */
  virtual void closed ()
  {
    //  by default, do nothing.
  }

private:
  virtual void accept ();

  BrowserSource m_default_source;
};

}

#endif

#endif  //  defined(HAVE_QT)
