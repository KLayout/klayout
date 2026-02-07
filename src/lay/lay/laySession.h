
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


#ifndef HDR_laySession
#define HDR_laySession

#include "layDisplayState.h"
#include "layBookmarkList.h"
#include "layAnnotationShapes.h"
#include "layLayerProperties.h"

#include <string>
#include <vector>

namespace lay
{

class MainWindow;

struct SessionLayoutDescriptor
{
  SessionLayoutDescriptor ()
    : save_options_valid (false)
  { }

  std::string name;
  std::string file_path;
  db::LoadLayoutOptions load_options;
  db::SaveLayoutOptions save_options;
  bool save_options_valid;
};

struct SessionAnnotationDescriptor
{
  std::string class_name;
  std::string value_string;
};

struct SessionHiddenCellNames
{
  std::vector<std::string> hidden_cell_names;
  
  std::vector<std::string>::const_iterator begin () const { return hidden_cell_names.begin (); }
  std::vector<std::string>::const_iterator end () const { return hidden_cell_names.end (); }
  void push_back (const std::string &name) { hidden_cell_names.push_back (name); }
  std::string &back () { return hidden_cell_names.back (); }
  void reserve (size_t n) { hidden_cell_names.reserve (n); }
};

struct SessionCellViewDescriptor
{
  std::string layout_name;
  std::string tech_name;
  SessionHiddenCellNames hidden_cell_names;
};

struct SessionCellViewDescriptors
{
  std::vector<SessionCellViewDescriptor> cellviews;
  
  std::vector<SessionCellViewDescriptor>::const_iterator begin () const { return cellviews.begin (); }
  std::vector<SessionCellViewDescriptor>::const_iterator end () const { return cellviews.end (); }
  void push_back (const SessionCellViewDescriptor &desc) { cellviews.push_back (desc); }
  SessionCellViewDescriptor &back () { return cellviews.back (); }
  void reserve (size_t n) { cellviews.reserve (n); }
};

struct SessionAnnotationShapes
{
  std::vector<SessionAnnotationDescriptor> annotation_shapes;

  std::vector<SessionAnnotationDescriptor>::const_iterator begin_annotation_shapes () const { return annotation_shapes.begin (); }
  std::vector<SessionAnnotationDescriptor>::const_iterator end_annotation_shapes () const { return annotation_shapes.end (); }
  void add_annotation_shape (const SessionAnnotationDescriptor &shape) { annotation_shapes.push_back (shape); }
  SessionAnnotationDescriptor &back () { return annotation_shapes.back (); }
};

struct SessionViewDescriptor
{
  SessionViewDescriptor () : current_layer_list (0), active_cellview (-1) { }

  //  backward compatibility helper
  void set_layer_properties(const lay::LayerPropertiesList &list)
  {
    layer_properties_lists.clear ();
    layer_properties_lists.push_back (list);
  }

  std::string title;
  lay::DisplayState display_state;
  lay::BookmarkList bookmarks;
  std::vector<lay::LayerPropertiesList> layer_properties_lists;
  unsigned int current_layer_list;
  std::vector<std::string> rdb_filenames;
  std::vector<std::string> l2ndb_filenames;
  SessionCellViewDescriptors cellviews;
  SessionAnnotationShapes annotation_shapes;
  int active_cellview;
};

/**
 *  @brief This class implements the persistency of the session
 */
class Session
{
public:
  /**
   *  @brief Represents a session
   */
  Session ();

  /**
   *  @brief Copy the current application status to the session
   */
  void fetch (const lay::MainWindow &mw);

  /**
   *  @brief Restore the session inside the application
   */
  void restore (lay::MainWindow &mw);

  /**
   *  @brief Load the session from a file
   */
  void load (const std::string &filename);

  /**
   *  @brief Save the session to a file
   */
  void save (const std::string &filename);

  //  persistency API
  std::vector<SessionLayoutDescriptor>::const_iterator begin_layouts () const { return m_layouts.begin (); }
  std::vector<SessionLayoutDescriptor>::const_iterator end_layouts () const { return m_layouts.end (); }
  void add_layout (const SessionLayoutDescriptor &l) { m_layouts.push_back (l); }
  std::vector<SessionViewDescriptor>::const_iterator begin_views () const { return m_views.begin (); }
  std::vector<SessionViewDescriptor>::const_iterator end_views () const { return m_views.end (); }
  void add_view (const SessionViewDescriptor &l) { m_views.push_back (l); }
  const std::string &window_state () const { return m_window_state; }
  void set_window_state (const std::string &s) { m_window_state = s; }
  const std::string &window_geometry () const { return m_window_geometry; }
  void set_window_geometry (const std::string &s) { m_window_geometry = s; }
  int width () const { return m_width; }
  void set_width (int n) { m_width = n; }
  int height () const { return m_height; }
  void set_height (int n) { m_height = n; }
  int current_view () const { return m_current_view; }
  void set_current_view (int n) { m_current_view = n; }

private:
  std::vector<SessionLayoutDescriptor> m_layouts;
  std::vector<SessionViewDescriptor> m_views;
  int m_width, m_height;
  int m_current_view;
  std::string m_window_state;
  std::string m_window_geometry;
  std::string m_base_dir;

  std::string make_absolute (const std::string &fp) const;
};

}

#endif

