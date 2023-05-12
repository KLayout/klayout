
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

#include "layLayoutView.h"

namespace lay
{

LayoutView::LayoutView (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : LayoutViewBase (this, mgr, editable, plugin_parent, options)
{
  //  NOTE: it's important to call LayoutViewBase::init from a fully constructed LayoutView (issue #1360)
  LayoutViewBase::init (mgr);
  config_setup ();
  finish ();
}

LayoutView::LayoutView (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : LayoutViewBase (this, mgr, editable, plugin_parent, options)
{
  //  NOTE: it's important to call LayoutViewBase::init from a fully constructed LayoutView (issue #1360)
  LayoutViewBase::init (mgr);
  copy_from (source);
  config_setup ();
  finish ();
}

//  NOTE: this methods needs to be called "frequently"
void
LayoutView::timer ()
{
  LayoutViewBase::timer ();

  //  Without Qt, this is also the opportunity to execute deferred methods
  tl::DeferredMethodScheduler::execute ();

  //  We also issue the "image_updated" event if the image ("screenshot") has been updated
  if (canvas ()->image_updated ()) {
    image_updated_event ();
  }

  //  And also the drawing_finished event
  if (canvas ()->drawing_finished ()) {
    drawing_finished_event ();
  }
}

static tl::weak_ptr<lay::LayoutView> s_current_view;

LayoutView *LayoutView::current ()
{
  return s_current_view.get ();
}

void LayoutView::set_current (LayoutView *view)
{
  s_current_view.reset (view);
}

void LayoutView::set_current ()
{
  s_current_view.reset (this);
}

} // namespace lay

#endif
