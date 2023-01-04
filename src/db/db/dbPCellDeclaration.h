
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


#ifndef HDR_dbPCellDeclaration
#define HDR_dbPCellDeclaration

#include "dbCommon.h"

#include "gsiObject.h"
#include "dbLayout.h"
#include "tlVariant.h"
#include "tlObject.h"

namespace db
{

typedef std::vector<tl::Variant> pcell_parameters_type;
    
/**
 *  @brief A declaration for one PCell parameter
 *
 *  A parameter is described by a name (potentially a variable name), a description text (for a UI label),
 *  a default value (a variant), a type (requested type for the variant), a choice list (if the parameter can
 *  accept discrete values only) and a hidden flag (if this is set to true, the parameter cannot be changed
 *  by the UI, but is accessible and changeable by the production code).
 *
 *  There is also a readonly flag (in this case, the user interface displays the property but does not allow
 *  to edit it) and a unit string (displayed left to the edit field for double and integer values).
 */
class DB_PUBLIC PCellParameterDeclaration
{
public:
  /**
   *  @brief A enum describing the type
   */
  enum type {
    t_int,      //  an integer value
    t_double,   //  a floating-point value
    t_string,   //  a string value
    t_boolean,  //  a boolean value
    t_layer,    //  a layer (value is a db::LayerProperties object)
    t_shape,    //  a shape (a db::Point, db::Box, db::Polygon, db::Edge or db::Path) rendering a guiding shape
    t_list,     //  a list of strings
    t_callback, //  callback only (button)
    t_none      //  no specific type 
  };

  /**
   *  @brief The default constructor
   */
  PCellParameterDeclaration ()
    : m_hidden (false), m_readonly (false), m_type (t_none)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor with a name
   */
  PCellParameterDeclaration (const std::string &name)
    : m_hidden (false), m_readonly (false), m_type (t_none), m_name (name)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor with a name, type and description
   */
  PCellParameterDeclaration (const std::string &name, type t, const std::string &description)
    : m_hidden (false), m_readonly (false), m_type (t), m_name (name), m_description (description)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor with a name, type, description and default value
   */
  PCellParameterDeclaration (const std::string &name, type t, const std::string &description, const tl::Variant &def)
    : m_default (def), m_hidden (false), m_readonly (false), m_type (t), m_name (name), m_description (description)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor with a name, type, description, default value and unit
   */
  PCellParameterDeclaration (const std::string &name, type t, const std::string &description, const tl::Variant &def, const std::string &unit)
    : m_default (def), m_hidden (false), m_readonly (false), m_type (t), m_name (name), m_description (description), m_unit (unit)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor with a name, type and description and choice values / choice descriptions
   */
  PCellParameterDeclaration (const std::string &name, type t, const std::string &description, const std::vector<tl::Variant> &choices, const std::vector<std::string> &choice_descriptions)
    : m_choices (choices), m_choice_descriptions (choice_descriptions), m_hidden (false), m_readonly (false), m_type (t), m_name (name), m_description (description)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Getter for the name property
   */
  const std::string &get_name () const
  {
    return m_name;
  }

  /**
   *  @brief Setter for the name property
   */
  void set_name (const std::string &name) 
  {
    m_name = name;
  }

  /**
   *  @brief Getter for the unit property
   */
  const std::string &get_unit () const
  {
    return m_unit;
  }

  /**
   *  @brief Setter for the unit property
   */
  void set_unit (const std::string &unit) 
  {
    m_unit = unit;
  }

  /**
   *  @brief Getter for the description property
   */
  const std::string &get_description () const
  {
    return m_description;
  }

  /**
   *  @brief Setter for the description property
   */
  void set_description (const std::string &description) 
  {
    m_description = description;
  }

  /**
   *  @brief Getter for the type property
   */
  type get_type () const
  {
    return m_type;
  }

  /**
   *  @brief Setter for the type property
   */
  void set_type (type t) 
  {
    m_type = t;
  }

  /**
   *  @brief Getter for the readonly property
   */
  bool is_readonly () const
  {
    return m_readonly;
  }

  /**
   *  @brief Setter for the readonly property
   */
  void set_readonly (bool readonly) 
  {
    m_readonly = readonly;
  }

  /**
   *  @brief Getter for the hidden property
   */
  bool is_hidden () const
  {
    return m_hidden;
  }

