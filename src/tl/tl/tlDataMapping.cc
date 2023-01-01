
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


#include "tlDataMapping.h"
#include "tlAssert.h"
#include "tlString.h"
#include "tlLog.h"

#include <math.h>
#include <algorithm>


namespace tl
{

// -------------------------------------------------------------------------
//  A compare operator comparing the first element of two double,double-pairs.

struct compare_first_double_of_pair 
{
  bool operator() (const std::pair<double, double> &a, const std::pair<double, double> &b) const
  {
    return a.first < b.first;
  }
};

// -------------------------------------------------------------------------
//  A helper method to interpolate a table entry at the given iterator 
//  (i[-1].first <= x, i->first >= x)

inline double 
interpolate (const std::vector< std::pair<double, double> > &v, std::vector< std::pair<double, double> >::const_iterator i, double x)
{
  if (i == v.end ()) {
    return v.back ().second;
  } else if (i == v.begin ()) {
    return v.front ().second;
  } else {
    return i[-1].second + (x - i[-1].first) * (i->second - i[-1].second) / (i->first - i[-1].first);
  }
}

// -------------------------------------------------------------------------
//  A helper method to interpolate a table entry at the given iterator 
//  (i[-1].first <= x, i->first >= x)

inline double 
interpolate (const std::vector< std::pair<double, double> > &v, double x)
{
  std::vector< std::pair<double, double> >::const_iterator i = std::lower_bound (v.begin (), v.end (), std::make_pair (x, 0.0), compare_first_double_of_pair ());
  return interpolate (v, i, x);
}

// -------------------------------------------------------------------------
//  CombinedDataMapping implementation

CombinedDataMapping::CombinedDataMapping (DataMappingBase *o, DataMappingBase *i)
  : mp_o (o), mp_i (i)
{
  // .. nothing yet ..
}

CombinedDataMapping::~CombinedDataMapping ()
{
  if (mp_o) {
    delete mp_o;
    mp_o = 0;
  }
  if (mp_i) {
    delete mp_i;
    mp_i = 0;
  }
}

double 
CombinedDataMapping::xmin () const
{
  return mp_i->xmin ();
}

double 
CombinedDataMapping::xmax () const
{
  return mp_i->xmax ();
}

void 
CombinedDataMapping::generate_table (std::vector< std::pair<double, double> > &table)
{
  std::vector< std::pair<double, double> > ti;
  mp_i->generate_table (ti);
  tl_assert (ti.size () >= 2);

  std::vector< std::pair<double, double> > to;
  mp_o->generate_table (to);
  tl_assert (to.size () >= 2);

  table.push_back (std::make_pair (ti.front ().first, interpolate (to, ti.front ().second)));

  for (std::vector< std::pair<double, double> >::const_iterator t = ti.begin () + 1; t != ti.end (); ++t) {

    double x1 = t[-1].first, x2 = t->first;
    double y1 = t[-1].second, y2 = t->second;

    std::vector< std::pair<double, double> >::const_iterator tt1 = std::lower_bound (to.begin (), to.end (), std::make_pair (y1, 0.0), compare_first_double_of_pair ());
    std::vector< std::pair<double, double> >::const_iterator tt2 = std::lower_bound (to.begin (), to.end (), std::make_pair (y2, 0.0), compare_first_double_of_pair ());

    while (tt1 < tt2) {

      //  Hint: can we be sure that tt1 == tt2 if y2 == y1 thus always y2 != y1 in this case?
      double y = tt1->first;
      double x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);

      table.push_back (std::make_pair (x, tt1->second));

      ++tt1;

    }

    while (tt2 < tt1) {

      //  Hint: can we be sure that tt1 == tt2 if y2 == y1 thus always y2 != y1 in this case?
      double y = tt2->first;
      double x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);

      table.push_back (std::make_pair (x, tt2->second));

      ++tt2;

    }

    table.push_back (std::make_pair (x2, interpolate (to, tt1, y2)));

  }

  //  sweep table and remove similar x values
  double epsilon = (table.back ().first - table.front ().first) * 1e-6;
  std::vector< std::pair<double, double> >::iterator tw = table.begin ();

  for (std::vector< std::pair<double, double> >::const_iterator t = table.begin (); t != table.end (); ++t) {
    if (t + 1 != table.end () && t->first + epsilon > t[1].first) {
      *tw = std::make_pair (0.5 * (t->first + t[1].first), 0.5 * (t->second + t[1].second));
      ++t;
    } else {
      *tw = *t;
    }
    ++tw;
  }

  table.erase (tw, table.end ());
}

