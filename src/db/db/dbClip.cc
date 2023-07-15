
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




#include "dbClip.h"
#include "dbLayout.h"
#include "dbPolygonGenerators.h"

namespace db
{

// ------------------------------------------------------------------------------
//  clip_poly implementation

struct EdgeCompareP1Func 
{
  bool operator () (const db::Edge &e1, const db::Edge &e2) const
  {
    return e1.p1 () < e2.p1 ();
  }
};

struct CoordSignPairCompareFunc
{
  CoordSignPairCompareFunc (int s)
    : m_s (s)
  { }

  bool operator() (const std::pair <db::Coord, int> &a, const std::pair <db::Coord, int> &b) const
  {
    if (a.first != b.first) {
      return a.first < b.first;
    } 
    return m_s > 0 ? (a.second < b.second) : (a.second > b.second);
  }

private:
  int m_s;
};

//  TODO: check, if the clip can be implemented by subsequent "cut" operations using all four borders
//  Is that more efficient?
template <class P, class PC> static void 
clip_poly (const P &poly, const db::Box &box, std::vector <P> &clipped_poly, bool resolve_holes)
{
  db::Box pbox = poly.box ();

  //  Polygon completely inside the clip box -> return the polygon
  if (pbox.inside (box)) {
    clipped_poly.push_back (poly);
    return;
  }

  //  Polygon completely outside the clip box -> return nothing.
  if (! pbox.overlaps (box)) {
    return;
  }

  //  first, extract and sort all edges
  std::vector <db::Edge> edges;
  edges.reserve (poly.hull ().size ());

  //  create a set of edges to consider
  for (typename P::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {

    db::Edge edge = *e;

    db::Coord y1 = std::min (edge.p1 ().y (), edge.p2 ().y ());
    db::Coord y2 = std::max (edge.p1 ().y (), edge.p2 ().y ());

    if (y1 < box.p2 ().y () && y2 > box.p1 ().y ()) {

      std::pair <bool, db::Edge> ce = edge.clipped (box);
      if (ce.first) {

        edges.push_back (ce.second);

        db::Edge e1 (db::Point (ce.second.p1 ().x (), edge.p1 ().y ()), ce.second.p1 ());
        db::Edge e2 (ce.second.p2 (), db::Point (ce.second.p2 ().x (), edge.p2 ().y ()));

        if (! e1.is_degenerate () && (ce = e1.clipped (box)).first && ! ce.second.is_degenerate ()) {
          edges.push_back (ce.second);
        }
        if (! e2.is_degenerate () && (ce = e2.clipped (box)).first && ! ce.second.is_degenerate ()) {
          edges.push_back (ce.second);
        }

      } else {

        //  determine whether the edge is "to the left" of the box
        bool left = false;
        if (edge.p1 ().y () <= box.top () && edge.p1 ().y () >= box.bottom ()) {
          left = (edge.p1 ().x () < box.left ());
        } else if (edge.p2 ().y () <= box.top () && edge.p2 ().y () >= box.bottom ()) {
          left = (edge.p2 ().x () < box.left ());
        } else {
          tl_assert ((edge.p1 ().y () < box.bottom () && edge.p2 ().y () > box.top ()) || (edge.p2 ().y () < box.bottom () && edge.p1 ().y () > box.top ()));
          double cx = double (edge.p1 ().x ()) + (double (box.center ().y ()) - double (edge.p1 ().y ())) * double (edge.dx ()) / double (edge.dy ());
          left = (cx < box.center ().x ());
        }

        //  compute a projection on the box'es walls
        if (left) {
          edge = db::Edge (db::Point (box.p1 ().x (), edge.p1 ().y ()), db::Point (box.p1 ().x (), edge.p2 ().y ()));
        } else {
          edge = db::Edge (db::Point (box.p2 ().x (), edge.p1 ().y ()), db::Point (box.p2 ().x (), edge.p2 ().y ()));
        }

        ce = edge.clipped (box);
        if (ce.first) {
          edges.push_back (ce.second);
        }

      }

    }

  }

  //  synthesize horizontal edges at upper and lower boundary of the clip rectangle
  std::vector <std::pair <db::Coord, int> > coord_values;
  std::vector <db::Point> p1_stack;
  std::vector <db::Point> p2_stack;

  for (unsigned int l = 0; l <= 1; ++l) {

    db::Coord y = (l > 0 ? box.p2 ().y () : box.p1 ().y ());

    coord_values.clear ();
    p1_stack.clear ();
    p2_stack.clear ();

    for (std::vector <db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (e->p1 ().y () == y) {
        coord_values.push_back (std::make_pair (e->p1 ().x (), -1));
      }
      if (e->p2 ().y () == y) {
        coord_values.push_back (std::make_pair (e->p2 ().x (), 1));
      }
    }

    std::sort (coord_values.begin (), coord_values.end (), CoordSignPairCompareFunc (l > 0 ? 1 : -1));

    for (std::vector <std::pair <db::Coord, int> >::const_iterator xv = coord_values.begin (); xv != coord_values.end (); ++xv) {
      db::Point p = db::Point (xv->first, y);
      if (xv->second < 0) {
        if (! p1_stack.empty ()) {
          if (p != p1_stack.back () && p != p1_stack.back ()) {
            edges.push_back (db::Edge (p1_stack.back (), p));
          }
          p1_stack.pop_back ();
        } else {
          p2_stack.push_back (p);
        }
      } else {
        if (! p2_stack.empty ()) {
          if (p != p2_stack.back () && p != p2_stack.back ()) {
            edges.push_back (db::Edge (p, p2_stack.back ()));
          }
          p2_stack.pop_back ();
        } else {
          p1_stack.push_back (p);
        }
      }
    }

    tl_assert (p1_stack.empty ());
    tl_assert (p2_stack.empty ());
    coord_values.clear ();

  }

  //  remove all edges being vertical and coincident with the clip box ..
  std::vector <db::Edge>::iterator e_write = edges.begin ();
  for (std::vector <db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
    if (e->dx () != 0 || (e->p1 ().x () > box.p1 ().x () && e->p1 ().x () < box.p2 ().x ())) {
      *e_write = *e;
      ++e_write;
    }
  }
  edges.erase (e_write, edges.end ());

  //  and synthesize them again thus removing coincident edges ..
  
  for (unsigned int l = 0; l <= 1; ++l) {

    db::Coord x = (l > 0 ? box.p2 ().x () : box.p1 ().x ());

    coord_values.clear ();
    p1_stack.clear ();
    p2_stack.clear ();

    for (std::vector <db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (e->p1 ().x () == x) {
        coord_values.push_back (std::make_pair (e->p1 ().y (), -1));
      }
      if (e->p2 ().x () == x) {
        coord_values.push_back (std::make_pair (e->p2 ().y (), 1));
      }
    }

    std::sort (coord_values.begin (), coord_values.end (), CoordSignPairCompareFunc (l > 0 ? -1 : 1));

    for (std::vector <std::pair <db::Coord, int> >::const_iterator xv = coord_values.begin (); xv != coord_values.end (); ++xv) {
      db::Point p = db::Point (x, xv->first);
      if (xv->second < 0) {
        if (! p1_stack.empty ()) {
          if (p != p1_stack.back () && p != p1_stack.back ()) {
            edges.push_back (db::Edge (p1_stack.back (), p));
          }
          p1_stack.pop_back ();
        } else {
          p2_stack.push_back (p);
        }
      } else {
        if (! p2_stack.empty ()) {
          if (p != p2_stack.back () && p != p2_stack.back ()) {
            edges.push_back (db::Edge (p, p2_stack.back ()));
          }
          p2_stack.pop_back ();
        } else {
          p1_stack.push_back (p);
        }
      }
    }

    tl_assert (p1_stack.empty ());
    tl_assert (p2_stack.empty ());
    coord_values.clear ();

  }

  //  Use the edge processor to merge and create the output polygons.
  //  This is slow, but there is no good alternative for producing the holes and some situations are not well caught by the 
  //  previous algorithm.
  //  Anyway it is faster that a pure AND.

  db::EdgeProcessor ep;
  ep.reserve (edges.size ());
  ep.insert_sequence (edges.begin (), edges.end ());
  edges.clear ();

  PC poly_cont (clipped_poly);

  db::PolygonGenerator poly_gen (poly_cont);
  poly_gen.min_coherence (false);
  poly_gen.resolve_holes (resolve_holes);

  SimpleMerge sm; 
  ep.process (poly_gen, sm);

}

void 
clip_poly (const db::Polygon &poly, const db::Box &box, std::vector <db::Polygon> &clipped_poly, bool resolve_holes)
{
  clip_poly<db::Polygon, db::PolygonContainer> (poly, box, clipped_poly, resolve_holes);
}

void 
clip_poly (const db::SimplePolygon &poly, const db::Box &box, std::vector <db::SimplePolygon> &clipped_poly, bool resolve_holes)
{
  clip_poly<db::SimplePolygon, db::SimplePolygonContainer> (poly, box, clipped_poly, resolve_holes);
}

// ------------------------------------------------------------------------------
//  helper method: clip a cell

static void 
clip_cell (const db::Layout &layout, 
           db::cell_index_type cell_index, 
           db::Layout &target_layout, 
           db::cell_index_type target_cell_index, 
           const db::Box &clip_box,
           std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type> &variants)
{
  const db::Cell &cell = layout.cell (cell_index);
  db::Cell &target_cell = target_layout.cell (target_cell_index);

  if (cell.bbox ().inside (clip_box)) {

    if (&target_layout != &layout || cell_index != target_cell_index) {

      //  simplification of shape copy op in case of no clipping ..
      for (unsigned int l = 0; l < layout.layers (); ++l) {
        if (layout.is_valid_layer (l)) {
          target_cell.shapes (l) = cell.shapes (l);
        }
      }

      for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {

        //  instance is completely inside: nevertheless we must look up the target cell for the different layout case.
        db::CellInstArray new_inst = inst->cell_inst ();

        const db::Cell &inst_cell = layout.cell (inst->cell_index ());
        if (! inst_cell.bbox ().empty ()) {

          std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_cell.bbox ()));
          tl_assert (vmp != variants.end ());

          new_inst.object () = db::CellInst (vmp->second);

          //  TODO: keep properties 
          target_cell.insert (new_inst);

        }

      }

    }

  } else {

    tl_assert (&layout != &target_layout || target_cell_index != cell_index);

    for (unsigned int l = 0; l < layout.layers (); ++l) {
      
      if (layout.is_valid_layer (l)) {

        for (db::ShapeIterator sh = cell.shapes (l).begin_touching (clip_box, db::ShapeIterator::All); ! sh.at_end (); ++sh) {

          if (sh->is_box ()) {

            db::Box new_box = sh->box () & clip_box;
            if (! new_box.empty () && new_box.width () > 0 && new_box.height () > 0) {
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (db::BoxWithProperties (new_box, sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (new_box);
              }
            }

          } else if (sh->is_path () && sh->bbox ().inside (clip_box)) {

            db::Path path;
            sh->path (path);

            if (sh->has_prop_id ()) {
              target_cell.shapes (l).insert (db::PathRefWithProperties (db::PathRef (path, target_layout.shape_repository ()), sh->prop_id ()));
            } else {
              target_cell.shapes (l).insert (db::PathRef (path, target_layout.shape_repository ()));
            }

          } else if (sh->is_simple_polygon () || sh->is_path ()) {

            db::SimplePolygon poly;

            if (sh->is_path ()) {
              db::Path path;
              sh->path (path);
              poly = path.simple_polygon ();
            } else {
              sh->simple_polygon (poly);
            }

            if (! poly.box ().inside (clip_box)) {
              std::vector <db::SimplePolygon> clipped_polygons;
              clip_poly (poly, clip_box, clipped_polygons);
              for (std::vector <db::SimplePolygon>::const_iterator cp = clipped_polygons.begin (); cp != clipped_polygons.end (); ++cp) {
                if (sh->has_prop_id ()) {
                  target_cell.shapes (l).insert (db::SimplePolygonRefWithProperties (db::SimplePolygonRef (*cp, target_layout.shape_repository ()), sh->prop_id ()));
                } else {
                  target_cell.shapes (l).insert (db::SimplePolygonRef (*cp, target_layout.shape_repository ()));
                }
              }
            } else {
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (db::SimplePolygonRefWithProperties (db::SimplePolygonRef (poly, target_layout.shape_repository ()), sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (db::SimplePolygonRef (poly, target_layout.shape_repository ()));
              }
            }

          } else if (sh->is_polygon ()) {

            db::Polygon poly;
            sh->polygon (poly);

            if (! poly.box ().inside (clip_box)) {
              std::vector <db::Polygon> clipped_polygons;
              clip_poly (poly, clip_box, clipped_polygons);
              for (std::vector <db::Polygon>::const_iterator cp = clipped_polygons.begin (); cp != clipped_polygons.end (); ++cp) {
                if (sh->has_prop_id ()) {
                  target_cell.shapes (l).insert (db::PolygonRefWithProperties (db::PolygonRef (*cp, target_layout.shape_repository ()), sh->prop_id ()));
                } else {
                  target_cell.shapes (l).insert (db::PolygonRef (*cp, target_layout.shape_repository ()));
                }
              }
            } else {
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (db::PolygonRefWithProperties (db::PolygonRef (poly, target_layout.shape_repository ()), sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (db::PolygonRef (poly, target_layout.shape_repository ()));
              }
            }

          } else if (sh->is_text ()) {

            if (sh->bbox ().inside (clip_box)) {
              db::Text text;
              sh->text (text);
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (db::TextRefWithProperties (db::TextRef (text, target_layout.shape_repository ()), sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (db::TextRef (text, target_layout.shape_repository ()));
              }
            }

          } else {
            tl_assert (false); // invalid shape type encountered
          }

        }

      }

    }

    db::box_convert <db::CellInst> bc (layout);

    for (db::Cell::touching_iterator inst = cell.begin_touching (clip_box); ! inst.at_end (); ++inst) {

      if (inst->cell_inst ().bbox (bc).inside (clip_box)) {

        //  instance is completely inside
        db::CellInstArray new_inst = inst->cell_inst ();

        const db::Cell &inst_cell = layout.cell (inst->cell_index ());

        std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_cell.bbox ()));
        tl_assert (vmp != variants.end ());

        new_inst.object () = db::CellInst (vmp->second);

        //  TODO: keep properties 
        target_cell.insert (new_inst);

      } else {

        for (db::CellInstArray::iterator a = inst->cell_inst ().begin_touching (clip_box, bc); ! a.at_end (); ++a) {

          db::Box inst_clip_box = db::Box (clip_box.transformed (inst->cell_inst ().complex_trans (*a).inverted ()));
          const db::Cell &inst_cell = layout.cell (inst->cell_index ());

          inst_clip_box &= inst_cell.bbox ();

          if (! inst_clip_box.empty ()) {

            std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_clip_box));
            tl_assert (vmp != variants.end ());

            db::CellInstArray new_inst;

            if (inst->is_complex ()) {
              new_inst = db::CellInstArray (db::CellInst (vmp->second), inst->cell_inst ().complex_trans (*a));
            } else {
              new_inst = db::CellInstArray (db::CellInst (vmp->second), *a);
            }

            target_cell.insert (new_inst);

          }

        }

      }

    }

  }

}

