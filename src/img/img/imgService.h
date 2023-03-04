
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



#ifndef HDR_imgService
#define HDR_imgService

#include "imgCommon.h"

#include "layViewObject.h"
#include "layEditable.h"
#include "layPlugin.h"
#include "laySnap.h"
#include "layAnnotationShapes.h"
#include "tlEvents.h"
#include "imgObject.h"
#include "tlAssert.h"

#include <map>
#include <vector>

namespace img {

class LayoutView;
class LayoutCanvas;
class Service;

// -------------------------------------------------------------

class IMG_PUBLIC View
  : public lay::ViewObject
{
public: 
  typedef lay::AnnotationShapes::iterator obj_iterator;
  enum Mode { mode_normal, mode_transient, mode_transient_move };

  /**
   *  @brief Constructor attaching to a certain object
   */
  View (img::Service *service, obj_iterator image_ref, Mode mode);

  /**
   *  @brief Constructor attaching to a certain object outside the database
   */
  View (img::Service *service, const img::Object *object, Mode mode);

  /**
   *  @brief The destructor
   */
  ~View ();

  /**
   *  @brief Set a transformation
   *
   *  The transformation how the image is transformed before being painted.
   */
  void transform_by (const db::DCplxTrans &p);

  /**
   *  @brief Get the image object that this view object is presenting
   */
  const img::Object *image_object () const
  {
    if (mp_image_object) {
      return mp_image_object;
    } else {
      return dynamic_cast <const img::Object *> ((*m_image_ref).ptr ());
    }
  }

  /**
   *  @brief Get the underlying image reference
   */
  obj_iterator image_ref () const
  {
    tl_assert (mp_image_object == 0);
    return m_image_ref;
  }

private:
  img::Service *mp_service;
  Mode m_mode;
  const img::Object *mp_image_object;
  obj_iterator m_image_ref;
  db::DCplxTrans m_trans;

  virtual void render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas);

  //  no copying nor default construction
  View (const View &d);
  View &operator= (const View &d);
  View ();
};

// -------------------------------------------------------------

/**
 *  @brief An iterator for "image annotation objects only"
 */
class IMG_PUBLIC ImageIterator
{
public:
  typedef const img::Object value_type;
  typedef const value_type *pointer; 
  typedef const value_type &reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  ImageIterator (lay::AnnotationShapes::iterator begin, lay::AnnotationShapes::iterator end)
    : m_current (begin), m_end (end)
  {
    next_valid ();
  }

  ImageIterator ()
    : m_current (), m_end ()
  {
    //  .. nothing yet ..
  }

  ImageIterator (const ImageIterator &d)
    : m_current (d.m_current), m_end (d.m_end)
  {
    //  .. nothing yet ..
  }

  ImageIterator &operator= (const ImageIterator &d)
  {
    if (this != &d) {
      m_current = d.m_current;
      m_end = d.m_end;
    }
    return *this;
  }

  const img::Object &operator* () const
  {
    return *(dynamic_cast <const img::Object *> (m_current->ptr ()));
  }

  const img::Object *operator-> () const
  {
    return dynamic_cast <const img::Object *> (m_current->ptr ());
  }

  ImageIterator &operator++ () 
  {
    ++m_current;
    next_valid ();
    return *this;
  }

  bool at_end () const
  {
    return m_current == m_end;
  }

  lay::AnnotationShapes::iterator basic_iterator () const
  {
    return m_current;
  }

private:
  void next_valid ()
  {
    while (m_current != m_end && dynamic_cast<const img::Object *> (m_current->ptr ()) == 0) {
      ++m_current;
    }
  }

  lay::AnnotationShapes::iterator m_current, m_end;
};

// -------------------------------------------------------------