void
CombinedDataMapping::dump () const
{
  tl::info << "CombinedDataMapping(";
  tl::info << "outer=" << tl::noendl;
  mp_o->dump ();
  tl::info << "inner=" << tl::noendl;
  mp_i->dump ();
  tl::info << ")";
}

// -------------------------------------------------------------------------
//  LinearCombinationDataMapping implementation

LinearCombinationDataMapping::LinearCombinationDataMapping (double c, DataMappingBase *a, double ca, DataMappingBase *b, double cb)
  : mp_a (a), mp_b (b), m_ca (ca), m_cb (cb), m_c (c)
{
  if (!mp_a && mp_b) {
    std::swap (mp_a, mp_b);
    std::swap (m_ca, m_cb);
  }
}

LinearCombinationDataMapping::~LinearCombinationDataMapping ()
{
  if (mp_a) {
    delete mp_a;
    mp_a = 0;
  }
  if (mp_b) {
    delete mp_b;
    mp_b = 0;
  }
}

double 
LinearCombinationDataMapping::xmin () const
{
  if (!mp_a) {
    return -1e23; // some large negative value
  } else if (!mp_b) {
    return mp_a->xmin ();
  } else {
    return std::min (mp_a->xmin (), mp_b->xmin ());
  }
}

double 
LinearCombinationDataMapping::xmax () const
{
  if (!mp_a) {
    return 1e23; // some large positive value
  } else if (!mp_b) {
    return mp_a->xmax ();
  } else {
    return std::max (mp_a->xmax (), mp_b->xmax ());
  }
}

void 
LinearCombinationDataMapping::generate_table (std::vector< std::pair<double, double> > &table)
{
  if (!mp_a) {

    table.push_back (std::make_pair (xmin (), m_c));
    table.push_back (std::make_pair (xmax (), m_c));

  } else if (!mp_b) {

    mp_a->generate_table (table);

    for (std::vector< std::pair<double, double> >::iterator t = table.begin (); t != table.end (); ++t) {
      t->second = m_c + m_ca * t->second;
    }

  } else {

    std::vector< std::pair<double, double> > ta;
    mp_a->generate_table (ta);
    tl_assert (ta.size () >= 2);

    std::vector< std::pair<double, double> > tb;
    mp_b->generate_table (tb);
    tl_assert (tb.size () >= 2);

    std::vector< std::pair<double, double> >::const_iterator a = ta.begin (); 
    std::vector< std::pair<double, double> >::const_iterator b = tb.begin (); 

    double epsilon = (xmax () - xmin ()) * 1e-6;

    while (a != ta.end () || b != tb.end ()) {
      
      if (a == ta.end ()) {
        table.push_back (std::make_pair (b->first, m_c + m_ca * ta.back ().second + m_cb * b->second));
        ++b;
      } else if (b == tb.end ()) {
        table.push_back (std::make_pair (a->first, m_c + m_ca * a->second + m_cb * tb.back ().second));
        ++a;
      } else if (a->first < b->first - epsilon) {
        table.push_back (std::make_pair (a->first, m_c + m_ca * a->second + m_cb * interpolate (tb, b, a->first)));
        ++a;
      } else if (a->first > b->first + epsilon) {
        table.push_back (std::make_pair (b->first, m_c + m_ca * interpolate (ta, a, b->first) + m_cb * b->second));
        ++b;
      } else {
        table.push_back (std::make_pair (0.5 * (a->first + b->first), m_c + m_ca * a->second + m_cb * b->second));
        ++a;
        ++b;
      }
    }

  }
}

void
LinearCombinationDataMapping::dump () const
{
  tl::info << "LinearCombinationDataMapping(" << m_c << "+";
  tl::info << "a=" << m_ca << "*" << tl::noendl;
  if (mp_a) {
    mp_a->dump ();
  } else {
    tl::info << "(null)";
  }
  tl::info << "b=" << m_cb << "*" << tl::noendl;
  if (mp_b) {
    mp_b->dump ();
  } else {
    tl::info << "(null)";
  }
  tl::info << ")";
}

// -------------------------------------------------------------------------
//  TableDataMapping implementation

