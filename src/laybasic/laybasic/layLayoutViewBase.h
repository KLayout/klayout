
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#ifndef HDR_layLayoutViewBase
#define HDR_layLayoutViewBase

#include "laybasicCommon.h"

#include <vector>
#include <map>
#include <set>
#include <list>
#include <string>
#include <memory>

#include "layLayerProperties.h"
#include "layAnnotationShapes.h"
#include "layBookmarkList.h"
#include "layDispatcher.h"
#include "layLayoutCanvas.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layLineStylePalette.h"
#include "layCellView.h"
#include "layViewOp.h"
#include "layEditable.h"
#include "layPlugin.h"
#include "layDisplayState.h"
#include "layBookmarkList.h"
#include "gsi.h"
#include "tlException.h"
#include "tlEvents.h"
#include "tlTimer.h"
#include "tlDeferredExecution.h"
#include "dbInstElement.h"

#if defined(HAVE_QT)
# include <QImage>
class QWidget;
#endif

namespace rdb {
  class Database;
}

namespace db {
  class Layout;
  class Manager;
  class SaveLayoutOptions;
  class LayoutToNetlist;
}

namespace lay {

class LayoutView;
class MouseTracker;
class ZoomService;
class SelectionService;
class MoveService;

#if defined(HAVE_QT)
class LayerControlPanel;
class HierarchyControlPanel;
class EditorOptionsPages;
#endif

/**
 *  @brief Stores a layer reference to create layers which have been added by some action
 *
 *  This object is delivered by LayoutViewBase::layer_snapshot and can be used in add_missing_layers
 *  to create new layer views for layers which have been created between layer_snapshot and 
 *  add_missing_layers.
 */
struct LayerState
{
  /** 
   *  @brief Constructor
   */
  LayerState () { }

  std::set<lay::ParsedLayerSource> present;
};

/**
 *  @brief A layer display properties structure
 *
 *  The layer properties encapsulate the settings relevant for
 *  the display of a layer.
 *
 *  "brightness" is a index that indicates how much to make the
 *  color brighter to darker rendering the effective color 
 *  (eff_frame_color (), eff_fill_color ()). It's value is roughly between
 *  -255 and 255.
 */
struct LAYBASIC_PUBLIC LayerDisplayProperties
{
  LayerDisplayProperties ();

  bool operator== (const LayerDisplayProperties &d);
  bool operator!= (const LayerDisplayProperties &d);

  /**
   *  @brief render the effective frame color
   *  
   *  The effective frame color is computed from the frame color brightness and the
   *  frame color.
   */
  tl::color_t eff_frame_color () const;

  /**
   *  @brief render the effective frame color
   *  
   *  The effective frame color is computed from the frame color brightness and the
   *  frame color.
   */
  tl::color_t eff_fill_color () const;

