
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

#if !defined(HAVE_QT)

#ifndef HDR_layLayoutViewNoQt
#define HDR_layLayoutViewNoQt

#include "layviewCommon.h"
#include "layLayoutViewBase.h"

namespace lay
{

/**
 *  @brief The layout view object
 *
 *  The layout view is responsible for displaying one or a set of layouts.
 *  It manages the layer display list and many other components.
 */
class LAYVIEW_PUBLIC LayoutView
  : public LayoutViewBase
{
public:
  /**
   *  @brief Constructor
   */
  LayoutView (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  /**
   *  @brief Constructor (clone from another view)
   */
  LayoutView (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  /**
   *  @brief This event is triggered in the "timer" callback when the image ("screenshot") was updated.
   */
  tl::Event image_updated_event;

  /**
   *  @brief This event is triggered in the "timer" callback when the drawing thread has finished.
   */
  tl::Event drawing_finished_event;

  /**
   *  @brief A callback that needs to be called "frequently"
   */
  void timer ();

  /**
   *  @brief Makes this view the current one
   */
  void set_current ();

  /**
   *  @brief Gets the current view
   */
  static LayoutView *current ();

  /**
   *  @brief Sets the current view
   */
  static void set_current (LayoutView *view);

protected:
  /**
   *  @brief Gets the LayoutView interface
   */
  virtual LayoutView *get_ui () { return this; }

private:
  using LayoutViewBase::ui;
};

}

#endif

#endif
