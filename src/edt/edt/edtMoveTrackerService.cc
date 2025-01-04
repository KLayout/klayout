
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

#include "layLayoutViewBase.h"
#include "edtMoveTrackerService.h"
#include "edtService.h"
#include "edtPartialService.h"

namespace edt
{

// -------------------------------------------------------------

MoveTrackerService::MoveTrackerService (lay::LayoutViewBase *view)
  : lay::EditorServiceBase (view),
    mp_view (view)
{ 
  //  .. nothing yet ..
}

MoveTrackerService::~MoveTrackerService ()
{
  //  .. nothing yet ..
}

bool  
MoveTrackerService::begin_move (lay::Editable::MoveMode mode, const db::DPoint & /*p*/, lay::angle_constraint_type /*ac*/)
{
  if (view ()->is_editable () && mode == lay::Editable::Selected) {
    open_editor_hooks ();
  }
  return false;
}

void
MoveTrackerService::issue_edit_events ()
{
  if (m_editor_hooks.empty ()) {
    return;
  }

  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::begin_edits);

  //  build the transformation variants cache
  TransformationVariants tv (view ());

  std::vector<edt::Service *> services = view ()->get_plugins<edt::Service> ();
  std::vector<lay::ObjectInstPath> sel;

  for (auto s = services.begin (); s != services.end (); ++s) {

    edt::Service *svc = *s;

    sel.clear ();
    svc->get_selection (sel);

    for (auto r = sel.begin (); r != sel.end (); ++r) {

      const lay::CellView &cv = view ()->cellview (r->cv_index ());

      //  compute the transformation into context cell's micron space
      double dbu = cv->layout ().dbu ();
      db::CplxTrans gt = db::CplxTrans (dbu) * cv.context_trans () * r->trans ();

      //  get one representative global transformation
      const std::vector<db::DCplxTrans> *tv_list = 0;
      if (r->is_cell_inst ()) {
        tv_list = tv.per_cv (r->cv_index ());
      } else {
        tv_list = tv.per_cv_and_layer (r->cv_index (), r->layer ());
      }
      if (tv_list && ! tv_list->empty ()) {
        gt = tv_list->front () * gt;
      }

      //  compute the move transformation in local object space
      db::ICplxTrans applied = gt.inverted () * db::DCplxTrans (svc->move_trans ()) * gt;

      call_editor_hooks<const lay::ObjectInstPath &, const db::ICplxTrans &, const db::CplxTrans &> (m_editor_hooks, &edt::EditorHooks::transformed, *r, applied, gt);

    }

  }

  //  make the Partial Edit Service issue "modify" events

  std::vector<edt::PartialService *> partial_services = view ()->get_plugins<edt::PartialService> ();

  for (auto s = partial_services.begin (); s != partial_services.end (); ++s) {
    (*s)->issue_editor_hook_calls (m_editor_hooks);
  }

  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::end_edits);
}

void  
MoveTrackerService::move (const db::DPoint & /*pu*/, lay::angle_constraint_type /*ac*/)
{
  //  we don't interpret this event, but use it to request status from the editor services
  issue_edit_events ();
}

void  
MoveTrackerService::move_transform (const db::DPoint & /*pu*/, db::DFTrans /*tr*/, lay::angle_constraint_type /*ac*/)
{
  //  we don't interpret this event, but use it to request status from the editor services
  issue_edit_events ();
}

void  
MoveTrackerService::end_move (const db::DPoint & /*p*/, lay::angle_constraint_type /*ac*/)
{
  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::commit_edit);
  move_cancel (); // formally this functionality fits here
}

void
MoveTrackerService::edit_cancel ()
{
  move_cancel ();
}

void
MoveTrackerService::move_cancel ()
{
  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::end_edit);
  m_editor_hooks.clear ();
}

void
MoveTrackerService::open_editor_hooks ()
{
  lay::CellViewRef cv_ref (view ()->cellview_ref (view ()->active_cellview_index ()));
  if (! cv_ref.is_valid ()) {
    return;
  }

  std::string technology;
  if (cv_ref->layout ().technology ()) {
    technology = cv_ref->layout ().technology ()->name ();
  }

  m_editor_hooks = edt::EditorHooks::get_editor_hooks (technology);
  call_editor_hooks<lay::CellViewRef &> (m_editor_hooks, &edt::EditorHooks::begin_edit, (lay::CellViewRef &) cv_ref);
}

} // namespace edt