  //  display styles
  tl::color_t frame_color;
  tl::color_t fill_color;
  int frame_brightness;
  int fill_brightness;
  unsigned int dither_pattern;
  bool visible;
  bool transparent;
  int width;
  bool marked;
  int animation;
  std::string name;
};

/**
 *  @brief The layout view object
 *
 *  The layout view is responsible for displaying one or a set of layouts.
 *  It is composed of a canvas and controls to control the appearance.
 *  It manages the layer display list, bookmark list etc.
 */
class LAYBASIC_PUBLIC LayoutViewBase :
    public lay::Editables,
    public lay::Dispatcher
{
public:
  typedef lay::CellView::unspecific_cell_path_type cell_path_type;
  typedef lay::CellView::cell_index_type cell_index_type;

  /**
   *  @brief Define some options for the view
   */
  enum options_type { 
    LV_Normal = 0, 
    LV_NoLayers = 1, 
    LV_NoHierarchyPanel = 2, 
    LV_NoLibrariesView = 4,
    LV_NoEditorOptionsPanel = 8,
    LV_NoBookmarksView = 16,
    LV_Naked = 32,
    LV_NoZoom = 64,
    LV_NoGrid = 128,
    LV_NoMove = 256,
    LV_NoTracker = 512,
    LV_NoSelection = 1024,
    LV_NoPlugins = 2048,
    LV_NoPropertiesPopup = 4096,
    LV_NoServices = LV_NoMove + LV_NoTracker + LV_NoSelection + LV_NoPlugins
  };

  enum drop_small_cells_cond_type { DSC_Max = 0, DSC_Min = 1, DSC_Sum = 2 };

  /**
   *  @brief Stand-alone Constructor
   */
  LayoutViewBase (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  /** 
   *  @brief Destructor
   */
  ~LayoutViewBase ();

  /**
   *  @brief Copies settings from the source view
   */
  void copy_from (lay::LayoutViewBase *source);

  /**
   *  @brief Gets the current mode
   */
  int mode () const
  {
    return m_mode;
  }

  /**
   *  @brief Switches the application's mode
   *
   *  Switches the mode on application level. Use this method to initiate
   *  a mode switch from the view.
   */
  virtual void switch_mode (int m);

  /**
   *  @brief Gets the name of the active mode
   */
  std::string mode_name () const;

  /**
   *  @brief Switches the mode according to the given name
   *
   *  The name must be one of the names provided by "mode_names".
   */
  void switch_mode (const std::string &name);

  /**
   *  @brief Gets the names of the available modes
   */
  std::vector<std::string> mode_names () const;

  /**
   *  @brief Determine if there is something to copy
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for content providers in the tree views for example.
   */
  virtual bool has_selection ();

  /**
   *  @brief Pastes from clipboard
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for paste receivers in the tree views for example.
   */
  void paste ();

  /**
   *  @brief Pastes from clipboard and initiates a move
   */
  void paste_interactive (bool transient_mode = false);

  /**
   *  @brief Copies to clipboard
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for copy providers in the tree views for example.
   */
  virtual void copy ();

  /**
   *  @brief Copies to clipboard (view objects only)
   *
   *  This version does not look for copy sources in the tree views.
   */
  void copy_view_objects ();

  /**
   *  @brief Cuts to clipboard
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for cut & copy providers in the tree views for example.
   */
  virtual void cut ();

  /**
   *  @brief Gets the explicit title string of the view
   *
   *  This is the one explicitly set, not the one displayed. The displayed text is composed of internal information 
   *  if no title string is set.
   */
  const std::string &title_string () const
  {
    return m_title;
  }

  /**
   *  @brief Display a status message
   */
  virtual void message (const std::string &s = "", int timeout = 10);

  /**
   *  @brief The "dirty" flag indicates that one of the layout has been modified
   *
   *  A signal is provided on a change of this flag (dirty_changed).
   */
  bool is_dirty () const;

  /**
   *  @brief Returns true, if the layer source shall be shown always in the layer properties tree
   */
  virtual bool always_show_source () const;

  /**
   *  @brief Returns true, if the layer/datatype shall be shown always in the layer properties tree
   */
  virtual bool always_show_ld () const;

  /**
   *  @brief Returns true, if the layout index shall be shown always in the layer properties tree
   */
  virtual bool always_show_layout_index () const;

  /**
   *  @brief Fill the layer properties for a new layer
   *
   *  The layer's source must be set already to allow computing of the initial color.
   *  It is assumed that the layer is appended at the end of the layer list. This
   *  is important to set the dither pattern index accordingly.
   */
  void init_layer_properties (LayerProperties &props) const;

  /**
   *  @brief Create a set of new layers for the given layers of the given cellview
   */
  void add_new_layers (const std::vector <unsigned int> &layer_ids, int cv_index);

  /**
   *  @brief Set the layer properties of a layer with the given position (by iterator) for the current layer list
   *
   *  @param iter Points to the layer to be replaced
   *  @param props The properties to replace the current properties
   */
  void set_properties (const LayerPropertiesConstIterator &iter, const LayerProperties &props)
  {
    set_properties (current_layer_list (), iter, props);
  }

  /**
   *  @brief Sets a value indicating whether the given node is expanded in the layer tree
   *
   *  @param iter Points to the layer node to be modified
   *  @param ex True if the layer node shall be expanded, false if it shall be collapsed
   */
  void set_layer_node_expanded (const LayerPropertiesConstIterator &iter, bool ex)
  {
    set_layer_node_expanded (current_layer_list (), iter, ex);
  }

  /**
   *  @brief Set the layer properties of a layer with the given position (by iterator) for the layer list with the given index
   *
   *  @param index The layer list's index
   *  @param iter Points to the layer to be replaced
   *  @param props The properties to replace the current properties
   */
  void set_properties (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerProperties &props);

  /**
   *  @brief Sets a value indicating whether the given node is expanded in the layer tree
   *
   *  @param index The layer list's index
   *  @param iter Points to the layer node to be modified
   *  @param ex True if the layer node shall be expanded, false if it shall be collapsed
   */
  void set_layer_node_expanded (unsigned int index, const LayerPropertiesConstIterator &iter, bool ex);

  /**
   *  @brief Expand the layer properties of all tabs 
   *
   *  This method will replace the wildcard specifications in the layer properties of all 
   *  tabs. All unspecific cv index specifications will be expanded to all cellviews, all 
   *  layer and datatype specs will be expanded into all available (remaining) specs.
   */
  void expand_properties ();
  
  /**
   *  @brief Expand the layer properties for the given tab
   *
   *  @param index The index of the tab to which to apply the expansion to.
   */
  void expand_properties (unsigned int index);

  /**
   *  @brief Expand the layer properties of all tabs with some options
   *
   *  @param map_cv_index Maps a specified cv index to the one to use. Use -1 for the first entry to map any present cv index. Map to -1 to specify expansion to all available cv indices.
   *  @param add_default Set this parameter to true to implicitly add an entry for all "missing" layers.
   */
  void expand_properties (const std::map<int, int> &map_cv_index, bool add_default);

  /**
   *  @brief Expand the layer properties of the specified tab with some options
   *
   *  @param index The index of the tab to which to apply the expansion to.
   *  @param map_cv_index Maps a specified cv index to the one to use. Use -1 for the first entry to map any present cv index. Map to -1 to specify expansion to all available cv indices.
   *  @param add_default Set this parameter to true to implicitly add an entry for all "missing" layers.
   */
  void expand_properties (unsigned int index, const std::map<int, int> &map_cv_index, bool add_default);

  /**
   *  @brief Replace the layer node with a new one for the current layer list
   */
  void replace_layer_node (const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &props)
  {
    replace_layer_node (current_layer_list (), iter, props);
  }

  /**
   *  @brief Replace the layer node with a new one for the layer list with the given index
   */
  void replace_layer_node (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &props);

  /**
   *  @brief Insert the given layer properties node into the list for the current layer list
   *
   *  This method returns a reference to the element created.
   */
  const LayerPropertiesNode &insert_layer (const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &props)
  {
    return insert_layer (current_layer_list (), iter, props);
  }

  /**
   *  @brief Insert the given layer properties node into the list before the given index
   *
   *  This method returns a reference to the element created.
   */
  const LayerPropertiesNode &insert_layer (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &props);

  /**
   *  @brief Delete the layer properties node for the current layer list
   *  
   *  This method deletes the object that the iterator points to and invalidates
   *  the iterator since the object that the iterator points to is no longer valid.
   */
  void delete_layer (LayerPropertiesConstIterator &iter)
  {
    delete_layer (current_layer_list (), iter);
  }

  /**
   *  @brief Delete the layer properties node for the layer list with the given index
   *  
   *  This method deletes the object that the iterator points to and invalidates
   *  the iterator since the object that the iterator points to is no longer valid.
   */
  void delete_layer (unsigned int index, LayerPropertiesConstIterator &iter);

  /**
   *  @brief Begin iterator for the layers
   *
   *  This iterator delivers recursively the layers of this view
   */
  LayerPropertiesConstIterator begin_layers () const
  {
    return get_properties ().begin_const_recursive ();
  }

  /**
   *  @brief End iterator for the layers
   */
  LayerPropertiesConstIterator end_layers () const
  {
    return get_properties ().end_const_recursive ();
  }

  /**
   *  @brief Begin iterator for the layers for a given list
   *
   *  This iterator delivers recursively the layers of this view
   */
  LayerPropertiesConstIterator begin_layers (unsigned int index) const
  {
    return get_properties (index).begin_const_recursive ();
  }

  /**
   *  @brief End iterator for the layers for a given list
   */
  LayerPropertiesConstIterator end_layers (unsigned int index) const
  {
    return get_properties (index).end_const_recursive ();
  }

  /**
   *  @brief Rename a layer properties list
   */
  void rename_properties (unsigned int index, const std::string &new_name);

  /**
   *  @brief Replace the current layer properties list
   */
  void set_properties (const LayerPropertiesList &list)
  {
    set_properties (current_layer_list (), list);
  }

  /**
   *  @brief Replace the specified layer properties list
   */
  void set_properties (unsigned int index, const LayerPropertiesList &list);

  /**
   *  @brief Clear the given layer view list
   */
  void clear_layers (unsigned int index)
  {
    set_properties (index, LayerPropertiesList ());
  }

  /**
   *  @brief Clear the current layer view list
   */
  void clear_layers ()
  {
    set_properties (LayerPropertiesList ());
  }

  /**
   *  @brief Access the current layer properties list
   */
  const LayerPropertiesList &get_properties () const
  {
    return get_properties (current_layer_list ());
  }

  /**
   *  @brief Access the specified layer properties list 
   */
  const LayerPropertiesList &get_properties (unsigned int index) const;

  /**
   *  @brief Get the number of lists present in this view
   */
  unsigned int layer_lists () const
  {
    return (unsigned int) m_layer_properties_lists.size ();
  }

  /**
   *  @brief Get the index of the current properties list
   */
  unsigned int current_layer_list () const
  {
    return m_current_layer_list;
  }

  /**
   *  @brief Set the index of the current properties list
   */
  void set_current_layer_list (unsigned int index);

  /**
   *  @brief Delete the specified layer properties list
   */
  void delete_layer_list (unsigned int index);

  /**
   *  @brief Insert the layer properties list at the given index and make it the current list.
   */
  void insert_layer_list (unsigned int index, const LayerPropertiesList &props);

  /**
   *  @brief Insert the layer properties list at the given index and make it the current list.
   */
  void insert_layer_list (unsigned int index)
  {
    insert_layer_list (index, LayerPropertiesList ());
  }

  /**
   *  @brief Sets the currently active layer by layer properties and cell view index
   *
   *  This method will look up that layer in the layer view tree and select that layer.
   *  This method will also select this layer.
   *
   *  Returns false if the layer is not a valid one.
   */
  bool set_current_layer (unsigned int cv_index, const db::LayerProperties &properties);

  /**
   *  @brief Sets the currently active layer
   *
   *  The active layer is the one that is active in the layer
   *  browser panel. This method will also select this layer.
   */
  virtual void set_current_layer (const lay::LayerPropertiesConstIterator &l);

  /**
   *  @brief Retrieve the index of the currently active layer
   *
   *  The active layer is the one that is active in the layer
   *  browser panel.
   *  This method returns a null iterator, if no layer is active.
   */
  virtual lay::LayerPropertiesConstIterator current_layer () const;

  /**
   *  @brief Return the layers that are selected in the layer browser
   *
   *  Returns an empty list if no layer is selected.
   */
  virtual std::vector<lay::LayerPropertiesConstIterator> selected_layers () const;

  /**
   *  @brief Gets a pixmap representing the given layer
   *
   *  @param iter indicates the layer
   *  @param w The width in logical pixels of the generated pixmap (will be multiplied by dpr)
   *  @param h The height in logical pixels of the generated pixmap (will be multiplied by dpr)
   *  @param dpr The device pixel ratio (number of image pixes per logical pixel) - negative values mean auto-detect
   *  @param di_off The dither pattern offset (used for animation)
   *  @param no_state If true, the state will not be indicated
   */
  tl::PixelBuffer icon_for_layer (const lay::LayerPropertiesConstIterator &iter, unsigned int w, unsigned int h, double dpr = -1.0, unsigned int di_off = 0, bool no_state = false);

  /**
   *  @brief Sets the layers that are selected in the layer browser
   */
  virtual void set_selected_layers (const std::vector<lay::LayerPropertiesConstIterator> &sel);

  /**
   *  @brief Set the custom dither pattern 
   */
  void set_dither_pattern (const DitherPattern &pattern);

  /**
   *  @brief Obtain the custom dither pattern 
   */
  const DitherPattern &dither_pattern () const
  {
    return mp_canvas->dither_pattern ();
  }

  /**
   *  @brief Set the custom line styles
   */
  void set_line_styles (const LineStyles &styles);

  /**
   *  @brief Obtain the custom line styles
   */
  const LineStyles &line_styles () const
  {
    return mp_canvas->line_styles ();
  }

  /**
   *  @brief An event signalling the change in the number of hierarchy levels shown
   */
  tl::Event hier_levels_changed_event;

  /**
   *  @brief An event signalling a change in the annotations shape list
   *
   *  If annotation shapes are added or removed, this event is triggered.
   */
  tl::Event annotations_changed_event;

  /**
   *  @brief An event signalling a change in the hierarchy of the layouts
   *  
   *  If the hierarchy of a layout is changed, this event is triggered.
   *  This may happen due to the removal or insertion of cells or instances.
   */
  tl::Event hier_changed_event;

  /**
   *  @brief An event signalling a change in the geometries of the layouts
   *  
   *  If something on the geometries of a cell in one the layouts changes, this
   *  event is triggered. This may happed due to the removal or insertion of shapes or cell instances.
   *  In general, this indicates the need for redrawing of the layout for example.
   */
  tl::Event geom_changed_event;

  /**
   *  @brief An event triggered before something on a cellview is changed
   *
   *  This event is triggered before something on the cellview (i.e. the current cell of
   *  of set of the cellview) is changed. The argument is the index of the cell view to be
   *  changed.
   *  This event is followed by a corresponding cellview_changed event after the cellview
   *  has changed.
   */
  tl::event<int> cellview_about_to_change_event;

  /**
   *  @brief An event triggered before something on the cellviews is changed
   *
   *  This event is triggered before something on the cellviews is changed, i.e. a cellview is deleted or inserted.
   *  Each of these events is followed by a cellviews_changed event after the change has been made.
   */
  tl::Event cellviews_about_to_change_event;

  /**
   *  @brief An event signalling that the cell views have changed.
   *  
   *  When a cellview is added or removed, this event is triggered after the change has been made.
   *  The corresponding event that is triggered before the change is made is cellviews_about_to_change_event.
   */
  tl::Event cellviews_changed_event;

  /**
   *  @brief An event signalling a change in a cellview.
   *  
   *  If a cellview is changed (i.e. the cell is changed) this event is triggered.
   *  The integer argument will receive the index of the cellview that has changed.
   *  The corresponding event that is triggered before the change is made is cellview_about_to_change_event.
   */
  tl::event<int> cellview_changed_event;

  /**
   *  @brief A event signalling that one cellview has requested a new technology
   *
   *  This event is triggered if a cellview has requested a new technology.
   *  The argument is the index of the cellview that received the new technology.
   */
  tl::event<int> apply_technology_event;

  /**
   *  @brief An event signalling that a file has been loaded.
   *  
   *  If a new file is loaded, this event is triggered.
   */
  tl::Event file_open_event;

  /**
   *  @brief An event signalling that the viewport has changed.
   *  
   *  If the viewport (the rectangle that is shown) changes, this event
   *  is triggered.
   */
  tl::Event viewport_changed_event;

  /**
   *  @brief This event is triggered if the background color changed
   */
  tl::Event background_color_changed_event;

  /**
   *  @brief An event signalling that the layer list has changed.
   *  
   *  If the layer list changes, this event is triggered with an integer argument.
   *  The arguments's bit 0 is set, if the properties have changed. If the arguments bit 1 is
   *  set, the hierarchy has changed. If the name of layer properties is changed, bit 2 is
   *  set.
   */
  tl::event<int> layer_list_changed_event;

  /**
   *  @brief An event signalling that a layer list was deleted.
   *  
   *  If a layer list is deleted from the layer list set, the event is triggered with
   *  the index of the deleted list as an integer parameter.
   */
  tl::event<int> layer_list_deleted_event;

  /**
   *  @brief An event signalling that a layer list was inserted.
   *  
   *  If a layer list is inserted into the layer list set, the event is triggered with
   *  the index of the new list as an integer parameter.
   */
  tl::event<int> layer_list_inserted_event;

  /**
   *  @brief An event signalling that the current layer list has changed.
   *  
   *  If the current layer list is changed, this event is triggered.
   */
  tl::event<int> current_layer_list_changed_event;

  /**
   *  @brief An event signalling that the current layer has changed
   */
  tl::event<const lay::LayerPropertiesConstIterator &> current_layer_changed_event;


  /**
   *  @brief An event signalling that the visibility of some cells has changed
   */
  tl::Event cell_visibility_changed_event;

  /**
   *  @brief Save the given cellview into the given file (with options)
   *  If "update" is true, the cell view's properties will be updated (options, filename etc.).
   */
  void save_as (unsigned int index, const std::string &filename, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update, int keep_backups);

  /**
   *  @brief Implementation of the undo operations
   */
  virtual void undo (db::Op *op);

  /**
   *  @brief Implementation of the redo operations
   */
  virtual void redo (db::Op *op);

  /** 
   *  @brief Set the cellview at the given index
   *
   *  If cvindex is used as the cellview index to associate the 
   *  layout with. As a side effect, this method will emit a 
   *  title_changed signal, which means that the cellview passed
   *  should be correctly named before to reflect the correct
   *  title if no explicit title is set.
   */
  void set_layout (const lay::CellView &cv, unsigned int cvindex);

  /**
   *  @brief clear the cellviews
   */
  void clear_cellviews ();

  /**
   *  @brief erase one cellview with the given index
   */
  void erase_cellview (unsigned int index);

  /**
   *  @brief Save the layer properties 
   */
  void save_layer_props (const std::string &fn);

  /**
   *  @brief Load the layer properties 
   *
   *  @param fn The file to load.
   */
  void load_layer_props (const std::string &fn);

  /**
   *  @brief Load the layer properties 
   *
   *  @param fn The file to load
   *
   *  This version allows one to specify whether defaults should be used for all other layers by 
   *  setting add_default to true
   */
  void load_layer_props (const std::string &fn, bool add_default);

  /**
   *  @brief Load the layer properties 
   *
   *  @param fn The file to load
   *
   *  This version allows one to specify whether defaults should be used for all other layers by 
   *  setting add_default to true. In addition, this version will apply the .lyp definitions
   *  to a specific cellview after removing all definitions for this one. If cv_index is set
   *  to -1, the .lyp file will be applied to each cellview. In any case, the cv index specs
   *  in the .lyp file will be overridden.
   */
  void load_layer_props (const std::string &fn, int cv_index, bool add_default);

  /**
   *  @brief Determine whether a given layer properties file is a single-layout file
   *
   *  @return True, if the file contains definitions of a single layout only.
   */
  static bool is_single_cv_layer_properties_file (const std::string &fn);

  /**
   *  @brief Bookmark the current view under the given name
   */
  void bookmark_view (const std::string &name);

  /**
   *  @brief Obtain the bookmarks list
   */
  const BookmarkList &bookmarks () const
  {
    return m_bookmarks;
  }

  /**
   *  @brief Set the bookmarks list
   */
  void bookmarks (const BookmarkList &b);

  /**
   *  @brief Save the screen content to a file
   */
  void save_screenshot (const std::string &fn);

#if defined(HAVE_QT)
  /**
   *  @brief Get the screen content as a QImage object
   */
  QImage get_screenshot ();
#endif

  /**
   *  @brief Gets the screen content as a tl::PixelBuffer object
   */
  tl::PixelBuffer get_screenshot_pb ();

  /**
   *  @brief Save an image file with the given width and height
   */
  void save_image (const std::string &fn, unsigned int width, unsigned int height);

  /**
   *  @brief Save an image file with some options
   *
   *  @param fn The path of the file to write
   *  @param width The width of the image in pixels
   *  @param height The height of the image
   *  @param linewidth The width of a line in pixels (usually 1) or 0 for default
   *  @param oversampling The oversampling factor (1..3) or 0 for default
   *  @param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default
   *  @param background The background color or tl::Color() for default
   *  @param foreground The foreground color or tl::Color() for default
   *  @param active The active color or tl::Color() for default
   *  @param target_box The box to draw or db::DBox() for default
   *  @param monochrome If true, monochrome images will be produced
   */
  void save_image_with_options (const std::string &fn, unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, tl::Color background, tl::Color foreground, tl::Color active_color, const db::DBox &target_box, bool monochrome);

#if defined(HAVE_QT)
  /**
   *  @brief Get the screen content as a QImage object with the given width and height
   */
  QImage get_image (unsigned int width, unsigned int height);
#endif

  /**
   *  @brief Gets the screen content as a tl::PixelBuffer object
   */
  tl::PixelBuffer get_pixels (unsigned int width, unsigned int height);

#if defined(HAVE_QT)
  /**
   *  @brief Get the screen content as a QImage object with the given width and height
   *
   *  @param width The width of the image in pixels
   *  @param height The height of the image
   *  @param linewidth The width of a line in pixels (usually 1) or 0 for default
   *  @param oversampling The oversampling factor (1..3) or 0 for default
   *  @param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default
   *  @param background The background color or tl::Color() for default
   *  @param foreground The foreground color or tl::Color() for default
   *  @param active The active color or tl::Color() for default
   *  @param target_box The box to draw or db::DBox() for default
   *  @param monochrome If true, monochrome images will be produced
   */
  QImage get_image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, tl::Color background, tl::Color foreground, tl::Color active_color, const db::DBox &target_box, bool monochrome);
#endif

  /**
   *  @brief Get the screen content as a tl::PixelBuffer object with the given width and height
   *
   *  @param width The width of the image in pixels
   *  @param height The height of the image
   *  @param linewidth The width of a line in pixels (usually 1) or 0 for default
   *  @param oversampling The oversampling factor (1..3) or 0 for default
   *  @param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default
   *  @param background The background color or tl::Color() for default
   *  @param foreground The foreground color or tl::Color() for default
   *  @param active The active color or tl::Color() for default
   *  @param target_box The box to draw or db::DBox() for default
   */
  tl::PixelBuffer get_pixels_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, tl::Color background, tl::Color foreground, tl::Color active_color, const db::DBox &target_box);

