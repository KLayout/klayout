
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


#include "dbEdgeProcessor.h"
#include "dbPolygonTools.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbRegion.h"
#include "tlExceptions.h"
#include "layLayoutView.h"
#include "laySelector.h"
#include "layFinder.h"
#include "layLayerProperties.h"
#include "laybasicConfig.h"
#include "tlProgress.h"
#include "edtPlugin.h"
#include "edtMainService.h"
#include "edtService.h"
#include "edtServiceImpl.h"
#include "edtConfig.h"
#include "edtDistribute.h"

#if defined(HAVE_QT)
#  include "layDialogs.h"
#  include "layLayerTreeModel.h"
#  include "layCellSelectionForm.h"
#  include "edtDialogs.h"
#  include "edtEditorOptionsPages.h"
#endif

#if defined(HAVE_QT)
#  include <QInputDialog>
#  include <QMessageBox>
#  include <QFontInfo>
#  include <QWidgetAction>
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  Main Service implementation

MainService::MainService (db::Manager *manager, lay::LayoutViewBase *view, lay::Dispatcher *root)
  : lay::Plugin (view),
    lay::Editable (view),
    db::Object (manager),
    mp_view (view),
    mp_root (root),
    m_needs_update (false),
    m_flatten_insts_levels (std::numeric_limits<int>::max ()),
    m_flatten_prune (false),
    m_align_hmode (0), m_align_vmode (0), m_align_visible_layers (false),
    m_hdistribute (true),
    m_distribute_hmode (1), m_distribute_hpitch (0.0), m_distribute_hspace (0.0),
    m_vdistribute (true),
    m_distribute_vmode (1), m_distribute_vpitch (0.0), m_distribute_vspace (0.0),
    m_distribute_visible_layers (false),
    m_origin_mode_x (-1), m_origin_mode_y (-1), m_origin_visible_layers_for_bbox (false),
    m_array_a (0.0, 1.0), m_array_b (1.0, 0.0),
    m_array_na (1), m_array_nb (1),
    m_router (0.0), m_rinner (0.0), m_npoints (64), m_undo_before_apply (true)
{
#if defined(HAVE_QT)
  mp_round_corners_dialog = 0;
  mp_area_and_perimeter_dialog = 0;
  mp_align_options_dialog = 0;
  mp_distribute_options_dialog = 0;
  mp_flatten_inst_options_dialog = 0;
  mp_make_cell_options_dialog = 0;
  mp_make_array_options_dialog = 0;
#endif
}

MainService::~MainService ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)

edt::RoundCornerOptionsDialog *
MainService::round_corners_dialog ()
{
  if (! mp_round_corners_dialog) {
    mp_round_corners_dialog = new edt::RoundCornerOptionsDialog (lay::widget_from_view (view ()));
  }
  return mp_round_corners_dialog;
}

edt::AreaAndPerimeterDialog *
MainService::area_and_perimeter_dialog ()
{
  if (! mp_area_and_perimeter_dialog) {
    mp_area_and_perimeter_dialog = new edt::AreaAndPerimeterDialog (lay::widget_from_view (view ()));
  }
  return mp_area_and_perimeter_dialog;
}

edt::AlignOptionsDialog *
MainService::align_options_dialog ()
{
  if (! mp_align_options_dialog) {
    mp_align_options_dialog = new edt::AlignOptionsDialog (lay::widget_from_view (view ()));
  }
  return mp_align_options_dialog;
}

edt::DistributeOptionsDialog *
MainService::distribute_options_dialog ()
{
  if (! mp_distribute_options_dialog) {
    mp_distribute_options_dialog = new edt::DistributeOptionsDialog (lay::widget_from_view (view ()));
  }
  return mp_distribute_options_dialog;
}

lay::FlattenInstOptionsDialog *
MainService::flatten_inst_options_dialog ()
{
  if (! mp_flatten_inst_options_dialog) {
    mp_flatten_inst_options_dialog = new lay::FlattenInstOptionsDialog (lay::widget_from_view (view ()), false /*don't allow pruning*/);
  }
  return mp_flatten_inst_options_dialog;
}

edt::MakeCellOptionsDialog *
MainService::make_cell_options_dialog ()
{
  if (! mp_make_cell_options_dialog) {
    mp_make_cell_options_dialog = new edt::MakeCellOptionsDialog (lay::widget_from_view (view ()));
  }
  return mp_make_cell_options_dialog;
}

edt::MakeArrayOptionsDialog *
MainService::make_array_options_dialog ()
{
  if (! mp_make_array_options_dialog) {
    mp_make_array_options_dialog = new edt::MakeArrayOptionsDialog (lay::widget_from_view (view ()));
  }
  return mp_make_array_options_dialog;
}

#endif

void
MainService::menu_activated (const std::string &symbol)
{
  if (symbol == "edt::descend") {
    cm_descend ();
  } else if (symbol == "edt::ascend") {
    cm_ascend ();
  } else if (symbol == "edt::sel_align") {
    cm_align ();
  } else if (symbol == "edt::sel_distribute") {
    cm_distribute ();
  } else if (symbol == "edt::sel_tap") {
    cm_tap ();
  } else if (symbol == "edt::sel_round_corners") {
    cm_round_corners ();
  } else if (symbol == "edt::sel_area_perimeter") {
    cm_area_perimeter ();
  } else if (symbol == "edt::sel_convert_to_pcell") {
    cm_convert_to_pcell ();
  } else if (symbol == "edt::sel_convert_to_cell") {
    cm_convert_to_cell ();
  } else if (symbol == "edt::sel_size") {
    cm_size ();
  } else if (symbol == "edt::sel_union") {
    cm_union ();
  } else if (symbol == "edt::sel_intersection") {
    cm_intersection ();
  } else if (symbol == "edt::sel_separate") {
    cm_separate ();
  } else if (symbol == "edt::sel_difference") {
    cm_difference ();
  } else if (symbol == "edt::sel_change_layer") {
    cm_change_layer ();
  } else if (symbol == "edt::sel_flatten_insts") {
    cm_flatten_insts ();
  } else if (symbol == "edt::sel_resolve_arefs") {
    cm_resolve_arefs ();
  } else if (symbol == "edt::sel_move_hier_up") {
    cm_move_hier_up ();
  } else if (symbol == "edt::sel_make_cell") {
    cm_make_cell ();
  } else if (symbol == "edt::sel_make_array") {
    cm_make_array ();
  } else if (symbol == "edt::sel_make_cell_variants") {
    cm_make_cell_variants ();
  }
}

/**
 *  @brief A helper class to determine the common part of a set of instance elements
 */
class CommonInsts
{
public:
  CommonInsts () 
    : m_valid (true), m_first (true), m_ambiguous (false), m_cv_index (0) 
  { 
    //  .. nothing yet ..  
  }

  void add (const lay::ObjectInstPath &path, unsigned int n)
  {
    if (! m_valid) {

      //  don't do anything

    } else if (m_first) {

      m_common_inst.reserve (n);
      unsigned int nn = 0;
      for (lay::ObjectInstPath::iterator p = path.begin (); p != path.end () && nn < n; ++nn) {
        m_common_inst.push_back (*p);
      }
      m_cv_index = path.cv_index ();
      m_first = false;

    } else if (m_cv_index != path.cv_index ()) {

      m_valid = false;

    } else {

      std::vector<db::InstElement>::iterator i1 = m_common_inst.begin ();
      lay::ObjectInstPath::iterator i2 = path.begin ();
      while (i1 != m_common_inst.end () && i2 != path.end () && *i1 == *i2) {
        ++i1; ++i2;
      }
      if (i1 != m_common_inst.end ()) {
        m_ambiguous = true;
        m_common_inst.erase (i1, m_common_inst.end ());
      }

    }
  }

  const std::vector<db::InstElement> &common_path () const
  {
    return m_common_inst;
  }

  bool valid () const
  {
    return m_valid;
  }

  bool anything () const
  {
    return ! m_first;
  }

  bool ambiguous () const
  {
    return m_ambiguous && m_common_inst.empty ();
  }

  bool empty () const
  {
    return m_common_inst.empty ();
  }

  unsigned int size () const
  {
    return (unsigned int) m_common_inst.size ();
  }

  unsigned int cv_index () const
  {
    return m_cv_index;
  }

private:
  std::vector<db::InstElement> m_common_inst;
  bool m_valid;
  bool m_first;
  bool m_ambiguous;
  unsigned int m_cv_index;
};


void  
MainService::cm_descend ()
{
  CommonInsts common_inst;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end () && common_inst.valid (); ++es) {
    for (edt::Service::objects::const_iterator sel = (*es)->selection ().begin (); sel != (*es)->selection ().end () && common_inst.valid (); ++sel) {
      common_inst.add (*sel, 1);
    }
  }

  //  cannot descend - we are at the lowest level already
  if (common_inst.empty ()) {
    return;
  }

  if (! common_inst.anything ()) {
    throw tl::Exception (tl::to_string (tr ("Select an object to determine into which instance to descend")));
  }
  if (! common_inst.valid () || common_inst.ambiguous ()) {
    throw tl::Exception (tl::to_string (tr ("Selection is ambiguous - cannot determine into which instance to descend")));
  }

  //  remove the common path and create a new set of selections

  std::vector< std::vector<lay::ObjectInstPath> > new_selections;
  new_selections.reserve (edt_services.size ());

  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

    new_selections.push_back (std::vector<lay::ObjectInstPath> ());
    new_selections.back ().reserve ((*es)->selection ().size ());

    for (edt::Service::objects::const_iterator sel = (*es)->selection ().begin (); sel != (*es)->selection ().end (); ++sel) {

      new_selections.back ().push_back (*sel);
      lay::ObjectInstPath &new_sel = new_selections.back ().back ();
      new_sel.remove_front (common_inst.size ());
      //  it may happen that this way we dive into the instance selected. The resulting selection
      //  is not pointing to any instance any more and must be discarded therefore:
      if (new_sel.is_cell_inst () && new_sel.begin () == new_sel.end ()) {
        new_selections.back ().pop_back ();
      }

    }

  }

  //  this will clear the selection:
  view ()->descend (common_inst.common_path (), common_inst.cv_index ());
  view ()->set_current_cell_path (common_inst.cv_index (), view ()->cellview (common_inst.cv_index ()).combined_unspecific_path ());

  //  set the new selections
  unsigned int index = 0;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es, ++index) {
    (*es)->set_selection (new_selections [index].begin (), new_selections [index].end ());
  }

}

