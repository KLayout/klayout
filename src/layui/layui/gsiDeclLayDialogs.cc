
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

#if defined(HAVE_QT) 

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "layBrowserDialog.h"
#include "layBrowserPanel.h"
#include "layFileDialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QMainWindow>
#include <QApplication>

#include <limits>

#if defined(HAVE_QTBINDINGS)

#  include "gsiQtGuiExternals.h"
#  include "gsiQtWidgetsExternals.h"

FORCE_LINK_GSI_QTGUI
FORCE_LINK_GSI_QTWIDGETS // for Qt5

#else
# define QT_EXTERNAL_BASE(x)
#endif

namespace gsi
{

//  the stub required to provide all interface logic for the virtual methods
//  (this enables reimplementation of the virtual function on the client side)
class BrowserDialog_Stub 
  : public lay::BrowserDialog,
    public gsi::ObjectBase
{
public:
  BrowserDialog_Stub () : lay::BrowserDialog () { }
  BrowserDialog_Stub (QWidget *parent) : lay::BrowserDialog (parent) { }
  BrowserDialog_Stub (const std::string &html) : lay::BrowserDialog (html) { }
  BrowserDialog_Stub (QWidget *parent, const std::string &html) : lay::BrowserDialog (parent, html) { }

  virtual void closed ()
  {
    if (closed_cb.can_issue ()) {
      closed_cb.issue<lay::BrowserDialog> (&lay::BrowserDialog::closed);
    } else {
      lay::BrowserDialog::closed ();
    }
  }

  gsi::Callback closed_cb;
};

//  the stub required to provide all interface logic for the virtual methods
//  (this enables reimplementation of the virtual function on the client side)
class BrowserSource_Stub : public lay::BrowserSource
{
public:
  BrowserSource_Stub () : lay::BrowserSource () { }
  BrowserSource_Stub (const std::string &html) : lay::BrowserSource (html) { }

  virtual std::string get (const std::string &url)
  {
    if (get_cb.can_issue ()) {
      return get_cb.issue<lay::BrowserSource, std::string, const std::string &> (&lay::BrowserSource::get, url);
    } else {
      return lay::BrowserSource::get (url);
    }
  }

  gsi::Callback get_cb;
};

// ---------------------------------------------------------------------------------
//  Value classes with "not set" capabilities

struct DoubleValue
{
  DoubleValue ()          : v (0), h (false) { }
  DoubleValue (double _v) : v (_v), h (true) { }

  double v;
  bool h;
  double value ()   const { return v; }
  bool has_value () const { return h; }
};

Class<DoubleValue> decl_DoubleValue ("lay", "DoubleValue",
  gsi::method ("has_value?", &DoubleValue::has_value,
    "@brief True, if a value is present"
  ) +
  gsi::method ("to_f", &DoubleValue::value,
    "@brief Get the actual value (a synonym for \\value)"
  ) +
  gsi::method ("value", &DoubleValue::value,
    "@brief Get the actual value"
  ),
  "@brief Encapsulate a floating point value\n"
  "@hide\n"
  "This class is provided as a return value of \\InputDialog::get_double.\n"
  "By using an object rather than a pure value, an object with \\has_value? = false can be returned indicating that\n"
  "the \"Cancel\" button was pressed. Starting with version 0.22, the InputDialog class offers new method which do no\n"
  "longer requires to use this class."
);

struct IntValue
{
  IntValue ()       : v (0), h (false) { }
  IntValue (int _v) : v (_v), h (true) { }

  int v;
  bool h;
  int value ()      const { return v; }
  bool has_value () const { return h; }
};

Class<IntValue> decl_IntValue ("lay", "IntValue",
  gsi::method ("has_value?", &IntValue::has_value,
    "@brief True, if a value is present"
  ) +
  gsi::method ("to_i", &IntValue::value,
    "@brief Get the actual value (a synonym for \\value)"
  ) +
  gsi::method ("value", &IntValue::value,
    "@brief Get the actual value"
  ),
  "@brief Encapsulate an integer value\n"
  "@hide\n"
  "This class is provided as a return value of \\InputDialog::get_int.\n"
  "By using an object rather than a pure value, an object with \\has_value? = false can be returned indicating that\n"
  "the \"Cancel\" button was pressed. Starting with version 0.22, the InputDialog class offers new method which do no\n"
  "longer requires to use this class."
);

struct StringValue
{
  StringValue ()                      : v (), h (false) { }
  StringValue (const std::string &_v) : v (_v), h (true) { }

  std::string v;
  bool h;
  const std::string &value () const { return v; }
  bool has_value ()           const { return h; }
};

Class<StringValue> decl_StringValue ("lay", "StringValue",
  gsi::method ("has_value?", &StringValue::has_value,
    "@brief True, if a value is present"
  ) +
  gsi::method ("to_s", &StringValue::value,
    "@brief Get the actual value (a synonym for \\value)"
  ) +
  gsi::method ("value", &StringValue::value,
    "@brief Get the actual value"
  ),
  "@brief Encapsulate a string value\n"
  "@hide\n"
  "This class is provided as a return value of \\InputDialog::get_string, \\InputDialog::get_item and \\FileDialog.\n"
  "By using an object rather than a pure value, an object with \\has_value? = false can be returned indicating that\n"
  "the \"Cancel\" button was pressed. Starting with version 0.22, the InputDialog class offers new method which do no\n"
  "longer requires to use this class."
);

struct StringListValue
{
  StringListValue ()                                   : v (), h (false) { }
  StringListValue (const std::vector<std::string> &_v) : v (_v), h (true) { }

