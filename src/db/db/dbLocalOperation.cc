
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

#include "dbLocalOperation.h"
#include "dbHierProcessor.h"

namespace db
{

// ---------------------------------------------------------------------------------------------
//  local_operations implementation

template <class TS, class TI, class TR>
void local_operation<TS, TI, TR>::compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
{
  if (interactions.num_subjects () <= 1 || ! requests_single_subjects ()) {

    do_compute_local (layout, subject_cell, interactions, results, proc);

  } else {

    std::unique_ptr<tl::RelativeProgress> progress;
    if (proc->report_progress ()) {
      progress.reset (new tl::RelativeProgress (proc->description (this), interactions.size ()));
    }

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const TS &subject_shape = interactions.subject_shape (i->first);

      shape_interactions<TS, TI> single_interactions;

      if (on_empty_intruder_hint () == OnEmptyIntruderHint::Drop) {
        single_interactions.add_subject_shape (i->first, subject_shape);
      } else {
        //  this includes the subject-without-intruder "interaction"
        single_interactions.add_subject (i->first, subject_shape);
      }

      const std::vector<unsigned int> &intruders = interactions.intruders_for (i->first);
      for (typename std::vector<unsigned int>::const_iterator ii = intruders.begin (); ii != intruders.end (); ++ii) {
        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);
        single_interactions.add_intruder_shape (*ii, is.first, is.second);
        single_interactions.add_interaction (i->first, *ii);
      }

      do_compute_local (layout, subject_cell, single_interactions, results, proc);

      if (progress.get ()) {
        ++*progress;
      }

    }

  }
}


//  explicit instantiations
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::Edge>;
template class DB_PUBLIC local_operation<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_operation<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Text, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_operation<db::Polygon, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_operation<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_operation<db::Edge, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_operation<db::TextRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::TextRef, db::PolygonRef, db::TextRef>;

}

