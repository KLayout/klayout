
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


#ifndef HDR_edtBoxService
#define HDR_edtBoxService

#include "edtShapeService.h"

namespace edt
{

/**
 *  @brief Implementation of edt::Service for box editing
 */
class BoxService
  : public ShapeEditService
{
public:
  static const char *configure_name ();
  static const char *function_name ();

  BoxService (db::Manager *manager, lay::LayoutViewBase *view);
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_finish_edit (bool accept);
  virtual void do_cancel_edit ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;
  virtual void function (const std::string &name, const std::string &value);

private:
  db::DPoint m_p1, m_p2;
  bool m_centered;

  void update_marker ();
  db::Box get_box () const;
};

}

#endif