  std::vector<std::string> v;
  bool h;
  const std::vector<std::string> &value () const { return v; }
  bool has_value () const                        { return h; }
};

Class<StringListValue> decl_StringListValue ("lay", "StringListValue",
  gsi::method ("has_value?", &StringListValue::has_value,
    "@brief True, if a value is present"
  ) +
  gsi::method ("value", &StringListValue::value,
    "@brief Get the actual value (a list of strings)"
  ),
  "@brief Encapsulate a string list\n"
  "@hide\n"
  "This class is provided as a return value of \\FileDialog.\n"
  "By using an object rather than a pure string list, an object with \\has_value? = false can be returned indicating that\n"
  "the \"Cancel\" button was pressed. Starting with version 0.22, the InputDialog class offers new method which do no\n"
  "longer requires to use this class."
);

// ---------------------------------------------------------------------------------
//  HTML browser

//  specialize the "set_source" method to the stub class
void set_source (BrowserDialog_Stub *s, BrowserSource_Stub *src)
{
  s->set_source (src);
}

void set_size (BrowserDialog_Stub *s, int width, int height)
{
  s->resize (QSize (width, height));
}

void set_caption (BrowserDialog_Stub *s, const std::string &caption)
{
  s->setWindowTitle (tl::to_qstring (caption));
}

BrowserDialog_Stub *new_browser_dialog_with_source (BrowserSource_Stub *source)
{
  BrowserDialog_Stub *bd = new BrowserDialog_Stub();
  bd->set_source (source);
  return bd;
}

BrowserDialog_Stub *new_browser_dialog_static (const std::string &html)
{
  return new BrowserDialog_Stub (html);
}

#if defined(HAVE_QTBINDINGS)
BrowserDialog_Stub *new_browser_dialog_with_source_and_parent (QWidget *parent, BrowserSource_Stub *source)
{
  BrowserDialog_Stub *bd = new BrowserDialog_Stub(parent);
  bd->set_source (source);
  return bd;
}

BrowserDialog_Stub *new_browser_dialog_static_and_parent (QWidget *parent, const std::string &html)
{
  return new BrowserDialog_Stub (parent, html);
}
#endif

Class<BrowserDialog_Stub> decl_BrowserDialog (QT_EXTERNAL_BASE (QDialog) "lay", "BrowserDialog",
  gsi::constructor ("new", &new_browser_dialog_with_source, gsi::arg ("source"),
    "@brief Creates a HTML browser window with a \\BrowserSource as the source of HTML code\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::constructor ("new", &new_browser_dialog_static, gsi::arg ("html"),
    "@brief Creates a HTML browser window with a static HTML content\n"
    "This method has been introduced in version 0.23.\n"
  ) +
#if defined(HAVE_QTBINDINGS)
  gsi::constructor ("new", &new_browser_dialog_with_source_and_parent, gsi::arg ("parent"), gsi::arg ("source"),
    "@brief Creates a HTML browser window with a \\BrowserSource as the source of HTML code\n"
    "This method variant with a parent argument has been introduced in version 0.24.2.\n"
  ) +
  gsi::constructor ("new", &new_browser_dialog_static_and_parent, gsi::arg ("parent"), gsi::arg ("html"),
    "@brief Creates a HTML browser window with a static HTML content\n"
    "This method variant with a parent argument has been introduced in version 0.24.2.\n"
  ) +
#else
  gsi::method ("hide", &BrowserDialog_Stub::hide, 
    "@brief Hides the HTML browser window"
  ) +
  gsi::method ("show", &BrowserDialog_Stub::show, 
    "@brief Shows the HTML browser window in a non-modal way"
  ) +
#endif
  gsi::method ("execute|#exec", &BrowserDialog_Stub::exec, 
    "@brief Executes the HTML browser dialog as a modal window\n"
  ) +
  gsi::method ("load", &BrowserDialog_Stub::load, gsi::arg ("url"),
    "@brief Loads the given URL into the browser dialog\n"
    "Typically the URL has the \"int:\" scheme so the HTML code is taken from the "
    "\\BrowserSource object.\n"
  ) +
  gsi::method ("label=", &BrowserDialog_Stub::set_label, gsi::arg ("label"),
    "@brief Sets the label text\n"
    "\n"
    "The label is shown left of the navigation buttons.\n"
    "By default, no label is specified.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("set_search_url", &BrowserDialog_Stub::set_search_url, gsi::arg ("url"), gsi::arg ("query_item"),
    "@brief Enables the search field and specifies the search URL generated for a search\n"
    "\n"
    "If a search URL is set, the search box right to the navigation bar will be enabled. "
    "When a text is entered into the search box, the browser will navigate to an URL composed "
    "of the search URL, the search item and the search text, i.e. \"myurl?item=search_text\".\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("search", &BrowserDialog_Stub::search, gsi::arg ("search_item"),
    "@brief Issues a search request using the given search item and the search URL specified with \\set_search_url\n"
    "\n"
    "See \\set_search_url for a description of the search mechanism.\n"
  ) +
  gsi::method_ext ("source=|#set_source", &set_source, gsi::arg ("source"),
    "@brief Connects to a source object\n"
    "\n"
    "Setting the source should be the first thing done after the BrowserDialog object is created. It will not "
    "have any effect after the browser has loaded the first page. In particular, \\home= should be called after the source "
    "was set."
  ) +
  gsi::method_ext ("resize|#set_size", &set_size, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Sets the size of the dialog window\n"
  ) +
  gsi::method_ext ("caption=|#set_caption", &set_caption, gsi::arg ("caption"),
    "@brief Sets the caption of the window\n"
  ) +
  gsi::method ("reload", &BrowserDialog_Stub::reload, 
    "@brief Reloads the current page"
  ) +
  gsi::method ("home=|#set_home", &BrowserDialog_Stub::set_home, gsi::arg ("home_url"),
    "@brief Sets the browser's initial and current URL which is selected if the \"home\" location is chosen\n"
    "The home URL is the one shown initially and the one which is selected when the \"home\" button is pressed. "
    "The default location is \"int:/index.html\".\n"
  ) +
  gsi::callback ("#closed", &BrowserDialog_Stub::closed, &BrowserDialog_Stub::closed_cb, 
    "@brief Callback when the dialog is closed"
    "\n"
    "This callback can be reimplemented to implement cleanup functionality when the "
    "dialog is closed."
  ),
  "@brief A HTML display and browser dialog\n"
  "\n"
  "The browser dialog displays HTML code in a browser panel. The HTML code is delivered through a separate "
  "object of class \\BrowserSource which acts as a \"server\" for a specific kind of URL scheme. Whenever the "
  "browser sees a URL starting with \"int:\" it will ask the connected BrowserSource object for the HTML code "
  "of that page using its 'get' method. The task of the BrowserSource object is to format the data requested "
  "in HTML and deliver it.\n"
  "\n"
  "One use case for that class is the implementation of rich data browsers for structured information. In a "
  "simple scenario, the browser dialog can be instantiated with a static HTML page. In that case, only the content "
  "of that page is shown.\n"
  "\n"
  "Here's a simple example:\n"
  "\n"
  "@code\n"
  "html = \"<html><body>Hello, world!</body></html>\"\n"
  "RBA::BrowserDialog::new(html).exec\n"
  "@/code\n"
  "\n"
  "And that is an example for the use case with a \\BrowserSource as the \"server\":\n"
  "\n"
  "@code\n"
  "class MySource < RBA::BrowserSource\n"
  "  def get(url)\n"
  "    if (url =~ /b.html$/)\n"
  "      return \"<html><body>The second page</body></html>\"\n"
  "    else\n"
  "      return \"<html><body>The first page with a <a href='int:b.html'>link</a></body></html>\"\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "source = MySource::new\n"
  "RBA::BrowserDialog::new(source).exec\n"
  "@/code\n"
);

static BrowserSource_Stub *new_html (const std::string &html)
{
  return new BrowserSource_Stub (html);
}

Class<lay::BrowserSource> decl_BrowserSource ("lay", "BrowserSource_Native",
#if defined(HAVE_QTBINDINGS)
  gsi::method ("get_image", &lay::BrowserSource::get_image, gsi::arg ("url")) +
#endif
  gsi::method ("next_topic", &lay::BrowserSource::next_topic, gsi::arg ("url")) +
  gsi::method ("prev_topic", &lay::BrowserSource::prev_topic, gsi::arg ("url")) +
  gsi::method ("get", &lay::BrowserSource::get, gsi::arg ("url")),
  "@hide\n@alias BrowserSource"
);

LAYUI_PUBLIC
Class<lay::BrowserSource> &laybasicdecl_BrowserSource ()
{
  return decl_BrowserSource;
}

Class<BrowserSource_Stub> decl_BrowserSourceStub ("lay", "BrowserSource",
  gsi::constructor ("new|#new_html", &new_html, gsi::arg ("html"),
    "@brief Constructs a BrowserSource object with a default HTML string\n"
    "\n"
    "The default HTML string is sent when no specific implementation is provided.\n"
  ) +
#if defined(HAVE_QTBINDINGS)
  gsi::method ("get_image", &lay::BrowserSource::get_image, gsi::arg ("url"),
    "@brief Gets the image object for a specific URL\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
#endif
  gsi::method ("next_topic", &lay::BrowserSource::next_topic, gsi::arg ("url"),
    "@brief Gets the next topic URL from a given URL\n"
    "An empty string will be returned if no next topic is available.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method ("prev_topic", &lay::BrowserSource::prev_topic, gsi::arg ("url"),
    "@brief Gets the previous topic URL from a given URL\n"
    "An empty string will be returned if no previous topic is available.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::callback ("get", &BrowserSource_Stub::get, &BrowserSource_Stub::get_cb, gsi::arg ("url"),
    "@brief Gets the HTML code for a given \"int\" URL.\n"
    "\n"
    "If this method returns an empty string, the browser will not be set to \n"
    "a new location. This allows implementing any functionality behind such links.\n"
    "If the method returns a string, the content of this string is displayed in the HTML\n"
    "browser page."
  ),
  "@brief The BrowserDialog's source for \"int\" URL's\n"
  "\n"
  "The source object basically acts as a \"server\" for special URL's using \"int\" as the scheme.\n"
  "Classes that want to implement such functionality must derive from BrowserSource and reimplement\n"
  "the \\get method. This method is supposed to deliver a HTML page for the given URL.\n\n"
  "Alternatively to implementing this functionality, a source object may be instantiated using the\n"
  "constructor with a HTML code string. This will create a source object that simply displays the given string\n"
  "as the initial and only page."
);

#if defined(HAVE_QTBINDINGS)
lay::BrowserPanel *new_browser_panel_with_source (QWidget *parent, lay::BrowserSource *source)
{
  lay::BrowserPanel *b = new lay::BrowserPanel (parent);
  b->set_source (source);
  return b;
}

lay::BrowserPanel *new_browser_panel (QWidget *parent)
{
  return new lay::BrowserPanel (parent);
}

Class<lay::BrowserPanel> decl_BrowserPanel (QT_EXTERNAL_BASE (QWidget) "lay", "BrowserPanel",
  gsi::constructor ("new", &new_browser_panel_with_source, gsi::arg ("parent"), gsi::arg ("source"),
    "@brief Creates a HTML browser widget with a \\BrowserSource as the source of HTML code\n"
  ) +
  gsi::constructor ("new", &new_browser_panel, gsi::arg ("parent"),
    "@brief Creates a HTML browser widget\n"
  ) +
  gsi::method ("load", &lay::BrowserPanel::load, gsi::arg ("url"),
    "@brief Loads the given URL into the browser widget\n"
    "Typically the URL has the \"int:\" scheme so the HTML code is taken from the "
    "\\BrowserSource object.\n"
  ) +
  gsi::method ("url", &lay::BrowserPanel::url, 
    "@brief Gets the URL currently shown\n"
  ) +
  gsi::method ("set_search_url", &lay::BrowserPanel::set_search_url, gsi::arg ("url"), gsi::arg ("query_item"),
    "@brief Enables the search field and specifies the search URL generated for a search\n"
    "\n"
    "If a search URL is set, the search box right to the navigation bar will be enabled. "
    "When a text is entered into the search box, the browser will navigate to an URL composed "
    "of the search URL, the search item and the search text, i.e. \"myurl?item=search_text\".\n"
  ) +
  gsi::method ("search", &lay::BrowserPanel::search, gsi::arg ("search_item"),
    "@brief Issues a search request using the given search item and the search URL specified with \\set_search_url\n"
    "\n"
    "See \\search_url= for a description of the search mechanism.\n"
  ) +
  gsi::method ("source=", &lay::BrowserPanel::set_source, gsi::arg ("source"),
    "@brief Connects to a source object\n"
    "\n"
    "Setting the source should be the first thing done after the BrowserDialog object is created. It will not "
    "have any effect after the browser has loaded the first page. In particular, \\home= should be called after the source "
    "was set."
  ) +
  gsi::method ("label=", &lay::BrowserPanel::set_label, gsi::arg ("label"),
    "@brief Sets the label text\n"
    "\n"
    "The label is shown left of the navigation buttons.\n"
    "By default, no label is specified.\n"
  ) +
  gsi::method ("reload", &lay::BrowserPanel::reload, 
    "@brief Reloads the current page"
  ) +
  gsi::method ("home=", &lay::BrowserPanel::set_home, gsi::arg ("home_url"),
    "@brief Sets the browser widget's initial and current URL which is selected if the \"home\" location is chosen\n"
    "The home URL is the one shown initially and the one which is selected when the \"home\" button is pressed. "
    "The default location is \"int:/index.html\".\n"
  ),
  "@brief A HTML display and browser widget\n"
  "\n"
  "This widget provides the functionality of \\BrowserDialog within a widget. It can be embedded into "
  "other dialogs. For details about the use model of this class see \\BrowserDialog.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);
#endif

// ---------------------------------------------------------------------------------
//  Input dialogs

static StringValue get_string (const std::string &title, const std::string &label, const std::string &value)
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), QLineEdit::Normal, tl::to_qstring (value), &ok);
  if (!ok) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (s));
  }
}

