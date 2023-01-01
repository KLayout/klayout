
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



#ifndef HDR_dbStreamLayers
#define HDR_dbStreamLayers

#include "dbCommon.h"

#include "tlException.h"
#include "tlIntervalMap.h"
#include "tlString.h"
#include "dbLayerProperties.h"
#include "gsiObject.h"

#include <map>
#include <limits>
#include <set>

namespace db
{

class Layout;

/**
 *  @brief Layer spec string format error exception
 */
class DB_PUBLIC LayerSpecFormatException
  : public tl::Exception 
{
public:
  LayerSpecFormatException (const char *s)
    : tl::Exception (tl::to_string (tr ("Not a valid layer map expression: '..%s' (use '/' to separated layer and datatype, ',' to list numbers for layer or datatype, '-' to create ranges and ';' to concatenate multiple subexpressions)")), s)
  { }
};

/**
 *  @brief The basic layer/datatype type
 */
typedef int ld_type;

/**
 *  @brief Some definitions to declare wildcard and relative datatypes or layers
 */
inline ld_type any_ld ()
{
  return -1;
}

inline bool is_any_ld (ld_type ld)
{
  return ld == -1;
}

inline bool is_static_ld (ld_type ld)
{
  return ld >= 0;
}

inline ld_type relative_ld (ld_type ld)
{
  if (ld < 0) {
    return std::numeric_limits<ld_type>::min() - ld;
  } else {
    //  NOTE: this way "any_ld" is equivalent to "relative_ld(0)"
    return -ld - 1;
  }
}

inline bool is_relative_ld (ld_type ld)
{
  return ld < 0;
}

inline ld_type ld_offset (ld_type ld)
{
  if (ld < 0) {
    ld_type neg = ld - std::numeric_limits<ld_type>::min();
    ld_type pos = -(ld + 1);
    return neg < pos ? -neg : pos;
  } else {
    return ld;
  }
}

/**
 *  @brief With a relative b, b is added to a. Otherwise b replaces a.
 */
inline ld_type ld_combine (ld_type a, ld_type b)
{
  return is_relative_ld (b) ? a + ld_offset (b) : b;
}

/**
 *  @brief A struct for a layer/datatype pair)
 */
struct DB_PUBLIC LDPair
{
  LDPair ()
    : layer (0), datatype (0)
  { }

  LDPair (ld_type l, ld_type d)
    : layer (l), datatype (d)
  { }

  static LDPair invalid () 
  {
    return LDPair (-1, -1);
  }

  bool operator< (const LDPair &d) const
  {
    return layer < d.layer || (layer == d.layer && datatype < d.datatype);
  }

  bool operator== (const LDPair &d) const
  {
    return layer == d.layer && datatype == d.datatype;
  }

  bool operator!= (const LDPair &d) const
  {
    return ! operator== (d);
  }

