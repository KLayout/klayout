
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


#ifndef HDR_layDisplayState
#define HDR_layDisplayState

#include "laybasicCommon.h"

#include "layCellView.h"

#include <string>
#include <vector>

namespace tl
{
  class XMLElementList;
}

namespace lay
{

/**
 *  @brief A structure encapsulating an specific instance
 *
 *  This is a normalized representation of a db::InstElement object suitable to being stored in a XML 
 *  document or disconnected from a layout object.
 */
struct LAYBASIC_PUBLIC SpecificInst
{
  std::string cell_name;
  db::ICplxTrans trans;
  db::Trans array_trans;

  SpecificInst ();
  SpecificInst (const db::InstElement &el, const db::Layout &layout);

  /**
   *  @brief Convert the specific instance to a db::InstElement object with the given parent cell
   *
   *  This method returns false in the first member of the returned pair, if the 
   *  specific instance cannot be converted back.
   */
  std::pair<bool, db::InstElement> to_inst_element (const db::Layout &layout, const db::Cell &parent_cell) const;

  std::string trans_str () const;
  void set_trans_str (const std::string &s);
  std::string array_trans_str () const;
  void set_array_trans_str (const std::string &s);
};

/**
 *  @brief A structure encapsulating a cell path and a context path
 *
 *  Basically this structure is just needed to provide a nice adaptor for the XML reader/writer 
 *  in the BookmarkList
 */
struct LAYBASIC_PUBLIC CellPath 
{
  std::vector<std::string> path;
  std::vector<SpecificInst> context_path;

  std::vector<std::string>::const_iterator begin_path () const 
  {
    return path.begin ();
  }

  std::vector<std::string>::const_iterator end_path () const 
  {
    return path.end ();
  }

  void push_back_path (const std::string &name)
  {
    path.push_back (name);
  }

  std::vector<SpecificInst>::const_iterator begin_context_path () const 
  {
    return context_path.begin ();
  }

  std::vector<SpecificInst>::const_iterator end_context_path () const 
  {
    return context_path.end ();
  }

  void push_back_context_path (const SpecificInst &inst)
  {
    context_path.push_back (inst);
  }

  static const tl::XMLElementList *xml_format ();
};

/**
 *  @brief This class encapsulates a display state
 *
 *  A display state is comprised of a box and a cell path for all views.
 *  It can be used to transfer a display state from one layout to another.
 */
struct LAYBASIC_PUBLIC DisplayState
{
  /**
   *  @brief Default ctor
   */
  DisplayState ();

  /**
   *  @brief Create a display state from a given set of cell views and a box
   */
  DisplayState (const db::DBox &b, int min_hier, int max_hier, const std::list<CellView> &cvs);

  /**
   *  @brief Create a display state from a given set of cell name paths 
   */
  DisplayState (const db::DBox &b, int min_hier, int max_hier, const std::list<CellPath> &cns);

  /**
   *  @brief Transform a partial display state back into a cellview 
   *
   *  This method provides some safety: if the display state cannot be
   *  transferred into the layout, some reasonable assumption is made 
   *  and an artifical state is created.
   *
   *  @param index The index of the cellview to obtain
   *  @param layout The layout to refer to
   *  @return A valid cellview for the given layout
   */
  lay::CellView cellview (unsigned int index, lay::LayoutHandle *layout_h) const;

  /**
   *  @brief Obtain minimum drawn hierarchy level
   */
  int min_hier () const
  {
    return m_min_hier;
  }

  /**
   *  @brief Set minimum drawn hierarchy level
   */
  void set_min_hier (int l) 
  {
    m_min_hier = l;
  }

  /**
   *  @brief Obtain maximum drawn hierarchy level
   */
  int max_hier () const
  {
    return m_max_hier;
  }

  /**
   *  @brief Set maximum drawn hierarchy level
   */
  void set_max_hier (int l) 
  {
    m_max_hier = l;
  }

  /**
   *  @brief Obtain the box
   */
  db::DBox box () const
  {
    return db::DBox (m_left, m_bottom, m_right, m_top);
  }

  double xleft () const
  {
    return m_left;
  }

  double xright () const
  {
    return m_right;
  }

  double ytop () const
  {
    return m_top;
  }

  double ybottom () const
  {
    return m_bottom;
  }

  void set_xleft (double c)
  {
    m_left = c;
  }

  void set_xright (double c)
  {
    m_right = c;
  }

  void set_ytop (double c)
  {
    m_top = c;
  }

  void set_ybottom (double c)
  {
    m_bottom = c;
  }

  /**
   *  @brief Obtain the raw cell name list
   */
  const std::list<CellPath> &paths () const
  {
    return m_paths;
  }

  /**
   *  @brief Add a new path
   */
  void set_paths (const std::list<CellPath> &p)
  {
    m_paths = p;
  }

  static const tl::XMLElementList *xml_format ();

private:
  double m_left, m_right, m_bottom, m_top;
  int m_min_hier, m_max_hier;
  std::list<CellPath> m_paths;
};


}

#endif