  /**
   *  @brief Get the screen content as a monochrome tl::BitmapBuffer object with the given options
   *
   *  @param width The width of the image in pixels
   *  @param height The height of the image
   *  @param linewidth The width of a line in pixels (usually 1) or 0 for default
   *  @param oversampling The oversampling factor (1..3) or 0 for default
   *  @param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default
   *  @param background The background color or tl::Color() for default
   *  @param foreground The foreground color or tl::Color() for default
   *  @param active The active color or tl::Color() for default
   *  @param target_box The box to draw or db::DBox() for default
   *
   *  The colors will are converted to "on" pixels with a green channel value >= 50%.
   */
  tl::BitmapBuffer get_pixels_with_options_mono (unsigned int width, unsigned int height, int linewidth, tl::Color background, tl::Color foreground, tl::Color active_color, const db::DBox &target_box);

#if defined(HAVE_QT)
  /**
   *  @brief Gets the widget object that view is embedded in
   */
  virtual QWidget *widget ()
  {
    return 0;
  }
#endif

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_hier_levels (std::pair<int, int> l);

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_min_hier_levels (int l)
  {
    set_hier_levels (std::pair<int, int> (l, get_max_hier_levels ()));
  }

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_max_hier_levels (int l)
  {
    set_hier_levels (std::pair<int, int> (get_min_hier_levels (), l));
  }

  /**
   *  @brief Hierarchy level selection getter
   */
  std::pair<int, int> get_hier_levels () const;

  /**
   *  @brief Hierarchy level selection getter
   */
  int get_min_hier_levels () const
  {
    return get_hier_levels ().first;
  }