void
TableDataMapping::dump () const
{
  tl::info << "TableDataMapping(xmin=" << m_xmin << ", xmax=" << m_xmax << ",";
  for (std::vector< std::pair<double, double> >::const_iterator p = m_table.begin (); p != m_table.end (); ++p) {
    tl::info << p->first << ":" << p->second << ";" << tl::noendl;
  }
  tl::info << "";
  tl::info << ")";
}

// -------------------------------------------------------------------------
//  DataMappingLookupTable implementation

DataMappingLookupTable::DataMappingLookupTable (DataMappingBase *dm)
  : m_dxinv (1.0), m_xmin (0.0), mp_y (0), mp_c (0), m_size (0), mp_dm (dm)
{
  // .. nothing yet ..
}

DataMappingLookupTable::~DataMappingLookupTable ()
{
  release ();
}

void
DataMappingLookupTable::release ()
{
  if (mp_y) {
    delete [] mp_y;
    mp_y = 0;
  }
  if (mp_c) {
    delete [] mp_c;
    mp_c = 0;
  }

  if (mp_dm) {
    delete mp_dm;
    mp_dm = 0;
  }
}

void 
DataMappingLookupTable::set_data_mapping (DataMappingBase *dm)
{
  release ();
  mp_dm = dm;
}

void 
DataMappingLookupTable::update_table (double xmin, double xmax, double delta_y, unsigned int ifactor)
{
  if (mp_y) {
    delete [] mp_y;
    mp_y = 0;
  }
  if (mp_c) {
    delete [] mp_c;
    mp_c = 0;
  }

  std::vector< std::pair<double, double> > table;

  if (mp_dm) {
    mp_dm->generate_table (table);
  }

  if (table.size () < 1) {

    //  TODO: should mimic a linear behaviour by observing delta_y
    m_dxinv = 1.0 / (xmax - xmin);
    m_xmin = xmin;
    mp_y = new double[3];
    m_size = 2;
    mp_y[0] = xmin;
    mp_y[1] = xmax;
    mp_y[2] = xmax;

  } else if (table.size () < 2) {

    double yconst = table[0].second;

    m_dxinv = 1.0 / (xmax - xmin);
    m_xmin = xmin;
    mp_y = new double[3];
    mp_y[0] = mp_y[1] = mp_y[2] = yconst;
    m_size = 2;

  } else {

    double delta_x = xmax - xmin;

    for (std::vector< std::pair<double, double> >::const_iterator t = table.begin () + 1; t != table.end (); ++t) {

      double dx = fabs (t->first - t[-1].first);
      double dy = fabs (t->second - t[-1].second);
  
      if (dx * delta_y < delta_x * dy) {
        delta_x = dx / dy * delta_y;
      }

    }

    size_t nsteps = size_t (ceil((xmax - xmin) / delta_x - 1e-6));

    //  Limit the number of interpolation points (this is an arbitrary number - it could be somewhat else)
    nsteps = std::min (size_t (16384), nsteps);

    delta_x = (xmax - xmin) / double (nsteps);

    mp_y = new double[nsteps + 1]; // plus one for safety
    m_size = nsteps;

    std::vector< std::pair<double, double> >::const_iterator t = table.begin ();
    size_t i = 0;
    for (double x = xmin; i < nsteps; ++i, x += delta_x) {
      while (t != table.end () && t->first <= x) {
        ++t;
      }
      mp_y[i] = interpolate (table, t, x);
    }

    //  add one item for safety (rounding problems in operator[] implementation)
    mp_y[i] = mp_y[i - 1];
    m_xmin = xmin - delta_x * 0.5;
    m_dxinv = 1.0 / delta_x;

  }

  mp_c = new unsigned int [m_size + 1];
  for (size_t i = 0; i < m_size; ++i) {
    mp_c [i] = (unsigned int) std::min (255.0, std::max (0.0, mp_y [i])) * ifactor;
  }
  mp_c [m_size] = mp_c [m_size - 1];
}

std::string
DataMappingLookupTable::dump () const
{
  std::string r;

  r += "xmin=" + tl::to_string (m_xmin) + ",dx=" + tl::to_string (1.0 / m_dxinv) + ":";
  for (size_t i = 0; i < m_size; ++i) {
    r += tl::to_string (mp_y [i]) + ";";
  }

  return r;
}

}