// ------------------------------------------------------------------------------
//  collect_clip_boxes implementation

static void 
collect_clip_boxes (const db::Layout &layout, 
                    db::cell_index_type cell_index, 
                    unsigned int layer, 
                    const db::CplxTrans &trans, 
                    std::vector <db::Box> &boxes)
{
  const db::Cell &cell = layout.cell (cell_index);
  if (! cell.bbox (layer).empty ()) {

    //  any shapes to consider ..
    for (db::ShapeIterator sh = cell.shapes (layer).begin (db::ShapeIterator::All); ! sh.at_end (); ++sh) {
      boxes.push_back (db::Box (sh->bbox ().transformed (trans)));
    }

    for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
      for (db::CellInstArray::iterator a = inst->cell_inst ().begin (); ! a.at_end (); ++a) {
        collect_clip_boxes (layout, inst->cell_index (), layer, trans * inst->cell_inst ().complex_trans (*a), boxes);
      }
    }

  }
}

void 
collect_clip_boxes (const db::Layout &layout, 
                    db::cell_index_type cell_index, 
                    unsigned int layer, 
                    std::vector <db::Box> &boxes)
{
  collect_clip_boxes (layout, cell_index, layer, db::CplxTrans (), boxes);
}

// ------------------------------------------------------------------------------
//  Helper function for the layout clipper