  ld_type layer;
  ld_type datatype;
};

/**
 *  @brief A layer map (stream to logical layers)
 *
 *  The mapping object provides a lookup for a given input layer
 *  (called "physical layer") to a logical layer specified as
 *  a layer index used inside db::Layout. The object allows
 *  looking up a layer index for a given layer/datatype/name.
 *
 *  It also allows one to specify a target layer per logical layer. That
 *  is the information attached to the layer when it is actually created.
 *  This allows mapping an input layer to another layer specification
 *  and effectively rename a layer or add layer name information to
 *  a GDS layer/datatype layer.
 *
 *  "Unmapping" can be used to create "holes" in ranges of layers.
 *  For example, by first mapping layers 1 to 100, datatype 0 and then
 *  unmapping layer 50, datatype 0, the layers 1 to 49 and 51 to 100, datatype 0
 *  are mapped.
 *
 *  The layer map supports multi-mapping. That is, one input layer is
 *  mapped to multiple target layers. It also supports merging but
 *  mapping different input layers to a single target layer.
 *
 *  A layer map object can be used as a standalone object or in 
 *  conjunction with a layout object. As a standalone object, the
 *  logical layers (indexes) are simply consecutive numbers.
 *  Such objects are used as input for the reader function inside
 *  LoadLayoutOptions for example. Layer map objects provided as
 *  output from reader functions have a connection to a real
 *  layout and for those, the layer index refers to the actual
 *  layer in that layout.
 *
 *  The object supports persistency to and from a string in two
 *  formats: a compact format and a linewise format used for storing
 *  the information in setup files.
 */
class DB_PUBLIC LayerMap
  : public gsi::ObjectBase
{
public:
  typedef tl::interval_map<ld_type, std::set<unsigned int> > datatype_map;
  typedef tl::interval_map<ld_type, datatype_map> ld_map;
  typedef ld_map::const_iterator const_iterator_layers;
  typedef datatype_map::const_iterator const_iterator_datatypes;
  typedef std::map<std::string, std::set<unsigned int> >::const_iterator const_iterator_names;

  /**
   *  @brief The constructor for an empty map
   */
  LayerMap ();

  /**
   *  @brief Returns the first logical layer for a given layer specification
   *  The first value of the pair indicates whether there is a valid mapping.
   *  The second value will give the layer to map to.
   */
  template <class L>
  std::pair<bool, unsigned int> first_logical (const L &p) const
  {
    std::set<unsigned int> r = logical (p);
    if (r.empty ()) {
      return std::make_pair (false, 0);
    } else {
      return std::make_pair (true, *r.begin ());
    }
  }

  /**
   *  @brief Returns the first logical layer for a given layer specification
   *  The first value of the pair indicates whether there is a valid mapping.
   *  The second value will give the layer to map to.
   */
  template <class L>
  std::pair<bool, unsigned int> first_logical (const L &p, db::Layout &layout) const
  {
    std::set<unsigned int> r = logical (p, layout);
    if (r.empty ()) {
      return std::make_pair (false, 0);
    } else {
      return std::make_pair (true, *r.begin ());
    }
  }

  /**
   *  @brief Query a layer mapping
   *
   *  @return A set of layers which are designated targets.
   */
  std::set<unsigned int> logical (const LDPair &p) const;

  /** 
   *  @brief Query a layer mapping from a name
   *
   *  @return A set of layers which are designated targets.
   */
  std::set<unsigned int> logical (const std::string &name) const;

  /** 
   *  @brief Query a layer mapping from a name or LDPair
   *
   *  @return A set of layers which are designated targets.
   *
   *  @param p The layer that is looked for
   */
  std::set<unsigned int> logical (const db::LayerProperties &p) const;

  /**
   *  @brief Query or install a layer mapping from a name or LDPair
   *
   *  @return A set of layers which are designated targets.
   *
   *  @param p The layer that is looked for
   *
   *  This version is used for wildcard and relative mapping. In this case,
   *  the logical layers are placeholder values which will be replaced by
   *  true layers during this method if a new layer is requested.
   */
  std::set<unsigned int> logical (const db::LayerProperties &p, db::Layout &layout) const;

  /**
   *  @brief Query or install a layer mapping from a LDPair
   *
   *  See the version for LayerProperties about details.
   */
  std::set<unsigned int> logical (const db::LDPair &p, db::Layout &layout) const;

  /**
   *  @brief Returns a value indicating whether a layer (given by layer/datatype) is mapped
   */
  bool is_mapped (const LDPair &p) const;

  /**
   *  @brief Returns a value indicating whether the given named layer is mapped
   */
  bool is_mapped (const std::string &name) const;

  /**
   *  @brief Returns a value indicating whether a layer is mapped
   */
  bool is_mapped (const db::LayerProperties &p) const;

  /**
   *  @brief Gets the target layer for a given logical layer
   *
   *  Returns 0 if no target layer mapping is supplied.
   */
  const db::LayerProperties *target (unsigned int l) const;

  /**
   *  @brief String description for the mapping of a logical layer
   *
   *  @return A string describing the mapping (an empty string if
   *  no mapping is present, a map expression (see below) if there
   *  is any mapping.
   */
  std::string mapping_str (unsigned int l) const;

  /** 
   *  @brief LayerProperties describing one mapping of a logical layer
   *
   *  In general, there are more than one LDPairs or names mapped
   *  to one logical layer. This method will return a single one of
   *  them. It will return the least layer and datatype that matches.
   *  It will return LayerProperties() if the layer is not mapped.
   *
   *  @return A LayerProperties giving the target layer
   */
  LayerProperties mapping (unsigned int l) const;

  /**
   *  @brief Get all layers to which a mapping exists
   */
  std::vector<unsigned int> get_layers () const;

  /**
   *  @brief Single-map a physical to a logical layer
   *
   *  "Single-mapping" substitutes a layer mapping. "Multimapping" (mmap_..)
   *  adds to a given mapping and allows generating 1:n mappings (m:n in fact).
   */
  template <class S>
  void map (const S &p, unsigned int l)
  {
    unmap (p);
    mmap (p, l);
  }

  /**
   *  @brief Single-map a physical to a logical layer with a target layer
   */
  template <class S>
  void map (const S &p, unsigned int l, const LayerProperties &t)
  {
    unmap (p);
    mmap (p, l, t);
  }

  /**
   *  @brief Single-map a physical layer interval with a target layer
   */
  void map (const LDPair &p1, const LDPair &p2, unsigned int l)
  {
    unmap (p1, p2);
    mmap (p1, p2, l);
  }

  /**
   *  @brief Single-map a physical layer interval with a target layer
   */
  void map (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &t)
  {
    unmap (p1, p2);
    mmap (p1, p2, l, t);
  }

  /**
   *  @brief Single-map a physical layer interval (given by an expression)
   */
  void map_expr (const std::string &expr, unsigned int l)
  {
    unmap_expr (expr);
    mmap_expr (expr, l);
  }

  /**
   *  @brief Same a map_expr with a string argument but taking the expression for a tl::Extractor
   */
  void map_expr (tl::Extractor &ex, unsigned int l)
  {
    tl::Extractor ex1 = ex;
    unmap_expr (ex1);
    mmap_expr (ex, l);
  }

  /**
   *  @brief Multi-map a ldpair to a logical layer
   *
   *  "Multimapping" will not substitute but add to the mapping.
   */
  void mmap (const LDPair &p, unsigned int l);

  /**
   *  @brief Multi-map a name to a logical layer
   *
   *  "Multimapping" will not substitute but add to the mapping.
   */
  void mmap (const std::string &name, unsigned int l);

  /**
   *  @brief Multi-map a name or LDPair to a logical layer
   *
   *  The algorithm chooses the LDPair from the LayerProperties structure and/or
   *  the name if no LDPair is given. If the source LayerProperties structure does
   *  not specify a valid layer, no mapping is provided.
   *
   *  @param f The source (where to derive the match expression from)
   *  @param l The logical layer to map to the match expression
   */
  void mmap (const LayerProperties &f, unsigned int l);

  /**
   *  @brief Multi-map a ldpair to a logical layer with a target layer
   *
   *  The target layer specifies which layer to create for the 
   *  corresponding input.
   */
  void mmap (const LDPair &p, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Multi-map a name to a logical layer with a target layer
   *
   *  The target layer specifies which layer to create for the 
   *  corresponding input.
   */
  void mmap (const std::string &name, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Multi-map a name or LDPair to a logical layer with a target layer
   *
   *  The algorithm chooses the LDPair from the LayerProperties structure or
   *  the name if no LDPair is given. If the source LayerProperties structure does
   *  not specify a valid layer, no mapping is provided.
   *
   *  @param f The source (where to derive the match expression from)
   *  @param l The logical layer to map to the match expression
   *  @param t The target layer to use for the mapped layer
   */
  void mmap (const LayerProperties &f, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Multi-map a range of ldpair's to a logical layer
   *
   *  The range is given by two pairs p1,p2. The layers
   *  mapped are [p1.l,p2.l], the datatypes mapped are [p1.d,p2.d].
   */
  void mmap (const LDPair &p1, const LDPair &p2, unsigned int l);

  /**
   *  @brief Multi-map a range of ldpair's to a logical layer with a target layer
   *
   *  The range is given by two pairs p1,p2. The layers
   *  mapped are [p1.l,p2.l], the datatypes mapped are [p1.d,p2.d].
   */
  void mmap (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &t);

  /** 
   *  @brief Map a range given by a string expression to a logical layer
   * 
   *  The string expression is constructed using the syntax: 
   *  "list[/list][;..]" for layer/datatype pairs. "list" is a 
   *  sequence of numbers, separated by comma values or a range 
   *  separated by a hyphen. Examples are: "1/2", "1-5/0", "1,2,5/0",
   *  "1/5;5/6".
   *
   *  layer/datatype wildcards can be specified with "*". When "*" is used
   *  for the upper limit, it is equivalent to "all layer above". When used
   *  alone, it is equivalent to "all layers". Examples: "1 / *", "* / 10-*".
   *
   *  Named layers are specified simply by specifying the name, if 
   *  necessary in single or double quotes (if the name begins with a digit or
   *  contains non-word characters). layer/datatype and name descriptions can
   *  be mixed, i.e. "AA;1/5" (meaning: name "AA" or layer 1/datatype 5).
   *
   *  A target layer can be specified with the ":<target>" notation, where
   *  target is a valid string for a LayerProperties() object.
   *
   *  A target can include relative layer/datatype specifications and wildcards.
   *  For example, "1-10/0: *+1/0" will add 1 to the original layer number.
   *  "1-10/0-50: * / *" will use the original layers.
   *
   *  This method will throw a LayerSpecFormatException if
   *  something is wrong with the format string
   */
  void mmap_expr (const std::string &expr, unsigned int l);

  /**
   *  @brief Same a map_expr with a string argument but taking the expression for a tl::Extractor
   */
  void mmap_expr (tl::Extractor &ex, unsigned int l);

  /**
   *  @brief Unmaps a LDPair
   */
  void unmap (const LDPair &f);

  /**
   *  @brief Unmaps the layer with the given name
   */
  void unmap (const std::string &name);

  /**
   *  @brief Unmaps a layer with the given layer properties
   */
  void unmap (const LayerProperties &f);

  /**
   *  @brief Removes any mapping for a range of ldpair's
   *
   *  The range is given by two pairs p1,p2. The layers
   *  between [p1.l,p2.l] and with datatypes between [p1.d,p2.d] are unmapped.
   */
  void unmap (const LDPair &p1, const LDPair &p2);

  /**
   *  @brief Removes any mapping for the layers given by the expression
   */
  void unmap_expr (const std::string &expr);

  /**
   *  @brief Removes any mapping for the layers given by the expression
   */
  void unmap_expr (tl::Extractor &ex);

  /**
   *  @brief Generic expression mapping
   *
   *  This generic mapping function takes a mapping expression. If it starts with "+",
   *  "mmap" is used, if it starts with "-", "unmap" is used. Otherwise, "map" is used.
   */
  void add_expr (const std::string &expr, unsigned int l);

  /**
   *  @brief Generic expression mapping
   *
   *  This generic mapping function takes a mapping expression. If it starts with "+",
   *  "mmap" is used, if it starts with "-", "unmap" is used. Otherwise, "map" is used.
   */
  void add_expr (tl::Extractor &ex, unsigned int l);

  /**
   *  @brief Prepares a layer mapping object for reading
   *
   *  This replaces all layer indexes by ones from the layout or it will create new layers
   *  if required. Note that for relative and wildcard targets (except '* / *'), the layer
   *  indexes will not be the true indexes but placeholders. They will be replaced later
   *  when calling logical with a layout argument.
   */
  void prepare (db::Layout &layout);

  /**
   *  @brief Get the next available index
   */
  unsigned int next_index () const
  {
    return m_next_index;
  }

  /**
   *  @brief Clear the map
   */
  void clear ();

  /**
   *  @brief Gets a value indicating whether the map is empty
   */
  bool is_empty () const;

  /**
   *  @brief Get the iterator for the mapping (begin)
   */
  const_iterator_layers begin () const
  {
    return m_ld_map.begin ();
  }

  /**
   *  @brief Get the iterator for the mapping (end)
   */
  const_iterator_layers end () const
  {
    return m_ld_map.end ();
  }

  /**
   *  @brief Get the iterator for the name mapping (begin)
   */
  const_iterator_names begin_name_mapping () const
  {
    return m_name_map.begin ();
  }

  /**
   *  @brief Get the iterator for the name mapping (end)
   */
  const_iterator_names end_name_mapping () const
  {
    return m_name_map.end ();
  }

  /**
   *  @brief Convert the layer mapping to a string
   */
  std::string to_string () const;

  /**
   *  @brief Convert a layer map to a string in the layer mapping file's format
   *  See "from_string_file_format" for details.
   */
  std::string to_string_file_format () const;

  /**
   *  @brief Read a layer mapping from a one-entry-per-line file format
   *
   *  The format is one expression per line (see map_expr):
   *    <exp> [#comment|//comment]
   *  The layer indexes are assigned incrementally starting with 0.
   *  Use "prepare" to assign real indexes for an existing layout.
   */
  static LayerMap from_string_file_format (const std::string &s);

private:
  ld_map m_ld_map;
  std::map<std::string, std::set<unsigned int> > m_name_map;
  std::map<unsigned int, LayerProperties> m_target_layers;
  std::vector<LayerProperties> m_placeholders;
  unsigned int m_next_index;

  void insert (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties *t);
  void insert (const std::string &name, unsigned int l, const LayerProperties *t);

  std::set<unsigned int> logical_internal (const LDPair &p, bool allow_placeholder) const;
  std::set<unsigned int> logical_internal (const std::string &name, bool allow_placeholder) const;
  std::set<unsigned int> logical_internal (const db::LayerProperties &p, bool allow_placeholder) const;

  std::set<unsigned int> substitute_placeholder (const db::LayerProperties &p, const std::set<unsigned int> &ph, db::Layout &layout);
  bool is_placeholder (const std::set<unsigned int> &l) const;
};

}

/**
 *  @brief Special extractors for the layer map
 */
namespace tl
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::LayerMap &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::LayerMap &t);
} // namespace tl

#endif