static StringValue get_string_password (const std::string &title, const std::string &label, const std::string &value)
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), QLineEdit::Password, tl::to_qstring (value), &ok);
  if (!ok) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (s));
  }
}

static DoubleValue get_double (const std::string &title, const std::string &label, double value, int digits)
{
  bool ok = false;
  double s = QInputDialog::getDouble (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, std::numeric_limits<double>::min (), std::numeric_limits<double>::max (), digits, &ok);
  if (!ok) {
    return DoubleValue ();
  } else {
    return DoubleValue (s);
  }
}

static DoubleValue get_double_ex (const std::string &title, const std::string &label, double value, double dmin, double dmax, int decimals)
{
  bool ok = false;
  double s = QInputDialog::getDouble (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, decimals, &ok);
  if (!ok) {
    return DoubleValue ();
  } else {
    return DoubleValue (s);
  }
}

static IntValue get_int (const std::string &title, const std::string &label, int value)
{
  bool ok = false;
#if QT_VERSION >= 0x050000
  int s = QInputDialog::getInt (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, std::numeric_limits<int>::min (), std::numeric_limits<int>::max (), 1, &ok);
#else
  int s = QInputDialog::getInteger (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, std::numeric_limits<int>::min (), std::numeric_limits<int>::max (), 1, &ok);
#endif
  if (!ok) {
    return IntValue ();
  } else {
    return IntValue (s);
  }
}

