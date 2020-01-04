
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
  typedef tl::interval_map<ld_type, unsigned int> datatype_map;
  typedef tl::interval_map<ld_type, datatype_map> ld_map;
  typedef ld_map::const_iterator const_iterator_layers;
  typedef datatype_map::const_iterator const_iterator_datatypes;
  typedef std::map<std::string, unsigned int>::const_iterator const_iterator_names;

  /**
   *  @brief The constructor for an empty map
   */
  LayerMap ();

  /** 
   *  @brief Query a layer mapping
   *
   *  @return A pair telling if the layer is mapped (first=true) and
   *  the logical layer mapped (second) if this is the case.
   */
  std::pair<bool, unsigned int> logical (const LDPair &p) const;

  /** 
   *  @brief Query a layer mapping from a name
   *
   *  @return A pair telling if the layer is mapped (first=true) and
   *  the logical layer mapped (second) if this is the case.
   */
  std::pair<bool, unsigned int> logical (const std::string &name) const;

  /** 
   *  @brief Query a layer mapping from a name or LDPair
   *
   *  @return A pair telling if the layer is mapped (first=true) and
   *  the logical layer mapped (second) if this is the case.
   *
   *  @param p The layer that is looked for
   */
  std::pair<bool, unsigned int> logical (const db::LayerProperties &p) const;

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
   *  @brief Map a ldpair to a logical layer 
   */
  void map (const LDPair &p, unsigned int l);

  /**
   *  @brief Map a name to a logical layer 
   */
  void map (const std::string &name, unsigned int l);

  /**
   *  @brief Map a name or LDPair to a logical layer 
   *
   *  The algorithm chooses the LDPair from the LayerProperties structure and/or
   *  the name if no LDPair is given. If the source LayerProperties structure does
   *  not specify a valid layer, no mapping is provided.
   *
   *  @param f The source (where to derive the match expression from)
   *  @param l The logical layer to map to the match expression
   */
  void map (const LayerProperties &f, unsigned int l);

  /**
   *  @brief Map a ldpair to a logical layer with a target layer
   *
   *  The target layer specifies which layer to create for the 
   *  corresponding input.
   */
  void map (const LDPair &p, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Map a name to a logical layer with a target layer
   *
   *  The target layer specifies which layer to create for the 
   *  corresponding input.
   */
  void map (const std::string &name, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Map a name or LDPair to a logical layer with a target layer
   *
   *  The algorithm chooses the LDPair from the LayerProperties structure or
   *  the name if no LDPair is given. If the source LayerProperties structure does
   *  not specify a valid layer, no mapping is provided.
   *
   *  @param f The source (where to derive the match expression from)
   *  @param l The logical layer to map to the match expression
   *  @param t The target layer to use for the mapped layer
   */
  void map (const LayerProperties &f, unsigned int l, const LayerProperties &t);

  /**
   *  @brief Map a range of ldpair's to a logical layer
   *
   *  The range is given by two pairs p1,p2. The layers
   *  mapped are [p1.l,p2.l], the datatypes mapped are [p1.d,p2.d].
   */
  void map (const LDPair &p1, const LDPair &p2, unsigned int l);

  /**
   *  @brief Map a range of ldpair's to a logical layer with a target layer
   *
   *  The range is given by two pairs p1,p2. The layers
   *  mapped are [p1.l,p2.l], the datatypes mapped are [p1.d,p2.d].
   */
  void map (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &t);

  /** 
   *  @brief Map a range given by a string expression to a logical layer
   * 
   *  The string expression is constructed using the syntax: 
   *  "list[/list][;..]" for layer/datatype pairs. "list" is a 
   *  sequence of numbers, separated by comma values or a range 
   *  separated by a hyphen. Examples are: "1/2", "1-5/0", "1,2,5/0",
   *  "1/5;5/6".
   *  Named layers are specified simply by specifying the name, if 
   *  necessary in single or double quotes (if the name begins with a digit or
   *  contains non-word characters). layer/datatype and name descriptions can
   *  be mixed, i.e. "AA;1/5" (meaning: name "AA" or layer 1/datatype 5).
   *  A target layer can be specified with the ":<target>" notation, where
   *  target is a valid string for a LayerProperties() object.
   *  This method will throw a LayerSpecFormatException if
   *  something is wrong with the format string
   */
  void map_expr (const std::string &expr, unsigned int l);

  /**
   *  @brief Same a map_expr with a string argument but taking the expression for a tl::Extractor
   */
  void map_expr (tl::Extractor &ex, unsigned int l);

  /**
   *  @brief Prepares a layer mapping object for reading
   *
   *  This replaces all layer indexes by ones either in the layout or it will create new layers
   *  in the layout
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
   *    <exp> [#commment|//comment]
   *  The layer indexes are assigned incrementally starting with 0.
   *  Use "prepare" to assign real indexes for an existing layout.
   */
  static LayerMap from_string_file_format (const std::string &s);

private:
  ld_map m_ld_map;
  std::map<std::string, unsigned int> m_name_map;
  std::map<unsigned int, LayerProperties> m_target_layers;
  unsigned int m_next_index;

  void insert (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &t);
  void insert (const std::string &name, unsigned int l, const LayerProperties &t);
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