  /**
   *  @brief Hierarchy level selection getter
   */
  int get_max_hier_levels () const
  {
    return get_hier_levels ().second;
  }

  /**
   *  @brief Return true, if there is a "last" display state
   */
  bool has_prev_display_state ();

  /**
   *  @brief Return true, if there is a "next" display state
   */
  bool has_next_display_state ();

  /**
   *  @brief Cell box/label color setter
   */
  void cell_box_color (tl::Color c);

  /**
   *  @brief Cell box/label getter
   */
  tl::Color cell_box_color () const
  {
    return m_box_color;
  }

  /**
   *  @brief Transform label flag setter
   */
  void cell_box_text_transform (bool xform);

  /**
   *  @brief Transform label flag getter
   */
  bool cell_box_text_transform () const
  {
    return m_box_text_transform;
  }

  /**
   *  @brief Label font setter
   */
  void cell_box_text_font (unsigned int f);

  /**
   *  @brief Label font setter
   */
  unsigned int cell_box_text_font () const
  {
    return m_box_font;
  }

  /** 
   *  @brief Visibility of cell boxes
   */
  void cell_box_visible (bool vis);

  /** 
   *  @brief Visibility of cell boxes
   */
  bool cell_box_visible () const
  {
    return m_cell_box_visible;
  }

  /**
   *  @brief Min instance label size setter
   */
  void min_inst_label_size (int px);

  /**
   *  @brief Min instance label size getter
   */
  int min_inst_label_size () const
  {
    return m_min_size_for_label;
  }

  /** 
   *  @brief Visibility of text objects
   */
  void text_visible (bool vis);

  /** 
   *  @brief Visibility of text objects
   */
  bool text_visible () const
  {
    return m_text_visible;
  }

  /** 
   *  @brief Show properties 
   */
  void show_properties_as_text (bool sp);

  /** 
   *  @brief Show properties 
   */
  bool show_properties_as_text () 
  {
    return m_show_properties;
  }

  /** 
   *  @brief Enable or disable bitmap caching
   *
   *  Bitmap caching is used to optimize drawing by storing bitmaps
   *  in a cache for each cell. Repeated cells will be drawn faster then. 
   */
  void bitmap_caching (bool en);

  /** 
   *  @brief Lazy rendering of text objects
   */
  bool bitmap_caching () 
  {
    return m_bitmap_caching;
  }

  /** 
   *  @brief Lazy rendering of text objects
   */
  void text_lazy_rendering (bool lzy);

  /** 
   *  @brief Lazy rendering of text objects
   */
  bool text_lazy_rendering () 
  {
    return m_text_lazy_rendering;
  }

  /**
   *  @brief Text object font setter
   */
  void text_font (unsigned int f);

  /**
   *  @brief Text object font getter
   */
  unsigned int text_font () const
  {
    return m_text_font;
  }

  /**
   *  @brief The default text size property
   */
  void default_text_size (double fs);

  /**
   *  @brief The default text size property
   */
  double default_text_size () const
  {
    return m_default_text_size;
  }

  /**
   *  @brief Sets text point mode
   *
   *  In point mode, the text is treated like a point.
   *  Selection happens at the texts' origin.
   */
  void text_point_mode (bool pm);

  /**
   *  @brief Gets text point mode
   */
  bool text_point_mode () const
  {
    return m_text_point_mode;
  }

  /**
   *  @brief Show or hide markers
   */
  void show_markers (bool f);

  /**
   *  @brief "show_markers" property getter
   */
  bool show_markers () const
  {
    return m_show_markers;
  }

  /**
   *  @brief Don't show stipples
   */
  void no_stipples (bool f);
  
  /**
   *  @brief "Don't show stipples" property getter
   */
  bool no_stipples () const
  {
    return m_no_stipples;
  }
  
  /**
   *  @brief Offset stipples property
   */
  void offset_stipples (bool f);
  
  /**
   *  @brief Offset stipples property getter
   */
  bool offset_stipples () const
  {
    return m_stipple_offset;
  }
  
  /**
   *  @brief Apply text transformation property
   */
  void apply_text_trans (bool f);
  
  /**
   *  @brief Apply text transformation property
   */
  bool apply_text_trans () const
  {
    return m_apply_text_trans;
  }
  
  /**
   *  @brief Text object color
   */
  void text_color (tl::Color c);

  /**
   *  @brief Text object color
   */
  tl::Color text_color () const
  {
    return m_text_color;
  }

  /**
   *  @brief Clear all rulers if a new cell is selected (setter)
   *
   *  If this property is set to true, all rulers are cleared
   *  if the cell is changed.
   */
  void clear_ruler_new_cell (bool f);
  
  /**
   *  @brief Clear all rulers if a new cell is selected (getter)
   *
   *  If this property is set to true, all rulers are cleared
   *  if the cell is changed.
   */
  bool clear_ruler_new_cell () const
  {
    return m_clear_ruler_new_cell;
  }
  
  /**
   *  @brief Switch new cell to full hierarchy property
   *
   *  If this property is set to true, all hierarchy levels are selected
   *  if the cell is changed.
   */
  void full_hier_new_cell (bool f);
  
  /**
   *  @brief Switch new cell to full hierarchy property
   *
   *  If this property is set to true, all hierarchy levels are selected
   *  if the cell is changed.
   */
  bool full_hier_new_cell () const
  {
    return m_full_hier_new_cell;
  }
  
  /**
   *  @brief Fit new cell property
   *
   *  If this property is set to false, the coordinate window is not
   *  changed if the cell is changed.
   */
  void fit_new_cell (bool f);
  
  /**
   *  @brief Fit new cell
   *
   *  If this property is set to false, the coordinate window is not
   *  changed if the cell is changed.
   */
  bool fit_new_cell () const
  {
    return m_fit_new_cell;
  }
  
  /**
   *  @brief The pan distance
   *
   *  The pan distance is given relative to the current width and height.
   */
  void pan_distance (double d);
  
  /**
   *  @brief Gets the pan distance
   */
  double pan_distance () const;

  /**
   *  @brief Get the mouse wheel mode
   *
   *  The mouse wheel mode determines how the wheel behaves. Mode 0 is the 
   *  default mode, mode 1 is the alternative mode (default is up/down, 
   *  shift is left/right, ctrl is zoom).
   */
  int mouse_wheel_mode () const 
  { 
    return m_wheel_mode; 
  }

  /**
   *  @brief Set the mouse wheel mode
   *
   *  The mouse wheel mode determines how the wheel behaves. Mode 0 is the 
   *  default mode, mode 1 is the alternative mode (default is up/down, 
   *  shift is left/right, ctrl is zoom).
   */
  void mouse_wheel_mode (int m)
  { 
    m_wheel_mode = m; 
  }
  
  /**
   *  @brief Sets the color palette
   *
   *  The color palette is used for coloring new layers and is shown in the
   *  layer toolbox.
   */
  void set_palette (const lay::ColorPalette &);
  
  /**
   *  @brief Gets the color palette
   */
  const lay::ColorPalette &get_palette () const
  {
    return m_palette;
  }
  
  /**
   *  @brief Sets the stipple palette
   *
   *  The stipple palette is used for selecting stipples for new layers and is shown in the
   *  layer toolbox.
   */
  void set_palette (const lay::StipplePalette &);
  
  /**
   *  @brief Gets the stipple palette
   */
  const lay::StipplePalette &get_stipple_palette () const
  {
    return m_stipple_palette;
  }
  
  /**
   *  @brief Sets the line style palette
   *
   *  The line style palette is used for selecting line styles and is shown in the
   *  layer toolbox.
   */
  void set_palette (const lay::LineStylePalette &);

  /**
   *  @brief Gets the line style palette
   */
  const lay::LineStylePalette &get_line_style_palette () const
  {
    return m_line_style_palette;
  }

  /**
   *  @brief Reloads the given cellview into the current view
   *
   *  The cellview is given by index in the current view's cellview list.
   */
  void reload_layout (unsigned int cv_index);

  /** 
   *  @brief Load a (new) file into the layout
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  The new layout will use the default technology.
   *
   *  @return The index of the cellview loaded.
   */
  unsigned int load_layout (const std::string &filename, bool add_cellview)
  {
    return load_layout (filename, std::string (), add_cellview);
  }

  /** 
   *  @brief Load a (new) file into the layout associating it with the given technology
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  @return The index of the cellview loaded.
   */
  unsigned int load_layout (const std::string &filename, const std::string &technology, bool add_cellview);

  /** 
   *  @brief Load a (new) file into the layout with the options
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  The new layout will use the default technology.
   *
   *  @param options The options to use when loading.
   *  @param add_cellview_param See above.
   *  @return The index of the cellview loaded.
   */
  unsigned int load_layout (const std::string &filename, const db::LoadLayoutOptions &options, bool add_cellview)
  {
    return load_layout (filename, options, std::string (), add_cellview);
  }

  /**  
   *  @brief Load a (new) file into the layout with the options and using the specified technology
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  @param options The options to use when loading.
   *  @param add_cellview_param See above.
   *  @return The index of the cellview loaded.
   */
  unsigned int load_layout (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology, bool add_cellview);

  /** 
   *  @brief Create a new, empty layout 
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  The new layout will use the default technology.
   *
   *  @return The index of the cellview loaded.
   */
  unsigned int create_layout (bool add_cellview)
  {
    return create_layout (std::string (), add_cellview, true);
  }

