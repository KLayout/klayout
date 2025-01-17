
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


#include "layHelpDialog.h"
#include "layHelpSource.h"
#include "layMainWindow.h"
#include "laybasicCommon.h"
#include "gsiDecl.h"

#if defined(HAVE_QTBINDINGS)
# include "gsiQtGuiExternals.h"
# include "gsiQtWidgetsExternals.h"
#else
# define QT_EXTERNAL_BASE(x)
#endif

namespace gsi
{

lay::HelpDialog *new_help_dialog (bool modal)
{
  return new lay::HelpDialog (lay::MainWindow::instance (), modal);
}

lay::HelpDialog *new_help_dialog_with_parent (QWidget *parent, bool modal)
{
  return new lay::HelpDialog (parent, modal);
}

Class<lay::HelpDialog> decl_HelpDialog (QT_EXTERNAL_BASE (QDialog) "lay", "HelpDialog",
  method ("new", new_help_dialog, gsi::arg ("modal"),
    "@brief Creates a new help dialog\n"
    "If the modal flag is true, the dialog will be shown as a modal window.\n"
  ) +
#if defined(HAVE_QTBINDINGS)
  method ("new", new_help_dialog_with_parent, gsi::arg ("parent"), gsi::arg ("modal"),
    "@brief Creates a new help dialog\n"
    "If the modal flag is true, the dialog will be shown as a modal window.\n"
  ) +
#else
  method ("show", &lay::HelpDialog::show, 
    "@brief Shows the dialog\n"
  ) +
  method ("exec", &lay::HelpDialog::exec, 
    "@brief Executes the dialog (shows it modally)\n"
  ) +
#endif
  method ("search", &lay::HelpDialog::search, gsi::arg ("topic"),
    "@brief Issues a search on the specified topic\n"
    "This method will call the search page with the given topic.\n"
  ) +
  method ("load", &lay::HelpDialog::load, gsi::arg ("url"),
    "@brief Loads the specified URL\n"
    "This method will call the page with the given URL.\n"
  ),

  "@brief The help dialog\n"
  "\n"
  "This class makes the help dialog available as an individual object.\n"
  "\n"
  "This class has been added in version 0.25.\n"

);

LAYBASIC_PUBLIC Class<lay::BrowserSource> &laybasicdecl_BrowserSource ();

static lay::HelpSource *plain_help_source ()
{
  return new lay::HelpSource (false);
}

Class<lay::HelpSource> decl_HelpSource (laybasicdecl_BrowserSource (), "lay", "HelpSource",
  gsi::constructor ("plain", &plain_help_source, "@brief Reserved for internal use") +
  gsi::method ("scan", static_cast<void (lay::HelpSource::*) ()> (&lay::HelpSource::scan), "@brief Reserved internal use") +
#if defined(HAVE_QTBINDINGS) && defined(HAVE_QT_XML)
  gsi::method ("get_dom", &lay::HelpSource::get_dom, gsi::arg ("path"), "@brief Reserved for internal use") +
#endif
  gsi::method ("set_option", &lay::HelpSource::set_option, gsi::arg ("key"), gsi::arg ("value"), "@brief Reserved for internal use") +
  gsi::method ("get_option", &lay::HelpSource::get_option, gsi::arg ("key"), "@brief Reserved for internal use") +
  gsi::method ("urls", &lay::HelpSource::urls, "@brief Reserved for internal use") +
  gsi::method ("title_for", &lay::HelpSource::title_for, gsi::arg ("path"), "@brief Reserved internal use") +
  gsi::method ("parent_of", &lay::HelpSource::parent_of, gsi::arg ("path"), "@brief Reserved internal use") +
  gsi::method ("create_index_file", &lay::HelpSource::create_index_file, gsi::arg ("path"), "@brief Reserved internal use"),
  "@brief A BrowserSource implementation delivering the help text for the help dialog\n"
  "This class can be used together with a \\BrowserPanel or \\BrowserDialog object to implement "
  "custom help systems.\n"
  "\n"
  "The basic URL's served by this class are: \"int:/index.xml\" for the index page and "
  "\"int:/search.xml?string=...\" for the search topic retrieval.\n"
  "\n"
  "This class has been added in version 0.25.\n"
);

}