static IntValue get_int_ex (const std::string &title, const std::string &label, int value, int dmin, int dmax, int step)
{
  bool ok = false;
#if QT_VERSION >= 0x050000
  int s = QInputDialog::getInt (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, step, &ok);
#else
  int s = QInputDialog::getInteger (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, step, &ok);
#endif
  if (!ok) {
    return IntValue ();
  } else {
    return IntValue (s);
  }
}

static StringValue get_item (const std::string &title, const std::string &label, const std::vector<std::string> &items, int selected)
{
  bool ok = false;
  QStringList ilist;
  for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
    ilist << tl::to_qstring (*i);
  }
  QString s = QInputDialog::getItem (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), ilist, selected, false, &ok);
  if (!ok) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (s));
  }
}

static tl::Variant ask_string (const std::string &title, const std::string &label, const std::string &value)
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), QLineEdit::Normal, tl::to_qstring (value), &ok);
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (tl::to_string (s));
  }
}

static tl::Variant ask_string_password (const std::string &title, const std::string &label, const std::string &value)
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), QLineEdit::Password, tl::to_qstring (value), &ok);
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (tl::to_string (s));
  }
}

static tl::Variant ask_double (const std::string &title, const std::string &label, double value, int digits)
{
  bool ok = false;
  double s = QInputDialog::getDouble (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, -std::numeric_limits<double>::max (), std::numeric_limits<double>::max (), digits, &ok);
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (s);
  }
}