  /** 
   *  @brief Create a new, empty layout using the specified technology
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before.
   *
   *  @return The index of the cellview created.
   */
  unsigned int create_layout (const std::string &technology, bool add_cellview)
  {
    return create_layout (technology, add_cellview, true);
  }

  /** 
   *  @brief Create a new, empty layout using the specified technology
   *
   *  The add_cellview param controls whether to create a new cellview
   *  or clear all cellviews before. This version allows one to specify whether layer properties shall be created.
   *
   *  @return The index of the cellview created.
   */
  unsigned int create_layout (const std::string &technology, bool add_cellview, bool initialize_layers);

  /**
   *  @brief Add an existing layout or replace the current cellview by the given layout
   *
   *  The add_cellview param controls whether to add the layout as a new cellview
   *  or clear all cellviews before.
   *
   *  The "initialize_layers" parameter allows one to specify whether the layer properties shall be initialized.
   *
   *  @return The index of the cellview loaded.
   */
  unsigned int add_layout (lay::LayoutHandle *layout_handle, bool add_cellview, bool initialize_layers = true);

  /**
   *  @brief Indicates the current position
   */ 
  virtual void current_pos (double x, double y);

  /**
   *  @brief Obtain the number of cellviews
   */
  unsigned int cellviews () const
  {
    return (unsigned int) m_cellviews.size ();
  }
  
  /**
   *  @brief Obtain the cellviews as a vector
   */
  const std::list<lay::CellView> &cellview_list () const
  {
    return m_cellviews;
  }
  
  /**
   *  @brief Obtain the cell view reference for an index
   *
   *  If the index is invalid, an empty cell view reference is returned
   */
  const CellView &cellview (unsigned int index) const;

  /**
   *  @brief Gets a cellview reference to the active cellview
   */
  lay::CellViewRef cellview_ref (unsigned int index);

  /**
   *  @brief Gets the index of the given cellview
   *  Based on the pointer passed to the function, the index of the respective cellview
   *  will be returned. If a null pointer is given or the pointer is not a valid cellview,
   *  -1 will be returned.
   */
  int index_of_cellview (const lay::CellView *cv) const;

  /**
   *  @brief Obtain the list of annotation shapes
   */
  const lay::AnnotationShapes &annotation_shapes () const
  {
    return m_annotation_shapes;
  }

  /**
   *  @brief Obtain the list of annotation shapes (non-const version)
   */
  lay::AnnotationShapes &annotation_shapes () 
  {
    return m_annotation_shapes;
  }

  /** 
   *  @brief Select the list of cellviews for this window and fit cell
   *
   *  Warning: use with care!
   */
  void select_cellviews_fit (const std::list <CellView> &cvs);

  /** 
   *  @brief Select the list of cellviews for this window
   *
   *  Warning: use with care!
   */
  void select_cellviews (const std::list <CellView> &cvs);

  /**
   *  @brief Configures the cellview with the given index
   *  @param index Index of the cellview to configure
   *  @param cv The cellview that provides the new configuration
   */
  void select_cellview (int index, const CellView &cv);

  /** 
   *  @brief Shift and scale the window
   */
  void shift_window (double f, double dx, double dy);

  /** 
   *  @brief Goto window
   *
   *  Position the window to the new position x and y with a size of s (approximately).
   *  If s is negative or zero, the windows is just shifted.
   */
  void goto_window (const db::DPoint &p, double s = -1.0);

  /** 
   *  @brief Return the displayed window
   */
  db::DBox box () const;

  /**
   *  @brief Create a new cell with the given in the given cellview
   *
   *  @param cv_index The index of the cellview where to create the cell
   *  @param cell_name The name of the cell (an exception is thrown if a cell of that name already exists)
   *  @return The index of the new cell
   */
  db::cell_index_type new_cell (int cv_index, const std::string &cell_name);
  
  std::pair <bool, std::string> redo_available ();
  std::pair <bool, std::string> undo_available ();

  /**
   *  @brief Select a certain mode (by index)
   */
  virtual void mode (int m);
  
  /**
   *  @brief Test, if the view is currently in move mode.
   */
  bool is_move_mode () const;

  /**
   *  @brief Test, if the view is currently in selection mode.
   */
  bool is_selection_mode () const;

  /**
   *  @brief Query the intrinsic mouse modes available
   */
  static unsigned int intrinsic_mouse_modes (std::vector<std::string> *descriptions);

  /**
   *  @brief Query the default mode
   */
  static int default_mode ();

  /**
   *  @brief Get a list of cellview index and transform variants 
   */
  std::set< std::pair<db::DCplxTrans, int> > cv_transform_variants () const;
  
  /**
   *  @brief Get the global transform variants for a given cellview index
   */
  std::vector<db::DCplxTrans> cv_transform_variants (int cv_index) const;

  /**
   *  @brief Get the global transform variants for a given cellview index and layer
   */
  std::vector<db::DCplxTrans> cv_transform_variants (int cv_index, unsigned int layer) const;

  /**
   *  @brief Get the transformation variants for a given cellview index ordered by layer
   */
  std::map<unsigned int, std::vector<db::DCplxTrans> > cv_transform_variants_by_layer (int cv_index) const;
  
  /**
   *  @brief Access to the hidden cell list
   */
  const std::vector <std::set <cell_index_type> > &hidden_cells () const
  { 
    return m_hidden_cells;
  }

  /**
   *  @brief Get the "dbu_coordinates" flag 
   *  
   *  If this flag is true, the property dialogs and other display functions should use 
   *  database units to display coordinates etc.
   */
  bool dbu_coordinates () const
  {
    return m_dbu_coordinates;
  }

  /**
   *  @brief Set the "dbu_coordinates" flag 
   *  
   *  If this flag is true, the property dialogs and other display functions should use 
   *  database units to display coordinates etc.
   */
  void dbu_coordinates (bool f); 

  /**
   *  @brief Get the "absolute_coordinates" flag 
   *  
   *  If this flag is true, the property dialogs and other display functions should use 
   *  absolute (on top level) coordinates for points etc.
   *  "absolute" may as well refer to orientation, not only to coordinates, 
   *  if transformations are considered.
   */
  bool absolute_coordinates () const
  {
    return m_absolute_coordinates;
  }

  /**
   *  @brief Set the "absolute_coordinates" flag 
   *  
   *  If this flag is true, the property dialogs and other display functions should use 
   *  absolute (on top level) coordinates for points etc.
   *  "absolute" may as well refer to orientation, not only to coordinates, 
   *  if transformations are considered.
   */
  void absolute_coordinates (bool f);

  /**
   *  @brief Gets the canvas object (where the layout is drawn and view objects are placed)
   */
  lay::LayoutCanvas *canvas ()
  {
    return mp_canvas;
  }

  /**
   *  @brief Gets the canvas object (const version)
   */
  const lay::LayoutCanvas *canvas () const
  {
    return mp_canvas;
  }

#if defined(HAVE_QT)
  /**
   *  @brief Gets the layer control panel
   */
  virtual lay::LayerControlPanel *control_panel ()
  {
    return 0;
  }

  /**
   *  @brief Gets the hierarchy panel
   */
  virtual lay::HierarchyControlPanel *hierarchy_panel ()
  {
    return 0;
  }

  /**
   *  @brief Gets the editor options page
   */
  virtual lay::EditorOptionsPages *editor_options_pages ()
  {
    return 0;
  }
#endif

  /**
   *  @brief Get the current viewport 
   */
  const lay::Viewport &viewport () const
  {
    return mp_canvas->viewport ();
  }

  /**
   *  @brief Resizes the view to the given width and height in pixels
   */
  void resize (unsigned int width, unsigned int height);

  /**
   *  @brief Background color property
   */
  tl::Color background_color () const
  {
    return mp_canvas->background_color ();
  }

  /**
   *  @brief Foreground color property
   */
  tl::Color foreground_color () const
  {
    return mp_canvas->foreground_color ();
  }

  /**
   *  @brief Active color property
   */
  tl::Color active_color () const
  {
    return mp_canvas->active_color ();
  }

  /**
   *  @brief Write accessor to the "drop small cells" flag
   */
  void drop_small_cells (bool m);

  /**
   *  @brief Read accessor to the "drop small cells" flag
   */
  bool drop_small_cells () const
  {
    return m_drop_small_cells;
  }

  /**
   *  @brief Write accessor to the "drop small cells" value
   */
  void drop_small_cells_value (unsigned int s);

  /**
   *  @brief Read accessor to the "drop small cells" value
   */
  unsigned int drop_small_cells_value () const
  {
    return m_drop_small_cells_value;
  }

  /**
   *  @brief Write accessor to the "drop small cells" condition 
   */
  void drop_small_cells_cond (drop_small_cells_cond_type t);

  /**
   *  @brief Read accessor to the "drop small cells" condition 
   */
  drop_small_cells_cond_type drop_small_cells_cond () const
  {
    return m_drop_small_cells_cond;
  }

  /**
   *  @brief Gets a value indicating whether guiding shapes are visible
   */
  bool guiding_shapes_visible () const
  {
    return m_guiding_shape_visible;
  }

  /**
   *  @brief Sets a value indicating whether guiding shapes are visible
   */
  void guiding_shapes_visible (bool v);

  /**
   *  @brief Gets the guiding shapes color
   */
  tl::Color guiding_shapes_color () const
  {
    return m_guiding_shape_color;
  }

