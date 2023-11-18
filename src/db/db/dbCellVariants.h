
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

#ifndef HDR_dbCellVariants
#define HDR_dbCellVariants

#include "dbCommon.h"

#include "dbTrans.h"
#include "dbLayout.h"

#include <memory>

namespace db
{

/**
 *  @brief The reducer interface
 *
 *  The transformation reducer is used by the variant builder to provide a
 *  reduced version of the transformation. Variants are built based on this
 *  reduced transformation.
 *
 *  Reduction must satisfy the modulo condition:
 *
 *   reduce(A*B) = reduce(reduce(A)*reduce(B))
 */
class DB_PUBLIC TransformationReducer
{
public:
  TransformationReducer () { }
  virtual ~TransformationReducer () { }

  virtual db::Trans reduce_trans (const db::Trans &trans) const { return reduce (trans); }
  virtual db::ICplxTrans reduce_trans (const db::ICplxTrans &trans) const { return reduce (trans); }
  virtual db::Trans reduce (const db::Trans &trans) const = 0;
  virtual db::ICplxTrans reduce (const db::ICplxTrans &trans) const = 0;
  virtual bool equals (const TransformationReducer *other) const = 0;
  virtual bool is_translation_invariant () const { return true; }
};

/**
 *  @brief An orientation reducer
 *
 *  This reducer incarnation reduces the transformation to it's rotation/mirror part.
 */
struct DB_PUBLIC OrientationReducer
  : public TransformationReducer
{
  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;
};

/**
 *  @brief A reducer for invariance against orthogonal transformations (rotations of multiple of 90 degree)
 */
struct DB_PUBLIC OrthogonalTransformationReducer
  : public TransformationReducer
{
  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;
};

/**
 *  @brief A magnification reducer
 *
 *  This reducer incarnation reduces the transformation to it's scaling part.
 */
struct DB_PUBLIC MagnificationReducer
  : public TransformationReducer
{
  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &) const;
  virtual bool equals (const TransformationReducer *other) const;
};

/**
 *  @brief A reducer for magnification and XYAnisotropy
 *
 *  This reducer is used for cases where an x and y-value is given, e.g. anisotropic size.
 */
struct DB_PUBLIC XYAnisotropyAndMagnificationReducer
  : public TransformationReducer
{
  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;
};

/**
 *  @brief A magnification and orientation reducer
 *
 *  This reducer incarnation reduces the transformation to it's rotation/mirror/magnification part (2d matrix)
 */
struct DB_PUBLIC MagnificationAndOrientationReducer
  : public TransformationReducer
{
  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;
};

/**
 *  @brief A grid reducer
 *
 *  This reducer incarnation reduces the transformation to it's displacement modulo a grid
 */
struct DB_PUBLIC GridReducer
  : public TransformationReducer
{
  GridReducer (db::Coord grid);

  db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;

  bool is_translation_invariant () const { return false; }

private:
  db::Coord m_grid;
};

/**
 *  @brief A scale+grid reducer
 *
 *  This reducer incarnation reduces the transformation to it's displacement modulo a grid
 *  after a specified scaling has been applied.
 *  The scaling is given by a divider and multiplier and is mult / div.
 */
struct DB_PUBLIC ScaleAndGridReducer
  : public TransformationReducer
{
  ScaleAndGridReducer (db::Coord grid, db::Coord mult, db::Coord div);

  virtual db::ICplxTrans reduce_trans (const db::ICplxTrans &trans) const;
  virtual db::Trans reduce_trans (const db::Trans &trans) const;
  virtual db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  virtual db::Trans reduce (const db::Trans &trans) const;
  virtual bool equals (const TransformationReducer *other) const;

  bool is_translation_invariant () const { return false; }

private:
  int64_t m_mult;
  int64_t m_grid;
};

/**
 *  @brief A class computing variants for cells according to a given criterion
 *
 *  The cell variants are build from the cell instances and are accumulated over
 *  the hierarchy path.
 */
class DB_PUBLIC VariantsCollectorBase
{
public:
  /**
   *  @brief Creates a variant collector without a transformation reducer
   */
  VariantsCollectorBase ();

  /**
   *  @brief Creates a variant collector with the given reducer
   */
  VariantsCollectorBase (const TransformationReducer *red);

  /**
   *  @brief Collects cell variants for the given layout starting from the top cell
   */
  void collect (Layout *layout, cell_index_type initial_cell);

  /**
   *  @brief Creates cell variants for singularization of the different variants
   *
   *  After this method can been used, all cells with more than one variant are separated and
   *  the corresponding instances are updated.
   *
   *  If given, *var_table will be filled with a map giving the new cell and variant against
   *  the old cell for all cells with more than one variant.
   */
  void separate_variants (std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > *var_table = 0);

