
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_gsiDeclDbContainerHelpers
#define HDR_gsiDeclDbContainerHelpers

#include "dbPropertiesRepository.h"
#include "dbCellVariants.h"
#include "tlVariant.h"
#include "gsiDecl.h"

#include <vector>
#include <map>

namespace gsi
{

template <class Container>
static void enable_properties (Container *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_pass_all ());
}

template <class Container>
static void remove_properties (Container *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_remove_all ());
}

template <class Container>
static void filter_properties (Container *c, const std::vector<tl::Variant> &keys)
{
  if (c->has_properties_repository ()) {
    std::set<tl::Variant> kf;
    kf.insert (keys.begin (), keys.end ());
    c->apply_property_translator (db::PropertiesTranslator::make_filter (c->properties_repository (), kf));
  }
}

template <class Container>
static void map_properties (Container *c, const std::map<tl::Variant, tl::Variant> &map)
{
  if (c->has_properties_repository ()) {
    c->apply_property_translator (db::PropertiesTranslator::make_key_mapper (c->properties_repository (), map));
  }
}

template <class Container>
static gsi::Methods
make_property_methods ()
{
  return
  gsi::method_ext ("enable_properties", &enable_properties<Container>,
    "@brief Enables properties for the given container.\n"
    "This method has an effect mainly on original layers and will import properties from such layers. "
    "By default, properties are not enabled on original layers. Alternatively you can apply \\filter_properties "
    "or \\map_properties to enable properties with a specific name key.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("remove_properties", &remove_properties<Container>,
    "@brief Removes properties for the given container.\n"
    "This will remove all properties on the given container.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("filter_properties", &filter_properties<Container>, gsi::arg ("keys"),
    "@brief Filters properties by certain keys.\n"
    "Calling this method on a container will reduce the properties to values with name keys from the 'keys' list.\n"
    "As a side effect, this method enables properties on original layers.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("map_properties", &map_properties<Container>, gsi::arg ("key_map"),
    "@brief Maps properties by name key.\n"
    "Calling this method on a container will reduce the properties to values with name keys from the 'keys' hash and "
    "renames the properties. Properties not listed in the key map will be removed.\n"
    "As a side effect, this method enables properties on original layers.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  );
}

// ---------------------------------------------------------------------------------
//  Generic shape filter declarations

template <class FilterBase>
class shape_filter_impl
  : public FilterBase
{
public:
  shape_filter_impl ()
  {
    mp_vars = &m_mag_and_orient;
    m_wants_variants = true;
    m_requires_raw_input = false;
  }

  //  overrides virtual method
  virtual const db::TransformationReducer *vars () const
  {
    return mp_vars;
  }

  //  maybe overrides virtual method
  virtual bool requires_raw_input () const
  {
    return m_requires_raw_input;
  }

  void set_requires_raw_input (bool f)
  {
    m_requires_raw_input = f;
  }

  //  overrides virtual method
  virtual bool wants_variants () const
  {
    return m_wants_variants;
  }

  void set_wants_variants (bool f)
  {
    m_wants_variants = f;
  }

  void is_isotropic ()
  {
    mp_vars = &m_mag;
  }

  void is_scale_invariant ()
  {
    mp_vars = &m_orientation;
  }

  void is_isotropic_and_scale_invariant ()
  {
    mp_vars = 0;
  }

  static gsi::Methods method_decls (bool with_requires_raw_input)
  {
    gsi::Methods decls;

    if (with_requires_raw_input) {
      decls =
        method ("requires_raw_input?", &shape_filter_impl::requires_raw_input,
          "@brief Gets a value indicating whether the filter needs raw (unmerged) input\n"
          "See \\requires_raw_input= for details.\n"
        ) +
        method ("requires_raw_input=", &shape_filter_impl::set_requires_raw_input, gsi::arg ("flag"),
          "@brief Sets a value indicating whether the filter needs raw (unmerged) input\n"
          "This flag must be set before using this filter. It tells the filter implementation whether the "
          "filter wants to have raw input (unmerged). The default value is 'false', meaning that\n"
          "the filter will receive merged polygons ('merged semantics').\n"
          "\n"
          "Setting this value to false potentially saves some CPU time needed for merging the polygons.\n"
          "Also, raw input means that strange shapes such as dot-like edges, self-overlapping polygons, "
          "empty or degenerated polygons are preserved."
        );
    }

    decls +=
      method ("wants_variants?", &shape_filter_impl::wants_variants,
        "@brief Gets a value indicating whether the filter prefers cell variants\n"
        "See \\wants_variants= for details.\n"
      ) +
      method ("wants_variants=", &shape_filter_impl::set_wants_variants, gsi::arg ("flag"),
        "@brief Sets a value indicating whether the filter prefers cell variants\n"
        "This flag must be set before using this filter for hierarchical applications (deep mode). "
        "It tells the filter implementation whether cell variants should be created (true, the default) "
        "or shape propagation will be applied (false).\n"
        "\n"
        "This decision needs to be made, if the filter indicates that it will deliver different results\n"
        "for scaled or rotated versions of the shape (see \\is_isotropic and the other hints). If a cell\n"
        "is present with different qualities - as seen from the top cell - the respective instances\n"
        "need to be differentiated. Cell variant formation is one way, shape propagation the other way.\n"
        "Typically, cell variant formation is less expensive, but the hierarchy will be modified."
      ) +
      method ("is_isotropic", &shape_filter_impl::is_isotropic,
        "@brief Indicates that the filter has isotropic properties\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the orientation of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "Examples for isotropic (polygon) filters are area or perimeter filters. The area or perimeter of a polygon "
        "depends on the scale, but not on the orientation of the polygon."
      ) +
      method ("is_scale_invariant", &shape_filter_impl::is_scale_invariant,
        "@brief Indicates that the filter is scale invariant\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the scale of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "An example for a scale invariant (polygon) filter is the bounding box aspect ratio (height/width) filter. "
        "The definition of heigh and width depends on the orientation, but the ratio is independent on scale."
      ) +
      method ("is_isotropic_and_scale_invariant", &shape_filter_impl::is_isotropic_and_scale_invariant,
        "@brief Indicates that the filter is isotropic and scale invariant\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the scale and orientation of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "An example for such a (polygon) filter is the square selector. Whether a polygon is a square or not does not depend on "
        "the polygon's orientation nor scale."
      );

    return decls;
  }

private:
  const db::TransformationReducer *mp_vars;
  db::OrientationReducer m_orientation;
  db::MagnificationReducer m_mag;
  db::MagnificationAndOrientationReducer m_mag_and_orient;
  bool m_requires_raw_input;
  bool m_wants_variants;
};

// ---------------------------------------------------------------------------------
//  Generic shape processor declarations

template <class ProcessorBase>
class shape_processor_impl
  : public ProcessorBase
{
public:
  typedef typename  ProcessorBase::shape_type shape_type;
  typedef typename  ProcessorBase::result_type result_type;

  shape_processor_impl ()
  {
    mp_vars = &m_mag_and_orient;
    m_wants_variants = true;
    m_requires_raw_input = false;
    m_result_is_merged = false;
    m_result_must_not_be_merged = false;
  }

  //  overrides virtual method
  virtual const db::TransformationReducer *vars () const
  {
    return mp_vars;
  }

  //  maybe overrides virtual method
  virtual bool requires_raw_input () const
  {
    return m_requires_raw_input;
  }

  void set_requires_raw_input (bool f)
  {
    m_requires_raw_input = f;
  }

  //  overrides virtual method
  virtual bool wants_variants () const
  {
    return m_wants_variants;
  }

  void set_wants_variants (bool f)
  {
    m_wants_variants = f;
  }

  //  overrides virtual method
  virtual bool result_is_merged () const
  {
    return m_result_is_merged;
  }

  void set_result_is_merged (bool f)
  {
    m_result_is_merged = f;
  }

  //  overrides virtual method
  virtual bool result_must_not_be_merged () const
  {
    return m_result_must_not_be_merged;
  }

  void set_result_must_not_be_merged (bool f)
  {
    m_result_must_not_be_merged = f;
  }

  void is_isotropic ()
  {
    mp_vars = &m_mag;
  }

  void is_scale_invariant ()
  {
    mp_vars = &m_orientation;
  }

  void is_isotropic_and_scale_invariant ()
  {
    mp_vars = 0;
  }

  virtual void process (const shape_type &shape, std::vector<result_type> &res) const
  {
    res = do_process (shape);
  }

  std::vector<result_type> issue_do_process (const shape_type &) const
  {
    return std::vector<result_type> ();
  }

  std::vector<result_type> do_process (const shape_type &shape) const
  {
    if (f_process.can_issue ()) {
      return f_process.issue<shape_processor_impl, std::vector<result_type>, const shape_type &> (&shape_processor_impl::issue_do_process, shape);
    } else {
      return issue_do_process (shape);
    }
  }

  gsi::Callback f_process;

  static gsi::Methods method_decls (bool with_merged_options)
  {
    gsi::Methods decls =
      callback ("process", &shape_processor_impl::issue_do_process, &shape_processor_impl::f_process, gsi::arg ("shape"),
        "@brief Processes a shape\n"
        "This method is the actual payload. It needs to be reimplemented in a derived class.\n"
        "If needs to process the input shape and deliver a list of output shapes.\n"
        "The output list may be empty to entirely discard the input shape. It may also contain more than a single shape.\n"
        "In that case, the number of total shapes may grow during application of the processor.\n"
      );

    if (with_merged_options) {
      decls +=
        method ("requires_raw_input?", &shape_processor_impl::requires_raw_input,
          "@brief Gets a value indicating whether the processor needs raw (unmerged) input\n"
          "See \\requires_raw_input= for details.\n"
        ) +
        method ("requires_raw_input=", &shape_processor_impl::set_requires_raw_input, gsi::arg ("flag"),
          "@brief Sets a value indicating whether the processor needs raw (unmerged) input\n"
          "This flag must be set before using this processor. It tells the processor implementation whether the "
          "processor wants to have raw input (unmerged). The default value is 'false', meaning that\n"
          "the processor will receive merged polygons ('merged semantics').\n"
          "\n"
          "Setting this value to false potentially saves some CPU time needed for merging the polygons.\n"
          "Also, raw input means that strange shapes such as dot-like edges, self-overlapping polygons, "
          "empty or degenerated polygons are preserved."
        ) +
        method ("result_is_merged?", &shape_processor_impl::result_is_merged,
          "@brief Gets a value indicating whether the processor delivers merged output\n"
          "See \\result_is_merged= for details.\n"
        ) +
        method ("result_is_merged=", &shape_processor_impl::set_result_is_merged, gsi::arg ("flag"),
          "@brief Sets a value indicating whether the processor delivers merged output\n"
          "This flag must be set before using this processor. If the processor maintains the merged condition\n"
          "by design (output is merged if input is), it is a good idea to set this predicate to 'true'.\n"
          "This will avoid additional merge steps when the resulting collection is used in further operations\n"
          "that need merged input\n."
        ) +
        method ("result_must_not_be_merged?", &shape_processor_impl::result_must_not_be_merged,
          "@brief Gets a value indicating whether the processor's output must not be merged\n"
          "See \\result_must_not_be_merged= for details.\n"
        ) +
        method ("result_must_not_be_merged=", &shape_processor_impl::set_result_must_not_be_merged, gsi::arg ("flag"),
          "@brief Sets a value indicating whether the processor's output must not be merged\n"
          "This flag must be set before using this processor. The processor can set this flag if it wants to\n"
          "deliver shapes that must not be merged - e.g. point-like edges or strange or degenerated polygons.\n."
        );
    }

    decls +=
      method ("wants_variants?", &shape_processor_impl::wants_variants,
        "@brief Gets a value indicating whether the filter prefers cell variants\n"
        "See \\wants_variants= for details.\n"
      ) +
      method ("wants_variants=", &shape_processor_impl::set_wants_variants, gsi::arg ("flag"),
        "@brief Sets a value indicating whether the filter prefers cell variants\n"
        "This flag must be set before using this filter for hierarchical applications (deep mode). "
        "It tells the filter implementation whether cell variants should be created (true, the default) "
        "or shape propagation will be applied (false).\n"
        "\n"
        "This decision needs to be made, if the filter indicates that it will deliver different results\n"
        "for scaled or rotated versions of the shape (see \\is_isotropic and the other hints). If a cell\n"
        "is present with different qualities - as seen from the top cell - the respective instances\n"
        "need to be differentiated. Cell variant formation is one way, shape propagation the other way.\n"
        "Typically, cell variant formation is less expensive, but the hierarchy will be modified."
      ) +
      method ("is_isotropic", &shape_processor_impl::is_isotropic,
        "@brief Indicates that the filter has isotropic properties\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the orientation of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "Examples for isotropic (polygon) processors are size or shrink operators. Size or shrink is not dependent "
        "on orientation unless size or shrink needs to be different in x and y direction."
      ) +
      method ("is_scale_invariant", &shape_processor_impl::is_scale_invariant,
        "@brief Indicates that the filter is scale invariant\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the scale of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "An example for a scale invariant (polygon) processor is the rotation operator. Rotation is not depending on scale, "
        "but on the original orientation as mirrored versions need to be rotated differently."
      ) +
      method ("is_isotropic_and_scale_invariant", &shape_processor_impl::is_isotropic_and_scale_invariant,
        "@brief Indicates that the filter is isotropic and scale invariant\n"
        "Call this method before using the filter to indicate that the selection is independent of "
        "the scale and orientation of the shape. This helps the filter algorithm optimizing the filter run, specifically in "
        "hierarchical mode.\n"
        "\n"
        "An example for such a (polygon) processor is the convex decomposition operator. The decomposition of a polygon into "
        "convex parts is an operation that is not depending on scale nor orientation."
      );

    return decls;
  }

private:
  const db::TransformationReducer *mp_vars;
  db::OrientationReducer m_orientation;
  db::MagnificationReducer m_mag;
  db::MagnificationAndOrientationReducer m_mag_and_orient;
  bool m_requires_raw_input;
  bool m_wants_variants;
  bool m_result_is_merged;
  bool m_result_must_not_be_merged;

  //  No copying
  shape_processor_impl &operator= (const shape_processor_impl &);
  shape_processor_impl (const shape_processor_impl &);
};

}

#endif
