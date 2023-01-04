
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



#include "dbLayout.h"
#include "dbCellGraphUtils.h"
#include "dbFuzzyCellMapping.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "tlFixedVector.h"

#include <memory>
#include <map>
#include <set>
#include <vector>

namespace db
{

class TransformationMatrixSum 
{
public:
  TransformationMatrixSum (const db::Layout &, const db::Cell &)
  {
  }

  TransformationMatrixSum (const db::Matrix2d &m)
  {
    m_m = m;
  }

  TransformationMatrixSum transformed (const db::CellInstArray &inst) const
  {
    TransformationMatrixSum result (*this);
    result.transform (inst);
    return result;
  }

  void transform (const db::CellInstArray &inst)
  {
    m_m = db::Matrix2d (inst.complex_trans ().inverted ());
  }

  void add (const TransformationMatrixSum &other)
  {
    m_m += other.m_m;
  }

  const db::Matrix2d &m () const { return m_m; }

private:
  db::Matrix2d m_m;
};

class InstanceReferenceSum
{
public:
  InstanceReferenceSum (const db::Layout &, const db::Cell &)
    : m_count (0)
  {
    // .. nothing yet ..
  }

  InstanceReferenceSum (size_t count, const db::Matrix2d &m, const db::DPoint &p)
    : m_count (count), m_m (m), m_p (p)
  {
    // .. nothing yet ..
  }

  InstanceReferenceSum transformed (const db::CellInstArray &inst) const
  {
    db::Matrix2d m_res; 
    db::DVector p_res;

    m_res += db::Matrix2d (inst.complex_trans ()) * double (inst.size ());

    for (db::CellInstArray::iterator a = inst.begin (); ! a.at_end (); ++a) {
      p_res += db::DVector ((*a).disp ());
    }

    if (m_count == 0) {
      return InstanceReferenceSum (inst.size (), m_res, m_p + p_res);
    } else {
      return InstanceReferenceSum (m_count * inst.size (), m_m * m_res, m_p + m_m * p_res);
    }
  }

  void add (const InstanceReferenceSum &other)
  {
    m_count += other.m_count;
    m_p += db::DVector (other.m_p);
    m_m += other.m_m;
  }

  size_t n () const { return m_count; }
  const db::DPoint &p () const { return m_p; }
  const db::Matrix2d &m () const { return m_m; }

private:
  size_t m_count;
  db::Matrix2d m_m;
  db::DPoint m_p;
};

// -------------------------------------------------------------------------------------
//  FuzzyCellMapping implementation

FuzzyCellMapping::FuzzyCellMapping ()
{
  // .. nothing yet ..
}

void FuzzyCellMapping::clear ()
{
  m_b2a_mapping.clear ();
}

inline double distance_func (double a, double b)
{
  if (fabs (a) + fabs (b) < 1e-6) {
    return 0.0;
  } else {
    return 2.0 * fabs (a - b) / (fabs (a) + fabs (b));
  }
}

struct CellSignature
{
  size_t weight;
  db::Box bbox;
  size_t instances;
  std::vector<size_t> shapes;
  db::Matrix2d tm_avg;
  db::DPoint p_avg;

