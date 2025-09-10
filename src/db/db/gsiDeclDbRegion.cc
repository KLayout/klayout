
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

#include "gsiDecl.h"
#include "gsiEnums.h"

#include "dbRegion.h"
#include "dbRegionUtils.h"
#include "dbDeepRegion.h"
#include "dbOriginalLayerRegion.h"
#include "dbPolygonTools.h"
#include "dbLayoutUtils.h"
#include "dbShapes.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbFillTool.h"
#include "dbRegionProcessors.h"
#include "dbCompoundOperation.h"
#include "dbLayoutToNetlist.h"
#include "dbPropertiesRepository.h"
#include "dbPropertiesFilter.h"
#include "tlGlobPattern.h"

#include "gsiDeclDbContainerHelpers.h"
#include "gsiDeclDbMeasureHelpers.h"

#include <memory>
#include <vector>
#include <set>

namespace gsi
{

// ---------------------------------------------------------------------------------
//  PolygonFilter binding

typedef shape_filter_impl<db::AllMustMatchFilter> PolygonFilterBase;

class PolygonFilterImpl
  : public PolygonFilterBase
{
public:
  PolygonFilterImpl () { }

  bool issue_selected (const db::PolygonWithProperties &) const
  {
    return false;
  }

  virtual bool selected (const db::Polygon &polygon, db::properties_id_type prop_id) const
  {
    if (f_selected.can_issue ()) {
      return f_selected.issue<PolygonFilterImpl, bool, const db::PolygonWithProperties &> (&PolygonFilterImpl::issue_selected, db::PolygonWithProperties (polygon, prop_id));
    } else {
      return issue_selected (polygon);
    }
  }

  virtual bool selected (const db::PolygonRef &polygon, db::properties_id_type prop_id) const
  {
    db::Polygon p;
    polygon.instantiate (p);
    return selected (p, prop_id);
  }

  gsi::Callback f_selected;

private:
  //  No copying
  PolygonFilterImpl &operator= (const PolygonFilterImpl &);
  PolygonFilterImpl (const PolygonFilterImpl &);
};

typedef db::polygon_properties_filter<gsi::PolygonFilterBase> PolygonPropertiesFilter;

static gsi::PolygonFilterBase *make_ppf1 (const tl::Variant &name, const tl::Variant &value, bool inverse)
{
  return new PolygonPropertiesFilter (name, value, inverse);
}

static gsi::PolygonFilterBase *make_ppf2 (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
{
  return new PolygonPropertiesFilter (name, from, to, inverse);
}

static gsi::PolygonFilterBase *make_pg (const tl::Variant &name, const std::string &glob, bool inverse, bool case_sensitive)
{
  tl::GlobPattern pattern (glob);
  pattern.set_case_sensitive (case_sensitive);
  return new PolygonPropertiesFilter (name, pattern, inverse);
}

static gsi::PolygonFilterBase *make_pe (const std::string &expression, bool inverse, const std::map<std::string, tl::Variant> &variables, double dbu)
{
  return new gsi::expression_filter<gsi::PolygonFilterBase, db::Region> (expression, inverse, dbu, variables);
}

Class<gsi::PolygonFilterBase> decl_PolygonFilterBase ("db", "PolygonFilterBase",
  gsi::PolygonFilterBase::method_decls (true) +
  gsi::constructor ("property_glob", &make_pg, gsi::arg ("name"), gsi::arg ("pattern"), gsi::arg ("inverse", false), gsi::arg ("case_sensitive", true),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The glob pattern to match the property value against.\n"
    "@param inverse If true, inverts the selection - i.e. all polygons without a matching property are selected.\n"
    "@param case_sensitive If true, the match is case sensitive (the default), if false, the match is not case sensitive.\n"
    "\n"
    "Apply this filter with \\Region#filtered:\n"
    "\n"
    "@code\n"
    "# region is a Region object\n"
    "# filtered_region contains all polygons where the 'net' property starts with 'C':\n"
    "filtered_region = region.filtered(RBA::PolygonFilterBase::property_glob('net', 'C*'))\n"
    "@/code\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter", &make_ppf1, gsi::arg ("name"), gsi::arg ("value"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The value against which the property is checked (exact match).\n"
    "@param inverse If true, inverts the selection - i.e. all polygons without a property with the given name and value are selected.\n"
    "\n"
    "Apply this filter with \\Region#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter_bounded", &make_ppf2, gsi::arg ("name"), gsi::arg ("from"), gsi::arg ("to"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param from The lower value against which the property is checked or 'nil' if no lower bound shall be used.\n"
    "@param to The upper value against which the property is checked or 'nil' if no upper bound shall be used.\n"
    "@param inverse If true, inverts the selection - i.e. all polygons without a property with the given name and value range are selected.\n"
    "\n"
    "This version does a bounded match. The value of the propery needs to be larger or equal to 'from' and less than 'to'.\n"
    "Apply this filter with \\Region#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("expression_filter", &make_pe, gsi::arg ("expression"), gsi::arg ("inverse", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates an expression-based filter\n"
    "@param expression The expression to evaluate.\n"
    "@param inverse If true, inverts the selection - i.e. all polygons without a property with the given name and value range are selected.\n"
    "@param dbu If given and greater than zero, the shapes delivered by the 'shape' function will be in micrometer units.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
    "\n"
    "Creates a filter that will evaluate the given expression on every shape and select the shape "
    "when the expression renders a boolean true value. "
    "The expression may use the following variables and functions:\n"
    "\n"
    "@ul\n"
    "@li @b shape @/b: The current shape (i.e. 'Polygon' without DBU specified or 'DPolygon' otherwise) @/li\n"
    "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
    "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
    "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
    "@/ul\n"
    "\n"
    "This feature has been introduced in version 0.30.3."
  ),
  "@hide"
);

Class<gsi::PolygonFilterImpl> decl_PolygonFilterImpl (decl_PolygonFilterBase, "db", "PolygonFilter",
  callback ("selected", &PolygonFilterImpl::issue_selected, &PolygonFilterImpl::f_selected, gsi::arg ("polygon"),
    "@brief Selects a polygon\n"
    "This method is the actual payload. It needs to be reimplemented in a derived class.\n"
    "It needs to analyze the polygon and return 'true' if it should be kept and 'false' if it should be discarded.\n"
    "\n"
    "Since version 0.30, the polygon carries properties."
  ),
  "@brief A generic polygon filter adaptor\n"
  "\n"
  "Polygon filters are an efficient way to filter polygons from a Region. To apply a filter, derive your own "
  "filter class and pass an instance to the \\Region#filter or \\Region#filtered method.\n"
  "\n"
  "Conceptually, these methods take each polygon from the region and present it to the filter's 'selected' method.\n"
  "Based on the result of this evaluation, the polygon is kept or discarded.\n"
  "\n"
  "The magic happens when deep mode regions are involved. In that case, the filter will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the filter behaves. You "
  "need to configure the filter by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using the filter.\n"
  "\n"
  "You can skip this step, but the filter algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that filters triangles:"
  "\n"
  "@code\n"
  "class TriangleFilter < RBA::PolygonFilter\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.is_isotropic_and_scale_invariant   # the triangle nature is not dependent on the scale or orientation\n"
  "  end\n"
  "  \n"
  "  # Select only triangles\n"
  "  def selected(polygon)\n"
  "    return polygon.holes == 0 && polygon.num_points == 3\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "region = ... # some Region\n"
  "triangles_only = region.filtered(TriangleFilter::new)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  PolygonProcessor binding

Class<db::PolygonProcessorBase> decl_PolygonProcessorBase ("db", "PolygonProcessorBase", "@hide");

Class<shape_processor_impl<db::PolygonProcessorBase> > decl_PolygonOperator (decl_PolygonProcessorBase, "db", "PolygonOperator",
  shape_processor_impl<db::PolygonProcessorBase>::method_decls (true),
  "@brief A generic polygon operator\n"
  "\n"
  "Polygon processors are an efficient way to process polygons from a Region. To apply a processor, derive your own "
  "operator class and pass an instance to the \\Region#process or \\Region#processed method.\n"
  "\n"
  "Conceptually, these methods take each polygon from the region and present it to the operators' 'process' method.\n"
  "The result of this call is a list of zero to many output polygons derived from the input polygon.\n"
  "The output region is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode regions are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that shrinks every polygon to half of the size but does not change the position.\n"
  "In this example the 'position' is defined by the center of the bounding box:"
  "\n"
  "@code\n"
  "class ShrinkToHalf < RBA::PolygonOperator\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.is_isotropic_and_scale_invariant   # scale or orientation do not matter\n"
  "  end\n"
  "  \n"
  "  # Shrink to half size\n"
  "  def process(polygon)\n"
  "    shift = polygon.bbox.center - RBA::Point::new   # shift vector\n"
  "    return [ (polygon.moved(-shift) * 0.5).moved(shift) ]\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "region = ... # some Region\n"
  "shrinked_to_half = region.processed(ShrinkToHalf::new)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

static
property_computation_processor<db::PolygonProcessorBase, db::Region> *
new_pcp (const db::Region *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  return new property_computation_processor<db::PolygonProcessorBase, db::Region> (container, expressions, copy_properties, dbu, variables);
}

property_computation_processor<db::PolygonProcessorBase, db::Region> *
new_pcps (const db::Region *container, const std::string &expression, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  std::map<tl::Variant, std::string> expressions;
  expressions.insert (std::make_pair (tl::Variant (), expression));
  return new property_computation_processor<db::PolygonProcessorBase, db::Region> (container, expressions, copy_properties, dbu, variables);
}

Class<property_computation_processor<db::PolygonProcessorBase, db::Region> > decl_PolygonPropertiesExpressions (decl_PolygonProcessorBase, "db", "PolygonPropertiesExpressions",
  property_computation_processor<db::PolygonProcessorBase, db::Region>::method_decls (true) +
  gsi::constructor ("new", &new_pcp, gsi::arg ("region"), gsi::arg ("expressions"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param region The region, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
    "@param expressions A map of property names and expressions used to generate the values of the properties (see class description for details).\n"
    "@param copy_properties If true, new properties will be added to existing ones.\n"
    "@param dbu If not zero, this value specifies the database unit to use. If given, the shapes returned by the 'shape' function will be micrometer-unit objects.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
  ) +
  gsi::constructor ("new", &new_pcps, gsi::arg ("region"), gsi::arg ("expression"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param region The region, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
    "@param expression A single expression evaluated for each shape (see class description for details).\n"
    "@param copy_properties If true, new properties will be added to existing ones.\n"
    "@param dbu If not zero, this value specifies the database unit to use. If given, the shapes returned by the 'shape' function will be micrometer-unit objects.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
  ),
  "@brief An operator attaching computed properties to the edge pairs\n"
  "\n"
  "This operator will execute a number of expressions and attach the results as new properties. "
  "The expression inputs can be taken either from the edges themselves or from existing properties.\n"
  "\n"
  "A number of expressions can be supplied with a name. The expressions will be evaluated and the result "
  "is attached to the output edge pairs as user properties with the given names.\n"
  "\n"
  "Alternatively, a single expression can be given. In that case, 'put' needs to be used to attach properties "
  "to the output shape. You can also use 'skip' to drop shapes in that case.\n"
  "\n"
  "The expression may use the following variables and functions:\n"
  "\n"
  "@ul\n"
  "@li @b shape @/b: The current shape (i.e. 'Polygon' without DBU specified or 'DPolygon' otherwise) @/li\n"
  "@li @b put(<name>, <value>) @/b: Attaches the given value as a property with name 'name' to the output shape @/li\n"
  "@li @b skip(<flag>) @/b: If called with a 'true' value, the shape is dropped from the output @/li\n"
  "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
  "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
  "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
  "@/ul\n"
  "\n"
  "This class has been introduced in version 0.30.3.\n"
);

Class<db::PolygonToEdgeProcessorBase> decl_PolygonToEdgeProcessorBase ("db", "PolygonToEdgeProcessorBase", "@hide");

Class<shape_processor_impl<db::PolygonToEdgeProcessorBase> > decl_PolygonToEdgeProcessor (decl_PolygonToEdgeProcessorBase, "db", "PolygonToEdgeOperator",
  shape_processor_impl<db::PolygonToEdgeProcessorBase>::method_decls (true),
  "@brief A generic polygon-to-edge operator\n"
  "\n"
  "Polygon processors are an efficient way to process polygons from a Region. To apply a processor, derive your own "
  "operator class and pass an instance to the \\Region#processed method.\n"
  "\n"
  "Conceptually, these methods take each polygon from the region and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output edges derived from the input polygon.\n"
  "The output edge collection is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode regions are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "For a basic example see the \\PolygonOperator class, with the exception that this incarnation has to deliver edges.\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

Class<db::PolygonToEdgePairProcessorBase> decl_PolygonToEdgePairProcessorBase ("db", "PolygonToEdgePairProcessorBase", "@hide");

Class<shape_processor_impl<db::PolygonToEdgePairProcessorBase> > decl_PolygonToEdgePairProcessor (decl_PolygonToEdgePairProcessorBase, "db", "PolygonToEdgePairOperator",
  shape_processor_impl<db::PolygonToEdgePairProcessorBase>::method_decls (true),
  "@brief A generic polygon-to-edge-pair operator\n"
  "\n"
  "Polygon processors are an efficient way to process polygons from a Region. To apply a processor, derive your own "
  "operator class and pass an instance to the \\Region#processed method.\n"
  "\n"
  "Conceptually, these methods take each polygon from the region and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output edge pairs derived from the input polygon.\n"
  "The output edge pair collection is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode regions are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "For a basic example see the \\PolygonOperator class, with the exception that this incarnation has to deliver edge pairs.\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  Region binding

static inline std::vector<db::Region> as_2region_vector (const std::pair<db::Region, db::Region> &rp)
{
  std::vector<db::Region> res;
  res.reserve (2);
  res.push_back (db::Region (const_cast<db::Region &> (rp.first).take_delegate ()));
  res.push_back (db::Region (const_cast<db::Region &> (rp.second).take_delegate ()));
  return res;
}

static db::Region *new_v ()
{
  return new db::Region ();
}

static db::Region *new_a (const std::vector <db::Polygon> &a)
{
  return new db::Region (a.begin (), a.end ());
}

static db::Region *new_ap (const std::vector <db::PolygonWithProperties> &a, bool)
{
  return new db::Region (a.begin (), a.end ());
}

static db::Region *new_b (const db::Box &o)
{
  return new db::Region (o);
}

static db::Region *new_bp (const db::BoxWithProperties &o)
{
  return new db::Region (o);
}

static db::Region *new_p (const db::Polygon &o)
{
  return new db::Region (o);
}

static db::Region *new_pp (const db::PolygonWithProperties &o)
{
  return new db::Region (o);
}

static db::Region *new_ps (const db::SimplePolygon &o)
{
  return new db::Region (o);
}

static db::Region *new_psp (const db::SimplePolygonWithProperties &o)
{
  return new db::Region (o);
}

static db::Region *new_path (const db::Path &o)
{
  return new db::Region (o);
}

static db::Region *new_pathp (const db::PathWithProperties &o)
{
  return new db::Region (o);
}

static db::Region *new_texts_as_boxes1 (const db::RecursiveShapeIterator &si, const std::string &pat, bool pattern, db::Coord enl)
{
  return new db::Region (db::Region (si).texts_as_boxes (pat, pattern, enl));
}

static db::Region *new_texts_as_boxes2 (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const std::string &pat, bool pattern, db::Coord enl)
{
  return new db::Region (db::Region (si).texts_as_boxes (pat, pattern, enl, dss));
}

static db::Edges *texts_as_dots1 (const db::Region *r, const std::string &pat, bool pattern)
{
  return new db::Edges (r->texts_as_dots (pat, pattern));
}

static db::Edges *texts_as_dots2 (const db::Region *r, db::DeepShapeStore &dss, const std::string &pat, bool pattern)
{
  return new db::Edges (r->texts_as_dots (pat, pattern, dss));
}

static db::Region *texts_as_boxes1 (const db::Region *r, const std::string &pat, bool pattern, db::Coord enl)
{
  return new db::Region (r->texts_as_boxes (pat, pattern, enl));
}

static db::Region *texts_as_boxes2 (const db::Region *r, db::DeepShapeStore &dss, const std::string &pat, bool pattern, db::Coord enl)
{
  return new db::Region (r->texts_as_boxes (pat, pattern, enl, dss));
}

static db::Edges corners_to_dots (const db::Region *r, double angle_start, double angle_end, bool include_angle_start, bool include_angle_end, bool inverse, bool absolute)
{
  return r->processed (db::CornersAsDots (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute));
}

static db::Region corners_to_boxes (const db::Region *r, double angle_start, double angle_end, db::Coord dim, bool include_angle_start, bool include_angle_end, bool inverse, bool absolute)
{
  return r->processed (db::CornersAsRectangles (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute, dim));
}

static db::EdgePairs corners_to_edge_pairs (const db::Region *r, double angle_start, double angle_end, bool include_angle_start, bool include_angle_end, bool inverse, bool absolute)
{
  return r->processed (db::CornersAsEdgePairs (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute));
}

static db::Region *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::Region (si);
}

static db::Region *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::Region (si, trans);
}

static db::Region *new_sis (const db::Shapes &si)
{
  return new db::Region (si);
}

static db::Region *new_sis2 (const db::Shapes &si, const db::ICplxTrans &trans)
{
  return new db::Region (si, trans);
}

static db::Region *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
{
  return new db::Region (si, dss, area_ratio, max_vertex_count);
}

static db::Region *new_sid2 (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans, double area_ratio, size_t max_vertex_count)
{
  return new db::Region (si, dss, trans, true, area_ratio, max_vertex_count);
}

static std::string to_string0 (const db::Region *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::Region *r, size_t n)
{
  return r->to_string (n);
}

static db::Region::area_type area1 (const db::Region *r)
{
  return r->area ();
}

static db::Region::area_type area2 (const db::Region *r, const db::Box &rect)
{
  return r->area (rect);
}

static db::Region::perimeter_type perimeter1 (const db::Region *r)
{
  return r->perimeter ();
}

static db::Region::perimeter_type perimeter2 (const db::Region *r, const db::Box &rect)
{
  return r->perimeter (rect);
}

static void insert_a (db::Region *r, const std::vector <db::Polygon> &a)
{
  for (std::vector <db::Polygon>::const_iterator p = a.begin (); p != a.end (); ++p) {
    r->insert (*p);
  }
}

static void insert_ap (db::Region *r, const std::vector <db::PolygonWithProperties> &a)
{
  for (std::vector <db::PolygonWithProperties>::const_iterator p = a.begin (); p != a.end (); ++p) {
    r->insert (*p);
  }
}

static void insert_r (db::Region *r, const db::Region &a)
{
  for (db::Region::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    r->insert (*p);
  }
}

template <class Trans>
static void insert_st (db::Region *r, const db::Shapes &a, const Trans &t)
{
  for (db::Shapes::shape_iterator p = a.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes | db::ShapeIterator::Paths); !p.at_end (); ++p) {
    db::Polygon poly;
    p->polygon (poly);
    if (p->prop_id () != 0) {
      r->insert (db::PolygonWithProperties (poly.transformed (t), p->prop_id ()));
    } else {
      r->insert (poly.transformed (t));
    }
  }
}

static void insert_s (db::Region *r, const db::Shapes &a)
{
  insert_st (r, a, db::UnitTrans ());
}

static void insert_si (db::Region *r, db::RecursiveShapeIterator si)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), si.trans ());
    ++si;
  }
}

static void insert_si2 (db::Region *r, db::RecursiveShapeIterator si, db::ICplxTrans &trans)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), trans * si.trans ());
    ++si;
  }
}

