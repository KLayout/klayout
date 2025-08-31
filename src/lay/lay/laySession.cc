
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


#include "laySession.h"
#include "layBookmarkList.h"
#include "layCellView.h"
#include "layLayoutView.h"
#include "layMainWindow.h"
#include "laySession.h"
#include "layStream.h"
#include "tlXMLParser.h"
#include "tlStream.h"
#include "tlFileUtils.h"
#include "tlUri.h"
#include "dbStream.h"
#include "dbLayoutToNetlist.h"
#include "rdb.h"

#include <fstream>
#include <QFileInfo>
#include <QDir>

namespace lay
{

Session::Session ()
  : m_width (0), m_height (0), m_current_view (-1)
{
  //  .. nothing yet ..
}

void 
Session::fetch (const lay::MainWindow &mw)
{
  m_width = mw.size ().width ();
  m_height = mw.size ().height ();
  m_window_state = mw.saveState ().toBase64 ().data ();
  m_window_geometry = mw.saveGeometry ().toBase64 ().data ();
  m_current_view = mw.current_view_index ();

  std::vector <std::string> layout_names;
  LayoutHandle::get_names (layout_names);

  for (std::vector <std::string>::const_iterator l = layout_names.begin (); l != layout_names.end (); ++l) {
    LayoutHandle *lh = LayoutHandle::find (*l);
    if (lh) {
      m_layouts.push_back (SessionLayoutDescriptor ());
      m_layouts.back ().name = *l;
      m_layouts.back ().file_path = tl::InputStream::absolute_file_path (lh->filename ());
      m_layouts.back ().load_options = lh->load_options ();
      m_layouts.back ().save_options = lh->save_options ();
      m_layouts.back ().save_options_valid = lh->save_options_valid ();
    }
  }

  for (unsigned int i = 0; i < mw.views (); ++i) {

    const lay::LayoutView *view = mw.view (i);
    m_views.push_back (SessionViewDescriptor ());
    SessionViewDescriptor &view_desc = m_views.back ();

    view_desc.active_cellview = view->active_cellview_index ();

    view_desc.title = view->title_string ();

    view_desc.cellviews.reserve (view->cellviews ());

    for (unsigned int j = 0; j < view->num_rdbs (); ++j) {

      const rdb::Database *rdb = view->get_rdb (j);
      if (rdb && ! rdb->filename ().empty ()) {
        view_desc.rdb_filenames.push_back (tl::InputStream::absolute_file_path (rdb->filename ()));
      }

    }

    for (unsigned int j = 0; j < view->num_l2ndbs (); ++j) {

      const db::LayoutToNetlist *l2ndb = view->get_l2ndb (j);
      if (l2ndb && ! l2ndb->filename ().empty ()) {
        view_desc.l2ndb_filenames.push_back (tl::InputStream::absolute_file_path (l2ndb->filename ()));
      }

    }

    for (unsigned int j = 0; j < view->cellviews (); ++j) {

      view_desc.cellviews.push_back (SessionCellViewDescriptor ());

      SessionCellViewDescriptor &cvd = view_desc.cellviews.back ();
      
      cvd.layout_name = view->cellview(j)->name ();
      cvd.tech_name = view->cellview(j)->tech_name ();

      const std::set<LayoutView::cell_index_type> &hidden_cells = view->hidden_cells (j);
      cvd.hidden_cell_names.reserve (hidden_cells.size ());
      for (std::set<LayoutView::cell_index_type>::const_iterator hc = hidden_cells.begin (); hc != hidden_cells.end (); ++hc) {
        cvd.hidden_cell_names.push_back (view->cellview(j)->layout ().cell_name (*hc));
      }

    }

    view->save_view (view_desc.display_state);
    view_desc.bookmarks = view->bookmarks ();
    view_desc.current_layer_list = view->current_layer_list ();
    view_desc.layer_properties_lists.clear ();
    for (unsigned int i = 0; i < view->layer_lists (); ++i) {
      view_desc.layer_properties_lists.push_back (view->get_properties (i));
    }

    for (lay::AnnotationShapes::iterator a = view->annotation_shapes ().begin (); a != view->annotation_shapes ().end (); ++a) {
      if (a->ptr ()->class_name () != 0) {
        view_desc.annotation_shapes.add_annotation_shape (SessionAnnotationDescriptor ());
        view_desc.annotation_shapes.back ().class_name = a->ptr ()->class_name ();
        view_desc.annotation_shapes.back ().value_string = a->ptr ()->to_string ();
      }
    }

  }

}

std::string
Session::make_absolute (const std::string &fp) const
{
  tl::URI fp_uri (fp);
  if (! m_base_dir.empty () && ! tl::is_absolute (fp_uri.path ())) {
    return tl::URI (m_base_dir).resolved (fp_uri).to_abstract_path ();
  } else {
    return fp;
  }
}

void 
Session::restore (lay::MainWindow &mw)
{
  mw.close_all ();

  mw.resize (QSize (m_width, m_height));
  if (! m_window_geometry.empty ()) {
    mw.restoreGeometry (QByteArray::fromBase64 (m_window_geometry.c_str ()));
  }
  if (! m_window_state.empty ()) {
    mw.restoreState (QByteArray::fromBase64 (m_window_state.c_str ()));
  }

  std::map <std::string, const SessionLayoutDescriptor *> ld_by_name;
  for (std::vector <SessionLayoutDescriptor>::const_iterator ld = m_layouts.begin (); ld != m_layouts.end (); ++ld) {
    ld_by_name.insert (std::make_pair (ld->name, ld.operator-> ()));
  }

  for (unsigned int iv = 0; iv < m_views.size (); ++iv) {

    lay::LayoutView *view = mw.view (mw.create_view ());

    SessionViewDescriptor &vd = m_views[iv];

    for (std::vector<SessionCellViewDescriptor>::const_iterator cvd = vd.cellviews.begin (); cvd != vd.cellviews.end (); ++cvd) {

      unsigned int cv = 0;

      lay::LayoutHandle *lh = lay::LayoutHandle::find (cvd->layout_name);
      if (lh) {
        cv = view->add_layout (lh, true /*add*/);
      } else {

        std::map <std::string, const SessionLayoutDescriptor *>::const_iterator ld = ld_by_name.find (cvd->layout_name);

        std::string fp = make_absolute (ld->second->file_path);

        bool ok = false;
        if (ld != ld_by_name.end ()) {
          try {
            cv = view->load_layout (fp, ld->second->load_options, cvd->tech_name, true /*add*/);
            view->cellview (cv)->set_save_options (ld->second->save_options, ld->second->save_options_valid);
            ok = true;
          } catch (...) { }
        } 
        if (!ok) {
          //  fallback if layout cannot be loaded
          cv = view->create_layout (true /*add*/);
          view->cellview (cv)->set_tech_name (cvd->tech_name);
        }

        view->cellview (cv)->rename (cvd->layout_name, true /*force*/);

      }

      const db::Layout &layout = view->cellview (cv)->layout ();

      for (std::vector<std::string>::const_iterator hc = cvd->hidden_cell_names.begin (); hc != cvd->hidden_cell_names.end (); ++hc) {
        std::pair<bool, db::cell_index_type> cc = layout.cell_by_name (hc->c_str ());
        if (cc.first) {
          view->hide_cell (cc.second, cv);
        }
      }

    }

    view->set_title (vd.title);
    view->bookmarks (vd.bookmarks);
    view->goto_view (vd.display_state);

    unsigned int index = 0;
    for (std::vector<lay::LayerPropertiesList>::const_iterator l = vd.layer_properties_lists.begin (); l != vd.layer_properties_lists.end (); ++l, ++index) {
      if (index < view->layer_lists ()) {
        view->set_properties (index, *l);
      } else {
        view->insert_layer_list (index, *l);
      }
    }
    while (view->layer_lists () > index) {
      view->delete_layer_list (index);
    }

    view->set_current_layer_list (vd.current_layer_list);

    for (unsigned int j = 0; j < vd.rdb_filenames.size (); ++j) {

      std::unique_ptr<rdb::Database> rdb (new rdb::Database ());

      try {
        rdb->load (make_absolute (vd.rdb_filenames[j]));
        view->add_rdb (rdb.release ());
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      } catch (...) {
      }

    }

    for (unsigned int j = 0; j < vd.l2ndb_filenames.size (); ++j) {

      try {
        db::LayoutToNetlist *l2ndb = db::LayoutToNetlist::create_from_file (make_absolute (vd.l2ndb_filenames [j]));
        view->add_l2ndb (l2ndb);
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      } catch (...) {
      }

    }

    lay::AnnotationShapes &as = view->annotation_shapes ();
    as.reserve (vd.annotation_shapes.annotation_shapes.size ());
    for (std::vector<SessionAnnotationDescriptor>::const_iterator ad = vd.annotation_shapes.annotation_shapes.begin (); ad != vd.annotation_shapes.annotation_shapes.end (); ++ad) {
      db::DUserObjectBase *obj = db::DUserObjectFactory::create (ad->class_name.c_str (), ad->value_string.c_str (), ! m_base_dir.empty () ? m_base_dir.c_str () : 0);
      if (obj) {
        as.insert (db::DUserObject (obj));
      } else {
        tl::warn << tl::to_string (tr ("Unable to restore session user object with unknown class: ")) << ad->class_name;
      }
    }

    view->update_content ();
   
    if (vd.active_cellview >= 0) {
      view->set_active_cellview_index (vd.active_cellview);
    }

  }

  if (current_view () >= 0) {
    mw.select_view (current_view ());
  }
}

//  declaration of the session file XML structure
static const tl::XMLStruct <Session>
session_structure ()
{
  return tl::XMLStruct <Session> ("session",
    tl::make_member<int, Session> (&Session::width, &Session::set_width, "window-width") +
    tl::make_member<int, Session> (&Session::height, &Session::set_height, "window-height") +
    tl::make_member<std::string, Session> (&Session::window_state, &Session::set_window_state, "window-state") +
    tl::make_member<std::string, Session> (&Session::window_geometry, &Session::set_window_geometry, "window-geometry") +
    tl::make_member<int, Session> (&Session::current_view, &Session::set_current_view, "current-view") +
    tl::make_element<SessionLayoutDescriptor, std::vector<SessionLayoutDescriptor>::const_iterator, Session> (&Session::begin_layouts, &Session::end_layouts, &Session::add_layout, "layout",
      tl::make_member<std::string, SessionLayoutDescriptor> (&SessionLayoutDescriptor::name, "name") +
      tl::make_member<std::string, SessionLayoutDescriptor> (&SessionLayoutDescriptor::file_path, "file-path") +
      tl::make_member<bool, SessionLayoutDescriptor> (&SessionLayoutDescriptor::save_options_valid, "save-options-valid") +
      tl::make_element<db::SaveLayoutOptions, SessionLayoutDescriptor> (&SessionLayoutDescriptor::save_options, "save-options",
        db::save_options_xml_element_list ()
      ) +
      tl::make_element<db::LoadLayoutOptions, SessionLayoutDescriptor> (&SessionLayoutDescriptor::load_options, "load-options",
        db::load_options_xml_element_list ()
      )
    ) +
    tl::make_element<SessionViewDescriptor, std::vector<SessionViewDescriptor>::const_iterator, Session> (&Session::begin_views, &Session::end_views, &Session::add_view, "view",
      tl::make_member<std::string, SessionViewDescriptor> (&SessionViewDescriptor::title, "title") +
      tl::make_member<int, SessionViewDescriptor> (&SessionViewDescriptor::active_cellview, "active-cellview-index") +
      tl::make_element<lay::DisplayState, SessionViewDescriptor> (&SessionViewDescriptor::display_state, "display", lay::DisplayState::xml_format ()) +
      tl::make_element<SessionCellViewDescriptors, SessionViewDescriptor> (&SessionViewDescriptor::cellviews, "cellviews",
        tl::make_element<SessionCellViewDescriptor, std::vector<SessionCellViewDescriptor>::const_iterator, SessionCellViewDescriptors> (&SessionCellViewDescriptors::begin, &SessionCellViewDescriptors::end, &SessionCellViewDescriptors::push_back, "cellview",
          tl::make_member<std::string, SessionCellViewDescriptor> (&SessionCellViewDescriptor::layout_name, "layout-ref") +
          tl::make_member<std::string, SessionCellViewDescriptor> (&SessionCellViewDescriptor::tech_name, "tech-name") +
          tl::make_element<SessionHiddenCellNames, SessionCellViewDescriptor> (&SessionCellViewDescriptor::hidden_cell_names, "hidden-cells",
            tl::make_member<std::string, std::vector<std::string>::const_iterator, SessionHiddenCellNames> (&SessionHiddenCellNames::begin, &SessionHiddenCellNames::end, &SessionHiddenCellNames::push_back, "hidden-cell")
          )
        )
      ) +
      tl::make_element<BookmarkList, SessionViewDescriptor> (&SessionViewDescriptor::bookmarks, "bookmarks",
        tl::make_element<BookmarkListElement, BookmarkList::const_iterator, BookmarkList> (&BookmarkList::begin, &BookmarkList::end, &BookmarkList::add, "bookmark", BookmarkListElement::xml_format())
      ) +
      tl::make_element<std::vector<std::string>, SessionViewDescriptor> (&SessionViewDescriptor::rdb_filenames, "rdb-files",
        tl::make_member<std::string, std::vector<std::string>::const_iterator, std::vector<std::string> > (&std::vector<std::string>::begin, &std::vector<std::string>::end, &std::vector<std::string>::push_back, "rdb-file")
      ) +
      tl::make_element<std::vector<std::string>, SessionViewDescriptor> (&SessionViewDescriptor::l2ndb_filenames, "l2ndb-files",
        tl::make_member<std::string, std::vector<std::string>::const_iterator, std::vector<std::string> > (&std::vector<std::string>::begin, &std::vector<std::string>::end, &std::vector<std::string>::push_back, "l2ndb-file")
      ) +
      //  for backward compatibility:
      tl::make_element<lay::LayerPropertiesList, SessionViewDescriptor> (&SessionViewDescriptor::set_layer_properties, "layer-properties", lay::LayerPropertiesList::xml_format ()) +
      tl::make_member<unsigned int, SessionViewDescriptor> (&SessionViewDescriptor::current_layer_list, "current-layer-property-tab") +
      tl::make_element<std::vector<lay::LayerPropertiesList>, SessionViewDescriptor> (&SessionViewDescriptor::layer_properties_lists, "layer-properties-tabs",
        tl::make_element<lay::LayerPropertiesList, std::vector<lay::LayerPropertiesList>::const_iterator, std::vector<lay::LayerPropertiesList> > (&std::vector<lay::LayerPropertiesList>::begin, &std::vector<lay::LayerPropertiesList>::end, &std::vector<lay::LayerPropertiesList>::push_back, "layer-properties", lay::LayerPropertiesList::xml_format())
      ) +
      tl::make_element<SessionAnnotationShapes, SessionViewDescriptor> (&SessionViewDescriptor::annotation_shapes, "annotations",
        tl::make_element<SessionAnnotationDescriptor, std::vector<SessionAnnotationDescriptor>::const_iterator, SessionAnnotationShapes> (&SessionAnnotationShapes::begin_annotation_shapes, &SessionAnnotationShapes::end_annotation_shapes, &SessionAnnotationShapes::add_annotation_shape, "annotation",
          tl::make_member<std::string, SessionAnnotationDescriptor> (&SessionAnnotationDescriptor::class_name, "class") +
          tl::make_member<std::string, SessionAnnotationDescriptor> (&SessionAnnotationDescriptor::value_string, "value")
        )
      )
    )
  );
}

void 
Session::load (const std::string &fn)
{
  //  Take the path to the file as the base directory
  m_base_dir = tl::to_string (QFileInfo (tl::to_qstring (fn)).absolutePath ());

  tl::XMLFileSource in (fn);

  session_structure ().parse (in, *this);

  tl::log << "Loaded session from " << fn;
}

void 
Session::save (const std::string &fn)
{
  tl::OutputStream os (fn, tl::OutputStream::OM_Plain);
  session_structure ().write (os, *this);

  tl::log << "Saved session to " << fn;
}

}

