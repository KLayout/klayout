
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



#ifndef HDR_dbLocalOperation
#define HDR_dbLocalOperation

#include "dbCommon.h"
#include "dbLayout.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace db
{

template <class TS, class TI> class shape_interactions;
class LocalProcessorBase;
class TransformationReducer;

/**
 *  @brief Indicates the desired behaviour for subject shapes for which there is no intruder
 */
enum OnEmptyIntruderHint {
  /**
   *  @brief Don't imply a specific behaviour
   */
  Ignore = 0,

  /**
   *  @brief Copy the subject shape
   */
  Copy,

  /**
   *  @brief Copy the subject shape to the second result
   */
  CopyToSecond,

  /**
   *  @brief Drop the subject shape
   */
  Drop
};

/**
 *  @brief A base class for "local operations"
 *  A local operation is any operation whose result can be computed by
 *  combining the results derived from individual shape pairs.
 *  The shape pairs can originate from different or the same layer.
 *  If the layers are different, one layer is the subject layer, the
 *  other layer is the "intruder" layer. Subject shapes are always
 *  considered, intruder shapes only if they interact with subject shapes.
 *  This class implements the actual operation. It receives a
 *  cluster of subject shapes vs. corresponding intruder shapes.
 */
template <class TS, class TI, class TR>
class DB_PUBLIC local_operation
{
public:
  /**
   *  @brief Constructor
   */
  local_operation () { }

  /**
   *  @brief Destructor
   */
  virtual ~local_operation () { }

  /**
   *  @brief Computes the results from a given set of interacting shapes
   *
   *  If the operation requests single subject mode, the interactions will be split into single subject/intruder clusters
   */
  void compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const;

  /**
   *  @brief Indicates the desired behaviour when a shape does not have an intruder
   */
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return Ignore; }

  /**
   *  @brief If this method returns true, this operation requests single subjects for meal
   */
  virtual bool requests_single_subjects () const { return false; }

  /**
   *  @brief Gets a description text for this operation
   */
  virtual std::string description () const = 0;

  /**
   *  @brief Gets the interaction distance
   *  A distance of means the shapes must overlap in order to interact.
   */
  virtual db::Coord dist () const { return 0; }

  /**
   *  @brief Gets the cell variant reducer that indicates whether to build cell variants and which
   */
  virtual const db::TransformationReducer *vars () const { return 0; }

protected:
  /**
   *  @brief Computes the results from a given set of interacting shapes
   *  @param layout The layout to which the shapes belong
   *  @param interactions The interaction set
   *  @param result The container to which the results are written
   */
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &result, const db::LocalProcessorBase *proc) const = 0;
};

}

#endif