void  
MainService::cm_ascend ()
{
  //  add one path component and create a new set of selections

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  std::vector< std::vector<lay::ObjectInstPath> > new_selections;
  new_selections.reserve (edt_services.size ());
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    new_selections.push_back (std::vector<lay::ObjectInstPath> ());
    new_selections.back ().insert (new_selections.back ().end (), (*es)->selection ().begin (), (*es)->selection ().end ());
  }

  //  this will clear the selection:
  for (int cv_index = 0; cv_index < int (view ()->cellviews ()); ++cv_index) {

    db::InstElement removed = view ()->ascend (cv_index);
    if (removed != db::InstElement ()) {

      db::cell_index_type new_top = view ()->cellview (cv_index).cell_index ();
      view ()->set_current_cell_path (cv_index, view ()->cellview (cv_index).combined_unspecific_path ());

      //  create and the new selections
      unsigned int index = 0;
      for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es, ++index) {
        for (std::vector<lay::ObjectInstPath>::iterator sel = new_selections [index].begin (); sel != new_selections [index].end (); ++sel) {
          if (int (sel->cv_index ()) == cv_index) {
            sel->insert_front (new_top, removed);
          }
        }
      }

    }

  }

  unsigned int index = 0;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es, ++index) {
    (*es)->set_selection (new_selections [index].begin (), new_selections [index].end ());
  }

}

void  
MainService::cm_flatten_insts ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

#if defined(HAVE_QT)
  //  TODO: make parameters persistent so we can set them externally
  if (! (flatten_inst_options_dialog ()->exec_dialog (m_flatten_insts_levels, m_flatten_prune) && m_flatten_insts_levels != 0)) {
    return;
  }
#endif

  view ()->cancel_edits ();

  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Flatten instances")));
  }

  std::set<db::Layout *> needs_cleanup;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

    for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {

      const lay::CellView &cv = view ()->cellview (r->cv_index ());
      if (cv.is_valid ()) {

        db::Cell &target_cell = cv->layout ().cell (r->cell_index ());
        if (r->is_cell_inst () && target_cell.is_valid (r->back ().inst_ptr)) {

          //  because we select whole arrays in editable mode, we can iterator over them
          db::CellInstArray cell_inst = r->back ().inst_ptr.cell_inst ();
          for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
            cv->layout ().flatten (cv->layout ().cell (r->cell_index_tot ()), target_cell, cell_inst.complex_trans (*a), m_flatten_insts_levels < 0 ? m_flatten_insts_levels : m_flatten_insts_levels - 1);
          }

          if (cv->layout ().cell (r->back ().inst_ptr.cell_index ()).is_proxy ()) {
            needs_cleanup.insert (& cv->layout ());
          }

          target_cell.erase (r->back ().inst_ptr);

        }

      }

    }

  }

  //  clean up the layouts that need to do so.
  for (std::set<db::Layout *>::const_iterator l = needs_cleanup.begin (); l != needs_cleanup.end (); ++l) {
    (*l)->cleanup ();
  }

  //  The selection is no longer valid
  view ()->clear_selection ();

  if (manager ()) {
    manager ()->commit ();
  }
}

void  
MainService::cm_move_hier_up ()
{
  view ()->cancel_edits ();
  check_no_guiding_shapes ();

  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Move up in hierarchy")));
  }

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

    std::vector<lay::ObjectInstPath> new_selection;
    new_selection.reserve ((*es)->selection ().size ());

    for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {

      const lay::CellView &cv = view ()->cellview (r->cv_index ());
      if (cv.is_valid ()) {

        db::Cell &target_cell = *cv.cell ();

        if (r->is_cell_inst ()) {

          db::Instance new_inst = target_cell.insert (r->back ().inst_ptr);
          new_inst = target_cell.transform (new_inst, db::ICplxTrans (r->trans ()));

          new_selection.push_back (lay::ObjectInstPath ());
          new_selection.back ().set_topcell (r->topcell ());
          new_selection.back ().set_cv_index (r->cv_index ());
          new_selection.back ().add_path (db::InstElement (new_inst, db::CellInstArray::iterator ()));

        } else {

          db::Shapes &target_shapes = target_cell.shapes (r->layer ());
          db::Shape new_shape = target_shapes.insert (r->shape ());
          new_shape = target_shapes.transform (new_shape, db::ICplxTrans (r->trans ()));

          new_selection.push_back (lay::ObjectInstPath ());
          new_selection.back ().set_topcell (r->topcell ());
          new_selection.back ().set_cv_index (r->cv_index ());
          new_selection.back ().set_layer (r->layer ());
          new_selection.back ().set_shape (new_shape);

        }

      }

    }
          
    //  delete all the objects currently selected and set the new selection
    (*es)->del_selected ();

    (*es)->set_selection (new_selection.begin (), new_selection.end ());

  }

  if (manager ()) {
    manager ()->commit ();
  }
}

/**
 *  @brief A helper class for the cell variant builder
 *
 *  The purpose of this class is to implement instance resolution for variant building.
 *  Resolution means converting to an instance of a different cell.
 *  A normal (single) instance is easy to convert: a new instance is created and the
 *  cell index set to point to the new one. For an array instance however that is 
 *  more complicated. Since the instance to resolve is usually just a part of that array, it
 *  is required to split the original array and create new instances for the parts. These
 *  parts must be maintained, because one task of the resolver is to map other array members
 *  of the original array to new instances. In that case, the resolver has to look for a 
 *  suitable piece and return an array member instance to that one.
 */
class ArrayResolver
{
public:
  /**
   *  @brief Default ctor
   */
  ArrayResolver ()
    : m_na_before (0), m_na_after (0), m_nb_before (0), m_nb_after (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Plain instance resolution constructor
   *  The new instance is simply taken as the given one. This can be used to map array instances unconditionally or 
   *  for mapping single instances.
   */
  ArrayResolver (const db::Instance &new_inst)
    : m_new_inst (new_inst), m_na_before (0), m_na_after (0), m_nb_before (0), m_nb_after (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Resolve the given instance (elem) to a new one with the given new cell (new_cell_index)
   *  This method will resolve elem, create a corresponding instance with the new cell index. If necessary, new 
   *  instances for array pieces are created (see above) and inserted into the cell. The resolver will map other
   *  instances to these pieces when necessary in the "resolve" function.
   */
  void resolve (db::Cell &parent_cell, const db::InstElement &elem, db::cell_index_type new_cell_index)
  {
    bool has_props = elem.inst_ptr.has_prop_id ();
    db::properties_id_type prop_id = elem.inst_ptr.prop_id ();

    db::Vector a, b;
    unsigned long na = 0, nb = 0;

    elem.inst_ptr.is_regular_array (a, b, na, nb);

    long ia = elem.array_inst.index_a ();
    long ib = elem.array_inst.index_b ();

    if (ia >= 0 && ia < long (na) && ib >= 0 && ib <= long (nb)) {

      db::cell_index_type org_cell_index = elem.inst_ptr.cell_index ();

      m_na_before = (unsigned long) ia;
      m_na_after = na - (unsigned long) ia - 1;
      m_nb_before = (unsigned long) ib;
      m_nb_after = nb - (unsigned long) ib - 1;

      if (m_na_before > 0) {
        db::CellInstArray ia;
        if (elem.inst_ptr.is_complex ()) {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.complex_trans (), a, b, m_na_before, nb);
        } else {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.front (), a, b, m_na_before, nb);
        }
        m_inst_w = parent_cell.insert (ia);
        if (has_props) {
          m_inst_w = parent_cell.replace_prop_id (m_inst_w, prop_id);
        }
      }

      if (m_na_after > 0) {
        db::CellInstArray ia;
        if (elem.inst_ptr.is_complex ()) {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.complex_trans () * db::CellInstArray::complex_trans_type (a * long (m_na_before + 1)), a, b, m_na_after, nb);
        } else {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.front () * db::CellInstArray::trans_type (a * long (m_na_before + 1)), a, b, m_na_after, nb);
        }
        m_inst_e = parent_cell.insert (ia);
        if (has_props) {
          m_inst_e = parent_cell.replace_prop_id (m_inst_e, prop_id);
        }
      }