  bool distance_less_or_equal (const CellSignature &other, double dmin, double &dmin_out) const
  {
    double d;
    d = distance_func (weight, other.weight);
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (bbox.left (), other.bbox.left ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (bbox.top (), other.bbox.top ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (bbox.right (), other.bbox.right ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (bbox.bottom (), other.bbox.bottom ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (instances, other.instances);
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    std::vector<size_t>::const_iterator s1, s2;
    for (s1 = shapes.begin (), s2 = other.shapes.begin (); s1 != shapes.end (); ++s1, ++s2) {
      d += distance_func (*s1, *s2);
      if (dmin >= 0.0 && d > dmin + 1e-6) {
        return false;
      }
    }

    d += distance_func (tm_avg.m11 (), other.tm_avg.m11 ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (tm_avg.m12 (), other.tm_avg.m12 ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (tm_avg.m21 (), other.tm_avg.m21 ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (tm_avg.m22 (), other.tm_avg.m22 ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (p_avg.x (), other.p_avg.x ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    d += distance_func (p_avg.y (), other.p_avg.y ());
    if (dmin >= 0.0 && d > dmin + 1e-6) {
      return false;
    }

    dmin_out = d;

    return true;
  }

  std::string to_string () const
  {
    std::string st;
    for (std::vector <size_t>::const_iterator s = shapes.begin (); s != shapes.end (); ++s) {
      if (! st.empty ()) {
        st += ",";
      }
      st += tl::to_string (*s);
    }

    return "weight=" + tl::to_string (weight) + 
           " bbox=" + bbox.to_string () + 
           " instances=" + tl::to_string (instances) + 
           " shapes=" + st + 
           " tm_avg=" + tm_avg.to_string () + 
           " p_avg=" + p_avg.to_string ();
  }
};

static void collect_cell_signatures (const db::Layout &layout, const std::vector <unsigned int> &layers, db::cell_index_type cell_index, std::map <db::cell_index_type, CellSignature> &metrics, const std::string &progress_report)
{
  db::InstanceStatistics<InstanceReferenceSum> rs (&layout, cell_index);

  tl::RelativeProgress progress (progress_report, rs.selection ().size ());

  for (db::CellCounter::selection_iterator c = rs.begin (); c != rs.end (); ++c) {

    ++progress;

    const db::Cell &cell = layout.cell (*c);
    const InstanceReferenceSum &rsv = rs.value (*c);

    CellSignature &m = metrics.insert (std::make_pair (*c, CellSignature ())).first->second;

    m.shapes.reserve (layers.size ());
    for (std::vector <unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
      const db::Shapes &shapes = cell.shapes (*l);
      size_t n = 0;
      //  TODO: right now, the only way to get the "true" shape count is to iterate ...
      for (db::ShapeIterator s = shapes.begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        ++n;
      }
      m.shapes.push_back (n);
    }

    m.weight = rsv.n ();

    m.bbox = cell.bbox ();
    size_t ni = 0;
    for (db::Instances::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
      ni += i->size ();
    }
    m.instances = ni;

    double n = std::max (1.0, double (m.weight));
    m.tm_avg = rsv.m () * (1.0 / n);
    m.p_avg = rsv.p () * (1.0 / n);

    if (tl::verbosity () >= 40) {
      tl::info << "  " << layout.cell_name (*c) << " " << m.to_string ();
    }

  }
}

void 
FuzzyCellMapping::create (const db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Cell mapping")));

  if (tl::verbosity () >= 20) {
    tl::info << "Cell mapping";
  }

  std::vector <unsigned int> la, lb;

  for (db::Layout::layer_iterator l = layout_a.begin_layers (); l != layout_a.end_layers (); ++l) {
    for (db::Layout::layer_iterator ll = layout_b.begin_layers (); ll != layout_b.end_layers (); ++ll) {
      if ((*l).second->log_equal (*(*ll).second)) {
        la.push_back ((*l).first);
        lb.push_back ((*ll).first);
        break;
      }
    }
  }

  if (tl::verbosity () >= 40) {
    tl::info << "Signatures (a):";
  }

  std::map <db::cell_index_type, CellSignature> ma;
  collect_cell_signatures (layout_a, la, cell_index_a, ma, tl::to_string (tr ("Collecting cell signatures (A)")));

  if (tl::verbosity () >= 40) {
    tl::info << "Signatures (b):";
  }

  std::map <db::cell_index_type, CellSignature> mb;
  collect_cell_signatures (layout_b, lb, cell_index_b, mb, tl::to_string (tr ("Collecting cell signatures (B)")));

  tl::RelativeProgress progress (tl::to_string (tr ("Finding matching cells")), ma.size () * ma.size ());

  for (std::map <db::cell_index_type, CellSignature>::const_iterator m = ma.begin (); m != ma.end (); ++m) {

    if (tl::verbosity () >= 30) {
      tl::info << "Treating cell (a) " << layout_a.cell_name (m->first);
    }

    ++progress;

    //  look up the nearest match in the "b" cells.
    double dmin = -1.0;
    std::vector <db::cell_index_type> cmin;
    for (std::map <db::cell_index_type, CellSignature>::const_iterator n = mb.begin (); n != mb.end (); ++n) {

      ++progress;

      double d = -1.0;
      if (n->second.distance_less_or_equal (m->second, dmin, d)) {

        if (dmin >= 0.0 && distance_func (d, dmin) > 1e-6) {
          cmin.clear ();
        }

        dmin = d;
        cmin.push_back (n->first);

      }

    }

    if (tl::verbosity () >= 40) {
      tl::info << "First-level candidates (b):" << tl::noendl;
      for (std::vector<db::cell_index_type>::const_iterator c = cmin.begin (); c != cmin.end (); ++c) {
        tl::info << " " << layout_b.cell_name (*c) << tl::noendl;
      }
      tl::info << "";
    }

    std::vector <db::cell_index_type> cmin_confirmed;

    //  if a unique match was found, confirm the match by looking if there is a "a" cell matching better.
    for (std::vector <db::cell_index_type>::const_iterator c = cmin.begin (); c != cmin.end (); ++c) {

      bool confirmed = true;

      const CellSignature &mmin = mb [*c];
      std::map <db::cell_index_type, CellSignature>::const_iterator mm = m;
      ++mm;
      for ( ; mm != ma.end () && confirmed; ++mm) {

        ++progress;
        double d = -1.0;
        if (mmin.distance_less_or_equal (mm->second, dmin, d) && distance_func (d, dmin) > 1e-6) {
          confirmed = false;
        }

      }

      if (confirmed) {
        cmin_confirmed.push_back (*c);
      }

    }

    std::swap (cmin, cmin_confirmed);

    if (tl::verbosity () >= 40) {
      tl::info << "Confirmed candidates (b):" << tl::noendl;
      for (std::vector<db::cell_index_type>::const_iterator c = cmin.begin (); c != cmin.end (); ++c) {
        tl::info << " " << layout_b.cell_name (*c) << tl::noendl;
      }
      tl::info << "";
    }

    //  If there is no unique mapping, use the name similarity (measured by the edit distance)
    //  as a distance measure
    if (cmin.size () > 1) {

      int min_ed = std::numeric_limits<int>::max ();
      db::cell_index_type min_ed_ci;

      for (std::vector<db::cell_index_type>::const_iterator c = cmin.begin (); c != cmin.end (); ++c) {

        int ed = tl::edit_distance (layout_a.cell_name (m->first), layout_b.cell_name (*c));
        if (ed < min_ed) {
          min_ed = ed;
          min_ed_ci = *c;
        }

      }

      cmin.clear ();
      cmin.push_back (min_ed_ci);

    }

    if (tl::verbosity () >= 40) {
      tl::info << "Refined candidates (b):" << tl::noendl;
      for (std::vector<db::cell_index_type>::const_iterator c = cmin.begin (); c != cmin.end (); ++c) {
        tl::info << " " << layout_b.cell_name (*c) << tl::noendl;
      }
      tl::info << "";
    }

    if (cmin.size () > 0) {

#if 0 // debugging
      if (std::string (layout_a.cell_name (m->first)) != std::string (layout_b.cell_name (cmin [0]))) {
        tl::info << "Name mismatch " << layout_a.cell_name (m->first) << " and " << layout_b.cell_name (cmin [0]);
        tl::info << " (A) signature " << m->second.to_string ();
        tl::info << " (B) signature " << mb [cmin [0]].to_string ();
      }
#endif

      if (tl::verbosity () >= 30) {
        tl::info << "Cell mapping - found a matching pair " << layout_a.cell_name (m->first) << " and " << layout_b.cell_name (cmin [0]);
      }

      mb.erase (cmin [0]);
      m_b2a_mapping.insert (std::make_pair (cmin [0], m->first));

    } else {

      if (tl::verbosity () >= 30) {
        if (cmin.size () == 0) {
          tl::info << "Cell mapping - no match found for " << layout_a.cell_name (m->first);
        } else {
          tl::info << "Cell mapping - multiple matches found for " << layout_a.cell_name (m->first) << ": " << tl::noendl;
          for (size_t i = 0; i < cmin.size (); ++i) {
            if (i > 0) {
              tl::info << ", " << tl::noendl;
            }
            tl::info << layout_b.cell_name (cmin [i]) << tl::noendl;
          }
          tl::info << ""; // implies eol
        }
      }

    }

  }

}

void 
FuzzyCellMapping::dump_mapping (const std::map <db::cell_index_type, std::vector<db::cell_index_type> > &candidates, 
                           const db::Layout &layout_a, const db::Layout &layout_b)
{
  for (std::map <db::cell_index_type, std::vector<db::cell_index_type> >::const_iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {
    tl::info << "  " << layout_a.cell_name (cand->first) << " ->" << tl::noendl;
    int n = 5;
    for (std::vector<db::cell_index_type>::const_iterator c = cand->second.begin (); c != cand->second.end () && --n > 0; ++c) {
      tl::info << " " << layout_b.cell_name (*c) << tl::noendl;
    }
    if (n == 0) {
      tl::info << " ..";
    } else {
      tl::info << "";
    }
  }
}

std::pair<bool, db::cell_index_type> 
FuzzyCellMapping::cell_mapping_pair (db::cell_index_type cell_index_b) const
{
  std::map <db::cell_index_type, db::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  if (m == m_b2a_mapping.end ()) {
    return std::make_pair (false, 0);
  } else {
    return std::make_pair (true, m->second);
  }
}

bool 
FuzzyCellMapping::has_mapping (db::cell_index_type cell_index_b) const
{
  std::map <db::cell_index_type, db::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  return (m != m_b2a_mapping.end ());
}

db::cell_index_type 
FuzzyCellMapping::cell_mapping (db::cell_index_type cell_index_b) const
{
  std::map <db::cell_index_type, db::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  tl_assert (m != m_b2a_mapping.end ());
  return m->second;
}

}

