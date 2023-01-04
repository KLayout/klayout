
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


#include "layDisplayState.h"
#include "tlString.h"
#include "tlXMLParser.h"

namespace lay
{

//  helper typedefs to make the templates more readable
typedef std::vector<std::string> string_v;
typedef std::vector<SpecificInst> specific_inst_v;
typedef std::list<CellPath> cell_path_v;

// -------------------------------------------------------------
//  CellPath implementation

const tl::XMLElementList *CellPath::xml_format ()
{
  static tl::XMLElementList format (
    tl::make_member<std::string, string_v::const_iterator, CellPath> (&CellPath::begin_path, &CellPath::end_path, &CellPath::push_back_path, "cellname") +
    tl::make_element<SpecificInst, specific_inst_v::const_iterator, CellPath> (&CellPath::begin_context_path, &CellPath::end_context_path, &CellPath::push_back_context_path, "cellinst",
      tl::make_member (&SpecificInst::cell_name, "cellname") +
      tl::make_member (&SpecificInst::trans_str, &SpecificInst::set_trans_str, "trans") +
      tl::make_member (&SpecificInst::array_trans_str, &SpecificInst::set_array_trans_str, "array_trans") 
    )
  );

  return &format;
}

// -------------------------------------------------------------
//  SpecificInst implementation

SpecificInst::SpecificInst ()
{
  //  .. nothing yet ..
}

SpecificInst::SpecificInst (const db::InstElement &el, const db::Layout &layout)
{
  cell_name = layout.cell_name (el.inst_ptr.cell_index ());
  trans = el.inst_ptr.complex_trans ();
  array_trans = *el.array_inst;
}

std::pair<bool, db::InstElement> 
SpecificInst::to_inst_element (const db::Layout &layout, const db::Cell &parent_cell) const
{
  //  first, we must find the cell by name
  std::pair<bool, db::cell_index_type> ci = layout.cell_by_name (cell_name.c_str ());
  if (! ci.first) {
    return std::make_pair(false, db::InstElement ());
  }

  db::cell_index_type cell_index = ci.second;

  for (db::Cell::const_iterator inst = parent_cell.begin (); ! inst.at_end (); ++inst) {

    //  use fuzzy comparison to find the base instance
    if (inst->cell_index () == cell_index && inst->complex_trans ().equal (trans)) {

      //  if a matching instance is found, look for the matching array instance.
      //  HINT: this can be optimized somewhat by inverting the transformation to a array instance
      //  directly rather than iterating.
      for (db::CellInstArray::iterator ainst = inst->begin (); ! ainst.at_end (); ++ainst) {
        if (*ainst == array_trans) {

          //  a matching instance/array instance is found: deliver this
          db::InstElement el;
          el.inst_ptr = *inst;
          el.array_inst = ainst;
          return std::make_pair (true, el);

        }
      }

    }

  }

  //  nothing found.
  return std::make_pair(false, db::InstElement ());
}

std::string 
SpecificInst::trans_str () const
{
  return trans.to_string ();
}

void 
SpecificInst::set_trans_str (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  ex.read (trans);
}

std::string 
SpecificInst::array_trans_str () const
{
  return array_trans.to_string ();
}

void 
SpecificInst::set_array_trans_str (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  ex.read (array_trans);
}

// -------------------------------------------------------------
//  DisplayState implementation

DisplayState::DisplayState ()
  : m_left (0.0), m_right (0.0), m_bottom (0.0), m_top (0.0), m_min_hier (1), m_max_hier (0), m_paths ()
{
  //  .. nothing yet ..
}

DisplayState::DisplayState (const db::DBox &b, int min_hier, int max_hier, const std::list<CellPath> &cns)
  : m_left (b.left ()), m_right (b.right ()), m_bottom (b.bottom ()), m_top (b.top ()), m_min_hier (min_hier), m_max_hier (max_hier), m_paths (cns)
{
  //  .. nothing yet ..
}

DisplayState::DisplayState (const db::DBox &b, int min_hier, int max_hier, const std::list<CellView> &cvs)
  : m_left (b.left ()), m_right (b.right ()), m_bottom (b.bottom ()), m_top (b.top ()), m_min_hier (min_hier), m_max_hier (max_hier)
{
  //  save the cell names
  for (std::list<CellView>::const_iterator cv = cvs.begin (); cv != cvs.end (); ++cv) {
    m_paths.push_back (CellPath ());
    if (cv->is_valid ()) {
      const CellView::unspecific_cell_path_type &p (cv->unspecific_path ());
      for (CellView::unspecific_cell_path_type::const_iterator cp = p.begin (); cp != p.end (); ++cp) {
        m_paths.back ().push_back_path ((*cv)->layout ().cell_name (*cp));
      }
      const CellView::specific_cell_path_type &sp (cv->specific_path ());
      for (CellView::specific_cell_path_type::const_iterator cp = sp.begin (); cp != sp.end (); ++cp) {
        m_paths.back ().push_back_context_path (SpecificInst (*cp, (*cv)->layout ()));
      }
    }
  }
}

lay::CellView 
DisplayState::cellview (unsigned int index, lay::LayoutHandle *layout_h) const
{
  //  try to restore the cell path
  lay::CellView::unspecific_cell_path_type cell_path;

  std::list<CellPath>::const_iterator cvi = m_paths.begin ();
  for (unsigned int i = 0; i < index && cvi != m_paths.end (); ++i) {
    ++cvi;
  }

  //  create the cellview to return
  lay::CellView cv;
  cv.set (layout_h);

  if (cvi != m_paths.end ()) {

    //  check, if we can reconstruct the path from the names
    bool valid_path = false;

    for (std::vector<std::string>::const_iterator cn = cvi->begin_path (); cn != cvi->end_path (); ++cn) {
      std::pair<bool, db::cell_index_type> pci = layout_h->layout ().cell_by_name (cn->c_str ());
      if (pci.first) {
        cell_path.push_back (pci.second);
        valid_path = true;
      } else {
        tl::warn << tl::to_string (tr ("Cellname cannot be reconstructed: ")) << *cn;
        valid_path = false;
        break;
      }
    }

    //  if available, take the path that was reconstructed
    if (valid_path) {

      cv.set_unspecific_path (cell_path);

      tl_assert (! cell_path.empty ());
      const db::Cell *pc = &layout_h->layout ().cell (cell_path.back ());

      std::vector<db::InstElement> context_path;

      //  try further to extract the context path component
      valid_path = false;
      for (std::vector<SpecificInst>::const_iterator ci = cvi->begin_context_path (); ci != cvi->end_context_path (); ++ci) {
        std::pair<bool, db::InstElement> ie = ci->to_inst_element (layout_h->layout (), *pc);
        if (ie.first) {
          context_path.push_back (ie.second);
          pc = &layout_h->layout ().cell (ie.second.inst_ptr.cell_index ());
          valid_path = true;
        } else {
          tl::warn << tl::to_string (tr ("Specific instance cannot be reconstructed: instantiated cell is ")) << ci->cell_name
                   << tl::to_string (tr (", parent cell is ")) << layout_h->layout ().cell_name (pc->cell_index ());
          valid_path = false;
        }
      }

      //  if possible, establish the context path now
      if (valid_path) {
        cv.set_specific_path (context_path);
      }

    } else {

      //  as the default behaviour, try to locate the cell by the last component's name
      if (m_paths.size () > index && cvi->begin_path () != cvi->end_path ()) {
        std::pair<bool, db::cell_index_type> pci = layout_h->layout ().cell_by_name (cvi->begin_path ()->c_str ());
        if (pci.first) {
          cv.set_cell (pci.second);
          return cv;
        }
      }

    }

  }

  //  if everything fails, return an empty cellview
  return cv;
}

const tl::XMLElementList *DisplayState::xml_format ()
{
  static tl::XMLElementList format (
    tl::make_member<double, DisplayState> (&DisplayState::xleft, &DisplayState::set_xleft, "x-left") + 
    tl::make_member<double, DisplayState> (&DisplayState::xright, &DisplayState::set_xright, "x-right") + 
    tl::make_member<double, DisplayState> (&DisplayState::ybottom, &DisplayState::set_ybottom, "y-bottom") + 
    tl::make_member<double, DisplayState> (&DisplayState::ytop, &DisplayState::set_ytop, "y-top") + 
    tl::make_member<int, DisplayState> (&DisplayState::min_hier, &DisplayState::set_min_hier, "min-hier") + 
    tl::make_member<int, DisplayState> (&DisplayState::max_hier, &DisplayState::set_max_hier, "max-hier") + 
    tl::make_element<cell_path_v, DisplayState> (&DisplayState::paths, &DisplayState::set_paths, "cellpaths", 
      tl::make_element<CellPath, cell_path_v::const_iterator, cell_path_v> (&cell_path_v::begin, &cell_path_v::end, &cell_path_v::push_back, "cellpath", CellPath::xml_format ())
    )
  );

  return &format;
}

}

