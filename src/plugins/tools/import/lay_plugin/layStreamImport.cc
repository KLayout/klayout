
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


#include "layStreamImportDialog.h"
#include "layStreamImporter.h"

#include "dbStream.h"

#include "tlExceptions.h"
#include "layPlugin.h"
#include "layMainWindow.h"
#include "layFileDialog.h"

#include <QApplication>

namespace lay
{

static const std::string cfg_stream_import_spec ("stream-import-spec2");

class StreamImportPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  StreamImportPluginDeclaration () 
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_stream_import_spec, ""));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("lay::import_stream", "import_stream:edit", "file_menu.import_menu.end", tl::to_string (QObject::tr ("Other Files Into Current"))));
  }

  virtual bool configure (const std::string &name, const std::string &value)
  {
    if (name == cfg_stream_import_spec) {
      m_import_spec = value;
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
    if (symbol == "lay::import_stream") {

      lay::LayoutView *view = lay::LayoutView::current ();
      if (! view) {
        throw tl::Exception (tl::to_string (QObject::tr ("No view open to import files into")));
      }

      StreamImportData data;
      try {
        data.from_string (m_import_spec);
      } catch (...) {
        data = StreamImportData ();
      }

      lay::Dispatcher *config_root = lay::Dispatcher::instance ();

      StreamImportDialog dialog (QApplication::activeWindow (), &data);
      lay::StreamImporter importer;

      bool ok = false;
      while (! ok && dialog.exec ()) {

        BEGIN_PROTECTED
        data.setup_importer (&importer);
        ok = true;
        END_PROTECTED

      }

      if (ok) {

        //  clear selection 
        lay::MainWindow::instance ()->cancel ();

        config_root->config_set (cfg_stream_import_spec, data.to_string ());
        config_root->config_end ();

        int cv_index = view->active_cellview_index ();
        const lay::CellView &cv = view->cellview (cv_index);

        std::vector <unsigned int> new_layer_ids;
        importer.read (cv->layout (), cv.cell_index (), new_layer_ids);

        //  create the initial layer properties
        std::vector <lay::ParsedLayerSource> new_layers;
        new_layers.reserve (cv->layout ().layers ());

        for (unsigned int i = 0; i < new_layer_ids.size (); ++i) {
          if (cv->layout ().is_valid_layer (new_layer_ids [i])) {
            db::LayerProperties lp (cv->layout ().get_properties (new_layer_ids [i]));
            new_layers.push_back (lay::ParsedLayerSource (lp, cv_index));
          }
        }

        std::sort (new_layers.begin (), new_layers.end ());

        lay::LayerPropertiesList new_props (view->get_properties ());

        //  create the layers and do a basic recoloring ..
        for (std::vector <lay::ParsedLayerSource>::const_iterator l = new_layers.begin (); l != new_layers.end (); ++l) {
          lay::LayerProperties p;
          p.set_source (*l);
          view->init_layer_properties (p);
          new_props.push_back (p);
        }

        view->set_properties (new_props);

        view->update_content ();

        config_root->config_set (cfg_stream_import_spec, data.to_string ());
        config_root->config_end ();

      }

      return true;

    } else {
      return false;
    }
  }

private:
  std::string m_import_spec;
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::StreamImportPluginDeclaration (), 1300, "lay::StreamImportPlugin");

}