      if (m_nb_before > 0) {
        db::CellInstArray ia;
        if (elem.inst_ptr.is_complex ()) {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.complex_trans () * db::CellInstArray::complex_trans_type (a * long (m_na_before)), a, b, 1, m_nb_before);
        } else {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.front () * db::CellInstArray::trans_type (a * long (m_na_before)), a, b, 1, m_nb_before);
        }
        m_inst_s = parent_cell.insert (ia);
        if (has_props) {
          m_inst_s = parent_cell.replace_prop_id (m_inst_s, prop_id);
        }
      }

      if (m_nb_after > 0) {
        db::CellInstArray ia;
        if (elem.inst_ptr.is_complex ()) {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.complex_trans () * db::CellInstArray::complex_trans_type (a * long (m_na_before) + b * long (m_nb_before + 1)), a, b, 1, m_nb_after);
        } else {
          ia = db::CellInstArray (db::CellInst (org_cell_index), elem.inst_ptr.front () * db::CellInstArray::trans_type (a * long (m_na_before) + b * long (m_nb_before + 1)), a, b, 1, m_nb_after);
        }
        m_inst_n = parent_cell.insert (ia);
        if (has_props) {
          m_inst_n = parent_cell.replace_prop_id (m_inst_n, prop_id);
        }
      }

    } else {

      m_na_before = m_na_after = 0;
      m_nb_before = m_nb_after = 0;

    }

    {
      db::CellInstArray ia;
      if (elem.inst_ptr.is_complex ()) {
        ia = db::CellInstArray (db::CellInst (new_cell_index), elem.inst_ptr.complex_trans () * db::CellInstArray::complex_trans_type (a * long (m_na_before) + b * long (m_nb_before)));
      } else {
        ia = db::CellInstArray (db::CellInst (new_cell_index), elem.inst_ptr.front () * db::CellInstArray::trans_type (a * long (m_na_before) + b * long (m_nb_before)));
      }
      m_new_inst = parent_cell.replace (elem.inst_ptr, ia);
      if (has_props) {
        m_new_inst = parent_cell.replace_prop_id (m_new_inst, prop_id);
      }
    }

  }

  /**
   *  @brief resolves the instance to the new one
   *  "elem" is the original instance which should be resolved. The instance part of elem must be identical to the
   *  one originally passed to "resolve" or the plain instance resolution constructor, but the array iterator part
   *  may be different. In the latter case, a suitable array piece is selected.
   */
  db::InstElement resolved (const db::InstElement &elem) const
  {
    long ia = elem.array_inst.index_a ();
    long ib = elem.array_inst.index_b ();

    if (ia >= 0 && ib >= 0 /* we have an array member */) {

      if (ia == long (m_na_before) && ib == long (m_nb_before)) {
        return db::InstElement (m_new_inst, m_new_inst.cell_inst ().begin ());
      } else if (ia < long (m_na_before)) {
        return db::InstElement (m_inst_w, m_inst_w.cell_inst ().begin (ia, ib));
      } else if (ia == long (m_na_before) && ib < long (m_nb_before)) {
        return db::InstElement (m_inst_s, m_inst_s.cell_inst ().begin (0, ib));
      } else if (ia == long (m_na_before)) {
        return db::InstElement (m_inst_n, m_inst_n.cell_inst ().begin (0, ib - m_nb_before - 1));
      } else {
        return db::InstElement (m_inst_e, m_inst_e.cell_inst ().begin (ia - m_na_before - 1, ib));
      }

    } else {
      return db::InstElement (m_new_inst, m_new_inst.cell_inst ().begin ());
    }
  }

private:
  db::Instance m_new_inst;
  db::Instance m_inst_w, m_inst_e, m_inst_n, m_inst_s;
  unsigned long m_na_before, m_na_after, m_nb_before, m_nb_after;
};

void  
MainService::cm_make_cell_variants ()
{
  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  //  TODO: this limitation is not really necessary, but makes the code somewhat simpler
  int cv_index = -1;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {
      if (cv_index < 0) {
        cv_index = r->cv_index ();
      } else if (cv_index != int (r->cv_index ())) {
        throw tl::Exception (tl::to_string (tr ("The selection must not contain objects from different layouts for 'make cell variants'")));
      }
    }
  }

  if (cv_index < 0) {
    return;
  }

  const lay::CellView &cv = view ()->cellview (cv_index);
  db::Layout &layout = cv->layout ();

  view ()->cancel_edits ();

  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Make cell variants for selection")));
  }

  std::vector<lay::ObjectInstPath> new_selection;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    new_selection.insert (new_selection.end (), (*es)->selection ().begin (), (*es)->selection ().end ());
  }

  size_t num_sel = new_selection.size ();

  //  TODO: the algorithm is basically O(2) in the number of selected items. A first 
  //  step to mitigate that problem is to provide a progress and hence a way to cancel it.
  tl::RelativeProgress progress (tl::to_string (tr ("Make cell variants for selection")), num_sel, 1);

  for (size_t nsel = 0; nsel < num_sel; ++nsel) {

    ++progress;

    std::vector<lay::ObjectInstPath> selection;
    selection.swap (new_selection);

    //  A map for a part of the selection path to a new instance (value.first is the next original cell of the path and
    //  value.second the ArrayResolver which can be used to fetch the corresponding new instance in the new target cell).
    std::map<std::pair<db::cell_index_type, db::Instance>, std::pair<db::cell_index_type, ArrayResolver> > new_instances;

    //  Collect the current path (pairs of cell / instance in that cell).
    //  TODO: this rewriting of the path is not really required.
    std::vector<std::pair<db::cell_index_type, db::InstElement> > path;

    db::cell_index_type pc = selection[nsel].topcell ();
    for (lay::ObjectInstPath::iterator p = selection[nsel].begin (); p != selection[nsel].end () && ! layout.cell (p->inst_ptr.cell_index ()).is_proxy (); ++p) {
      path.push_back (std::make_pair (pc, *p));
      pc = p->inst_ptr.cell_index ();
    }

    if (! path.empty ()) {

      db::InstElement elem = path.front ().second;
      db::cell_index_type parent_cell_index = path.front ().first;

      bool needs_variant = false;

      //  create variants for each part of the path if required. While doing so, store information about the 
      //  mapping to the new path in new_instances.
      for (std::vector<std::pair<db::cell_index_type, db::InstElement> >::const_iterator p = path.begin (); p != path.end (); ++p) {

        db::Cell &parent_cell = layout.cell (parent_cell_index);
        db::Cell &org_cell = layout.cell (elem.inst_ptr.cell_index ());

        //  if the selection is concerning a single instance of an array, we always need to create variants.
        if (elem.inst_ptr.cell_inst ().size () > 1) {
          needs_variant = true;
        }

        if (! needs_variant) {
          //  needs a variant if more than one instance of it exists and then all child cells need a variant as well.
          db::Cell::parent_inst_iterator pi = org_cell.begin_parent_insts ();
          if (! pi.at_end ()) {
            ++pi;
          }
          needs_variant = ! pi.at_end ();
        }

        if (needs_variant) {

          //  need to create a variant: create a new cell
          db::cell_index_type new_cell_index = layout.add_cell (layout, elem.inst_ptr.cell_index ());

          //  prepare a new variant cell
          db::Cell &new_cell = layout.cell (new_cell_index);

          //  copy the shapes
          for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
            //  insert shape by shape (the container assignment does not support undo currently)
            db::Shapes &target = new_cell.shapes ((*l).first);
            for (db::ShapeIterator s = org_cell.shapes ((*l).first).begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
              target.insert (*s);
            }
          }

          //  copy the instances
          db::Instance next_inst;
          for (db::Cell::const_iterator i = org_cell.begin (); ! i.at_end (); ++i) {
            db::Instance ni = new_cell.insert (*i);
            if (p + 1 != path.end () && p[1].second.inst_ptr == *i) {
              next_inst = ni;
            } else {
              //  Plain resolution for all side branches
              new_instances.insert(std::make_pair (std::make_pair (org_cell.cell_index (), *i), std::make_pair (i->cell_index (), ArrayResolver (ni))));
            }
          }

          db::cell_index_type next_org = p->second.inst_ptr.cell_index ();

          //  Resolve the original instance into a new one (and split arrays while doing so)
          new_instances.insert(std::make_pair (std::make_pair (p->first, p->second.inst_ptr), std::make_pair (next_org, ArrayResolver ()))).first->second.second.resolve (parent_cell, elem, new_cell_index);

          if (p + 1 != path.end ()) {
            parent_cell_index = new_cell_index;
            elem = p[1].second;
            elem.inst_ptr = next_inst;
          }

        } else if (p + 1 != path.end ()) {
          parent_cell_index = elem.inst_ptr.cell_index ();
          elem = p[1].second;
        }

      }

    }

    //  map the selection to the new instances
    for (std::vector<lay::ObjectInstPath>::const_iterator r = selection.begin (); r != selection.end (); ++r) {

      db::cell_index_type cell = r->topcell ();

      new_selection.push_back (lay::ObjectInstPath ());
      lay::ObjectInstPath &new_path = new_selection.back ();

      new_path.set_seq (r->seq ());
      new_path.set_topcell (cell);
      new_path.set_cv_index (cv_index);

      //  map the path and move "cell" further along the original path.
      bool needs_translate = true;
      for (lay::ObjectInstPath::iterator p = r->begin (); p != r->end (); ++p) {
        std::map<std::pair<db::cell_index_type, db::Instance>, std::pair<db::cell_index_type, ArrayResolver> >::const_iterator ni = new_instances.end ();
        if (needs_translate) {
          ni = new_instances.find (std::make_pair (cell, p->inst_ptr));
        }
        if (ni != new_instances.end ()) {
          new_path.add_path (ni->second.second.resolved (*p));
          cell = ni->second.first;
          //  resolve may fold the path back to the original one if a side piece of an array instance was taken - stop translating in that case now.
          if (new_path.back ().inst_ptr.cell_index () == cell) {
            needs_translate = false;
          }
        } else {
          new_path.add_path (*p);
          cell = p->inst_ptr.cell_index ();
          needs_translate = false; // stop translating the path in the first original cell
        }
      }

      if (! r->is_cell_inst ()) {
        //  map the shape as well. Note that "cell" is the original cell where the shape came from.
        new_path.set_layer (r->layer ());
        if (new_path.begin () != new_path.end () && cell != new_path.back ().inst_ptr.cell_index ()) {
          db::Cell &new_cell = layout.cell (new_path.back ().inst_ptr.cell_index ());
          db::Shapes &shapes = new_cell.shapes (r->layer ());
          db::Shape shape = shapes.find (r->shape ());
          new_path.set_shape (shape);
        } else {
          new_path.set_shape (r->shape ());
        }
      }

    }

  }

  //  Install the new selection
  size_t i0 = 0;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    size_t n = (*es)->selection ().size ();
    if (n + i0 <= new_selection.size ()) {
      (*es)->set_selection (new_selection.begin () + i0, new_selection.begin () + i0 + n);
    }
    i0 += n;
  }

  if (manager ()) {
    manager ()->commit ();
  }
}

