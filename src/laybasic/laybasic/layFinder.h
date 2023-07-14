
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


#ifndef HDR_layFinder
#define HDR_layFinder

#include "laybasicCommon.h"

#include <vector>

#include "tlVector.h"
#include "layLayoutViewBase.h"
#include "dbBoxConvert.h"
#include "dbLayout.h"
#include "dbBox.h"
#include "dbShape.h"
#include "layObjectInstPath.h"

namespace tl
{
  class AbsoluteProgress;
}

namespace lay
{

class TextInfo;

/**
 *  @brief A generic finder class
 *
 *  A finder traverses the hierarchy and calls the virtual "find"
 *  method on each cell.
 */
class LAYBASIC_PUBLIC Finder
{
public:
  /**
   *  @brief Constructor
   *
   *  The point_mode is true, if the finder is supposed to operate in "point mode".
   *  In point mode, the center of the search region is the reference point. In 
   *  non-point mode, every relevant found inside the search region will be
   *  recorded (also see point_mode method).
   *  The base class implementation just stores this flag and provides a read
   *  accessor with the point_mode () method.
   */
  Finder (bool point_mode, bool top_level_sel);

  /**
   *  @brief Gets a flag indicating whether point mode is enabled
   *  If point mode is enabled in the constructor, the first will check for objects overlapping the
   *  point (rather than being inside the box) and by default select a single object only.
   *  See also "set_catch_all".
   */
  bool point_mode () const
  {
    return m_point_mode;
  }

  /**
   *  @brief Gets a flag indicating the capture all founds even in point mode
   */
  bool catch_all () const
  {
    return m_catch_all;
  }

  /**
   *  @brief Sets a flag indicating the capture all founds even in point mode
   *  By default, in point mode only the closest found is returned. To catch all
   *  founds in point mode too, set this flag to true.
   */
  void set_catch_all (bool f)
  {
    m_catch_all = f;
  }

  /**
   *  @brief Destructor (just provided to please the compiler)
   */
  virtual ~Finder ();

  /**
   *  @brief Proximity getter
   *
   *  The "proximity" is the closest value passed to the "closer" method.
   *  It returns std::numeric_limits<double>::max () if nothing was found.
   */
  double proximity () const
  {
    return m_distance;
  }
  
protected:
  const std::vector<int> &layers () const
  {
    return m_layers;
  }

  const std::vector<db::InstElement> &path () const
  {
    return m_path;
  }

  const db::Layout &layout () const
  {
    return *mp_layout;
  }

  int min_level () const
  {
    return m_min_level;
  }

  int max_level () const
  {
    return m_max_level;
  }

  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  bool closer (double d);

  /** 
   *  @brief Start the scan with the given parameters
   *
   *  Starts the cell scan on the given layout object, with the given region,
   *  starting at the given cell, with the given range of hierarchy levels to 
   *  consider and using just the given layer or layers (unless the vector is empty, in which case all layers
   *  are used). For each matching cell, the "visit_cell" method is called. A 
   *  path of instantiations up to the top cell is maintained and accessible by
   *  the path() accessor.
   *
   *  @param view The layout view to run the scan on
   *  @param cv_index The cell view to run the scan on
   *  @param trans A set of visual transformations applied to the display (layer properties transformations) in micron space
   *  @param region The hit region which the object is checked against
   *  @param scan_region The region where the object is looked up (can be bigger than the hit region for visual label box detection)
   *  @param min_level The minimum hierarchy level to check
   *  @param max_level The maximum hierarchy level to check
   *  @param layers A set of layers to check
   */
  void start (LayoutViewBase *view, unsigned int cv_index, const std::vector<db::DCplxTrans> &trans, const db::DBox &region, const db::DBox &scan_region, int min_level, int max_level, const std::vector<int> &layers = std::vector<int> ());

  /**
   *  @brief Provide a basic edge test facility
   *
   *  This method computes a "distance" of the edge to the reference point (the
   *  center of the search region). It updates "distance" if the computed distance
   *  is less than the one stored in "distance" or "match" is false. If the 
   *  distance is updated, match is set to true.
   *
   *  "trans" is the transformation to be applied to the edge before the test.
   *
   *  If "points" is true, only points are tested, otherwise edges are tested.
   *
   *  This method returns a mask indicating which point of the edge was matching.
   *  Bit 0 of this mask indicates the first point is matching, bit 1 indicates the
   *  second point is matching.
   */
  unsigned int test_edge (const db::ICplxTrans &trans, const db::Edge &edge, bool points, double &distance, bool &match);

  /**
   *  @brief Tests an edge in point mode and edge mode (later)
   */
  void test_edge (const db::ICplxTrans &trans, const db::Edge &edge, double &distance, bool &match);

private:
  void do_find (const db::Cell &cell, int level, const db::DCplxTrans &vp, const db::ICplxTrans &t);

  /**
   *  @brief Visitor sugar function
   *
   *  This method is supposed to do whatever the finder is supposed to do on the
   *  cell. It may use the "closer" method to determine if something is closer
   *  to whatever.
   */
  virtual void visit_cell (const db::Cell &cell, const db::Box &hit_box, const db::Box &scan_box, const db::DCplxTrans &vp, const db::ICplxTrans &t, int level) = 0;