  /**
   *  @brief Sets the guiding shapes color
   */
  void guiding_shapes_color (tl::Color c);

  /**
   *  @brief Gets the guiding shapes line width
   */
  int guiding_shapes_line_width () const
  {
    return m_guiding_shape_line_width;
  }

  /**
   *  @brief Sets the guiding shapes line width
   */
  void guiding_shapes_line_width (int lw);

  /**
   *  @brief Gets the guiding shapes vertex size
   */
  int guiding_shapes_vertex_size () const
  {
    return m_guiding_shape_vertex_size;
  }

  /**
   *  @brief Sets the guiding shapes vertex size
   */
  void guiding_shapes_vertex_size (int lw);

  /**
   *  @brief Read accessor to the "abstract mode width" property.
   *
   *  The abstract mode width enables abstract mode (for values > 0) and
   *  specifies the border to search for touching shapes.
   */
  double abstract_mode_width () const
  {
    return m_abstract_mode_enabled ? m_abstract_mode_width : 0.0;
  }

  /**
   *  @brief Read accessor to the "child context enabled" property.
   *
   *  If child context is enabled, a third set of bit planes must be created by the redraw thread
   *  containing the shapes for the below-top-level objects.
   */
  bool child_context_enabled () const
  {
    return m_child_ctx_enabled;
  }

  /**
   *  @brief Write accessor to the "draw array border instances" flag
   */
  void draw_array_border_instances (bool m);

  /**
   *  @brief Read accessor to the "draw array border instances" flag
   */
  bool draw_array_border_instances () const
  {
    return m_draw_array_border_instances;
  }

  /**
   *  @brief Get the Drawings interface
   */
  lay::Drawings *drawings () 
  {
    return mp_canvas;
  }

  /**
   *  @brief Select synchronous mode or deselect it
   */
  void set_synchronous (bool sync_mode);

  /**
   *  @brief Tell the state of synchronous mode
   */
  bool synchronous () const
  {
    return m_synchronous;
  }

  /**
   *  @brief Set the number of drawing workers (if not synchronous)
   */
  void set_drawing_workers (int workers);

  /**
   *  @brief Get the number of drawing workers 
   */
  int drawing_workers () const
  {
    return m_drawing_workers;
  }

  /**
   *  @brief Gets a value indicating whether the view will accept a dropped file with the given URL or path
   */
  virtual bool accepts_drop (const std::string &path_or_url) const;

  /**
   *  @brief Gets called when a file or URL is dropped on the view
   */
  virtual void drop_url (const std::string &path_or_url);

  /**
   *  @brief Returns true if the layer control panels model got updated
   *  Internally used by CellTreeModel to synchronize
   */
  virtual bool layer_model_updated ()
  {
    return false;
  }

  /**
   *  @brief Gets a list of all plugins
   */
  const std::vector<lay::Plugin *> &plugins ()
  {
    return mp_plugins;
  }

  /**
   *  @brief Localize a plugin by name
   *
   *  This method will return 0, if no such plugin is registered
   */
  Plugin *get_plugin_by_name (const std::string &name) const;