class IMG_PUBLIC Service
  : public lay::BackgroundViewObject,
    public lay::Editable,
    public lay::Plugin,
    public db::Object
{
public: 
  typedef lay::AnnotationShapes::iterator obj_iterator;
  enum MoveMode { move_none, move_selected, move_landmark, move_l, move_r, move_t, move_b, move_lr, move_tr, move_ll, move_tl, move_all };

  Service (db::Manager *manager, lay::LayoutViewBase *view);

  ~Service ();

  /** 
   *  @brief Clear all highlights (for current object highlighting)
   */
  void clear_highlights ();

  /** 
   *  @brief Restore all highlights (for current object highlighting)
   */
  void restore_highlights ();

  /** 
   *  @brief Highlight a certain object
   */
  void highlight (unsigned int n);

  /**
   *  @brief Cancel any edit operations (such as move)
   */
  void edit_cancel ();

  /** 
   *  @brief Clear all images (menu callback)
   */
  void clear_images ();

  /** 
   *  @brief Add an image (menu callback)
   */
  void add_image ();

  /** 
   *  @brief Bring selected images to front
   */
  void bring_to_front ();

  /** 
   *  @brief Bring selected images to back
   */
  void bring_to_back ();

  /**
   *  @brief Get the image object by Id
   *
   *  If the Id is not valid, 0 is returned.
   */
  const img::Object *object_by_id (size_t id) const; 

  /** 
   *  @brief "delete" operation
   */
  virtual void del ();

  /** 
   *  @brief "cut" operation
   */
  virtual void cut ();

  /** 
   *  @brief "copy" operation
   */
  virtual void copy ();

  /** 
   *  @brief "paste" operation
   */
  virtual void paste ();

  /**
   *  @brief Indicates if any objects are selected
   */
  virtual bool has_selection ();

  /**
   *  @brief Indicates how many objects are selected
   */
  virtual size_t selection_size ();

  /**
   *  @brief Indicates if any objects are selected in transient mode
   */
  virtual bool has_transient_selection ();

  /**
   *  @brief point selection proximity predicate
   */
  virtual double click_proximity (const db::DPoint &pos, lay::Editable::SelectionMode mode);

  /**
   *  @brief Gets the catch distance for single click
   */
  virtual double catch_distance ();

  /**
   *  @brief Gets the catch distance for box
   */
  virtual double catch_distance_box ();

  /**
   *  @brief "select" operation
   */
  virtual bool select (const db::DBox &box, lay::Editable::SelectionMode mode);

  /**
   *  @brief Clears the previous selection
   */
  virtual void clear_previous_selection ();

  /**
   *  @brief Establish a transient selection
   */
  virtual bool transient_select (const db::DPoint &pos);

  /**
   *  @brief Turns the transient selection to the selection
   */
  virtual void transient_to_selection ();

  /**
   *  @brief Clear the transient selection
   */
  virtual void clear_transient_selection ();

  /**
   *  @brief Insert an image 
   */
  img::Object *insert_image (const img::Object &image);

  /**
   *  @brief Reimplement the mouse move handler
   */
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Begin a "move" operation
   */
  virtual bool begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Continue a "move" operation
   */
  virtual void move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Transform during a move operation
   */
  virtual void move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type ac);

  /**
   *  @brief Terminate a "move" operation
   */
  virtual void end_move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Return the bbox of the selection (reimplementation of lay::Editable interface)
   */
  virtual db::DBox selection_bbox ();

  /**
   *  @brief Transform the selection (reimplementation of lay::Editable interface)
   */
  virtual void transform (const db::DCplxTrans &trans);

