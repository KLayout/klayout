
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


#ifndef HDR_edtShapeService
#define HDR_edtShapeService

#include "edtService.h"
#include "edtEditorHooks.h"

namespace edt
{

/**
 *  @brief Implementation of the edt::Service for generic shape editing
 */
class ShapeEditService
  : public edt::Service
{
public:
  ShapeEditService (db::Manager *manager, lay::LayoutViewBase *view, db::ShapeIterator::flags_type shape_types);
  
protected:
  void get_edit_layer ();
  void change_edit_layer (const db::LayerProperties &lp);

  const db::VCplxTrans &trans () const { return m_trans; }
  unsigned int layer () const          { return m_layer; }
  unsigned int cv_index () const       { return m_cv_index; }
  db::Cell &cell () const              { return *mp_cell; }
  db::Layout &layout () const          { return *mp_layout; }

  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual void tap (const db::DPoint &initial);

  virtual bool configure (const std::string &name, const std::string &value);
  virtual void activated ();

protected:
  std::pair <bool, db::DPoint> interpolate (const db::DPoint &m, const db::DPoint &o, const db::DPoint &p) const;
  void deliver_shape (const db::Polygon &poly);
  void deliver_shape (const db::Path &path);
  void deliver_shape (const db::Box &box);
  void deliver_shape (const db::Point &point);
  void set_layer (const db::LayerProperties &lp, unsigned int cv_index);
  void open_editor_hooks ();
  template <class Shape>
  void deliver_shape_to_hooks (const Shape &shape);
  void close_editor_hooks (bool with_commit);
  combine_mode_type combine_mode () const { return m_combine_mode; }
  void config_recent_for_layer (const db::LayerProperties &lp, int cv_index);

  const tl::weak_collection<edt::EditorHooks> &editor_hooks ()
  {
    return m_editor_hooks;
  }

private:
  db::VCplxTrans m_trans;
  unsigned int m_layer;
  unsigned int m_cv_index;
  db::Cell *mp_cell;
  db::Layout *mp_layout;
  combine_mode_type m_combine_mode;
  tl::weak_collection<edt::EditorHooks> m_editor_hooks;
  bool m_update_edit_layer_enabled;

  void update_edit_layer (const lay::LayerPropertiesConstIterator &iter);
};

}

#endif

