
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

#include "dbTexts.h"
#include "dbRegion.h"
#include "dbDeepTexts.h"
#include "dbTextsUtils.h"
#include "dbPropertiesFilter.h"

#include "gsiDeclDbContainerHelpers.h"
#include "gsiDeclDbMeasureHelpers.h"

namespace gsi
{

// ---------------------------------------------------------------------------------
//  TextFilter binding

typedef shape_filter_impl<db::TextFilterBase> TextFilterBase;

class TextFilterImpl
  : public TextFilterBase
{
public:
  TextFilterImpl () { }

  bool issue_selected (const db::TextWithProperties &) const
  {
    return false;
  }

  virtual bool selected (const db::Text &text, db::properties_id_type prop_id) const
  {
    if (f_selected.can_issue ()) {
      return f_selected.issue<TextFilterImpl, bool, const db::TextWithProperties &> (&TextFilterImpl::issue_selected, db::TextWithProperties (text, prop_id));
    } else {
      return issue_selected (db::TextWithProperties (text, prop_id));
    }
  }

  gsi::Callback f_selected;

private:
  //  No copying
  TextFilterImpl &operator= (const TextFilterImpl &);
  TextFilterImpl (const TextFilterImpl &);
};

typedef db::generic_properties_filter<gsi::TextFilterBase, db::Text> TextPropertiesFilter;

static gsi::TextFilterBase *make_ppf1 (const tl::Variant &name, const tl::Variant &value, bool inverse)
{
  return new TextPropertiesFilter (name, value, inverse);
}

static gsi::TextFilterBase *make_ppf2 (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
{
  return new TextPropertiesFilter (name, from, to, inverse);
}

static gsi::TextFilterBase *make_pg (const tl::Variant &name, const std::string &glob, bool inverse, bool case_sensitive)
{
  tl::GlobPattern pattern (glob);
  pattern.set_case_sensitive (case_sensitive);
  return new TextPropertiesFilter (name, pattern, inverse);
}

static gsi::TextFilterBase *make_pe (const std::string &expression, bool inverse, const std::map<std::string, tl::Variant> &variables, double dbu)
{
  return new gsi::expression_filter<gsi::TextFilterBase, db::Texts> (expression, inverse, dbu, variables);
}

Class<gsi::TextFilterBase> decl_TextFilterBase ("db", "TextFilterBase",
  gsi::TextFilterBase::method_decls (false) +
  gsi::constructor ("property_glob", &make_pg, gsi::arg ("name"), gsi::arg ("pattern"), gsi::arg ("inverse", false), gsi::arg ("case_sensitive", true),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The glob pattern to match the property value against.\n"
    "@param inverse If true, inverts the selection - i.e. all texts without a matching property are selected.\n"
    "@param case_sensitive If true, the match is case sensitive (the default), if false, the match is not case sensitive.\n"
    "\n"
    "Apply this filter with \\Texts#filtered:\n"
    "\n"
    "@code\n"
    "# texts is a Texts object\n"
    "# filtered_texts contains all texts where the 'net' property starts with 'C':\n"
    "filtered_texts = texts.filtered(RBA::TextFilterBase::property_glob('net', 'C*'))\n"
    "@/code\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter", &make_ppf1, gsi::arg ("name"), gsi::arg ("value"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param value The value against which the property is checked (exact match).\n"
    "@param inverse If true, inverts the selection - i.e. all texts without a property with the given name and value are selected.\n"
    "\n"
    "Apply this filter with \\Texts#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("property_filter_bounded", &make_ppf2, gsi::arg ("name"), gsi::arg ("from"), gsi::arg ("to"), gsi::arg ("inverse", false),
    "@brief Creates a single-valued property filter\n"
    "@param name The name of the property to use.\n"
    "@param from The lower value against which the property is checked or 'nil' if no lower bound shall be used.\n"
    "@param to The upper value against which the property is checked or 'nil' if no upper bound shall be used.\n"
    "@param inverse If true, inverts the selection - i.e. all texts without a property with the given name and value range are selected.\n"
    "\n"
    "This version does a bounded match. The value of the propery needs to be larger or equal to 'from' and less than 'to'.\n"
    "Apply this filter with \\Texts#filtered. See \\property_glob for an example.\n"
    "\n"
    "This feature has been introduced in version 0.30."
  ) +
  gsi::constructor ("expression_filter", &make_pe, gsi::arg ("expression"), gsi::arg ("inverse", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates an expression-based filter\n"
    "@param expression The expression to evaluate.\n"
    "@param inverse If true, inverts the selection - i.e. all texts without a property with the given name and value range are selected.\n"
    "@param dbu If given and greater than zero, the shapes delivered by the 'shape' function will be in micrometer units.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
    "\n"
    "Creates a filter that will evaluate the given expression on every shape and select the shape "
    "when the expression renders a boolean true value. "
    "The expression may use the following variables and functions:\n"
    "\n"
    "@ul\n"
    "@li @b shape @/b: The current shape (i.e. 'Text' without DBU specified or 'DText' otherwise) @/li\n"
    "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
    "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
    "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
    "@/ul\n"
    "\n"
    "This feature has been introduced in version 0.30.3."
  ),
  "@hide"
);

Class<gsi::TextFilterImpl> decl_TextFilterImpl (decl_TextFilterBase, "db", "TextFilter",
  callback ("selected", &TextFilterImpl::issue_selected, &TextFilterImpl::f_selected, gsi::arg ("text"),
    "@brief Selects a text\n"
    "This method is the actual payload. It needs to be reimplemented in a derived class.\n"
    "It needs to analyze the text and return 'true' if it should be kept and 'false' if it should be discarded."
    "\n"
    "Since version 0.30, the text carries properties."
  ),
  "@brief A generic text filter adaptor\n"
  "\n"
  "Text filters are an efficient way to filter texts from a Texts collection. To apply a filter, derive your own "
  "filter class and pass an instance to \\Texts#filter or \\Texts#filtered method.\n"
  "\n"
  "Conceptually, these methods take each text from the collection and present it to the filter's 'selected' method.\n"
  "Based on the result of this evaluation, the text is kept or discarded.\n"
  "\n"
  "The magic happens when deep mode text collections are involved. In that case, the filter will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the filter behaves. You "
  "need to configure the filter by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using the filter.\n"
  "\n"
  "You can skip this step, but the filter algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that filters texts with a given string length:"
  "\n"
  "@code\n"
  "class TextStringLengthFilter < RBA::TextFilter\n"
  "\n"
  "  # Constructor\n"
  "  def initialize(string_length)\n"
  "    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter\n"
  "    @string_length = string_length\n"
  "  end\n"
  "  \n"
  "  # Select texts with given string length\n"
  "  def selected(text)\n"
  "    return text.string.size == @string_length\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "texts = ... # some Texts object\n"
  "with_length_3 = edges.filtered(TextStringLengthFilter::new(3))\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  TextProcessor binding

Class<db::TextProcessorBase> decl_TextProcessorBase ("db", "TextProcessorBase", "@hide");

Class<shape_processor_impl<db::TextProcessorBase> > decl_TextProcessor (decl_TextProcessorBase, "db", "TextOperator",
  shape_processor_impl<db::TextProcessorBase>::method_decls (false),
  "@brief A generic text operator\n"
  "\n"
  "Text processors are an efficient way to process texts from an text collection. To apply a processor, derive your own "
  "operator class and pass an instance to the \\Texts#processed or \\Texts#process method.\n"
  "\n"
  "Conceptually, these methods take each text from the edge pair collection and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output texts derived from the input text.\n"
  "The output text collection is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode text collections are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "Here is some example that replaces the text string:"
  "\n"
  "@code\n"
  "class ReplaceTextString < RBA::TextOperator\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter\n"
  "  end\n"
  "  \n"
  "  # Replaces the string by a number representing the string length\n"
  "  def process(text)\n"
  "    new_text = text.dup   # need a copy as we cannot modify the text passed\n"
  "    new_text.string = text.string.size.to_s\n"
  "    return [ new_text ]\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "texts = ... # some Texts object\n"
  "modified = texts.processed(ReplaceTextString::new)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

static
property_computation_processor<db::TextProcessorBase, db::Texts> *
new_pcp (const db::Texts *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  return new property_computation_processor<db::TextProcessorBase, db::Texts> (container, expressions, copy_properties, dbu, variables);
}

property_computation_processor<db::TextProcessorBase, db::Texts> *
new_pcps (const db::Texts *container, const std::string &expression, bool copy_properties, const std::map <std::string, tl::Variant> &variables, double dbu)
{
  std::map<tl::Variant, std::string> expressions;
  expressions.insert (std::make_pair (tl::Variant (), expression));
  return new property_computation_processor<db::TextProcessorBase, db::Texts> (container, expressions, copy_properties, dbu, variables);
}

Class<property_computation_processor<db::TextProcessorBase, db::Texts> > decl_TextPropertiesExpressions (decl_TextProcessorBase, "db", "TextPropertiesExpressions",
  property_computation_processor<db::TextProcessorBase, db::Texts>::method_decls (true) +
  gsi::constructor ("new", &new_pcp, gsi::arg ("texts"), gsi::arg ("expressions"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param texts The text collection, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
    "@param expressions A map of property names and expressions used to generate the values of the properties (see class description for details).\n"
    "@param copy_properties If true, new properties will be added to existing ones.\n"
    "@param dbu If not zero, this value specifies the database unit to use. If given, the shapes returned by the 'shape' function will be micrometer-unit objects.\n"
    "@param variables Arbitrary values that are available as variables inside the expressions.\n"
  ) +
  gsi::constructor ("new", &new_pcps, gsi::arg ("texts"), gsi::arg ("expression"), gsi::arg ("copy_properties", false), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", 0.0),
    "@brief Creates a new properties expressions operator\n"
    "\n"
    "@param texts The text collection, the processor will be used on. Can be nil, but if given, allows some optimization.\n"
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
  "@li @b shape @/b: The current shape (i.e. 'Text' without DBU specified or 'DText' otherwise) @/li\n"
  "@li @b put(<name>, <value>) @/b: Attaches the given value as a property with name 'name' to the output shape @/li\n"
  "@li @b skip(<flag>) @/b: If called with a 'true' value, the shape is dropped from the output @/li\n"
  "@li @b value(<name>) @/b: The value of the property with the given name (the first one if there are multiple properties with the same name) @/li\n"
  "@li @b values(<name>) @/b: All values of the properties with the given name (returns a list) @/li\n"
  "@li @b <name> @/b: A shortcut for 'value(<name>)' (<name> is used as a symbol) @/li\n"
  "@/ul\n"
  "\n"
  "This class has been introduced in version 0.30.3.\n"
);

Class<db::TextToPolygonProcessorBase> decl_TextToPolygonProcessorBase ("db", "TextToPolygonProcessorBase", "@hide");

Class<shape_processor_impl<db::TextToPolygonProcessorBase> > decl_TextToPolygonProcessor (decl_TextToPolygonProcessorBase, "db", "TextToPolygonOperator",
  shape_processor_impl<db::TextToPolygonProcessorBase>::method_decls (false),
  "@brief A generic text-to-polygon operator\n"
  "\n"
  "Text processors are an efficient way to process texts from an text collection. To apply a processor, derive your own "
  "operator class and pass an instance to the \\Texts#processed method.\n"
  "\n"
  "Conceptually, these methods take each text from the text collection and present it to the operator's 'process' method.\n"
  "The result of this call is a list of zero to many output polygons derived from the input text.\n"
  "The output region is the sum over all these individual results.\n"
  "\n"
  "The magic happens when deep mode text collections are involved. In that case, the processor will use as few calls as possible "
  "and exploit the hierarchical compression if possible. It needs to know however, how the operator behaves. You "
  "need to configure the operator by calling \\is_isotropic, \\is_scale_invariant or \\is_isotropic_and_scale_invariant "
  "before using it.\n"
  "\n"
  "You can skip this step, but the processor algorithm will assume the worst case then. This usually leads to cell variant "
  "formation which is not always desired and blows up the hierarchy.\n"
  "\n"
  "For a basic example see the \\TextOperator class, with the exception that this incarnation delivers polygons.\n"
  "\n"
  "This class has been introduced in version 0.29.\n"
);

// ---------------------------------------------------------------------------------
//  Texts binding

static inline std::vector<db::Texts> as_2texts_vector (const std::pair<db::Texts, db::Texts> &rp)
{
  std::vector<db::Texts> res;
  res.reserve (2);
  res.push_back (db::Texts (const_cast<db::Texts &> (rp.first).take_delegate ()));
  res.push_back (db::Texts (const_cast<db::Texts &> (rp.second).take_delegate ()));
  return res;
}

static db::Texts *new_v ()
{
  return new db::Texts ();
}

static db::Texts *new_a (const std::vector<db::Text> &t)
{
  return new db::Texts (t.begin (), t.end ());
}

static db::Texts *new_ap (const std::vector<db::TextWithProperties> &t, bool)
{
  return new db::Texts (t.begin (), t.end ());
}

static db::Texts *new_text (const db::Text &t)
{
  return new db::Texts (t);
}

static db::Texts *new_textp (const db::TextWithProperties &t)
{
  return new db::Texts (t);
}

static db::Texts *new_shapes (const db::Shapes &s)
{
  db::Texts *r = new db::Texts ();
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Texts); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::Texts *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::Texts (si);
}

static db::Texts *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::Texts (si, trans);
}

static db::Texts *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss)
{
  return new db::Texts (si, dss);
}

static db::Texts *new_si2d (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans)
{
  return new db::Texts (si, dss, trans);
}

static std::string to_string0 (const db::Texts *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::Texts *r, size_t n)
{
  return r->to_string (n);
}

static db::Texts &move_p (db::Texts *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::Texts &move_xy (db::Texts *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::Texts moved_p (const db::Texts *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::Texts moved_xy (const db::Texts *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region polygons0 (const db::Texts *e, db::Coord d, const tl::Variant &text_prop)
{
  db::Region r;
  e->polygons (r, d, text_prop);
  return r;
}

static db::Region extents1 (const db::Texts *r, db::Coord dx, db::Coord dy)
{
  db::Region output;
  r->processed (output, db::extents_processor<db::Text> (dx, dy));
  return output;
}

static db::Region extents0 (const db::Texts *r, db::Coord d)
{
  return extents1 (r, d, d);
}

static db::Edges edges (const db::Texts *ep)
{
  db::Edges e;
  ep->edges (e);
  return e;
}

static void insert_t (db::Texts *t, const db::Texts &a)
{
  for (db::Texts::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    t->insert (*p);
  }
}

static bool is_deep (const db::Texts *t)
{
  return dynamic_cast<const db::DeepTexts *> (t->delegate ()) != 0;
}

static size_t id (const db::Texts *t)
{
  return tl::id_of (t->delegate ());
}

static db::Texts filtered (const db::Texts *r, const gsi::TextFilterBase *f)
{
  return r->filtered (*f);
}

static void filter (db::Texts *r, const gsi::TextFilterBase *f)
{
  r->filter (*f);
}

static std::vector<db::Texts> split_filter (const db::Texts *r, const TextFilterImpl *f)
{
  return as_2texts_vector (r->split_filter (*f));
}

static db::Texts processed_tt (const db::Texts *r, const db::TextProcessorBase *f)
{
  return r->processed (*f);
}

static void process_tt (db::Texts *r, const db::TextProcessorBase *f)
{
  r->process (*f);
}

static db::Region processed_tp (const db::Texts *r, const db::TextToPolygonProcessorBase *f)
{
  db::Region out;
  r->processed (out, *f);
  return out;
}

static db::Texts with_text (const db::Texts *r, const std::string &text, bool inverse)
{
  db::TextStringFilter f (text, inverse);
  return r->filtered (f);
}

static std::vector<db::Texts> split_with_text (const db::Texts *r, const std::string &text)
{
  db::TextStringFilter f (text, false);
  return as_2texts_vector (r->split_filter (f));
}

static db::Texts with_match (const db::Texts *r, const std::string &pattern, bool inverse)
{
  db::TextPatternFilter f (pattern, inverse);
  return r->filtered (f);
}

static std::vector<db::Texts> split_with_match (const db::Texts *r, const std::string &pattern)
{
  db::TextPatternFilter f (pattern, false);
  return as_2texts_vector (r->split_filter (f));
}

static db::Region pull_interacting (const db::Texts *r, const db::Region &other)
{
  db::Region out;
  r->pull_interacting (out, other);
  return out;
}

static tl::Variant nth (const db::Texts *texts, size_t n)
{
  const db::Text *t = texts->nth (n);
  if (! t) {
    return tl::Variant ();
  } else {
    return tl::Variant (db::TextWithProperties (*t, texts->nth_prop_id (n)));
  }
}

static db::generic_shape_iterator<db::TextWithProperties> begin_texts (const db::Texts *texts)
{
  return db::generic_shape_iterator<db::TextWithProperties> (db::make_wp_iter (texts->delegate ()->begin ()));
}

extern Class<db::ShapeCollection> decl_dbShapeCollection;

Class<db::Texts> decl_Texts (decl_dbShapeCollection, "db", "Texts",
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty text collection.\n"
  ) + 
  constructor ("new", &new_a, gsi::arg ("array"),
    "@brief Constructor from a text array\n"
    "\n"
    "This constructor creates an text collection from an array of \\Text objects.\n"
  ) +
  //  This is a dummy constructor that allows creating a Texts collection from an array
  //  of TextWithProperties objects too. GSI needs the dummy argument to
  //  differentiate between the cases when an empty array is passed.
  constructor ("new", &new_ap, gsi::arg ("array"), gsi::arg ("dummy", true),
    "@hide"
  ) +
  constructor ("new", &new_text, gsi::arg ("text"),
    "@brief Constructor from a single text object\n"
    "\n"
    "This constructor creates an text collection with a single text.\n"
  ) +
  constructor ("new", &new_textp, gsi::arg ("text"),
    "@brief Constructor from a single text object\n"
    "\n"
    "This constructor creates an text collection with a single text with properties.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  constructor ("new", &new_shapes, gsi::arg ("shapes"),
    "@brief Shapes constructor\n"
    "\n"
    "This constructor creates an text collection from a \\Shapes collection.\n"
  ) +
  constructor ("new", &new_si, gsi::arg ("shape_iterator"),
    "@brief Constructor from a hierarchical shape set\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only texts are taken from the shape set and other shapes are ignored.\n"
    "This method allows feeding the text collection from a hierarchy of cells.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only texts are taken from the shape set and other shapes are ignored.\n"
    "The given transformation is applied to each text taken.\n"
    "This method allows feeding the text collection from a hierarchy of cells.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("dss"),
    "@brief Creates a hierarchical text collection from an original layer\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical text collection which supports hierarchical operations.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2d, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("trans"),
    "@brief Creates a hierarchical text collection from an original layer with a transformation\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical text collection which supports hierarchical operations.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  method ("write", &db::Texts::write, gsi::arg ("filename"),
    "@brief Writes the region to a file\n"
    "This method is provided for debugging purposes. It writes the object to a flat layer 0/0 in a single top cell.\n"
    "\n"
    "This method has been introduced in version 0.29."
  ) +
  method ("insert_into", &db::Texts::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this texts into the given layout, below the given cell and into the given layer.\n"
    "If the text collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
  ) +
  method ("insert_into_as_polygons", &db::Texts::insert_into_as_polygons, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("e"),
    "@brief Inserts this texts into the given layout, below the given cell and into the given layer.\n"
    "If the text collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "The texts will be converted to polygons with the enlargement value given be 'e'. See \\polygon or \\extents for details.\n"
  ) +
  method ("insert", (void (db::Texts::*) (const db::Text &)) &db::Texts::insert, gsi::arg ("text"),
    "@brief Inserts a text into the collection\n"
  ) +
  method ("insert", (void (db::Texts::*) (const db::TextWithProperties &)) &db::Texts::insert, gsi::arg ("text"),
    "@brief Inserts a text into the collection\n"
    "\n"
    "This variant accepting a text with properties has been introduced in version 0.30."
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the edge pair collection is a deep (hierarchical) one\n"
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
  ) +
  method ("+|join", &db::Texts::operator+, gsi::arg ("other"),
    "@brief Returns the combined text collection of self and the other one\n"
    "\n"
    "@return The resulting text collection\n"
    "\n"
    "This operator adds the texts of the other collection to self and returns a new combined set.\n"
    "\n"
    "The 'join' alias has been introduced in version 0.28.12."
  ) +
  method ("+=|join_with", &db::Texts::operator+=, gsi::arg ("other"),
    "@brief Adds the texts of the other text collection to self\n"
    "\n"
    "@return The text collection after modification (self)\n"
    "\n"
    "This operator adds the texts of the other collection to self.\n"
    "\n"
    "Note that in Ruby, the '+=' operator actually does not exist, but is emulated by '+' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'join_with' instead.\n"
    "\n"
    "The 'join_with' alias has been introduced in version 0.28.12."
  ) +
  method_ext ("move", &move_p, gsi::arg ("v"),
    "@brief Moves the text collection\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved text collection. The text collection is overwritten.\n"
    "\n"
    "@param v The distance to move the texts.\n"
    "\n"
    "@return The moved texts (self).\n"
  ) +
  method_ext ("move", &move_xy, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Moves the text collection\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved texts. The edge pair collection is overwritten.\n"
    "\n"
    "@param dx The x distance to move the texts.\n"
    "@param dy The y distance to move the texts.\n"
    "\n"
    "@return The moved texts (self).\n"
  ) +
  method_ext ("moved", &moved_p, gsi::arg ("v"),
    "@brief Returns the moved text collection (does not modify self)\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved texts. The text collection is not modified.\n"
    "\n"
    "@param v The distance to move the texts.\n"
    "\n"
    "@return The moved texts.\n"
  ) +
  method_ext ("moved", &moved_xy, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved texts. The text collection is not modified.\n"
    "\n"
    "@param dx The x distance to move the texts.\n"
    "@param dy The y distance to move the texts.\n"
    "\n"
    "@return The moved texts.\n"
  ) +
  method ("transformed", (db::Texts (db::Texts::*)(const db::Trans &) const) &db::Texts::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection\n"
    "\n"
    "Transforms the texts with the given transformation.\n"
    "Does not modify the edge pair collection but returns the transformed texts.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed texts.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::Texts (db::Texts::*)(const db::ICplxTrans &) const) &db::Texts::transformed, gsi::arg ("t"),
    "@brief Transform the text collection with a complex transformation\n"
    "\n"
    "Transforms the text with the given complex transformation.\n"
    "Does not modify the text collection but returns the transformed texts.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed texts.\n"
  ) +
  method ("transform", (db::Texts &(db::Texts::*)(const db::Trans &)) &db::Texts::transform, gsi::arg ("t"),
    "@brief Transform the text collection (modifies self)\n"
    "\n"
    "Transforms the text collection with the given transformation.\n"
    "This version modifies the text collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed text collection.\n"
  ) +
  method ("transform|#transform_icplx", (db::Texts &(db::Texts::*)(const db::ICplxTrans &)) &db::Texts::transform, gsi::arg ("t"),
    "@brief Transform the text collection with a complex transformation (modifies self)\n"
    "\n"
    "Transforms the text collection with the given transformation.\n"
    "This version modifies the text collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed text collection.\n"
  ) +
  method_ext ("insert", &insert_t, gsi::arg ("texts"),
    "@brief Inserts all texts from the other text collection into this collection\n"
  ) +
  method_ext ("edges", &edges,
    "@brief Returns dot-like edges for the texts\n"
    "@return An edge collection containing the individual, dot-like edges\n"
  ) +
  method_ext ("extents", &extents0, gsi::arg ("d", db::Coord (1)),
    "@brief Returns a region with the enlarged bounding boxes of the texts\n"
    "Text bounding boxes are point-like boxes which vanish unless an enlargement of >0 is specified.\n"
    "The bounding box is centered at the text's location.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents1, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Returns a region with the enlarged bounding boxes of the texts\n"
    "This method acts like the other version of \\extents, but allows giving different enlargements for x and y direction.\n"
  ) + 
  method_ext ("polygons", &polygons0, gsi::arg ("e", db::Coord (1)), gsi::arg ("text_prop", tl::Variant (), "nil"),
    "@brief Converts the edge pairs to polygons\n"
    "This method creates polygons from the texts. This is basically equivalent to calling \\extents. "
    "In addition, a user property with the key given by 'text_prop' can be attached. The value of that "
    "user property will be the text string. If 'text_prop' is nil, no user property is attached.\n"
    "\n"
    "The 'text_prop' argument has been added in version 0.30."
  ) +
  method_ext ("filter", &filter, gsi::arg ("filter"),
    "@brief Applies a generic filter in place (replacing the texts from the Texts collection)\n"
    "See \\TextFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("filtered", &filtered, gsi::arg ("filtered"),
    "@brief Applies a generic filter and returns a filtered copy\n"
    "See \\TextFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("split_filter", &split_filter, gsi::arg ("filter"),
    "@brief Applies a generic filter and returns a copy with all matching shapes and one with the non-matching ones\n"
    "See \\TextFilter for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("process", &process_tt, gsi::arg ("process"),
    "@brief Applies a generic text processor in place (replacing the texts from the text collection)\n"
    "See \\TextProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_tt, gsi::arg ("processed"),
    "@brief Applies a generic text processor and returns a processed copy\n"
    "See \\TextProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("processed", &processed_tp, gsi::arg ("processed"),
    "@brief Applies a generic text-to-polygon processor and returns a region with the results\n"
    "See \\TextToPolygonProcessor for a description of this feature.\n"
    "\n"
    "This method has been introduced in version 0.29.\n"
  ) +
  method_ext ("with_text", with_text, gsi::arg ("text"), gsi::arg ("inverse"),
    "@brief Filter the text by text string\n"
    "If \"inverse\" is false, this method returns the texts with the given string.\n"
    "If \"inverse\" is true, this method returns the texts not having the given string.\n"
  ) +
  method_ext ("split_with_text", split_with_text, gsi::arg ("text"),
    "@brief Like \\with_text, but returning two text collections\n"
    "The first text collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method_ext ("with_match", with_match, gsi::arg ("pattern"), gsi::arg ("inverse"),
    "@brief Filter the text by glob pattern\n"
    "\"pattern\" is a glob-style pattern (e.g. \"A*\" will select all texts starting with a capital \"A\").\n"
    "If \"inverse\" is false, this method returns the texts matching the pattern.\n"
    "If \"inverse\" is true, this method returns the texts not matching the pattern.\n"
  ) +
  method_ext ("split_with_match", split_with_match, gsi::arg ("pattern"),
    "@brief Like \\with_match, but returning two text collections\n"
    "The first text collection will contain all matching shapes, the other the non-matching ones.\n"
    "\n"
    "This method has been introduced in version 0.29.12.\n"
  ) +
  method ("interacting|&", (db::Texts (db::Texts::*) (const db::Region &) const)  &db::Texts::selected_interacting, gsi::arg ("other"),
    "@brief Returns the texts from this text collection which are inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A new text collection containing the texts inside or on the edge of polygons from the region\n"
  ) +
  method ("not_interacting|-", (db::Texts (db::Texts::*) (const db::Region &) const)  &db::Texts::selected_not_interacting, gsi::arg ("other"),
    "@brief Returns the texts from this text collection which are not inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A new text collection containing the texts not inside or on the edge of polygons from the region\n"
  ) +
  method ("select_interacting", (db::Texts &(db::Texts::*) (const db::Region &)) &db::Texts::select_interacting, gsi::arg ("other"),
    "@brief Selects the texts from this text collection which are inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A text collection after the texts have been selected (self)\n"
    "\n"
    "In contrast to \\interacting, this method will modify self.\n"
  ) +
  method ("select_not_interacting", (db::Texts &(db::Texts::*) (const db::Region &)) &db::Texts::select_not_interacting, gsi::arg ("other"),
    "@brief Selects the texts from this text collection which are not inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A text collection after the texts have been selected (self)\n"
    "\n"
    "In contrast to \\interacting, this method will modify self.\n"
  ) +
  method_ext ("pull_interacting", &pull_interacting, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are including texts of this text set\n"
    "The \"pull_...\" method is similar to \"select_...\" but works the opposite way: it "
    "selects shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" method "
    "provide a way for re-hierarchization.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for the polygon region.\n"
  ) +
  method ("clear", &db::Texts::clear,
    "@brief Clears the text collection\n"
  ) +
  method ("swap", &db::Texts::swap, gsi::arg ("other"),
    "@brief Swap the contents of this collection with the contents of another collection\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method ("bbox", &db::Texts::bbox,
    "@brief Return the bounding box of the text collection\n"
    "The bounding box is the box enclosing all origins of all texts.\n"
  ) +
  method ("is_empty?", &db::Texts::empty,
    "@brief Returns true if the collection is empty\n"
  ) +
  method ("count|#size", (size_t (db::Texts::*) () const) &db::Texts::count,
    "@brief Returns the (flat) number of texts in the text collection\n"
    "\n"
    "The count is computed 'as if flat', i.e. texts inside a cell are multiplied by the number of times a cell is instantiated.\n"
    "\n"
    "Starting with version 0.27, the method is called 'count' for consistency with \\Region. 'size' is still provided as an alias."
  ) +
  method ("hier_count", (size_t (db::Texts::*) () const) &db::Texts::hier_count,
    "@brief Returns the (hierarchical) number of texts in the text collection\n"
    "\n"
    "The count is computed 'hierarchical', i.e. texts inside a cell are counted once even if the cell is instantiated multiple times.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::iterator_ext ("each", &begin_texts,
    "@brief Returns each text of the text collection\n"
    "\n"
    "Starting with version 0.30, the iterator delivers TextWithProperties objects."
  ) +
  method_ext ("[]", &nth, gsi::arg ("n"),
    "@brief Returns the nth text\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat texts only - i.e. "
    "those for which \\has_valid_texts? is true. Use \\flatten to explicitly flatten an text collection.\n"
    "\n"
    "The \\each iterator is the more general approach to access the texts.\n"
    "\n"
    "Since version 0.30.1, this method returns a \\TextWithProperties object."
  ) +
  method ("flatten", &db::Texts::flatten,
    "@brief Explicitly flattens an text collection\n"
    "\n"
    "If the collection is already flat (i.e. \\has_valid_texts? returns true), this method will "
    "not change the collection.\n"
  ) +
  method ("has_valid_texts?", &db::Texts::has_valid_texts,
    "@brief Returns true if the text collection is flat and individual texts can be accessed randomly\n"
  ) +
  method ("enable_progress", &db::Texts::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the text collection will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::Texts::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the text collection to a string\n"
    "The length of the output is limited to 20 texts to avoid giant strings on large collections. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1, gsi::arg ("max_count"),
    "@brief Converts the text collection to a string\n"
    "This version allows specification of the maximum number of texts contained in the string."
  ) +
  gsi::make_property_methods<db::Texts> ()
  ,
  "@brief Texts (a collection of texts)\n"
  "\n"
  "Text objects are useful as labels for net names, to identify certain regions and to specify specific locations in general. "
  "Text collections provide a way to store - also in a hierarchical fashion - and manipulate a collection of text objects.\n"
  "\n"
  "Text objects can be turned into polygons by creating small boxes around the texts (\\polygons). Texts can also be turned into dot-like "
  "edges (\\edges). Texts can be filtered by string, either by matching against a fixed string (\\with_text) or a glob-style pattern (\\with_match).\n"
  "\n"
  "Text collections can be filtered geometrically against a polygon \\Region using \\interacting or \\non-interacting. "
  "Vice versa, texts can be used to select polygons from a \\Region using \\pull_interacting.\n"
  "\n"
  "Beside that, text collections can be transformed, flattened and combined, similar to \\EdgePairs.\n"
  "\n"
  "This class has been introduced in version 0.27.\n"
);

}
