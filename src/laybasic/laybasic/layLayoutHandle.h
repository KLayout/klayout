
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

#ifndef HDR_layLayoutViewHandle
#define HDR_layLayoutViewHandle

#include "laybasicCommon.h"

#include "dbLayout.h"
#include "dbStream.h"

#if defined(HAVE_QT)
#  include "tlFileSystemWatcher.h"
#endif

#include <string>
#include <vector>
#include <map>

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

}

#endif