static tl::Variant ask_double_ex (const std::string &title, const std::string &label, double value, double dmin, double dmax, int decimals)
{
  bool ok = false;
  double s = QInputDialog::getDouble (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, decimals, &ok);
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (s);
  }
}

static tl::Variant ask_int (const std::string &title, const std::string &label, int value)
{
  bool ok = false;
#if QT_VERSION >= 0x050000
  int s = QInputDialog::getInt (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, std::numeric_limits<int>::min (), std::numeric_limits<int>::max (), 1, &ok);
#else
  int s = QInputDialog::getInteger (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, std::numeric_limits<int>::min (), std::numeric_limits<int>::max (), 1, &ok);
#endif
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (s);
  }
}

static tl::Variant ask_int_ex (const std::string &title, const std::string &label, int value, int dmin, int dmax, int step)
{
  bool ok = false;
#if QT_VERSION >= 0x050000
  int s = QInputDialog::getInt (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, step, &ok);
#else
  int s = QInputDialog::getInteger (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), value, dmin, dmax, step, &ok);
#endif
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (s);
  }
}

static tl::Variant ask_item (const std::string &title, const std::string &label, const std::vector<std::string> &items, int selected)
{
  bool ok = false;
  QStringList ilist;
  for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
    ilist << tl::to_qstring (*i);
  }
  QString s = QInputDialog::getItem (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (label), ilist, selected, false, &ok);
  if (!ok) {
    return tl::Variant ();
  } else {
    return tl::Variant (s);
  }
}

struct InputDialog { };

Class<InputDialog> decl_InputDialog ("lay", "InputDialog",
  gsi::method ("#get_string", &get_string, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting a string\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return A \\StringValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_item", &get_item, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("items"), gsi::arg ("value"),
    "@brief Open an input dialog requesting an item from a list\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param items The list of items to show in the selection element\n"
    "@param selection The initial selection (index of the element selected initially)\n"
    "@return A \\StringValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_string_password", &get_string_password, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting a string without showing the actual characters entered\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return A \\StringValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_double", &get_double, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("digits"),
    "@brief Open an input dialog requesting a floating-point value\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param digits The number of digits allowed\n"
    "@return A \\DoubleValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_double_ex", &get_double_ex, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("min"), gsi::arg ("max"), gsi::arg ("digits"),
    "@brief Open an input dialog requesting a floating-point value with enhanced capabilities\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param min The minimum value allowed\n"
    "@param max The maximum value allowed\n"
    "@param digits The number of digits allowed\n"
    "@return A \\DoubleValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_int", &get_int, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting an integer value\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return A \\IntValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("#get_int_ex", &get_int_ex, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("min"), gsi::arg ("max"), gsi::arg ("step"),
    "@brief Open an input dialog requesting an integer value with enhanced capabilities\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param min The minimum value allowed\n"
    "@param max The maximum value allowed\n"
    "@param step The step size for the spin buttons\n"
    "@return A \\IntValue object with has_value? set to true, if \"Ok\" was pressed and the value given in its value attribute\n"
    "Starting from 0.22, this method is deprecated and it is recommended to use the ask_... equivalent."
  ) +
  gsi::method ("ask_string", &ask_string, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting a string\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return The string entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_item", &ask_item, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("items"), gsi::arg ("value"),
    "@brief Open an input dialog requesting an item from a list\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param items The list of items to show in the selection element\n"
    "@param selection The initial selection (index of the element selected initially)\n"
    "@return The string of the item selected if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_string_password", &ask_string_password, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting a string without showing the actual characters entered\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return The string entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_double", &ask_double, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("digits"),
    "@brief Open an input dialog requesting a floating-point value\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param digits The number of digits allowed\n"
    "@return The value entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_double_ex", &ask_double_ex, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("min"), gsi::arg ("max"), gsi::arg ("digits"),
    "@brief Open an input dialog requesting a floating-point value with enhanced capabilities\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param min The minimum value allowed\n"
    "@param max The maximum value allowed\n"
    "@param digits The number of digits allowed\n"
    "@return The value entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_int", &ask_int, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"),
    "@brief Open an input dialog requesting an integer value\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@return The value entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ) +
  gsi::method ("ask_int_ex", &ask_int_ex, gsi::arg ("title"), gsi::arg ("label"), gsi::arg ("value"), gsi::arg ("min"), gsi::arg ("max"), gsi::arg ("step"),
    "@brief Open an input dialog requesting an integer value with enhanced capabilities\n"
    "@param title The title to display for the dialog\n"
    "@param label The label text to display for the dialog\n"
    "@param value The initial value for the input field\n"
    "@param min The minimum value allowed\n"
    "@param max The maximum value allowed\n"
    "@param step The step size for the spin buttons\n"
    "@return The value entered if \"Ok\" was pressed or nil if \"Cancel\" was pressed\n"
    "This method has been introduced in 0.22 and is somewhat easier to use than the get_.. equivalent."
  ),
  "@brief Various methods to open a dialog requesting data entry"
  "\n"
  "This class provides some basic dialogs to enter a single value. Values can be strings "
  "floating-point values, integer values or an item from a list.\n"
  "This functionality is provided through the static (class) methods ask_...\n"
  "\n"
  "Here are some examples:\n"
  "\n"
  "@code\n"
  "# get a double value between -10 and 10 (initial value is 0):\n"
  "v = RBA::InputDialog::ask_double_ex(\"Dialog Title\", \"Enter the value here:\", 0, -10, 10, 1)\n"
  "# get an item from a list:\n"
  "v = RBA::InputDialog::ask_item(\"Dialog Title\", \"Select one:\", [ \"item 1\", \"item 2\", \"item 3\" ], 1)\n"
  "@/code\n"
  "\n"
  "All these examples return the \"nil\" value if \"Cancel\" is pressed.\n"
  "\n"
  "If you have enabled the Qt binding, you can use \\QInputDialog directly.\n"
);

// ---------------------------------------------------------------------------------
//  FileDialog

struct FileDialog { };

static StringValue get_existing_dir (const std::string &title, const std::string &dir)
{
  QString f = QFileDialog::getExistingDirectory (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir));
  if (f.isEmpty ()) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (f));
  }
}