static db::Region delaunay (const db::Region *r)
{
  db::TriangulationProcessor tri (0.0, 0.0);
  db::Region res = r->processed (tri);
  res.set_merged_semantics (false);
  return res;
}

static db::Region refined_delaunay (const db::Region *r, double max_area, double min_b)
{
  db::TriangulationProcessor tri (max_area, min_b);
  db::Region res = r->processed (tri);
  res.set_merged_semantics (false);
  return res;
}

static db::Region drc_hull (const db::Region *r, db::metrics_type metrics, db::Coord space, size_t n_circle)
{
  return r->processed (db::DRCHullProcessor (space, metrics, n_circle));
}

static db::Region minkowski_sum_pe (const db::Region *r, const db::Edge &e)
{
  return r->processed (db::minkowski_sum_computation<db::Edge> (e));
}

static db::Region minkowski_sum_pp (const db::Region *r, const db::Polygon &q)
{
  return r->processed (db::minkowski_sum_computation<db::Polygon> (q));
}

static db::Region minkowski_sum_pb (const db::Region *r, const db::Box &q)
{
  return r->processed (db::minkowski_sum_computation<db::Box> (q));
}

static db::Region minkowski_sum_pc (const db::Region *r, const std::vector<db::Point> &q)
{
  return r->processed (db::minkowski_sum_computation<std::vector<db::Point> > (q));
}