  /** 
   *  @brief Localize the plugin of the given Type
   *
   *  This method will return 0, if no such plugin is registered
   */
  template <class PI>
  PI *get_plugin () const
  {
    PI *pi = 0;
    for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end () && !pi; ++p) {
      pi = dynamic_cast<PI *> (*p);
    }
    return pi;
  }

  /** 
   *  @brief Localize the plugins of the given Type
   *
   *  This method will return 0, if no such plugin is registered
   */
  template <class PI>
  std::vector<PI *> get_plugins () const
  {
    std::vector<PI *> pi;
    for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
      if (dynamic_cast<PI *> (*p) != 0) {
        pi.push_back (dynamic_cast<PI *> (*p));
      }
    }
    return pi;
  }

  /** 
   *  @brief Create a plugin of the given type
   *
   *  This method can be used to selectively create plugins when the NoPlugin option
   *  is used. If no such plugin is registered, no plugin is created.
   *  If a plugin with that name is already registered, it is not created again.
   *  PD is the type of the declaration.
   */
  template <class PD>
  void create_plugin ()
  {
    for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
      if (dynamic_cast <const PD *> ((*p)->plugin_declaration ()) != 0) {
        return;
      }
    }
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      if (dynamic_cast <const PD *> (&*cls) != 0) {
        create_plugin (&*cls);
        break;
      }
    }
  }


  /**
   *  @brief Enable or disable the actions for edit functions 
   * 
   *  This method is used by non-modal dialogs that want to suppress any editing
   *  activities (like browsers) while they are open.
   *
   *  This method can be called multiple times and will count the enable/disable
   *  transitions. Each disable will cancel on enable call.
   */
  void enable_edits (bool enable);

  /**
   *  @brief Gets a value indicating whether edits are enabled or not
   *
   *  "enable_edits" will change this value.
   */
  bool edits_enabled () const
  {
    return m_disabled_edits <= 0;
  }

  /**
   *  @brief Rename the given cellview to the given name
   *
   *  The name of the cell view is shown in the title
   */
  void rename_cellview (const std::string &name, int cellview_index);

  /** 
   *  @brief Descend into the hierarchy along the given specific path for the given cellview
   */
  void descend (const std::vector<db::InstElement> &path, int cellview_index);

  /** 
   *  @brief Ascend one level in the hierarchy for the given cellview
   *
   *  @return The instance element removed by ascending the path
   */
  db::InstElement ascend (int cellview_index);

  /**
   *  @brief Get the index of the active cellview (shown in hierarchy browser)
   */
  const lay::CellView &active_cellview () const;

  /**
   *  @brief Gets a cellview reference to the active cellview
   */
  lay::CellViewRef active_cellview_ref ();

  /**
   *  @brief Get the index of the active cellview (shown in hierarchy browser)
   */
  virtual int active_cellview_index () const;

  /**
   *  @brief Select a certain cellview for the active one
   */
  virtual void set_active_cellview_index (int index);

  /**
   *  @brief An event triggered if the active cellview changes
   *  This event is triggered after the active cellview changed.
   */
  tl::Event active_cellview_changed_event;

  /**
   *  @brief An event triggered if the active cellview changes
   *  This event is triggered after the active cellview changed. The integer parameter is the index of the
   *  new cellview.
   */
  tl::event<int> active_cellview_changed_with_index_event;

  /**
   *  @brief Cell paths of the selected cells
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  current path is returned for the cellview given by cv_index.
   */
  virtual void selected_cells_paths (int cv_index, std::vector<cell_path_type> &paths) const;

  /**
   *  @brief Cell path of the current cell
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  current path is returned for the cellview given by cv_index.
   */
  virtual void current_cell_path (int cv_index, cell_path_type &path) const;

  /**
   *  @brief Cell path of the current cell
   *
   *  This method version is provided for automation purposes mainly.
   */
  cell_path_type get_current_cell_path (int cv_index) const
  {
    cell_path_type r;
    current_cell_path (cv_index, r);
    return r;
  }

  /**
   *  @brief Cell path of the current cell in the active cellview
   *
   *  This is a convenience function returning the path for the active cellview.
   */
  void current_cell_path (cell_path_type &path) const
  {
    current_cell_path (active_cellview_index (), path);
  }

  /**
   *  @brief Set the path to the current cell
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  cell given by the path is highlighted and scrolled into view.
   */
  virtual void set_current_cell_path (int cv_index, const cell_path_type &path);

  /**
   *  @brief Set the path to the current cell is the current cellview
   *
   *  This is a convenience function setting the path for the active cellview.
   */
  void set_current_cell_path (const cell_path_type &path)
  {
    set_current_cell_path (active_cellview_index (), path);
  }

  /**
   *  @brief Select a cell by path for a certain cell view and fit cell
   */
  void select_cell_fit (const cell_path_type &path, int cellview_index);

  /** 
   *  @brief Select a cell by index for a certain cell view and fit cell
   */
  void select_cell_fit (cell_index_type index, int cellview_index);

  /** 
   *  @brief Select a cell by path for a certain cell view
   */
  void select_cell (const cell_path_type &path, int cellview_index);

  /** 
   *  @brief Select a cell by index for a certain cell view
   */
  void select_cell (cell_index_type index, int cellview_index);

  /**
   *  @brief Check if a cell is hidden
   */
  bool is_cell_hidden (cell_index_type ci, int cellview_index) const;

  /**
   *  @brief Get the hidden cells for a certain cellview 
   */
  const std::set<cell_index_type> &hidden_cells (int cellview_index) const;

  /**
   *  @brief Add a cell to the hidden cell list
   */
  void hide_cell (cell_index_type ci, int cellview_index);

  /**
   *  @brief Remove a cell from the hidden cell list
   */
  void show_cell (cell_index_type ci, int cellview_index);

  /**
   *  @brief Clears the hidden cell list
   */
  void show_all_cells ();

  /**
   *  @brief Clears the hidden cell list for the given cellview
   */
  void show_all_cells (int cv_index);

  /**
   *  @brief Update the layout view to the current state
   *
   *  This method must be called in order to trigger the update of
   *  the contents. It will be known, what needs to be updated.
   */
  void update_content ();

  /**
   *  @brief Force an unconditional update
   *
   *  This method is supposed to be used to ensure everything is updated.
   *  TODO: this method should become obsolete once there is a consistent
   *  signalling of states implemented.
   */
  void force_update_content ();

  /**
   *  @brief Create a set of initial layer properties for the given cellview 
   *
   *  @param cv_index The cellview for which to produce a set of layer properties
   *  @param lyp_file The layer properties file to load or empty if no file should be loaded
   *  @param add_missing True, if missing layers should be added (ignored if lyp_file is empty)
   */
  void create_initial_layer_props (int cv_index, const std::string &lyp_file, bool add_missing);

  /**
   *  @brief Merges the given properties into the cell properties of this view 
   */
  void merge_layer_props (const std::vector<lay::LayerPropertiesList> &props);

  /**
   *  @brief Get the "select inside PCells" selection mode
   *
   *  @return true, objects inside PCells can be selected
   */
  bool select_inside_pcells_mode () const
  {
    return m_sel_inside_pcells;
  }

  /**
   *  @brief Get the transient selection mode 
   *
   *  @return true, if transient (hover) selection mode is enabled
   */
  bool transient_selection_mode () const
  {
    return m_transient_selection_mode;
  }

  /**
   *  @brief Get the default color for markers
   */
  tl::Color default_marker_color () const
  {
    return m_marker_color;
  }

  /**
   *  @brief Get the default line width for markers
   */
  int default_marker_line_width () const
  {
    return m_marker_line_width;
  }

  /**
   *  @brief Get the default marker dither pattern index
   */
  int default_dither_pattern () const
  {
    return m_marker_dither_pattern;
  }

  /**
   *  @brief Get the default marker line style index
   */
  int default_line_style () const
  {
    return m_marker_line_style;
  }

  /**
   *  @brief Get the default vertex size for markers
   */
  int default_marker_vertex_size () const
  {
    return m_marker_vertex_size;
  }

  /**
   *  @brief Get the default halo flag for markers
   */
  int default_marker_halo () const
  {
    return m_marker_halo;
  }

  /**
   *  @brief Gets the "search range" in pixels (for single click)
   *  The search range applies whenever some object is looked up in the vicinity of the
   *  mouse cursor. This value gives the search range in pixels.
   */
  unsigned int search_range ();

  /**
   *  @brief Sets the "search range" in pixels (for single click)
   */
  void set_search_range (unsigned int sr);

  /**
   *  @brief Gets the "search range" in pixels (for box)
   *  The search range applies whenever some object is looked up in the vicinity of the
   *  mouse cursor. This value gives the search range in pixels.
   */
  unsigned int search_range_box ();

  /**
   *  @brief Sets the "search range" in pixels (for box)
   */
  void set_search_range_box (unsigned int sr);

  /**
   *  @brief Return true, if any cellview is editable
   */
  bool is_editable () const;

  /**
   *  @brief Get the view_op's for rendering the layers
   */
  const std::vector <lay::ViewOp> &get_view_ops () const
  {
    return mp_canvas->get_view_ops ();
  }

  /**
   *  @brief Get the redraw layer info vector 
   */
  const std::vector <lay::RedrawLayerInfo> &get_redraw_layers () const
  {
    return mp_canvas->get_redraw_layers ();
  }

  /**
   *  @brief Add missing layers
   */
  void add_missing_layers ();

  /**
   *  @brief Remove unused layers
   */
  void remove_unused_layers ();

  /**
   *  @brief Add layers which are not part of the LayerState
   */
  void add_new_layers (const LayerState &snapshot);

  /**
   *  @brief Gets a snapshot of the current layers (can be used for add_missing_layers later)
   */
  LayerState layer_snapshot () const;

  /**
   *  @brief Add a marker database
   *
   *  The layout view will become owner of the database.
   *  
   *  @param rdb The database to add
   *  @return The index of the database
   */
  unsigned int add_rdb (rdb::Database *rdb);

  /**
   *  @brief Replaces a marker database
   *
   *  The layout view will become owner of the database.
   *  If the index is not valid, the database will be added and the new index will be returned.
   *
   *  @param db_index The index of the database to replace
   *  @param rdb The database to add
   */
  unsigned int replace_rdb (unsigned int db_index, rdb::Database *rdb);

  /**
   *  @brief Get the marker database by index
   *
   *  @param index The index of the database
   *  @return A pointer to the database or 0 if the index was not valid.
   */
  rdb::Database *get_rdb (int index);

  /**
   *  @brief Get the marker database by index (const version)
   *
   *  @param index The index of the database
   *  @return A pointer to the database or 0 if the index was not valid.
   */
  const rdb::Database *get_rdb (int index) const;

  /**
   *  @brief Remove the marker database with the given index
   *
   *  This will release the marker database at the given index. The list 
   *  will be reduced by that element. This means, that the following elements
   *  will have different indicies.
   */
  void remove_rdb (unsigned int index);

  /**
   *  @brief Get the number of marker databases
   */
  unsigned int num_rdbs () const
  {
    return (unsigned int) m_rdbs.size ();
  }

  /**
   *  @brief Open the RDB browser for a given database and associated cv index
   */
  virtual void open_rdb_browser (int /*rdb_index*/, int /*cv_index*/) { }

  /**
   *  @brief An event signalling a change in the marker database list
   *  
   *  If marker databases are added or removed, this event is triggered.
   */
  tl::Event rdb_list_changed_event;

  /**
   *  @brief Add a Netlist database
   *
   *  The layout view will become owner of the database.
   *
   *  @param l2ndb The database to add
   *  @return The index of the database
   */
  unsigned int add_l2ndb (db::LayoutToNetlist *l2ndb);

  /**
   *  @brief Replaces a Netlist database
   *
   *  The layout view will become owner of the database.
   *  If the index is not valid, the database will be added and the new index will be returned.
   *
   *  @param db_index The index of the database to replace
   *  @param l2ndb The database to add
   */
  unsigned int replace_l2ndb (unsigned int db_index, db::LayoutToNetlist *l2ndb);

  /**
   *  @brief Get the netlist database by index
   *
   *  @param index The index of the database
   *  @return A pointer to the database or 0 if the index was not valid.
   */
  db::LayoutToNetlist *get_l2ndb (int index);

  /**
   *  @brief Get the netlist database by index (const version)
   *
   *  @param index The index of the database
   *  @return A pointer to the database or 0 if the index was not valid.
   */
  const db::LayoutToNetlist *get_l2ndb (int index) const;

  /**
   *  @brief Remove the netlist database with the given index
   *
   *  This will release the netlist database at the given index. The list
   *  will be reduced by that element. This means, that the following elements
   *  will have different indicies.
   */
  void remove_l2ndb (unsigned int index);

  /**
   *  @brief Get the number of netlist databases
   */
  unsigned int num_l2ndbs () const
  {
    return (unsigned int) m_l2ndbs.size ();
  }

  /**
   *  @brief Open the L2NDB browser for a given database and associated cv index
   */
  virtual void open_l2ndb_browser (int /*l2ndb_index*/, int /*cv_index*/) { }

  /**
   *  @brief An event signalling a change in the netlist database list
   *
   *  If netlist databases are added or removed, this event is triggered.
   */
  tl::Event l2ndb_list_changed_event;

  /**
   *  @brief Ensure the given box is visible
   */
  void ensure_visible (const db::DBox &b);

  /**
   *  @brief Specify the global transformation
   */
  void set_global_trans (const db::DCplxTrans &b);

  /**
   *  @brief Gets the global transformation
   */
  const db::DCplxTrans &global_trans () const
  {
    return mp_canvas->global_trans ();
  }

  /**
   *  @brief Gets the window title of the view
   */
  std::string title () const;

  /**
   *  @brief Sets the window title to an explicit string
   */
  void set_title (const std::string &t);

  /**
   *  @brief Resets the explicit title and enable the automatic naming
   */
  void reset_title ();

  /**
   *  @brief Removes the previous state from the stack
   */
  void pop_state ();

  /**
   *  @brief Clears the state stack
   */
  void clear_states ();

  /**
   *  @brief Zoom the given box into view
   */
  void zoom_box (const db::DBox &b);

  /**
   *  @brief Zoom the given box into view and select hierarchy levels
   */
  void zoom_box_and_set_hier_levels (const db::DBox &b, const std::pair<int, int> &levels);

  /**
   *  @brief Specify the transformation explicitly
   */
  void zoom_trans (const db::DCplxTrans &b);

  /**
   *  @brief Move the viewport such that the given point is at the center of the viewport
   */
  void pan_center (const db::DPoint &p);

  /**
   *  @brief Goto a position/cell view that was saved with save_view 
   */
  void goto_view (const DisplayState &state);

  /**
   *  @brief Save a position/cell view that can be used for "goto_view"
   */
  void save_view (DisplayState &state) const;

  //  access to the basic services
  lay::MouseTracker *mouse_tracker () const { return mp_tracker; }
  lay::ZoomService *zoom_service () const { return mp_zoom_service; }
  lay::SelectionService *selection_service () const { return mp_selection_service; }
  lay::MoveService *move_service () const { return mp_move_service; }

  /**
   *  @brief Gets the full field box
   *
   *  This is the box to which the view will zoom on zoom_fit().
   *  This box is supposed to cover everything inside the view.
   */
  db::DBox full_box () const;

  /**
   *  @brief Gets called when a menu item is activated
   */
  void menu_activated (const std::string &symbol);

  /**
   *  @brief Gets all available menu symbols
   */
  static std::vector<std::string> menu_symbols ();

  /**
   *  @brief Store the current state on the "previous states" stack
   */
  void store_state ();

  /**
   *  @brief Cancels all edit operations, clears the selection and resets the mode to "Select"
   */
  void cancel_esc ();

  /**
   *  @brief Cancels all edit operations and clears the selection
   */
  void cancel ();

  /**
   *  @brief Cancels all edit operations but maintains selection
   */
  void cancel_edits ();

  /**
   *  @brief Select all levels of hierarchy available
   */
  void max_hier ();

  /**
   *  @brief Stop activities like shape browser and redrawing thread
   */
  void stop ();

  /**
   *  @brief Stop redrawing thread
   */
  void stop_redraw ();

  /** 
   *  @brief Select last display state
   */
  void prev_display_state ();

  /** 
   *  @brief Select next display state
   */
  void next_display_state ();

  /**
   *  @brief Ensure the selection is visible 
   */
  void ensure_selection_visible ();

  /** 
   *  @brief Select a cell by index for a certain cell view
   *
   *  This will be forwarded to select_cell or select_cell_fit depending
   *  on the m_fit_new_cell flag.
   */
  void select_cell_dispatch (const cell_path_type &path, int cellview_index);

  /**
   *  @brief Called when the current layer changed
   */
  void current_layer_changed_slot (const lay::LayerPropertiesConstIterator &iter);

  //  zoom slots
  void zoom_fit ();
  void zoom_fit_sel ();
  void zoom_in ();
  void zoom_out ();
  void pan_left ();
  void pan_up ();
  void pan_right ();
  void pan_down ();
  void pan_left_fast ();
  void pan_up_fast ();
  void pan_right_fast ();
  void pan_down_fast ();

  //  called by children and owner
  void redraw ();
  void redraw_later ();
  void redraw_layer (unsigned int index);
  void redraw_deco_layer ();
  void redraw_cell_boxes ();
  void timer ();

  virtual void deactivate_all_browsers ();

  /**
   *  @brief Gets the LayoutView interface
   */
  LayoutView *ui ()
  {
    return get_ui ();
  }

  /**
   *  @brief Gets the LayoutView interface (const version)
   */
  const LayoutView *ui () const
  {
    return const_cast<LayoutViewBase *> (this)->get_ui ();
  }

  /**
   *  @brief Unregisters the given plugin
   */
  void unregister_plugin (lay::Plugin *pi);

  /**
   *  @brief Gets the options the view was created with
   */
  unsigned int options () const
  {
    return m_options;
  }

