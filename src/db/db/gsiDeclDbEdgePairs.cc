
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


#include "gsiDecl.h"
#include "gsiEnums.h"

#include "dbEdgePairs.h"
#include "dbEdges.h"
#include "dbRegion.h"
#include "dbDeepEdgePairs.h"
#include "dbEdgesUtils.h"
#include "dbEdgePairFilters.h"
#include "dbPropertiesFilter.h"
#include "dbRegionProcessors.h"

#include "gsiDeclDbContainerHelpers.h"
#include "gsiDeclDbMeasureHelpers.h"

namespace gsi
{

// ---------------------------------------------------------------------------------
//  EdgePairFilter binding

typedef shape_filter_impl<db::EdgePairFilterBase> EdgePairFilterBase;

class EdgePairFilterImpl
  : public EdgePairFilterBase
{
public:
  EdgePairFilterImpl () { }

  bool issue_selected (const db::EdgePairWithProperties &) const
  {
    return false;
  }

  virtual bool selected (const db::EdgePair &edge_pair, db::properties_id_type prop_id) const
  {
    if (f_selected.can_issue ()) {
      return f_selected.issue<EdgePairFilterImpl, bool, const db::EdgePairWithProperties &> (&EdgePairFilterImpl::issue_selected, db::EdgePairWithProperties (edge_pair, prop_id));
    } else {
      return issue_selected (db::EdgePairWithProperties (edge_pair, prop_id));
    }
  }

  gsi::Callback f_selected;

private:
  //  No copying
  EdgePairFilterImpl &operator= (const EdgePairFilterImpl &);
  EdgePairFilterImpl (const EdgePairFilterImpl &);
};

typedef db::generic_properties_filter<gsi::EdgePairFilterBase, db::EdgePair> EdgePairPropertiesFilter;

static gsi::EdgePairFilterBase *make_ppf1 (const tl::Variant &name, const tl::Variant &value, bool inverse)
{
  return new EdgePairPropertiesFilter (name, value, inverse);
}

static gsi::EdgePairFilterBase *make_ppf2 (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
{
  return new EdgePairPropertiesFilter (name, from, to, inverse);
}

static gsi::EdgePairFilterBase *make_pg (const tl::Variant &name, const std::string &glob, bool inverse, bool case_sensitive)
{
  tl::GlobPattern pattern (glob);
  pattern.set_case_sensitive (case_sensitive);
  return new EdgePairPropertiesFilter (name, pattern, inverse);
}

static gsi::EdgePairFilterBase *make_pe (const std::string &expression, bool inverse, const std::map<std::string, tl::Variant> &variables, double dbu)
{
  return new gsi::expression_filter<gsi::EdgePairFilterBase, db::EdgePairs> (expression, inverse, dbu, variables);
}

Class<gsi::EdgePairFilterBase> decl_EdgePairFilterBase ("db", "EdgePairFilterBase",
  gsi::EdgePairFilterBase::method_decls (true) +
  gsi::constructor ("property_glob", &make_pg, gsi::arg ("name"), gsi::arg ("pattern"), gsi::arg ("inverse", false), gsi::arg ("case_sensitive", true),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The glob pattern to match the property value against.\n"
    "@param inverse If true, inverts the selection - i.e. all edge pairs without a matching property are selected.\n"
    "@param case_sensitive If true, the match is case sensitive (the default), if false, the match is not case sensitive.\n"
    "\n"
    "Apply this filter with \\EdgePairs#filtered:\n"
    "\n"
    "@code\n"
    "# edge_pairs is a EdgePairs object\n"
    "# filtered_edge_pairs contains all edge pairs where the 'net' property starts with 'C':\n"
    "filtered_edge_pairs = edge_pairs.filtered(RBA::EdgePairFilterBase::property_glob('net', 'C*'))\n"
    "@/code\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter", &make_ppf1, gsi::arg ("name"), gsi::arg ("value"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The value against which the property is checked (exact match).\n"
    "@param inverse If true, inverts the selection - i.e. all edge pairs without a property with the given name and value are selected.\n"
    "\n"
    "Apply this filter with \\EdgePairs#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter_bounded", &make_ppf2, gsi::arg ("name"), gsi::arg ("from"), gsi::arg ("to"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param from The lower value against which the property is checked or 'nil' if no lower bound shall be used.\n"
    "@param to The upper value against which the property is checked or 'nil' if no upper bound shall be used.\n"
    "@param inverse If true, inverts the selection - i.e. all edge pairs without a property with the given name and value range are selected.\n"
    "\n"
    "This version does a bounded match. The value of the propery needs to be larger or equal to 'from' and less than 'to'.\n"
    "Apply this filter with \\EdgePairs#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("expression_filter", &make_pe, gsi::arg ("expression"), gsi::arg ("inverse", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates an expression-based filter\n"
    "@param expression The expression to evaluate.\n"
    "@param inverse If true, inverts the selection - i.e. all edge pairs without a property with the given name and value range are selected.\n"
    "@param dbu If given and greater than zero, the shapes delivered by the 'shape' function will be in micrometer units.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
    "\n"
    "Creates a filter that will evaluate the given expression on every shape and select the shape "
    "when the expression renders a boolean true value. "
    "The expression may use the following variables and functions:\n"
    "\n"
    "@ul\n"
    "@li @b shape @/b: The current shape (i.e. 'EdgePair' without DBU specified or 'DEdgePair' otherwise) @/li\n"
    "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
    "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
    "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
    "@/ul\n"
    "\n"
    "This feature has been introduced in version 0.30.3."
  ),
  "@hide"
);

Class<gsi::EdgePairFilterImpl> decl_EdgePairFilterImpl (decl_EdgePairFilterBase, "db", "EdgePairFilter",
  callback ("selected", &EdgePairFilterImpl::issue_selected, &EdgePairFilterImpl::f_selected, gsi::arg ("text"),
    "@brief Selects an edge pair\n"
    "This method is the actual payload. It needs to be reimplemented in a derived class.\n"
    "It needs to analyze the edge pair and return 'true' if it should be kept and 'false' if it should be discarded."
    "\n"
    "Since version 0.30, the edge pair carries properties."
  ),
  "@brief A generic edge pair filter adaptor\n"
  "\n"
  "EdgePair filters are an efficient way to filter edge pairs from a EdgePairs collection. To apply a filter, derive your own "
  "filter class and pass an instance to \\EdgePairs#filter or \\EdgePairs#filtered method.\n"
  "\n"
  "Conceptually, these methods take each edge pair from the collection and present it to the filter's 'selected' method.\n"
  "Based on the result of this evaluation, the edge pair is kept or discarded.\n"
  "\n"
  "The magic happens when deep mode edge pair collections are involved. In that case, the filter will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the filter behaves. You "
  "need to configure the filter by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using the filter.\n"
  "\n"
  "You can skip this step, but the filter algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that filters edge pairs where the edges are perpendicular:"
  "\n"
  "@code\n"
  "class PerpendicularEdgesFilter < RBA::EdgePairFilter\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter\n"
  "  end\n"
  "  \n"
  "  # Select edge pairs where the edges are perpendicular\n"
  "  def selected(edge_pair)\n"
  "    return edge_pair.first.d.sprod_sign(edge_pair.second.d) == 0\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "edge_pairs = ... # some EdgePairs object\n"
  "perpendicular_only = edge_pairs.filtered(PerpendicularEdgesFilter::new)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  EdgePairProcessor binding

Class<db::EdgePairProcessorBase> decl_EdgePairProcessorBase ("db", "EdgePairProcessorBase", "@hide");

Class<shape_processor_impl<db::EdgePairProcessorBase> > decl_EdgePairProcessor (decl_EdgePairProcessorBase, "db", "EdgePairOperator",
  shape_processor_impl<db::EdgePairProcessorBase>::method_decls (false),
  "@brief A generic edge-pair operator\n"
  "\n"
  "Edge pair processors are an efficient way to process edge pairs from an edge pair collection. To apply a processor, derive your own "
  "operator class and pass an instance to the \\EdgePairs#processed or \\EdgePairs#process method.\n"
  "\n"
  "Conceptually, these methods take each edge pair from the edge pair collection and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output edge pairs derived from the input edge pair.\n"
  "The output edge pair collection is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode edge pair collections are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that flips the edge pairs (swaps first and second edge):"
  "\n"
  "@code\n"
  "class FlipEdgePairs < RBA::EdgePairOperator\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter\n"
  "  end\n"
  "  \n"
  "  # Flips the edge pair\n"
  "  def process(edge_pair)\n"
  "    return [ RBA::EdgePair::new(edge_pair.second, edge_pair.first) ]\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "edge_pairs = ... # some EdgePairs object\n"
  "flipped = edge_pairs.processed(FlipEdgePairs::new)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

static
property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs> *
new_pcp (const db::EdgePairs *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  return new property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs> (container, expressions, copy_properties, dbu, variables);
}

static
property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs> *
new_pcps (const db::EdgePairs *container, const std::string &expression, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  std::map<tl::Variant, std::string> expressions;
  expressions.insert (std::make_pair (tl::Variant (), expression));
  return new property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs> (container, expressions, copy_properties, dbu, variables);
}

Class<property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs> > decl_EdgePairPropertiesExpressions (decl_EdgePairProcessorBase, "db", "EdgePairPropertiesExpressions",
  property_computation_processor<db::EdgePairProcessorBase, db::EdgePairs>::method_decls (true) +
  gsi::constructor ("new", &new_pcp, gsi::arg ("edge_pairs"), gsi::arg ("expressions"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param edge_pairs The edge pair collection, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
    "@param expressions A map of property names and expressions used to generate the values of the properties (see class description for details).\n"
    "@param copy_properties If true, new properties will be added to existing ones.\n"
    "@param dbu If not zero, this value specifies the database unit to use. If given, the shapes returned by the 'shape' function will be micrometer-unit objects.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
  ) +
  gsi::constructor ("new", &new_pcps, gsi::arg ("edge_pairs"), gsi::arg ("expression"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param edge_pairs The edge pair collection, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
    "@param expression A single expression evaluated for each shape (see class description for details).\n"
    "@param copy_properties If true, new properties will be added to existing ones.\n"
    "@param dbu If not zero, this value specifies the database unit to use. If given, the shapes returned by the 'shape' function will be micrometer-unit objects.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
  ),
  "@brief An operator attaching computed properties to the edge pairs\n"
  "\n"
  "This operator will execute a number of expressions and attach the results as new properties. "
  "The expression inputs can be taken either from the edge pairs themselves or from existing properties.\n"
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
  "@li @b shape @/b: The current shape (i.e. 'EdgePair' without DBU specified or 'DEdgePair' otherwise) @/li\n"
  "@li @b put(<name>, <value>) @/b: Attaches the given value as a property with name 'name' to the output shape @/li\n"
  "@li @b skip(<flag>) @/b: If called with a 'true' value, the shape is dropped from the output @/li\n"
  "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
  "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
  "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
  "@/ul\n"
  "\n"
  "This class has been introduced in version 0.30.3.\n"
);

Class<db::EdgePairToPolygonProcessorBase> decl_EdgePairToPolygonProcessorBase ("db", "EdgePairToPolygonProcessorBase", "@hide");

Class<shape_processor_impl<db::EdgePairToPolygonProcessorBase> > decl_EdgePairToPolygonProcessor (decl_EdgePairToPolygonProcessorBase, "db", "EdgePairToPolygonOperator",
  shape_processor_impl<db::EdgePairToPolygonProcessorBase>::method_decls (false),
  "@brief A generic edge-pair-to-polygon operator\n"
  "\n"
  "Edge pair processors are an efficient way to process edge pairs from an edge pair collection. To apply a processor, derive your own "
  "operator class and pass an instance to the \\EdgePairs#processed method.\n"
  "\n"
  "Conceptually, these methods take each edge pair from the edge pair collection and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output polygons derived from the input edge pair.\n"
  "The output region is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode edge pair collections are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "For a basic example see the \\EdgeToPolygonOperator class, with the exception that this incarnation receives edge pairs.\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

Class<db::EdgePairToEdgeProcessorBase> decl_EdgePairToEdgeProcessorBase ("db", "EdgePairToEdgeProcessorBase", "@hide");

Class<shape_processor_impl<db::EdgePairToEdgeProcessorBase> > decl_EdgePairToEdgeProcessor (decl_EdgePairToEdgeProcessorBase, "db", "EdgePairToEdgeOperator",
  shape_processor_impl<db::EdgePairToEdgeProcessorBase>::method_decls (false),
  "@brief A generic edge-pair-to-edge operator\n"
  "\n"
  "Edge processors are an efficient way to process edge pairs from an edge pair collection. To apply a processor, derive your own "
  "operator class and pass an instance to \\EdgePairs#processed method.\n"
  "\n"
  "Conceptually, these methods take each edge from the edge collection and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output edges derived from the input edge pair.\n"
  "The output edge pair collection is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode edge pair collections are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "For a basic example see the \\EdgeToEdgePairOperator class, with the exception that this incarnation has to deliver edges and takes edge pairs.\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  EdgePairs binding

static inline std::vector<db::EdgePairs> as_2edge_pairs_vector (const std::pair<db::EdgePairs, db::EdgePairs> &rp)
{
  std::vector<db::EdgePairs> res;
  res.reserve (2);
  res.push_back (db::EdgePairs (const_cast<db::EdgePairs &> (rp.first).take_delegate ()));
  res.push_back (db::EdgePairs (const_cast<db::EdgePairs &> (rp.second).take_delegate ()));
  return res;
}

static db::EdgePairs *new_v ()
{
  return new db::EdgePairs ();
}

static db::EdgePairs *new_a (const std::vector<db::EdgePair> &pairs)
{
  return new db::EdgePairs (pairs.begin (), pairs.end ());
}

static db::EdgePairs *new_ap (const std::vector<db::EdgePairWithProperties> &pairs, bool)
{
  return new db::EdgePairs (pairs.begin (), pairs.end ());
}

static db::EdgePairs *new_ep (const db::EdgePair &pair)
{
  return new db::EdgePairs (pair);
}

static db::EdgePairs *new_epp (const db::EdgePairWithProperties &pair)
{
  return new db::EdgePairs (pair);
}

static db::EdgePairs *new_shapes (const db::Shapes &s)
{
  db::EdgePairs *r = new db::EdgePairs ();
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::EdgePairs); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::EdgePairs *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::EdgePairs (si);
}

static db::EdgePairs *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::EdgePairs (si, trans);
}

static db::EdgePairs *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss)
{
  return new db::EdgePairs (si, dss);
}

static db::EdgePairs *new_si2d (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans)
{
  return new db::EdgePairs (si, dss, trans);
}

static std::string to_string0 (const db::EdgePairs *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::EdgePairs *r, size_t n)
{
  return r->to_string (n);
}

static db::EdgePairs &move_p (db::EdgePairs *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::EdgePairs &move_xy (db::EdgePairs *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::EdgePairs moved_p (const db::EdgePairs *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::EdgePairs moved_xy (const db::EdgePairs *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region pull_interacting_polygons (const db::EdgePairs *r, const db::Region &other)
{
  db::Region out;
  r->pull_interacting (out, other);
  return out;
}

static db::Edges pull_interacting_edges (const db::EdgePairs *r, const db::Edges &other)
{
  db::Edges out;
  r->pull_interacting (out, other);
  return out;
}

static std::vector<db::EdgePairs> split_inside_with_region (const db::EdgePairs *r, const db::Region &other)
{
  return as_2edge_pairs_vector (r->selected_inside_differential (other));
}

static std::vector<db::EdgePairs> split_outside_with_region (const db::EdgePairs *r, const db::Region &other)
{
  return as_2edge_pairs_vector (r->selected_outside_differential (other));
}

static std::vector<db::EdgePairs> split_interacting_with_edges (const db::EdgePairs *r, const db::Edges &other, size_t min_count, size_t max_count)
{
  return as_2edge_pairs_vector (r->selected_interacting_differential (other, min_count, max_count));
}

static std::vector<db::EdgePairs> split_interacting_with_region (const db::EdgePairs *r, const db::Region &other, size_t min_count, size_t max_count)
{
  return as_2edge_pairs_vector (r->selected_interacting_differential (other, min_count, max_count));
}

static db::Region polygons1 (const db::EdgePairs *e)
{
  db::Region r;
  e->polygons (r);
  return r;
}

static db::Region polygons2 (const db::EdgePairs *e, db::Coord d)
{
  db::Region r;
  e->polygons (r, d);
  return r;
}

static db::Region extents2 (const db::EdgePairs *r, db::Coord dx, db::Coord dy)
{
  db::Region output;
  r->processed (output, db::extents_processor<db::EdgePair> (dx, dy));
  return output;
}

static db::Region extents1 (const db::EdgePairs *r, db::Coord d)
{
  return extents2 (r, d, d);
}

static db::Region extents0 (const db::EdgePairs *r)
{
  return extents2 (r, 0, 0);
}

namespace {

//  a combined processor that implements db::RelativeExtents on the edge bounding boxes

class EdgePairsRelativeExtents
  : virtual public db::EdgePairToPolygonProcessorBase,
    virtual public db::RelativeExtents
{
public:
  EdgePairsRelativeExtents (double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
    : db::RelativeExtents (fx1, fy1, fx2, fy2, dx, dy)
  {
    //  .. nothing yet ..
  }

  //  not needed, but mutes
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const
  {
    db::RelativeExtents::process (poly, result);
  }

  void process (const db::EdgePairWithProperties &ep, std::vector<db::PolygonWithProperties> &result) const
  {
    db::RelativeExtents::process (db::Polygon (ep.bbox ()), result);
  }
};

class EdgePairsRelativeExtentsAsEdges
  : virtual public db::EdgePairToEdgeProcessorBase,
    virtual public db::RelativeExtentsAsEdges
{
public:
  EdgePairsRelativeExtentsAsEdges (double fx1, double fy1, double fx2, double fy2)
    : db::RelativeExtentsAsEdges (fx1, fy1, fx2, fy2)
  {
    //  .. nothing yet ..
  }

  void process (const db::PolygonWithProperties &poly, std::vector<db::EdgeWithProperties> &result) const
  {
    db::RelativeExtentsAsEdges::process (poly, result);
  }

  void process (const db::EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &result) const
  {
    db::RelativeExtentsAsEdges::process (db::Polygon (ep.bbox ()), result);
  }
};

}

static db::Region extent_refs (const db::EdgePairs *r, double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
{
  db::Region result;
  r->processed (result, EdgePairsRelativeExtents (fx1, fy1, fx2, fy2, dx, dy));
  return result;
}

static db::Edges extent_refs_edges (const db::EdgePairs *r, double fx1, double fy1, double fx2, double fy2)
{
  db::Edges result;
  r->processed (result, EdgePairsRelativeExtentsAsEdges (fx1, fy1, fx2, fy2));
  return result;
}

static db::Edges edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->edges (e);
  return e;
}

static db::Edges first_edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->first_edges (e);
  return e;
}

static db::Edges second_edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->second_edges (e);
  return e;
}

static void insert_e (db::EdgePairs *e, const db::EdgePairs &a)
{
  for (db::EdgePairs::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    e->insert (*p);
  }
}

static bool is_deep (const db::EdgePairs *ep)
{
  return dynamic_cast<const db::DeepEdgePairs *> (ep->delegate ()) != 0;
}

static size_t id (const db::EdgePairs *ep)
{
  return tl::id_of (ep->delegate ());
}

static db::EdgePairs filtered (const db::EdgePairs *r, const gsi::EdgePairFilterBase *f)
{
  return r->filtered (*f);
}

static void filter (db::EdgePairs *r, const gsi::EdgePairFilterBase *f)
{
  r->filter (*f);
}

static std::vector<db::EdgePairs> split_filter (const db::EdgePairs *r, const EdgePairFilterImpl *f)
{
  return as_2edge_pairs_vector (r->split_filter (*f));
}

static db::EdgePairs processed_epep (const db::EdgePairs *r, const db::EdgePairProcessorBase *f)
{
  return r->processed (*f);
}

static void process_epep (db::EdgePairs *r, const db::EdgePairProcessorBase *f)
{
  r->process (*f);
}

static db::Edges processed_epe (const db::EdgePairs *r, const db::EdgePairToEdgeProcessorBase *f)
{
  db::Edges out;
  r->processed (out, *f);
  return out;
}

static db::Region processed_epp (const db::EdgePairs *r, const db::EdgePairToPolygonProcessorBase *f)
{
  db::Region out;
  r->processed (out, *f);
  return out;
}

static db::EdgePairs with_distance1 (const db::EdgePairs *r, db::EdgePairs::distance_type length, bool inverse)
{
  db::EdgePairFilterByDistance ef (length, length + 1, inverse);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_distance1 (const db::EdgePairs *r, db::EdgePairs::distance_type length)
{
  db::EdgePairFilterByDistance ef (length, length + 1, false);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_distance2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgePairFilterByDistance ef (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_distance2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max)
{
  db::EdgePairFilterByDistance ef (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), false);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_length1 (const db::EdgePairs *r, db::EdgePairs::distance_type length, bool inverse)
{
  db::EdgeLengthFilter f (length, length + 1, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_length1 (const db::EdgePairs *r, db::EdgePairs::distance_type length, bool inverse)
{
  db::EdgeLengthFilter f (length, length + 1, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_length2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgeLengthFilter f (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_length2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgeLengthFilter f (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_length_both1 (const db::EdgePairs *r, db::EdgePairs::distance_type length, bool inverse)
{
  db::EdgeLengthFilter f (length, length + 1, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_length_both1 (const db::EdgePairs *r, db::EdgePairs::distance_type length, bool inverse)
{
  db::EdgeLengthFilter f (length, length + 1, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_length_both2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgeLengthFilter f (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_length_both2 (const db::EdgePairs *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgeLengthFilter f (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_abs_angle1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_abs_angle1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_abs_angle2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_abs_angle2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle3 (const db::EdgePairs *r, db::SpecialEdgeOrientationFilter::FilterType type, bool inverse)
{
  db::SpecialEdgeOrientationFilter f (type, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle3 (const db::EdgePairs *r, db::SpecialEdgeOrientationFilter::FilterType type, bool inverse)
{
  db::SpecialEdgeOrientationFilter f (type, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, true /*one must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle_both1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle_both1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle_both2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle_both2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, false);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_abs_angle_both1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_abs_angle_both1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_abs_angle_both2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_abs_angle_both2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse, true);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_angle_both3 (const db::EdgePairs *r, db::SpecialEdgeOrientationFilter::FilterType type, bool inverse)
{
  db::SpecialEdgeOrientationFilter f (type, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return r->filtered (ef);
}

static std::vector<db::EdgePairs> split_with_angle_both3 (const db::EdgePairs *r, db::SpecialEdgeOrientationFilter::FilterType type, bool inverse)
{
  db::SpecialEdgeOrientationFilter f (type, inverse);
  db::EdgeFilterBasedEdgePairFilter ef (&f, false /*both must match*/);
  return as_2edge_pairs_vector (r->split_filter (ef));
}

static db::EdgePairs with_internal_angle1 (const db::EdgePairs *r, double a, bool inverse)
{
  db::InternalAngleEdgePairFilter f (a, inverse);
  return r->filtered (f);
}

static std::vector<db::EdgePairs> split_with_internal_angle1 (const db::EdgePairs *r, double a)
{
  db::InternalAngleEdgePairFilter f (a, false);
  return as_2edge_pairs_vector (r->split_filter (f));
}

static db::EdgePairs with_internal_angle2 (const db::EdgePairs *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::InternalAngleEdgePairFilter f (amin, include_amin, amax, include_amax, inverse);
  return r->filtered (f);
}

static std::vector<db::EdgePairs> split_with_internal_angle2 (const db::EdgePairs *r, double amin, double amax, bool include_amin, bool include_amax)
{
  db::InternalAngleEdgePairFilter f (amin, include_amin, amax, include_amax, false);
  return as_2edge_pairs_vector (r->split_filter (f));
}

static db::EdgePairs with_area1 (const db::EdgePairs *r, db::EdgePair::area_type a, bool inverse)
{
  db::EdgePairFilterByArea f (a, a + 1, inverse);
  return r->filtered (f);
}

static std::vector<db::EdgePairs> split_with_area1 (const db::EdgePairs *r, db::EdgePair::area_type a)
{
  db::EdgePairFilterByArea f (a, a + 1, false);
  return as_2edge_pairs_vector (r->split_filter (f));
}

static db::EdgePairs with_area2 (const db::EdgePairs *r, db::EdgePair::area_type amin, db::EdgePair::area_type amax, bool inverse)
{
  db::EdgePairFilterByArea f (amin, amax, inverse);
  return r->filtered (f);
}

static std::vector<db::EdgePairs> split_with_area2 (const db::EdgePairs *r, db::EdgePair::area_type amin, db::EdgePair::area_type amax)
{
  db::EdgePairFilterByArea f (amin, amax, false);
  return as_2edge_pairs_vector (r->split_filter (f));
}

static tl::Variant nth (const db::EdgePairs *edge_pairs, size_t n)
{
  const db::EdgePair *ep = edge_pairs->nth (n);
  if (! ep) {
    return tl::Variant ();
  } else {
    return tl::Variant (db::EdgePairWithProperties (*ep, edge_pairs->nth_prop_id (n)));
  }
}

static db::generic_shape_iterator<db::EdgePairWithProperties> begin_edge_pairs (const db::EdgePairs *edge_pairs)
{
  return db::generic_shape_iterator<db::EdgePairWithProperties> (db::make_wp_iter (edge_pairs->delegate ()->begin ()));
}

extern Class<db::ShapeCollection> decl_dbShapeCollection;

Class<db::EdgePairs> decl_EdgePairs (decl_dbShapeCollection, "db", "EdgePairs",
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty edge pair collection.\n"
  ) + 
  constructor ("new", &new_a, gsi::arg ("array"),
    "@brief Constructor from an edge pair array\n"
    "\n"
    "This constructor creates an edge pair collection from an array of \\EdgePair objects.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  //  This is a dummy constructor that allows creating a EdgePairs collection from an array
  //  of EdgePairWithProperties objects too. GSI needs the dummy argument to
  //  differentiate between the cases when an empty array is passed.
  constructor ("new", &new_ap, gsi::arg ("array"), gsi::arg ("dummy", true),
    "@hide"
  ) +
  constructor ("new", &new_ep, gsi::arg ("edge_pair"),
    "@brief Constructor from a single edge pair object\n"
    "\n"
    "This constructor creates an edge pair collection with a single edge pair.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_epp, gsi::arg ("edge_pair"),
    "@brief Constructor from a single edge pair object with properties\n"
    "\n"
    "This constructor creates an edge pair collection with a single edge pair.\n"
    "\n"
    "This constructor has been introduced in version 0.30."
  ) +
  constructor ("new", &new_shapes, gsi::arg ("shapes"),
    "@brief Shapes constructor\n"
    "\n"
    "This constructor creates an edge pair collection from a \\Shapes collection.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si, gsi::arg ("shape_iterator"),
    "@brief Constructor from a hierarchical shape set\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only edge pairs are taken from the shape set and other shapes are ignored.\n"
    "This method allows feeding the edge pair collection from a hierarchy of cells.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only edge pairs are taken from the shape set and other shapes are ignored.\n"
    "The given transformation is applied to each edge pair taken.\n"
    "This method allows feeding the edge pair collection from a hierarchy of cells.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("dss"),
    "@brief Creates a hierarchical edge pair collection from an original layer\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical edge pair collection which supports hierarchical operations.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si2d, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("trans"),
    "@brief Creates a hierarchical edge pair collection from an original layer with a transformation\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical edge pair collection which supports hierarchical operations.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  method ("write", &db::EdgePairs::write, gsi::arg ("filename"),
    "@brief Writes the region to a file\n"
    "This method is provided for debugging purposes. It writes the object to a flat layer 0/0 in a single top cell.\n"
    "\n"
    "This method has been introduced in version 0.29."
  ) +
  method ("insert_into", &db::EdgePairs::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this edge pairs into the given layout, below the given cell and into the given layer.\n"
    "If the edge pair collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("insert_into_as_polygons", &db::EdgePairs::insert_into_as_polygons, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("e"),
    "@brief Inserts this edge pairs into the given layout, below the given cell and into the given layer.\n"
    "If the edge pair collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "The edge pairs will be converted to polygons with the enlargement value given be 'e'.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("insert", (void (db::EdgePairs::*) (const db::Edge &, const db::Edge &)) &db::EdgePairs::insert, gsi::arg ("first"), gsi::arg ("second"),
    "@brief Inserts an edge pair into the collection\n"
  ) +
  method ("insert", (void (db::EdgePairs::*) (const db::EdgePair &)) &db::EdgePairs::insert, gsi::arg ("edge_pair"),
    "@brief Inserts an edge pair into the collection\n"
  ) +
  method ("insert", (void (db::EdgePairs::*) (const db::EdgePairWithProperties &)) &db::EdgePairs::insert, gsi::arg ("edge_pair"),
    "@brief Inserts an edge pair with properties into the collection\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the edge pair collection is a deep (hierarchical) one\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method ("+|join", &db::EdgePairs::operator+, gsi::arg ("other"),
    "@brief Returns the combined edge pair collection of self and the other one\n"
    "\n"
    "@return The resulting edge pair collection\n"
    "\n"
    "This operator adds the edge pairs of the other collection to self and returns a new combined set.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
    "The 'join' alias has been introduced in version 0.28.12."
  ) +
  method ("+=|join_with", &db::EdgePairs::operator+=, gsi::arg ("other"),
    "@brief Adds the edge pairs of the other edge pair collection to self\n"
    "\n"
    "@return The edge pair collection after modification (self)\n"
    "\n"
    "This operator adds the edge pairs of the other collection to self.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
    "\n"
    "Note that in Ruby, the '+=' operator actually does not exist, but is emulated by '+' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'join_with' instead.\n"
    "\n"
    "The 'join_with' alias has been introduced in version 0.28.12."
  ) +
  method_ext ("move", &move_p, gsi::arg ("v"),
    "@brief Moves the edge pair collection\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pair collection. The edge pair collection is overwritten.\n"
    "\n"
    "@param v The distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs (self).\n"
    "\n"
    "Starting with version 0.25 the displacement is of vector type."
  ) +
  method_ext ("move", &move_xy, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Moves the edge pair collection\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is overwritten.\n"
    "\n"
    "@param dx The x distance to move the edge pairs.\n"
    "@param dy The y distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs (self).\n"
  ) +
  method_ext ("moved", &moved_p, gsi::arg ("v"),
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is not modified.\n"
    "\n"
    "@param v The distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs.\n"
    "\n"
    "Starting with version 0.25 the displacement is of vector type."
  ) +
  method_ext ("moved", &moved_xy, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is not modified.\n"
    "\n"
    "@param dx The x distance to move the edge pairs.\n"
    "@param dy The y distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs.\n"
  ) +
  method ("transformed", (db::EdgePairs (db::EdgePairs::*)(const db::Trans &) const) &db::EdgePairs::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection\n"
    "\n"
    "Transforms the edge pairs with the given transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::EdgePairs (db::EdgePairs::*)(const db::ICplxTrans &) const) &db::EdgePairs::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection with a complex transformation\n"
    "\n"
    "Transforms the edge pairs with the given complex transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
  ) +
  method ("transformed", (db::EdgePairs (db::EdgePairs::*)(const db::IMatrix2d &) const) &db::EdgePairs::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection\n"
    "\n"
    "Transforms the edge pairs with the given 2d matrix transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method ("transformed", (db::EdgePairs (db::EdgePairs::*)(const db::IMatrix3d &) const) &db::EdgePairs::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection\n"
    "\n"
    "Transforms the edge pairs with the given 3d matrix transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method ("transform", (db::EdgePairs &(db::EdgePairs::*)(const db::Trans &)) &db::EdgePairs::transform, gsi::arg ("t"),
    "@brief Transform the edge pair collection (modifies self)\n"
    "\n"
    "Transforms the edge pair collection with the given transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
  ) +
  method ("transform|#transform_icplx", (db::EdgePairs &(db::EdgePairs::*)(const db::ICplxTrans &)) &db::EdgePairs::transform, gsi::arg ("t"),
    "@brief Transform the edge pair collection with a complex transformation (modifies self)\n"
    "\n"
    "Transforms the edge pair collection with the given transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
  ) +
  method ("transform", (db::EdgePairs &(db::EdgePairs::*)(const db::IMatrix2d &)) &db::EdgePairs::transform, gsi::arg ("t"),
    "@brief Transform the edge pair collection (modifies self)\n"
    "\n"
    "Transforms the edge pair collection with the given 2d matrix transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method ("transform", (db::EdgePairs &(db::EdgePairs::*)(const db::IMatrix3d &)) &db::EdgePairs::transform, gsi::arg ("t"),
    "@brief Transform the edge pair collection (modifies self)\n"
    "\n"
    "Transforms the edge pair collection with the given 3d matrix transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method_ext ("insert", &insert_e, gsi::arg ("edge_pairs"),
    "@brief Inserts all edge pairs from the other edge pair collection into this edge pair collection\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("edges", &edges,
    "@brief Decomposes the edge pairs into single edges\n"
    "@return An edge collection containing the individual edges\n"
  ) +
  method_ext ("first_edges", &first_edges,
    "@brief Returns the first one of all edges\n"
    "@return An edge collection containing the first edges\n"
  ) +
  method_ext ("second_edges", &second_edges,
    "@brief Returns the second one of all edges\n"
    "@return An edge collection containing the second edges\n"
  ) +
  method_ext ("extents", &extents0,
    "@brief Returns a region with the bounding boxes of the edge pairs\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents1, gsi::arg ("d"),
    "@brief Returns a region with the enlarged bounding boxes of the edge pairs\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs enlarged by the given distance d.\n"
    "The enlargement is specified per edge, i.e the width and height will be increased by 2*d.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents2, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Returns a region with the enlarged bounding boxes of the edge pairs\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs enlarged by the given distance dx in x direction and dy in y direction.\n"
    "The enlargement is specified per edge, i.e the width will be increased by 2*dx.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extent_refs", &extent_refs,
    "@hide\n"
    "This method is provided for DRC implementation.\n"
  ) +
  method_ext ("extent_refs_edges", &extent_refs_edges,
    "@hide\n"
    "This method is provided for DRC implementation.\n"
  ) +
 method_ext ("filter", &filter, gsi::arg ("filter"),
    "@brief Applies a generic filter in place (replacing the edge pairs from the EdgePair collection)\n"
    "See \\EdgePairFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("filtered", &filtered, gsi::arg ("filter"),
    "@brief Applies a generic filter and returns a filtered copy\n"
    "See \\EdgePairFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("split_filter", &split_filter, gsi::arg ("filter"),
    "@brief Applies a generic filter and returns a copy with all matching shapes and one with the non-matching ones\n"
    "See \\EdgePairFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("process", &process_epep, gsi::arg ("process"),
    "@brief Applies a generic edge pair processor in place (replacing the edge pairs from the EdgePairs collection)\n"
    "See \\EdgePairProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_epep, gsi::arg ("processed"),
    "@brief Applies a generic edge pair processor and returns a processed copy\n"
    "See \\EdgePairProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_epe, gsi::arg ("processed"),
    "@brief Applies a generic edge-pair-to-edge processor and returns an edge collection with the results\n"
    "See \\EdgePairToEdgeProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_epp, gsi::arg ("processed"),
    "@brief Applies a generic edge-pair-to-polygon processor and returns an Region with the results\n"
    "See \\EdgePairToPolygonProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("with_length", with_length1, gsi::arg ("length"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by length of one of their edges\n"
    "Filters the edge pairs in the edge pair collection by length of at least one of their edges. If \"inverse\" is false, only "
    "edge pairs with at least one edge having the given length are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_length", with_length2, gsi::arg ("min_length"), gsi::arg ("max_length"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by length of one of their edges\n"
    "Filters the edge pairs in the edge pair collection by length of at least one of their edges. If \"inverse\" is false, only "
    "edge pairs with at least one edge having a length between min_length and max_length (excluding max_length itself) are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("split_with_length", split_with_length1, gsi::arg ("length"), gsi::arg ("inverse"),
    "@brief Like \\with_length, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_length", split_with_length2, gsi::arg ("min_length"), gsi::arg ("max_length"), gsi::arg ("inverse"),
    "@brief Like \\with_length, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_length_both", with_length_both1, gsi::arg ("length"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by length of both of their edges\n"
    "Filters the edge pairs in the edge pair collection by length of both of their edges. If \"inverse\" is false, only "
    "edge pairs where both edges have the given length are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_length_both", with_length_both2, gsi::arg ("min_length"), gsi::arg ("max_length"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by length of both of their edges\n"
    "Filters the edge pairs in the edge pair collection by length of both of their edges. If \"inverse\" is false, only "
    "edge pairs with both edges having a length between min_length and max_length (excluding max_length itself) are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("split_with_length_both", split_with_length_both1, gsi::arg ("length"), gsi::arg ("inverse"),
    "@brief Like \\with_length_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_length_both", split_with_length_both2, gsi::arg ("min_length"), gsi::arg ("max_length"), gsi::arg ("inverse"),
    "@brief Like \\with_length_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_distance", with_distance1, gsi::arg ("distance"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by the distance of the edges\n"
    "Filters the edge pairs in the edge pair collection by distance of the edges. If \"inverse\" is false, only "
    "edge pairs where both edges have the given distance are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "Distance is measured as the shortest distance between any of the points on the edges.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_distance", with_distance2, gsi::arg ("min_distance"), gsi::arg ("max_distance"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by the distance of the edges\n"
    "Filters the edge pairs in the edge pair collection by distance of the edges. If \"inverse\" is false, only "
    "edge pairs where both edges have a distance between min_distance and max_distance (max_distance itself is excluded) are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "Distance is measured as the shortest distance between any of the points on the edges.\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("split_with_distance", split_with_distance1, gsi::arg ("distance"),
    "@brief Like \\with_distance, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_distance", split_with_distance2, gsi::arg ("min_distance"), gsi::arg ("max_distance"),
    "@brief Like \\with_distance, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_angle", with_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with at least one edge having the given angle to the x-axis are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This will filter edge pairs with at least one horizontal edge:\n"
    "\n"
    "@code\n"
    "horizontal = edge_pairs.with_angle(0, false)\n"
    "@/code\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle is delivered by \\with_angle_both with the inverse flag set as edge pairs are unselected when both edges fail to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle(0, false)\n"
    "others = edge_pairs.with_angle_both(0, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_angle", with_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with at least one edge having an angle between min_angle and max_angle are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "With \"include_min_angle\" set to true (the default), the minimum angle is included in the criterion while with false, the "
    "minimum angle itself is not included. Same for \"include_max_angle\" where the default is false, meaning the maximum angle is not included in the range.\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle is delivered by \\with_angle_both with the inverse flag set as edge pairs are unselected when both edges fail to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle(0, 45, false)\n"
    "others = edge_pairs.with_angle_both(0, 45, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_angle", with_angle3, gsi::arg ("type"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with at least one edge having an angle of the given type are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This version allows specifying an edge type instead of an angle. Edge types include multiple distinct orientations "
    "and are specified using one of the \\Edges#OrthoEdges, \\Edges#DiagonalEdges or \\Edges#OrthoDiagonalEdges types.\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle is delivered by \\with_angle_both with the inverse flag set as edge pairs are unselected when both edges fail to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle(RBA::Edges::Ortho, false)\n"
    "others = edge_pairs.with_angle_both(RBA::Edges::Ortho, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method_ext ("split_with_angle", split_with_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Like \\with_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_angle", split_with_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Like \\with_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_angle", split_with_angle3, gsi::arg ("type"), gsi::arg ("inverse"),
    "@brief Like \\with_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_abs_angle", with_abs_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "\n"
    "This method behaves like \\with_angle, but angles are always positive - i.e. there is no "
    "differentiation between edges sloping 'down' vs. edges sloping 'up.\n"
    "\n"
    "This method has been added in version 0.29.1.\n"
  ) +
  method_ext ("with_abs_angle", with_abs_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "\n"
    "This method behaves like \\with_angle, but angles are always positive - i.e. there is no "
    "differentiation between edges sloping 'down' vs. edges sloping 'up.\n"
    "\n"
    "This method has been added in version 0.29.1.\n"
  ) +
  method_ext ("split_with_abs_angle", split_with_abs_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Like \\with_abs_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_abs_angle", split_with_abs_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Like \\with_abs_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_angle_both", with_angle_both1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of both of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with both edges having the given angle to the x-axis are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This will filter edge pairs with at least one horizontal edge:\n"
    "\n"
    "@code\n"
    "horizontal = edge_pairs.with_angle_both(0, false)\n"
    "@/code\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle_both is delivered by \\with_angle with the inverse flag set as edge pairs are unselected when one edge fails to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle_both(0, false)\n"
    "others = edge_pairs.with_angle(0, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_angle_both", with_angle_both2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Filter the edge pairs by orientation of both of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with both edges having an angle between min_angle and max_angle are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "With \"include_min_angle\" set to true (the default), the minimum angle is included in the criterion while with false, the "
    "minimum angle itself is not included. Same for \"include_max_angle\" where the default is false, meaning the maximum angle is not included in the range.\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle_both is delivered by \\with_angle with the inverse flag set as edge pairs are unselected when one edge fails to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle_both(0, 45, false)\n"
    "others = edge_pairs.with_angle(0, 45, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.27.1.\n"
  ) +
  method_ext ("with_angle_both", with_angle_both3, gsi::arg ("type"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of their edges\n"
    "Filters the edge pairs in the edge pair collection by orientation. If \"inverse\" is false, only "
    "edge pairs with both edges having an angle of the given type are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion for both edges are returned.\n"
    "\n"
    "This version allows specifying an edge type instead of an angle. Edge types include multiple distinct orientations "
    "and are specified using one of the \\Edges#OrthoEdges, \\Edges#DiagonalEdges or \\Edges#OrthoDiagonalEdges types.\n"
    "\n"
    "Note that the inverse @b result @/b of \\with_angle_both is delivered by \\with_angle with the inverse flag set as edge pairs are unselected when one edge fails to meet the criterion.\n"
    "I.e\n"
    "\n"
    "@code\n"
    "result = edge_pairs.with_angle_both(RBA::Edges::Ortho, false)\n"
    "others = edge_pairs.with_angle(RBA::Edges::Ortho, true)\n"
    "@/code\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method_ext ("split_with_angle_both", split_with_angle_both1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Like \\with_angle_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_angle_both", split_with_angle_both2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Like \\with_angle_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_angle_both", split_with_angle_both3, gsi::arg ("type"), gsi::arg ("inverse"),
    "@brief Like \\with_angle_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_abs_angle_both", with_abs_angle_both1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filter the edge pairs by orientation of both of their edges\n"
    "\n"
    "This method behaves like \\with_angle_both, but angles are always positive - i.e. there is no "
    "differentiation between edges sloping 'down' vs. edges sloping 'up.\n"
    "\n"
    "This method has been added in version 0.29.1.\n"
  ) +
  method_ext ("with_abs_angle_both", with_abs_angle_both2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "\n"
    "This method behaves like \\with_angle_both, but angles are always positive - i.e. there is no "
    "differentiation between edges sloping 'down' vs. edges sloping 'up.\n"
    "\n"
    "This method has been added in version 0.29.1.\n"
  ) +
  method_ext ("split_with_abs_angle_both", split_with_abs_angle_both1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Like \\with_abs_angle_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_abs_angle_both", split_with_abs_angle_both2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Like \\with_abs_angle_both, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "Note that 'inverse' controls the way each edge is checked, not overall.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_area", with_area1, gsi::arg ("area"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by the enclosed area\n"
    "Filters the edge pairs in the edge pair collection by enclosed area. If \"inverse\" is false, only "
    "edge pairs with the given area are returned. If \"inverse\" is true, "
    "edge pairs not with the given area are returned.\n"
    "\n"
    "This method has been added in version 0.27.2.\n"
  ) +
  method_ext ("with_area", with_area2, gsi::arg ("min_area"), gsi::arg ("max_area"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by the enclosed area\n"
    "Filters the edge pairs in the edge pair collection by enclosed area. If \"inverse\" is false, only "
    "edge pairs with an area between min_area and max_area (max_area itself is excluded) are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "This method has been added in version 0.27.2.\n"
  ) +
  method_ext ("split_with_area", split_with_area1, gsi::arg ("area"),
    "@brief Like \\with_area, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_area", split_with_area2, gsi::arg ("min_area"), gsi::arg ("max_area"),
    "@brief Like \\with_area, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_internal_angle", with_internal_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filters the edge pairs by the angle between their edges\n"
    "Filters the edge pairs in the edge pair collection by the angle between their edges. If \"inverse\" is false, only "
    "edge pairs with the given angle are returned. If \"inverse\" is true, "
    "edge pairs not with the given angle are returned.\n"
    "\n"
    "The angle is measured between the two edges. It is between 0 (parallel or anti-parallel edges) and 90 degree (perpendicular edges).\n"
    "\n"
    "This method has been added in version 0.27.2.\n"
  ) +
  method_ext ("with_internal_angle", with_internal_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Filters the edge pairs by the angle between their edges\n"
    "Filters the edge pairs in the edge pair collection by the angle between their edges. If \"inverse\" is false, only "
    "edge pairs with an angle between min_angle and max_angle (max_angle itself is excluded) are returned. If \"inverse\" is true, "
    "edge pairs not fulfilling this criterion are returned.\n"
    "\n"
    "The angle is measured between the two edges. It is between 0 (parallel or anti-parallel edges) and 90 degree (perpendicular edges).\n"
    "\n"
    "With \"include_min_angle\" set to true (the default), the minimum angle is included in the criterion while with false, the "
    "minimum angle itself is not included. Same for \"include_max_angle\" where the default is false, meaning the maximum angle is not included in the range.\n"
    "\n"
    "This method has been added in version 0.27.2.\n"
  ) +
  method_ext ("split_with_internal_angle", split_with_internal_angle1, gsi::arg ("angle"),
    "@brief Like \\with_internal_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("split_with_internal_angle", split_with_internal_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Like \\with_internal_angle, but returning two edge pair collections\n"
    "The first edge pair collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("polygons", &polygons1,
    "@brief Converts the edge pairs to polygons\n"
    "This method creates polygons from the edge pairs. Each polygon will be a triangle or quadrangle "
    "which connects the start and end points of the edges forming the edge pair."
  ) +
  method_ext ("polygons", &polygons2, gsi::arg ("e"),
    "@brief Converts the edge pairs to polygons\n"
    "This method creates polygons from the edge pairs. Each polygon will be a triangle or quadrangle "
    "which connects the start and end points of the edges forming the edge pair. "
    "This version allows one to specify an enlargement which is applied to the edges. The length of the edges is "
    "modified by applying the enlargement and the edges are shifted by the enlargement. By specifying an "
    "enlargement it is possible to give edge pairs an area which otherwise would not have one (coincident edges, "
    "two point-like edges)."
  ) +
  method ("interacting", (db::EdgePairs (db::EdgePairs::*) (const db::Edges &, size_t, size_t) const)  &db::EdgePairs::selected_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the edge pairs from this edge pair collection which overlap or touch edges from the other edge collection\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs overlapping or touching edges from the other edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times an edge pair of this collection "
    "has to interact with (different) edges of the other collection to make the edge pair selected. An edge pair is "
    "not selected by this method if the number of edges interacting with an edge pair of this collection is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("not_interacting", (db::EdgePairs (db::EdgePairs::*) (const db::Edges &, size_t, size_t) const)  &db::EdgePairs::selected_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the edge pairs from this edge pair collection which do not overlap or touch edges from the other edge collection\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs not overlapping or touching edges from the other edge collection\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times an edge pair of this collection "
    "has to interact with (different) edges of the other collection to make the edge pair selected. An edge pair is "
    "not selected by this method if the number of edges interacting with an edge pair of this collection is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_interacting", (db::EdgePairs &(db::EdgePairs::*) (const db::Edges &, size_t, size_t)) &db::EdgePairs::select_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which overlap or touch edges from the other edge collection\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "This is the in-place version of \\interacting - i.e. self is modified rather than a new collection is returned.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_not_interacting", (db::EdgePairs &(db::EdgePairs::*) (const db::Edges &, size_t, size_t)) &db::EdgePairs::select_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which do not overlap or touch edges from the other edge collection\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "This is the in-place version of \\not_interacting - i.e. self is modified rather than a new collection is returned.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("split_interacting", &split_interacting_with_edges, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which do and do not interact with edges from the other collection\n"
    "\n"
    "@return A two-element list of edge pair collections (first: interacting, second: non-interacting)\n"
    "\n"
    "This method provides a faster way to compute both interacting and non-interacting edges compared to using separate methods. "
    "It has been introduced in version 0.29.6.\n"
  ) +
  method ("interacting", (db::EdgePairs (db::EdgePairs::*) (const db::Region &, size_t, size_t) const)  &db::EdgePairs::selected_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the edge pairs from this edge pair collection which overlap or touch polygons from the region\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs overlapping or touching polygons from the region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times an edge pair of this collection "
    "has to interact with (different) polygons of the other region to make the edge pair selected. An edge pair is "
    "not selected by this method if the number of polygons interacting with an edge pair of this collection is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("not_interacting", (db::EdgePairs (db::EdgePairs::*) (const db::Region &, size_t, size_t) const)  &db::EdgePairs::selected_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Returns the edge pairs from this edge pair collection which do not overlap or touch polygons from the region\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs not overlapping or touching polygons from the region\n"
    "\n"
    "'min_count' and 'max_count' impose a constraint on the number of times an edge pair of this collection "
    "has to interact with (different) polygons of the other region to make the edge pair selected. An edge pair is "
    "not selected by this method if the number of polygons interacting with an edge pair of this collection is between min_count and max_count "
    "(including max_count).\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_interacting", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &, size_t, size_t)) &db::EdgePairs::select_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which overlap or touch polygons from the region\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "This is the in-place version of \\interacting - i.e. self is modified rather than a new collection is returned.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_not_interacting", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &, size_t, size_t)) &db::EdgePairs::select_not_interacting, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which do not overlap or touch polygons from the region\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "This is the in-place version of \\not_interacting - i.e. self is modified rather than a new collection is returned.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("split_interacting", &split_interacting_with_region, gsi::arg ("other"), gsi::arg ("min_count", size_t (1)), gsi::arg ("max_count", size_t (std::numeric_limits<size_t>::max ()), "unlimited"),
    "@brief Selects the edge pairs from this edge pair collection which do and do not interact with polygons from the other region\n"
    "\n"
    "@return A two-element list of edge pair collections (first: interacting, second: non-interacting)\n"
    "\n"
    "This method provides a faster way to compute both interacting and non-interacting edges compared to using separate methods. "
    "It has been introduced in version 0.29.6.\n"
  ) +
  method ("inside", (db::EdgePairs (db::EdgePairs::*) (const db::Region &) const) &db::EdgePairs::selected_inside, gsi::arg ("other"),
    "@brief Returns the edge pairs from this edge pair collection which are inside (completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs completely inside polygons from the region\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("not_inside", (db::EdgePairs (db::EdgePairs::*) (const db::Region &) const) &db::EdgePairs::selected_not_inside, gsi::arg ("other"),
    "@brief Returns the edge pairs from this edge pair collection which are not inside (not completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge pair collection containing the edge pairs not completely inside polygons from the region\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_inside", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &)) &db::EdgePairs::select_inside, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are inside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_not_inside", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &)) &db::EdgePairs::select_not_inside, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are not inside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge pair collection after the edge pairs have been selected (self)\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("split_inside", &split_inside_with_region, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are and are not inside (completely covered by) polygons from the other region\n"
    "\n"
    "@return A two-element list of edge pair collections (first: inside, second: non-inside)\n"
    "\n"
    "This method provides a faster way to compute both inside and non-inside edge pairs compared to using separate methods. "
    "It has been introduced in version 0.29.6."
  ) +
  method ("outside", (db::EdgePairs (db::EdgePairs::*) (const db::Region &) const) &db::EdgePairs::selected_outside, gsi::arg ("other"),
    "@brief Returns the edge pairs from this edge pair collection which are outside (not overlapped by) polygons from the other region\n"
    "\n"
    "@return A new edge pair collection containing the the selected edges\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("not_outside", (db::EdgePairs (db::EdgePairs::*) (const db::Region &) const) &db::EdgePairs::selected_not_outside, gsi::arg ("other"),
    "@brief Returns the edge pairs from this edge pair collection which are not outside (partially overlapped by) polygons from the other region\n"
    "\n"
    "@return A new edge pair collection containing the the selected edges\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_outside", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &)) &db::EdgePairs::select_outside, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are outside (not overlapped by) polygons from the other region\n"
    "\n"
    "@return The edge pair collection after the edges have been selected (self)\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("select_not_outside", (db::EdgePairs &(db::EdgePairs::*) (const db::Region &)) &db::EdgePairs::select_not_outside, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are not outside (partially overlapped by) polygons from the other region\n"
    "\n"
    "@return The edge pair collection after the edges have been selected (self)\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("split_outside", &split_outside_with_region, gsi::arg ("other"),
    "@brief Selects the edge pairs from this edge pair collection which are and are not outside (not overlapped by) polygons from the other region\n"
    "\n"
    "@return A two-element list of edge pair collections (first: outside, second: non-outside)\n"
    "\n"
    "This method provides a faster way to compute both outside and non-outside edges compared to using separate methods. "
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("pull_interacting", &pull_interacting_edges, gsi::arg ("other"),
    "@brief Returns all edges of \"other\" which are interacting with (overlapping, touching) edge pairs of this set\n"
    "The \"pull_...\" methods are similar to \"select_...\" but work the opposite way: they "
    "select shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" methods "
    "provide a way for re-hierarchization.\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "@return The edge collection after the edges have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method_ext ("pull_interacting", &pull_interacting_polygons, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are interacting with (overlapping, touching) edge pairs of this set\n"
    "The \"pull_...\" methods are similar to \"select_...\" but work the opposite way: they "
    "select shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" methods "
    "provide a way for re-hierarchization.\n"
    "\n"
    "Edge pairs are considered 'filled' in the context of this operation - i.e. the area between the edges belongs to "
    "the edge pair, hence participates in the check.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "This method has been introduced in version 0.29.6\n"
  ) +
  method ("clear", &db::EdgePairs::clear,
    "@brief Clears the edge pair collection\n"
  ) +
  method ("swap", &db::EdgePairs::swap, gsi::arg ("other"),
    "@brief Swap the contents of this collection with the contents of another collection\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method ("bbox", &db::EdgePairs::bbox,
    "@brief Return the bounding box of the edge pair collection\n"
    "The bounding box is the box enclosing all points of all edge pairs.\n"
  ) +
  method ("is_empty?", &db::EdgePairs::empty,
    "@brief Returns true if the collection is empty\n"
  ) +
  method ("count|#size", (size_t (db::EdgePairs::*) () const) &db::EdgePairs::count,
    "@brief Returns the (flat) number of edge pairs in the edge pair collection\n"
    "\n"
    "The count is computed 'as if flat', i.e. edge pairs inside a cell are multiplied by the number of times a cell is instantiated.\n"
    "\n"
    "Starting with version 0.27, the method is called 'count' for consistency with \\Region. 'size' is still provided as an alias."
  ) +
  method ("hier_count", (size_t (db::EdgePairs::*) () const) &db::EdgePairs::hier_count,
    "@brief Returns the (hierarchical) number of edge pairs in the edge pair collection\n"
    "\n"
    "The count is computed 'hierarchical', i.e. edge pairs inside a cell are counted once even if the cell is instantiated multiple times.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::iterator_ext ("each", &begin_edge_pairs,
    "@brief Returns each edge pair of the edge pair collection\n"
    "\n"
    "Starting with version 0.30, the iterator delivers EdgePairWithProperties objects."
  ) +
  method_ext ("[]", &nth, gsi::arg ("n"),
    "@brief Returns the nth edge pair\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat edge pairs only - i.e. "
    "those for which \\has_valid_edge_pairs? is true. Use \\flatten to explicitly flatten an edge pair collection.\n"
    "\n"
    "The \\each iterator is the more general approach to access the edge pairs.\n"
    "\n"
    "Since version 0.30.1, this method returns a \\EdgePairWithProperties object."
  ) +
  method ("flatten", &db::EdgePairs::flatten,
    "@brief Explicitly flattens an edge pair collection\n"
    "\n"
    "If the collection is already flat (i.e. \\has_valid_edge_pairs? returns true), this method will "
    "not change the collection.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("has_valid_edge_pairs?", &db::EdgePairs::has_valid_edge_pairs,
    "@brief Returns true if the edge pair collection is flat and individual edge pairs can be accessed randomly\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("enable_progress", &db::EdgePairs::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the edge pair collection will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::EdgePairs::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the edge pair collection to a string\n"
    "The length of the output is limited to 20 edge pairs to avoid giant strings on large regions. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1, gsi::arg ("max_count"),
    "@brief Converts the edge pair collection to a string\n"
    "This version allows specification of the maximum number of edge pairs contained in the string."
  ) +
  gsi::make_property_methods<db::EdgePairs> ()
  ,
  "@brief EdgePairs (a collection of edge pairs)\n"
  "\n"
  "Edge pairs are used mainly in the context of the DRC functions (width_check, space_check etc.) of \\Region and \\Edges. "
  "A single edge pair represents two edges participating in a DRC violation. In the two-layer checks (inside, overlap) "
  "The first edge represents an edge from the first layer and the second edge an edge from the second layer. "
  "For single-layer checks (width, space) the order of the edges is arbitrary.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}