void  
MainService::cm_resolve_arefs ()
{
  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  edt::InstService *inst_service = view ()->get_plugin <edt::InstService> ();
  if (! inst_service) {
    return;
  }

  //  collect the instances to resolve
  std::vector<lay::ObjectInstPath> insts_to_resolve;

  int cv_index = -1;

  for (edt::Service::objects::const_iterator r = inst_service->selection ().begin (); r != inst_service->selection ().end (); ++r) {
    if (r->is_cell_inst () && r->back ().inst_ptr.size () > 1) {
      if (cv_index < 0) {
        cv_index = r->cv_index ();
      } else if (cv_index != int (r->cv_index ())) {
        //  TODO: this limitation is not really necessary, but makes the code somewhat simpler
        throw tl::Exception (tl::to_string (tr ("The selection must not contain objects from different layouts for 'resolve array references'")));
      }
      insts_to_resolve.push_back (*r);
    }
  }

  if (cv_index < 0 || insts_to_resolve.empty ()) {
    return;
  }

  view ()->cancel_edits ();

  db::Layout &layout = view ()->cellview (cv_index)->layout ();

  std::vector<lay::ObjectInstPath> new_selection;

  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Resolve array references")));
  }

  for (std::vector<lay::ObjectInstPath>::const_iterator p = insts_to_resolve.begin (); p != insts_to_resolve.end (); ++p) {

    db::Cell &parent_cell = layout.cell (p->cell_index ());

    db::CellInstArray aa = p->back ().inst_ptr.cell_inst ();
    for (db::CellInstArray::iterator ai = aa.begin (); ! ai.at_end (); ++ai) {

      db::CellInstArray na;
      if (aa.is_complex ()) {
        na = db::CellInstArray (aa.object (), aa.complex_trans (*ai));
      } else {
        na = db::CellInstArray (aa.object (), *ai);
      }

      db::Instance new_inst;
      if (p->back ().inst_ptr.has_prop_id ()) {
        new_inst = parent_cell.insert (db::CellInstArrayWithProperties (na, p->back ().inst_ptr.prop_id ()));
      } else {
        new_inst = parent_cell.insert (na);
      }

      new_selection.push_back (*p);
      new_selection.back ().back () = db::InstElement (new_inst);

    }

  }

  for (std::vector<lay::ObjectInstPath>::const_iterator p = insts_to_resolve.begin (); p != insts_to_resolve.end (); ++p) {
    layout.cell (p->cell_index ()).erase (p->back ().inst_ptr);
  }

  //  The selection is no longer valid: establish a new one
  view ()->clear_selection ();

  inst_service->set_selection (new_selection.begin (), new_selection.end ());

  if (manager ()) {
    manager ()->commit ();
  }
}

void  
MainService::cm_make_cell ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  int cv_index = -1;
  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {
      if (cv_index < 0) {
        cv_index = r->cv_index ();
      } else if (cv_index != int (r->cv_index ())) {
        throw tl::Exception (tl::to_string (tr ("The selection must not contain objects from different layouts for 'make cell'")));
      }
    }
  }

  if (cv_index >= 0) {

    const lay::CellView &cv = view ()->cellview (cv_index);

#if defined(HAVE_QT)
    //  TODO: make parameters persistent so we can set them externally
    if (! make_cell_options_dialog ()->exec_dialog (cv->layout (), m_make_cell_name, m_origin_mode_x, m_origin_mode_y)) {
      return;
    }
 #endif

    //  Compute the selection's bbox to establish a good origin for the new cell
    db::Box selection_bbox;
    db::box_convert<db::CellInst> bc (cv->layout ());
    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
      for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {
        if (r->is_cell_inst ()) {
          selection_bbox += db::ICplxTrans (r->trans ()) * r->back ().bbox (bc);
        } else {
          selection_bbox += db::ICplxTrans (r->trans ()) * r->shape ().bbox ();
        }
      }
    }

    if (selection_bbox.empty ()) {
      throw tl::Exception (tl::to_string (tr ("The selection is empty. Cannot create a cell from an empty selection.")));
    }

    view ()->cancel_edits ();

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Make cell from selection")));
    }

    db::cell_index_type target_ci = cv->layout ().add_cell (m_make_cell_name.c_str ());
    //  create target cell
    db::Cell &target_cell = cv->layout ().cell (target_ci);

    //  create target cell instance
    db::Vector ref;
    if (m_origin_mode_x >= -1) {
      ref = db::Vector (selection_bbox.left () + ((m_origin_mode_x + 1) * selection_bbox.width ()) / 2, selection_bbox.bottom () + ((m_origin_mode_y + 1) * selection_bbox.height ()) / 2);
    }

    db::Instance target_cell_inst = cv.cell ()->insert (db::CellInstArray (db::CellInst (target_ci), db::Trans (ref)));
    db::ICplxTrans to = db::ICplxTrans (db::Trans (-ref));

    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

      for (edt::Service::objects::const_iterator r = (*es)->selection ().begin (); r != (*es)->selection ().end (); ++r) {

        if (r->is_cell_inst ()) {

          db::Instance new_inst = target_cell.insert (r->back ().inst_ptr);
          target_cell.transform (new_inst, to * db::ICplxTrans (r->trans ()));

        } else {

          db::Shapes &target_shapes = target_cell.shapes (r->layer ());
          db::Shape new_shape = target_shapes.insert (r->shape ());
          target_shapes.transform (new_shape, to * db::ICplxTrans (r->trans ()));

        }

      }

      //  delete all the objects currently selected and set the new selection
      (*es)->del_selected ();

      //  establish the new instance as selection for the instance service
      std::vector<lay::ObjectInstPath> new_selection;
      if ((*es)->flags () == db::ShapeIterator::Nothing) {
        new_selection.push_back (lay::ObjectInstPath ());
        new_selection.back ().set_topcell (cv.cell_index ());
        new_selection.back ().set_cv_index (cv_index);
        new_selection.back ().add_path (db::InstElement (target_cell_inst));
      }
      (*es)->set_selection (new_selection.begin (), new_selection.end ());

    }

    if (manager ()) {
      manager ()->commit ();
    }

  }

}

void
MainService::cm_convert_to_cell ()
{
  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  view ()->cancel_edits ();

  try {

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Convert to static cell")));
    }

    std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

    std::set<db::Layout *> needs_cleanup;

    //  Do the conversion
    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

      for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

        const lay::CellView &cv = view ()->cellview (s->cv_index ());
        db::cell_index_type ci = s->cell_index_tot ();
        db::cell_index_type parent_ci = s->cell_index ();

        if (cv.is_valid () && s->is_cell_inst () && cv->layout ().cell (ci).is_proxy ()) {

          db::Cell &parent = cv->layout ().cell (parent_ci);
          if (parent.is_valid (s->back ().inst_ptr)) {

            //  convert the cell to static and replace the instances with the new cell
            db::cell_index_type new_ci = cv->layout ().convert_cell_to_static (ci);
            if (new_ci != ci) {

              db::CellInstArray na = s->back ().inst_ptr.cell_inst ();
              na.object ().cell_index (new_ci);
              parent.replace (s->back ().inst_ptr, na);

              needs_cleanup.insert (&cv->layout ());

            }

          }

        }

      }

    }

    if (needs_cleanup.empty ()) {
      throw tl::Exception (tl::to_string (tr ("No instance of a PCell or library cell selected - nothing to convert")));
    }

    //  clean up the layouts that need to do so.
    for (std::set<db::Layout *>::const_iterator l = needs_cleanup.begin (); l != needs_cleanup.end (); ++l) {
      (*l)->cleanup ();
    }

    //  The selection might no longer be valid
    view ()->clear_selection ();

    if (manager ()) {
      manager ()->commit ();
    }

  } catch (...) {
    if (manager ()) {
      manager ()->commit ();
    }
    throw;
  }
}

void
MainService::cm_convert_to_pcell ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  //  check whether the selection contains instances and reject it in that case
  size_t num_selected = 0;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    num_selected += (*es)->selection ().size ();
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      if (s->is_cell_inst ()) {
        throw tl::Exception (tl::to_string (tr ("Selection contains instances - they cannot be converted to PCells.")));
      }
    }
  }

  //  Collected items
  std::vector<std::string> pcell_items;

  //  Collect the libraries and PCells within these libraries that are candidates here
  std::vector<std::pair<db::Library *, db::pcell_id_type> > pcells;
  for (db::LibraryManager::iterator l = db::LibraryManager::instance ().begin (); l != db::LibraryManager::instance ().end (); ++l) {

    db::Library *lib = db::LibraryManager::instance ().lib (l->second);
    for (db::Layout::pcell_iterator pc = lib->layout ().begin_pcells (); pc != lib->layout ().end_pcells (); ++pc) {

      try {

        const db::PCellDeclaration *pc_decl = lib->layout ().pcell_declaration (pc->second);
        size_t n = 1000; // 1000 tries max.
        for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); n > 0 && pc_decl && es != edt_services.end (); ++es) {
          for (edt::Service::obj_iterator s = (*es)->selection ().begin (); n > 0 && pc_decl && s != (*es)->selection ().end (); ++s) {
            const lay::CellView &cv = view ()->cellview (s->cv_index ());
            if (pc_decl->can_create_from_shape (cv->layout (), s->shape (), s->layer ())) {
              --n;
            } else {
              pc_decl = 0; // stop
            }
          }
        }

        //  We have positive hit
        if (pc_decl) {
          pcells.push_back (std::make_pair (lib, pc->second));
          pcell_items.push_back (lib->get_name () + "." + pc_decl->name ());
        }

      } catch (...) {
        //  ignore errors in can_create_from_shape
      }

    }

  }

  if (pcell_items.empty ()) {
    throw tl::Exception (tl::to_string (tr ("No PCell found that accepts the selected shapes for conversion.")));
  }

  int index = 0;

#if defined(HAVE_QT)
  //  TODO: In Qt-less case keep selection persistent so we can set it externally
  QStringList items;
  for (auto i = pcell_items.begin (); i != pcell_items.end (); ++i) {
    items.push_back (tl::to_qstring (*i));
  }

  bool ok = false;
  QString item = QInputDialog::getItem (lay::widget_from_view (view ()),
                                        tr ("Select Target PCell"),
                                        tr ("Select the PCell the shape should be converted into"),
                                        items, 0, false, &ok);
  if (! ok) {
    return;
  }

  index = items.indexOf (item);
  if (index < 0) {
    return;
  }