private:
  //  event handlers used to connect to the layout object's events
  void signal_hier_changed ();
  void signal_bboxes_from_layer_changed (unsigned int cv_index, unsigned int layer_index);
  void signal_bboxes_changed ();
  void signal_prop_ids_changed ();
  void signal_layer_properties_changed ();
  void signal_cell_name_changed ();
  void signal_annotations_changed ();
  void signal_plugin_enabled_changed ();
  void signal_apply_technology (lay::LayoutHandle *layout_handle);

private:
  lay::LayoutView *mp_ui;
  tl::DeferredMethod<lay::LayoutViewBase> dm_redraw;
  bool m_editable;
  int m_disabled_edits;
  unsigned int m_options;
  lay::LayoutCanvas *mp_canvas;
  std::list <CellView> m_cellviews;
  lay::AnnotationShapes m_annotation_shapes;
  std::vector <std::set <cell_index_type> > m_hidden_cells;
  std::string m_title;
  tl::vector <rdb::Database *> m_rdbs;
  tl::vector <db::LayoutToNetlist *> m_l2ndbs;
  std::string m_def_lyp_file;
  bool m_add_other_layers;
  bool m_synchronous;
  int m_drawing_workers;

  int m_from_level, m_to_level;
  double m_pan_distance;
  int m_paste_display_mode;
  int m_wheel_mode;
  bool m_guiding_shape_visible;
  tl::Color m_guiding_shape_color;
  int m_guiding_shape_line_width;
  int m_guiding_shape_vertex_size;

  tl::Color m_ctx_color;
  int m_ctx_dimming;
  bool m_ctx_hollow;

  tl::Color m_child_ctx_color;
  int m_child_ctx_dimming;
  bool m_child_ctx_hollow;
  bool m_child_ctx_enabled;

  double m_abstract_mode_width;
  bool m_abstract_mode_enabled;

  tl::Color m_box_color;
  bool m_box_text_transform;
  unsigned int m_box_font;
  int m_min_size_for_label;
  bool m_cell_box_visible;

  tl::Color m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_dither_pattern;
  int m_marker_line_style;
  bool m_marker_halo;

  unsigned int m_search_range;
  unsigned int m_search_range_box;

  bool m_transient_selection_mode;
  bool m_sel_inside_pcells;

  int m_default_font_size;
  bool m_text_visible;
  bool m_text_lazy_rendering;
  bool m_bitmap_caching;
  bool m_show_properties;
  tl::Color m_text_color;
  bool m_apply_text_trans;
  double m_default_text_size;
  bool m_text_point_mode;
  unsigned int m_text_font;
  bool m_show_markers;
  bool m_no_stipples;
  bool m_stipple_offset;

  bool m_drop_small_cells;
  unsigned int m_drop_small_cells_value;
  drop_small_cells_cond_type m_drop_small_cells_cond;

  bool m_draw_array_border_instances;

  bool m_fit_new_cell;
  bool m_full_hier_new_cell;
  bool m_clear_ruler_new_cell;
  bool m_dbu_coordinates;
  bool m_absolute_coordinates;

  bool m_dirty;
  bool m_prop_changed;
  bool m_animated;
  unsigned int m_phase;

  lay::ColorPalette m_palette;
  lay::StipplePalette m_stipple_palette;
  lay::LineStylePalette m_line_style_palette;

  std::vector <DisplayState> m_display_states;
  unsigned int m_display_state_ptr;

  BookmarkList m_bookmarks;

  std::vector<lay::LayerPropertiesList *> m_layer_properties_lists;
  unsigned int m_current_layer_list;

  //  service and editable management
  int m_mode;
  
  //  services & editables
  lay::MouseTracker *mp_tracker;
  lay::ZoomService *mp_zoom_service;
  lay::SelectionService *mp_selection_service;
  lay::MoveService *mp_move_service;

  std::vector<lay::Plugin *> mp_plugins;

  lay::Plugin *mp_active_plugin;

  int m_active_cellview_index;
  bool m_active_cellview_changed_event_enabled;
  std::set<int> m_active_cellview_changed_events;

  lay::LayerPropertiesConstIterator m_current_layer;
  std::vector<lay::LayerPropertiesConstIterator> m_selected_layers;

  std::vector<cell_path_type> m_current_cell_per_cellview;

  bool m_visibility_changed;

  tl::Clock m_clock, m_last_checked;

  void do_prop_changed ();
  void do_redraw (int layer);
  void do_redraw ();

  void set_view_ops ();
  void background_color (tl::Color c);
  void ctx_color (tl::Color c);
  void ctx_dimming (int percent);
  void ctx_hollow (bool h);
  void child_ctx_color (tl::Color c);
  void child_ctx_dimming (int percent);
  void child_ctx_hollow (bool h);
  void child_ctx_enabled (bool e);
  void abstract_mode_width (double w);
  void abstract_mode_enabled (bool e);
  bool has_max_hier () const;
  int max_hier_level () const;

  void zoom_by (double f);

  void update_event_handlers ();
  void viewport_changed ();
  void cellview_changed (unsigned int index);

  void do_load_layer_props (const std::string &fn, bool map_cv, int cv_index, bool add_default);
  void finish_cellviews_changed ();
  void init_layer_properties (LayerProperties &props, const LayerPropertiesList &lp_list) const;
  void merge_dither_pattern (lay::LayerPropertiesList &props);

  void refresh ();

protected:
  /**
   *  @brief Constructor for calling from a LayoutView
   */
  LayoutViewBase (lay::LayoutView *ui, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  void init (db::Manager *mgr);

  lay::Plugin *active_plugin () const
  {
    return mp_active_plugin;
  }

  virtual LayoutView *get_ui ();

  bool configure (const std::string &name, const std::string &value);
  void config_finalize ();

  std::list<lay::CellView>::iterator cellview_iter (int cv_index);
  std::list<lay::CellView>::const_iterator cellview_iter (int cv_index) const;

  lay::Plugin *create_plugin (const lay::PluginDeclaration *cls);
  void clear_plugins ();
  virtual void create_plugins (const lay::PluginDeclaration *except_this = 0);

  void free_resources ();
  void shutdown ();
  void init_menu ();

  virtual void finish ();
  virtual tl::Color default_background_color ();
  virtual void do_set_background_color (tl::Color color, tl::Color contrast);
  virtual void do_paste ();
  virtual void begin_layer_updates ();
  virtual void end_layer_updates ();
  virtual void clear_layer_selection ();
  virtual void do_set_no_stipples (bool no_stipples);
  virtual void do_set_phase (int phase);
  virtual bool is_activated () const;
  virtual void do_change_active_cellview ();
  virtual void update_content_for_cv (int cv_index);
  virtual bool set_hier_levels_basic (std::pair<int, int> l);

  virtual void bookmarks_changed () { }

  void ensure_layer_selected ();

  void enable_active_cellview_changed_event (bool enable, bool silent = false);
  void active_cellview_changed (int index);

  virtual void emit_edits_enabled_changed () { }
  virtual void emit_title_changed () { }
  virtual void emit_dirty_changed () { }
  virtual void emit_layer_order_changed () { }
};

}

#endif