static void 
collect_clip_variants (const db::Layout &layout,
                       db::Layout &target_layout,
                       db::cell_index_type cell_index,
                       const db::Box &clip_box,
                       std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type> &variants,
                       bool stable)
{
  const db::Cell &cell = layout.cell (cell_index);
  db::box_convert <db::CellInst> bc (layout);

  db::Box cell_box;
  if (stable) {
    cell_box = clip_box;
  } else {
    cell_box = cell.bbox () & clip_box;
    if (cell_box.empty ()) {
      return;
    }
  }

  std::pair <std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::iterator, bool> vmp = variants.insert (std::make_pair (std::make_pair (cell_index, cell_box), 0));
  if (vmp.second) {

    for (db::Cell::touching_iterator inst = cell.begin_touching (cell_box); ! inst.at_end (); ++inst) {
      for (db::CellInstArray::iterator a = inst->cell_inst ().begin_touching (cell_box, bc); ! a.at_end (); ++a) {
        db::Box inst_clip_box = db::Box (cell_box.transformed (inst->cell_inst ().complex_trans (*a).inverted ()));
        collect_clip_variants (layout, target_layout, inst->cell_index (), inst_clip_box, variants, false);
      }
    }

  }
}

static void 
make_clip_variants (const db::Layout &layout,
                    db::Layout &target_layout,
                    std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type> &variants)
{
  for (std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::iterator v = variants.begin (); v != variants.end (); ++v) {
    if (v->first.second != layout.cell (v->first.first).bbox () || &layout != &target_layout) {
      //  need for a new cell
      v->second = target_layout.add_cell (layout, v->first.first);
    } else {
      v->second = v->first.first;
    }
  }
}

