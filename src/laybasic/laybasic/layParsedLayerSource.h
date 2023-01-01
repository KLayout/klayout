
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


#ifndef HDR_layParsedLayerSource
#define HDR_layParsedLayerSource

#include "laybasicCommon.h"

#include <string>
#include <set>

#include "tlString.h"
#include "dbTrans.h"
#include "dbPropertiesRepository.h"

namespace db
{
  class Layout;
  struct LayerProperties;
}

namespace lay 
{

class LayerProperties;
class LayoutViewBase;
class PropertySelectorBase;

/**
 *  @brief A structure describing a hierarchy level display specification
 */
class LAYBASIC_PUBLIC HierarchyLevelSelection
{
public:
  /**
   *  @brief Describes the mode how to use a hierarchy level spec
   *  
   *  absolute - use the value as it is
   *  minimum - use the minimum of the set level (in the level controls) and the value
   *  maximum - use the maximum of the set level (in the level controls) and the value
   */
  enum level_mode_type { absolute = 0, minimum = 1, maximum = 2 };

  /**
   *  @brief Default constructor
   *
   *  This will create a "neutral" hierarchy level specification without any 
   *  particular selection.
   */
  HierarchyLevelSelection ()
    : m_has_from_level (false), m_from_level_relative (false), m_from_level (0), m_from_mode (absolute),
      m_has_to_level (false), m_to_level_relative (false), m_to_level (0), m_to_mode (absolute)
  { }

  /**
   *  @brief Constructor creating a constrained specification
   *
   *  This constructor will create a specification that has a "from" and a "to"
   *  level.
   */
  HierarchyLevelSelection (int from_level, bool from_level_relative, level_mode_type from_mode, int to_level, bool to_level_relative, level_mode_type to_mode)
    : m_has_from_level (true), m_from_level_relative (from_level_relative), m_from_level (from_level), m_from_mode (from_mode),
      m_has_to_level (true), m_to_level_relative (to_level_relative), m_to_level (to_level), m_to_mode (to_mode)
  { }

  /**
   *  @brief Combine two hierarchy level specifications
   *
   *  The resulting specification will have "b" specifications overridden, if 
   *  there are levels specified in "*this".
   */
  HierarchyLevelSelection combine (const HierarchyLevelSelection &b) const
  {
    HierarchyLevelSelection s (b);
    if (m_has_from_level) {
      s.set_from_level (m_from_level, m_from_level_relative, m_from_mode);
    }
    if (m_has_to_level) {
      s.set_to_level (m_to_level, m_to_level_relative, m_to_mode);
    }
    return s;
  }

  /**
   *  @brief Tell, if we have a "from_level" specification
   */
  bool has_from_level () const
  {
    return m_has_from_level;
  }

  /**
   *  @brief Return the "from_level"
   */
  int from_level (int context_levels, int from_level_set) const
  {
    int l = m_from_level_relative ? m_from_level + context_levels : m_from_level;
    if (m_from_mode == minimum) {
      return std::min (l, from_level_set);
    } else if (m_from_mode == maximum) {
      return std::max (l, from_level_set);
    } else {
      return l;
    }
  }

  /**
   *  @brief Return the "from_level" 
   */
  int from_level () const
  {
    return m_from_level;
  }

  /**
   *  @brief Return the "from_level_relative"  flag
   */
  bool from_level_relative () const
  {
    return m_from_level_relative;
  }

  /**
   *  @brief Return the "from_level_mode"
   */
  level_mode_type from_level_mode () const
  {
    return m_from_mode;
  }

  /**
   *  @brief Set the "from_level" with relative flag and mode
   */
  void set_from_level (int from_level, bool relative, level_mode_type mode)
  {
    m_from_level = from_level;
    m_from_level_relative = relative;
    m_from_mode = mode;
    m_has_from_level = true;
  }

  /**
   *  @brief Clear the "from_level"
   *
   *  "has_from_level" will return false after the "from_level" was set.
   */
  void clear_from_level ()
  {
    m_has_from_level = false;
  }

  /**
   *  @brief Tell, if we have a "to_level" specification
   */
  bool has_to_level () const
  {
    return m_has_to_level;
  }