  /**
   *  @brief Setter for the hidden property
   */
  void set_hidden (bool hidden) 
  {
    m_hidden = hidden;
  }

  /**
   *  @brief Getter for the default value
   */
  const tl::Variant &get_default () const
  {
    return m_default;
  }

  /**
   *  @brief Setter for the default value
   */
  void set_default (const tl::Variant &def) 
  {
    m_default = def;
  }

  /**
   *  @brief Getter for the choices
   */
  const std::vector<tl::Variant> &get_choices () const
  {
    return m_choices;
  }

  /**
   *  @brief Setter for the choices
   */
  void set_choices (const std::vector<tl::Variant> &choices) 
  {
    m_choices = choices;
  }

  /**
   *  @brief Getter for the choice descriptions
   *
   *  The choice descriptions correspond to choice values. The descriptions
   *  can be shown instead of the values in a drop-down box for parameters with
   *  discrete values.
   */
  const std::vector<std::string> &get_choice_descriptions () const
  {
    return m_choice_descriptions;
  }

  /**
   *  @brief Setter for the choice descriptions
   */
  void set_choice_descriptions (const std::vector<std::string> &choice_descriptions) 
  {
    m_choice_descriptions = choice_descriptions;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const db::PCellParameterDeclaration &d) const
  {
    return m_choices == d.m_choices &&
           m_choice_descriptions == d.m_choice_descriptions &&
           m_default == d.m_default &&
           m_hidden == d.m_hidden &&
           m_readonly == d.m_readonly &&
           m_type == d.m_type &&
           m_name == d.m_name &&
           m_description == d.m_description &&
           m_unit == d.m_unit;
  }

private:
  std::vector<tl::Variant> m_choices;
  std::vector<std::string> m_choice_descriptions;
  tl::Variant m_default;
  bool m_hidden, m_readonly;
  type m_type;
  std::string m_name;
  std::string m_description, m_unit;
};

/**
 *  @brief A layer declaration for PCells
 *
 *  PCells must declare the layers it wants to create.
 *  Such a layer declaration consists of a db::LayerProperties description (layer, datatype and/or name)
 *  and a symbolic name which can potentially be used as a variable name.
 */
class DB_PUBLIC PCellLayerDeclaration
  : public db::LayerProperties
{
public:
  /**
   *  @brief The default constructor
   */
  PCellLayerDeclaration ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The constructor from a db::LayerProperties object
   */
  PCellLayerDeclaration (const db::LayerProperties &lp)
    : db::LayerProperties (lp)
  {
    //  .. nothing yet ..
  }

  std::string symbolic;
};

/**
 *  @brief Represents the dynamic state of a single parameter
 */
class DB_PUBLIC ParameterState
{
public:
  /**
   *  @brief A enum describing the icon type
   */
  enum Icon {
    NoIcon = 0,
    InfoIcon = 1,
    ErrorIcon = 2,
    WarningIcon = 3
  };

  /**
   *  @brief Parameterized constructor
   */
  ParameterState ()
    : m_value (), m_visible (true), m_enabled (true), m_readonly (false), m_icon (NoIcon)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the value
   */
  const tl::Variant &value () const
  {
    return m_value;
  }

  /**
   *  @brief Sets the value
   */
  void set_value (const tl::Variant &v)
  {
    m_value = v;
  }

  /**
   *  @brief Gets the visibility state
   */
  bool is_visible () const
  {
    return m_visible;
  }

  /**
   *  @brief Sets the visibility
   */
  void set_visible (bool v)
  {
    m_visible = v;
  }

  /**
   *  @brief Gets the enabled state
   */
  bool is_enabled () const
  {
    return m_enabled;
  }

  /**
   *  @brief Sets the enabled state
   */
  void set_enabled (bool v)
  {
    m_enabled = v;
  }

  /**
   *  @brief Gets a value indicating whether the parameter is read-only
   */
  bool is_readonly () const
  {
    return m_readonly;
  }

  /**
   *  @brief Sets a value indicating whether the parameter is read-only
   */
  void set_readonly (bool f)
  {
    m_readonly = f;
  }

  /**
   *  @brief Gets the tooltip for the parameter
   */
  const std::string &tooltip () const
  {
    return m_tooltip;
  }

  /**
   *  @brief Sets the tooltip
   */
  void set_tooltip (const std::string &s)
  {
    m_tooltip = s;
  }

