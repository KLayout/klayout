
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


#ifndef HDR_tlDataMapping
#define HDR_tlDataMapping

#include "tlCommon.h"

#include <vector>
#include <string>

namespace tl
{

/**
 *  @brief A generic class that maps some scalar value to another scalar values
 * 
 *  The mapping is given by a table of sorted values x and associated y which are
 *  interpolated linearly. A data mapping object must be capable of generating 
 *  such a table. 
 *  Data mapping expressions can be built by concatenating data mapping operators.
 */
class TL_PUBLIC DataMappingBase
{
public:
  /**
   *  @brief The constructor
   */
  DataMappingBase () { }

  /**
   *  @brief The destructor
   */
  virtual ~DataMappingBase () { }

  /**
   *  @brief Get the minimum x value for the data mapping
   *
   *  Any reimplementation of this class must provide the minimum value for the 
   *  x value that is data mapping is covering. 
   */
  virtual double xmin () const = 0;

  /**
   *  @brief Get the maximum x value for the data mapping
   *
   *  Any reimplementation of this class must provide the maximum value for the 
   *  x value that is data mapping is covering. 
   */
  virtual double xmax () const = 0;

  /**
   *  @brief Create the table
   *
   *  Any reimplementation of this class must provide a list of x/y pairs sorted by x.
   *  Mapping for x values outside this range is performed by constant extrapolation.
   */
  virtual void generate_table (std::vector< std::pair<double, double> > &table) = 0;

  /**
   *  @brief Dump the contents
   */
  virtual void dump () const = 0;
};

/**
 *  @brief A table-based data mapping
 * 
 *  The table is to be built by adding new entries, whose x values must be sorted.
 */
class TL_PUBLIC TableDataMapping
  : public DataMappingBase
{
public:
  /**
   *  @brief The constructor
   */
  TableDataMapping () : m_xmin (0.0), m_xmax (0.0) { }

  /**
   *  @brief The destructor
   */
  virtual ~TableDataMapping () { }

  /**
   *  @brief Get the minimum x value for the data mapping
   *
   *  Any reimplementation of this class must provide the minimum value for the 
   *  x value that is data mapping is covering. 
   */
  virtual double xmin () const 
  {
    return m_xmin;
  }

  /**
   *  @brief Get the maximum x value for the data mapping
   *
   *  Any reimplementation of this class must provide the maximum value for the 
   *  x value that is data mapping is covering. 
   */
  virtual double xmax () const 
  {
    return m_xmax;
  }

  /**
   *  @brief Create the table
   *
   *  Any reimplementation of this class must provide a list of x/y pairs sorted by x.
   *  Mapping for x values outside this range is performed by constant extrapolation.
   */
  virtual void generate_table (std::vector< std::pair<double, double> > &table) 
  {
    table = m_table;
  }

  /**
   *  @brief Add a new entry
   */
  void push_back (double x, double y)
  {
    if (m_table.empty ()) {
      m_xmin = m_xmax = x;
    }

    if (m_xmin > x) {
      m_xmin = x;
    }
    if (m_xmax < x) {
      m_xmax = x;
    }

    m_table.push_back (std::make_pair (x, y));
  }

  /**
   *  @brief Dump the contents
   */
  virtual void dump () const;

public:
  double m_xmin, m_xmax;
  std::vector< std::pair<double, double> > m_table;
};

/**
 *  @brief Combination of two data mappings
 *
 *  This data mapping is composed of two mappings o and i where o is the outer
 *  and i the inner mapping. The result is the application of i and then o: 
 *  y = o(i(x)). 
 */
class TL_PUBLIC CombinedDataMapping 
  : public DataMappingBase
{
public:
  /**
   *  @brief Create a combined data mapping
   *  
   *  @param o The outer data mapping. This object will be owned by CombinedDataMapping.
   *  @param i The inner data mapping. This object will be owned by CombinedDataMapping.
   */
  CombinedDataMapping (DataMappingBase *o, DataMappingBase *i);

  /**
   *  @brief Destructor
   */
  virtual ~CombinedDataMapping ();

  /**
   *  @brief Get the minimum x value for the data mapping
   *
   *  Implementation of the DataMappingBase interface.
   */
  double xmin () const;

  /**
   *  @brief Get the maximum x value for the data mapping
   *
   *  Implementation of the DataMappingBase interface.
   */
  double xmax () const;

  /**
   *  @brief Create the table
   *
   *  Implementation of the DataMappingBase interface.
   */
  void generate_table (std::vector< std::pair<double, double> > &table);

  /**
   *  @brief Dump the contents
   */
  virtual void dump () const;

private:
  DataMappingBase *mp_o, *mp_i;
};

/**
 *  @brief Linear combination of two data mappings
 *
 *  This data mapping is composed of two mappings a and b and offers two coefficients ca and cb and a constant c. 
 *  The result is:
 *  y = ca * a(x) + cb * b(x) + c.
 */
class TL_PUBLIC LinearCombinationDataMapping 
  : public DataMappingBase
{
public:
  /**
   *  @brief Create a combined data mapping
   *  
   *  @param a The first data mapping. This object will be owned by CombinedDataMapping.
   *  @param ca The first coefficient
   *  @param b The second data mapping. This object will be owned by CombinedDataMapping. This parameter can be 0.
   *  @param cb The first coefficient
   *  @param c The constant.
   *
   *  If the b object is 0, the linear combination degenerates to y = ca * a(x) + c.
   *  If the a and b object is 0, the linear combination degenerates to y = c.
   */
  LinearCombinationDataMapping (double c, DataMappingBase *a = 0, double ca = 1.0, DataMappingBase *b = 0, double cb = 1.0);

  /**
   *  @brief Destructor
   */
  virtual ~LinearCombinationDataMapping ();

  /**
   *  @brief Get the minimum x value for the data mapping
   *
   *  Implementation of the DataMappingBase interface.
   */
  double xmin () const;

  /**
   *  @brief Get the maximum x value for the data mapping
   *
   *  Implementation of the DataMappingBase interface.
   */
  double xmax () const;

  /**
   *  @brief Create the table
   *
   *  Implementation of the DataMappingBase interface.
   */
  void generate_table (std::vector< std::pair<double, double> > &table);

  /**
   *  @brief Dump the contents
   */
  virtual void dump () const;

private:
  DataMappingBase *mp_a, *mp_b;
  double m_ca, m_cb, m_c;
};

/**
 *  @brief Create a lookup table from a data mapping 
 *
 *  This class receives a data mapping object (and will own it) and
 *  is capable of creating a fast lookup table for the given granularity 
 *  of result values. 
 */
class TL_PUBLIC DataMappingLookupTable
{
public:
  /**
   *  @brief Instantiate a data mapping table object
   *
   *  @param The underlying data mapping - will become owned by the DataMappingLookupTable object.
   */
  DataMappingLookupTable (DataMappingBase *dm = 0);