  /**
   *  @brief Return the "to_level"
   */
  int to_level (int context_levels, int to_level_set) const
  {
    int l = m_to_level_relative ? m_to_level + context_levels : m_to_level;
    if (m_to_mode == minimum) {
      return std::min (l, to_level_set);
    } else if (m_to_mode == maximum) {
      return std::max (l, to_level_set);
    } else {
      return l;
    }
  }

  /**
   *  @brief Return the "to_level" 
   */
  int to_level () const
  {
    return m_to_level;
  }

  /**
   *  @brief Return the "to_level_relative"  flag
   */
  bool to_level_relative () const
  {
    return m_to_level_relative;
  }

  /**
   *  @brief Return the "to_level_mode"
   */
  level_mode_type to_level_mode () const
  {
    return m_to_mode;
  }

  /**
   *  @brief Set the "to_level" with relative flag and mode
   */
  void set_to_level (int to_level, bool relative, level_mode_type mode)
  {
    m_to_level = to_level;
    m_to_level_relative = relative;
    m_to_mode = mode;
    m_has_to_level = true;
  }

  /**
   *  @brief Clear the "to_level"
   *
   *  "has_to_level" will return false after the "to_level" was set.
   */
  void clear_to_level ()
  {
    m_has_to_level = false;
  }


  /**
   *  @brief Equality
   */
  bool operator== (const HierarchyLevelSelection &b) const
  {
    return m_has_from_level == b.m_has_from_level && 
           (! m_has_from_level || 
             (m_from_level_relative == b.m_from_level_relative &&
             m_from_level == b.m_from_level &&
             m_from_mode == b.m_from_mode)) &&
           m_has_to_level == b.m_has_to_level &&
           (! m_has_to_level || 
             (m_to_level_relative == b.m_to_level_relative &&
             m_to_level == b.m_to_level && 
             m_to_mode == b.m_to_mode));
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const HierarchyLevelSelection &b) const
  {
    return ! operator== (b);
  }

  /**
   *  @brief Comparison
   */
  bool operator< (const HierarchyLevelSelection &b) const
  {
    if (m_has_from_level != b.m_has_from_level) {
      return m_has_from_level < b.m_has_from_level;
    }
    if (m_has_from_level) {
      if (m_from_level_relative != b.m_from_level_relative) {
        return m_from_level_relative < b.m_from_level_relative;
      }
      if (m_from_level != b.m_from_level) {
        return m_from_level < b.m_from_level;
      }
      if (m_from_mode != b.m_from_mode) {
        return m_from_mode < b.m_from_mode;
      }
    }
    if (m_has_to_level != b.m_has_to_level) {
      return m_has_to_level < b.m_has_to_level;
    }
    if (m_has_to_level) {
      if (m_to_level_relative != b.m_to_level_relative) {
        return m_to_level_relative < b.m_to_level_relative;
      }
      if (m_to_level != b.m_to_level) {
        return m_to_level < b.m_to_level;
      }
      if (m_to_mode != b.m_to_mode) {
        return m_to_mode < b.m_to_mode;
      }
    }
    return false;
  }

private:
  bool m_has_from_level;
  bool m_from_level_relative;
  int m_from_level;
  level_mode_type m_from_mode;
  bool m_has_to_level;
  bool m_to_level_relative;
  int m_to_level;
  level_mode_type m_to_mode;
};

/**
 *  @brief An object tracking whether a cell is selected while traversing a tree
 */
class LAYBASIC_PUBLIC PartialTreeSelector
{
public:
  /**
   *  @brief Default constructor
   */
  PartialTreeSelector ();

  /**
   *  @brief Copy constructor
   */
  PartialTreeSelector (const PartialTreeSelector &ts);

  /**
   *  @brief Assignment
   */
  PartialTreeSelector &operator= (const PartialTreeSelector &ts);

  /**
   *  @brief Returns the selected status of the given child cell from the current cell
   *  The return value indicates whether the given child cell is contained in the selected
   *  set.
   *  The value is:
   *    -1:  The cell is not selected but one indirect child may be
   *    0:   The cell is not selected and no child of it will be
   *    1:   The cell is selected, their children may be selected
   */
  int is_child_selected (db::cell_index_type child) const;

