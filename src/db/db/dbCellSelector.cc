
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "dbCellSelector.h"

#include "dbLayout.h"
#include "tlGlobPattern.h"
#include "tlString.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  CellSelector implementation

CellSelector::CellSelector ()
  : m_start_all (false), m_stop_all (false), m_is_valid (true)
{
  //  .. nothing yet ..
}

void
CellSelector::reset_selection ()
{
  clear (false, false);
}

void
CellSelector::clear (bool start_all, bool stop_all)
{
  m_start.clear ();
  m_start_all_except.clear ();
  m_start_all = start_all;

  m_stop.clear ();
  m_stop_all_except.clear ();
  m_stop_all = stop_all;
}

bool
CellSelector::operator== (const CellSelector &other) const
{
  if (m_is_valid && other.m_is_valid) {
    return m_start == other.m_start && m_start_all == other.m_start_all && m_start_all_except == other.m_start_all_except &&
           m_stop == other.m_stop && m_stop_all == other.m_stop_all && m_stop_all_except == other.m_stop_all_except;
  } else {
    return m_original_string == other.m_original_string;
  }
}

bool
CellSelector::operator< (const CellSelector &other) const
{
  if (m_is_valid && other.m_is_valid) {
    if (m_start != other.m_start) {
      return m_start < other.m_start;
    }
    if (m_start_all != other.m_start_all) {
      return m_start_all < other.m_start_all;
    }
    if (m_start_all_except != other.m_start_all_except) {
      return m_start_all_except < other.m_start_all_except;
    }
    if (m_stop != other.m_stop) {
      return m_stop < other.m_stop;
    }
    if (m_stop_all != other.m_stop_all) {
      return m_stop_all < other.m_stop_all;
    }
    if (m_stop_all_except != other.m_stop_all_except) {
      return m_stop_all_except < other.m_stop_all_except;
    }
    return false;
  } else {
    return m_original_string < other.m_original_string;
  }
}

bool
CellSelector::empty () const
{
  if (m_is_valid) {
    return ! m_start_all && m_start.empty () && ! m_stop_all && m_stop.empty ();
  } else {
    return m_original_string.empty ();
  }
}

bool
CellSelector::selects_all () const
{
  tl_assert (m_is_valid);
  return m_start_all && m_start_all_except.empty ();
}

bool
CellSelector::unselects_all () const
{
  tl_assert (m_is_valid);
  return m_stop_all && m_stop_all_except.empty ();
}

void
CellSelector::select_or_unselect_cell_exclude (bool select, db::cell_index_type cell_index, bool exclude)
{
  if (select) {

    if (exclude) {
      if (m_start_all) {
        m_start_all_except.insert (cell_index);
      }
    } else {
      select_cell (cell_index);
    }

  } else {

    if (exclude) {
      if (m_stop_all) {
        m_stop_all_except.insert (cell_index);
      }
    } else {
      unselect_cell (cell_index);
    }

  }
}

void
CellSelector::unselect_cells (const std::set<db::cell_index_type> &cells)
{
  for (auto c = cells.begin (); c != cells.end (); ++c) {
    unselect_cell (*c);
  }
}

void
CellSelector::unselect_cell (db::cell_index_type cell)
{
  if (m_stop_all) {
    m_stop_all_except.erase (cell);
  } else {
    m_stop.insert (cell);
  }

  //  take out this cell from the selected set
  if (m_start_all) {
    m_start_all_except.insert (cell);
  } else {
    m_start.erase (cell);
  }
}

void
CellSelector::unselect_all_cells ()
{
  clear (false, true);
}

void
CellSelector::select_cells (const std::set<db::cell_index_type> &cells)
{
  for (auto c = cells.begin (); c != cells.end (); ++c) {
    select_cell (*c);
  }
}

void
CellSelector::select_cell (db::cell_index_type cell)
{
  if (m_start_all) {
    m_start_all_except.erase (cell);
  } else {
    m_start.insert (cell);
  }

  //  take out this cell from the unselected set
  if (m_stop_all) {
    m_stop_all_except.insert (cell);
  } else {
    m_stop.erase (cell);
  }
}

void
CellSelector::select_all_cells ()
{
  clear (true, false);
}

int
CellSelector::in_selection (db::cell_index_type cell_index) const
{
  tl_assert (m_is_valid);

  bool in_start;
  if (m_start_all) {
    in_start = (m_start_all_except.find (cell_index) == m_start_all_except.end ());
  } else {
    in_start = (m_start.find (cell_index) != m_start.end ());
  }

  bool in_stop;
  if (m_stop_all) {
    in_stop = (m_stop_all_except.find (cell_index) == m_stop_all_except.end ());
  } else {
    in_stop = (m_stop.find (cell_index) != m_stop.end ());
  }

  tl_assert (! (in_start && in_stop));
  return (in_start ? 1 : (in_stop ? -1 : 0));
}

