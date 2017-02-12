
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


#include "extGerberImporter.h"
#include "extGerberImportDialog.h"
#include "extRS274XReader.h"

#include "dbStream.h"

#include "layPlugin.h"
#include "layFileDialog.h"
#include "layMainWindow.h"
#include "layLayoutView.h"
#include "tlExceptions.h"

#include <QApplication>

namespace ext
{

static const std::string cfg_pcb_import_spec ("pcb-import-spec");

class GerberImportPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  GerberImportPluginDeclaration () 
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_pcb_import_spec, ""));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::MenuEntry ("ext::import_gerber", "import_gerber_menu:edit", "file_menu.import_menu.end", tl::to_string (QObject::tr ("Gerber PCB")), true));
    menu_entries.push_back (lay::MenuEntry ("ext::import_gerber_new", "import_gerber_new:edit", "file_menu.import_menu.import_gerber_menu.end", tl::to_string (QObject::tr ("New Project"))));
    menu_entries.push_back (lay::MenuEntry ("ext::import_gerber_new_free", "import_gerber_new_free:edit", "file_menu.import_menu.import_gerber_menu.end", tl::to_string (QObject::tr ("New Project - Free Layer Mapping"))));
    menu_entries.push_back (lay::MenuEntry ("ext::import_gerber_open", "import_gerber_open:edit", "file_menu.import_menu.import_gerber_menu.end", tl::to_string (QObject::tr ("Open Project"))));
    menu_entries.push_back (lay::MenuEntry ("ext::import_gerber_recent", "import_gerber_recent:edit", "file_menu.import_menu.import_gerber_menu.end", tl::to_string (QObject::tr ("Recent Project"))));
  }

  virtual bool configure (const std::string &name, const std::string &value)
  {
    if (name == cfg_pcb_import_spec) {
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
    if (symbol == "ext::import_gerber_recent" || 
        symbol == "ext::import_gerber_new_free" ||
        symbol == "ext::import_gerber_new" ||
        symbol == "ext::import_gerber_open") {

      GerberImportData data;
      try {
        data.from_string (m_import_spec);
      } catch (...) {
        data = GerberImportData ();
      }

      if (symbol == "ext::import_gerber_new_free") {

        data.reset ();
        data.free_layer_mapping = true;

      } else if (symbol == "ext::import_gerber_new") {

        data.reset ();
        data.free_layer_mapping = false;

      } else if (symbol == "ext::import_gerber_open") {

        //  Get the name of the file to open
        lay::FileDialog open_dialog (QApplication::activeWindow (), tl::to_string (QObject::tr ("Gerber Import Project File")), tl::to_string (QObject::tr ("PCB project file (*.pcb);;All files (*)")));
        std::string fn = data.current_file;
        if (! open_dialog.get_open (fn)) {
          return true;
        }

        //  set the base dir to point to where the project file is located
        QFileInfo fi (tl::to_qstring (fn));
        data.base_dir = tl::to_string (fi.absoluteDir ().path ());
        data.load (fn);

      }

      lay::PluginRoot *config_root = lay::PluginRoot::instance ();

      GerberImportDialog dialog (QApplication::activeWindow (), &data);
      ext::GerberImporter importer;

      bool ok = false;
      while (! ok && dialog.exec ()) {

        BEGIN_PROTECTED
        data.setup_importer (&importer);
        ok = true;
        END_PROTECTED

      }

      if (ok) {

        config_root->config_set (cfg_pcb_import_spec, data.to_string ());
        config_root->config_end ();

        //  TODO: discard layout when an error occurs
        if (data.mode != ext::GerberImportData::ModeIntoLayout) {
          lay::MainWindow::instance ()->create_layout (data.mode == ext::GerberImportData::ModeSamePanel ? 2 : 1);
        }

        lay::LayoutView *view = lay::LayoutView::current ();
        int cv_index = view->active_cellview_index ();
        const lay::CellView &cv = view->cellview (cv_index);

        std::string lyp_file = data.get_layer_properties_file ();

        if (data.mode == ext::GerberImportData::ModeIntoLayout) {

          importer.read (cv->layout (), cv.cell_index ());
          view->create_initial_layer_props (cv_index, lyp_file, true /*add missing*/);

        } else {

          db::cell_index_type ci = importer.read (cv->layout ());

          view->create_initial_layer_props (cv_index, lyp_file, true /*add missing*/);
          view->select_cell_fit (ci, view->active_cellview_index ());

        }

        view->update_content ();

        config_root->config_set (cfg_pcb_import_spec, data.to_string ());
        config_root->config_end ();

      }

      return true;

    } else {
      return false;
    }
  }

private:
  mutable std::string m_filename;
  std::string m_import_spec;
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new ext::GerberImportPluginDeclaration (), 1200, "ext::GerberImportPlugin");

// ---------------------------------------------------------------
//  Plugin for the stream reader


class GerberReader
  : public db::ReaderBase
{
public:

  GerberReader (tl::InputStream &s)
    : m_stream (s)
  {
    //  .. nothing yet ..
  }

  ~GerberReader ()
  {
    //  .. nothing yet ..
  }

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions & /*options*/) throw (tl::Exception)
  {
    //  TODO: too simple, should provide at least a layer filtering.
    return read (layout);
  }

  virtual const db::LayerMap &read (db::Layout &layout) throw (tl::Exception)
  {
    GerberImportData data;

    std::string fn (m_stream.source ());
    if (! fn.empty ()) {
      QFileInfo fi (tl::to_qstring (fn));
      data.base_dir = tl::to_string (fi.absoluteDir ().path ());
    }

    data.load (m_stream);

    ext::GerberImporter importer;
    data.setup_importer (&importer);

    importer.read (layout);

    std::string lyr_file = data.get_layer_properties_file ();
    if (! lyr_file.empty ()) {
      layout.add_meta_info (db::MetaInfo ("layer-properties-file", "Layer Properties File", lyr_file));
    }

    return m_layers;
  }

  virtual const char *format () const 
  { 
    return "GerberPCB"; 
  }

private:
  tl::InputStream &m_stream;
  db::LayerMap m_layers;
};

class GerberFormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "GerberPCB"; }
  virtual std::string format_desc () const { return "Gerber PCB"; }
  virtual std::string format_title () const { return "Gerber PCB (project files)"; }
  virtual std::string file_format () const { return "Gerber PCB project files (*.pcb)"; }

  virtual bool detect (tl::InputStream &stream) const 
  {
    //  The test is that somewhere within the first 1000 bytes, a <pcb-project> XML tag appears.
    //  1000 bytes are within the initial block that the stream reader reads and hence
    //  this does not trigger any reread which is not available on some sources.
    //  TODO: this is pretty simple test. A more elaborate test would be in place here.
    std::string h = stream.read_all (1000);
    //  HINT: this assumes UTF8 or ISO encoding ...
    if (h.find ("<pcb-project>") != std::string::npos) {
      return true;
    } else {
      return false;
    }
  }

  virtual db::ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new GerberReader (s);
  }

  virtual db::WriterBase *create_writer () const 
  {
    return 0;
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return false;
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new GerberFormatDeclaration (), 1000, "GerberPCB");


}