static db::Region &move_p (db::Region *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::Region &move_xy (db::Region *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::Region moved_p (const db::Region *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::Region moved_xy (const db::Region *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region extents2 (const db::Region *r, db::Coord dx, db::Coord dy)
{
  return r->processed (db::extents_processor<db::Polygon> (dx, dy));
}

static db::Region extents1 (const db::Region *r, db::Coord d)
{
  return extents2 (r, d, d);
}

static db::Region extents0 (const db::Region *r)
{
  return extents2 (r, 0, 0);
}

static db::Region extent_refs (const db::Region *r, double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
{
  return r->processed (db::RelativeExtents (fx1, fy1, fx2, fy2, dx, dy));
}

static db::Edges extent_refs_edges (const db::Region *r, double fx1, double fy1, double fx2, double fy2)
{
  return r->processed (db::RelativeExtentsAsEdges (fx1, fy1, fx2, fy2));
}

static db::Region filtered (const db::Region *r, const PolygonFilterBase *f)
{
  return r->filtered (*f);
}

static void filter (db::Region *r, const PolygonFilterBase *f)
{
  r->filter (*f);
}

static std::vector<db::Region> split_filter (const db::Region *r, const PolygonFilterBase *f)
{
  return as_2region_vector (r->split_filter (*f));
}

static db::Region processed_pp (const db::Region *r, const db::PolygonProcessorBase *f)
{
  return r->processed (*f);
}

static void process_pp (db::Region *r, const db::PolygonProcessorBase *f)
{
  r->process (*f);
}

static db::EdgePairs processed_pep (const db::Region *r, const db::PolygonToEdgePairProcessorBase *f)
{
  return r->processed (*f);
}

static db::Edges processed_pe (const db::Region *r, const db::PolygonToEdgeProcessorBase *f)
{
  return r->processed (*f);
}

static db::Region with_perimeter1 (const db::Region *r, db::Region::perimeter_type perimeter, bool inverse)
{
  db::RegionPerimeterFilter f (perimeter, perimeter + 1, inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_perimeter1 (const db::Region *r, db::Region::perimeter_type perimeter)
{
  db::RegionPerimeterFilter f (perimeter, perimeter + 1, false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_perimeter2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionPerimeterFilter f (min.is_nil () ? db::Region::perimeter_type (0) : min.to<db::Region::perimeter_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_perimeter2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionPerimeterFilter f (min.is_nil () ? db::Region::perimeter_type (0) : min.to<db::Region::perimeter_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_area1 (const db::Region *r, db::Region::area_type area, bool inverse)
{
  db::RegionAreaFilter f (area, area + 1, inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_area1 (const db::Region *r, db::Region::area_type area)
{
  db::RegionAreaFilter f (area, area + 1, false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_area2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionAreaFilter f (min.is_nil () ? db::Region::area_type (0) : min.to<db::Region::area_type> (), max.is_nil () ? std::numeric_limits <db::Region::area_type>::max () : max.to<db::Region::area_type> (), inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_area2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionAreaFilter f (min.is_nil () ? db::Region::area_type (0) : min.to<db::Region::area_type> (), max.is_nil () ? std::numeric_limits <db::Region::area_type>::max () : max.to<db::Region::area_type> (), false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_holes1 (const db::Region *r, size_t n, bool inverse)
{
  db::HoleCountFilter f (n, n + 1, inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_holes1 (const db::Region *r, size_t n)
{
  db::HoleCountFilter f (n, n + 1, false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_holes2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::HoleCountFilter f (min.is_nil () ? size_t (0) : min.to<size_t> (), max.is_nil () ? std::numeric_limits <size_t>::max () : max.to<size_t> (), inverse);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_holes2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::HoleCountFilter f (min.is_nil () ? size_t (0) : min.to<size_t> (), max.is_nil () ? std::numeric_limits <size_t>::max () : max.to<size_t> (), false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_width1 (const db::Region *r, db::Region::distance_type bbox_width, bool inverse)
{
  db::RegionBBoxFilter f (bbox_width, bbox_width + 1, inverse, db::RegionBBoxFilter::BoxWidth);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_width1 (const db::Region *r, db::Region::distance_type bbox_width)
{
  db::RegionBBoxFilter f (bbox_width, bbox_width + 1, false, db::RegionBBoxFilter::BoxWidth);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_width2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxWidth);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_width2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), false, db::RegionBBoxFilter::BoxWidth);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_height1 (const db::Region *r, db::Region::distance_type bbox_height, bool inverse)
{
  db::RegionBBoxFilter f (bbox_height, bbox_height + 1, inverse, db::RegionBBoxFilter::BoxHeight);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_height1 (const db::Region *r, db::Region::distance_type bbox_height)
{
  db::RegionBBoxFilter f (bbox_height, bbox_height + 1, false, db::RegionBBoxFilter::BoxHeight);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_height2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxHeight);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_height2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), false, db::RegionBBoxFilter::BoxHeight);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_min1 (const db::Region *r, db::Region::distance_type bbox_min, bool inverse)
{
  db::RegionBBoxFilter f (bbox_min, bbox_min + 1, inverse, db::RegionBBoxFilter::BoxMinDim);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_min1 (const db::Region *r, db::Region::distance_type bbox_min)
{
  db::RegionBBoxFilter f (bbox_min, bbox_min + 1, false, db::RegionBBoxFilter::BoxMinDim);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_min2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxMinDim);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_min2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), false, db::RegionBBoxFilter::BoxMinDim);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_max1 (const db::Region *r, db::Region::distance_type bbox_max, bool inverse)
{
  db::RegionBBoxFilter f (bbox_max, bbox_max + 1, inverse, db::RegionBBoxFilter::BoxMaxDim);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_max1 (const db::Region *r, db::Region::distance_type bbox_max)
{
  db::RegionBBoxFilter f (bbox_max, bbox_max + 1, false, db::RegionBBoxFilter::BoxMaxDim);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_max2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxMaxDim);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_max2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), false, db::RegionBBoxFilter::BoxMaxDim);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_aspect_ratio1 (const db::Region *r, double v, bool inverse)
{
  db::RegionRatioFilter f (v, true, v, true, inverse, db::RegionRatioFilter::AspectRatio);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_aspect_ratio1 (const db::Region *r, double v)
{
  db::RegionRatioFilter f (v, true, v, true, false, db::RegionRatioFilter::AspectRatio);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_bbox_aspect_ratio2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, inverse, db::RegionRatioFilter::AspectRatio);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_bbox_aspect_ratio2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, false, db::RegionRatioFilter::AspectRatio);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_area_ratio1 (const db::Region *r, double v, bool inverse)
{
  db::RegionRatioFilter f (v, true, v, true, inverse, db::RegionRatioFilter::AreaRatio);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_area_ratio1 (const db::Region *r, double v)
{
  db::RegionRatioFilter f (v, true, v, true, false, db::RegionRatioFilter::AreaRatio);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_area_ratio2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, inverse, db::RegionRatioFilter::AreaRatio);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_area_ratio2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, false, db::RegionRatioFilter::AreaRatio);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_relative_height1 (const db::Region *r, double v, bool inverse)
{
  db::RegionRatioFilter f (v, true, v, true, inverse, db::RegionRatioFilter::RelativeHeight);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_relative_height1 (const db::Region *r, double v)
{
  db::RegionRatioFilter f (v, true, v, true, false, db::RegionRatioFilter::RelativeHeight);
  return as_2region_vector (r->split_filter (f));
}

static db::Region with_relative_height2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, inverse, db::RegionRatioFilter::RelativeHeight);
  return r->filtered (f);
}

static std::vector<db::Region> split_with_relative_height2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool min_included, bool max_included)
{
  db::RegionRatioFilter f (min.is_nil () ? 0.0 : min.to<double> (), min_included, max.is_nil () ? std::numeric_limits <double>::max () : max.to<double> (), max_included, false, db::RegionRatioFilter::RelativeHeight);
  return as_2region_vector (r->split_filter (f));
}

static db::EdgePairs angle_check1 (const db::Region *r, double angle, bool inverse)
{
  return r->angle_check (angle, angle + 1e-4, inverse);
}

static db::EdgePairs angle_check2 (const db::Region *r, double amin, double amax, bool inverse)
{
  return r->angle_check (amin, amax, inverse);
}

static db::Region in (const db::Region *r, const db::Region &other)
{
  return r->in (other, false);
}

static db::Region not_in (const db::Region *r, const db::Region &other)
{
  return r->in (other, true);
}

static std::vector<db::Region> in_and_out (const db::Region *r, const db::Region &other)
{
  return as_2region_vector (r->in_and_out (other));
}

static db::Region rectangles (const db::Region *r)
{
  db::RectangleFilter f (false, false);
  return r->filtered (f);
}

static db::Region non_rectangles (const db::Region *r)
{
  db::RectangleFilter f (false, true);
  return r->filtered (f);
}

static std::vector<db::Region> split_rectangles (const db::Region *r)
{
  db::RectangleFilter f (false, false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region squares (const db::Region *r)
{
  db::RectangleFilter f (true, false);
  return r->filtered (f);
}

static db::Region non_squares (const db::Region *r)
{
  db::RectangleFilter f (true, true);
  return r->filtered (f);
}

static std::vector<db::Region> split_squares (const db::Region *r)
{
  db::RectangleFilter f (true, false);
  return as_2region_vector (r->split_filter (f));
}

static db::Region rectilinear (const db::Region *r)
{
  db::RectilinearFilter f (false);
  return r->filtered (f);
}

static db::Region non_rectilinear (const db::Region *r)
{
  db::RectilinearFilter f (true);
  return r->filtered (f);
}

static std::vector<db::Region> split_rectilinear (const db::Region *r)
{
  db::RectilinearFilter f (false);
  return as_2region_vector (r->split_filter (f));
}

static void break_polygons (db::Region *r, size_t max_vertex_count, double max_area_ratio)
{
  r->process (db::PolygonBreaker (max_vertex_count, max_area_ratio));
}

static db::Region &merge_ext1 (db::Region *r, int min_wc)
{
  r->merge (false, std::max (0, min_wc - 1));
  return *r;
}

static db::Region &merge_ext2 (db::Region *r, bool min_coherence, int min_wc, bool jpm)
{
  r->merge (min_coherence, std::max (0, min_wc - 1), jpm);
  return *r;
}

static db::Region merged_ext1 (db::Region *r, int min_wc)
{
  return r->merged (false, std::max (0, min_wc - 1));
}

static db::Region merged_ext2 (db::Region *r, bool min_coherence, int min_wc, bool jpm)
{
  return r->merged (min_coherence, std::max (0, min_wc - 1), jpm);
}

static db::EdgePairs width2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->width_check (d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            db::NoOppositeFilter,
                                            db::NoRectFilter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                        );
}

static db::EdgePairs notch2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->notch_check (d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            db::NoOppositeFilter,
                                            db::NoRectFilter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                        );
}

static db::EdgePairs isolated2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->isolated_check (d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                           );
}

static db::EdgePairs space2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->space_check (d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                        );
}

static db::EdgePairs inside2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->inside_check (other, d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                         );
}

static db::EdgePairs overlap2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->overlap_check (other, d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                          );
}

static db::EdgePairs enclosing2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->enclosing_check (other, d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                            );
}

static db::EdgePairs separation2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite, db::RectFilter rect_filter, bool negative, db::PropertyConstraint prop_constraint, db::zero_distance_mode zero_distance_mode)
{
  return r->separation_check (other, d, db::RegionCheckOptions (whole_edges,
                                            metrics,
                                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                            shielded,
                                            opposite,
                                            rect_filter,
                                            negative,
                                            prop_constraint,
                                            zero_distance_mode)
                             );
}

static std::vector<db::Region> andnot (const db::Region *r, const db::Region &other, db::PropertyConstraint prop_constraint)
{
  return as_2region_vector (r->andnot (other, prop_constraint));
}

static std::vector<db::Region> split_inside (const db::Region *r, const db::Region &other)
{
  return as_2region_vector (r->selected_inside_differential (other));
}

static std::vector<db::Region> split_outside (const db::Region *r, const db::Region &other)
{
  return as_2region_vector (r->selected_outside_differential (other));
}

static std::vector<db::Region> split_overlapping (const db::Region *r, const db::Region &other, size_t min_count, size_t max_count)
{
  return as_2region_vector (r->selected_overlapping_differential (other, min_count, max_count));
}

static std::vector<db::Region> split_covering (const db::Region *r, const db::Region &other, size_t min_count, size_t max_count)
{
  return as_2region_vector (r->selected_enclosing_differential (other, min_count, max_count));
}

static std::vector<db::Region> split_interacting_with_region (const db::Region *r, const db::Region &other, size_t min_count, size_t max_count)
{
  return as_2region_vector (r->selected_interacting_differential (other, min_count, max_count));
}

static std::vector<db::Region> split_interacting_with_edges (const db::Region *r, const db::Edges &other, size_t min_count, size_t max_count)
{
  return as_2region_vector (r->selected_interacting_differential (other, min_count, max_count));
}

static std::vector<db::Region> split_interacting_with_texts (const db::Region *r, const db::Texts &other, size_t min_count, size_t max_count)
{
  return as_2region_vector (r->selected_interacting_differential (other, min_count, max_count));
}

template <class Container>
static Container *decompose_convex (const db::Region *r, int mode)
{
  std::unique_ptr<Container> shapes (new Container ());
  db::SimplePolygonContainer sp;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end(); ++p) {
    sp.polygons ().clear ();
    db::decompose_convex (*p, db::PreferredOrientation (mode), sp);
    for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
      shapes->insert (*i);
    }
  }
  return shapes.release ();
}

template <class Container>
static Container *decompose_trapezoids (const db::Region *r, int mode)
{
  std::unique_ptr<Container> shapes (new Container ());
  db::SimplePolygonContainer sp;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end(); ++p) {
    sp.polygons ().clear ();
    db::decompose_trapezoids (*p, db::TrapezoidDecompositionMode (mode), sp);
    for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
      shapes->insert (*i);
    }
  }
  return shapes.release ();
}

static bool is_deep (const db::Region *region)
{
  return dynamic_cast<const db::DeepRegion *> (region->delegate ()) != 0;
}

static size_t id (const db::Region *r)
{
  return tl::id_of (r->delegate ());
}


tl::Variant complex_op (db::Region *region, db::CompoundRegionOperationNode *node, db::PropertyConstraint prop_constraint)
{
  if (node->result_type () == db::CompoundRegionOperationNode::Region) {
    return tl::Variant (region->cop_to_region (*node, prop_constraint));
  } else if (node->result_type () == db::CompoundRegionOperationNode::Edges) {
    return tl::Variant (region->cop_to_edges (*node, prop_constraint));
  } else if (node->result_type () == db::CompoundRegionOperationNode::EdgePairs) {
    return tl::Variant (region->cop_to_edge_pairs (*node, prop_constraint));
  } else {
    return tl::Variant ();
  }
}

static void
fill_region (const db::Region *fr, db::Cell *cell, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point *origin,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  db::fill_region (cell, *fr, fill_cell_index, fc_box, origin ? *origin : db::Point (), origin == 0, remaining_parts, fill_margin, remaining_polygons, glue_box, exclude_area);
}

static void
fill_region_skew (const db::Region *fr, db::Cell *cell, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step, const db::Point *origin,
                  db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  db::fill_region (cell, *fr, fill_cell_index, fc_box, row_step, column_step, origin ? *origin : db::Point (), origin == 0, remaining_parts, fill_margin, remaining_polygons, glue_box, exclude_area);
}

static void
fill_region_multi (const db::Region *fr, db::Cell *cell, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step,
                   const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  db::fill_region_repeat (cell, *fr, fill_cell_index, fc_box, row_step, column_step, fill_margin, remaining_polygons, glue_box, exclude_area);
}

static db::Region
nets (const db::Region *region, db::LayoutToNetlist &l2n, const tl::Variant &net_prop_name, const std::vector<const db::Net *> *net_filter)
{
  return region->nets (l2n, net_prop_name.is_nil () ? db::NPM_NoProperties : db::NPM_NetQualifiedNameOnly, net_prop_name, net_filter);
}

static db::Region
sized_dvm (const db::Region *region, const db::Vector &dv, unsigned int mode)
{
  return region->sized (dv.x (), dv.y (), mode);
}

static db::Region &
size_dvm (db::Region *region, const db::Vector &dv, unsigned int mode)
{
  region->size (dv.x (), dv.y (), mode);
  return *region;
}

static db::Region
sized_inside_ddm (const db::Region *region, const db::Region &inside, db::Coord dx, db::Coord dy, int steps, unsigned int mode)
{
  return region->sized_inside (inside, false, dx, dy, steps, mode);
}

static db::Region
sized_inside_dvm (const db::Region *region, const db::Region &inside, const db::Vector &dv, int steps, unsigned int mode)
{
  return region->sized_inside (inside, false, dv.x (), dv.y (), steps, mode);
}

static db::Region
sized_inside_dm (const db::Region *region, const db::Region &inside, db::Coord d, int steps, unsigned int mode)
{
  return region->sized_inside (inside, false, d, steps, mode);
}

static db::Region &
size_inside_ddm (db::Region *region, const db::Region &inside, db::Coord dx, db::Coord dy, int steps, unsigned int mode)
{
  region->size_inside (inside, false, dx, dy, steps, mode);
  return *region;
}

static db::Region &
size_inside_dvm (db::Region *region, const db::Region &inside, const db::Vector &dv, int steps, unsigned int mode)
{
  region->size_inside (inside, false, dv.x (), dv.y (), steps, mode);
  return *region;
}

static db::Region &
size_inside_dm (db::Region *region, const db::Region &inside, db::Coord d, int steps, unsigned int mode)
{
  region->size_inside (inside, false, d, steps, mode);
  return *region;
}

static db::Region
sized_outside_ddm (const db::Region *region, const db::Region &inside, db::Coord dx, db::Coord dy, int steps, unsigned int mode)
{
  return region->sized_inside (inside, true, dx, dy, steps, mode);
}

static db::Region
sized_outside_dvm (const db::Region *region, const db::Region &inside, const db::Vector &dv, int steps, unsigned int mode)
{
  return region->sized_inside (inside, true, dv.x (), dv.y (), steps, mode);
}

static db::Region
sized_outside_dm (const db::Region *region, const db::Region &inside, db::Coord d, int steps, unsigned int mode)
{
  return region->sized_inside (inside, true, d, steps, mode);
}

static db::Region &
size_outside_ddm (db::Region *region, const db::Region &inside, db::Coord dx, db::Coord dy, int steps, unsigned int mode)
{
  region->size_inside (inside, true, dx, dy, steps, mode);
  return *region;
}

static db::Region &
size_outside_dvm (db::Region *region, const db::Region &inside, const db::Vector &dv, int steps, unsigned int mode)
{
  region->size_inside (inside, true, dv.x (), dv.y (), steps, mode);
  return *region;
}

static db::Region &
size_outside_dm (db::Region *region, const db::Region &inside, db::Coord d, int steps, unsigned int mode)
{
  region->size_inside (inside, true, d, steps, mode);
  return *region;
}

static db::Edges
edges (const db::Region *region, db::PolygonToEdgeProcessor::EdgeMode mode)
{
  if (mode != db::PolygonToEdgeProcessor::All) {
    db::PolygonToEdgeProcessor proc (mode);
    return region->edges (proc);
  } else {
    //  this version is more efficient in the hierarchical case
    return region->edges ();
  }
}

static std::vector<std::vector<double> >
rasterize2 (const db::Region *region, const db::Point &origin, const db::Vector &pixel_distance, const db::Vector &pixel_size, unsigned int nx, unsigned int ny)
{
  db::DAreaMap am (db::DPoint (origin), db::DVector (pixel_distance), db::DVector (pixel_size), nx, ny);

  auto pi = region->begin ();
  pi = pi.confined (db::Box (am.bbox ()), false /*not overlapping*/);

  while (! pi.at_end ()) {
    db::DPolygon dp (*pi);
    db::rasterize (dp, am);
    ++pi;
  }

  std::vector<std::vector<double> > result;
  result.reserve (ny);
  for (unsigned int y = 0; y < ny; ++y) {
    result.push_back (std::vector<double> ());
    std::vector<double> &row = result.back ();
    row.reserve (nx);
    for (unsigned int x = 0; x < nx; ++x) {
      row.push_back (am.get (x, y));
    }
  }

  return result;
}

static std::vector<std::vector<double> >
rasterize1 (const db::Region *region, const db::Point &origin, const db::Vector &pixel_size, unsigned int nx, unsigned int ny)
{
  return rasterize2 (region, origin, pixel_size, pixel_size, nx, ny);
}

static tl::Variant nth (const db::Region *region, size_t n)
{
  const db::Polygon *poly = region->nth (n);
  if (! poly) {
    return tl::Variant ();
  } else {
    return tl::Variant (db::PolygonWithProperties (*poly, region->nth_prop_id (n)));
  }
}

static db::generic_shape_iterator<db::PolygonWithProperties> begin_region (const db::Region *region)
{
  return db::generic_shape_iterator<db::PolygonWithProperties> (db::make_wp_iter (region->delegate ()->begin ()));
}

static db::generic_shape_iterator<db::PolygonWithProperties> begin_region_merged (const db::Region *region)
{
  return db::generic_shape_iterator<db::PolygonWithProperties> (db::make_wp_iter (region->delegate ()->begin_merged ()));
}

static tl::Variant begin_shapes_rec (const db::Region *region)
{
  auto res = region->begin_iter ();
  tl::Variant r = tl::Variant (std::vector<tl::Variant> ());
  r.push (tl::Variant (res.first));
  r.push (tl::Variant (res.second));
  return r;
}

static tl::Variant begin_merged_shapes_rec (const db::Region *region)
{
  auto res = region->begin_merged_iter ();
  tl::Variant r = tl::Variant (std::vector<tl::Variant> ());
  r.push (tl::Variant (res.first));
  r.push (tl::Variant (res.second));
  return r;
}

static db::Point default_origin;

//  provided by gsiDeclDbPolygon.cc:
int td_simple ();
int po_any ();

extern Class<db::ShapeCollection> decl_dbShapeCollection;


Class<db::Region> decl_Region (decl_dbShapeCollection, "db", "Region",
  constructor ("new", &new_v,
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty region.\n"
  ) +
  constructor ("new", &new_a, gsi::arg ("array"),
    "@brief Constructor from a polygon array\n"
    "\n"
    "This constructor creates a region from an array of polygons.\n"
  ) +
  //  This is a dummy constructor that allows creating a Region from an array
  //  of PolygonWithProperties objects too. GSI needs the dummy argument to
  //  differentiate between the cases when an empty array is passed.
  constructor ("new", &new_ap, gsi::arg ("array"), gsi::arg ("dummy", true),
    "@hide"
  ) +
  constructor ("new", &new_b, gsi::arg ("box"),
    "@brief Box constructor\n"
    "\n"
    "This constructor creates a region from a box.\n"
  ) +
  constructor ("new", &new_bp, gsi::arg ("box"),
    "@brief Box constructor\n"
    "\n"
    "This constructor creates a region from a box with properties.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  constructor ("new", &new_p, gsi::arg ("polygon"),
    "@brief Polygon constructor\n"
    "\n"
    "This constructor creates a region from a polygon.\n"
  ) +
  constructor ("new", &new_pp, gsi::arg ("polygon"),
    "@brief Polygon constructor\n"
    "\n"
    "This constructor creates a region from a polygon with properties.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  constructor ("new", &new_ps, gsi::arg ("polygon"),
    "@brief Simple polygon constructor\n"
    "\n"
    "This constructor creates a region from a simple polygon.\n"
  ) +
  constructor ("new", &new_psp, gsi::arg ("polygon"),
    "@brief Simple polygon constructor\n"
    "\n"
    "This constructor creates a region from a simple polygon with properties.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  constructor ("new", &new_path, gsi::arg ("path"),
    "@brief Path constructor\n"
    "\n"
    "This constructor creates a region from a path.\n"
  ) +
  constructor ("new", &new_pathp, gsi::arg ("path"),
    "@brief Path constructor\n"
    "\n"
    "This constructor creates a region from a path with properties.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  constructor ("new", &new_si, gsi::arg ("shape_iterator"),
    "@brief Constructor from a hierarchical shape set\n"
    "\n"
    "This constructor creates a region from the shapes delivered by the given recursive shape iterator.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This method allows feeding the shapes from a hierarchy of cells into the region.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Region::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "\n"
    "This constructor creates a region from the shapes delivered by the given recursive shape iterator.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "On the delivered shapes it applies the given transformation.\n"
    "This method allows feeding the shapes from a hierarchy of cells into the region.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Region::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_sis, gsi::arg ("shapes"),
    "@brief Constructor from a shapes container\n"
    "\n"
    "This constructor creates a region from the shapes container.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This method allows feeding the shapes from a hierarchy of cells into the region.\n"
    "\n"
    "This constructor has been introduced in version 0.25 and extended in version 0.29."
  ) +
  constructor ("new", &new_sis2, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Constructor from a shapes container with a transformation\n"
    "\n"
    "This constructor creates a region from the shapes container after applying the transformation.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This method allows feeding the shapes from a hierarchy of cells into the region.\n"
    "\n"
    "This constructor variant has been introduced in version 0.29."
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("deep_shape_store"), gsi::arg ("area_ratio", 0.0), gsi::arg ("max_vertex_count", size_t (0)),
    "@brief Constructor for a deep region from a hierarchical shape set\n"
    "\n"
    "This constructor creates a hierarchical region. Use a \\DeepShapeStore object to "
    "supply the hierarchical heap. See \\DeepShapeStore for more details.\n"
    "\n"
    "'area_ratio' and 'max_vertex' supply two optimization parameters which control how "
    "big polygons are split to reduce the region's polygon complexity.\n"
    "\n"
    "@param shape_iterator The recursive shape iterator which delivers the hierarchy to take\n"
    "@param deep_shape_store The hierarchical heap (see there)\n"
    "@param area_ratio The maximum ratio of bounding box to polygon area before polygons are split\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  constructor ("new", &new_sid2, gsi::arg ("shape_iterator"), gsi::arg ("deep_shape_store"), gsi::arg ("trans"), gsi::arg ("area_ratio", 0.0), gsi::arg ("max_vertex_count", size_t (0)),
    "@brief Constructor for a deep region from a hierarchical shape set\n"
    "\n"
    "This constructor creates a hierarchical region. Use a \\DeepShapeStore object to "
    "supply the hierarchical heap. See \\DeepShapeStore for more details.\n"
    "\n"
    "'area_ratio' and 'max_vertex' supply two optimization parameters which control how "
    "big polygons are split to reduce the region's polygon complexity.\n"
    "\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@param shape_iterator The recursive shape iterator which delivers the hierarchy to take\n"
    "@param deep_shape_store The hierarchical heap (see there)\n"
    "@param area_ratio The maximum ratio of bounding box to polygon area before polygons are split\n"
    "@param trans The transformation to apply when storing the layout data\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  constructor ("new", &new_texts_as_boxes1, gsi::arg("shape_iterator"), gsi::arg ("expr"), gsi::arg ("as_pattern", true), gsi::arg ("enl", 1),
    "@brief Constructor from a text set\n"
    "\n"
    "@param shape_iterator The iterator from which to derive the texts\n"
    "@param expr The selection string\n"
    "@param as_pattern If true, the selection string is treated as a glob pattern. Otherwise the match is exact.\n"
    "@param enl The per-side enlargement of the box to mark the text (1 gives a 2x2 DBU box)"
    "\n"
    "This special constructor will create a region from the text objects delivered by the shape iterator. "
    "Each text object will give a small (non-empty) box that represents the text origin.\n"
    "Texts can be selected by their strings - either through a glob pattern or by exact comparison with "
    "the given string. The following options are available:\n"
    "\n"
    "@code\n"
    "region = RBA::Region::new(iter, \"*\")           # all texts\n"
    "region = RBA::Region::new(iter, \"A*\")          # all texts starting with an 'A'\n"
    "region = RBA::Region::new(iter, \"A*\", false)   # all texts exactly matching 'A*'\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.25. The enlargement parameter has been added in version 0.26.\n"
  ) +
  constructor ("new", &new_texts_as_boxes2, gsi::arg("shape_iterator"), gsi::arg ("dss"), gsi::arg ("expr"), gsi::arg ("as_pattern", true), gsi::arg ("enl", 1),
    "@brief Constructor from a text set\n"
    "\n"
    "@param shape_iterator The iterator from which to derive the texts\n"
    "@param dss The \\DeepShapeStore object that acts as a heap for hierarchical operations.\n"
    "@param expr The selection string\n"
    "@param as_pattern If true, the selection string is treated as a glob pattern. Otherwise the match is exact.\n"
    "@param enl The per-side enlargement of the box to mark the text (1 gives a 2x2 DBU box)"
    "\n"
    "This special constructor will create a deep region from the text objects delivered by the shape iterator. "
    "Each text object will give a small (non-empty) box that represents the text origin.\n"
    "Texts can be selected by their strings - either through a glob pattern or by exact comparison with "
    "the given string. The following options are available:\n"
    "\n"
    "@code\n"
    "region = RBA::Region::new(iter, dss, \"*\")           # all texts\n"
    "region = RBA::Region::new(iter, dss, \"A*\")          # all texts starting with an 'A'\n"
    "region = RBA::Region::new(iter, dss, \"A*\", false)   # all texts exactly matching 'A*'\n"
    "@/code\n"
    "\n"
    "This variant has been introduced in version 0.26.\n"
  ) +
  method ("insert_into", &db::Region::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this region into the given layout, below the given cell and into the given layer.\n"
    "If the region is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("write", &db::Region::write, gsi::arg ("filename"),
    "@brief Writes the region to a file\n"
    "This method is provided for debugging purposes. It writes the object to a flat layer 0/0 in a single top cell.\n"
    "\n"
    "This method has been introduced in version 0.29."
  ) +
  factory_ext ("texts", &texts_as_boxes1, gsi::arg ("expr", std::string ("*")), gsi::arg ("as_pattern", true), gsi::arg ("enl", 1),
    "@hide\n"
    "This method is provided for DRC implementation only."
  ) +
  factory_ext ("texts", &texts_as_boxes2, gsi::arg ("dss"), gsi::arg ("expr", std::string ("*")), gsi::arg ("as_pattern", true), gsi::arg ("enl", 1),
    "@hide\n"
    "This method is provided for DRC implementation only."
  ) +
  factory_ext ("texts_dots", &texts_as_dots1, gsi::arg ("expr", std::string ("*")), gsi::arg ("as_pattern", true),
    "@hide\n"
    "This method is provided for DRC implementation only."
  ) +
  factory_ext ("texts_dots", &texts_as_dots2, gsi::arg ("dss"), gsi::arg ("expr", std::string ("*")), gsi::arg ("as_pattern", true),
    "@hide\n"
    "This method is provided for DRC implementation only."
  ) +
  method ("merged_semantics=", &db::Region::set_merged_semantics, gsi::arg ("f"),
    "@brief Enables or disables merged semantics\n"
    "If merged semantics is enabled (the default), coherent polygons will be considered\n"
    "as single regions and artificial edges such as cut-lines will not be considered.\n"
    "Merged semantics thus is equivalent to considering coherent areas rather than\n"
    "single polygons\n"
  ) +
  method ("merged_semantics?", &db::Region::merged_semantics,
    "@brief Gets a flag indicating whether merged semantics is enabled\n"
    "See \\merged_semantics= for a description of this attribute.\n"
  ) +
  method ("join_properties_on_merge=", &db::Region::set_join_properties_on_merge, gsi::arg ("f"),
    "@brief Sets a flag indicating whether to join properties on merge\n"
    "\n"
    "When this flag is set to true, properties are joined on 'merge'.\n"
    "That is: shapes merging into bigger shapes will have their properties joined.\n"
    "With the flag set to false (the default), 'merge' will not join properties and return merged\n"
    "shapes only if the sub-shapes have the same properties - i.e. properties form\n"
    "separate shape classes on merge.\n"
    "\n"
    "This attribute has been introduced in version 0.30.3."
  ) +
  method ("join_properties_on_merge?", &db::Region::join_properties_on_merge,
    "@brief Gets a flag indicating whether to join properties on merge\n"
    "See \\join_properties_on_merge= for a description of this attribute.\n"
    "\n"
    "This attribute has been introduced in version 0.30.3."
  ) +
  method ("strict_handling=", &db::Region::set_strict_handling, gsi::arg ("f"),
    "@brief Enables or disables strict handling\n"
    "\n"
    "Strict handling means to leave away some optimizations. Specifically the \n"
    "output of boolean operations will be merged even if one input is empty.\n"
    "Without strict handling, the operation will be optimized and output \n"
    "won't be merged.\n"
    "\n"
    "Strict handling is disabled by default and optimization is in place.\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) +
  method ("strict_handling?", &db::Region::strict_handling,
    "@brief Gets a flag indicating whether merged semantics is enabled\n"
    "See \\strict_handling= for a description of this attribute.\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) +
  method ("min_coherence=", &db::Region::set_min_coherence, gsi::arg ("f"),
    "@brief Enable or disable minimum coherence\n"
    "If minimum coherence is set, the merge operations (explicit merge with \\merge or\n"
    "implicit merge through merged_semantics) are performed using minimum coherence mode.\n"
    "The coherence mode determines how kissing-corner situations are resolved. If\n"
    "minimum coherence is selected, they are resolved such that multiple polygons are \n"
    "created which touch at a corner).\n"
    "\n"
    "The default setting is maximum coherence (min_coherence = false).\n"
  ) +
  method ("min_coherence?", &db::Region::min_coherence,
    "@brief Gets a flag indicating whether minimum coherence is selected\n"
    "See \\min_coherence= for a description of this attribute.\n"
  ) +
  method_ext ("complex_op", &complex_op, gsi::arg ("node"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Executes a complex operation (see \\CompoundRegionOperationNode for details)\n"
    "\n"
    "This method has been introduced in version 0.27."
    "\n"
    "The 'property_constraint' parameter controls whether properties are considered: with 'SamePropertiesConstraint' "
    "the operation is only applied between shapes with identical properties. With 'DifferentPropertiesConstraint' only "
    "between shapes with different properties. This option has been introduced in version 0.28.4."
  ) +
  method_ext ("with_perimeter", with_perimeter1, gsi::arg ("perimeter"), gsi::arg ("inverse"),
    "@brief Filter the polygons by perimeter\n"
    "Filters the polygons of the region by perimeter. If \"inverse\" is false, only "
    "polygons which have the given perimeter are returned. If \"inverse\" is true, "
    "polygons not having the given perimeter are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_perimeter", with_perimeter2, gsi::arg ("min_perimeter"), gsi::arg ("max_perimeter"), gsi::arg ("inverse"),
    "@brief Filter the polygons by perimeter\n"
    "Filters the polygons of the region by perimeter. If \"inverse\" is false, only "
    "polygons which have a perimeter larger or equal to \"min_perimeter\" and less than \"max_perimeter\" are "
    "returned. If \"inverse\" is true, "
    "polygons having a perimeter less than \"min_perimeter\" or larger or equal than \"max_perimeter\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_perimeter", split_with_perimeter1, gsi::arg ("perimeter"),
    "@brief Like \\with_perimeter, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_perimeter", split_with_perimeter2, gsi::arg ("min_perimeter"), gsi::arg ("max_perimeter"),
    "@brief Like \\with_perimeter, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_area", with_area1, gsi::arg ("area"), gsi::arg ("inverse"),
    "@brief Filter the polygons by area\n"
    "Filters the polygons of the region by area. If \"inverse\" is false, only "
    "polygons which have the given area are returned. If \"inverse\" is true, "
    "polygons not having the given area are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_area", with_area2, gsi::arg ("min_area"), gsi::arg ("max_area"), gsi::arg ("inverse"),
    "@brief Filter the polygons by area\n"
    "Filters the polygons of the region by area. If \"inverse\" is false, only "
    "polygons which have an area larger or equal to \"min_area\" and less than \"max_area\" are "
    "returned. If \"inverse\" is true, "
    "polygons having an area less than \"min_area\" or larger or equal than \"max_area\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_area", split_with_area1, gsi::arg ("area"),
    "@brief Like \\with_area, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_area", split_with_area2, gsi::arg ("min_area"), gsi::arg ("max_area"),
    "@brief Like \\with_area, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_holes", with_holes1, gsi::arg ("nholes"), gsi::arg ("inverse"),
    "@brief Filters the polygons by their number of holes\n"
    "Filters the polygons of the region by number of holes. If \"inverse\" is false, only "
    "polygons which have the given number of holes are returned. If \"inverse\" is true, "
    "polygons not having the given of holes are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("with_holes", with_holes2, gsi::arg ("min_nholes"), gsi::arg ("max_nholes"), gsi::arg ("inverse"),
    "@brief Filter the polygons by their number of holes\n"
    "Filters the polygons of the region by number of holes. If \"inverse\" is false, only "
    "polygons which have a hole count larger or equal to \"min_nholes\" and less than \"max_nholes\" are "
    "returned. If \"inverse\" is true, "
    "polygons having a hole count less than \"min_nholes\" or larger or equal than \"max_nholes\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("split_with_holes", split_with_holes1, gsi::arg ("nholes"),
    "@brief Like \\with_holes, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_holes", split_with_holes2, gsi::arg ("min_nholes"), gsi::arg ("max_nholes"),
    "@brief Like \\with_holes, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_bbox_width", with_bbox_width1, gsi::arg ("width"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width\n"
    "Filters the polygons of the region by the width of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has the given width are returned. If \"inverse\" is true, "
    "polygons whose bounding box does not have the given width are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_bbox_width", with_bbox_width2, gsi::arg ("min_width"), gsi::arg ("max_width"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width\n"
    "Filters the polygons of the region by the width of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has a width larger or equal to \"min_width\" and less than \"max_width\" are "
    "returned. If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_bbox_width", split_with_bbox_width1, gsi::arg ("width"),
    "@brief Like \\with_bbox_width, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_bbox_width", split_with_bbox_width2, gsi::arg ("min_width"), gsi::arg ("max_width"),
    "@brief Like \\with_bbox_width, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_bbox_height", with_bbox_height1, gsi::arg ("height"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box height\n"
    "Filters the polygons of the region by the height of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has the given height are returned. If \"inverse\" is true, "
    "polygons whose bounding box does not have the given height are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_bbox_height", with_bbox_height2, gsi::arg ("min_height"), gsi::arg ("max_height"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box height\n"
    "Filters the polygons of the region by the height of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has a height larger or equal to \"min_height\" and less than \"max_height\" are "
    "returned. If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_bbox_height", split_with_bbox_height1, gsi::arg ("height"),
    "@brief Like \\with_bbox_height, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_bbox_height", split_with_bbox_height2, gsi::arg ("min_height"), gsi::arg ("max_height"),
    "@brief Like \\with_bbox_height, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_bbox_min", with_bbox_min1, gsi::arg ("dim"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width or height, whichever is smaller\n"
    "Filters the polygons inside the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's smaller dimension is equal to the given value "
    "are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_bbox_min", with_bbox_min2, gsi::arg ("min_dim"), gsi::arg ("max_dim"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width or height, whichever is smaller\n"
    "Filters the polygons of the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's smaller dimension is larger or equal to \"min_dim\" "
    "and less than \"max_dim\" are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_bbox_min", split_with_bbox_min1, gsi::arg ("dim"),
    "@brief Like \\with_bbox_min, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_bbox_min", split_with_bbox_min2, gsi::arg ("min_dim"), gsi::arg ("max_dim"),
    "@brief Like \\with_bbox_min, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_bbox_max", with_bbox_max1, gsi::arg ("dim"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width or height, whichever is larger\n"
    "Filters the polygons of the region by the maximum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's larger dimension is equal to the given value "
    "are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_bbox_max", with_bbox_max2, gsi::arg ("min_dim"), gsi::arg ("max_dim"), gsi::arg ("inverse"),
    "@brief Filter the polygons by bounding box width or height, whichever is larger\n"
    "Filters the polygons of the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's larger dimension is larger or equal to \"min_dim\" "
    "and less than \"max_dim\" are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_with_bbox_max", split_with_bbox_max1, gsi::arg ("dim"),
    "@brief Like \\with_bbox_max, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_bbox_max", split_with_bbox_max2, gsi::arg ("min_dim"), gsi::arg ("max_dim"),
    "@brief Like \\with_bbox_max, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_bbox_aspect_ratio", with_bbox_aspect_ratio1, gsi::arg ("ratio"), gsi::arg ("inverse"),
    "@brief Filters the polygons by the aspect ratio of their bounding boxes\n"
    "Filters the polygons of the region by the aspect ratio of their bounding boxes. "
    "The aspect ratio is the ratio of larger to smaller dimension of the bounding box. "
    "A square has an aspect ratio of 1.\n"
    "\n"
    "With 'inverse' set to false, this version filters polygons which have a bounding box aspect ratio equal to the given value. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("with_bbox_aspect_ratio", with_bbox_aspect_ratio2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("inverse"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Filters the polygons by the aspect ratio of their bounding boxes\n"
    "Filters the polygons of the region by the aspect ratio of their bounding boxes. "
    "The aspect ratio is the ratio of larger to smaller dimension of the bounding box. "
    "A square has an aspect ratio of 1.\n"
    "\n"
    "With 'inverse' set to false, this version filters polygons which have a bounding box aspect ratio between 'min_ratio' and 'max_ratio'. "
    "With 'min_included' set to true, the 'min_ratio' value is included in the range, otherwise it's excluded. Same for 'max_included' and 'max_ratio'. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("split_with_bbox_aspect_ratio", split_with_bbox_aspect_ratio1, gsi::arg ("ratio"),
    "@brief Like \\with_bbox_aspect_ratio, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_bbox_aspect_ratio", split_with_bbox_aspect_ratio2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Like \\with_bbox_aspect_ratio, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_area_ratio", with_area_ratio1, gsi::arg ("ratio"), gsi::arg ("inverse"),
    "@brief Filters the polygons by the bounding box area to polygon area ratio\n"
    "The area ratio is defined by the ratio of bounding box area to polygon area. It's a measure "
    "how much the bounding box is approximating the polygon. 'Thin polygons' have a large area ratio, boxes has an area ratio of 1.\n"
    "The area ratio is always larger or equal to 1. "
    "\n"
    "With 'inverse' set to false, this version filters polygons which have an area ratio equal to the given value. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("with_area_ratio", with_area_ratio2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("inverse"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Filters the polygons by the aspect ratio of their bounding boxes\n"
    "The area ratio is defined by the ratio of bounding box area to polygon area. It's a measure "
    "how much the bounding box is approximating the polygon. 'Thin polygons' have a large area ratio, boxes has an area ratio of 1.\n"
    "The area ratio is always larger or equal to 1. "
    "\n"
    "With 'inverse' set to false, this version filters polygons which have an area ratio between 'min_ratio' and 'max_ratio'. "
    "With 'min_included' set to true, the 'min_ratio' value is included in the range, otherwise it's excluded. Same for 'max_included' and 'max_ratio'. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("split_with_area_ratio", split_with_area_ratio1, gsi::arg ("ratio"),
    "@brief Like \\with_area_ratio, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_area_ratio", split_with_area_ratio2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Like \\with_area_ratio, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_relative_height", with_relative_height1, gsi::arg ("ratio"), gsi::arg ("inverse"),
    "@brief Filters the polygons by the ratio of height to width\n"
    "This method filters the polygons of the region by the ratio of height vs. width of their bounding boxes. "
    "'Tall' polygons have a large value while 'flat' polygons have a small value. A square has a relative height of 1.\n"
    "\n"
    "An alternative method is 'with_area_ratio' which can be more efficient because it's isotropic.\n"
    "\n"
    "With 'inverse' set to false, this version filters polygons which have a relative height equal to the given value. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("with_relative_height", with_relative_height2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("inverse"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Filters the polygons by the bounding box height to width ratio\n"
    "This method filters the polygons of the region by the ratio of height vs. width of their bounding boxes. "
    "'Tall' polygons have a large value while 'flat' polygons have a small value. A square has a relative height of 1.\n"
    "\n"
    "An alternative method is 'with_area_ratio' which can be more efficient because it's isotropic.\n"
    "\n"
    "With 'inverse' set to false, this version filters polygons which have a relative height between 'min_ratio' and 'max_ratio'. "
    "With 'min_included' set to true, the 'min_ratio' value is included in the range, otherwise it's excluded. Same for 'max_included' and 'max_ratio'. "
    "With 'inverse' set to true, all other polygons will be returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("split_with_relative_height", split_with_relative_height1, gsi::arg ("ratio"),
    "@brief Like \\with_relative_height, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_relative_height", split_with_relative_height2, gsi::arg ("min_ratio"), gsi::arg ("max_ratio"), gsi::arg ("min_included", true), gsi::arg ("max_included", true),
    "@brief Like \\with_relative_height, but returning two regions\n"
    "The first region will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method ("strange_polygon_check", &db::Region::strange_polygon_check,
    "@brief Returns a region containing those parts of polygons which are \"strange\"\n"
    "Strange parts of polygons are self-overlapping parts or non-orientable parts (i.e. in the \"8\" configuration).\n"
    "\n"
    "Merged semantics does not apply for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("snapped", &db::Region::snapped, gsi::arg ("gx"), gsi::arg ("gy"),
    "@brief Returns the snapped region\n"
    "This method will snap the region to the given grid and return the snapped region (see \\snap). The original region is not modified.\n"
  ) +
  method ("snap", &db::Region::snap, gsi::arg ("gx"), gsi::arg ("gy"),
    "@brief Snaps the region to the given grid\n"
    "This method will snap the region to the given grid - each x or y coordinate is brought on the gx or gy grid by rounding "
    "to the nearest value which is a multiple of gx or gy.\n"
    "\n"
    "If gx or gy is 0, no snapping happens in that direction.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("scaled_and_snapped", &db::Region::scaled_and_snapped, gsi::arg ("gx"), gsi::arg ("mx"), gsi::arg ("dx"), gsi::arg ("gy"),gsi::arg ("my"), gsi::arg ("dy"),
    "@brief Returns the scaled and snapped region\n"
    "This method will scale and snap the region to the given grid and return the scaled and snapped region (see \\scale_and_snap). The original region is not modified.\n"
    "\n"
    "This method has been introduced in version 0.26.1."
  ) +
  method ("scale_and_snap", &db::Region::scale_and_snap, gsi::arg ("gx"), gsi::arg ("mx"), gsi::arg ("dx"), gsi::arg ("gy"),gsi::arg ("my"), gsi::arg ("dy"),
    "@brief Scales and snaps the region to the given grid\n"
    "This method will first scale the region by a rational factor of mx/dx horizontally and my/dy vertically and then "
    "snap the region to the given grid - each x or y coordinate is brought on the gx or gy grid by rounding "
    "to the nearest value which is a multiple of gx or gy.\n"
    "\n"
    "If gx or gy is 0, the result is brought on a grid of 1.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.26.1."
  ) +
  method ("grid_check", &db::Region::grid_check, gsi::arg ("gx"), gsi::arg ("gy"),
    "@brief Returns a marker for all vertices not being on the given grid\n"
    "This method will return an edge pair object for every vertex whose x coordinate is not a multiple of gx or whose "
    "y coordinate is not a multiple of gy. The edge pair objects contain two edges consisting of the same single point - the "
    "original vertex.\n"
    "\n"
    "If gx or gy is 0 or less, the grid is not checked in that direction.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_angle", angle_check1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Returns markers on every corner with the given angle (or not with the given angle)\n"
    "If the inverse flag is false, this method returns an error marker (an \\EdgePair object) for every corner whose connected edges "
    "form an angle with the given value (in degree). If the inverse flag is true, the method returns markers for every corner whose "
    "angle is not the given value.\n"
    "\n"
    "The edge pair objects returned will contain both edges forming the angle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("with_angle", angle_check2, gsi::arg ("amin"), gsi::arg ("amax"), gsi::arg ("inverse"),
    "@brief Returns markers on every corner with an angle of more than amin and less than amax (or the opposite)\n"
    "If the inverse flag is false, this method returns an error marker (an \\EdgePair object) for every corner whose connected edges "
    "form an angle whose value is more or equal to amin (in degree) or less (but not equal to) amax. If the inverse flag is true, the method returns markers for every corner whose "
    "angle is not matching that criterion.\n"
    "\n"
    "The edge pair objects returned will contain both edges forming the angle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::Box &)) &db::Region::insert, gsi::arg ("box"),
    "@brief Inserts a box\n"
    "\n"
    "Inserts a box into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::BoxWithProperties &)) &db::Region::insert, gsi::arg ("box"),
    "@brief Inserts a box\n"
    "\n"
    "Inserts a box with properties into the region.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method ("insert", (void (db::Region::*)(const db::Polygon &)) &db::Region::insert, gsi::arg ("polygon"),
    "@brief Inserts a polygon\n"
    "\n"
    "Inserts a polygon into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::PolygonWithProperties &)) &db::Region::insert, gsi::arg ("polygon"),
    "@brief Inserts a polygon\n"
    "\n"
    "Inserts a polygon with properties into the region.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method ("insert", (void (db::Region::*)(const db::SimplePolygon &)) &db::Region::insert, gsi::arg ("polygon"),
    "@brief Inserts a simple polygon\n"
    "\n"
    "Inserts a simple polygon into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::SimplePolygonWithProperties &)) &db::Region::insert, gsi::arg ("polygon"),
    "@brief Inserts a simple polygon\n"
    "\n"
    "Inserts a simple polygon with properties into the region.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method ("insert", (void (db::Region::*)(const db::Path &)) &db::Region::insert, gsi::arg ("path"),
    "@brief Inserts a path\n"
    "\n"
    "Inserts a path into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::PathWithProperties &)) &db::Region::insert, gsi::arg ("path"),
    "@brief Inserts a path\n"
    "\n"
    "Inserts a path with properties into the region.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method_ext ("insert", &insert_si, gsi::arg ("shape_iterator"),
    "@brief Inserts all shapes delivered by the recursive shape iterator into this region\n"
    "\n"
    "This method will insert all shapes delivered by the shape iterator and insert them into the region.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
  ) +
  method_ext ("insert", &insert_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Inserts all shapes delivered by the recursive shape iterator into this region with a transformation\n"
    "\n"
    "This method will insert all shapes delivered by the shape iterator and insert them into the region.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This variant will apply the given transformation to the shapes. This is useful to scale the "
    "shapes to a specific database unit for example.\n"
  ) +
  method_ext ("insert", &insert_a, gsi::arg ("array"),
    "@brief Inserts all polygons from the array into this region\n"
  ) +
  method_ext ("insert", &insert_ap, gsi::arg ("array"),
    "@brief Inserts all polygons with properties from the array into this region\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method_ext ("insert", &insert_r, gsi::arg ("region"),
    "@brief Inserts all polygons from the other region into this region\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_s, gsi::arg ("shapes"),
    "@brief Inserts all polygons from the shape collection into this region\n"
    "This method takes each \"polygon-like\" shape from the shape collection and "
    "inserts this shape into the region. Paths and boxes are converted to polygons during this process. "
    "Edges and text objects are ignored.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_st<db::Trans>, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Inserts all polygons from the shape collection into this region with transformation\n"
    "This method takes each \"polygon-like\" shape from the shape collection and "
    "inserts this shape into the region after applying the given transformation. "
    "Paths and boxes are converted to polygons during this process. "
    "Edges and text objects are ignored.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_st<db::ICplxTrans>, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Inserts all polygons from the shape collection into this region with complex transformation\n"
    "This method takes each \"polygon-like\" shape from the shape collection and "
    "inserts this shape into the region after applying the given complex transformation. "
    "Paths and boxes are converted to polygons during this process. "
    "Edges and text objects are ignored.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("extents", &extents0,
    "@brief Returns a region with the bounding boxes of the polygons\n"
    "This method will return a region consisting of the bounding boxes of the polygons.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("extents", &extents1, gsi::arg ("d"),
    "@brief Returns a region with the enlarged bounding boxes of the polygons\n"
    "This method will return a region consisting of the bounding boxes of the polygons enlarged by the given distance d.\n"
    "The enlargement is specified per edge, i.e the width and height will be increased by 2*d.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("extents", &extents2, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Returns a region with the enlarged bounding boxes of the polygons\n"
    "This method will return a region consisting of the bounding boxes of the polygons enlarged by the given distance dx in x direction and dy in y direction.\n"
    "The enlargement is specified per edge, i.e the width will be increased by 2*dx.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("extent_refs", &extent_refs,
    "@hide\n"
    "This method is provided for DRC implementation.\n"
  ) +
  method_ext ("extent_refs_edges", &extent_refs_edges,
    "@hide\n"
    "This method is provided for DRC implementation.\n"
  ) +
  method_ext ("corners", &corners_to_boxes, gsi::arg ("angle_min", -180.0), gsi::arg ("angle_max", 180.0), gsi::arg ("dim", 1), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", true), gsi::arg ("inverse", false), gsi::arg ("absolute", false),
    "@brief This method will select all corners whose attached edges satisfy the angle condition.\n"
    "\n"
    "The angle values specify a range of angles: all corners whose attached edges form an angle "
    "between angle_min and angle_max will be reported boxes with 2*dim x 2*dim dimension. The default dimension is 2x2 DBU.\n"
    "\n"
    "If 'include_angle_min' is true, the angle condition is >= min. angle, otherwise it is > min. angle. "
    "Same for 'include_angle_,ax' and the max. angle.\n"
    "\n"
    "With 'absolute' set to false (the default), the angle is measured "
    "between the incoming and the outcoming edge in mathematical sense: a positive value is a turn left "
    "while a negative value is a turn right. Since polygon contours are oriented clockwise, positive "
    "angles will report concave corners while negative ones report convex ones.\n"
    "With the 'absolute' option set to true, there is no such distinction and angle values are always positive.\n"
    "\n"
    "With 'inverse' set to true, the method will select corners not meeting the angle criterion.\n"
    "\n"
    "A similar function that reports corners as point-like edges is \\corners_dots.\n"
    "\n"
    "This method has been introduced in version 0.25. 'include_min_angle' and 'include_max_angle' have been added in version 0.27. "
    "'inverse' and 'absolute' have been added in version 0.29.1.\n"
  ) +
  method_ext ("corners_dots", &corners_to_dots, gsi::arg ("angle_start", -180.0), gsi::arg ("angle_end", 180.0), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", true), gsi::arg ("inverse", false), gsi::arg ("absolute", false),
    "@brief This method will select all corners whose attached edges satisfy the angle condition.\n"
    "\n"
    "This method is similar to \\corners, but delivers an \\Edges collection with dot-like edges for each corner.\n"
    "\n"
    "This method has been introduced in version 0.25. 'include_min_angle' and 'include_max_angle' have been added in version 0.27. "
    "'inverse' and 'absolute' have been added in version 0.29.1.\n"
  ) +
  method_ext ("corners_edge_pairs", &corners_to_edge_pairs, gsi::arg ("angle_start", -180.0), gsi::arg ("angle_end", 180.0), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", true), gsi::arg ("inverse", false), gsi::arg ("absolute", false),
    "@brief This method will select all corners whose attached edges satisfy the angle condition.\n"
    "\n"
    "This method is similar to \\corners, but delivers an \\EdgePairs collection with an edge pairs for each corner.\n"
    "The first edge is the incoming edge of the corner, the second one the outgoing edge.\n"
    "\n"
    "This method has been introduced in version 0.27.1. "
    "'inverse' and 'absolute' have been added in version 0.29.1.\n"
  ) +
  method ("merge", (db::Region &(db::Region::*) ()) &db::Region::merge,
    "@brief Merge the region\n"
    "\n"
    "@return The region after it has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "If the region is already merged, this method does nothing.\n"
    "This method will behave according to the settings of the \\min_coherence and \\join_properties_on_merge attributes."
  ) +
  method_ext ("merge", &merge_ext1, gsi::arg ("min_wc"),
    "@brief Merge the region with options\n"
    "\n"
    "@param min_wc Overlap selection\n"
    "@return The region after it has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides one additional option: \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "This method is equivalent to \"merge(false, min_wc, false).\n"
  ) +
  method_ext ("merge", &merge_ext2, gsi::arg ("min_coherence"), gsi::arg ("min_wc"), gsi::arg ("join_properties_on_merge", false),
    "@brief Merge the region with options\n"
    "\n"
    "@param min_coherence A flag indicating whether the resulting polygons shall have minimum coherence\n"
    "@param min_wc Overlap selection\n"
    "@param join_properties_on_merge See below\n"
    "@return The region after it has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides two additional options: if \"min_coherence\" is set to true, \"kissing corners\" are "
    "resolved by producing separate polygons. \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "The 'join_properties_on_merge' argument indicates how properties should be handled: if true, "
    "the properties of the parts are joined and attached to the merged shape. If false, "
    "only shapes with the same properties are merged - i.e. different properties form shape classes "
    "that are merged individually.\n"
    "\n"
    "'join_properties_on_merge' has been added in version 0.30.3."
  ) +
  method ("merged", (db::Region (db::Region::*) () const) &db::Region::merged,
    "@brief Returns the merged region\n"
    "\n"
    "@return The region after it has been merged.\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "If the region is already merged, this method does nothing.\n"
    "This method will behave according to the settings of the \\min_coherence and \\join_properties_on_merge attributes.\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
  ) +
  method_ext ("merged", &merged_ext1, gsi::arg ("min_wc"),
    "@brief Returns the merged region (with options)\n"
    "\n"
    "@param min_wc Overlap selection\n"
    "@return The region after it has been merged.\n"
    "\n"
    "This version provides one additional options: \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "This method is equivalent to \"merged(false, min_wc, false)\".\n"
    "\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
  ) +
  method_ext ("merged", &merged_ext2, gsi::arg ("min_coherence"), gsi::arg ("min_wc"), gsi::arg ("join_properties_on_merge", false),
    "@brief Returns the merged region (with options)\n"
    "\n"
    "@param min_coherence A flag indicating whether the resulting polygons shall have minimum coherence\n"
    "@param min_wc Overlap selection\n"
    "@param join_properties_on_merge See below\n"
    "@return The region after it has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides two additional options: if \"min_coherence\" is set to true, \"kissing corners\" are "
    "resolved by producing separate polygons. \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "\n"
    "The 'join_properties_on_merge' argument indicates how properties should be handled: if true, "
    "the properties of the parts are joined and attached to the merged shape. If false, "
    "only shapes with the same properties are merged - i.e. different properties form shape classes "
    "that are merged individually.\n"
    "\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
    "\n"
    "'join_properties_on_merge' has been added in version 0.30.3."
  ) +
  method ("round_corners", &db::Region::round_corners, gsi::arg ("r_inner"), gsi::arg ("r_outer"), gsi::arg ("n"),
    "@brief Corner rounding\n"
    "@param r_inner Inner corner radius (in database units)\n"
    "@param r_outer Outer corner radius (in database units)\n"
    "@param n The number of points per circle\n"
    "\n"
    "This method rounds the corners of the polygons in the region. Inner corners will be rounded with "
    "a radius of r_inner and outer corners with a radius of r_outer. The circles will be approximated "
    "by segments using n segments per full circle.\n"
    "\n"
    "This method modifies the region. \\rounded_corners is a method that does the same but returns a new "
    "region without modifying self. Merged semantics applies for this method.\n"
  ) +
  method ("rounded_corners", &db::Region::rounded_corners, gsi::arg ("r_inner"), gsi::arg ("r_outer"), gsi::arg ("n"),
    "@brief Corner rounding\n"
    "@param r_inner Inner corner radius (in database units)\n"
    "@param r_outer Outer corner radius (in database units)\n"
    "@param n The number of points per circle\n"
    "\n"
    "See \\round_corners for a description of this method. This version returns a new region instead of "
    "modifying self (out-of-place)."
  ) +
  method ("smooth", &db::Region::smooth, gsi::arg ("d"), gsi::arg ("keep_hv", false),
    "@brief Smoothing\n"
    "@param d The smoothing tolerance (in database units)\n"
    "@param keep_hv If true, horizontal and vertical edges are maintained\n"
    "\n"
    "This method will simplify the merged polygons of the region by removing vertexes if the "
    "resulting polygon stays equivalent with the original polygon. Equivalence is measured "
    "in terms of a deviation which is guaranteed to not become larger than \\d."
    "\n"
    "This method modifies the region. \\smoothed is a method that does the same but returns a new "
    "region without modifying self. Merged semantics applies for this method.\n"
  ) +
  method ("smoothed", &db::Region::smoothed, gsi::arg ("d"), gsi::arg ("keep_hv", false),
    "@brief Smoothing\n"
    "@param d The smoothing tolerance (in database units)\n"
    "@param keep_hv If true, horizontal and vertical edges are maintained\n"
    "\n"
    "See \\smooth for a description of this method. This version returns a new region instead of "
    "modifying self (out-of-place). It has been introduced in version 0.25."
  ) +
  method ("size", (db::Region & (db::Region::*) (db::Coord, db::Coord, unsigned int)) &db::Region::size, gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Anisotropic sizing (biasing)\n"
    "\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "Shifts the contour outwards (dx,dy>0) or inwards (dx,dy<0).\n"
    "dx is the sizing in x-direction and dy is the sizing in y-direction. The sign of dx and dy should be identical.\n"
    "\n"
    "This method applies a sizing to the region. Before the sizing is done, the\n"
    "region is merged if this is not the case already.\n"
    "\n"
    "The mode defines at which bending angle cutoff occurs \n"
    "(0:>0, 1:>45, 2:>90, 3:>135, 4:>approx. 168, other:>approx. 179)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The result is a set of polygons which may be overlapping, but are not self-\n"
    "intersecting. Polygons may overlap afterwards because they grew big enough to overlap their neighbors.\n"
    "In that case, \\merge can be used to detect this overlaps by setting the \"min_wc\" parameter to value 1:\n"
    "\n"
    "@code\n"
    "r = RBA::Region::new\n"
    "r.insert(RBA::Box::new(0, 0, 50, 50))\n"
    "r.insert(RBA::Box::new(100, 0, 150, 50))\n"
    "r.size(50, 2)\n"
    "r.merge(false, 1)\n"
    "# r now is (50,-50;50,100;100,100;100,-50)\n"
    "@/code\n"
  ) + 
  method_ext ("size", &size_dvm, gsi::arg ("dv"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Anisotropic sizing (biasing)\n"
    "\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size(dv.x, dv.y, mode)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This variant has been introduced in version 0.28."
  ) +
  method ("size", (db::Region & (db::Region::*) (db::Coord, unsigned int)) &db::Region::size, gsi::arg ("d"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Isotropic sizing (biasing)\n"
    "\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size(d, d, mode)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) + 
  method ("sized", (db::Region (db::Region::*) (db::Coord, db::Coord, unsigned int) const) &db::Region::sized, gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Returns the anisotropically sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method returns the sized region (see \\size), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) + 
  method_ext ("sized", &sized_dvm, gsi::arg ("dv"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Returns the (an)isotropically sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method is equivalent to \"sized(dv.x, dv.y, mode)\".\n"
    "This method returns the sized region (see \\size), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This variant has been introduced in version 0.28."
  ) +
  method ("sized", (db::Region (db::Region::*) (db::Coord, unsigned int) const) &db::Region::sized, gsi::arg ("d"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Returns the isotropically sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method is equivalent to \"sized(d, d, mode)\".\n"
    "This method returns the sized region (see \\size), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("size_inside", &size_inside_ddm, gsi::arg ("inside"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("steps"), gsi::arg ("mode"),
    "@brief Incremental, anisotropic sizing inside of another region\n"
    "\n"
    "@param inside The region the incremental sizing will stay inside.\n"
    "@param dx The x sizing value\n"
    "@param dy The y sizing value\n"
    "@param steps The number of steps to take\n"
    "@param mode The sizing mode (see \\size)\n"
    "\n"
    "@return The region after the sizing has been applied (self)\n"
    "\n"
    "Sizes the region, keeping inside another region and performing the size in discrete steps.\n"
    "\n"
    "Using this method is equivalent to applying a single-step size and consecutively doing a boolean AND with the 'inside' region. "
    "This is repeated until the full sizing value is applied.\n"
    "\n"
    "This operation is employed to implement latch-up rules, where a device needs to be close to a well tap within the "
    "same well. For this, the tap footprint is incrementally sized, with the well as the 'inside' region. The steps is chosen so "
    "the per-step sizing is somewhat less than the minimum well space. Sizing the tap shape results in a growing footprint that "
    "follows the well contours and a small enough per-step sizing value ensures the sized contour does not cross well gaps.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("size_outside", &size_outside_ddm, gsi::arg ("outside"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("steps"), gsi::arg ("mode"),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("size_inside", &size_inside_dvm, gsi::arg ("inside"), gsi::arg ("dv"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, anisotropic sizing inside of another region\n"
    "\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size_inside(dv.x, dv.y, steps, mode)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("size_outside", &size_outside_dvm, gsi::arg ("outside"), gsi::arg ("dv"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("size_inside", &size_inside_dm, gsi::arg ("inside"), gsi::arg ("d"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, isotropic sizing inside of another region\n"
    "\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size_inside(d, d, steps, mode)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("size_outside", &size_outside_dm, gsi::arg ("outside"), gsi::arg ("d"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("sized_inside", &sized_inside_ddm, gsi::arg ("inside"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("steps"), gsi::arg ("mode"),
    "@brief Returns the incrementally and anisotropically sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method returns the incrementally sized region (see \\size_inside), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("sized_outside", &sized_outside_ddm, gsi::arg ("outside"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("steps"), gsi::arg ("mode"),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("sized_inside", &sized_inside_dvm, gsi::arg ("inside"), gsi::arg ("dv"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Returns the incrementally and anisotropically sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method returns the incrementally sized region (see \\size_inside), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This variant has been introduced in version 0.28."
  ) +
  method_ext ("sized_outside", &sized_outside_dvm, gsi::arg ("outside"), gsi::arg ("dv"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("sized_inside", &sized_inside_dm, gsi::arg ("inside"), gsi::arg ("d"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Returns the incrementally sized region\n"
    "\n"
    "@return The sized region\n"
    "\n"
    "This method returns the incrementally sized region (see \\size_inside), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("sized_outside", &sized_outside_dm, gsi::arg ("outside"), gsi::arg ("d"), gsi::arg ("steps"), gsi::arg ("mode", (unsigned int) 2),
    "@brief Incremental, anisotropic sizing outside of another region\n"
    "\n"
    "This method is equivalent to \\size_inside, except that sizing is performed outside the given 'outside' region. "
    "Technically this corresponds to a boolean 'NOT' operation instead of a boolean 'AND'.\n"
    "\n"
    "This method has been introduced in version 0.29.3."
  ) +
  method_ext ("andnot", &andnot, gsi::arg ("other"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Returns the boolean AND and NOT between self and the other region\n"
    "\n"
    "@return A two-element array of regions with the first one being the AND result and the second one being the NOT result\n"
    "\n"
    "This method will compute the boolean AND and NOT between two regions simultaneously. "
    "Because this requires a single sweep only, using this method is faster than doing AND and NOT separately.\n"
    "\n"
    "This method has been added in version 0.27.\n"
  ) +
  method ("&", &db::Region::operator&, gsi::arg ("other"),
    "@brief Returns the boolean AND between self and the other region\n"
    "\n"
    "@return The result of the boolean AND operation\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) +
  method ("and", &db::Region::bool_and, gsi::arg ("other"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Returns the boolean AND between self and the other region\n"
    "\n"
    "@return The result of the boolean AND operation\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "It allows specification of a property constaint - e.g. only performing the boolean operation between "
    "shapes with the same user properties.\n"
    "\n"
    "This variant has been introduced in version 0.28.4."
  ) +
  method ("&=", &db::Region::operator&=, gsi::arg ("other"),
    "@brief Performs the boolean AND between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "\n"
    "Note that in Ruby, the '&=' operator actually does not exist, but is emulated by '&' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'and_with' instead."
  ) +
  method ("and_with", &db::Region::bool_and_with, gsi::arg ("other"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Performs the boolean AND between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "It allows specification of a property constaint - e.g. only performing the boolean operation between "
    "shapes with the same user properties.\n"
    "\n"
    "This variant has been introduced in version 0.28.4."
  ) +
  method ("-", &db::Region::operator-, gsi::arg ("other"),
    "@brief Returns the boolean NOT between self and the other region\n"
    "\n"
    "@return The result of the boolean NOT operation\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) +
  method ("not", &db::Region::bool_not, gsi::arg ("other"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Returns the boolean NOT between self and the other region\n"
    "\n"
    "@return The result of the boolean NOT operation\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "It allows specification of a property constaint - e.g. only performing the boolean operation between "
    "shapes with the same user properties.\n"
    "\n"
    "This variant has been introduced in version 0.28.4."
  ) +
  method ("-=", &db::Region::operator-=, gsi::arg ("other"),
    "@brief Performs the boolean NOT between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "\n"
    "Note that in Ruby, the '-=' operator actually does not exist, but is emulated by '-' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'not_with' instead."
  ) +
  method ("not_with", &db::Region::bool_not_with, gsi::arg ("other"), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"),
    "@brief Performs the boolean NOT between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "It allows specification of a property constaint - e.g. only performing the boolean operation between "
    "shapes with the same user properties.\n"
    "\n"
    "This variant has been introduced in version 0.28.4."
  ) +
  method ("^|xor", &db::Region::operator^, gsi::arg ("other"),
    "@brief Returns the boolean XOR between self and the other region\n"
    "\n"
    "@return The result of the boolean XOR operation\n"
    "\n"
    "This method will compute the boolean XOR (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "\n"
    "The 'xor' alias has been introduced in version 0.28.12."
  ) +
  method ("^=|xor_with", &db::Region::operator^=, gsi::arg ("other"),
    "@brief Performs the boolean XOR between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean XOR (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
    "\n"
    "Note that in Ruby, the '^=' operator actually does not exist, but is emulated by '^' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'xor_with' instead.\n"
    "\n"
    "The 'xor_with' alias has been introduced in version 0.28.12."
  ) +
  method ("\\||or", &db::Region::operator|, gsi::arg ("other"),
    "@brief Returns the boolean OR between self and the other region\n"
    "\n"
    "@return The resulting region\n"
    "\n"
    "The boolean OR is implemented by merging the polygons of both regions. To simply join the regions "
    "without merging, the + operator is more efficient."
    "\n"
    "The 'or' alias has been introduced in version 0.28.12."
  ) +
  method ("\\|=|or_with", &db::Region::operator|=, gsi::arg ("other"),
    "@brief Performs the boolean OR between self and the other region in-place (modifying self)\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "The boolean OR is implemented by merging the polygons of both regions. To simply join the regions "
    "without merging, the + operator is more efficient."
    "\n"
    "Note that in Ruby, the '|=' operator actually does not exist, but is emulated by '|' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'or_with' instead.\n"
    "\n"
    "The 'or_with' alias has been introduced in version 0.28.12."
  ) +
  method ("+|join", &db::Region::operator+, gsi::arg ("other"),
    "@brief Returns the combined region of self and the other region\n"
    "\n"
    "@return The resulting region\n"
    "\n"
    "This operator adds the polygons of the other region to self and returns a new combined region. "
    "This usually creates unmerged regions and polygons may overlap. Use \\merge if you want to ensure the result region is merged.\n"
    "\n"
    "The 'join' alias has been introduced in version 0.28.12."
  ) +
  method ("+=|join_with", &db::Region::operator+=, gsi::arg ("other"),
    "@brief Adds the polygons of the other region to self\n"
    "\n"
    "@return The region after modification (self)\n"
    "\n"
    "This operator adds the polygons of the other region to self. "
    "This usually creates unmerged regions and polygons may overlap. Use \\merge if you want to ensure the result region is merged.\n"
    "\n"
    "Note that in Ruby, the '+=' operator actually does not exist, but is emulated by '+' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'join_with' instead.\n"
    "\n"
    "The 'join_with' alias has been introduced in version 0.28.12."
  ) +
  method ("covering", &db::Region::selected_enclosing, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are completely covering polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are covering polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This attribute is sometimes called 'enclosing' instead of 'covering', but this term is reserved for the respective DRC function.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("not_covering", &db::Region::selected_not_enclosing, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are not completely covering polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are not covering polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This attribute is sometimes called 'enclosing' instead of 'covering', but this term is reserved for the respective DRC function.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method_ext ("split_covering", &split_covering, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are completely covering polygons from the other region and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\covering, the second the result of \\not_covering\n"
    "\n"
    "This method is equivalent to calling \\covering and \\not_covering, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_covering", &db::Region::select_enclosing, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons of this region which are completely covering polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This attribute is sometimes called 'enclosing' instead of 'covering', but this term is reserved for the respective DRC function.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_not_covering", &db::Region::select_not_enclosing, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons of this region which are not completely covering polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This attribute is sometimes called 'enclosing' instead of 'covering', but this term is reserved for the respective DRC function.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("inside", &db::Region::selected_inside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are completely inside polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are inside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("not_inside", &db::Region::selected_not_inside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are not completely inside polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are not inside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_inside", &split_inside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are completely inside polygons from the other region and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\inside, the second the result of \\not_inside\n"
    "\n"
    "This method is equivalent to calling \\inside and \\not_inside, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_inside", &db::Region::select_inside, gsi::arg ("other"),
    "@brief Selects the polygons of this region which are completely inside polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("select_not_inside", &db::Region::select_not_inside, gsi::arg ("other"),
    "@brief Selects the polygons of this region which are not completely inside polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("outside", &db::Region::selected_outside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are completely outside polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are outside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("not_outside", &db::Region::selected_not_outside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are not completely outside polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons which are not outside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_outside", &split_outside, gsi::arg ("other"),
    "@brief Returns the polygons of this region which are completely outside polygons from the other region and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\outside, the second the result of \\not_outside\n"
    "\n"
    "This method is equivalent to calling \\outside and \\not_outside, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_outside", &db::Region::select_outside, gsi::arg ("other"),
    "@brief Selects the polygons of this region which are completely outside polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("select_not_outside", &db::Region::select_not_outside, gsi::arg ("other"),
    "@brief Selects the polygons of this region which are not completely outside polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method ("interacting", (db::Region (db::Region::*) (const db::Region &, size_t, size_t) const) &db::Region::selected_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which overlap or touch polygons from the other region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with (different) polygons of the other region to make the polygon selected. A polygon is "
    "selected by this method if the number of polygons interacting with a polygon of this region is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons overlapping or touching polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("not_interacting", (db::Region (db::Region::*) (const db::Region &, size_t, size_t) const) &db::Region::selected_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which do not overlap or touch polygons from the other region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with (different) polygons of the other region to make the polygon not selected. A polygon is not "
    "selected by this method if the number of polygons interacting with a polygon of this region is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons not overlapping or touching polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method_ext ("split_interacting", &split_interacting_with_region, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are interacting with polygons from the other region and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\interacting, the second the result of \\not_interacting\n"
    "\n"
    "This method is equivalent to calling \\interacting and \\not_interacting, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_interacting", (db::Region &(db::Region::*) (const db::Region &, size_t, size_t)) &db::Region::select_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which overlap or touch polygons from the other region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with (different) polygons of the other region to make the polygon selected. A polygon is "
    "selected by this method if the number of polygons interacting with a polygon of this region is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("select_not_interacting", (db::Region &(db::Region::*) (const db::Region &, size_t, size_t)) &db::Region::select_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which do not overlap or touch polygons from the other region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with (different) polygons of the other region to make the polygon not selected. A polygon is not "
    "selected by this method if the number of polygons interacting with a polygon of this region is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("interacting", (db::Region (db::Region::*) (const db::Edges &, size_t, size_t) const) &db::Region::selected_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which overlap or touch edges from the edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with edges of the edge collection to make the polygon selected. A polygon is "
    "selected by this method if the number of edges interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons overlapping or touching edges from the edge collection\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("not_interacting", (db::Region (db::Region::*) (const db::Edges &, size_t, size_t) const) &db::Region::selected_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which do not overlap or touch edges from the edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with edges of the edge collection to make the polygon not selected. A polygon is not "
    "selected by this method if the number of edges interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons not overlapping or touching edges from the edge collection\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.25\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method_ext ("split_interacting", &split_interacting_with_edges, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are interacting with edges from the other edge collection and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\interacting, the second the result of \\not_interacting\n"
    "\n"
    "This method is equivalent to calling \\interacting and \\not_interacting, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_interacting", (db::Region &(db::Region::*) (const db::Edges &, size_t, size_t)) &db::Region::select_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which overlap or touch edges from the edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with edges of the edge collection to make the polygon selected. A polygon is "
    "selected by this method if the number of edges interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.25\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("select_not_interacting", (db::Region &(db::Region::*) (const db::Edges &, size_t, size_t)) &db::Region::select_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which do not overlap or touch edges from the edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with edges of the edge collection to make the polygon not selected. A polygon is not "
    "selected by this method if the number of edges interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.25\n"
    "The min_count and max_count arguments have been added in version 0.27.\n"
  ) +
  method ("interacting", (db::Region (db::Region::*) (const db::Texts &, size_t, size_t) const) &db::Region::selected_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which overlap or touch texts\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with texts of the text collection to make the polygon selected. A polygon is "
    "selected by this method if the number of texts interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons overlapping or touching texts\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  method ("not_interacting", (db::Region (db::Region::*) (const db::Texts &, size_t, size_t) const) &db::Region::selected_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which do not overlap or touch texts\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with texts of the text collection to make the polygon not selected. A polygon is not "
    "selected by this method if the number of texts interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return A new region containing the polygons not overlapping or touching texts\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  method_ext ("split_interacting", &split_interacting_with_texts, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are interacting with texts from the other text collection and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\interacting, the second the result of \\not_interacting\n"
    "\n"
    "This method is equivalent to calling \\interacting and \\not_interacting, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_interacting", (db::Region &(db::Region::*) (const db::Texts &, size_t, size_t)) &db::Region::select_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons of this region which overlap or touch texts\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with texts of the text collection to make the polygon selected. A polygon is "
    "selected by this method if the number of texts interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  method ("select_not_interacting", (db::Region &(db::Region::*) (const db::Texts &, size_t, size_t)) &db::Region::select_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons of this region which do not overlap or touch texts\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times a polygon of this region "
    "has to interact with texts of the text collection to make the polygon not selected. A polygon is not "
    "selected by this method if the number of texts interacting with the polygon is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  method ("overlapping", &db::Region::selected_overlapping, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which overlap polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons overlapping polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The count options have been introduced in version 0.27."
  ) +
  method ("not_overlapping", &db::Region::selected_not_overlapping, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which do not overlap polygons from the other region\n"
    "\n"
    "@return A new region containing the polygons not overlapping polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The count options have been introduced in version 0.27."
  ) +
  method_ext ("split_overlapping", &split_overlapping, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the polygons of this region which are overlapping with polygons from the other region and the ones which are not at the same time\n"
    "\n"
    "@return Two new regions: the first containing the result of \\overlapping, the second the result of \\not_overlapping\n"
    "\n"
    "This method is equivalent to calling \\overlapping and \\not_overlapping, but is faster when both results are required.\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("select_overlapping", &db::Region::select_overlapping, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which overlap polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The count options have been introduced in version 0.27."
  ) +
  method ("select_not_overlapping", &db::Region::select_not_overlapping, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the polygons from this region which do not overlap polygons from the other region\n"
    "\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The count options have been introduced in version 0.27."
  ) +
  method ("pull_inside", &db::Region::pull_inside, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are inside polygons of this region\n"
    "The \"pull_...\" methods are similar to \"select_...\" but work the opposite way: they "
    "select shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" methods "
    "provide a way for re-hierarchization.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("pull_overlapping", &db::Region::pull_overlapping, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are overlapping polygons of this region\n"
    "See \\pull_inside for a description of the \"pull_...\" methods.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("pull_interacting", static_cast<db::Region (db::Region::*) (const db::Region &) const> (&db::Region::pull_interacting), gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are interacting with (overlapping, touching) polygons of this region\n"
    "See \\pull_inside for a description of the \"pull_...\" methods.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("pull_interacting", static_cast<db::Edges (db::Region::*) (const db::Edges &) const> (&db::Region::pull_interacting), gsi::arg ("other"),
    "@brief Returns all edges of \"other\" which are interacting with polygons of this region\n"
    "See \\pull_inside for a description of the \"pull_...\" methods.\n"
    "\n"
    "@return The edge collection after the edges have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("pull_interacting", static_cast<db::Texts (db::Region::*) (const db::Texts &) const> (&db::Region::pull_interacting), gsi::arg ("other"),
    "@brief Returns all texts of \"other\" which are interacting with polygons of this region\n"
    "See \\pull_inside for a description of the \"pull_...\" methods.\n"
    "\n"
    "@return The text collection after the texts have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  method ("is_box?", &db::Region::is_box,
    "@brief Returns true, if the region is a simple box\n"
    "\n"
    "@return True if the region is a box.\n"
    "\n"
    "This method does not apply implicit merging if merge semantics is enabled.\n"
    "If the region is not merged, this method may return false even\n"
    "if the merged region would be a box.\n"
  ) +
  method_ext ("edges", &edges, gsi::arg ("mode", db::PolygonToEdgeProcessor::All, "All"),
    "@brief Returns an edge collection representing all edges of the polygons in this region\n"
    "This method will decompose the polygons into the individual edges. Edges making up the hulls "
    "of the polygons are oriented clockwise while edges making up the holes are oriented counterclockwise.\n"
    "\n"
    "The 'mode' parameter allows selecting specific edges, such as convex or concave ones. By default, "
    "all edges are selected.\n"
    "\n"
    "The edge collection returned can be manipulated in various ways. See \\Edges for a description of the "
    "features of the edge collection.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The mode argument has been added in version 0.29."
  ) +
  factory_ext ("decompose_convex", &decompose_convex<db::Shapes>, gsi::arg ("preferred_orientation", po_any (), "\\Polygon#PO_any"),
    "@brief Decomposes the region into convex pieces.\n"
    "\n"
    "This method will return a \\Shapes container that holds a decomposition of the region into convex, simple polygons.\n"
    "See \\Polygon#decompose_convex for details. If you want \\Region output, you should use \\decompose_convex_to_region.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  factory_ext ("decompose_convex_to_region", &decompose_convex<db::Region>, gsi::arg ("preferred_orientation", po_any (), "\\Polygon#PO_any"),
    "@brief Decomposes the region into convex pieces into a region.\n"
    "\n"
    "This method is identical to \\decompose_convex, but delivers a \\Region object.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  factory_ext ("decompose_trapezoids", &decompose_trapezoids<db::Shapes>, gsi::arg ("mode", td_simple (), "\\Polygon#TD_simple"),
    "@brief Decomposes the region into trapezoids.\n"
    "\n"
    "This method will return a \\Shapes container that holds a decomposition of the region into trapezoids.\n"
    "See \\Polygon#decompose_trapezoids for details. If you want \\Region output, you should use \\decompose_trapezoids_to_region.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  factory_ext ("decompose_trapezoids_to_region", &decompose_trapezoids<db::Region>, gsi::arg ("mode", td_simple (), "\\Polygon#TD_simple"),
    "@brief Decomposes the region into trapezoids.\n"
    "\n"
    "This method is identical to \\decompose_trapezoids, but delivers a \\Region object.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("clear", &db::Region::clear,
    "@brief Clears the region\n"
  ) +
  method ("swap", &db::Region::swap, gsi::arg ("other"),
    "@brief Swap the contents of this region with the contents of another region\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. "
  ) +
  method ("holes", &db::Region::holes,
    "@brief Returns the holes of the region\n"
    "This method returns all holes as filled polygons.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merge semantics is not enabled, the holes may not be detected if the polygons "
    "are taken from a hole-less representation (i.e. GDS2 file). Use explicit merge (\\merge method) "
    "in order to merge the polygons and detect holes.\n"
  ) +
  method ("hulls", &db::Region::hulls,
    "@brief Returns the hulls of the region\n"
    "This method returns all hulls as polygons. The holes will be removed (filled). "
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merge semantics is not enabled, the hull may also enclose holes if the polygons "
    "are taken from a hole-less representation (i.e. GDS2 file). Use explicit merge (\\merge method) "
    "in order to merge the polygons and detect holes.\n"
  ) +
  method_ext ("members_of|in", &in, gsi::arg ("other"),
    "@brief Returns all polygons which are members of the other region\n"
    "This method returns all polygons in self which can be found in the other region as well with exactly the same "
    "geometry."
  ) +
  method_ext ("not_members_of|not_in", &not_in, gsi::arg ("other"),
    "@brief Returns all polygons which are not members of the other region\n"
    "This method returns all polygons in self which can not be found in the other region with exactly the same "
    "geometry."
  ) +
  method_ext ("in_and_out", &in_and_out, gsi::arg ("other"),
    "@brief Returns all polygons which are members and not members of the other region\n"
    "This method is equivalent to calling \\members_of and \\not_members_of, but delivers both results at the same time and "
    "is more efficient than two separate calls. "
    "The first element returned is the \\members_of part, the second is the \\not_members_of part.\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  method_ext ("filter", &filter, gsi::arg ("filter"),
    "@brief Applies a generic filter in place (replacing the polygons from the Region)\n"
    "See \\PolygonFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("filtered", &filtered, gsi::arg ("filter"),
    "@brief Applies a generic filter and returns a filtered copy\n"
    "See \\PolygonFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("split_filter", &split_filter, gsi::arg ("filter"),
    "@brief Applies a generic filter and returns a copy with all matching shapes and one with the non-matching ones\n"
    "See \\PolygonFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("process", &process_pp, gsi::arg ("process"),
    "@brief Applies a generic polygon processor in place (replacing the polygons from the Region)\n"
    "See \\PolygonProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_pp, gsi::arg ("processed"),
    "@brief Applies a generic polygon processor and returns a processed copy\n"
    "See \\PolygonProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_pep, gsi::arg ("processed"),
    "@brief Applies a generic polygon-to-edge-pair processor and returns an edge pair collection with the results\n"
    "See \\PolygonToEdgePairProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_pe, gsi::arg ("processed"),
    "@brief Applies a generic polygon-to-edge processor and returns an edge collection with the results\n"
    "See \\PolygonToEdgeProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("rectangles", &rectangles,
    "@brief Returns all polygons which are rectangles\n"
    "This method returns all polygons in self which are rectangles."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("non_rectangles", &non_rectangles,
    "@brief Returns all polygons which are not rectangles\n"
    "This method returns all polygons in self which are not rectangles."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_rectangles", &split_rectangles,
    "@brief Combined results of \\rectangles and \\non_rectangles\n"
    "This method returns a list with two Regions, the first is the result of \\rectangles, the second the result of \\non_rectangles. "
    "Using this method is faster when you need both.\n\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("squares", &squares,
    "@brief Returns all polygons which are squares\n"
    "This method returns all polygons in self which are squares."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("non_squares", &non_squares,
    "@brief Returns all polygons which are not squares\n"
    "This method returns all polygons in self which are not squares."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("split_squares", &split_squares,
    "@brief Combined results of \\squares and \\non_squares\n"
    "This method returns a list with two Regions, the first is the result of \\squares, the second the result of \\non_squares. "
    "Using this method is faster when you need both.\n\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("rectilinear", &rectilinear,
    "@brief Returns all polygons which are rectilinear\n"
    "This method returns all polygons in self which are rectilinear."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("non_rectilinear", &non_rectilinear,
    "@brief Returns all polygons which are not rectilinear\n"
    "This method returns all polygons in self which are not rectilinear."
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("split_rectilinear", &split_rectilinear,
    "@brief Combined results of \\rectilinear and \\non_rectilinear\n"
    "This method returns a list with two Regions, the first is the result of \\rectilinear, the second the result of \\non_rectilinear. "
    "Using this method is faster when you need both.\n\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("break_polygons|#break", &break_polygons, gsi::arg ("max_vertex_count"), gsi::arg ("max_area_ratio", 0.0),
    "@brief Breaks the polygons of the region into smaller ones\n"
    "\n"
    "There are two criteria for splitting a polygon: a polygon is split into parts with less then "
    "'max_vertex_count' points and an bounding box-to-polygon area ratio less than 'max_area_ratio'. "
    "The area ratio is supposed to render polygons whose bounding box is a better approximation. "
    "This applies for example to 'L' shape polygons.\n"
    "\n"
    "Using a value of 0 for either limit means that the respective limit isn't checked. "
    "Breaking happens by cutting the polygons into parts at 'good' locations. The "
    "algorithm does not have a specific goal to minimize the number of parts for example. "
    "The only goal is to achieve parts within the given limits.\n"
    "\n"
    "This method has been introduced in version 0.26. The 'break_polygons' alias has been introduced "
    "in version 0.29.5 to avoid issues with reserved keywords."
  ) +
  method_ext ("delaunay", &delaunay,
    "@brief Computes a constrained Delaunay triangulation from the given region\n"
    "\n"
    "@return A new region holding the triangles of the constrained Delaunay triangulation.\n"
    "\n"
    "Note that the result is a region in raw mode as otherwise the triangles are likely to get "
    "merged later on.\n"
    "\n"
    "This method has been introduced in version 0.29."
  ) +
  method_ext ("delaunay", &refined_delaunay, gsi::arg ("max_area"), gsi::arg ("min_b", 1.0),
    "@brief Computes a refined, constrained Delaunay triangulation from the given region\n"
    "\n"
    "@return A new region holding the triangles of the refined, constrained Delaunay triangulation.\n"
    "\n"
    "Refinement is implemented by Chew's second algorithm. A maximum area can be given. Triangles "
    "larger than this area will be split. In addition 'skinny' triangles will be resolved where "
    "possible. 'skinny' is defined in terms of shortest edge to circumcircle radius ratio (b). "
    "A minimum number for b can be given. The default of 1.0 corresponds to a minimum angle of 30 degree "
    "and is usually a good choice. The algorithm is stable up to roughly 1.2 which corresponds to "
    "a minimum angle of abouth 37 degree.\n"
    "\n"
    "The minimum angle of the resulting triangles relates to the 'b' parameter as: @t min_angle = arcsin(B/2) @/t.\n"
    "\n"
    "The area value is given in terms of DBU units. Picking a value of 0.0 for area and min b will "
    "make the implementation skip the refinement step. In that case, the results are identical to "
    "the standard constrained Delaunay triangulation.\n"
    "\n"
    "Note that the result is a region in raw mode as otherwise the triangles are likely to get "
    "merged later on.\n"
    "\n"
    "This method has been introduced in version 0.29."
  ) +
  method_ext ("minkowski_sum|#minkowsky_sum", &minkowski_sum_pe, gsi::arg ("e"),
    "@brief Compute the Minkowski sum of the region and an edge\n"
    "\n"
    "@param e The edge.\n"
    "\n"
    "@return The new polygons representing the Minkowski sum with the edge e.\n"
    "\n"
    "The Minkowski sum of a region and an edge basically results in the area covered when "
    "\"dragging\" the region along the line given by the edge. The effect is similar to drawing the line "
    "with a pencil that has the shape of the given region.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("minkowski_sum|#minkowsky_sum", &minkowski_sum_pp, gsi::arg ("p"),
    "@brief Compute the Minkowski sum of the region and a polygon\n"
    "\n"
    "@param p The first argument.\n"
    "\n"
    "@return The new polygons representing the Minkowski sum of self and p.\n"
    "\n"
    "The Minkowski sum of a region and a polygon is basically the result of \"painting\" "
    "the region with a pen that has the shape of the second polygon.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("minkowski_sum|#minkowsky_sum", &minkowski_sum_pb, gsi::arg ("b"),
    "@brief Compute the Minkowski sum of the region and a box\n"
    "\n"
    "@param b The box.\n"
    "\n"
    "@return The new polygons representing the Minkowski sum of self and the box.\n"
    "\n"
    "The result is equivalent to the region-with-polygon Minkowski sum with the box used "
    "as the second polygon.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("minkowski_sum|#minkowsky_sum", &minkowski_sum_pc, gsi::arg ("b"),
    "@brief Compute the Minkowski sum of the region and a contour of points (a trace)\n"
    "\n"
    "@param b The contour (a series of points forming the trace).\n"
    "\n"
    "@return The new polygons representing the Minkowski sum of self and the contour.\n"
    "\n"
    "The Minkowski sum of a region and a contour basically results in the area covered when "
    "\"dragging\" the region along the contour. The effect is similar to drawing the contour "
    "with a pencil that has the shape of the given region.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
  ) +
  method_ext ("drc_hull", &drc_hull, gsi::arg ("metrics"), gsi::arg ("space"), gsi::arg ("n_circle", size_t (64)),
    "@brief Computes a visualization of the forbidden region for a DRC space check\n"
    "\n"
    "@param metrics The metrics to apply\n"
    "@param space The space value to apply\n"
    "@param n_circle The full-circle number of points for the Euclidian space visualization\n"
    "\n"
    "@return The new polygons representing the forbidden region.\n"
    "\n"
    "This method has been introduced in version 0.29.1.\n"
  ) +
  method_ext ("move", &move_p, gsi::arg ("v"),
    "@brief Moves the region\n"
    "\n"
    "Moves the polygon by the given offset and returns the \n"
    "moved region. The region is overwritten.\n"
    "\n"
    "@param v The distance to move the region.\n"
    "\n"
    "Starting with version 0.25 this method accepts a vector argument.\n"
    "\n"
    "@return The moved region (self).\n"
  ) +
  method_ext ("move", &move_xy, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Moves the region\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is overwritten.\n"
    "\n"
    "@param dx The x distance to move the region.\n"
    "@param dy The y distance to move the region.\n"
    "\n"
    "@return The moved region (self).\n"
  ) +
  method_ext ("moved", &moved_p, gsi::arg ("v"),
    "@brief Returns the moved region (does not modify self)\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is not modified.\n"
    "\n"
    "Starting with version 0.25 this method accepts a vector argument.\n"
    "\n"
    "@param v The distance to move the region.\n"
    "\n"
    "@return The moved region.\n"
  ) +
  method_ext ("moved", &moved_xy, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Returns the moved region (does not modify self)\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is not modified.\n"
    "\n"
    "@param dx The x distance to move the region.\n"
    "@param dy The y distance to move the region.\n"
    "\n"
    "@return The moved region.\n"
  ) +
  method ("transform", (db::Region &(db::Region::*)(const db::Trans &)) &db::Region::transform, gsi::arg ("t"),
    "@brief Transform the region (modifies self)\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transform|#transform_icplx", (db::Region &(db::Region::*)(const db::ICplxTrans &)) &db::Region::transform, gsi::arg ("t"),
    "@brief Transform the region with a complex transformation (modifies self)\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transform", (db::Region &(db::Region::*)(const db::IMatrix2d &)) &db::Region::transform, gsi::arg ("t"),
    "@brief Transform the region (modifies self)\n"
    "\n"
    "Transforms the region with the given 2d matrix transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
    "\n"
    "This variant was introduced in version 0.27.\n"
  ) +
  method ("transform", (db::Region &(db::Region::*)(const db::IMatrix3d &)) &db::Region::transform, gsi::arg ("t"),
    "@brief Transform the region (modifies self)\n"
    "\n"
    "Transforms the region with the given 3d matrix transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
    "\n"
    "This variant was introduced in version 0.27.\n"
  ) +
  method ("transformed", (db::Region (db::Region::*)(const db::Trans &) const) &db::Region::transformed, gsi::arg ("t"),
    "@brief Transforms the region\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::Region (db::Region::*)(const db::ICplxTrans &) const) &db::Region::transformed, gsi::arg ("t"),
    "@brief Transforms the region with a complex transformation\n"
    "\n"
    "Transforms the region with the given complex transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transformed", (db::Region (db::Region::*)(const db::IMatrix2d &) const) &db::Region::transformed, gsi::arg ("t"),
    "@brief Transforms the region\n"
    "\n"
    "Transforms the region with the given 2d matrix transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
    "\n"
    "This variant was introduced in version 0.27.\n"
  ) +
  method ("transformed", (db::Region (db::Region::*)(const db::IMatrix3d &) const) &db::Region::transformed, gsi::arg ("t"),
    "@brief Transforms the region\n"
    "\n"
    "Transforms the region with the given 3d matrix transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
    "\n"
    "This variant was introduced in version 0.27.\n"
  ) +
  method_ext ("width_check", &width2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs a width check with options\n"
    "@param d The minimum width for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param negative If true, edges not violation the condition will be output as pseudo-edge pairs\n"
    "@param property_constraint Only \\IgnoreProperties and \\NoPropertyConstraint are allowed - in the last case, properties are copied from the original shapes to the output. "
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "Other than 'width' allow more options here.\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded' and 'negative' options have been introduced in version 0.27. "
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.28.16."
  ) +
  method_ext ("space_check", &space2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs a space check with options\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative If true, edges not violation the condition will be output as pseudo-edge pairs\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("notch_check", &notch2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs a space check between edges of the same polygon with options\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param negative If true, edges not violation the condition will be output as pseudo-edge pairs\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param property_constraint Only \\IgnoreProperties and \\NoPropertyConstraint are allowed - in the last case, properties are copied from the original shapes to the output"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the space check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded' and 'negative' options have been introduced in version 0.27.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("isolated_check", &isolated2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs a space check between edges of different polygons with options\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative If true, edges not violation the condition will be output as pseudo-edge pairs\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("inside_check|enclosed_check", &inside2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs an inside check with options\n"
    "@param d The minimum distance for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative Negative output from the first input\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "If \"negative\" is true, only edges from the first input are output as pseudo edge-pairs where the distance is "
    "larger or equal to the limit. This is a way to flag the parts of the first input where the distance to the second "
    "input is bigger. Note that only the first input's edges are output. The output is still edge pairs, but each edge pair "
    "contains one edge from the original input and the reverse version of the edge as the second edge.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27. "
    "The interpretation of the 'negative' flag has been restriced to first-layout only output in 0.27.1.\n"
    "The 'enclosed_check' alias was introduced in version 0.27.5.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("overlap_check", &overlap2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs an overlap check with options\n"
    "@param d The minimum overlap for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative Negative output from the first input\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "If \"negative\" is true, only edges from the first input are output as pseudo edge-pairs where the overlap is "
    "larger or equal to the limit. This is a way to flag the parts of the first input where the overlap vs. the second "
    "input is bigger. Note that only the first input's edges are output. The output is still edge pairs, but each edge pair "
    "contains one edge from the original input and the reverse version of the edge as the second edge.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27. "
    "The interpretation of the 'negative' flag has been restriced to first-layout only output in 0.27.1.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("enclosing_check", &enclosing2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs an enclosing check with options\n"
    "@param d The minimum enclosing distance for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative Negative output from the first input\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "If \"negative\" is true, only edges from the first input are output as pseudo edge-pairs where the enclosure is "
    "larger or equal to the limit. This is a way to flag the parts of the first input where the enclosure vs. the second "
    "input is bigger. Note that only the first input's edges are output. The output is still edge pairs, but each edge pair "
    "contains one edge from the original input and the reverse version of the edge as the second edge.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27. "
    "The interpretation of the 'negative' flag has been restriced to first-layout only output in 0.27.1.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("separation_check", &separation2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::metrics_type::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false), gsi::arg ("property_constraint", db::IgnoreProperties, "IgnoreProperties"), gsi::arg ("zero_distance_mode", db::IncludeZeroDistanceWhenTouching, "IncludeZeroDistanceWhenTouching"),
    "@brief Performs a separation check with options\n"
    "@param d The minimum separation for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "@param shielded Enables shielding (see below)\n"
    "@param opposite_filter Specifies a filter mode for errors happening on opposite sides of inputs shapes\n"
    "@param rect_filter Specifies an error filter for rectangular input shapes\n"
    "@param negative Negative output from the first input\n"
    "@param property_constraint Specifies whether to consider only shapes with a certain property relation\n"
    "@param zero_distance_mode Specifies how to handle edges with zero distance\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "\"shielded\" controls whether shielding is applied. Shielding means that rule violations are not "
    "detected 'through' other features. Measurements are only made where the opposite edge is unobstructed.\n"
    "Shielding often is not optional as a rule violation in shielded case automatically comes with rule "
    "violations between the original and the shielding features. If not necessary, shielding can be disabled by setting this flag to "
    "false. In general, this will improve performance somewhat.\n"
    "\n"
    "\"opposite_filter\" specifies whether to require or reject errors happening on opposite sides of a figure. "
    "\"rect_filter\" allows suppressing specific error configurations on rectangular input figures.\n"
    "\n"
    "If \"negative\" is true, only edges from the first input are output as pseudo edge-pairs where the separation is "
    "larger or equal to the limit. This is a way to flag the parts of the first input where the distance to the second "
    "input is bigger. Note that only the first input's edges are output. The output is still edge pairs, but each edge pair "
    "contains one edge from the original input and the reverse version of the edge as the second edge.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= for a description of this concept)\n"
    "\n"
    "The 'shielded', 'negative', 'not_opposite' and 'rect_sides' options have been introduced in version 0.27. "
    "The interpretation of the 'negative' flag has been restriced to first-layout only output in 0.27.1.\n"
    "'property_constraint' has been added in version 0.28.4.\n"
    "'zero_distance_mode' has been added in version 0.29."
  ) +
  method_ext ("area", &area1,
    "@brief The area of the region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merged semantics is not enabled, overlapping areas are counted twice.\n"
  ) +
  method_ext ("area", &area2, gsi::arg ("rect"),
    "@brief The area of the region (restricted to a rectangle)\n"
    "This version will compute the area of the shapes, restricting the computation to the given rectangle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merged semantics is not enabled, overlapping areas are counted twice.\n"
  ) +
  method_ext ("perimeter", &perimeter1,
    "@brief The total perimeter of the polygons\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merged semantics is not enabled, internal edges are counted as well.\n"
  ) +
  method_ext ("perimeter", &perimeter2, gsi::arg ("rect"),
    "@brief The total perimeter of the polygons (restricted to a rectangle)\n"
    "This version will compute the perimeter of the polygons, restricting the computation to the given rectangle.\n"
    "Edges along the border are handled in a special way: they are counted when they are oriented with their inside "
    "side toward the rectangle (in other words: outside edges must coincide with the rectangle's border in order to be counted).\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= for a description of this concept)\n"
    "If merged semantics is not enabled, internal edges are counted as well.\n"
  ) +
  method ("bbox", &db::Region::bbox,
    "@brief Return the bounding box of the region\n"
    "The bounding box is the box enclosing all points of all polygons.\n"
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the region is a deep (hierarchical) one\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method ("is_merged?", &db::Region::is_merged,
    "@brief Returns true if the region is merged\n"
    "If the region is merged, polygons will not touch or overlap. You can ensure merged state "
    "by calling \\merge.\n"
  ) +
  method ("is_empty?", &db::Region::empty,
    "@brief Returns true if the region is empty\n"
  ) +
  method ("count|#size", (size_t (db::Region::*) () const) &db::Region::count,
    "@brief Returns the (flat) number of polygons in the region\n"
    "\n"
    "This returns the number of raw polygons (not merged polygons if merged semantics is enabled).\n"
    "The count is computed 'as if flat', i.e. polygons inside a cell are multiplied by the number of times a cell is instantiated.\n"
    "\n"
    "The 'count' alias has been provided in version 0.26 to avoid ambiguity with the 'size' method which applies a geometrical bias."
  ) +
  method ("hier_count", (size_t (db::Region::*) () const) &db::Region::hier_count,
    "@brief Returns the (hierarchical) number of polygons in the region\n"
    "\n"
    "This returns the number of raw polygons (not merged polygons if merged semantics is enabled).\n"
    "The count is computed 'hierarchical', i.e. polygons inside a cell are counted once even if the cell is instantiated multiple times.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  iterator_ext ("each", &begin_region,
    "@brief Returns each polygon of the region\n"
    "\n"
    "This returns the raw polygons (not merged polygons if merged semantics is enabled).\n"
    "\n"
    "Starting with version 0.30, the iterator delivers a RegionWithProperties object."
  ) +
  iterator_ext ("each_merged", &begin_region_merged,
    "@brief Returns each merged polygon of the region\n"
    "\n"
    "This returns the raw polygons if merged semantics is disabled or the merged ones if merged semantics is enabled.\n"
    "Starting with version 0.30, the iterator delivers a RegionWithProperties object."
  ) +
  method_ext ("[]", &nth, gsi::arg ("n"),
    "@brief Returns the nth polygon of the region\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat regions only - i.e. "
    "those for which \\has_valid_polygons? is true. Use \\flatten to explicitly flatten a region.\n"
    "This method returns the raw polygon (not merged polygons, even if merged semantics is enabled).\n"
    "\n"
    "The \\each iterator is the more general approach to access the polygons.\n"
    "\n"
    "Since version 0.30.1, this method returns a \\PolygonWithProperties object."
  ) +
  method ("flatten", &db::Region::flatten,
    "@brief Explicitly flattens a region\n"
    "\n"
    "If the region is already flat (i.e. \\has_valid_polygons? returns true), this method will "
    "not change it.\n"
    "\n"
    "Returns 'self', so this method can be used in a dot concatenation.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("has_valid_polygons?", &db::Region::has_valid_polygons,
    "@brief Returns true if the region is flat and individual polygons can be accessed randomly\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the region to a string\n"
    "The length of the output is limited to 20 polygons to avoid giant strings on large regions. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1, gsi::arg ("max_count"),
    "@brief Converts the region to a string\n"
    "This version allows specification of the maximum number of polygons contained in the string."
  ) +
  method_ext ("rasterize", &rasterize1, gsi::arg ("origin"), gsi::arg ("pixel_size"), gsi::arg ("nx"), gsi::arg ("ny"),
    "@brief A grayscale rasterizer delivering the area covered per pixel\n"
    "@param origin The lower-left corner of the lowest-left pixel\n"
    "@param pixel_size The dimension of each pixel (the x component gives the width, the y component the height)\n"
    "@param nx The number of pixels in horizontal direction\n"
    "@param ny The number of pixels in vertical direction\n"
    "The method will create a grayscale, high-resolution density map of a rectangular region.\n"
    "The scan region is defined by the origin, the pixel size and the number of pixels in horizontal (nx) and\n"
    "vertical (ny) direction. The resulting array will contain the area covered by polygons from the region\n"
    "in square database units.\n"
    "\n"
    "For non-overlapping polygons, the maximum density value is px*py. Overlapping polygons are counted multiple\n"
    "times, so the actual values may be larger. If you want overlaps removed, you have to\n"
    "merge the region before. Merge semantics does not apply for the 'rasterize' method.\n"
    "\n"
    "The resulting area values are precise within the limits of double-precision floating point arithmetics.\n"
    "\n"
    "A second version exists that allows specifying an active pixel size which is smaller than the\n"
    "pixel distance hence allowing pixels samples that do not cover the full area, but leave gaps between the pixels.\n"
    "\n"
    "This method has been added in version 0.29.\n"
  ) +
  method_ext ("rasterize", &rasterize2, gsi::arg ("origin"), gsi::arg ("pixel_distance"), gsi::arg ("pixel_size"), gsi::arg ("nx"), gsi::arg ("ny"),
    "@brief A version of 'rasterize' that allows a pixel step distance which is larger than the pixel size\n"
    "This version behaves like the first variant of 'rasterize', but the pixel distance (pixel-to-pixel step raster)\n"
    "can be specified separately from the pixel size. Currently, the pixel size must be equal or smaller than the\n"
    "pixel distance - i.e. the pixels must not overlap.\n"
    "\n"
    "This method has been added in version 0.29.\n"
  ) +
  method ("enable_progress", &db::Region::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the region will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::Region::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method ("base_verbosity=", &db::Region::set_base_verbosity, gsi::arg ("verbosity"),
    "@brief Sets the minimum verbosity for timing reports\n"
    "Timing reports will be given only if the verbosity is larger than this value. "
    "Detailed reports will be given when the verbosity is more than this value plus 10.\n"
    "In binary operations, the base verbosity of the first argument is considered.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  method ("base_verbosity", &db::Region::base_verbosity,
    "@brief Gets the minimum verbosity for timing reports\n"
    "See \\base_verbosity= for details.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("fill", &fill_region, gsi::arg ("in_cell"),
                                         gsi::arg ("fill_cell_index"),
                                         gsi::arg ("fc_box"),
                                         gsi::arg ("origin", &default_origin, "(0, 0)"),
                                         gsi::arg ("remaining_parts", (db::Region *)0, "nil"),
                                         gsi::arg ("fill_margin", db::Vector ()),
                                         gsi::arg ("remaining_polygons", (db::Region *)0, "nil"),
                                         gsi::arg ("glue_box", db::Box ()),
                                         gsi::arg ("exclude_area", db::Region (), "empty"),
    "@brief A mapping of \\Cell#fill_region to the Region class\n"
    "\n"
    "This method is equivalent to \\Cell#fill_region, but is based on Region (with the cell being the first parameter).\n"
    "\n"
    "This method has been introduced in version 0.27. The 'exclude_area' argument has been added in version 0.30.4.\n"
  ) +
  gsi::method_ext ("fill", &fill_region_skew, gsi::arg ("in_cell"),
                                              gsi::arg ("fill_cell_index"),
                                              gsi::arg ("fc_bbox"),
                                              gsi::arg ("row_step"),
                                              gsi::arg ("column_step"),
                                              gsi::arg ("origin", &default_origin, "(0, 0)"),
                                              gsi::arg ("remaining_parts", (db::Region *)0, "nil"),
                                              gsi::arg ("fill_margin", db::Vector ()),
                                              gsi::arg ("remaining_polygons", (db::Region *)0, "nil"),
                                              gsi::arg ("glue_box", db::Box ()),
                                              gsi::arg ("exclude_area", db::Region (), "empty"),
    "@brief A mapping of \\Cell#fill_region to the Region class\n"
    "\n"
    "This method is equivalent to \\Cell#fill_region, but is based on Region (with the cell being the first parameter).\n"
    "\n"
    "This method has been introduced in version 0.27. The 'exclude_area' argument has been added in version 0.30.4.\n"
  ) +
  gsi::method_ext ("fill_multi", &fill_region_multi, gsi::arg ("in_cell"),
                                                     gsi::arg ("fill_cell_index"),
                                                     gsi::arg ("fc_bbox"),
                                                     gsi::arg ("row_step"),
                                                     gsi::arg ("column_step"),
                                                     gsi::arg ("fill_margin", db::Vector ()),
                                                     gsi::arg ("remaining_polygons", (db::Region *)0, "nil"),
                                                     gsi::arg ("glue_box", db::Box ()),
                                                     gsi::arg ("exclude_area", db::Region (), "empty"),
    "@brief A mapping of \\Cell#fill_region to the Region class\n"
    "\n"
    "This method is equivalent to \\Cell#fill_region, but is based on Region (with the cell being the first parameter).\n"
    "\n"
    "This method has been introduced in version 0.27. The 'exclude_area' argument has been added in version 0.30.4.\n"
  ) +
  gsi::method_ext ("nets", &nets, gsi::arg ("extracted"), gsi::arg ("net_prop_name", tl::Variant (), "nil"), gsi::arg ("net_filter", (const std::vector<const db::Net *> *) (0), "nil"),
    "@brief Pulls the net shapes from a LayoutToNetlist database\n"
    "This method will create a new layer with the net shapes from the LayoutToNetlist database, provided that this "
    "region was an input to the netlist extraction on this database.\n"
    "\n"
    "A (circuit name, net name) tuple will be attached as properties to the shapes if 'net_prop_name' is given and not nil. "
    "This allows generating unique properties per shape, flagging the net the shape is on. This feature is good for "
    "performing net-dependent booleans and DRC checks.\n"
    "\n"
    "A net filter can be provided with the 'net_filter' argument. If given, only nets from this "
    "set are produced. Example:\n"
    "\n"
    "@code\n"
    "connect(metal1, via1)\n"
    "connect(via1, metal2)\n"
    "\n"
    "metal1_all_nets = metal1.nets\n"
    "@/code\n"
    "\n"
    "This method was introduced in version 0.28.4."
  ) +
  gsi::method_ext ("begin_shapes_rec", &begin_shapes_rec,
    "@brief Returns a recursive shape iterator plus a transformation for the shapes constituting this region.\n"
    "This method returns a pair consisting of a \\RecursiveShapeIterator plus a \\ICplxTrans transformation. "
    "Both objects allow accessing the shapes (polygons) of the region in a detailed fashion. To iterate the "
    "the polygons use a code like this:\n"
    "\n"
    "@code\n"
    "iter, trans = region.begin_shapes_rec\n"
    "iter.each do |i|\n"
    "  polygon = trans * iter.trans * i.shape.polygon\n"
    "  ...\n"
    "end\n"
    "@/code\n"
    "\n"
    "This method is the most powerful way of accessing the shapes inside the region. I allows for example to obtain the "
    "properties attached to the polygons of the region. It is primarily intended for special applications like iterating net-annotated shapes.\n"
    "\n"
    "This speciality method was introduced in version 0.29.5."
  ) +
  gsi::method_ext ("begin_merged_shapes_rec", &begin_merged_shapes_rec,
    "@brief Returns a recursive shape iterator plus a transformation for the shapes constituting the merged region.\n"
    "It can be used like \\begin_shapes_rec, but delivers shapes from the merged polygons pool.\n"
    "\n"
    "This speciality method was introduced in version 0.29.5."
  ) +
  gsi::make_property_methods<db::Region> ()
  ,
  "@brief A region (a potentially complex area consisting of multiple polygons)\n"
  "\n\n"
  "This class was introduced to simplify operations on polygon sets like boolean or sizing operations. "
  "Regions consist of many polygons and thus are a generalization of single polygons which describes "
  "a single coherence set of points. Regions support a variety of operations and have several states. "
  "\n\n"
  "The region's state can be empty (does not contain anything) or box-like, i.e. the region consists "
  "of a single box. In that case, some operations can be simplified. Regions can have merged state. In merged "
  "state, regions consist of merged (non-touching, non-self overlapping) polygons. Each polygon describes "
  "one coherent area in merged state."
  "\n\n"
  "The preferred representation of polygons inside the region are polygons with holes."
  "\n\n"
  "Regions are always expressed in database units. If you want to use regions from different database unit "
  "domains, scale the regions accordingly, i.e. by using the \\transformed method.\n"
  "\n\n"
  "Regions provide convenient operators for the boolean operations. Hence it is often no longer required "
  "to work with the \\EdgeProcessor class. For example:\n"
  "\n"
  "@code\n"
  "r1 = RBA::Region::new(RBA::Box::new(0, 0, 100, 100))\n"
  "r2 = RBA::Region::new(RBA::Box::new(20, 20, 80, 80))\n"
  "# compute the XOR:\n"
  "r1_xor_r2 = r1 ^ r2\n"
  "@/code\n"
  "\n"
  "Regions can be used in two different flavors: in raw mode or merged semantics. With merged semantics (the "
  "default), connected polygons are considered to belong together and are effectively merged.\n"
  "Overlapping areas are counted once in that mode. Internal edges (i.e. arising from cut lines) are not considered.\n"
  "In raw mode (without merged semantics), each polygon is considered as it is. Overlaps between polygons\n"
  "may exists and merging has to be done explicitly using the \\merge method. The semantics can be\n"
  "selected using \\merged_semantics=.\n"
  "\n\n"
  "This class has been introduced in version 0.23.\n"
);

gsi::Enum<db::metrics_type> decl_Metrics ("db", "Metrics",
  gsi::enum_const ("Euclidian", db::Euclidian,
    "@brief Specifies Euclidian metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies Euclidian metrics, i.e. the distance between two points is measured by:\n"
    "\n"
    "@code\n"
    "d = sqrt(dx^2 + dy^2)\n"
    "@/code\n"
    "\n"
    "All points within a circle with radius d around one point are considered to have a smaller distance than d."
  ) +
  gsi::enum_const ("Square", db::Square,
    "@brief Specifies square metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies square metrics, i.e. the distance between two points is measured by:\n"
    "\n"
    "@code\n"
    "d = max(abs(dx), abs(dy))\n"
    "@/code\n"
    "\n"
    "All points within a square with length 2*d around one point are considered to have a smaller distance than d in this metrics."
  ) +
  gsi::enum_const ("Projection", db::Projection,
    "@brief Specifies projected distance metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies projected metrics, i.e. the distance is defined as the minimum distance "
    "measured perpendicular to one edge. That implies that the distance is defined only where two "
    "edges have a non-vanishing projection onto each other."
  ),
  "@brief This class represents the metrics type for \\Region#width and related checks.\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

//  Inject the Region::Metrics declarations into Region and Edges:
//  (Edges injection has to be done here because only here defs() is available)
gsi::ClassExt<db::Region> inject_Metrics_in_Region (decl_Metrics.defs ());
gsi::ClassExt<db::Edges> inject_Metrics_in_Edges (decl_Metrics.defs ());

gsi::Enum<db::PolygonToEdgeProcessor::EdgeMode> decl_EdgeMode ("db", "EdgeMode",
  gsi::enum_const ("All", db::PolygonToEdgeProcessor::All,
    "@brief Selects all edges\n"
  ) +
  gsi::enum_const ("Concave", db::PolygonToEdgeProcessor::Concave,
    "@brief Selects only concave edges\n"
  ) +
  gsi::enum_const ("NotConcave", db::PolygonToEdgeProcessor::NotConcave,
    "@brief Selects only edges which are not concave\n"
  ) +
  gsi::enum_const ("Convex", db::PolygonToEdgeProcessor::Convex,
    "@brief Selects only convex edges\n"
  ) +
  gsi::enum_const ("NotConvex", db::PolygonToEdgeProcessor::NotConvex,
    "@brief Selects only edges which are not convex\n"
  ) +
  gsi::enum_const ("Step", db::PolygonToEdgeProcessor::Step,
    "@brief Selects only step edges leading inside or outside\n"
  ) +
  gsi::enum_const ("NotStep", db::PolygonToEdgeProcessor::NotStep,
    "@brief Selects only edges which are not steps\n"
  ) +
  gsi::enum_const ("StepIn", db::PolygonToEdgeProcessor::StepIn,
    "@brief Selects only step edges leading inside\n"
  ) +
  gsi::enum_const ("NotStepIn", db::PolygonToEdgeProcessor::NotStepIn,
    "@brief Selects only edges which are not steps leading inside\n"
  ) +
  gsi::enum_const ("StepOut", db::PolygonToEdgeProcessor::StepOut,
    "@brief Selects only step edges leading outside\n"
  ) +
  gsi::enum_const ("NotStepOut", db::PolygonToEdgeProcessor::NotStepOut,
    "@brief Selects only edges which are not steps leading outside\n"
  ),
  "@brief This class represents the edge mode type for \\Region#edges.\n"
  "\n"
  "This enum has been introduced in version 0.29."
);

//  Inject the Region::EdgeMode declarations into Region:
gsi::ClassExt<db::Region> inject_EdgeMode_in_Region (decl_EdgeMode.defs ());

gsi::Enum<db::zero_distance_mode> decl_ZeroDistanceMode ("db", "ZeroDistanceMode",
  gsi::enum_const ("NeverIncludeZeroDistance", db::NeverIncludeZeroDistance,
    "@brief Specifies that check functions should never include edges with zero distance.\n"
    "With this specification, the check functions will ignore edges which are collinear or touch."
  ) +
  gsi::enum_const ("AlwaysIncludeZeroDistance", db::AlwaysIncludeZeroDistance,
    "@hide\n"
    "@brief Specifies that check functions should always include edges with zero distance\n"
    "This mode has little practical value.\n"
  ) +
  gsi::enum_const ("IncludeZeroDistanceWhenTouching", db::IncludeZeroDistanceWhenTouching,
    "@brief Specifies that check functions should include edges when they touch\n"
    "With this specification, the check functions will also check edges if they share at least one common point. "
    "This is the mode that includes checking the 'kissing corner' cases. This mode is default for version 0.28.16 and later. "
  ) +
  gsi::enum_const ("IncludeZeroDistanceWhenCollinearAndTouching", db::IncludeZeroDistanceWhenCollinearAndTouching,
    "@brief Specifies that check functions should include edges when they are collinear and touch\n"
    "With this specification, the check functions will also check edges if they share at least one common point and are collinear. "
    "This is the mode that includes checking the 'kissing corner' cases when the kissing edges are collinear. This mode was default up to version 0.28. "
  ) +
  gsi::enum_const ("IncludeZeroDistanceWhenOverlapping", db::IncludeZeroDistanceWhenOverlapping,
    "@brief Specifies that check functions should include edges when they overlap\n"
    "With this specification, the check functions will also check edges which are collinear and share more than a single point. "
    "This is the mode that excludes the 'kissing corner' cases."
  ),
  "@brief This class represents the zero_distance_mode type for \\Region#width and related checks.\n"
  "This mode determines how edges with zero distance are treated in the DRC checks. Formally these edges do neither represent "
  "a space other other relation as they do not face each other. There are three modes available to treat this boundary case: "
  "Ignore such edges (\\NeverIncludeZeroDistance) or only include them "
  "if they share at least one common point (\\IncludeZeroDistanceWhenTouching). The latter mode allows activating checks for "
  "the 'kissing corner' case and is the default mode in most checks."
  "\n"
  "This enum has been introduced in version 0.28.16."
);

//  Inject the Region::ZeroDistanceMode declarations into Region and Edges:
//  (Edges injection has to be done here because only here defs() is available)
gsi::ClassExt<db::Region> inject_ZeroDistanceMode_in_Region (decl_ZeroDistanceMode.defs ());
gsi::ClassExt<db::Edges> inject_ZeroDistanceMode_in_Edges (decl_ZeroDistanceMode.defs ());

gsi::Enum<db::PropertyConstraint> decl_PropertyConstraint ("db", "PropertyConstraint",
  gsi::enum_const ("IgnoreProperties", db::IgnoreProperties,
    "@brief Specifies to ignore properties\n"
    "When using this constraint - for example on a boolean operation - properties are ignored and are not generated in the output."
  ) +
  gsi::enum_const ("NoPropertyConstraint", db::NoPropertyConstraint,
    "@brief Specifies not to apply any property constraint\n"
    "When using this constraint - for example on a boolean operation - shapes are considered "
    "regardless of their user properties. Properties are generated on the output shapes where applicable."
  ) +
  gsi::enum_const ("SamePropertiesConstraint", db::SamePropertiesConstraint,
    "@brief Specifies to consider shapes only if their user properties are the same\n"
    "When using this constraint - for example on a boolean operation - shapes are considered "
    "only if their user properties are the same. Properties are generated on the output shapes where applicable."
  ) +
  gsi::enum_const ("SamePropertiesConstraintDrop", db::SamePropertiesConstraintDrop,
    "@brief Specifies to consider shapes only if their user properties are the same\n"
    "When using this constraint - for example on a boolean operation - shapes are considered "
    "only if their user properties are the same. No properties are generated on the output shapes."
  ) +
  gsi::enum_const ("DifferentPropertiesConstraint", db::DifferentPropertiesConstraint,
    "@brief Specifies to consider shapes only if their user properties are different\n"
    "When using this constraint - for example on a boolean operation - shapes are considered "
    "only if their user properties are different. Properties are generated on the output shapes where applicable."
  ) +
  gsi::enum_const ("DifferentPropertiesConstraintDrop", db::DifferentPropertiesConstraintDrop,
    "@brief Specifies to consider shapes only if their user properties are different\n"
    "When using this constraint - for example on a boolean operation - shapes are considered "
    "only if their user properties are the same. No properties are generated on the output shapes."
  ),
  "@brief This class represents the property constraint for boolean and check functions.\n"
  "\n"
  "This enum has been introduced in version 0.28.4."
);

//  Inject the Region::PropertyConstraint declarations into Region and Edges:
//  (Edges injection has to be done here because only here defs() is available)
gsi::ClassExt<db::Region> inject_PropertyConstraint_in_Region (decl_PropertyConstraint.defs ());
gsi::ClassExt<db::Edges> inject_PropertyConstraint_in_Edges (decl_PropertyConstraint.defs ());

gsi::EnumIn<db::Region, db::RectFilter> decl_RectFilter ("db", "RectFilter",
  gsi::enum_const ("NoRectFilter", db::RectFilter::NoRectFilter,
    "@brief Specifies no filtering"
  ) +
  gsi::enum_const ("OneSideAllowed", db::RectFilter::OneSideAllowed,
    "@brief Allow errors on one side"
  ) +
  gsi::enum_const ("TwoSidesAllowed", db::RectFilter::TwoSidesAllowed,
    "@brief Allow errors on two sides (not specified which)"
  ) +
  gsi::enum_const ("TwoConnectedSidesAllowed", db::RectFilter::TwoConnectedSidesAllowed,
    "@brief Allow errors on two sides (\"L\" configuration)"
  ) +
  gsi::enum_const ("TwoOppositeSidesAllowed", db::RectFilter::TwoOppositeSidesAllowed,
    "@brief Allow errors on two opposite sides"
  ) +
  gsi::enum_const ("ThreeSidesAllowed", db::RectFilter::ThreeSidesAllowed,
    "@brief Allow errors when on three sides"
  ) +
  gsi::enum_const ("FourSidesAllowed", db::RectFilter::FourSidesAllowed,
    "@brief Allow errors when on all sides"
  ),
  "@brief This class represents the error filter mode on rectangles for \\Region#separation and related checks.\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

//  Inject the Region::RectFilter declarations into Region:
gsi::ClassExt<db::Region> inject_RectFilter_in_Region (decl_RectFilter.defs ());

gsi::EnumIn<db::Region, db::OppositeFilter> decl_OppositeFilter ("db", "OppositeFilter",
  gsi::enum_const ("NoOppositeFilter", db::OppositeFilter::NoOppositeFilter,
    "@brief No opposite filtering\n"
  ) +
  gsi::enum_const ("OnlyOpposite", db::OppositeFilter::OnlyOpposite,
    "@brief Only errors appearing on opposite sides of a figure will be reported\n"
  ) +
  gsi::enum_const ("NotOpposite", db::OppositeFilter::NotOpposite,
    "@brief Only errors NOT appearing on opposite sides of a figure will be reported\n"
  ),
  "@brief This class represents the opposite error filter mode for \\Region#separation and related checks.\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

//  Inject the Region::OppositeFilter declarations into Region:
gsi::ClassExt<db::Region> inject_OppositeFilter_in_Region (decl_OppositeFilter.defs ());

}
