
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


#include "tlUnitTest.h"
#include "tlStream.h"
#include "dbLayoutToNetlist.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"
#include "dbHierProcessor.h"

// @@@
#include "dbLocalOperationUtils.h"

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

class BoolBetweenNets
  : public db::local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRef>
{
public:
  BoolBetweenNets (bool is_and, bool connected)
    : m_is_and (is_and), m_connected (connected)
  {
    //  .. nothing yet ..
  }

  db::OnEmptyIntruderHint
  on_empty_intruder_hint () const
  {
    return m_is_and ? db::Drop : db::Copy;
  }

  std::string
  description () const
  {
    return m_is_and ? tl::to_string (tr ("AND operation")) : tl::to_string (tr ("NOT operation"));
  }

  void
  do_compute_local (db::Layout *layout, const db::shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::PolygonRef> &result = results.front ();

    db::EdgeProcessor ep;

    //  NOTE: is guess we do not need to handle subject/subject interactions ...
    for (auto i = interactions.begin (); i != interactions.end (); ++i) {

      const db::PolygonRefWithProperties &subject = interactions.subject_shape (i->first);

      bool any_intruder = false;
      for (auto j = i->second.begin (); j != i->second.end () && ! any_intruder; ++j) {
        const db::PolygonRefWithProperties &other = interactions.intruder_shape (*j).second;
        any_intruder = ((other.properties_id () == subject.properties_id ()) == m_connected);
      }

      if (! any_intruder) {

        //  shortcut (not: keep, and: drop)
        if (! m_is_and) {
          result.insert (subject);
        }

      } else {

        ep.clear ();

        for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, 0);
        }

        for (auto j = i->second.begin (); j != i->second.end (); ++j) {

          const db::PolygonRefWithProperties &other = interactions.intruder_shape (*j).second;
          if ((other.properties_id () == subject.properties_id ()) == m_connected) {
            for (db::PolygonRef::polygon_edge_iterator e = other.begin_edge (); ! e.at_end(); ++e) {
              ep.insert (*e, 1);
            }
          }

        }

        db::BooleanOp op (m_is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
        db::PolygonRefGenerator pr (layout, result);
        db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
        db::PolygonGenerator pg (splitter, true, true);
        ep.set_base_verbosity (50);
        ep.process (pg, op);

      }

    }

  }

private:
  bool m_is_and;
  bool m_connected;
};


TEST(0_Develop)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int poly       = define_layer (ly, lmap, 7);
  unsigned int cont       = define_layer (ly, lmap, 14);
  unsigned int metal1     = define_layer (ly, lmap, 15);
  unsigned int via1       = define_layer (ly, lmap, 16);
  unsigned int metal2     = define_layer (ly, lmap, 17);
  unsigned int via2       = define_layer (ly, lmap, 18);
  unsigned int metal3     = define_layer (ly, lmap, 19);
  unsigned int via3       = define_layer (ly, lmap, 20);
  unsigned int metal4     = define_layer (ly, lmap, 21);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    // @@@ std::string fn (tl::testdata ());
    // @@@ fn = tl::combine_path (fn, "algo");
    // @@@ fn = tl::combine_path (fn, "device_extract_l1.gds");

    std::string fn ("/home/matthias/klayout/testdata/laurent_ANA_DRIVE7/ANA_DRIVE7.gds"); // @@@

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Region> rcont (l2n.make_polygon_layer (cont, "cont"));
  std::unique_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Region> rvia2 (l2n.make_polygon_layer (via1, "via2"));
  std::unique_ptr<db::Region> rmetal3 (l2n.make_polygon_layer (metal2, "metal3"));
  std::unique_ptr<db::Region> rvia3 (l2n.make_polygon_layer (via1, "via3"));
  std::unique_ptr<db::Region> rmetal4 (l2n.make_polygon_layer (metal2, "metal4"));

  //  net extraction

  //  Intra-layer
  l2n.connect (*rpoly);
  l2n.connect (*rcont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (*rvia2);
  l2n.connect (*rmetal3);
  l2n.connect (*rvia3);
  l2n.connect (*rmetal4);
  //  Inter-layer
  l2n.connect (*rpoly,      *rcont);
  l2n.connect (*rcont,       *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rmetal2,    *rvia2);
  l2n.connect (*rvia2,      *rmetal3);
  l2n.connect (*rmetal3,    *rvia3);
  l2n.connect (*rvia3,      *rmetal4);

  l2n.extract_netlist ();

  //  ....

  db::Layout ly2;
  ly2.dbu (l2n.internal_layout ()->dbu ());
  db::Cell &top2 = ly2.cell (ly2.add_cell (ly.cell_name (tc.cell_index ())));

  db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, false /*without device cells*/);

  std::map<unsigned int, const db::Region *> lmap_write;
  unsigned int wpoly, wcont, wmetal1, wvia1, wmetal2;
  lmap_write [wpoly = ly2.insert_layer (db::LayerProperties (7, 0))] = l2n.layer_by_name ("poly");
  lmap_write [wcont = ly2.insert_layer (db::LayerProperties (14, 0))] = l2n.layer_by_name ("cont");
  lmap_write [wmetal1 = ly2.insert_layer (db::LayerProperties (15, 0))] = l2n.layer_by_name ("metal1");
  lmap_write [wvia1 = ly2.insert_layer (db::LayerProperties (16, 0))] = l2n.layer_by_name ("via1");
  lmap_write [wmetal2 = ly2.insert_layer (db::LayerProperties (17, 0))] = l2n.layer_by_name ("metal2");

  l2n.build_all_nets (cm, ly2, lmap_write, "NET_", db::LayoutToNetlist::NetIDOnly, tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "SC_", 0 /*don't produce devices*/);

  unsigned int out = ly2.insert_layer (db::LayerProperties (1000, 0));

  db::local_processor<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRef> proc (&ly2, &top2);
  BoolBetweenNets n2n (true, true);
  proc.run (&n2n, wmetal1, wmetal2, out);

  //  @@@ may no work correctly as we have used fake prop ids
  {
    db::SaveLayoutOptions options;

    std::string fn ("net_out.gds"); // @@@

    tl::OutputStream stream (fn);
    db::Writer writer (options);
    writer.write (ly2, stream);
  }
}