static StringListValue get_open_file_names (const std::string &title, const std::string &dir, const std::string &filter)
{
  QStringList f = QFileDialog::getOpenFileNames (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter));
  if (f.isEmpty ()) {
    return StringListValue ();
  } else {
    std::vector <std::string> l;
    for (QStringList::const_iterator s = f.begin (); s != f.end (); ++s) {
      l.push_back (tl::to_string (*s));
    }
    return StringListValue (l);
  }
}

static StringValue get_open_file_name (const std::string &title, const std::string &dir, const std::string &filter)
{
  QString f = QFileDialog::getOpenFileName (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter));
  if (f.isEmpty ()) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (f));
  }
}

static StringValue get_save_file_name (const std::string &title, const std::string &dir, const std::string &filter)
{
  QString f = QFileDialog::getSaveFileName (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter));
  if (f.isEmpty ()) {
    return StringValue ();
  } else {
    return StringValue (tl::to_string (f));
  }
}

static tl::Variant ask_existing_dir (const std::string &title, const std::string &dir)
{
  QString f = QFileDialog::getExistingDirectory (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir));
  if (f.isEmpty ()) {
    return tl::Variant ();
  } else {
    return tl::Variant (tl::to_string (f));
  }
}

static tl::Variant ask_open_file_names (const std::string &title, const std::string &dir, const std::string &filter)
{
  QStringList f = QFileDialog::getOpenFileNames (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter));
  if (f.isEmpty ()) {
    return tl::Variant ();
  } else {
    std::vector <std::string> l;
    for (QStringList::const_iterator s = f.begin (); s != f.end (); ++s) {
      l.push_back (tl::to_string (*s));
    }
    return tl::Variant (l.begin (), l.end ());
  }
}

static tl::Variant ask_open_file_name (const std::string &title, const std::string &dir, const std::string &filter)
{
  QString f = QFileDialog::getOpenFileName (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter));
  if (f.isEmpty ()) {
    return tl::Variant ();
  } else {
    return tl::Variant (tl::to_string (f));
  }
}

static tl::Variant ask_save_file_name (const std::string &title, const std::string &dir, const std::string &filter)
{
  QString selected_filter;

  QString f = QFileDialog::getSaveFileName (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), tl::to_qstring (filter), &selected_filter);
  if (f.isEmpty ()) {
    return tl::Variant ();
  } else {
    return tl::Variant (lay::FileDialog::add_default_extension (tl::to_string (f), selected_filter));
  }
}

static tl::Variant ask_save_file_name2 (const std::string &title, const std::string &dir, const std::string &filter)
{
  QString selected_filter;
  QString qfilter = tl::to_qstring (filter);

  QString f = QFileDialog::getSaveFileName (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (dir), qfilter, &selected_filter);
  if (f.isEmpty ()) {
    return tl::Variant ();
  } else {
    tl::Variant v;
    v.set_list ();
    v.push (lay::FileDialog::add_default_extension (tl::to_string (f), selected_filter));
    v.push (lay::FileDialog::find_selected_filter (qfilter, selected_filter));
    return v;
  }
}