  /**
   *  @brief Gets the icon
   */
  Icon icon () const
  {
    return m_icon;
  }

  /**
   *  @brief Sets the icon
   */
  void set_icon (Icon i)
  {
    m_icon = i;
  }

private:
  tl::Variant m_value;
  bool m_visible, m_enabled, m_readonly;
  std::string m_tooltip;
  Icon m_icon;
};

/**
 *  @brief Represents the state of call parameters for the callback implementation
 */
class DB_PUBLIC ParameterStates
{
public:
  /**
   *  @brief Default constructor
   */
  ParameterStates ();

  /**
   *  @brief Copy constructor
   */
  ParameterStates (const ParameterStates &other);

  /**
   *  @brief Move constructor
   */
  ParameterStates (ParameterStates &&other);

  /**
   *  @brief Assignment
   */
  ParameterStates &operator= (const ParameterStates &other);

  /**
   *  @brief Sets a parameter from a given state
   */
  void set_parameter (const std::string &name, const ParameterState &ps);

  /**
   *  @brief Gets the parameter state for the parameter with the given name
   *
   *  If the name is not a valid parameter name, the behavior is undefined.
   */
  ParameterState &parameter (const std::string &name);

  /**
   *  @brief Gets the parameter state for the parameter with the given name
   *
   *  If the name is not a valid parameter name, the behavior is undefined.
   */
  const ParameterState &parameter (const std::string &name) const;

  /**
   *  @brief Gets a value indicating whether a parameter with that name is present
   */
  bool has_parameter (const std::string &name) const;

