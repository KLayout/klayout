
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


#ifndef HDR_edtServiceImpl
#define HDR_edtServiceImpl

#include "edtService.h"
#include "edtConfig.h"

#include <memory>

namespace lay
{
  class CellView;
  class LayoutViewBase;
  class LayerPropertiesConstIterator;
}

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

  const db::VCplxTrans &trans () const { return m_trans; }
  unsigned int layer () const          { return m_layer; }
  unsigned int cv_index () const       { return m_cv_index; }
  db::Cell &cell () const              { return *mp_cell; }
  db::Layout &layout () const          { return *mp_layout; }

  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual void tap (const db::DPoint &initial);

  virtual bool configure (const std::string &name, const std::string &value);

protected:
  std::pair <bool, db::DPoint> interpolate (const db::DPoint &m, const db::DPoint &o, const db::DPoint &p) const;
  void deliver_shape (const db::Polygon &poly);
  void deliver_shape (const db::Path &path);
  void deliver_shape (const db::Box &box);
  void deliver_shape (const db::Point &point);
  virtual void current_layer_changed () { }

private:
  db::VCplxTrans m_trans;
  unsigned int m_layer;
  unsigned int m_cv_index;
  db::Cell *mp_cell;
  db::Layout *mp_layout;
  combine_mode_type m_combine_mode;

  void update_edit_layer (const lay::LayerPropertiesConstIterator &iter);
};

/**
 *  @brief Implementation of edt::Service for polygon editing
 */
class PolygonService
  : public ShapeEditService
{
public:
  PolygonService (db::Manager *manager, lay::LayoutViewBase *view);
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_delete ();
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_finish_edit ();
  virtual void do_cancel_edit ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

private:
  std::vector <db::DPoint> m_points;
  bool m_closure_set;
  db::DPoint m_closure;
  db::DPoint m_last;

  void update_marker ();
  db::Polygon get_polygon () const;
  void add_closure ();
  void set_last_point (const db::DPoint &p);
};

/**
 *  @brief Implementation of edt::Service for box editing
 */
class BoxService
  : public ShapeEditService
{
public:
  BoxService (db::Manager *manager, lay::LayoutViewBase *view);
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_finish_edit ();
  virtual void do_cancel_edit ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

private:
  db::DPoint m_p1, m_p2;

  void update_marker ();
  db::Box get_box () const;
};

/**
 *  @brief Implementation of edt::Service for point editing
 */
class PointService
  : public ShapeEditService
{
public:
  PointService (db::Manager *manager, lay::LayoutViewBase *view);

#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_finish_edit ();
  virtual void do_cancel_edit ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

private:
  db::DPoint m_p;

  void update_marker ();
  db::Point get_point () const;
};

/**
 *  @brief Implementation of edt::Service for text editing
 */
class TextService
  : public ShapeEditService
{
public:
  TextService (db::Manager *manager, lay::LayoutViewBase *view);
  ~TextService ();
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_transform (const db::DPoint &p, db::DFTrans trans);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_finish_edit ();
  virtual void do_cancel_edit ();
  virtual bool do_activated ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

protected:
  virtual bool configure (const std::string &name, const std::string &value);

private:
  db::DText m_text;
  unsigned int m_rot;

  void update_marker ();
  db::Text get_text () const;
};

/**
 *  @brief Implementation of edt::Service for path editing
 */
class PathService
  : public ShapeEditService
{
public:
  PathService (db::Manager *manager, lay::LayoutViewBase *view);
  ~PathService ();
  
#if defined(HAVE_QT)
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif
  virtual void do_begin_edit (const db::DPoint &p);
  virtual void do_mouse_move (const db::DPoint &p);
  virtual bool do_mouse_click (const db::DPoint &p);
  virtual void do_mouse_move_inactive (const db::DPoint &p);
  virtual void do_delete ();
  virtual void do_finish_edit ();
  virtual void do_cancel_edit ();
  virtual bool do_activated ();
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

protected:
  bool configure (const std::string &name, const std::string &value);
  void config_finalize ();

private:
  std::vector <db::DPoint> m_points;
  double m_width, m_bgnext, m_endext;
  enum { Flush = 0, Square, Variable, Round } m_type;
  bool m_needs_update;
  db::DPoint m_last;

  void update_marker ();
  db::Path get_path () const;
  void set_last_point (const db::DPoint &p);
};

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
  virtual void do_finish_edit ();
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

  void update_marker ();
  bool get_inst (db::CellInstArray &inst);
  std::pair<bool, db::cell_index_type> make_cell (const lay::CellView &cv);
  tl::Variant get_default_layer_for_pcell ();
  void sync_to_config ();
  void switch_cell_or_pcell (bool switch_parameters);
};

}

#endif