  int m_min_level, m_max_level;
  std::vector<db::InstElement> m_path;
  const db::Layout *mp_layout;
  lay::LayoutViewBase *mp_view;
  unsigned int m_cv_index;
  db::Box m_region;
  db::Box m_scan_region;
  std::vector<int> m_layers;
  double m_distance;
  bool m_point_mode;
  bool m_catch_all;
  bool m_top_level_sel;
  db::box_convert <db::CellInst> m_box_convert;
  db::box_convert <db::Cell> m_cell_box_convert;
};

/**
 *  @brief Shape finder utility class
 *
 *  This class specializes the finder to finding shapes.
 */
class LAYBASIC_PUBLIC ShapeFinder
  : public Finder
{
public:
  struct StopException { };

  typedef std::vector<lay::ObjectInstPath> founds_vector_type;
  typedef founds_vector_type::const_iterator iterator;

  ShapeFinder (bool point_mode, bool top_level_sel, db::ShapeIterator::flags_type flags, const std::set<lay::ObjectInstPath> *excludes = 0);

  bool find (LayoutViewBase *view, const lay::LayerProperties &lprops, const db::DBox &region_mu);
  bool find (LayoutViewBase *view, const db::DBox &region_mu);

  iterator begin () const
  {
    return m_founds.begin ();
  }

  iterator end () const
  {
    return m_founds.end ();
  }

protected:
  db::ShapeIterator::flags_type flags () const 
  {
    return m_flags;
  }

  const lay::TextInfo *text_info () const
  {
    return mp_text_info;
  }

  unsigned int cv_index () const
  {
    return m_cv_index;
  }

  const std::set<db::properties_id_type> *prop_sel () const 
  {
    return mp_prop_sel;
  }

  bool inv_prop_sel () const 
  {
    return m_inv_prop_sel;
  }

  db::cell_index_type topcell () const
  {
    return m_topcell;
  }

  void set_test_count (int n)
  {
    m_tries = n;
  }

  void checkpoint ();

private:
  virtual void visit_cell (const db::Cell &cell, const db::Box &hit_box, const db::Box &scan_box, const db::DCplxTrans &vp, const db::ICplxTrans &t, int level);

  bool find_internal (LayoutViewBase *view,
                      unsigned int cv_index,
                      const std::set<db::properties_id_type> *prop_sel,
                      bool inv_prop_sel,
                      const lay::HierarchyLevelSelection &hier_sel,
                      const std::vector<db::DCplxTrans> &trans_mu,
                      const std::vector<int> &layers,
                      const db::DBox &region_mu);

  const std::set<lay::ObjectInstPath> *mp_excludes;
  std::vector<lay::ObjectInstPath> m_founds;
  db::ShapeIterator::flags_type m_flags;
  unsigned int m_cv_index;
  db::cell_index_type m_topcell;
  const lay::TextInfo *mp_text_info;
  const std::set<db::properties_id_type> *mp_prop_sel;
  bool m_inv_prop_sel;
  int m_tries;
  tl::AbsoluteProgress *mp_progress;
  std::vector<int> m_context_layers;
  std::map<db::cell_index_type, bool> m_cells_with_context;
};

/**
 *  @brief Instance finder utility class
 *
 *  This class specializes the finder to finding instanced.
 */
class LAYBASIC_PUBLIC InstFinder
  : public lay::Finder
{
public:
  struct StopException { };

  typedef std::vector<lay::ObjectInstPath> founds_vector_type;
  typedef founds_vector_type::const_iterator iterator;

  InstFinder (bool point_mode, bool top_level_sel, bool full_arrays, bool enclose_inst = true, const std::set<lay::ObjectInstPath> *excludes = 0, bool visible_layers = false);

  bool find (LayoutViewBase *view, unsigned int cv_index, const db::DCplxTrans &trans, const db::DBox &region_mu);
  bool find (LayoutViewBase *view, const db::DBox &region_mu);
 
  iterator begin () const
  {
    return m_founds.begin ();
  }

  iterator end () const
  {
    return m_founds.end ();
  }

private:
  virtual void visit_cell (const db::Cell &cell, const db::Box &hit_box, const db::Box &scan_box, const db::DCplxTrans &vp, const db::ICplxTrans &t, int level);

  bool find_internal (LayoutViewBase *view, unsigned int cv_index, const db::DCplxTrans &trans_mu, const db::DBox &region_mu);

  unsigned int m_cv_index;
  db::cell_index_type m_topcell;
  const std::set<lay::ObjectInstPath> *mp_excludes;
  std::vector<lay::ObjectInstPath> m_founds;
  int m_tries;
  bool m_full_arrays;
  bool m_enclose_insts;
  bool m_visible_layers;
  std::vector<int> m_visible_layer_indexes;
  LayoutViewBase *mp_view;
  tl::AbsoluteProgress *mp_progress;
};

}

#endif

