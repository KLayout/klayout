
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


#ifndef HDR_tlRecipe
#define HDR_tlRecipe

#include "tlCommon.h"
#include "tlVariant.h"
#include "tlTypeTraits.h"
#include "tlClassRegistry.h"

namespace tl
{

/**
 *  @brief A facility for providing reproducable recipes
 *
 *  The idea of this facility is to provide a service by which an object
 *  can be reproduced in a parametrized way. The intended use case is a 
 *  DRC report for example, where the DRC script is the generator. 
 *  
 *  In this use case, the DRC engine will register a recipe. It will 
 *  put the serialized version of the recipe into the DRC report. If the 
 *  user requests a re-run of the DRC, the recipe will be called and 
 *  the implementation is supposed to deliver a new database.
 *
 *  To register a recipe, reimplement tl::Recipe and create a singleton
 *  instance. To serialize a recipe, use "generator", to execute the
 *  recipe, use "make". 
 *
 *  Parameters are kept as a generic key/value map.
 */
class TL_PUBLIC Recipe
  : public tl::RegisteredClass<Recipe>
{
public:
  /**
   *  @brief @brief Creates a new recipe object
   */
  Recipe (const std::string &name, const std::string &description = std::string ());

  /**
   *  @brief Destructor
   */
  virtual ~Recipe () { }

  /**
   *  @brief Gets the recipes name (a unique identifier)
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets the description text
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief An utility function to get a parameters
   */
  template <class T>
  static T get_value (const std::map<std::string, tl::Variant> &params, const std::string &pname, const T &def_value)
  {
    std::map<std::string, tl::Variant>::const_iterator p = params.find (pname);
    if (p != params.end ()) {
      const tl::Variant &v = p->second;
      return v.to<T> ();
    } else {
      return def_value;
    }
  }

  /**
   *  @brief Serializes the given recipe
   */
  std::string generator (const std::map<std::string, tl::Variant> &params);

  /**
   *  @brief Executes the recipe from the generator
   *
   *  Returns nil if the recipe can't be executed, e.g. because the recipe isn't known.
   *  Additional parameters can be passed in the second argument.
   *  They have lower priority than the parameters kept in the generator argument.
   */
  static tl::Variant make (const std::string &generator, const std::map<std::string, tl::Variant> &params = std::map<std::string, tl::Variant> ());

  /**
   *  @brief Recipe interface: executes the recipe with the given parameters
   */
  virtual tl::Variant execute (const std::map<std::string, tl::Variant> &params) const = 0;

private:
  Recipe (const Recipe &) : tl::RegisteredClass<tl::Recipe> (this) { }
  Recipe &operator= (const Recipe &) { return *this; }

  std::string m_name;
  std::string m_description;
};

template<> struct type_traits<tl::Recipe> : public type_traits<void>
{
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

} // namespace tl

#endif

