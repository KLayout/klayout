
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

#include "layHelpAboutDialog.h"
#include "layApplication.h"
#include "layVersion.h"
#include "layHelpSource.h" // because of escape_xml
#include "layInit.h"
#include "dbInit.h"
#include "gsiInterpreter.h"

#include "ui_HelpAboutDialog.h"

#include <list>

namespace lay
{

// ------------------------------------------------------------
//  Implementation of the "help about" dialog

HelpAboutDialog::HelpAboutDialog (QWidget *parent)
  : QDialog (parent)
{
  mp_ui = new Ui::HelpAboutDialog ();
  mp_ui->setupUi (this);

  std::vector<std::string> build_options;
  if (lay::ApplicationBase::instance ()->ruby_interpreter ().available ()) {
    build_options.push_back (tl::to_string (tr ("Ruby interpreter ")) + lay::ApplicationBase::instance ()->ruby_interpreter ().version ());
  }
  if (lay::ApplicationBase::instance ()->python_interpreter ().available ()) {
    build_options.push_back (tl::to_string (tr ("Python interpreter ")) + lay::ApplicationBase::instance ()->python_interpreter ().version ());
  }
#if defined(HAVE_QTBINDINGS)
  build_options.push_back (tl::to_string (tr ("Qt bindings for scripts")));
#endif
#if defined(HAVE_64BIT_COORD)
  build_options.push_back (tl::to_string (tr ("Wide coordinates (64 bit)")));
#endif

  std::string s;

  s = "<html><body>";

  s += "<h1>";
  s += escape_xml (std::string (lay::Version::name ()) + " " + lay::Version::version ());
  s += "</h1>";

  std::vector<std::string> about_paras = tl::split (lay::Version::about_text (), "\n\n");
  for (std::vector<std::string>::const_iterator p = about_paras.begin (); p != about_paras.end (); ++p) {
    s += std::string ("<p>") + escape_xml (*p) + "</p>";
  }

  if (! build_options.empty ()) {
    s += "<p>";
    s += "<h4>";
    s += escape_xml (tl::to_string (QObject::tr ("Build options:")));
    s += "</h4><ul>";
    for (std::vector<std::string>::const_iterator bo = build_options.begin (); bo != build_options.end (); ++bo) {
      s += "<li>";
      s += escape_xml (*bo);
      s += "</li>";
    }
    s += "</ul>";
  }

  if (! lay::plugins ().empty () || ! db::plugins ().empty ()) {

    s += "<p>";
    s += "<h4>";
    s += escape_xml (tl::to_string (QObject::tr ("Binary extensions:")));
    s += "</h4><ul>";

    for (std::list<lay::PluginDescriptor>::const_iterator pd = lay::plugins ().begin (); pd != lay::plugins ().end (); ++pd) {
      s += "<li>";
      if (! pd->description.empty ()) {
        s += escape_xml (pd->description);
      } else {
        s += escape_xml (pd->path);
      }
      if (! pd->version.empty ()) {
        s += " (" + escape_xml (pd->version) + ")";
      }
      s += "</li>";
    }

    for (std::list<db::PluginDescriptor>::const_iterator pd = db::plugins ().begin (); pd != db::plugins ().end (); ++pd) {
      s += "<li>";
      if (! pd->description.empty ()) {
        s += escape_xml (pd->description);
      } else {
        s += escape_xml (pd->path);
      }
      if (! pd->version.empty ()) {
        s += " (" + escape_xml (pd->version) + ")";
      }
      s += "</li>";
    }

    s += "</ul>";

  }

  s += "</body></html>";

  std::string t = tl::to_string (QObject::tr ("About ")) + lay::Version::name ();

  setWindowTitle (tl::to_qstring (t));

  mp_ui->main->setWordWrap (true);
  mp_ui->main->setText (tl::to_qstring (s));
}

HelpAboutDialog::~HelpAboutDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

}
