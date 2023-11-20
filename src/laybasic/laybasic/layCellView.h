
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


#ifndef HDR_layCellView
#define HDR_layCellView

#include "laybasicCommon.h"

#include <string>
#include <vector>

#include "tlObject.h"
#include "dbLayout.h"
#include "dbMetaInfo.h"
#include "dbReader.h"
#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbInstElement.h"
#include "dbTechnology.h"
#include "gsi.h"

#if defined(HAVE_QT)
#  include "tlFileSystemWatcher.h"
#endif

namespace lay 
{

class LayoutViewBase;

/**
 *  @brief A layout handle
 *
 *  This object controls a layout object. A layout object can be
 *  identified through a name. Additionally, a reference count
 *  is maintained that controls when the layout object is deleted.
 */
class LAYBASIC_PUBLIC LayoutHandle
  : public tl::Object
{
public:
  /**
   *  @brief Creates a layout handle to the given object
   *
   *  This constructor creates a new handle to the given 
   *  layout object. The handle takes over the ownership over the
   *  layout object.
   *  The initial reference count is zero (see remove_ref).
   *  The filename is a string that is supposed to identify
   *  the layout further. It can be retrieved with the filename 
   *  method.
   */
  LayoutHandle (db::Layout *layout, const std::string &filename);

  /**
   *  @brief Destructor
   *
   *  The destructor will delete the layout object that
   *  was associated with this handle.
   */
  ~LayoutHandle ();

  /**
   *  @brief Renames the layout object
   *
   *  If "force" is set to true, the layout will be given that name, irregardless if
   *  the name already is being used. If "force" is false, a new unique name is created.
   */
  void rename (const std::string &name, bool force = false);

  /**
   *  @brief Gets the name of the handle
   */
  const std::string &name () const;

  /**
   *  @brief Gets the layout object that this handle points to
   */
  db::Layout &layout () const;

  /**
   *  @brief Sets the file name associated with this handle
   */
  void set_filename (const std::string &);

  /**
   *  @brief Gets the file name associated with this handle
   */
  const std::string &filename () const;

  /**
   *  @brief Gets the technology attached to this layout
   */
  const db::Technology *technology () const;

  /**
   *  @brief Gets the technology name for this layout
   *
   *  An empty name indicates the default technology should be used.
   */
  const std::string &tech_name () const;

  /**
   *  @brief Applies the given technology
   *
   *  This will set the technology to the new one and send the apply event.
   *  This event is sent always, even if the technology did not change.
   */
  void apply_technology (const std::string &tn);

  /**
   *  @brief Sets the technology name
   *
   *  If there is no technology with that name, the default technology
   *  will be used.
   */
  void set_tech_name (const std::string &tn);

  /**
   *  @brief Finds a layout object by name
   *
   *  @param name The name under which to find the layout object
   *  @return 0, if there is no layout object with this name. Otherwise a pointer to its handle
   */
  static LayoutHandle *find (const std::string &name);

  /**
   *  @brief Finds a handle by layout object
   *
   *  @param layout The Layout object bound to the handle
   *  @return 0, if there is no layout object with this name. Otherwise a pointer to its handle
   */
  static LayoutHandle *find_layout (const db::Layout *layout);

  /**
   *  @brief Gets the names of all registered layout objects
   */
  static void get_names (std::vector <std::string> &names);

  /**
   *  @brief Gets the reference count
   */
  int get_ref_count () const
  {
    return m_ref_count;
  }

  /**
   *  @brief Adds a reference to the layout handle
   * 
   *  This method will increment the reference counter of this handle
   */
  void add_ref ();

  /**
   *  @brief Removes a reference to the layout handle
   *
   *  This method will decrement the reference counter. Once the 
   *  reference count reaches zero, the layout object and the
   *  handle is deleted.
   *  Upon initialization, the reference count is zero.
   *  Hint: it is generally not safe to access the handle after
   *  a remove_ref was issued.
   */
  void remove_ref ();

  /**
   *  @brief Returns true, if the layout is "dirty"
   *
   *  A layout is "dirty", if it needs to be saved. 
   *  It is set dirty if one of the signal handlers is triggered.
   */
  bool is_dirty () const
  {
    return m_dirty;
  }

  /**
   *  @brief Loads the layout
   *
   *  Load the layout from the file given in the constructor using the given layer map.
   *  The dirty flag is reset.
   *
   *  @param lmap The layer map specifying the layers to read
   *  @param technology The technology to use for layer map and other settings or empty for "default technology"
   *  @return The new layer map (can differ from the input since layers may be created)
   */
  db::LayerMap load (const db::LoadLayoutOptions &options, const std::string &technology);

  /**
   *  @brief Loads the layout
   *
   *  Load the layout from the file given in the constructor.
   *  The dirty flag is reset.
   *
   *  @return The new layer map
   */
  db::LayerMap load ();

  /**
   *  @brief Saves the layout
   *
   *  Save the layout under the given file name and with the given options.
   *  If update is true, this method updates the cell view's filename, title, save options and dirty flag.
   */
  void save_as (const std::string &filename, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update = true, int keep_backups = 0);

  /**
   *  @brief Sets the save options and a flag indicating whether they are valid
   *
   *  This method is mainly used by the session restore feature to restore the handle's state.
   */
  void set_save_options (const db::SaveLayoutOptions &options, bool valid);

  /**
   *  @brief Gets the current saving options
   *
   *  The saving options are set by the last save_as method call.
   */
  const db::SaveLayoutOptions &save_options () const
  {
    return m_save_options;
  }

  /**
   *  @brief Updates the given save options with attributes from this cell view
   *
   *  Some formats will initialize attributes from the cell view and the layout's
   *  metadata (example: libname of GDS2). This method will update the options
   *  if the layout provides attributes for initializing the latter.
   */
  void update_save_options (db::SaveLayoutOptions &options);

  /**
   *  @brief Gets a flag indicating whether the save options are valid
   *
   *  The save options are valid once the layout has been saved with specific
   *  options using "save_as" with the "update" options.
   */
  bool save_options_valid () const
  {
    return m_save_options_valid;
  }

  /**
   *  @brief Gets the current reader options
   *
   *  The reader options are set by the load method call.
   */
  const db::LoadLayoutOptions &load_options () const
  {
    return m_load_options;
  }

  /**
   *  @brief An event indicating that the technology has changed
   *  This event is triggered if the technology was changed.
   */
  tl::Event technology_changed_event;

  /**
   *  @brief An event indicating that a technology shall be applied
   *  This event is triggered to make the listeners apply a new technology
   *  to the layout.
   */
  tl::Event apply_technology_event;

  /**
   *  @brief An event indicating that a technology shall be applied
   *  This event is triggered to make the listeners apply a new technology
   *  to the layout. This version supplies a sender pointer.
   */
  tl::event<lay::LayoutHandle *> apply_technology_with_sender_event;

  /**
   *  @brief An event handler for a layout change
   *  This handler is attached to a layout changed event and will invalidate the handle.
   */
  void layout_changed ();

#if defined(HAVE_QT)
  /**
   *  @brief Gets the file system watcher that delivers events when one of the layouts gets updated
   */
  static tl::FileSystemWatcher &file_watcher ();
#endif

  /**
   *  @brief Removes a file from the watcher
   */
  static void remove_file_from_watcher (const std::string &path);

  /**
   *  @brief Adds a file to the watcher
   */
  static void add_file_to_watcher (const std::string &path);

private:
  db::Layout *mp_layout;
  int m_ref_count;
  std::string m_name;
  std::string m_filename;
  bool m_dirty;
  db::SaveLayoutOptions m_save_options;
  bool m_save_options_valid;
  db::LoadLayoutOptions m_load_options;

  void on_technology_changed ();

  static std::map <std::string, LayoutHandle *> ms_dict;
#if defined(HAVE_QT)
  static tl::FileSystemWatcher *mp_file_watcher;
#endif
};

/**
 *  @brief A layout handle reference
 *
 *  This class encapsulates a reference to a layout handle.
 *  The main purpose for this class is to automate the reference
 *  counting on the handle.
 */
class LAYBASIC_PUBLIC LayoutHandleRef 
{
public:
  LayoutHandleRef ();
  LayoutHandleRef (LayoutHandle *h);
  LayoutHandleRef (const LayoutHandleRef &r);
  ~LayoutHandleRef ();

