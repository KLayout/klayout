
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "dbLEFImporter.h"
#include "dbDEFImporter.h"

#include "layLEFDEFImportDialogs.h"
#include "layPlugin.h"
#include "layMainWindow.h"
#include "layFileDialog.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlXMLParser.h"
#include "tlExceptions.h"

#include <QFileInfo>
#include <QApplication>

namespace lay
{

// -----------------------------------------------------------------------------------------------
//  Plugin declaration

static const std::string cfg_lef_import_spec ("lef-import-spec");
static const std::string cfg_def_import_spec ("def-import-spec");

class LEFDEFImportPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  LEFDEFImportPluginDeclaration () 
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_lef_import_spec, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_def_import_spec, ""));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::MenuEntry ("db::import_lef", "import_lef:edit", "file_menu.import_menu.end", tl::to_string (QObject::tr ("LEF"))));
    menu_entries.push_back (lay::MenuEntry ("db::import_def", "import_def:edit", "file_menu.import_menu.end", tl::to_string (QObject::tr ("DEF/LEF"))));
  }

  virtual bool configure (const std::string &name, const std::string &value)
  {
    if (name == cfg_lef_import_spec) {
      m_lef_spec = value;
      return true;
    } else if (name == cfg_def_import_spec) {
      m_def_spec = value;
      return true;
    } else {
      return false;
    }
  }

  virtual void config_finalize ()
  {
    // .. nothing yet ..
  }

  virtual bool menu_activated (const std::string &symbol) const
  {
    if (symbol == "db::import_lef" || symbol == "db::import_def") {

      bool import_lef = (symbol == "db::import_lef");

      LEFDEFImportData data;
      try {
        if (import_lef) {
          data.from_string (m_lef_spec);
        } else {
          data.from_string (m_def_spec);
        }
      } catch (...) {
        data = LEFDEFImportData ();
      }

      LEFDEFImportOptionsDialog import_dialog (QApplication::activeWindow (), import_lef);
      if (import_dialog.exec_dialog (data)) {

        //  clear selection 
        lay::MainWindow::instance ()->cancel ();

        //  store configuration
        lay::PluginRoot *config_root = lay::PluginRoot::instance ();
        if (import_lef) {
          config_root->config_set (cfg_lef_import_spec, data.to_string ());
        } else {
          config_root->config_set (cfg_def_import_spec, data.to_string ());
        }
        config_root->config_end ();

        std::auto_ptr<db::Layout> layout (new db::Layout ());

        tl::InputStream stream (data.file);

        std::string tech_name = lay::MainWindow::instance ()->initial_technology ();
        if (! db::Technologies::instance ()->has_technology (tech_name)) {
          tech_name.clear (); // use default technology
        }
        const db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
        db::LEFDEFReaderOptions options;
        if (tech) {
          const db::LEFDEFReaderOptions *tech_options = dynamic_cast<const db::LEFDEFReaderOptions *>(tech->load_layout_options ().get_options ("LEFDEF"));
          if (tech_options) {
            options = *tech_options;
          }
        }

        db::LEFDEFLayerDelegate layers (&options);
        layers.prepare (*layout);
        layout->dbu (options.dbu ());

        if (import_lef) {

          tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Reading LEF file")));

          db::LEFImporter importer;

          for (std::vector<std::string>::const_iterator l = options.begin_lef_files (); l != options.end_lef_files (); ++l) {
            tl::InputStream lef_stream (*l);
            tl::log << tl::to_string (QObject::tr ("Reading")) << " " << *l;
            importer.read (lef_stream, *layout, layers);
          }

          tl::log << tl::to_string (QObject::tr ("Reading")) << " " << data.file;
          importer.read (stream, *layout, layers);

        } else {

          tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Reading DEF file")));

          db::DEFImporter importer;

          QFileInfo def_fi (tl::to_qstring (data.file));

          std::vector<std::string> lef_files;
          lef_files.insert (lef_files.end (), options.begin_lef_files (), options.end_lef_files ());
          lef_files.insert (lef_files.end (), data.lef_files.begin (), data.lef_files.end ());

          for (std::vector<std::string>::const_iterator l = lef_files.begin (); l != lef_files.end (); ++l) {

            QFileInfo fi (tl::to_qstring (*l));
            if (fi.isAbsolute ()) {
              tl::InputStream lef_stream (*l);
              tl::log << tl::to_string (QObject::tr ("Reading")) << " " << *l;
              importer.read_lef (lef_stream, *layout, layers);
            } else {
              std::string ex_l = tl::to_string (def_fi.absoluteDir ().absoluteFilePath (tl::to_qstring (*l)));
              tl::InputStream lef_stream (ex_l);
              tl::log << tl::to_string (QObject::tr ("Reading")) << " " << *l;
              importer.read_lef (lef_stream, *layout, layers);
            }

          }

          tl::log << tl::to_string (QObject::tr ("Reading")) << " " << data.file;

          importer.read (stream, *layout, layers);

        }

        layers.finish (*layout);

        lay::LayoutView *view = lay::LayoutView::current ();
        if (! view || data.mode == 1) {
          view = lay::MainWindow::instance ()->view (lay::MainWindow::instance ()->create_view ());
        }

        lay::LayoutHandle *handle = new lay::LayoutHandle (layout.release (), std::string ());
        handle->rename (tl::to_string (QFileInfo (tl::to_qstring (data.file)).fileName ()));
        handle->set_tech_name (tech_name);
        view->add_layout (handle, data.mode == 2);

      }

      return true;

    } else {
      return false;
    }
  }

private:
  std::string m_lef_spec;
  std::string m_def_spec;
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::LEFDEFImportPluginDeclaration (), 1400, "db::LEFDEFImportPlugin");

}