  /**
   *  @brief Commits the shapes for different variants to the current cell hierarchy
   *
   *  This is an alternative approach and will push the variant shapes into the parent hierarchy.
   *  "to_commit" initially is a set of shapes to commit for the given cell and variant.
   *  This map is modified during the algorithm and should be discarded later.
   */
  void commit_shapes (unsigned int layer, std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > &to_commit);

  /**
   *  @brief Gets the variants for a given cell
   *
   *  The keys of the map are the variants, the values is the instance count of the variant
   *  (as seen from the top cell).
   */
  const std::set<db::ICplxTrans> &variants (db::cell_index_type ci) const;

  /**
   *  @brief Gets the transformation for a single variant
   *
   *  This requires the cell not to be a variant (i.e. already separated).
   *  It returns the corresponding transformation.
   */
  const db::ICplxTrans &single_variant_transformation (db::cell_index_type ci) const;

  /**
   *  @brief Returns true, if variants have been built
   */
  bool has_variants () const;

  /**
   *  @brief Utility: copy all shapes from one cell to another
   */
  static void copy_shapes (db::Layout &layout, db::cell_index_type ci_to, db::cell_index_type ci_from);

private:
  std::map<db::cell_index_type, std::set<db::ICplxTrans> > m_variants;
  std::set<db::cell_index_type> m_called;
  const TransformationReducer *mp_red;
  db::Layout *mp_layout;

  void add_variant (std::set<ICplxTrans> &variants, const db::CellInstArray &inst, bool tl_invariant) const;
  void add_variant_non_tl_invariant (std::set<db::ICplxTrans> &variants, const db::CellInstArray &inst) const;
  void add_variant_tl_invariant (std::set<ICplxTrans> &variants, const db::CellInstArray &inst) const;
  void product (const std::set<db::ICplxTrans> &v1, const std::set<db::ICplxTrans> &v2, std::set<db::ICplxTrans> &prod) const;
  void create_var_instances (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table, bool tl_invariant) const;
  void create_var_instances_non_tl_invariant (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table) const;
  void create_var_instances_tl_invariant (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table) const;
};

/**
 *  @brief A template using a specific transformation reducer
 */
template <class RED>
class DB_PUBLIC_TEMPLATE cell_variants_collector
  : public VariantsCollectorBase
{
public:
  /**
   *  @brief Creates a variant collector without a transformation reducer
   */
  cell_variants_collector ()
    : VariantsCollectorBase (&m_red)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a variant collector with the given reducer
   *
   *  The collector will take ownership over the reducer
   */
  cell_variants_collector (const RED &red)
    : VariantsCollectorBase (&m_red), m_red (red)
  {
    //  .. nothing yet ..
  }

private:
  RED m_red;
};

/**
 *  @brief A class computing variants for cells with statistics
 *
 *  This version provides detailed information about the multiplicity of a certain variant.
 *  It does not offer a way to seperate variants.
 */
class DB_PUBLIC VariantStatistics
{
public:
  /**
   *  @brief Creates a variant collector without a transformation reducer
   */
  VariantStatistics ();

  /**
   *  @brief Creates a variant collector with the given reducer
   */
  VariantStatistics (const TransformationReducer *red);

  /**
   *  @brief Collects cell variants for the given layout starting from the top cell
   */
  void collect (const db::Layout *layout, db::cell_index_type initial_cell);

  /**
   *  @brief Gets the variants for a given cell
   *
   *  The keys of the map are the variants, the values is the instance count of the variant
   *  (as seen from the top cell).
   */
  const std::map<db::ICplxTrans, size_t> &variants (db::cell_index_type ci) const;

  /**
   *  @brief Returns true, if variants have been built
   */
  bool has_variants () const;

private:
  std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> > m_variants;
  const TransformationReducer *mp_red;

  void add_variant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst, bool tl_invariant) const;
  void add_variant_non_tl_invariant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst) const;
  void add_variant_tl_invariant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst) const;
  void product (const std::map<db::ICplxTrans, size_t> &v1, const std::map<db::ICplxTrans, size_t> &v2, std::map<db::ICplxTrans, size_t> &prod) const;
};

/**
 *  @brief A template using a specific transformation reducer
 */
template <class RED>
class DB_PUBLIC_TEMPLATE cell_variants_statistics
  : public VariantStatistics
{
public:
  /**
   *  @brief Creates a variant statistics without a transformation reducer
   */
  cell_variants_statistics ()
    : VariantStatistics (&m_red)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a variant statistics with the given reducer
   *
   *  The statistics object will take ownership over the reducer
   */
  cell_variants_statistics (const RED &red)
    : VariantStatistics (&m_red), m_red (red)
  {
    //  .. nothing yet ..
  }

private:
  RED m_red;
};


}  // namespace db

#endif