#endif

  db::Library *lib = pcells [index].first;
  db::pcell_id_type pcid = pcells [index].second;
  const db::PCellDeclaration *pcell_decl = lib->layout ().pcell_declaration (pcid);
  tl_assert (pcell_decl != 0);

  view ()->cancel_edits ();

  try {

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Convert to PCell")));
    }

    std::vector<edt::Service::obj_iterator> to_delete;
    std::vector<lay::ObjectInstPath> new_selection;

    bool any_non_converted = false;
    bool any_converted = false;

    {
      tl::RelativeProgress progress (tl::to_string (tr ("Convert to PCell")), num_selected, 1000);

      //  convert the shapes which can be converted
      for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

        for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

          const lay::CellView &cv = view ()->cellview (s->cv_index ());

          if (! s->is_cell_inst ()) {

            if (pcell_decl->can_create_from_shape (cv->layout (), s->shape (), s->layer ())) {

              db::pcell_parameters_type parameters = pcell_decl->parameters_from_shape (cv->layout (), s->shape (), s->layer ());
              db::Trans trans = pcell_decl->transformation_from_shape (cv->layout (), s->shape (), s->layer ());

              pcell_decl->coerce_parameters (cv->layout (), parameters);
              db::cell_index_type pcell_cid = lib->layout ().get_pcell_variant (pcid, parameters);
              db::cell_index_type cid = cv->layout ().get_lib_proxy (lib, pcell_cid);

              db::Instance cell_inst = cv.cell ()->insert (db::CellInstArray (db::CellInst (cid), trans));

              //  add the new cell to the selection
              new_selection.push_back (lay::ObjectInstPath ());
              new_selection.back ().set_topcell (cv.cell_index ());
              new_selection.back ().set_cv_index (s->cv_index ());
              new_selection.back ().add_path (db::InstElement (cell_inst));

              //  mark the shape for delete (later)
              to_delete.push_back (s);

              any_converted = true;

            } else {
              any_non_converted = true;
            }

            ++progress;

          }

        }

      }

    }

    if (! any_converted) {
      throw tl::Exception (tl::to_string (tr ("None of the shapes could be converted to the desired PCell")));
    }

    //  Delete the shapes which have been converted
    for (std::vector<edt::Service::obj_iterator>::const_iterator td = to_delete.begin (); td != to_delete.end (); ++td) {
      db::Cell &cell = view ()->cellview ((*td)->cv_index ())->layout ().cell ((*td)->cell_index ());
      if (cell.shapes ((*td)->layer ()).is_valid ((*td)->shape ())) {
        cell.shapes ((*td)->layer ()).erase_shape ((*td)->shape ());
      }
    }

    //  The selection is no longer valid
    view ()->clear_selection ();

    //  Establish the new instance as selection for the instance service
    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
      if ((*es)->flags () == db::ShapeIterator::Nothing) {
        (*es)->set_selection (new_selection.begin (), new_selection.end ());
        break;
      }
    }

    if (any_non_converted) {
      tl::warn << tl::to_string (tr ("Some of the shapes could not be converted to the desired PCell"));
#if defined(HAVE_QT)
      QMessageBox::warning (lay::widget_from_view (view ()), tr ("Warning"), tr ("Some of the shapes could not be converted to the desired PCell"));
#endif
    }

    if (manager ()) {
      manager ()->commit ();
    }

  } catch (...) {
    if (manager ()) {
      manager ()->commit ();
    }
    throw;
  }
}

void MainService::cm_area_perimeter ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  double dbu = 0.0;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  db::Region region;

  //  get (common) cellview index of the primary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (s->is_cell_inst ()) {
        continue;
      }

      db::Polygon poly;
      if (!s->shape ().polygon (poly)) {
        continue;
      }

      double shape_dbu = view ()->cellview (s->cv_index ())->layout ().dbu ();

      if (dbu == 0.0) {
        //  first CV is used for reference DBU
        dbu = shape_dbu;
      }

      if (fabs (shape_dbu - dbu) < db::epsilon) {
        region.insert (s->trans () * poly);
      } else {
        region.insert ((db::ICplxTrans (shape_dbu / dbu) * s->trans ()) * poly);
      }

    }

  }

#if defined(HAVE_QT)
  if (region.count () > 100000) {
    if (QMessageBox::warning (lay::widget_from_view (view ()), tr ("Warning: Big Selection"),
                                    tr ("The selection contains many shapes. Area and perimeter computation may take a long time.\nContinue anyway?"),
                                    QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
      return;
    }
  }
#endif

  double area = region.area () * dbu * dbu;
  double perimeter = region.perimeter () * dbu;

#if defined(HAVE_QT)
  area_and_perimeter_dialog ()->exec_dialog (area, perimeter);
#endif
}

static bool extract_rad (std::vector <db::Polygon> &poly, double &rinner, double &router, unsigned int &n)
{
  std::vector <db::Point> new_pts;
  bool any_extracted = false;

  for (std::vector<db::Polygon>::iterator p = poly.begin (); p != poly.end (); ++p) {

    db::Polygon new_poly;

    new_pts.clear ();
    if (! extract_rad_from_contour (p->begin_hull (), p->end_hull (), rinner, router, n, &new_pts) &&
        ! extract_rad_from_contour (p->begin_hull (), p->end_hull (), rinner, router, n, &new_pts, true)) {
      //  ultimate fallback: assign original contour
      new_poly.assign_hull (p->begin_hull (), p->end_hull (), false /*don't compress*/);
    } else {
      new_poly.assign_hull (new_pts.begin (), new_pts.end (), true /*compress*/);
      any_extracted = true;
    }

    for (unsigned int h = 0; h < p->holes (); ++h) {

      new_pts.clear ();
      if (! extract_rad_from_contour (p->begin_hole (h), p->end_hole (h), rinner, router, n, &new_pts) &&
          ! extract_rad_from_contour (p->begin_hole (h), p->end_hole (h), rinner, router, n, &new_pts, true)) {
        //  ultimate fallback: assign original contour
        new_poly.insert_hole (p->begin_hole (h), p->end_hole (h), false /*don't compress*/);
      } else {
        new_poly.insert_hole (new_pts.begin (), new_pts.end (), true /*compress*/);
        any_extracted = true;
      }

    }

    p->swap (new_poly);

  }

  return any_extracted;
}

void
MainService::cm_round_corners ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  int cv_index = -1;
  int layer_index = -1;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  std::vector<db::Polygon> primary;

  //  get (common) cellview index of the primary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {

        if (cv_index >= 0 && cv_index != int (s->cv_index ())) {
          throw tl::Exception (tl::to_string (tr ("Selection originates from different layouts - cannot compute result in this case.")));
        }
        cv_index = int (s->cv_index ());

        if (layer_index >= 0 && layer_index != int (s->layer ())) {
          throw tl::Exception (tl::to_string (tr ("Selection originates from different layers - cannot compute result in this case.")));
        }
        layer_index = int (s->layer ());

        primary.push_back (db::Polygon ());
        s->shape ().polygon (primary.back ());
        primary.back ().transform (s->trans ());

      }

    }
  }

  if (cv_index < 0 || layer_index < 0) {
    throw tl::Exception (tl::to_string (tr ("Selection does not contain polygons")));
  }

  //  prepare: merge to remove cutlines and smooth to remove effects of cutlines
  db::EdgeProcessor ep;
  std::vector <db::Polygon> in;
  ep.merge (primary, in, 0 /*min_wc*/, false /*resolve holes*/, true /*min coherence*/);
  for (std::vector <db::Polygon>::iterator p = in.begin (); p != in.end (); ++p) {
    *p = smooth (*p, 1, true);
  }

  std::vector <db::Polygon> out = in;

  unsigned int n = 100;
  double rinner = 0.0, router = 0.0;
  bool has_extracted = extract_rad (out, rinner, router, n);

  const lay::CellView &cv = view ()->cellview (cv_index);
  double dbu = cv->layout ().dbu ();

  rinner *= dbu;
  router *= dbu;

#if defined(HAVE_QT)
  //  TODO: make parameters persistent so we can set them externally
  if (! round_corners_dialog ()->exec_dialog (cv->layout (), m_router, m_rinner, m_npoints, m_undo_before_apply, router, rinner, n, has_extracted)) {
    return;
  }
#endif

  if (! m_undo_before_apply || ! has_extracted) {
    out.swap (in);
  }

  for (std::vector <db::Polygon>::iterator p = out.begin (); p != out.end (); ++p) {
    *p = compute_rounded (*p, m_rinner / dbu, m_router / dbu, m_npoints);
  }

  //  remove holes (result in primary)
  primary.clear ();
  ep.merge (out, primary, 0 /*min_wc*/, true /*resolve holes*/, true /*min coherence*/);

  view ()->cancel_edits ();
  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Corner rounding operation on selection")));
  }

  //  Delete the current selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      if (! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {
        db::Cell &cell = view ()->cellview (s->cv_index ())->layout ().cell (s->cell_index ());
        if (cell.shapes (s->layer ()).is_valid (s->shape ())) {
          cell.shapes (s->layer ()).erase_shape (s->shape ());
        }
      }
    }
  }

  //  The selection is no longer valid
  view ()->clear_selection ();

  //  Insert the new shapes on top level
  std::vector<lay::ObjectInstPath> new_selection;
  new_selection.reserve (primary.size ());

  //  create the new shapes 
  db::Shapes &target_shapes = cv->layout ().cell (cv.cell_index ()).shapes (layer_index);
  for (std::vector <db::Polygon>::const_iterator p = primary.begin (); p != primary.end (); ++p) {
    db::Shape new_shape = target_shapes.insert (*p);
    new_selection.push_back (lay::ObjectInstPath ());
    new_selection.back ().set_topcell (cv.cell_index ());
    new_selection.back ().set_cv_index (cv_index);
    new_selection.back ().set_layer (layer_index);
    new_selection.back ().set_shape (new_shape);
  }
  
  //  set the new selection on the polygon service (because now we have polygons)
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    if (dynamic_cast <edt::PolygonService *> (*es) != 0) {
      (*es)->set_selection (new_selection.begin (), new_selection.end ());
      break;
    }
  }

  if (manager ()) {
    manager ()->commit ();
  }
}