  /**
   *  @brief Returns true, if the values of the parameter states are equal
   */
  bool values_are_equal (const db::ParameterStates &other) const;

public:
  std::map<std::string, ParameterState> m_states;
};

/**
 *  @brief A declaration for a PCell
 */
class DB_PUBLIC PCellDeclaration
  : public gsi::ObjectBase,
    public tl::Object
{
public:
  /**
   *  @brief The default constructor
   */
  PCellDeclaration ();

  /**
   *  @brief The destructor
   */
  virtual ~PCellDeclaration ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Return the layer declarations
   *
   *  This method is supposed to return a set of layer declarations. When the 
   *  layout is produced, the production method will receive a vector of layer id's
   *  corresponding to the layer declarations delivered by this method.
   *  This method receives the current PCell parameters which allows it to deduce layers
   *  from the parameters.
   */
  virtual std::vector<PCellLayerDeclaration> get_layer_declarations (const pcell_parameters_type & /*parameters*/) const
  {
    return std::vector<PCellLayerDeclaration> ();
  }

  /**
   *  @brief Coerces the parameters
   *
   *  This method can be reimplemented to change the parameter set according to some
   *  constraints for example. The reimplementation may modify the parameters in a way
   *  that they are usable for the "produce" method
   *  The method receives a reference to the layout so it is able to verify
   *  the parameters against layout properties.
   *  It can throw an exception to indicate that something is not correct.
   */
  virtual void coerce_parameters (const db::Layout & /*layout*/, db::pcell_parameters_type & /*parameters*/) const
  {
    //  the default implementation does not change the parameters
  }

  /**
   *  @brief Callback on parameter change
   *
   *  This method allows implementing dynamic behavior on the change of a parameter value.
   *  A ParameterStatus object is supplied that allows changing parameter enabled status, visibility and value.
   *  The callback also acts as receiver for t_callback type parameters which only present a button.
   *
   *  The callback function receives the name of the parameter that was changed.
   *  On some occasions, the callback is called unspecifically, for example for the initialization.
   *  In that case, the parameter name is empty.
   *
   *  Exceptions from this implementation are ignored.
   */
  virtual void callback (const db::Layout & /*layout*/, const std::string & /*name*/, ParameterStates & /*states*/) const
  {
    //  the default implementation does nothing
  }

  /**
   *  @brief Produces a layout for the given parameter set and using the given layers.
   *
   *  A reimplementation of that method should produce the desired layout for the given parameter set.
   *  The layout shall be put into the given cell. This code may create cell instances to other cells 
   *  inside the given layout.
   */
  virtual void produce (const db::Layout & /*layout*/, const std::vector<unsigned int> & /*layer_ids*/, const pcell_parameters_type & /*parameters*/, db::Cell & /*cell*/) const
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Get the display name for a PCell with the given parameters
   *
   *  If an empty string is returned, a default display name will be generated.
   */
  virtual std::string get_display_name (const pcell_parameters_type &) const
  {
    return std::string ();
  }

  /**
   *  @brief Returns true, if the PCell can be created from the given shape on the given layer
   *
   *  If this method returns true, KLayout allows this PCell to be created from the given shape.
   *  It will use "parameters_from_shape" and "transformation_from_shape" to derive the 
   *  parameters for the new instance.
   */
  virtual bool can_create_from_shape (const db::Layout & /*layout*/, const db::Shape & /*shape*/, unsigned int /*layer*/) const
  {
    return false;
  }

  /**
   *  @brief Returns the parameter set for a PCell of that kind of shape on the given layer
   *
   *  This method is used to derive the parameters for a new instance from a shape
   *  when can_create_from_shape is true.
   */
  virtual pcell_parameters_type parameters_from_shape (const db::Layout & /*layout*/, const db::Shape & /*shape*/, unsigned int /*layer*/) const
  {
    return pcell_parameters_type ();
  }

  /**
   *  @brief Returns the cell transformation for a PCell of that kind of shape on the given layer
   *
   *  This method is used to derive the initial cell's transformation a new instance from a shape
   *  when can_create_from_shape is true.
   */
  virtual db::Trans transformation_from_shape (const db::Layout & /*layout*/, const db::Shape & /*shape*/, unsigned int /*layer*/) const
  {
    return db::Trans ();
  }

  /**
   *  @brief Returns a value indicating that the PCell wants lazy evaluation
   *
   *  In lazy evaluation mode, the PCell is not immediately updated when a parameter is changed in the UI, but only when it is requested
   *  to be updated.
   */
  virtual bool wants_lazy_evaluation () const
  {
    return false;
  }

  /**
   *  @brief Gets the Layout object the PCell is registered inside or NULL if it is not registered
   */
  db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Add a reference to this object
   *
   *  Since the declaration is supposed to be a static object, it can be shared between 
   *  multiple PCellHeader instances. Using reference counting avoids having to create
   *  copies which would involve clone() methods.
   */
  void add_ref ();

  /**
   *  @brief Release a reference from this object
   *
   *  If the reference count reaches zero, this object is destroyed.
   */
  void release_ref ();

  /**
   *  @brief Returns the name of the PCell
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Returns the ID of the PCell declaration
   */
  db::pcell_id_type id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the cached parameter declarations
   *
   *  This method should be preferred over get_parameter_declarations because it returns
   *  the cached declarations and therefore is much faster, in particular if the actual
   *  implementation is done in a script.
   */
  const std::vector<PCellParameterDeclaration> &parameter_declarations () const;

  /**
   *  @brief Gets the parameter name for the given parameter index
   */
  const std::string &parameter_name (size_t index);

  /**
   *  @brief Return the parameter declarations
   *
   *  This method is supposed to return a set of parameter declarations. When the 
   *  layout is produced, the production method will receive a vector of tl::Variant objects
   *  corresponding to the parameter declarations delivered by this method.
   */
  virtual std::vector<PCellParameterDeclaration> get_parameter_declarations () const
  {
    return std::vector<PCellParameterDeclaration> ();
  }

  /**
   *  @brief Map a indexed parameter set to a parameter vector
   */
  pcell_parameters_type map_parameters (const std::map<size_t, tl::Variant> &indexed_parameters) const;

  /**
   *  @brief Map a named parameter set to a parameter vector
   */
  pcell_parameters_type map_parameters (const std::map<std::string, tl::Variant> &named_parameters) const;

  /**
   *  @brief Converts a parameter vector to named parameters
   */
  std::map<std::string, tl::Variant> named_parameters (const pcell_parameters_type &pv) const;

protected:
  /**
   *  @brief Gets a value indicating whether the PCell wants caching of the parameter declarations
   *
   *  Some PCells with a dynamic parameter definition may not want paramater declaration caching. These
   *  PCells can override this method and return false.
   */
  virtual bool wants_parameter_declaration_caching () const
  {
    return true;
  }

private:
  int m_ref_count;
  pcell_id_type m_id;
  std::string m_name;
  db::Layout *mp_layout;
  mutable bool m_has_parameter_declarations;
  mutable std::vector<PCellParameterDeclaration> m_parameter_declarations;

  friend class db::Layout;
};

}

#endif

