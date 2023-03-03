
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


#ifndef HDR_layLayerProperties
#define HDR_layLayerProperties

#include "laybasicCommon.h"

#include "dbBox.h"

#include "layViewOp.h"
#include "layParsedLayerSource.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "tlStableVector.h"
#include "tlXMLParser.h"
#include "tlObject.h"

#include "gsi.h"

#include <vector>
#include <string>
#include <iostream>

namespace tl {
  class XMLSource;
}

namespace lay {

class LayoutViewBase;
class LayerPropertiesList;
class LayerPropertiesNode;

/**
 *  @brief A layer properties structure
 *
 *  The layer properties encapsulate the settings relevant for
 *  the display and source of a layer. 
 *
 *  Each attribute is present in two incarnations: local and real.
 *  "real" refers to the effective attribute after collecting the 
 *  attributes from the parents to the leaf property node.
 *  The "real" attributes are computed when the property tree is
 *  "realized". In the spirit of this distinction, all read accessors
 *  are present in "local" and "real" form. The read accessors take
 *  a boolean parameter "real" that must be set to true, if the real
 *  value shall be returned.
 *
 *  The source is specified in two ways: once in "source" and once
 *  in an internal representation that can be used by the drawing engine.
 *  The "realize" method converts the generic into the internal 
 *  representation.
 *
 *  "brightness" is a index that indicates how much to make the
 *  color brighter to darker rendering the effective color 
 *  (eff_frame_color (), eff_fill_color ()). It's value is roughly between
 *  -255 and 255.
 */
class LAYBASIC_PUBLIC LayerProperties
  : public gsi::ObjectBase
{
public:
  /**
   *  @brief Constructor
   */
  LayerProperties ();

  /**
   *  @brief Destructor
   */
  virtual ~LayerProperties ();

  /**
   *  @brief Copy constructor
   */
  LayerProperties (const LayerProperties &d);

  /**
   *  @brief assignment
   */
  LayerProperties &operator= (const LayerProperties &d);

  /**
   *  @brief Gets the generation number
   *
   *  The generation number changes whenever something is changed
   *  with this layer properties object. "0" is reserved for the
   *  "uninitialized" state.
   */
  size_t gen_id () const
  {
    return m_gen_id;
  }

  /**
   *  @brief Assignment alias for GSI binding
   */
  void assign_lp (const LayerProperties &d)
  {
    operator= (d);
  }

  /**
   *  @brief Equality 
   */
  bool operator== (const LayerProperties &d) const;
  
  /**
   *  @brief Inequality 
   */
  bool operator!= (const LayerProperties &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Utility: compute the effective color from a color with brightness correction
   */
  static tl::color_t brighter (tl::color_t in, int b);

  /**
   *  @brief render the effective frame color 
   *  
   *  The effective frame color is computed from the frame color brightness and the
   *  frame color.
   */
  tl::color_t eff_frame_color (bool real) const;

  /**
   *  @brief render the effective fill color
   *  
   *  The effective fill color is computed from the frame color brightness and the
   *  frame color.
   */
  tl::color_t eff_fill_color (bool real) const;

  /**
   *  @brief render the effective frame color plus an additional brightness adjustment
   *  
   *  This method returns the effective frame color with an additional brightness adjustment 
   *  applied.
   */
  tl::color_t eff_frame_color_brighter (bool real, int plus_brightness) const;

  /**
   *  @brief render the effective frame color plus an additional brightness adjustment
   *  
   *  This method returns the effective fill color with an additional brightness adjustment 
   *  applied.
   */
  tl::color_t eff_fill_color_brighter (bool real, int plus_brightness) const;

  /**
   *  @brief Get the frame color
   *
   *  This method may return an invalid color if the color is not set.
   */
  tl::color_t frame_color (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_frame_color_real;
    } else {
      refresh ();
      return m_frame_color;
    }
  }
  
  /**
   *  @brief Set the frame color code to the given value
   *
   *  To clear the frame color, pass 0 to this method. Valid colors have the
   *  upper 8 bits set.
   */
  void set_frame_color_code (tl::color_t c)
  {
    refresh ();
    if (m_frame_color != c) {
      m_frame_color = c;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Set the frame color to the given value
   */
  void set_frame_color (tl::color_t c)
  {
    set_frame_color_code (c | 0xff000000);
  }

  /**
   *  @brief Reset the frame color 
   */
  void clear_frame_color ()
  {
    set_frame_color_code (0);
  }

  /**
   *  @brief Test, if the frame color is set
   */
  bool has_frame_color (bool real) const
  {
    return frame_color (real) != 0;
  }

  /**
   *  @brief Get the fill color
   *
   *  This method may return an invalid color if the color is not set.
   */
  tl::color_t fill_color (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_fill_color_real;
    } else {
      refresh ();
      return m_fill_color;
    }
  }
  
  /**
   *  @brief Set the fill color code to the given value
   *
   *  To clear the fill color, pass 0 to this method. Valid colors have the
   *  upper 8 bits set.
   */
  void set_fill_color_code (tl::color_t c)
  {
    refresh ();
    if (m_fill_color != c) {
      m_fill_color = c;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Set the fill color to the given value
   */
  void set_fill_color (tl::color_t c)
  {
    set_fill_color_code (c | 0xff000000);
  }
  
  /**
   *  @brief Reset the fill color
   */
  void clear_fill_color ()
  {
    set_fill_color_code (0);
  }
  
  /**
   *  @brief Test, if the frame color is set
   */
  bool has_fill_color (bool real) const
  {
    return fill_color (real) != 0;
  }
  
  /**
   *  @brief Set the frame brightness
   *  
   *  For neutral brightness set this value to 0.
   */
  void set_frame_brightness (int b)
  {
    refresh ();
    if (m_frame_brightness != b) {
      m_frame_brightness = b;
      need_realize (nr_visual);
    }
  }

  /**
   *  @brief Get the frame brightness value
   */
  int frame_brightness (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_frame_brightness_real;
    } else {
      refresh ();
      return m_frame_brightness;
    }
  }
    
  /**
   *  @brief Set the fill brightness
   *  
   *  For neutral brightness set this value to 0.
   */
  void set_fill_brightness (int b)
  {
    refresh ();
    if (m_fill_brightness != b) {
      m_fill_brightness = b;
      need_realize (nr_visual);
    }
  }

  /**
   *  @brief Get the fill brightness value
   * 
   *  If the brightness is not set, this method may return an invalid value
   */
  int fill_brightness (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_fill_brightness_real;
    } else {
      refresh ();
      return m_fill_brightness;
    }
  }
    
  /**
   *  @brief Set the dither pattern index
   */
  void set_dither_pattern (int index)
  {
    refresh ();
    if (m_dither_pattern != index) {
      m_dither_pattern = index;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the effective dither pattern index
   */
  unsigned int eff_dither_pattern (bool real) const
  {
    if (! has_dither_pattern (real)) {
      return 1; // empty fill pattern
    } else {
      return (unsigned int) dither_pattern (real);
    }
  }
  
  /**
   *  @brief Get the dither pattern index
   *  
   *  This method may deliver an invalid dither pattern index if it is not set.
   */
  int dither_pattern (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_dither_pattern_real;
    } else {
      refresh ();
      return m_dither_pattern;
    }
  }
  
  /**
   *  @brief Clear the dither pattern
   */
  void clear_dither_pattern ()
  {
    set_dither_pattern (-1);
  }
  
  /**
   *  @brief Test, if the dither pattern is set
   */
  bool has_dither_pattern (bool real) const
  {
    return dither_pattern (real) >= 0;
  }
  
  /**
   *  @brief Set the line style index
   */
  void set_line_style (int index)
  {
    refresh ();
    if (m_line_style != index) {
      m_line_style = index;
      need_realize (nr_visual);
    }
  }

  /**
   *  @brief Get the effective line style index
   */
  unsigned int eff_line_style (bool real) const
  {
    if (! has_line_style (real)) {
      return 0; // solid line style
    } else {
      return (unsigned int) line_style (real);
    }
  }

  /**
   *  @brief Get the line style index
   *
   *  This method may deliver an invalid line style index if it is not set.
   */
  int line_style (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_line_style_real;
    } else {
      refresh ();
      return m_line_style;
    }
  }

  /**
   *  @brief Clear the line style
   */
  void clear_line_style ()
  {
    set_line_style (-1);
  }

  /**
   *  @brief Test, if the line style is set
   */
  bool has_line_style (bool real) const
  {
    return line_style (real) >= 0;
  }

  /**
   *  @brief Set the validity state
   */
  void set_valid (bool v)
  {
    refresh ();
    if (m_valid != v) {
      m_valid = v;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the validity state
   */
  bool valid (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_valid_real;
    } else {
      refresh ();
      return m_valid;
    }
  }

  /**
   *  @brief Set the visibility state
   */
  void set_visible (bool v)
  {
    refresh ();
    if (m_visible != v) {
      m_visible = v;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the visibility state
   */
  bool visible (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_visible_real;
    } else {
      refresh ();
      return m_visible;
    }
  }

  /**
   *  @brief Returns true, if the layer is showing "something"
   *
   *  A layer "shows something" if it is visible and it displays some information,
   *  either shapes or cell boxes. Invalid layers, i.e. such that have a layer selection
   *  which does not match one of the layers in the layout or invalid layers, are not considered "visual".
   *
   *  Only "visual" layers are considered for selection, object snapping etc.  
   *
   *  This method evaluates the state in "real" mode.
   */
  bool is_visual () const;

  /**
   *  @brief shape layer attribute
   *
   *  This attribute is true, if the layer provides shapes and is a valid layer.
   */
  bool is_shape_layer () const
  {
    return layer_index () >= 0;
  }
  
  /**
   *  @brief standard layer attribute
   *
   *  This attribute is true, if the layer provides layout shapes.
   */
  bool is_standard_layer () const
  {
    return source (true).special_purpose () == lay::ParsedLayerSource::SP_None;
  }
  
  /**
   *  @brief cell_box layer attribute
   *
   *  This attribute is true, if the layer provides cell boxes.
   */
  bool is_cell_box_layer () const
  {
    return source (true).special_purpose () == lay::ParsedLayerSource::SP_CellFrame;
  }
  
  /**
   *  @brief Set the transparency state
   */
  void set_transparent (bool t)
  {
    refresh ();
    if (m_transparent != t) {
      m_transparent = t;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the transparency state
   */
  bool transparent (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_transparent_real;
    } else {
      refresh ();
      return m_transparent;
    }
  }
  
  /**
   *  @brief Set the line width
   */
  void set_width (int w)
  {
    refresh ();
    if (m_width != w) {
      m_width = w;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the line width
   */
  int width (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_width_real;
    } else {
      refresh ();
      return m_width;
    }
  }
  
  /**
   *  @brief Set the marked state
   */
  void set_marked (bool t)
  {
    refresh ();
    if (m_marked != t) {
      m_marked = t;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the marked state
   */
  bool marked (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_marked_real;
    } else {
      refresh ();
      return m_marked;
    }
  }
  
  /**
   *  @brief Set the animation state
   */
  void set_animation (int a)
  {
    refresh ();
    if (m_animation != a) {
      m_animation = a;
      need_realize (nr_visual);
    }
  }
  
  /**
   *  @brief Get the animation state
   */
  int animation (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_animation_real;
    } else {
      refresh ();
      return m_animation;
    }
  }

  /**
   *  @brief Gets a value indicating that shapes are drawn with a diagonal cross
   */
  bool xfill (bool real) const
  {
    if (real) {
      ensure_visual_realized ();
      return m_xfill_real;
    } else {
      refresh ();
      return m_xfill;
    }
  }

  /**
   *  @brief Sets a value indicating that shapes are drawn with a diagonal cross
   */
  void set_xfill (bool xf);
  
  /**
   *  @brief Set the name 
   */
  void set_name (const std::string &n)
  {
    refresh ();
    if (m_name != n) {
      m_name = n;
      need_realize (nr_meta);
    }
  }
  
  /**
   *  @brief Get the name
   */
  const std::string &name () const
  {
    refresh ();
    return m_name;
  }
  
  /**
   *  @brief Obtain the display string for this layer
   *
   *  This string is supposed to be shown in the layer list. It
   *  reflects the source specification in an abbreviated fashion.
   *
   *  If always_with_source is set to true, the source will be shown always. 
   *  If it is set to false, the view's always_show_source attribute with determine whether the source is 
   *  shown.
   */
  std::string display_string (const lay::LayoutViewBase *view, bool real, bool always_with_source = false) const;
  
  /**
   *  @brief The source specification 
   *
   *  This method delivers the source specification as a string
   */
  std::string source_string (bool real) const
  {
    return source(real).to_string ();
  }
  
  /**
   *  @brief Load the source specification from a string
   *
   *  This method may throw an exception if the specification
   *  is not valid. In order to make the layer usable, the properties
   *  object must be "realized" with respect to a LayoutViewBase object.
   */
  void set_source (const std::string &s);
  
  /**
   *  @brief Load the source specification 
   *
   *  In order to make the layer usable, the properties
   *  object must be "realized" with respect to a LayoutViewBase object.
   */
  void set_source (const lay::ParsedLayerSource &s);
  
  /**
   *  @brief Access to the layer source
   */
  const lay::ParsedLayerSource &source (bool real) const
  {
    if (real) {
      ensure_source_realized ();
      return m_source_real;
    } else {
      refresh ();
      return m_source;
    }
  }
  
  /**
   *  @brief The "source specification": layer index
   *
   *  This is the layer index of the respective layer in the layout object. 
   *  The index may be negative, in which case no specific layer is addressed.
   *  This is a property derived from the source specification that can only 
   *  be obtained in "real" semantics.
   */
  int layer_index () const
  {
    ensure_source_realized ();
    return m_layer_index;
  }
  
  /**
   *  @brief The "source specification": cellview index
   *
   *  This is the index of the cell view that this layer resides on.
   *  It may be negative to indicate that it is not considered.
   *  This is a property derived from the source specification that can only 
   *  be obtained in "real" semantics.
   */
  int cellview_index () const
  {
    ensure_source_realized ();
    return m_cellview_index;
  }
  
  /**
   *  @brief Access to the transformation (in database units)
   *
   *  This is a derived attribute that can only be obtained in "real"
   *  semantics.
   */
  const std::vector<db::DCplxTrans> &trans () const
  {
    ensure_source_realized ();
    return m_trans;
  }

  /**
   *  @brief Access to the hierarchy level specification 
   *
   *  This is a derived attribute that can only be obtained in "real"
   *  semantics.
   */
  const HierarchyLevelSelection &hier_levels () const
  {
    ensure_source_realized ();
    return m_hier_levels;
  }

  /**
   *  @brief Access to the property selection
   *
   *  This set will define the selected property set id's.
   *  It is a derived attribute that can only be obtained in "real"
   *  semantics.
   */
  const std::set<db::properties_id_type> &prop_sel () const
  {
    ensure_source_realized ();
    return m_prop_set;
  }

  /**
   *  @brief Access to the "inverse property selection" attribute
   *
   *  This boolean is true, if the prop_sel selection is to be inverted, i.e.
   *  all properties ids in this set are actually unselected.
   */
  bool inverse_prop_sel () const
  {
    ensure_source_realized ();
    return m_inv_prop_set;
  }

  /**
   *  @brief return the "flattened" object
   *  
   *  Compute the "effective" properties (if the object is part of a hierarchy) and create a
   *  new object that represents the same properties but as "local".
   */
  LayerProperties flat () const;

  /**
   *  @brief Actually realize the visual properties (implemented by node class)
   */
  virtual void realize_visual () const;

  /**
   *  @brief Actually realize the source properties (implemented by node class)
   *
   *  This method may be called for example, if new layers have been added to 
   *  a layout and the layer properties should reflect that.
   */
  virtual void realize_source () const;

  /**
   *  @brief Adaptors required for the XML reader
   */
  tl::color_t frame_color_loc () const        { return frame_color (false);      }
  tl::color_t fill_color_loc () const         { return fill_color (false);       }
  int frame_brightness_loc () const           { return frame_brightness (false); }
  int fill_brightness_loc () const            { return fill_brightness (false);  }
  int dither_pattern_loc () const             { return dither_pattern (false);   }
  int line_style_loc () const                 { return line_style (false);       }
  bool visible_loc () const                   { return visible (false);          }
  bool valid_loc () const                     { return valid (false);            }
  bool transparent_loc () const               { return transparent (false);      }
  int width_loc () const                      { return width (false);            }
  bool marked_loc () const                    { return marked (false);           }
  bool xfill_loc () const                     { return xfill (false);            }
  int animation_loc () const                  { return animation (false);        }
  std::string source_string_loc () const      { return source_string (false);    }

protected:
  /**
   *  @brief Merge the parents "real" style with the child's (*this) local style and store the style in the child's real style.
   * 
   *  If the parent argument is zero, the local style is just copied into the 
   *  real style.
   *
   *  @param d The parent properties object or 0 if there is no parent
   */
  void merge_visual (const LayerProperties *d) const; 

  /**
   *  @brief Merge the parents "real" source with the child's (*this) source and store the style in the child's real style.
   * 
   *  If the parent argument is zero, the local source is just copied into the 
   *  real style.
   *
   *  @param d The parent properties object or 0 if there is no parent
   */
  void merge_source (const LayerProperties *d) const; 

  /**
   *  @brief Compute the internal state 
   * 
   *  Computes the internal state such as property selection set.
   *
   *  @param view The view the properties refer to or 0 if there is no view.
   */
  void do_realize (const LayoutViewBase *view) const;

  /** 
   *  @brief Tell, if a realize of the visual properties is needed
   */
  bool realize_needed_visual () const
  {
    return m_realize_needed_visual;
  }

  /** 
   *  @brief Tell, if a realize of the source properties is needed
   */
  bool realize_needed_source () const
  {
    return m_realize_needed_source;
  }

  /**
   *  @brief Marks the properties object as modified
   *
   *  This will basically increment the generation count.
   *  This method is implied when calling "needs_realize".
   */
  void touch ();

  /**
   *  @brief Tells the children that a realize of the visual properties is needed
   *  This is also the "forward sync" for the LayerPropertiesNodeRef.
   */
  virtual void need_realize (unsigned int flags, bool force = false);

  /**
   *  @brief indicates a change of the collapsed/expanded state
   */
  virtual void expanded_state_changed ();

  /**
   *  @brief Fetches the current status from the original properties for the LayerPropertiesNodeRef implementation
   */
  virtual void refresh () const { }

private:
  //  the generation number
  size_t m_gen_id;
  //  display styles
  tl::color_t m_frame_color;
  mutable tl::color_t m_frame_color_real;
  tl::color_t m_fill_color;
  mutable tl::color_t m_fill_color_real;
  int m_frame_brightness;
  mutable int m_frame_brightness_real;
  int m_fill_brightness;
  mutable int m_fill_brightness_real;
  int m_dither_pattern;
  mutable int m_dither_pattern_real;
  int m_line_style;
  mutable int m_line_style_real;
  bool m_valid;
  mutable bool m_valid_real;
  bool m_visible;
  mutable bool m_visible_real;
  bool m_transparent;
  mutable bool m_transparent_real;
  int m_width;
  mutable int m_width_real;
  bool m_marked;
  mutable bool m_marked_real;
  bool m_xfill;
  mutable bool m_xfill_real;
  int m_animation;
  mutable int m_animation_real;
  std::string m_name;

  lay::ParsedLayerSource m_source;
  mutable lay::ParsedLayerSource m_source_real;

  //  this set of members is realized with the realize method
  mutable int m_layer_index;
  mutable int m_cellview_index;
  mutable std::vector<db::DCplxTrans> m_trans;
  mutable HierarchyLevelSelection m_hier_levels;
  mutable std::set<db::properties_id_type> m_prop_set;
  mutable bool m_inv_prop_set;

  void ensure_realized () const;
  void ensure_source_realized () const;
  void ensure_visual_realized () const;

protected:
  enum {
    nr_visual = 1,
    nr_source = 2,
    nr_meta = 4,
    nr_hierarchy = 8
  };

  mutable bool m_realize_needed_source : 1;
  mutable bool m_realize_needed_visual : 1;
};

/**
 *  @brief A layer properties node structure
 *
 *  This adds a hierarchy to the layer properties.
 */
class LAYBASIC_PUBLIC LayerPropertiesNode
  : public LayerProperties,
    public tl::Object
{
public:
  typedef tl::stable_vector<LayerPropertiesNode> child_list;
  typedef child_list::const_iterator const_iterator;
  typedef child_list::iterator iterator;

  /**
   *  @brief Constructor
   */
  LayerPropertiesNode ();

  /**
   *  @brief Destructor
   */
  ~LayerPropertiesNode ();

  /**
   *  @brief Constructor for a leaf element
   */
  LayerPropertiesNode (const LayerProperties &d);

  /**
   *  @brief Constructor for a leaf element
   */
  LayerPropertiesNode (const LayerPropertiesNode &d);

  /**
   *  @brief Assignment of a LayerProperties element
   */
  LayerPropertiesNode &operator= (const LayerProperties &d)
  {
    LayerProperties::operator= (d);
    return *this;
  }

  /**
   *  @brief Assignment of a LayerPropertiesNode element
   */
  LayerPropertiesNode &operator= (const LayerPropertiesNode &d);

  /**
   *  @brief Assignment alias for GSI binding
   */
  void assign (const LayerPropertiesNode &d)
  {
    operator= (d);
  }

  /**
   *  @brief Equality 
   */
  bool operator== (const LayerPropertiesNode &d) const;
  
  /**
   *  @brief Inequality 
   */
  bool operator!= (const LayerPropertiesNode &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief return the "flattened" object
   *  
   *  Contrary to what the name suggests, this method does not flatten the 
   *  hierarchy but rather returns an object that does not need a parent
   *  for the "real" properties. See lay::LayerProperties::flat for a 
   *  description of this process. 
   *  The child list of the returned object will be the same that of the 
   *  original object.
   */
  LayerPropertiesNode flat () const
  {
    LayerPropertiesNode r (*this);
    r = lay::LayerProperties::flat ();
    return r;
  }

  /**
   *  @brief Sets the expanded state of the layer properties tree node
   */
  void set_expanded (bool ex);

  /**
   *  @brief Gets the expanded state of the layer properties node
   */
  bool expanded () const
  {
    refresh ();
    return m_expanded;
  }

  /**
   *  @brief Child layers: begin iterator
   */
  const_iterator begin_children () const
  {
    refresh ();
    return m_children.begin ();
  }
  
  /**
   *  @brief Child layers: end iterator
   */
  const_iterator end_children () const
  {
    refresh ();
    return m_children.end ();
  }
  
  /**
   *  @brief Child layers: begin iterator
   */
  iterator begin_children ()
  {
    refresh ();
    return m_children.begin ();
  }
  
  /**
   *  @brief Child layers: end iterator
   */
  iterator end_children () 
  {
    refresh ();
    return m_children.end ();
  }

  /**
   *  @brief Add a child
   */
  void add_child (const LayerPropertiesNode &child);

  /**
   *  @brief clear the children
   */
  void clear_children ()
  {
    m_children.clear ();
  }
  
  /**
   *  @brief Return a reference to the last child
   */
  LayerPropertiesNode &last_child ()
  {
    return m_children.back ();
  }
  
  /**
   *  @brief Return a reference to the last child
   */
  const LayerPropertiesNode &last_child () const
  {
    return m_children.back ();
  }
  
  /**
   *  @brief Insert a child
   */
  LayerPropertiesNode &insert_child (const iterator &iter, const LayerPropertiesNode &child);
  
  /**
   *  @brief Delete a child
   */
  void erase_child (const iterator &iter);
  
  /**
   *  @brief Test, if there are children
   */
  bool has_children () const
  {
    return ! m_children.empty ();
  }
  
  /**
   *  @brief Compute the bbox of this layer
   *
   *  This takes the layout and path definition (supported by the
   *  given default layout or path, if no specific is given).
   *  The node must have been attached to a view to make this
   *  operation possible.
   *  
   *  @return A bbox in micron units
   */
  db::DBox bbox () const;
  
  /**
   *  @brief Attach to a view
   *
   *  This method attaches the properties node and it's children to a view.
   *  This enables the node to realize itself against the view, i.e.
   *  compute the actual property selection set.
   */
  void attach_view (LayoutViewBase *view, unsigned int list_index);

  /**
   *  @brief Gets the layout view the node lives in
   */
  LayoutViewBase *view () const;

  /**
   *  @brief Gets the index of the layer properties list that node lives in
   */
  unsigned int list_index () const;

  /**
   *  @brief Gets the parent node
   */
  const LayerPropertiesNode *parent () const
  {
    return mp_parent.get ();
  }

  /**
   *  @brief Obtain the unique ID
   *
   *  Each layer properties node object has a unique ID that is created 
   *  when a new LayerPropertiesNode object is instantiated. The ID is
   *  copied when the object is copied. The ID can be used to identify the
   *  object irregardless of it's content.
   */
  unsigned int id () const 
  {
    return m_id;
  }

  virtual void realize_source () const;
  virtual void realize_visual () const;

  void set_expanded_silent (bool ex)
  {
    m_expanded = ex;
  }

protected: 
  virtual void need_realize (unsigned int flags, bool force);
  virtual void expanded_state_changed ();
  void set_parent (const LayerPropertiesNode *);

private:
  //  A reference to the view 
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  unsigned int m_list_index;
  bool m_expanded;
  //  the parent node
  tl::weak_ptr<LayerPropertiesNode> mp_parent;
  //  the list of children
  child_list m_children;
  //  the unique Id
  unsigned int m_id;
};

/**
 *  @brief Flat layer iterator
 *
 *  This iterator provides a flat view for the layers in the layer tree
 */
class LAYBASIC_PUBLIC LayerPropertiesConstIterator
  : public tl::Object
{
public:
  /**
   *  @brief Default constructor
   */
  LayerPropertiesConstIterator ();

  /**
   *  @brief Creates an iterator from a LayerPropertiesNode pointer
   *  This requires the node pointer to point to a node that is addressable by an iterator - i.e.
   *  lives within a view and is either a direct member or a child of a member of a layer properties
   *  list. If this is not the case, the resulting iterator will be a null iterator.
   */
  LayerPropertiesConstIterator (const lay::LayerPropertiesNode *node);

  /**
   *  @brief Constructor: create an iterator pointing either at the beginning or at the end
   */
  LayerPropertiesConstIterator (const LayerPropertiesList &list, bool last = false);

  /**
   *  @brief Constructor: create an iterator from a index
   */
  LayerPropertiesConstIterator (const LayerPropertiesList &list, size_t uint);

  /**
   *  @brief Copy constructor
   */
  LayerPropertiesConstIterator (const LayerPropertiesConstIterator &d);

  /**
   *  @brief Assignment
   */
  LayerPropertiesConstIterator &operator= (const LayerPropertiesConstIterator &d); 

  /**
   *  @brief Inequality
   */
  bool operator!= (const LayerPropertiesConstIterator &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Equality
   */
  bool operator== (const LayerPropertiesConstIterator &d) const
  {
    return m_uint == d.m_uint;
  }

  /**
   *  @brief Comparison
   */
  bool operator< (const LayerPropertiesConstIterator &d) const;

  /**
   *  @brief At-the-top property
   * 
   *  This predicate is true if there is no parent level.
   */
  bool at_top () const;

  /**
   *  @brief At-the-end property
   * 
   *  This predicate is true if the iterator is at the end of either all elements or
   *  at the end of the child list (if down_next_child() or down_first_child() is used to iterate).
   */
  bool at_end () const;

  /**
   *  @brief "is null" predicate
   * 
   *  This predicate is true if the iterator is "null". Such an iterator can be
   *  created with the default constructor or by moving a top-level iterator up.
   */
  bool is_null () const
  {
    return m_uint == 0;
  }

  /**
   *  @brief Increment operator
   */
  LayerPropertiesConstIterator &operator++ ()
  {
    inc (1);
    return *this;
  }

  /**
   *  @brief Move up
   * 
   *  The iterator is moved to point to the current element's parent.
   *  If the current element does not have a parent, the iterator will
   *  be undefined.
   */
  LayerPropertiesConstIterator &up ();

  /**
   *  @brief Move to the next sibling
   * 
   *  The iterator is moved to the nth next sibling of the current element.
   */
  LayerPropertiesConstIterator &next_sibling (ptrdiff_t n = 1);

  /**
   *  @brief Move to the sibling with the given index
   * 
   *  The iterator is moved to the nth sibling of the current element.
   */
  LayerPropertiesConstIterator &to_sibling (size_t n);

  /**
   *  @brief Return the number of siblings
   */
  size_t num_siblings () const;

  /**
   *  @brief Move to the first child
   *
   *  This method moves to the child of the current element. If there is
   *  no child, at_end() will be true. Even then, the iterator points to 
   *  the child level and up() can be used to move back.
   */
  LayerPropertiesConstIterator &down_first_child ();

  /**
   *  @brief Move to the last child
   *
   *  This method moves to the child of the current element. at_end() will be
   *  true then. Even then, the iterator points to the child level and up() 
   *  can be used to move back.
   */
  LayerPropertiesConstIterator &down_last_child ();

  /**
   *  @brief Access to the current element
   */
  const LayerPropertiesNode &operator* () const
  {
    const LayerPropertiesNode *o = obj ();
    tl_assert (o != 0);
    return *o;
  }

  /**
   *  @brief Access to the current element
   */
  const LayerPropertiesNode *operator-> () const
  {
    const LayerPropertiesNode *o = obj ();
    tl_assert (o != 0);
    return o;
  }

  /**
   *  @brief Invalidate the object pointer
   *
   *  This method must be called whenever the object underlying the 
   *  current element is changed, i.e. deleted. This call invalidates
   *  the pointer to the current element thus forcing a refresh of 
   *  the object reference.
   */
  void invalidate ();

  /**
   *  @brief Obtain the parent iterator 
   * 
   *  If there is no parent, the returned iterator will be 'null'.
   */
  LayerPropertiesConstIterator parent () const
  {
    LayerPropertiesConstIterator p (*this);
    p.up ();
    return p;
  }

  /**
   *  @brief Obtain the iterator pointing to the first child
   * 
   *  If there is no children, the iterator will be a valid insert point but not
   *  point to any valid element. It will report "at_end".
   */
  LayerPropertiesConstIterator first_child () const
  {
    LayerPropertiesConstIterator p (*this);
    p.down_first_child ();
    return p;
  }

  /**
   *  @brief Obtain the iterator pointing to the last child
   * 
   *  The iterator will be a valid insert point but not
   *  point to any valid element. It will report "at_end".
   */
  LayerPropertiesConstIterator last_child () const
  {
    LayerPropertiesConstIterator p (*this);
    p.down_last_child ();
    return p;
  }

  /**
   *  @brief Obtain the parent object and the index of the child 
   *
   *  The parent object pointer is 0 if there is no parent. The second
   *  member of the pair indicates which child is addressed.
   */
  std::pair <const LayerPropertiesNode *, size_t> parent_obj () const;

  /**
   *  @brief Obtain the current index
   */
  size_t uint () const
  {
    return m_uint;
  }
  
  /**
   *  @brief Obtain the index of the child within the parent
   *
   *  This method returns the index, that the element pointed to has in the list
   *  of children of it's parent. If the element does not have a parent, the 
   *  index of the element in the global list is returned.
   */
  size_t child_index () const;
  
  /**
   *  @brief Obtain a pointer to the object referenced
   *
   *  If the iterator is null, the pointer returned is 0.
   */
  const LayerPropertiesNode *obj () const
  {
    if (! mp_obj) {
      set_obj ();
    }
    return mp_obj.get ();
  }

  /**
   *  @brief Gets the layer properties list this iterator operates on
   */
  const LayerPropertiesList *list () const
  {
    return m_list.get ();
  }
  
private:
  friend class LayerPropertiesList;

  size_t m_uint;
  tl::weak_ptr<LayerPropertiesList> m_list;
  mutable tl::weak_ptr<LayerPropertiesNode> mp_obj;

  void inc (unsigned int d);
  std::pair <size_t, size_t> factor () const;
  void set_obj () const;
};

/**
 *  @brief A helper function to compare layer properties nodes bottom-up
 */

struct LAYBASIC_PUBLIC CompareLayerIteratorBottomUp 
{
  bool operator() (const lay::LayerPropertiesConstIterator &a, const lay::LayerPropertiesConstIterator &b)
  {
    return a.uint () > b.uint ();
  }
};

/**
 *  @brief Flat, non-const layer iterator
 *
 *  This iterator provides a flat view for the layers in the layer tree
 *  and allows changing the item pointed to.
 */
class LAYBASIC_PUBLIC LayerPropertiesIterator
  : public LayerPropertiesConstIterator
{
public:
  /**
   *  @brief Default constructor
   */
  LayerPropertiesIterator ()
    : LayerPropertiesConstIterator ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Creates an iterator from a LayerPropertiesNode pointer
   *  This requires the node pointer to point to a node that is addressable by an iterator - i.e.
   *  lives within a view and is either a direct member or a child of a member of a layer properties
   *  list. If this is not the case, the resulting iterator will be a null iterator.
   */
  LayerPropertiesIterator (lay::LayerPropertiesNode *node)
    : LayerPropertiesConstIterator (node)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Constructor: create an iterator pointing either at the beginning or at the end
   */
  LayerPropertiesIterator (const LayerPropertiesList &list, bool last = false)
    : LayerPropertiesConstIterator (list, last)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Constructor: create an iterator from a index
   */
  LayerPropertiesIterator (const LayerPropertiesList &list, size_t uint)
    : LayerPropertiesConstIterator (list, uint)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Copy constructor
   */
  LayerPropertiesIterator (const LayerPropertiesIterator &d)
    : LayerPropertiesConstIterator (d)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment
   */
  LayerPropertiesConstIterator &operator= (const LayerPropertiesConstIterator &d)
  {
    LayerPropertiesConstIterator::operator= (d);
    return *this;
  }

  /**
   *  @brief Increment operator
   */
  LayerPropertiesIterator &operator++ ()
  {
    LayerPropertiesConstIterator::operator++ ();
    return *this;
  }

  /**
   *  @brief Access to the current element
   */
  LayerPropertiesNode &operator* () const
  {
    return const_cast <LayerPropertiesNode &> (LayerPropertiesConstIterator::operator* ());
  }

  /**
   *  @brief Access to the current element
   */
  LayerPropertiesNode *operator-> () const
  {
    return const_cast <LayerPropertiesNode *> (LayerPropertiesConstIterator::operator-> ());
  }

  /**
   *  @brief Obtain the parent iterator 
   */
  LayerPropertiesIterator parent () const
  {
    return LayerPropertiesIterator (LayerPropertiesConstIterator::parent ());
  }

  /**
   *  @brief Obtain the parent object and the index of the child 
   */
  std::pair <LayerPropertiesNode *, size_t> parent_obj () const
  {
    std::pair <const LayerPropertiesNode *, size_t> pp = LayerPropertiesConstIterator::parent_obj ();
    return std::make_pair (const_cast<LayerPropertiesNode *> (pp.first), pp.second);
  }

  /**
   *  @brief Move up
   */
  LayerPropertiesIterator &up ()
  {
    LayerPropertiesConstIterator::up ();
    return *this;
  }

private:
  LayerPropertiesIterator (const LayerPropertiesConstIterator &d)
    : LayerPropertiesConstIterator (d)
  { }
};

/**
 *  @brief A list of layer properties
 *
 *  This object represents a list of layer properties.
 */
class LAYBASIC_PUBLIC LayerPropertiesList
  : public tl::Object
{
public:
  typedef LayerPropertiesNode::child_list layer_list;
  typedef LayerPropertiesNode::const_iterator const_iterator;
  typedef LayerPropertiesNode::iterator iterator;
  
  /**
   *  @brief Constructor
   */
  LayerPropertiesList ();

  /**
   *  @brief Destructor
   */
  virtual ~LayerPropertiesList ();

  /**
   *  @brief Copy constructor
   */
  LayerPropertiesList (const LayerPropertiesList &d);

  /**
   *  @brief Assignment 
   */
  LayerPropertiesList &operator= (const LayerPropertiesList &d);

  /**
   *  @brief Equality
   */
  bool operator== (const LayerPropertiesList &d) const;
  
  /**
   *  @brief Inequality
   */
  bool operator!= (const LayerPropertiesList &d) const
  {
    return !operator== (d);
  }
  
  /**
   *  @brief Flat iterator
   *
   *  This iterator delivers all elements including the nodes.
   */
  LayerPropertiesConstIterator begin_const_recursive () const;

  /**
   *  @brief Flat iterator
   *
   *  This iterator delivers the past-the-end flat iterator
   */
  LayerPropertiesConstIterator end_const_recursive () const;

  /**
   *  @brief Flat iterator
   *
   *  This iterator delivers all elements including the nodes.
   */
  LayerPropertiesIterator begin_recursive ();

  /**
   *  @brief Flat iterator
   *
   *  This iterator delivers the past-the-end flat iterator
   */
  LayerPropertiesIterator end_recursive ();

  /**
   *  @brief Start iterator for the layers (top-level)
   */
  const_iterator begin_const () const;
  
  /**
   *  @brief End iterator for the layers (top-level)
   */
  const_iterator end_const () const;
  
  /**
   *  @brief Start iterator for the layers (top-level)
   */
  iterator begin ();
  
  /**
   *  @brief End iterator for the layers (top-level)
   */
  iterator end ();
  
  /**
   *  @brief Last element of the list
   */
  LayerPropertiesNode &back ();
  
  /**
   *  @brief Last element of the list
   */
  const LayerPropertiesNode &back () const;
  
  /**
   *  @brief Expand the layer properties list
   *
   *  Expansion of the layer properties list means to apply wildcards - i.e. 
   *  "@*" is expanded into an outer iteration of layout indices.
   *
   *  @param map_cv_index Maps a specified cv index to the one to use. Use -1 for the first entry to map any present cv index. Map to -1 to specify expansion to all available cv indices.
   *  @param add_default Set this parameter to true to implicitly add an entry for all "missing" layers.
   */
  void expand (const std::map<int, int> &map_cv_index, bool add_default);

  /**
   *  @brief Remove all entries related to a certain cv index or not related to a certain cv index
   *
   *  @param cv_index The index of the cellview whose references shall be removed if except is false or the index of the cellview which shall be retained if except is true
   */
  void remove_cv_references (int cv_index, bool except = false);

  /**
   *  @brief Translate all cv references to the given cv_index
   */
  void translate_cv_references (int cv_index);

  /**
   *  @brief Append another list to this list
   *
   *  This does not attach the view. It is provided to encapsulate the merging of the dither pattern.
   */
  void append (const LayerPropertiesList &other);

  /**
   *  @brief Add a new layer properties element
   */
  void push_back (const LayerPropertiesNode &d);

  /**
   *  @brief Insert a new element into the tree before the given position
   * 
   *  The iterator specifies at which element to insert the new element.
   *  The iterator may have a special form in which it points to a parent
   *  and past-the-end of the respective child list (stack.back ().first==stack.back ().second).
   *  In this case, the element is inserted at the end of the respective parent's child
   *  list.
   *  A reference to the element created is returned.
   */
  LayerPropertiesNode &
  insert (const LayerPropertiesIterator &iter, const LayerPropertiesNode &node);
  
  /**
   *  @brief Delete the given element from the tree
   * 
   *  The iterator specifies the element to delete. 
   */
  void erase (const LayerPropertiesIterator &iter);
  
  /**
   *  @brief Iterator over the custom dither pattern: begin iterator
   */
  lay::DitherPattern::iterator begin_custom_dither_pattern () const
  {
    return m_dither_pattern.begin_custom ();
  }

  /**
   *  @brief Iterator over the custom dither pattern: end iterator
   */
  lay::DitherPattern::iterator end_custom_dither_pattern () const
  {
    return m_dither_pattern.end ();
  }

  /**
   *  @brief Add a new custom dither pattern
   */
  void push_custom_dither_pattern (const lay::DitherPatternInfo &info)
  {
    return m_dither_pattern.replace_pattern (m_dither_pattern.count (), info);
  }

  /**
   *  @brief Access the dither pattern object
   */
  const lay::DitherPattern &dither_pattern () const
  {
    return m_dither_pattern;
  }

  /**
   *  @brief Replace the dither pattern object
   */
  void set_dither_pattern (const lay::DitherPattern &pattern);

  /**
   *  @brief Iterator over the custom line_styles: begin iterator
   */
  lay::LineStyles::iterator begin_custom_line_styles () const
  {
    return m_line_styles.begin_custom ();
  }

  /**
   *  @brief Iterator over the custom line_styles: end iterator
   */
  lay::LineStyles::iterator end_custom_line_styles () const
  {
    return m_line_styles.end ();
  }

  /**
   *  @brief Add a new custom line style
   */
  void push_custom_line_style (const lay::LineStyleInfo &info)
  {
    return m_line_styles.replace_style (m_line_styles.count (), info);
  }

  /**
   *  @brief Access the line styles object
   */
  const lay::LineStyles &line_styles () const
  {
    return m_line_styles;
  }

  /**
   *  @brief Replace the line styles object
   */
  void set_line_styles (const lay::LineStyles &styles);

  /**
   *  @brief Get the name of the layer properties set (for tabbed layer properties)
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Set the name of the layer properties set
   */
  void set_name (const std::string &name) 
  {
    m_name = name;
  }

  /**
   *  @brief Deliver the XML format description for external use
   */
  static const tl::XMLElementList *xml_format ();

  /**
   *  @brief Load the layers from a XML stream
   */
  void load (tl::XMLSource &stream);
  
  /**
   *  @brief Save the layers in XML format
   */
  void save (tl::OutputStream &os) const;
  
  /**
   *  @brief Load a set of layer properties lists from a XML stream
   */
  static void load (tl::XMLSource &stream, std::vector <lay::LayerPropertiesList> &properties_lists);
  
  /**
   *  @brief Save a set of layers in XML format
   */
  static void save (tl::OutputStream &os, const std::vector <lay::LayerPropertiesList> &properties_lists);
  
  /**
   *  @brief Attach the list to the view
   * 
   *  This step attaches all nodes in the list to the given view.
   *  It is required before the nodes can be fully realized.
   *  "load" automatically attaches the view.
   *  This method has the side effect of recomputing the layer source parameters.
   */
  void attach_view (LayoutViewBase *view, unsigned int list_index);

  /**
   *  @brief Gets the layout view this list is attached to
   */
  lay::LayoutViewBase *view () const;

  /**
   *  @brief Gets the layout list
   */
  unsigned int list_index () const;

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  unsigned int m_list_index;
  layer_list m_layer_properties;
  lay::DitherPattern m_dither_pattern;
  lay::LineStyles m_line_styles;
  std::string m_name;
};

/**
 *  @brief A reference into a node hierarchy in the layout view
 *
 *  This object is employed to provide "live" modification of the layer
 *  properties hierarchy and properties.
 *  First, it acts as a proxy for the properties of the node. Changing
 *  a node's property will update the properties in the view as well.
 *  Second, changes in the node's hierarchy will be reflected in the
 *  view's layer hierarchy too.
 *
 *  The implementation is based on a synchronized mirror copy of the
 *  original node. This is not quite efficient and cumbersome. In the
 *  future, this may be replaced by a simple reference.
 */
class LAYBASIC_PUBLIC LayerPropertiesNodeRef
  : public LayerPropertiesNode
{
public:
  /**
   *  @brief Constructor
   */
  LayerPropertiesNodeRef (LayerPropertiesNode *node);

  /**
   *  @brief Constructor from an iterator with an iterator copy
   */
  LayerPropertiesNodeRef (const LayerPropertiesConstIterator &iter);

  /**
   *  @brief Default constructor
   *  This constructor will create an invalid reference.
   */
  LayerPropertiesNodeRef ();

  /**
   *  @brief Copy constructor
   */
  LayerPropertiesNodeRef (const LayerPropertiesNodeRef &other);

  /**
   *  @brief Assignment
   */
  LayerPropertiesNodeRef &operator= (const LayerPropertiesNodeRef &other);

  /**
   *  @brief Deletes the current node
   *  After this operation, the reference will point to the next element.
   */
  void erase ();

  /**
   *  @brief Returns true, if the reference is actively connected to a view
   */
  bool is_valid () const;

  /**
   *  @brief Gets the iterator the reference uses as a pointer
   *  This method returns null if the reference is not through an iterator.
   */
  const LayerPropertiesConstIterator &iter () const;

  /**
   *  @brief Gets the target pointer of the reference
   *  This method returns null if the reference does not point to a specific target.
   *  This method returns a const pointer since the target is not supposed to be
   *  modified directly.
   */
  const LayerPropertiesNode *target () const
  {
    return mp_node.get ();
  }

private:
  LayerPropertiesConstIterator m_iter;
  tl::weak_ptr<LayerPropertiesNode> mp_node;
  size_t m_synched_gen_id;

  virtual void need_realize (unsigned int flags, bool force);
  virtual void expanded_state_changed ();
  virtual void refresh () const;
};

}

#endif