  LayoutHandleRef &operator= (const LayoutHandleRef &r);
  
  bool operator== (const LayoutHandleRef &r) const;

  bool operator!= (const LayoutHandleRef &r) const
  {
    return !operator== (r);
  }

  LayoutHandle *operator-> () const;

  LayoutHandle *get () const;
  void set (LayoutHandle *h);

private:
  LayoutHandle *mp_handle;
};

/**
 *  @brief A "cell view" reference
 *
 *  A cell view reference points to a certain cell within a certain layout.
 *  The layout pointer can be 0, indicating that it is invalid.
 *  Also, the cell view describes a cell within that layout. The cell
 *  is addressed by an cell_index or a cell pointer. 
 *  The cell is not only identified by it's index or pointer but as well 
 *  by the path leading to that cell. This path describes how to find the
 *  cell in the context of parent cells. 
 *  The path is in fact composed in twofold: once in an unspecific fashion,
 *  just describing which parent cells are used. The target of this path
 *  is called the context cell. It is accessible by the ctx_cell_index
 *  or ctx_cell method.
 *  Additionally the path may further identify a certain instance of a certain
 *  subcell in the context cell. This is done through a set of db::InstElement
 *  objects. The target of this context path is the actual cell addressed by the
 *  cellview.
 *  In the viewer, the target cell is shown in the context of the context cell.
 *  The hierarchy levels are counted from the context cell, which is on level 0.
 *  If the context path is empty, the context cell is identical with the target cell.
 */

class LAYBASIC_PUBLIC CellView
  : public tl::Object
{
public:
  typedef db::cell_index_type cell_index_type;
  typedef std::vector <cell_index_type> unspecific_cell_path_type;
  typedef std::vector <db::InstElement> specific_cell_path_type;

  /**
   *  @brief Constructor: create an invalid cellview
   */
  CellView ();

  /**
   *  @brief Equality: compares the cell the cv points to, not the path
   */
  bool operator== (const CellView &cv) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellView &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Test if the cv points to a valid cell
   */
  bool is_valid () const;

  /**
   *  @brief Return the layout handle
   */
  lay::LayoutHandle *operator-> () const
  {
    return m_layout_href.get ();
  }

  /**
   *  @brief Return the layout handle (not via operator->)
   */
  lay::LayoutHandle *handle () const
  {
    return m_layout_href.get ();
  }

  /**
   *  @brief Set the unspecific part of the path explicitly
   *
   *  Setting the unspecific part will clear the context part and
   *  update the context and target cell.
   */
  void set_unspecific_path (const unspecific_cell_path_type &p);

  /**
   *  @brief Set the context part of the path explicitly
   *
   *  This method assumes that the unspecific part of the path 
   *  is established already and that the context part starts
   *  from the context cell.
   */
  void set_specific_path (const specific_cell_path_type &p);

  /**
   *  @brief Set the path to the given cell
   *
   *  This method will construct any path to this cell, not a 
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (cell_index_type index);

  /**
   *  @brief Set the cell by name
   *
   *  If the name is not a valid one, the cellview will become
   *  invalid.
   *  This method will construct any path to this cell, not a 
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (const std::string &name);

  /**
   *  @brief Reset the cell 
   *
   *  The cellview will become invalid. The layout object will
   *  still be attached to the cellview.
   */
  void reset_cell ();

  /**
   *  @brief Set the layout handle
   *
   *  Connect the cellview with a certain layout.
   *  This will reset the target and context cell.
   */
  void set (lay::LayoutHandle *handle);

  /**
   *  @brief Get the context cell pointer
   */
  db::Cell *ctx_cell () const
  {
    return mp_ctx_cell;
  }

  /**
   *  @brief Get the context cell index
   */
  cell_index_type ctx_cell_index () const
  {
    return m_ctx_cell_index;
  }

  /**
   *  @brief Get the target cell pointer
   */
  db::Cell *cell () const
  {
    return mp_cell;
  }

  /**
   *  @brief Get the target cell index
   */
  cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Get the cell's combined path in an unspecific form
   */
  unspecific_cell_path_type combined_unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const unspecific_cell_path_type &unspecific_path () const
  {
    return m_unspecific_path;
  }

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const specific_cell_path_type &specific_path () const
  {
    return m_specific_path;
  }

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path
   */
  db::ICplxTrans context_trans () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path as a micron-unit transformation
   */
  db::DCplxTrans context_dtrans () const;

  /**
   *  @brief Deep copy of the cellview
   *
   *  This method performs a deep copy on the cellview.
   *  A layout must be set already. Rather the creating another reference to the layout
   *  (which is done on operator= for example), this method copies the content of the 
   *  source layout to the current one and transfers cell path and other parameters.
   *
   *  @param manager The database object manager that the new layout is put under
   */
  CellView deep_copy (db::Manager *manager) const;

private:
  lay::LayoutHandleRef m_layout_href;
  db::Cell *mp_ctx_cell;
  cell_index_type m_ctx_cell_index;
  db::Cell *mp_cell;
  cell_index_type m_cell_index;
  unspecific_cell_path_type m_unspecific_path;
  specific_cell_path_type m_specific_path;
};

/**
 *  @brief A cellview reference
 *
 *  This object acts like a proxy to a lay::CellView object. It is connected to a cellview
 *  and a LayoutView and upon changes, the LayoutView will be configured accordingly.
 */
class LAYBASIC_PUBLIC CellViewRef
  : public gsi::ObjectBase
{
public:
  typedef CellView::unspecific_cell_path_type unspecific_cell_path_type;
  typedef CellView::specific_cell_path_type specific_cell_path_type;
  typedef CellView::cell_index_type cell_index_type;

  /**
   *  @brief Default constructor
   *  This constructor creates an invalid cellview reference
   */
  CellViewRef ();

  /**
   *  @brief Constructor
   *  @param cv The reference to the target cellview
   *  @param view The reference to the layout view
   */
  CellViewRef (lay::CellView *cv, lay::LayoutViewBase *view);

  /**
   *  @brief Gets the cellview index of this reference
   *  If the cellview is not valid, -1 will be returned.
   */
  int index () const;

  /**
   *  @brief Gets the LayoutViewBase the reference is pointing to
   */
  lay::LayoutViewBase *view ();

  /**
   *  @brief Equality: Gives true, if the cellviews are identical
   */
  bool operator== (const CellView &cv) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellView &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Equality: Gives true, if the references point to the same cellview
   */
  bool operator== (const CellViewRef &cv) const
  {
    return mp_cv.get () == cv.mp_cv.get ();
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellViewRef &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Test if the cv points to a valid cell and is valid otherwise
   */
  bool is_valid () const;

  /**
   *  @brief Returns the layout handle
   */
  lay::LayoutHandle *operator-> () const;

  /**
   *  @brief Return the layout handle (not via operator->)
   */
  lay::LayoutHandle *handle () const
  {
    return operator-> ();
  }

  /**
   *  @brief Sets the name of the cellview
   *
   *  This equivalent to calling Layout#rename_cellview.
   *  The name is made unique, hence the final name may
   *  differ from the one given.
   */
  void set_name (const std::string &name);

  /**
   *  @brief Set the unspecific part of the path explicitly
   *
   *  Setting the unspecific part will clear the context part and
   *  update the context and target cell.
   */
  void set_unspecific_path (const unspecific_cell_path_type &p);

  /**
   *  @brief Set the context part of the path explicitly
   *
   *  This method assumes that the unspecific part of the path
   *  is established already and that the context part starts
   *  from the context cell.
   */
  void set_specific_path (const specific_cell_path_type &p);

  /**
   *  @brief Set the path to the given cell
   *
   *  This method will construct any path to this cell, not a
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (cell_index_type index);

  /**
   *  @brief Set the cell by name
   *
   *  If the name is not a valid one, the cellview will become
   *  invalid.
   *  This method will construct any path to this cell, not a
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (const std::string &name);

  /**
   *  @brief Resets the cell
   */
  void reset_cell ();

  /**
   *  @brief Get the context cell pointer
   *  An invalid cellview reference will return a null pointer.
   */
  db::Cell *ctx_cell () const;

  /**
   *  @brief Get the context cell index
   */
  cell_index_type ctx_cell_index () const
  {
    db::Cell *c = ctx_cell ();
    return c ? c->cell_index () : 0;
  }

  /**
   *  @brief Get the target cell pointer
   *  An invalid cellview reference will return a null pointer.
   */
  db::Cell *cell () const;

  /**
   *  @brief Get the target cell index
   */
  cell_index_type cell_index () const
  {
    db::Cell *c = cell ();
    return c ? c->cell_index () : 0;
  }

  /**
   *  @brief Get the cell's combined path in an unspecific form
   */
  unspecific_cell_path_type combined_unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const unspecific_cell_path_type &unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const specific_cell_path_type &specific_path () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path
   */
  db::ICplxTrans context_trans () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path in micron units
   */
  db::DCplxTrans context_dtrans() const;

private:
  tl::weak_ptr<lay::CellView> mp_cv;
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
};

}

#endif