// ------------------------------------------------------------------------------
//  clip_layout implementation

std::vector<db::cell_index_type> 
clip_layout (const Layout &layout, 
             Layout &target_layout, 
             db::cell_index_type cell_index, 
             const std::vector <db::Box> &clip_boxes,
             bool stable)
{
  std::vector<db::cell_index_type> result;

  //  since we know that we are not changing something on the cells we need as input, 
  //  we can disable updates for the target layout after doing an explicit update. 
  //  Otherwise this would cause recursion when target_layout == layout:
  layout.update();
  target_layout.start_changes ();

  try {

    //  create clip variants of cells
    std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type> variants;

    for (std::vector <db::Box>::const_iterator cbx = clip_boxes.begin (); cbx != clip_boxes.end (); ++cbx) {
      collect_clip_variants (layout, target_layout, cell_index, *cbx, variants, stable);
    }

    make_clip_variants (layout, target_layout, variants);

    //  actually do the clipping by filling the variants
    for (std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator var = variants.begin (); var != variants.end (); ++var) {
      clip_cell (layout, var->first.first, target_layout, var->second, var->first.second, variants);
    }

    //  prepare the result vector ..
    if (! stable) {

      for (std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator var = variants.begin (); var != variants.end (); ++var) {
        if (var->first.first == cell_index) {
          result.push_back (var->second);
        }
      }

    } else {

      //  We have made sure before that there is a top-level entry for each clip box that was input
      for (std::vector <db::Box>::const_iterator cbx = clip_boxes.begin (); cbx != clip_boxes.end (); ++cbx) {
        std::map <std::pair <db::cell_index_type, db::Box>, db::cell_index_type>::const_iterator var = variants.find (std::make_pair (cell_index, *cbx));
        tl_assert (var != variants.end ());
        result.push_back (var->second);
      }

    }

    //  release the under construction state
    target_layout.end_changes ();

  } catch (...) {
    //  ensure we have a end_changes call corresponding to the start_changes call.
    target_layout.end_changes ();
    throw;
  } 

  return result;

}

} // namespace db