  /**
   *  @brief Returns true, if the current cell is selected
   */
  bool is_selected () const
  {
    return m_selected;
  }

  /**
   *  @brief Descend into the given child cell
   */
  void descend (db::cell_index_type child);

  /**
   *  @brief Ascends to the parent we came from with descend
   */
  void ascend ();

private:
  const db::Layout *mp_layout;
  int m_state;
  bool m_selected;
  std::vector <int> m_state_stack;
  std::vector <bool> m_selected_stack;
  std::vector <std::map <db::cell_index_type, std::pair<int, int> > > m_state_machine;
  friend class CellSelector;

  /**
   *  @brief Constructor
   */
  PartialTreeSelector (const db::Layout &layout, bool initially_selected);

  /**
   *  @brief Adds a cell index to a given state with a target state and a selection state
   *  The target state may be -1 indicating the decision is final. 
   *  The selected flag can be -1 (no change), 0 (deselect) and 1 (select).
   */
  void add_state_transition (int initial_state, db::cell_index_type cell_index, int target_state, int selected);

  /**
   *  @brief Adds all cells to a given state with a target state and a selection state
   *  The target state may be -1 indicating the decision is final. 
   *  This transition will be performed for every cell.
   *  The selected flag can be -1 (no change), 0 (deselect) and 1 (select).
   */
  void add_state_transition (int initial_state, int target_state, int selected);
};

/**
 *  @brief A structure describing a cell selection through the hierarchy
 */
class LAYBASIC_PUBLIC CellSelector
{
public:
  /**
   *  @brief Constructor
   *  Creates an empty cell selector.
   */
  CellSelector ();

  /**
   *  @brief Copy constructor
   */
  CellSelector (const CellSelector &d);

  /**
   *  @brief Assignment 
   */
  CellSelector &operator= (const CellSelector &d);

  /**
   *  @brief Equality
   */
  bool operator== (const CellSelector &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellSelector &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const CellSelector &d) const;

  /**
   *  @brief Parse one additional selector contribution from the extractor
   *  This method can be called multiple times which will add another 
   *  contribution to the selector.
   */
  void parse (tl::Extractor &ex);

  /**
   *  @brief Converts the selector to a string
   */
  std::string to_string () const;

  /**
   *  @brief Creates a partial tree selector object 
   *  This object is the actual selector which can be used to determine whether a 
   *  cell is selected while traversing the tree.
   */
  PartialTreeSelector create_tree_selector (const db::Layout &layout, db::cell_index_type initial_cell) const;

  /**
   *  @brief Returns true, if the selector selects all
   */
  bool is_empty () const
  {
    return m_selectors.empty ();
  }

private:
  std::vector <std::vector <std::pair <bool, std::string> > > m_selectors;
};

/**
 *  @brief A property selector
 */
class LAYBASIC_PUBLIC PropertySelector
{
public:
  /**
   *  @brief Constructor
   *
   *  This constructor creates an empty selector matching everything. 
   */
  PropertySelector ();

  /**
   *  @brief Copy Constructor
   */
  PropertySelector (const PropertySelector &sel);

  /**
   *  @brief Destructor
   */
  ~PropertySelector ();

  /**
   *  @brief Assignment 
   */
  PropertySelector &operator= (const PropertySelector &sel);

  /**
   *  @brief Equality 
   */
  bool operator== (const PropertySelector &sel) const;

  /**
   *  @brief Inequality 
   */
  bool operator!= (const PropertySelector &sel) const
  {
    return !operator== (sel);
  }

  /**
   *  @brief Less than 
   */
  bool operator< (const PropertySelector &sel) const;

  /**
   *  @brief Extractor: get from a string
   *
   *  This method may throw an exception if the string is not
   *  a valid property selector expression.
   */
  void extract (tl::Extractor &ex);

  /**
   *  @brief Convert to a string
   */
  std::string to_string (size_t max_len) const;

  /**
   *  @brief Join with another property selector
   *
   *  The selectors will be combined to for "A&&B" property selection.
   */
  void join (const PropertySelector &d);

  /**
   *  @brief Check, if the selector applies to a given property set
   *
   *  The given properties set is checked against the selector and "true" is returned
   *  if the selector applies to the given set.
   */
  bool check (const db::PropertiesRepository &rep, db::properties_id_type id) const;

