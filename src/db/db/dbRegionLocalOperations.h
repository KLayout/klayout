
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


#ifndef HDR_dbRegionLocalOperations
#define HDR_dbRegionLocalOperations

#include "dbCommon.h"
#include "dbEdgePairRelations.h"
#include "dbLocalOperation.h"

namespace db
{

class CheckLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::EdgePair>
{
public:
  CheckLocalOperation (const EdgeRelationFilter &check, bool different_polygons, bool has_other, bool shielded);

  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;

  virtual db::Coord dist () const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  EdgeRelationFilter m_check;
  bool m_different_polygons;
  bool m_has_other;
  bool m_shielded;
};

template <class TS, class TI, class TR>
class interacting_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_local_operation (int mode, bool touching, bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual typename local_operation<TS, TI, TR>::on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> InteractingLocalOperation;

template <class TS, class TI, class TR>
class pull_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_local_operation (int mode, bool touching);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual typename local_operation<TS, TI, TR>::on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
};

typedef pull_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> PullLocalOperation;

template <class TS, class TI, class TR>
class interacting_with_edge_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_edge_local_operation (bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual typename local_operation<TS, TI, TR>::on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_with_edge_local_operation<db::PolygonRef, db::Edge, db::PolygonRef> InteractingWithEdgeLocalOperation;

class PullWithEdgeLocalOperation
  : public local_operation<db::PolygonRef, db::Edge, db::Edge>
{
public:
  PullWithEdgeLocalOperation ();

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;
};

template <class TS, class TI, class TR>
class interacting_with_text_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_text_local_operation (bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual typename local_operation<TS, TI, TR>::on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_with_text_local_operation<db::PolygonRef, db::TextRef, db::PolygonRef> InteractingWithTextLocalOperation;

class PullWithTextLocalOperation
  : public local_operation<db::PolygonRef, db::TextRef, db::TextRef>
{
public:
  PullWithTextLocalOperation ();

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::TextRef> &interactions, std::vector<std::unordered_set<db::TextRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;
};

} // namespace db

#endif