  /**
   *  @brief Destructor
   */
  virtual ~DataMappingLookupTable ();

  /**
   *  @brief Update the lookup table
   * 
   *  This method must be called to produce a lookup table with the given granularity (resolution)
   *  of y values and the given x range. 
   *  This method must be called when the data mapping base object has been changed.
   *
   *  @param xmin The minimum x value covered. No x value below this value can be mapped.
   *  @param xmax The maximum x value covered. No x value above this value can be mapped.
   *  @param delta_y The y granularity. This determines the resolution of the lookup table.
   *  @param ifactor the factor by which the integer table entries are multiplied
   */
  void update_table (double xmin, double xmax, double delta_y, unsigned int ifactor);

  /**
   *  @brief Actually do the mapping
   *
   *  This method is optimized for performance and does not check for 
   *  the table actually being created or over- or underflow.
   *  Before this method can be called, update_table() must have been called.
   */
  inline double operator[] (double x) const
  {
    size_t i = size_t ((x - m_xmin) * m_dxinv);
    return mp_y[i];
  }

  /**
   *  @brief Actually do the mapping to unsigned char
   *
   *  This method is optimized for performance and does not check for 
   *  the table actually being created or over- or underflow.
   *  Before this method can be called, update_table() must have been called.
   */
  inline unsigned int operator() (double x) const
  {
    size_t i = size_t ((x - m_xmin) * m_dxinv);
    return mp_c[i];
  }

  /**
   *  @brief Assignment of a new data mapping
   */
  void set_data_mapping (DataMappingBase *dm);

  /**
   *  @brief Dump the table (for test purposes)
   */
  std::string dump () const;

private:
  double m_dxinv;
  double m_xmin;
  double *mp_y;
  unsigned int *mp_c;
  size_t m_size;
  DataMappingBase *mp_dm;

  //  no copy ctor
  DataMappingLookupTable (const DataMappingLookupTable &d);
  DataMappingLookupTable &operator= (const DataMappingLookupTable &d);

  void release ();
};

}

#endif