std::string
CellSelector::to_string (const db::Layout *layout) const
{
  if (! m_is_valid) {
    return m_original_string;
  }

  std::string res;

  if (m_start_all) {

    if (! res.empty ()) {
      res += " ";
    }
    res += "+*";

    for (auto c = m_start_all_except.begin (); c != m_start_all_except.end (); ++c) {
      if (! res.empty ()) {
        res += " ";
      }
      res += "+-";
      if (layout) {
        res += tl::to_word_or_quoted_string (layout->cell_name (*c));
      } else {
        res += "#";
        res += tl::to_string (*c);
      }
    }

  } else {

    for (auto c = m_start.begin (); c != m_start.end (); ++c) {
      if (! res.empty ()) {
        res += " ";
      }
      res += "+";
      if (layout) {
        res += tl::to_word_or_quoted_string (layout->cell_name (*c));
      } else {
        res += "#";
        res += tl::to_string (*c);
      }
    }

  }

  if (m_stop_all) {

    if (! res.empty ()) {
      res += " ";
    }
    res += "-*";

    for (auto c = m_stop_all_except.begin (); c != m_stop_all_except.end (); ++c) {
      if (! res.empty ()) {
        res += " ";
      }
      res += "--";
      if (layout) {
        res += tl::to_word_or_quoted_string (layout->cell_name (*c));
      } else {
        res += "#";
        res += tl::to_string (*c);
      }
    }

  } else {

    for (auto c = m_stop.begin (); c != m_stop.end (); ++c) {
      if (! res.empty ()) {
        res += " ";
      }
      res += "-";
      if (layout) {
        res += tl::to_word_or_quoted_string (layout->cell_name (*c));
      } else {
        res += "#";
        res += tl::to_string (*c);
      }
    }

  }

  return res;
}

CellSelector
CellSelector::from_string (const std::string &s, const db::Layout *layout, db::cell_index_type top_cell)
{
  CellSelector cs;
  tl::Extractor ex (s.c_str ());

  bool res = cs.parse (ex, layout, top_cell);
  tl_assert (res);
  ex.expect_end ();

  return cs;
}

bool
CellSelector::parse (tl::Extractor &ex, const db::Layout *layout, db::cell_index_type top_cell)
{
  const char *start = ex.skip ();

  m_original_string.clear ();

  bool res = do_parse (ex, layout, top_cell);
  if (res) {
    m_original_string = tl::trim (std::string (start, ex.get () - start));
    if (! m_is_valid) {
      //  not useful
      clear (false, false);
    }
    return true;
  } else {
    return false;
  }
}

void
CellSelector::ensure_valid (const db::Layout *layout, db::cell_index_type top_cell)
{
  if (! m_is_valid) {

    tl::Extractor ex (m_original_string.c_str ());

    bool res = do_parse (ex, layout, top_cell);
    tl_assert (res);
    tl_assert (m_is_valid);

  }
}

bool
CellSelector::do_parse (tl::Extractor &ex, const db::Layout *layout, db::cell_index_type top_cell)
{
  reset_selection ();
  m_is_valid = true;

  while (! ex.at_end ()) {

    bool plus;
    if (ex.test ("-")) {
      plus = false;
    } else if (ex.test ("+")) {
      plus = true;
    } else {
      //  stop here.
      return true;
    }

    bool exclude = ex.test ("-");

    if (ex.test ("#")) {

      db::cell_index_type cell_index = 0;
      if (! ex.try_read (cell_index)) {
        return false;
      }

      select_or_unselect_cell_exclude (plus, cell_index, exclude);

    } else if (ex.test ("@")) {

      if (top_cell != std::numeric_limits<db::cell_index_type>::max ()) {
        select_or_unselect_cell_exclude (plus, top_cell, exclude);
      } else {
        m_is_valid = false;
      }

    } else {

      //  NOTE: single quotes indicate "exact" match - i.e.
      //  'C*' is the cell called "*" while "C*" are all cells
      //  starting with "C".
      bool exact = *ex.skip () == '\'';

      std::string name;
      if (! ex.try_read_word_or_quoted (name, "_$*?")) {
        return false;
      }
      tl::GlobPattern pat (name);
      pat.set_exact (exact);

      if (pat.is_catchall ()) {

        if (plus) {

          if (exclude) {
            m_start_all = false;
            m_start.clear ();
            m_start_all_except.clear ();
          } else {
            select_all_cells ();
          }

        } else {

          if (exclude) {
            m_stop_all = false;
            m_stop.clear ();
            m_stop_all_except.clear ();
          } else {
            unselect_all_cells ();
          }

        }

      } else if (pat.is_const ()) {

        if (layout) {
          auto cell = layout->cell_by_name (pat.pattern ().c_str ());
          if (cell.first) {
            select_or_unselect_cell_exclude (plus, cell.second, exclude);
          }
        } else {
          m_is_valid = false;
        }

      } else {

        if (layout) {
          for (auto c = layout->begin (); c != layout->end (); ++c) {
            if (pat.match (layout->cell_name (c->cell_index ()))) {
              select_or_unselect_cell_exclude (plus, c->cell_index (), exclude);
            }
          }
        } else {
          m_is_valid = false;
        }

      }

    }

  }

  return true;
}

}