#if defined(HAVE_QT)
  /**
   *  @brief Create the properties page
   */
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif

  /**
   *  @brief Get the selection for the properties page
   */
  void get_selection (std::vector <obj_iterator> &selection) const;

  /**
   *  @brief Direct access to the selection 
   */
  const std::map<obj_iterator, unsigned int> &selection () const
  {
    return m_selected;
  }

  /**
   *  @brief Delete a specific image 
   */
  void erase_image (obj_iterator pos);

  /**
   *  @brief Delete a specific image by Id
   *
   *  If the Id is not valid, the object is not deleted.
   */
  void erase_image_by_id (size_t id);

  /**
   *  @brief Change a specific image 
   */
  void change_image (obj_iterator pos, const img::Object &to);

  /**
   *  @brief Change a specific image by id
   *
   *  If the Id is not valid, the object is not deleted.
   */
  void change_image_by_id (size_t id, const img::Object &to);

  /**
   *  @brief Implementation of "Plugin" interface: configuration setup
   */
  bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Implementation of "Plugin" interface: configuration finalization
   */
  void config_finalize ();

  /**
   *  @brief Obtain the lay::Editable interface
   */
  lay::Editable *editable_interface ()
  {
    return this;
  }

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  /**
   *  @brief Shows or hides the images
   */
  void show_images (bool f);

  /**
   *  @brief Returns a value indicating whether images are shown or hidden
   */
  bool images_visible () const
  {
    return m_images_visible;
  }
  
  /**
   *  @brief Implement the menu response function
   */
  void menu_activated (const  std::string &symbol);

  /**
   *  @brief Return the iterator that delivers the image annotations (and only these)
   */
  ImageIterator begin_images () const;

  /**
   *  @brief An event indicating that something with the images has changed
   *  This event is triggered if images are added or removed
   */
  tl::Event images_changed_event;

  /**
   *  @brief An event indicating that the properties of one image have changed
   *  This event is triggered if the properties of one image have changed. The event
   *  argument is the ID of the image that has changed.
   */
  tl::event<int> image_changed_event;

  /**
   *  @brief An event indicating that the image selection has changed
   */
  tl::Event image_selection_changed_event;

private:
  //  The layout view that the image service is attached to
  lay::LayoutViewBase *mp_view;

  //  The view objects representing the selection and the moved images in move mode
  std::vector<View *> m_selected_image_views;
  //  The present views - only used for issueing a proper
  //  The selection
  std::map<obj_iterator, unsigned int> m_selected;
  //  The previous selection
  std::map<obj_iterator, unsigned int> m_previous_selection;
  //  The reference point in move mode
  db::DPoint m_p1;
  //  The image object representing the image being moved as it was before it was moved
  img::Object m_initial;
  //  The image object representing the image being moved
  img::Object m_current;
  //  The transformation in MoveSelection mode
  db::DTrans m_trans;
  //  The image representing the transient selection
  img::View *mp_transient_view;
  //  The current move mode
  MoveMode m_move_mode;
  //  The index of the landmark being moved
  size_t m_moved_landmark;
  //  Flag indicating that we want to keep the selection after the landmark was moved
  bool m_keep_selection_for_move;
  //  Flag indicating whether images are visible
  bool m_images_visible;

  void show_message ();

  /**
   *  @brief Select a certain image
   *
   *  @return true, if the selection has changed
   */
  bool select (obj_iterator obj, lay::Editable::SelectionMode mode);

  /**
   *  @brief Clear the selection
   */
  void clear_selection ();

  /**
   *  @brief Delete the selected images 
   *
   *  Used as implementation for "del" and "cut"
   */
  void del_selected ();

  /**
   *  @brief Copy the selected images to the clipboard
   *
   *  Used as implementation for "copy" and "cut"
   */
  void copy_selected ();

  /**
   *  @brief Finds an image object from the given point
   */
  const db::DUserObject *find_image (const db::DPoint &p, const db::DBox &search_box, double l, double &dmin, const std::map<img::Service::obj_iterator, unsigned int> *exclude = 0);

  /**
   *  @brief Update m_selected_image_views to reflect the selection
   */
  void selection_to_view (img::View::Mode mode = img::View::mode_normal);

  /**
   *  @brief Implementation of ViewObject: render the images on the background
   */
  void render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas);

  /**
   *  @brief Get the image object iterator by Id
   *
   *  If the Id is not valid, the end iterator is returned.
   */
  obj_iterator object_iter_by_id (size_t id) const; 

  /**
   *  @brief Display a message about the current selection
   */
  void display_status (bool transient);

  /**
   *  @brief Gets a value indicating the (new) top z position
   */
  int top_z_position () const;

  /**
   *  @brief Event handler for changes in the annotations
   */
  void annotations_changed ();
};

}

#endif

