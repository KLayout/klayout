
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

#include "layBrowserPanel.h"
#include "layBrowserDialog.h"
#include "tlExceptions.h"

namespace lay 
{

BrowserDialog::BrowserDialog ()
  : QDialog (0)
{
  Ui::BrowserDialog::setupUi (this);

  setObjectName (QString::fromUtf8 ("html_browser"));
  set_home ("int:/index.html");
  show ();
}

BrowserDialog::BrowserDialog (QWidget *parent)
  : QDialog (parent)
{
  Ui::BrowserDialog::setupUi (this);

  setObjectName (QString::fromUtf8 ("html_browser"));
  set_home ("int:/index.html");
  show ();
}

BrowserDialog::BrowserDialog (const std::string &html)
  : QDialog (0), m_default_source (html)
{
  Ui::BrowserDialog::setupUi (this);

  setObjectName (QString::fromUtf8 ("html_browser"));
  set_source (& m_default_source);
  set_home ("int:/index.html");
  show ();
}

BrowserDialog::BrowserDialog (QWidget *parent, const std::string &html)
  : QDialog (parent), m_default_source (html)
{
  Ui::BrowserDialog::setupUi (this);

  setObjectName (QString::fromUtf8 ("html_browser"));
  set_source (& m_default_source);
  set_home ("int:/index.html");
  show ();
}

BrowserDialog::~BrowserDialog ()
{
  set_source (0);
}

void 
BrowserDialog::load (const std::string &s)
{
  browser->load (s);
}

void 
BrowserDialog::set_source (BrowserSource *source)
{
  browser->set_source (source);
}

void 
BrowserDialog::set_home (const std::string &url)
{
  browser->set_home (url);
}

void 
BrowserDialog::set_label (const std::string &label)
{
  browser->set_label (label);
}

void 
BrowserDialog::set_search_url (const std::string &url, const std::string &query_item)
{
  browser->set_search_url (url, query_item);
}

void 
BrowserDialog::search (const std::string &s)
{
  browser->search (s);
}

void 
BrowserDialog::reload ()
{
  browser->reload ();
}

void
BrowserDialog::accept ()
{
  BEGIN_PROTECTED
  closed ();
  END_PROTECTED
  QDialog::accept ();
}

}

#endif