  /**
   *  @brief Obtain a list of properties ids that satisfy the selection
   *
   *  A set of properties ids is determined that each satisfies the given 
   *  selection. The set should be empty on enter. Otherwise the results are
   *  not defined.
   *  The return value is true if the ids are to be interpreted inversely (every properties id
   *  matches that is not in the set). This allows optimizing the computation of the set.
   */
  bool matching (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &matching) const;

  /**
   *  @brief Return true, if the property selector is not set
   */
  bool is_null () const
  {
    return mp_base == 0;
  }

private:
  PropertySelectorBase *mp_base;
};

/**
 *  @brief Representation of a "parsed" layer source specification
 *
 *  This class implements a parser, a container and a dumper for a layer source 
 *  specification. Such a specification consists of a layer, datatype and 
 *  cellview index currently.
 *  The parser layer source specification implements a "less than" operator for
 *  easy comparison.
 */
class LAYBASIC_PUBLIC ParsedLayerSource
{
public:
  /**
   *  @brief Enumeration for the special purpose layers
   */
  enum SpecialPurpose { SP_None = 0, SP_CellFrame };

  /**
   *  @brief Direct constructor: create from a layer, a datatype and a cellview index
   *
   *  If the cellview index is less than zero, it is "any" or "invalid".
   */
  ParsedLayerSource (int layer, int datatype, int cv_index);

  /**
   *  @brief Direct constructor: create from a layer index and a cellview index
   *
   *  If the cellview index is less than zero, it is "any" or "invalid".
   */
  ParsedLayerSource (int layer_index, int cv_index);

  /**
   *  @brief Direct constructor: create from a named layer and a cellview index
   *
   *  If the cellview index is less than zero, it is "any" or "invalid".
   */
  ParsedLayerSource (const std::string &name, int cv_index);

  /**
   *  @brief Direct constructor: create from a db::LayerProperties object
   *
   *  If the cellview index is less than zero, it is "any" or "invalid".
   */
  ParsedLayerSource (const db::LayerProperties &lp, int cv_index);

  /**
   *  @brief Construct a parsed layer source from a string
   *
   *  This constructor may throw an exception if the string is not a valid
   *  source representation.
   */
  ParsedLayerSource (const std::string &src);

  /**
   *  @brief Copy ctor
   */
  ParsedLayerSource (const ParsedLayerSource &d);

  /**
   *  @brief Default ctor
   */
  ParsedLayerSource ();

  /**
   *  @brief Assignment
   */
  ParsedLayerSource &operator= (const ParsedLayerSource &d);

  /**
   *  @brief Conversion to a string
   *
   *  This method delivers a string that can be converted back to a valid
   *  ParsedLayerSource object.
   */
  std::string to_string () const;

  /**
   *  @brief Conversion to a string
   *
   *  This method delivers a display version that may be abbreviated and is supposed
   *  to be used in the layer list display.
   */
  std::string display_string (const lay::LayoutViewBase *view) const;

  /**
   *  @brief Comparison (equality)
   */
  bool operator== (const ParsedLayerSource &d) const; 

