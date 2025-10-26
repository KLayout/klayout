
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


#ifndef HDR_edtPathService
#define HDR_edtPathService

#include "edtShapeService.h"

namespace edt
{

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
  virtual void via (int dir);
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

protected:
  bool configure (const std::string &name, const std::string &value);
  void config_finalize ();

private:
  struct PathSegment
  {
    PathSegment () : cv_index (0), transaction_id (0) { }

    db::LayerProperties layer;
    int cv_index;
    std::list<std::pair<std::string, std::string> > config;
    std::vector<db::DPoint> points;
    db::DPoint last_point;
    db::Shape path_shape;
    db::Instance via_instance;
    db::ViaType via_type;
    db::Manager::transaction_id_t transaction_id;
  };

  std::vector<db::DPoint> m_points;
  double m_width, m_bgnext, m_endext;
  enum { Flush = 0, Square, Variable, Round } m_type;
  bool m_needs_update;
  db::DPoint m_last;
  std::list<PathSegment> m_previous_segments;

  void update_marker ();
  db::Path get_path () const;
  void set_last_point (const db::DPoint &p);
  void update_via ();
  void compute_via_wh (double &w, double &h, const db::DVector &dwire, double var_ext, double grid);
  db::Instance make_via (const db::SelectedViaDefinition &via_def, double w_bottom, double h_bottom, double w_top, double h_top, const db::DPoint &via_pos);
  void via_initial (int dir);
  void via_editing (int dir);
  bool get_via_for (const db::LayerProperties &lp, unsigned int cv_index, int dir, db::SelectedViaDefinition &via_def);
  db::LayerProperties get_layer_for_via (unsigned int cv_index);
  void push_segment (const db::Shape &shape, const db::Instance &instance, const db::ViaType &via_type, db::Manager::transaction_id_t transaction_id);
  void pop_segment ();
};

}

#endif