void
MainService::cm_size ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  int cv_index = -1;
  int layer_index = -1;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  std::vector<db::Polygon> primary;

  //  get (common) cellview index of the primary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {

        if (cv_index >= 0 && cv_index != int (s->cv_index ())) {
          throw tl::Exception (tl::to_string (tr ("Selection originates from different layouts - cannot compute result in this case.")));
        }
        cv_index = int (s->cv_index ());

        if (layer_index >= 0 && layer_index != int (s->layer ())) {
          throw tl::Exception (tl::to_string (tr ("Selection originates from different layers - cannot compute result in this case.")));
        }
        layer_index = int (s->layer ());

        primary.push_back (db::Polygon ());
        s->shape ().polygon (primary.back ());
        primary.back ().transform (s->trans ());

      }

    }
  }

  if (cv_index < 0 || layer_index < 0) {
    throw tl::Exception (tl::to_string (tr ("Selection does not contain polygons")));
  }

  std::string sl ("0.0");

#if defined(HAVE_QT)
  //  TODO: keep the value persistent so we can set it externally in the Qt-less case
  bool ok = false;
  QString s = QInputDialog::getText (lay::widget_from_view (view ()),
                                     tr ("Sizing"),
                                     tr ("Sizing (in micron, positive or negative). Two values (dx, dy) for anisotropic sizing."),
                                     QLineEdit::Normal, QString::fromUtf8 ("0.0"), 
                                     &ok);

  if (!ok) {
    return;
  }

  sl = tl::to_string (s);
#endif

  double dx = 0.0, dy = 0.0;
  tl::Extractor ex (sl.c_str ());
  ex.read (dx);
  if (ex.test (",")) {
    ex.read (dy);
  } else {
    dy = dx;
  }

  const lay::CellView &cv = view ()->cellview (cv_index);
  double dbu = cv->layout ().dbu ();
  db::Coord idx = db::coord_traits<db::Coord>::rounded (dx / dbu);
  db::Coord idy = db::coord_traits<db::Coord>::rounded (dy / dbu);

  std::vector <db::Polygon> out;
  db::EdgeProcessor ep;
  ep.size (primary, idx, idy, out, 2 /*mode, TODO: make variable*/, true /*resolve holes*/, true /*min coherence*/);

  view ()->cancel_edits ();
  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Sizing operation on selection")));
  }

  //  Delete the current selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      if (! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {
        db::Cell &cell = view ()->cellview (s->cv_index ())->layout ().cell (s->cell_index ());
        if (cell.shapes (s->layer ()).is_valid (s->shape ())) {
          cell.shapes (s->layer ()).erase_shape (s->shape ());
        }
      }
    }
  }

  //  The selection is no longer valid
  view ()->clear_selection ();

  //  Insert the new shapes on top level
  std::vector<lay::ObjectInstPath> new_selection;
  new_selection.reserve (out.size ());

  //  create the new shapes 
  db::Shapes &target_shapes = cv->layout ().cell (cv.cell_index ()).shapes (layer_index);
  for (std::vector <db::Polygon>::const_iterator p = out.begin (); p != out.end (); ++p) {
    db::Shape new_shape = target_shapes.insert (*p);
    new_selection.push_back (lay::ObjectInstPath ());
    new_selection.back ().set_topcell (cv.cell_index ());
    new_selection.back ().set_cv_index (cv_index);
    new_selection.back ().set_layer (layer_index);
    new_selection.back ().set_shape (new_shape);
  }
  
  //  set the new selection on the polygon service (because now we have polygons)
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    if (dynamic_cast <edt::PolygonService *> (*es) != 0) {
      (*es)->set_selection (new_selection.begin (), new_selection.end ());
      break;
    }
  }

  if (manager ()) {
    manager ()->commit ();
  }
}

void
MainService::boolean_op (int mode)
{
  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  int cv_index = -1;
  int layer_index = -1;

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  std::vector<db::Polygon> primary;

  //  get (common) cellview index of the primary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (s->seq () == 0 && ! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {

        if (cv_index >= 0 && cv_index != int (s->cv_index ())) {
          throw tl::Exception (tl::to_string (tr ("Primary selection originates from different layouts - cannot compute result in this case.")));
        }
        cv_index = int (s->cv_index ());

        if (layer_index >= 0 && layer_index != int (s->layer ())) {
          throw tl::Exception (tl::to_string (tr ("Primary selection originates from different layers - cannot compute result in this case.")));
        }
        layer_index = int (s->layer ());

        primary.push_back (db::Polygon ());
        s->shape ().polygon (primary.back ());
        primary.back ().transform (s->trans ());

      }

    }
  }

  if (cv_index < 0 || layer_index < 0) {
    throw tl::Exception (tl::to_string (tr ("Primary selection does not contain polygons")));
  }

  std::vector<db::Polygon> secondary;

  const lay::CellView &cv = view ()->cellview (cv_index);
  double dbu_prim = cv->layout ().dbu ();

  //  get the secondary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (s->seq () > 0 && ! s->is_cell_inst () && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {

        double dbu_sec = view ()->cellview (s->cv_index ())->layout ().dbu ();

        secondary.push_back (db::Polygon ());
        s->shape ().polygon (secondary.back ());

        //  HINT: this transforms the shape without any grid snapping precautions ..
        secondary.back ().transform (db::CplxTrans (dbu_sec / dbu_prim) * s->trans ());

      }

    }
  }

  std::vector <db::Polygon> out;
  db::EdgeProcessor ep;

  if (mode == -1 /*== separate*/) {
    ep.boolean (primary, secondary, out, db::BooleanOp::And);
    ep.boolean (primary, secondary, out, db::BooleanOp::ANotB);
  } else {
    ep.boolean (primary, secondary, out, mode);
  }

  view ()->cancel_edits ();
  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Boolean operation on selection")));
  }

  //  Delete the current selection
  //  NOTE: we delete only those shapes from the primary layer and keep shapes from other layers.
  //  Let's see whether this heuristics is more accepted.

  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      if (! s->is_cell_inst () && int (s->layer ()) == layer_index && (s->shape ().is_polygon () || s->shape ().is_path () || s->shape ().is_box ())) {
        db::Cell &cell = view ()->cellview (s->cv_index ())->layout ().cell (s->cell_index ());
        if (cell.shapes (s->layer ()).is_valid (s->shape ())) {
          cell.shapes (s->layer ()).erase_shape (s->shape ());
        }
      }
    }
  }

  //  The selection is no longer valid
  view ()->clear_selection ();

  //  Insert the new shapes on top level
  std::vector<lay::ObjectInstPath> new_selection;
  new_selection.reserve (out.size ());

  //  create the new shapes 
  db::Shapes &target_shapes = cv->layout ().cell (cv.cell_index ()).shapes (layer_index);
  for (std::vector <db::Polygon>::const_iterator p = out.begin (); p != out.end (); ++p) {
    db::Shape new_shape = target_shapes.insert (*p);
    new_selection.push_back (lay::ObjectInstPath ());
    new_selection.back ().set_topcell (cv.cell_index ());
    new_selection.back ().set_cv_index (cv_index);
    new_selection.back ().set_layer (layer_index);
    new_selection.back ().set_shape (new_shape);
  }
  
  //  set the new selection on the polygon service (because now we have polygons)
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    if (dynamic_cast <edt::PolygonService *> (*es) != 0) {
      (*es)->set_selection (new_selection.begin (), new_selection.end ());
      break;
    }
  }

  if (manager ()) {
    manager ()->commit ();
  }
}

void 
MainService::cm_union ()
{
  boolean_op (db::BooleanOp::Or);
}

void 
MainService::cm_intersection ()
{
  boolean_op (db::BooleanOp::And);
}

void
MainService::cm_difference ()
{
  boolean_op (db::BooleanOp::ANotB);
}

void
MainService::cm_separate ()
{
  boolean_op (-1); // == separate (And + ANotB)
}

static
db::DVector compute_alignment_vector (const db::DBox &prim_box, const db::DBox &box, int hmode, int vmode)
{
  double dx = 0.0;
  if (hmode == 1) {
    dx = prim_box.left () - box.left ();
  } else if (hmode == 2) {
    dx = prim_box.center ().x () - box.center ().x ();
  } else if (hmode == 3) {
    dx = prim_box.right () - box.right ();
  }

  double dy = 0.0;
  if (vmode == 1) {
    dy = prim_box.top () - box.top ();
  } else if (vmode == 2) {
    dy = prim_box.center ().y () - box.center ().y ();
  } else if (vmode == 3) {
    dy = prim_box.bottom () - box.bottom ();
  }

  return db::DVector (dx, dy);
}

static db::DBox 
inst_bbox (const db::CplxTrans &tr, lay::LayoutViewBase *view, int cv_index, const db::InstElement &inst_element, bool visible_only)
{
  db::DBox box;

  const db::Layout &layout = view->cellview (cv_index)->layout ();

  if (visible_only) {
    for (lay::LayerPropertiesConstIterator l = view->begin_layers (); ! l.at_end (); ++l) {
      if (l->cellview_index () == cv_index && l->visible (true)) {
        db::box_convert<db::CellInst> bc (layout, l->layer_index ());
        box += tr * inst_element.bbox (bc);
      }
    }
  } else {
    db::box_convert<db::CellInst> bc (layout);
    box += tr * inst_element.bbox (bc);
  }

  return box;
}

void 
MainService::cm_align ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