  /**
   *  @brief Comparison (inequality)
   */
  bool operator!= (const ParsedLayerSource &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Comparison (less than)
   */
  bool operator< (const ParsedLayerSource &d) const; 

  /**
   *  @brief Gets the color order index
   *
   *  The color order index is some "characteristic number" used to derive a default color
   *  for the layer.
   */
  unsigned int color_index () const;

  /**
   *  @brief Read accessor to the cellview index
   */
  int cv_index () const
  {
    return m_cv_index;
  }
  
  /**
   *  @brief Write accessor to the cellview index
   */
  ParsedLayerSource &cv_index (int cvi) 
  {
    m_cv_index = cvi;
    return *this;
  }

  /**
   *  @brief Read accessor to the "has_name" property
   */
  bool has_name () const
  {
    return m_has_name;
  }
  
  /**
   *  @brief Read accessor to the "name" property
   */
  const std::string &name () const
  {
    return m_name;
  }
  
  /**
   *  @brief Write accessor to the "name" property
   */
  ParsedLayerSource &name (const std::string &n) 
  {
    m_has_name = !n.empty ();
    m_name = n;
    return *this;
  }
  
  /**
   *  @brief Reset the name property
   */
  ParsedLayerSource &clear_name ()
  {
    m_has_name = false;
    return *this;
  }
  
  /**
   *  @brief Read accessor to the "layer_index" property
   */
  int layer_index () const
  {
    return m_layer_index;
  }
  
  /**
   *  @brief Write accessor to the "layer_index" property
   */
  ParsedLayerSource &layer_index (int layer_index) 
  {
    m_layer_index = layer_index;
    return *this;
  }
  
  /**
   *  @brief Read accessor to the "layer" property
   */
  int layer () const
  {
    return m_layer;
  }
  
  /**
   *  @brief Write accessor to the "layer" property
   */
  ParsedLayerSource &layer (int layer) 
  {
    m_layer = layer;
    return *this;
  }
  
  /**
   *  @brief Read accessor to the "datatype" property
   */
  int datatype () const
  {
    return m_datatype;
  }

  /**
   *  @brief Write accessor to the "datatype" property
   */
  ParsedLayerSource &datatype (int datatype) 
  {
    m_datatype = datatype;
    return *this;
  }
  
  /**
   *  @brief Read accessor to the transformation 
   *
   *  The transformation is in micron units
   */
  const std::vector<db::DCplxTrans> &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Set the transformation
   */
  void set_trans (const std::vector<db::DCplxTrans> &t)
  {
    m_trans = t;
  }

  /**
   *  @brief Get the special purpose code
   */
  SpecialPurpose special_purpose () const
  {
    return m_special_purpose;
  }

  /**
   *  @brief Set the special purpose code
   */
  void set_special_purpose (SpecialPurpose sp) 
  {
    m_special_purpose = sp;
  }

  /**
   *  @brief Gets the cell selector
   */
  const CellSelector &cell_selector () const
  {
    return m_cell_sel;
  }

  /**
   *  @brief Sets the cell selector
   */
  void set_cell_selector (const CellSelector &cs) 
  {
    m_cell_sel = cs;
  }

  /**
   *  @brief The hierarchy level specification
   *
   *  The returned structure describes if and which hierarchy level display selector is set.
   */
  HierarchyLevelSelection hier_levels () const
  {
    return m_hier_levels;
  }

  /**
   *  @brief Set the hierarchy level specification
   */
  void set_hier_levels (const HierarchyLevelSelection &hl)
  {
    m_hier_levels = hl;
  }

  /**
   *  @brief Read accessor to the property expression
   *
   *  The property expression selects a shape subset by a property selection
   *  expression. 
   */
  const PropertySelector &property_selector () const
  {
    return m_property_sel;
  }

  /**
   *  @brief Set the property expression
   *
   *  The ownership of the pointer is transferred to the ParsedLayerSource object 
   *  and is deleted by it.
   */
  void set_property_selector (const PropertySelector &sel)
  {
    m_property_sel = sel;
  }

  /**
   *  @brief Concatenate two source specifications
   */
  ParsedLayerSource &operator+= (const ParsedLayerSource &d);
  
  /**
   *  @brief Return a db::LayerProperties object that would match this
   */
  db::LayerProperties layer_props () const;

  /**
   *  @brief Match against a db::LayerProperties structure
   *
   *  Returns true, if the layer source matches a specified db::LayerProperties
   *  object. This is the case if either the name matches (and the layer and
   *  datatype are wildcarded) or layer and datatype matches (and no name is
   *  used).
   */
  bool match (const db::LayerProperties &lp) const;

  /**
   *  @brief Test, if this is a wildcard layer specification
   *
   *  Returns true, if the layer is a wildcard, but the cellview index is specified (* / * @n).
   *  Such layers describe cell frame layers for the redraw thread for example.
   */
  bool is_wildcard_layer () const;

private:
  bool m_has_name;
  SpecialPurpose m_special_purpose;
  int m_layer_index;
  int m_layer;
  int m_datatype;
  std::string m_name;
  int m_cv_index;
  std::vector<db::DCplxTrans> m_trans;
  CellSelector m_cell_sel;
  PropertySelector m_property_sel;
  HierarchyLevelSelection m_hier_levels;

  void parse_from_string (const char *cp);
};

}

#endif