Class<FileDialog> decl_FileDialog ("lay", "FileDialog",
  gsi::method ("#get_existing_dir", &get_existing_dir, gsi::arg ("title"), gsi::arg ("dir"),
    "@brief Open a dialog to select a directory\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@return A \\StringValue object that contains the directory path selected or with has_value? = false if \"Cancel\" was pressed\n"
    "\n"
    "Starting with version 0.23 this method is deprecated. Use \\ask_existing_dir instead.\n"
  ) +
  gsi::method ("#get_open_file_names", &get_open_file_names, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one or multiple files for opening\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return A \\StringListValue object that contains the files selected or with has_value? = false if \"Cancel\" was pressed\n"
    "\n"
    "Starting with version 0.23 this method is deprecated. Use \\ask_open_file_names instead.\n"
  ) +
  gsi::method ("#get_open_file_name", &get_open_file_name, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one file for opening\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return A \\StringValue object that contains the files selected or with has_value? = false if \"Cancel\" was pressed\n"
    "\n"
    "Starting with version 0.23 this method is deprecated. Use \\ask_open_file_name instead.\n"
  ) +
  gsi::method ("#get_save_file_name", &get_save_file_name, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one file for writing\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return A \\StringValue object that contains the files selected or with has_value? = false if \"Cancel\" was pressed\n"
    "\n"
    "Starting with version 0.23 this method is deprecated. Use \\ask_save_file_name instead.\n"
  ) +
  gsi::method ("ask_existing_dir", &ask_existing_dir, gsi::arg ("title"), gsi::arg ("dir"),
    "@brief Open a dialog to select a directory\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@return The directory path selected or \"nil\" if \"Cancel\" was pressed\n"
    "\n"
    "This method has been introduced in version 0.23. It is somewhat easier to use than the get_... equivalent.\n"
  ) +
  gsi::method ("ask_open_file_names", &ask_open_file_names, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one or multiple files for opening\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return An array with the file paths selected or \"nil\" if \"Cancel\" was pressed\n"
    "\n"
    "This method has been introduced in version 0.23. It is somewhat easier to use than the get_... equivalent.\n"
  ) +
  gsi::method ("ask_open_file_name", &ask_open_file_name, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one file for opening\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return The path of the file selected or \"nil\" if \"Cancel\" was pressed\n"
    "\n"
    "This method has been introduced in version 0.23. It is somewhat easier to use than the get_... equivalent.\n"
  ) +
  gsi::method ("ask_save_file_name", &ask_save_file_name, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one file for writing\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return The path of the file chosen or \"nil\" if \"Cancel\" was pressed\n"
    "\n"
    "This method has been introduced in version 0.23. It is somewhat easier to use than the get_... equivalent.\n"
  ) +
  gsi::method ("ask_save_file_name_with_filter", &ask_save_file_name2, gsi::arg ("title"), gsi::arg ("dir"), gsi::arg ("filter"),
    "@brief Select one file for writing\n"
    "\n"
    "@param title The title of the dialog\n"
    "@param dir The directory selected initially\n"
    "@param filter The filters available, for example \"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)\"\n"
    "@return \"nil\" if \"Cancel\" was pressed, otherwise a pair: The path of the file chosen and the index selected file type (-1 if no specific type was selected)\n"
    "\n"
    "This method has been introduced in version 0.28.11.\n"
  ),
  "@brief Various methods to request a file name\n"
  "\n"
  "This class provides some basic dialogs to select a file or directory. "
  "This functionality is provided through the static (class) methods ask_...\n"
  "\n"
  "Here are some examples:\n"
  "\n"
  "@code\n"
  "# get an existing directory:\n"
  "v = RBA::FileDialog::ask_existing_dir(\"Dialog Title\", \".\")\n"
  "# get multiple files:\n"
  "v = RBA::FileDialog::ask_open_file_names(\"Title\", \".\", \"All files (*)\")\n"
  "# ask for one file name to save a file:\n"
  "v = RBA::FileDialog::ask_save_file_name(\"Title\", \".\", \"All files (*)\")\n"
  "@/code\n"
  "\n"
  "All these examples return the \"nil\" value if \"Cancel\" is pressed.\n"
  "\n"
  "If you have enabled the Qt binding, you can use \\QFileDialog directly.\n"
);


// ---------------------------------------------------------------------------------
//  MessageBox

static int b_ok ()     { return 1 << 0; }
static int b_yes ()    { return 1 << 1; }
static int b_no ()     { return 1 << 2; }
static int b_abort ()  { return 1 << 3; }
static int b_retry ()  { return 1 << 4; }
static int b_ignore () { return 1 << 5; }
static int b_cancel () { return 1 << 6; }

static int qbuttons [] = {
    int (QMessageBox::Ok),
    int (QMessageBox::Yes),
    int (QMessageBox::No),
    int (QMessageBox::Abort),
    int (QMessageBox::Retry),
    int (QMessageBox::Ignore),
    int (QMessageBox::Cancel)
};

#if QT_VERSION >= 0x040200

typedef QMessageBox::StandardButton (*msg_func) (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons, QMessageBox::StandardButton);