#if defined(HAVE_QT)
  //  TODO: make parameters persistent so we can set them externally
  if (! align_options_dialog ()->exec_dialog (m_align_hmode, m_align_vmode, m_align_visible_layers)) {
    return;
  }
#endif

  db::DBox prim_box;
  bool has_secondary = false;

  //  get (common) bbox index of the primary selection
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      if (s->seq () == 0) {

        const db::Layout &layout = view ()->cellview (s->cv_index ())->layout ();
        db::CplxTrans tr = db::CplxTrans (layout.dbu ()) * s->trans ();

        if (! s->is_cell_inst ()) {
          prim_box += tr * s->shape ().bbox ();
        } else {
          prim_box += inst_bbox (tr, view (), s->cv_index (), s->back (), m_align_visible_layers);
        }

      } else {
        has_secondary = true;
      }

    }
  }

  if (! prim_box.empty ()) {

    view ()->cancel_edits ();
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Alignment")));
    }

    //  do the alignment
    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

      //  create a transformation vector that describes each shape's transformation
      std::vector <db::DCplxTrans> tv;
      tv.reserve ((*es)->selection ().size ());

      for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

        db::DVector v;

        if (s->seq () > 0 || !has_secondary) {

          db::Layout &layout = view ()->cellview (s->cv_index ())->layout ();
          db::CplxTrans tr = db::CplxTrans (layout.dbu ()) * s->trans ();

          if (! s->is_cell_inst ()) {

            db::DBox box = tr * s->shape ().bbox ();
            v = compute_alignment_vector (prim_box, box, m_align_hmode, m_align_vmode);

          } else {

            db::DBox box = inst_bbox (tr, view (), s->cv_index (), s->back (), m_align_visible_layers);
            v = compute_alignment_vector (prim_box, box, m_align_hmode, m_align_vmode);

          }

        }

        tv.push_back (db::DCplxTrans (db::DTrans (v)));

      }

      //  use the "transform" method to transform the shapes and instances (with individual transformations)
      (*es)->transform (db::DCplxTrans () /*dummy*/, &tv);

    }

    if (manager ()) {
      manager ()->commit ();
    }

  }
}

void
MainService::cm_distribute ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

#if defined(HAVE_QT)
  //  TODO: make parameters persistent so we can set them externally
  if (! distribute_options_dialog ()->exec_dialog (m_hdistribute, m_distribute_hmode, m_distribute_hpitch, m_distribute_hspace,
                                                   m_vdistribute, m_distribute_vmode, m_distribute_vpitch, m_distribute_vspace,
                                                   m_distribute_visible_layers)) {
    return;
  }
#endif

  if (! m_hdistribute && ! m_vdistribute) {
    return;
  }

  //  count the items
  size_t n = 0;
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      ++n;
    }
  }

  std::vector<std::pair<size_t, size_t> > objects_for_service;
  std::vector<db::DCplxTrans> transformations;

  {

    std::vector<db::DBox> org_boxes;
    org_boxes.reserve (n);

    edt::distributed_placer<db::DBox, size_t> placer;
    placer.reserve (n);

    size_t i = 0;

    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

      objects_for_service.push_back (std::make_pair (i, i));

      for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

        const db::Layout &layout = view ()->cellview (s->cv_index ())->layout ();
        db::CplxTrans tr = db::CplxTrans (layout.dbu ()) * s->trans ();

        db::DBox box;
        if (! s->is_cell_inst ()) {
          box = tr * s->shape ().bbox ();
        } else {
          box = inst_bbox (tr, view (), s->cv_index (), s->back (), m_distribute_visible_layers);
        }

        org_boxes.push_back (box);
        placer.insert (box, i);

        ++i;

      }

      objects_for_service.back ().second = i;

    }

    int href = int (m_distribute_hmode - 2);
    int vref = 2 - int (m_distribute_vmode);

    if (m_hdistribute && m_vdistribute) {
      placer.distribute_matrix (href, m_distribute_hpitch, m_distribute_hspace,
                                vref, m_distribute_vpitch, m_distribute_vspace);
    } else if (m_hdistribute) {
      placer.distribute_h (href, vref, m_distribute_hpitch, m_distribute_hspace);
    } else if (m_vdistribute) {
      placer.distribute_v (vref, href, m_distribute_vpitch, m_distribute_vspace);
    }

    transformations.resize (org_boxes.size ());

    for (edt::distributed_placer<db::DBox, size_t>::iterator i = placer.begin (); i != placer.end (); ++i) {
      transformations[i->second] = db::DCplxTrans (i->first.p1 () - org_boxes[i->second].p1 ());
    }

  }

  {
    view ()->cancel_edits ();
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Distribution")));
    }

    //  do the distribution
    for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

      size_t ie = es - edt_services.begin ();

      //  create a transformation vector that describes each shape's transformation
      std::vector <db::DCplxTrans> tv (transformations.begin () + objects_for_service [ie].first, transformations.begin () + objects_for_service [ie].second);

      //  use the "transform" method to transform the shapes and instances (with individual transformations)
      (*es)->transform (db::DCplxTrans () /*dummy*/, &tv);

    }

    if (manager ()) {
      manager ()->commit ();
    }

  }
}

void
MainService::cm_make_array ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

  size_t n = 0;
  check_no_guiding_shapes ();

  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();

  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      ++n;
    }
  }

  if (n == 0) {
    throw tl::Exception (tl::to_string (tr ("Nothing selected to make arrays of")));
  }

#if defined(HAVE_QT)
  //  TODO: make parameters persistent so we can set them externally
  if (! make_array_options_dialog ()->exec_dialog (m_array_a, m_array_na, m_array_b, m_array_nb)) {
    return;
  }
#endif

  view ()->cancel_edits ();

  //  undo support for small arrays only
  bool has_undo = (m_array_na * m_array_nb < 1000);

  //  No undo support currently - the undo buffering is pretty inefficient right now.
  if (manager ()) {
    if (! has_undo) {
      manager ()->clear ();
    } else {
      manager ()->transaction (tl::to_string (tr ("Make array")));
    }
  }

  tl::RelativeProgress progress (tl::to_string (tr ("Make array")), (size_t (m_array_na) * size_t (m_array_nb) - 1) * n, 1000);

  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {

    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {

      const lay::CellView &cv = view ()->cellview (s->cv_index ());
      if (! cv.is_valid ()) {
        continue;
      }

      db::Cell &target_cell = cv->layout ().cell (s->cell_index ());

      if (s->is_cell_inst ()) {

        for (unsigned int ia = 0; ia < m_array_na; ++ia) {
          for (unsigned int ib = 0; ib < m_array_nb; ++ib) {

            //  don't create a copy
            if (ia == 0 && ib == 0) {
              continue;
            }

            db::DCplxTrans dtrans (m_array_a * double (ia) + m_array_b * double (ib));
            db::ICplxTrans itrans (db::DCplxTrans (s->trans ()).inverted () * db::DCplxTrans (1.0 / cv->layout ().dbu ()) * dtrans * db::DCplxTrans (cv->layout ().dbu ()) * db::DCplxTrans (s->trans ()));

            db::Instance new_inst = target_cell.insert (s->back ().inst_ptr);
            target_cell.transform (new_inst, itrans);

            ++progress;

          }

        }

      } else {

        db::Shapes &target_shapes = target_cell.shapes (s->layer ());

        for (unsigned int ia = 0; ia < m_array_na; ++ia) {
          for (unsigned int ib = 0; ib < m_array_nb; ++ib) {

            //  don't create a copy
            if (ia == 0 && ib == 0) {
              continue;
            }

            db::DCplxTrans dtrans (m_array_a * double (ia) + m_array_b * double (ib));
            db::ICplxTrans itrans (db::DCplxTrans (s->trans ()).inverted () * db::DCplxTrans (1.0 / cv->layout ().dbu ()) * dtrans * db::DCplxTrans (cv->layout ().dbu ()) * db::DCplxTrans (s->trans ()));

            db::Shape new_shape = target_shapes.insert (s->shape ());
            target_shapes.transform (new_shape, itrans);

            ++progress;

          }
        }

      }

    }

  }

  if (has_undo && manager ()) {
    manager ()->commit ();
  }
}

void 
MainService::cm_tap ()
{
#if ! defined(HAVE_QT)
  tl_assert (false); // see TODO
#endif

#if defined(HAVE_QT)
  QWidget *view_widget = lay::widget_from_view (view ());
  if (! view_widget) {
    return;
  }
#endif

  if (! view ()->canvas ()->mouse_in_window ()) {
    return;
  }

  lay::ShapeFinder finder (true /*point mode*/, false /*all hierarchy levels*/, db::ShapeIterator::All, 0);

  //  capture all objects in point mode (default: capture one only)
  finder.set_catch_all (true);

  //  go through all visible layers of all cellviews
  db::DPoint pt = view ()->canvas ()->mouse_position_um ();
  finder.find (view (), db::DBox (pt, pt));

  std::set<std::pair<unsigned int, unsigned int> > layers_in_selection;

  for (lay::ShapeFinder::iterator f = finder.begin (); f != finder.end (); ++f) {
    //  ignore guiding shapes
    if (f->layer () != view ()->cellview (f->cv_index ())->layout ().guiding_shape_layer ()) {
      layers_in_selection.insert (std::make_pair (f->cv_index (), f->layer ()));
    }
  }

  std::vector<lay::LayerPropertiesConstIterator> tapped_layers;
  for (lay::LayerPropertiesConstIterator lp = view ()->begin_layers (view ()->current_layer_list ()); ! lp.at_end (); ++lp) {
    const lay::LayerPropertiesNode *ln = lp.operator-> ();
    if (layers_in_selection.find (std::make_pair ((unsigned int) ln->cellview_index (), (unsigned int) ln->layer_index ())) != layers_in_selection.end ()) {
      tapped_layers.push_back (lp);
    }
  }

  if (tapped_layers.empty ()) {
    return;
  }

  //  List the layers under the cursor as pop up a menu

#if defined(HAVE_QT)
  //  TODO: what to do here in Qt-less case? Store results in configuration so they can be retrieved externally?

#if QT_VERSION >= 0x050000
  double dpr = view_widget->devicePixelRatio ();
#else
  double dpr = 1.0;
#endif

  std::unique_ptr<QMenu> menu (new QMenu (view_widget));
  menu->show ();

  int icon_size = menu->style ()->pixelMetric (QStyle::PM_ButtonIconSize);

  db::DPoint mp_local = view ()->canvas ()->mouse_position ();
  QPoint mp = view ()->canvas ()->widget ()->mapToGlobal (QPoint (mp_local.x (), mp_local.y ()));

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = tapped_layers.begin (); l != tapped_layers.end (); ++l) {
    QAction *a = menu->addAction (lay::LayerTreeModel::icon_for_layer (*l, view (), icon_size, icon_size, dpr, 0, true), tl::to_qstring ((*l)->display_string (view (), true, true /*with source*/)));
    a->setData (int (l - tapped_layers.begin ()));
  }

  QAction *action = menu->exec (mp);
  if (action) {

    int index = action->data ().toInt ();
    lay::LayerPropertiesConstIterator iter = tapped_layers [index];
    view ()->set_current_layer (iter);

    edt::Service *es = dynamic_cast<edt::Service *> (view ()->canvas ()->active_service ());
    if (es) {
      es->tap (pt);
    }

  }
