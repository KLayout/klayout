
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

#include "gsiDeclDbMeasureHelpers.h"

namespace gsi
{

/**
 *  @brief Provides methods to handle measurement functions on various containers
 */
template <class Container, class ProcessorBase, class FilterBase>
struct measure_methods
{
  /**
   *  @brief Computes one or many properties from expressions
   *
   *  This method will use the shapes from the "input" container and compute properties from them using the
   *  given expression from "expressions". This map specifies the name of the target property, the value
   *  specifies the expression to execute.
   *
   *  The expressions can make use of the following variables and functions:
   *  * "shape": the shape which is currently seen
   *  * "<prop-name>": an existing property from the shape currently seen (or nil, if no such property is present).
   *    This is a shortcut, only for properties with string names that are compatible with variable names
   *  * "value(<name>)": the value of the property with the given name - if multiple properties with that
   *    name are present, one value is returned
   *  * "values(<name>)": a list of values for all properties with the given name
   *
   *  Returns the new container with the computed properties attached.
   */
  Container computed_properties (Container *input, const std::map<tl::Variant, std::string> &expressions, bool clear_properties);

  /**
   *  @brief Computes one or many properties from expressions
   *
   *  Like "computed_properties", this method computes properties, but attaches them to the existing shapes.
   *  As a side effect, the shapes may be merged if "merged_semantics" applies. If "clear_properties" is true,
   *  any existing properties will be removed. If not, the new properties are added to the existing ones.
   */
  void compute_properties_in_place (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties);

  /**
   *  @brief Selects all shapes for which the condition expression renders true (or the inverse)
   *
   *  The condition expression can use the features as described for "computed_properties".
   *  If inverse is false, all shapes are selected for which the condition renders true. If
   *  inverse is true, all shapes are selected for which the condition renders false.
   */
  Container selected_if (const Container *container, const std::string &condition_expression, bool inverse);

  /**
   *  @brief In-place version of "selected_if"
   */
  void select_if (Container *container, const std::string &condition_expression, bool inverse);

  /**
   *  @brief Splits the container into one for which is the condition is true and one with the other shapes
   */
  std::pair<Container, Container> split_if (const Container *container, const std::string &condition_expression);
};

template <class Container, class ProcessorBase, class FilterBase>
Container
measure_methods<Container, ProcessorBase, FilterBase>::computed_properties (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties)
{
  property_computation_processor<ProcessorBase, Container> proc (container, expressions, !clear_properties);
  return container->processed (proc);
}

template <class Container, class ProcessorBase, class FilterBase>
void
measure_methods<Container, ProcessorBase, FilterBase>::compute_properties_in_place (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties)
{
  property_computation_processor<ProcessorBase, Container> proc (container, expressions, !clear_properties);
  container->process (proc);
}

template <class Container, class ProcessorBase, class FilterBase>
Container
measure_methods<Container, ProcessorBase, FilterBase>::selected_if (const Container *container, const std::string &condition_expression, bool inverse)
{
  expression_filter<FilterBase, Container> filter (condition_expression, inverse);
  return container->filtered (filter);
}

template <class Container, class ProcessorBase, class FilterBase>
void
measure_methods<Container, ProcessorBase, FilterBase>::select_if (Container *container, const std::string &condition_expression, bool inverse)
{
  expression_filter<FilterBase, Container> filter (condition_expression, inverse);
  container->filter (filter);
}

template <class Container, class ProcessorBase, class FilterBase>
std::pair<Container, Container>
measure_methods<Container, ProcessorBase, FilterBase>::split_if (const Container *container, const std::string &condition_expression)
{
  expression_filter<FilterBase, Container> filter (condition_expression, false);
  return container->split_filter (filter);
}

//  explicit instantiations
template struct measure_methods<db::Region, db::shape_collection_processor<db::Polygon, db::Polygon>, db::AllMustMatchFilter>;
template struct measure_methods<db::Edges, db::shape_collection_processor<db::Edge, db::Edge>, db::AllEdgesMustMatchFilter>;
template struct measure_methods<db::EdgePairs, db::shape_collection_processor<db::EdgePair, db::EdgePair>, db::EdgePairFilterBase>;
template struct measure_methods<db::Texts, db::shape_collection_processor<db::Text, db::Text>, db::TextFilterBase>;

}
