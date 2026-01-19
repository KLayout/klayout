
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


#ifndef HDR_edtInstService
#define HDR_edtInstService

#include "edtService.h"
#include "edtEditorHooks.h"

namespace lay
{
  class CellView;
  class DragDropDataBase;
}

namespace edt
{

/**
 *  @brief Implementation of edt::Service for instance editing
 */
class InstService
  : public edt::Service
{
public:
  InstService (db::Manager *manager, lay::LayoutViewBase *view);
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_mouse_transform (const db::DPoint &p, db::DFTrans trans);
  virtual void do_finish_edit (bool accept);
  virtual void do_cancel_edit ();
  virtual bool do_activated ();
#if defined(HAVE_QT)
  virtual bool drag_enter_event (const db::DPoint &p, const lay::DragDropDataBase *data);
  virtual bool drag_move_event (const db::DPoint &p, const lay::DragDropDataBase *data);
  virtual void drag_leave_event ();
  virtual bool drop_event (const db::DPoint &p, const lay::DragDropDataBase *data);
#endif
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

protected:
  bool configure (const std::string &name, const std::string &value);
  void service_configuration_changed ();

  void config_finalize ();

private:
  double m_angle;
  double m_scale;
  bool m_mirror;
  db::DPoint m_disp;
  std::string m_cell_or_pcell_name, m_lib_name;
  std::string m_cell_or_pcell_name_previous, m_lib_name_previous;
  std::map<std::string, tl::Variant> m_pcell_parameters;
  std::map<std::pair<std::string, std::string>, std::map<std::string, tl::Variant> > m_stored_pcell_parameters;
  bool m_is_pcell;
  bool m_array;
  unsigned int m_rows, m_columns;
  double m_row_x, m_row_y, m_column_x, m_column_y;
  bool m_place_origin;
  db::Manager::transaction_id_t m_reference_transaction_id;
  bool m_needs_update, m_parameters_changed;
  bool m_has_valid_cell;
  bool m_in_drag_drop;
  db::cell_index_type m_current_cell;
  db::Layout *mp_current_layout;
  const db::PCellDeclaration *mp_pcell_decl;
  int m_cv_index;
  db::ICplxTrans m_trans;
  tl::weak_collection<edt::EditorHooks> m_editor_hooks;

  void update_marker ();
  bool get_inst (db::CellInstArray &inst);
  std::pair<bool, db::cell_index_type> make_cell (const lay::CellView &cv);
  tl::Variant get_default_layer_for_pcell ();
  void sync_to_config ();
  void switch_cell_or_pcell (bool switch_parameters);
  void open_editor_hooks ();
  void close_editor_hooks (bool with_commit);

  const tl::weak_collection<edt::EditorHooks> &editor_hooks ()
  {
    return m_editor_hooks;
  }
};

}

#endif