static int show_msg_box (msg_func mf, const std::string &title, const std::string &text, int buttons)
{
  QMessageBox::StandardButton b = QMessageBox::StandardButton (0);
  for (unsigned int i = 0; i < sizeof (qbuttons) / sizeof (qbuttons [0]); ++i) {
    if ((buttons & (1 << i)) != 0) {
      buttons &= ~(1 << i);
      b = QMessageBox::StandardButton (int (b) | qbuttons [i]);
    }
  }

  int res = (*mf) (QApplication::activeWindow (), tl::to_qstring (title), tl::to_qstring (text), QMessageBox::StandardButton (b), QMessageBox::NoButton);

  for (unsigned int i = 0; i < sizeof (qbuttons) / sizeof (qbuttons [0]); ++i) {
    if (res == qbuttons [i]) {
      return (1 << i);
    }
  }
  return 0;
}

#else

typedef int (*msg_func) (QWidget *, const QString &, const QString &, int, int, int);

static int show_msg_box (msg_func mf, const std::string &title, const std::string &text, int buttons)
{
  std::vector<int> b;

  for (unsigned int i = 0; i < sizeof (qbuttons) / sizeof (qbuttons [0]); ++i) {
    if ((buttons & (1 << i)) != 0) {
      buttons &= ~(1 << i);
      b.push_back (qbuttons [i]);
    }
  }

  while (b.size () < 3) {
    b.push_back (0);
  }

  int res = (*mf) (QApplication::activeWindow (), title.c_str (), text.c_str (), b[0], b[1], b[2]);

  for (unsigned int i = 0; i < sizeof (qbuttons) / sizeof (qbuttons [0]); ++i) {
    if (res == qbuttons [i]) {
      return (1 << i);
    }
  }
  return 0;
}

#endif

static int critical (const std::string &title, const std::string &text, int buttons)
{
  return show_msg_box (&QMessageBox::critical, title, text, buttons);
}

static int info (const std::string &title, const std::string &text, int buttons)
{
  return show_msg_box (&QMessageBox::information, title, text, buttons);
}

static int question (const std::string &title, const std::string &text, int buttons)
{
  return show_msg_box (&QMessageBox::question, title, text, buttons);
}

static int warning (const std::string &title, const std::string &text, int buttons)
{
  return show_msg_box (&QMessageBox::warning, title, text, buttons);
}

struct MessageBox { };

Class<MessageBox> decl_MessageBox (QT_EXTERNAL_BASE (QMainWindow) "lay", "MessageBox",
  gsi::method ("Ok|#b_ok",               &b_ok,              "@brief A constant describing the 'Ok' button") +
  gsi::method ("Cancel|#b_cancel",       &b_cancel,          "@brief A constant describing the 'Cancel' button") +
  gsi::method ("Yes|#b_yes",             &b_yes,             "@brief A constant describing the 'Yes' button") +
  gsi::method ("No|#b_no",               &b_no,              "@brief A constant describing the 'No' button") +
  gsi::method ("Abort|#b_abort",         &b_abort,           "@brief A constant describing the 'Abort' button") +
  gsi::method ("Retry|#b_retry",         &b_retry,           "@brief A constant describing the 'Retry' button") +
  gsi::method ("Ignore|#b_ignore",       &b_ignore,          "@brief A constant describing the 'Ignore' button") +
  gsi::method ("warning", &warning, gsi::arg ("title"), gsi::arg ("text"), gsi::arg ("buttons"),
    "@brief Open a warning message box\n"
    "@param title The title of the window\n"
    "@param text The text to show\n"
    "@param buttons A combination (+) of button constants (\\Ok and so on) describing the buttons to show for the message box\n"
    "@return The button constant describing the button that was pressed\n"
  ) +
  gsi::method ("question", &question, gsi::arg ("title"), gsi::arg ("text"), gsi::arg ("buttons"),
    "@brief Open a question message box\n"
    "@param title The title of the window\n"
    "@param text The text to show\n"
    "@param buttons A combination (+) of button constants (\\Ok and so on) describing the buttons to show for the message box\n"
    "@return The button constant describing the button that was pressed\n"
  ) +
  gsi::method ("info", &info, gsi::arg ("title"), gsi::arg ("text"), gsi::arg ("buttons"),
    "@brief Open a information message box\n"
    "@param title The title of the window\n"
    "@param text The text to show\n"
    "@param buttons A combination (+) of button constants (\\Ok and so on) describing the buttons to show for the message box\n"
    "@return The button constant describing the button that was pressed\n"
  ) +
  gsi::method ("critical", &critical, gsi::arg ("title"), gsi::arg ("text"), gsi::arg ("buttons"),
    "@brief Open a critical (error) message box\n"
    "@param title The title of the window\n"
    "@param text The text to show\n"
    "@param buttons A combination (+) of button constants (\\Ok and so on) describing the buttons to show for the message box\n"
    "@return The button constant describing the button that was pressed\n"
  ),
  "@brief Various methods to display message boxes"
  "\n"
  "This class provides some basic message boxes. "
  "This functionality is provided through the static (class) methods \\warning, \\question and so on.\n"
  "\n"
  "Here is some example:\n"
  "\n"
  "@code\n"
  "# issue a warning and ask whether to continue:\n"
  "v = RBA::MessageBox::warning(\"Dialog Title\", \"Something happened. Continue?\", RBA::MessageBox::Yes + RBA::MessageBox::No)\n"
  "if v == RBA::MessageBox::Yes\n"
  "  ... continue ...\n"
  "end\n"
  "@/code\n"
  "\n"
  "If you have enabled the Qt binding, you can use \\QMessageBox directly.\n"
);

}

#endif