#endif
}

void 
MainService::cm_change_layer ()
{
  tl_assert (view ()->is_editable ());
  check_no_guiding_shapes ();

  int cv_index = -1;

  //  get (common) cellview index of the selected shapes
  for (SelectionIterator s (view ()); ! s.at_end (); ++s) {
    if (cv_index >= 0 && cv_index != int (s->cv_index ())) {
      throw tl::Exception (tl::to_string (tr ("Selections originate from different layouts - cannot switch layer in this case.")));
    }
    cv_index = int (s->cv_index ());
  }

  if (cv_index >= 0) {

    //  HINT: the following code is copied from ShapeEditService::get_edit_layer - it should
    //  be at some common place

    lay::LayerPropertiesConstIterator cl = view ()->current_layer ();
    if (cl.is_null ()) {
      throw tl::Exception (tl::to_string (tr ("Please select a layer first")).c_str ());
    }
    
    if (cv_index != cl->cellview_index ()) {
      throw tl::Exception (tl::to_string (tr ("Shapes cannot be moved to a different layout")).c_str ());
    }

    const lay::CellView &cv = view ()->cellview (cv_index);
    int layer = cl->layer_index ();

    if (! cv.is_valid ()) {
      throw tl::Exception (tl::to_string (tr ("Please select a cell first")).c_str ());
    }

    if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

      if (cl->has_children ()) {
        throw tl::Exception (tl::to_string (tr ("Please select a valid drawing layer first")).c_str ());
      } else {

        //  create this layer now
        const lay::ParsedLayerSource &source = cl->source (true /*real*/);

        db::LayerProperties db_lp;
        if (source.has_name ()) {
          db_lp.name = source.name ();
        }
        db_lp.layer = source.layer ();
        db_lp.datatype = source.datatype ();

        cv->layout ().insert_layer (db_lp);

        //  update the layer index inside the layer view
        cl->realize_source ();
          
        //  Hint: we could have taken the new index from insert_layer, but this 
        //  is a nice test:
        layer = cl->layer_index ();
        tl_assert (layer >= 0);

      }

    }

    view ()->cancel_edits ();

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Change layer")));
    }

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    //  Insert and delete the shape. This exploits the fact, that a shape can be erased multiple times -
    //  this is important since the selection potentially contains the same shape multiple times.
    for (SelectionIterator s (view ()); ! s.at_end (); ++s) {

      if (!s->is_cell_inst () && int (s->layer ()) != layer) {

        db::Cell &cell = layout.cell (s->cell_index ());
        if (cell.shapes (s->layer ()).is_valid (s->shape ())) {
          cell.shapes (layer).insert (s->shape ());
          cell.shapes (s->layer ()).erase_shape (s->shape ());
        }

      } else if (s->is_cell_inst ()) {

        //  If the selected object is a PCell instance, and there is exactly one visible, writable layer type parameter, change this one

        db::Instance inst = s->back ().inst_ptr;
        db::Cell &cell = layout.cell (s->cell_index ());

        if (cell.is_valid (inst)) {

          const db::PCellDeclaration *pcell_decl = layout.pcell_declaration_for_pcell_variant (inst.cell_index ());
          if (pcell_decl) {

            size_t layer_par_index = 0;
            int n_layer_par = 0;
            for (std::vector<db::PCellParameterDeclaration>::const_iterator d = pcell_decl->parameter_declarations ().begin (); d != pcell_decl->parameter_declarations ().end () && n_layer_par < 2; ++d) {
              if (d->get_type () == db::PCellParameterDeclaration::t_layer && !d->is_hidden () && !d->is_readonly ()) {
                ++n_layer_par;
                layer_par_index = size_t (d - pcell_decl->parameter_declarations ().begin ());
              }
            }

            if (n_layer_par == 1) {
              std::vector<tl::Variant> parameters = cell.get_pcell_parameters (inst);
              tl_assert (layer_par_index < parameters.size ());
              parameters [layer_par_index] = layout.get_properties (layer);
              cell.change_pcell_parameters (inst, parameters);
            }

          }

        }

      }

    }
    
    //  remove superfluous proxies
    layout.cleanup ();

    //  The selection is no longer valid
    view ()->clear_selection ();

    if (manager ()) {
      manager ()->commit ();
    }

  } else {
    throw tl::Exception (tl::to_string (tr ("Nothing selected to switch layers for")));
  }

}

void
MainService::check_no_guiding_shapes ()
{
  std::vector<edt::Service *> edt_services = view ()->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator es = edt_services.begin (); es != edt_services.end (); ++es) {
    for (edt::Service::obj_iterator s = (*es)->selection ().begin (); s != (*es)->selection ().end (); ++s) {
      if (! s->is_cell_inst ()) {
        if (s->layer () == view ()->cellview (s->cv_index ())->layout ().guiding_shape_layer ()) {
          throw tl::Exception (tl::to_string (tr ("This function cannot be applied to PCell guiding shapes")));
        }
      }
    }
  }
}

/**
 *  @brief Implementation of the insert notification object 
 *
 *  The basic purpose of this object is to provide a new selection for the
 *  new shapes inserted.
 */
class NewObjectsSelection
 : public db::ClipboardDataInsertReceiver 
{
public:
  NewObjectsSelection (int cv_index, db::cell_index_type topcell, lay::LayoutViewBase *view)
    : m_cv_index (cv_index), m_topcell (topcell)
  {
    mp_polygon_service = view->get_plugin <edt::PolygonService> ();
    mp_box_service = view->get_plugin <edt::BoxService> ();
    mp_point_service = view->get_plugin <edt::PointService> ();
    mp_text_service = view->get_plugin <edt::TextService> ();
    mp_path_service = view->get_plugin <edt::PathService> ();
    mp_inst_service = view->get_plugin <edt::InstService> ();
    tl_assert (mp_polygon_service);
    tl_assert (mp_box_service);
    tl_assert (mp_text_service);
    tl_assert (mp_path_service);
    tl_assert (mp_inst_service);
  }

  void shape_inserted (db::cell_index_type cell, int layer, const db::Shape &shape) 
  {
    lay::ObjectInstPath sel;
    sel.set_cv_index (m_cv_index);
    sel.set_topcell (m_topcell);
    sel.set_layer (layer);
    sel.set_shape (shape);

    if (m_topcell != cell) {
      return; // HINT insertion into subcell not supported currently
    }

    if (shape.is_polygon ()) {
      mp_polygon_service->add_selection (sel);
    } else if (shape.is_box ()) {
      mp_box_service->add_selection (sel);
    } else if (shape.is_text ()) {
      mp_text_service->add_selection (sel);
    } else if (shape.is_path ()) {
      mp_path_service->add_selection (sel);
    } 
  }

  void instance_inserted (db::cell_index_type cell, const db::Instance &instance) 
  {
    lay::ObjectInstPath sel;
    sel.set_cv_index (m_cv_index);
    sel.set_topcell (m_topcell);
    sel.add_path (db::InstElement (instance, db::CellInstArray::iterator ()));

    if (m_topcell != cell) {
      return; // HINT insertion into subcell not supported currently
    }

    mp_inst_service->add_selection (sel);
  }

private:
  edt::PolygonService *mp_polygon_service;
  edt::BoxService *mp_box_service;
  edt::PointService *mp_point_service;
  edt::TextService *mp_text_service;
  edt::PathService *mp_path_service;
  edt::InstService *mp_inst_service;
  int m_cv_index;
  db::cell_index_type m_topcell;
};

void 
MainService::paste ()
{
  if (view ()->is_editable ()) {

    int cv_index = view ()->active_cellview_index ();
    const lay::CellView &cv = view ()->cellview (cv_index);

    NewObjectsSelection insert_notification (cv_index, cv.cell_index (), view ());

    std::vector<unsigned int> new_layers;

    //  paste the content into the active cellview.
    for (db::Clipboard::iterator c = db::Clipboard::instance ().begin (); c != db::Clipboard::instance ().end (); ++c) {
      const db::ClipboardValue<edt::ClipboardData> *value = dynamic_cast<const db::ClipboardValue<edt::ClipboardData> *> (*c);
      if (value) {

        if (! cv.is_valid ()) {
          throw tl::Exception (tl::to_string (tr ("No cell selected to paste something into")));
        }

        std::vector<unsigned int> nl = value->get ().insert (cv->layout (), cv.context_trans ().inverted (), &cv->layout ().cell (cv.cell_index ()), 0 /*new_tops*/, &insert_notification);
        new_layers.insert (new_layers.end (), nl.begin (), nl.end ());

      }
    }

    //  Add new layers to the view if required.
    view ()->add_new_layers (new_layers, cv_index);
    view ()->update_content ();

  }
}

} // namespace edt


