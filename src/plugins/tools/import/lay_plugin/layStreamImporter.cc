
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "layStreamImporter.h"
#include "layLayoutView.h"
#include "tlStream.h"
#include "tlString.h"
#include "tlString.h"
#include "tlLog.h"
#include "dbEdgeProcessor.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

#include <QDir>
#include <QMessageBox>
#include <QApplication>

#include <math.h>

namespace lay
{

// ---------------------------------------------------------------------------------------
//  Implementation of StreamImporter

StreamImporter::StreamImporter ()
  : m_cell_mapping (StreamImportData::Simple), m_layer_mapping (StreamImportData::Original)
{
  // .. nothing yet ..
}

void 
StreamImporter::read (db::Layout &target, db::cell_index_type target_cell_index, std::vector <unsigned int> &new_layers)
{
  //  Clear the undo buffer
  if (target.manager () && !target.manager ()->transacting ()) {
    target.manager ()->clear ();
  }

  tl::log << tl::to_string (QObject::tr ("Importing stream data"));
  // tl::Progress m_progress (tl::to_string (QObject::tr ("Importing stream data")), m_files.size (), 1);

  //  TODO: this code should be available otherwise ...
  //  derive the actual global transformation from the reference points
  db::DCplxTrans global_trans (m_global_trans);
  if (! m_reference_points.empty ()) {

    db::DPoint p1_pcb = m_reference_points[0].first;
    db::DPoint p1_ly = m_reference_points[0].second;

    if (m_reference_points.size () > 1) {

      db::DPoint p2_pcb = m_reference_points[1].first;
      db::DPoint p2_ly = m_reference_points[1].second;

      db::DVector d12_pcb = (p2_pcb - p1_pcb) * (1.0 / p2_pcb.distance (p1_pcb));
      db::DVector d12_ly = (p2_ly - p1_ly) * (1.0 / p2_ly.distance (p1_ly));

      int ru = -1;
      int rm = -1;
      for (int f = 0; f < 8; ++f) {
        db::DVector d12 = db::DTrans (f) * d12_pcb;
        if ((d12 - d12_ly).length () < 0.1) {
          if (f < 4) {
            ru = f;
          } else {
            rm = f;
          }
        }
      }

      if (ru < 0 || rm < 0) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unable to deduce rotation from reference points p1 and p2 (imported and existing layout)")));
      }

      if (m_reference_points.size () > 2) {

        db::DPoint p3_pcb = m_reference_points[2].first;
        db::DPoint p3_ly = m_reference_points[2].second;

        db::DVector d13_pcb = (p3_pcb - p1_pcb) * (1.0 / p3_pcb.distance (p1_pcb));
        db::DVector d13_ly = (p3_ly - p1_ly) * (1.0 / p3_ly.distance (p1_ly));

        double vp_pcb = d13_pcb.x () * d12_pcb.y () - d13_pcb.y () * d12_pcb.x ();
        double vp_gds = d13_ly.x () * d12_ly.y () - d13_ly.y () * d12_ly.x ();

        if (vp_pcb * vp_gds < 0.0) {
          global_trans = db::DCplxTrans (db::DFTrans (rm));
        } else {
          global_trans = db::DCplxTrans (db::DFTrans (ru));
        }

      } else {

        if (global_trans.is_mirror ()) {
          global_trans = db::DCplxTrans (db::DFTrans (rm));
        } else {
          global_trans = db::DCplxTrans (db::DFTrans (ru));
        }

      }

    }

    global_trans = db::DCplxTrans (p1_ly - (db::DPoint () + global_trans.disp ())) * global_trans * db::DCplxTrans (db::DPoint () - p1_pcb);

  }

  //  Issue a warning, if the transformation is not ortho etc.
  if (fabs (global_trans.mag () - floor (global_trans.mag () + 0.5)) > 1e-6 || ! global_trans.is_ortho ()) {

    if (QMessageBox::warning (QApplication::activeWindow (),
      QObject::tr ("Complex Transformation"),
      tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("The specified transformation (%s) is complex.\nGrid snapping to the database unit grid can occur and\neffectively alter the geometry of the layout.\nPress 'Ok' to continue.")), global_trans.to_string ())),
      QMessageBox::Ok | QMessageBox::Cancel,
      QMessageBox::Ok) != QMessageBox::Ok) {
      return;
    }

  }

  //  TODO: Currently no merging is provided for non-unity transformations
  if (m_cell_mapping == StreamImportData::Merge && ! global_trans.equal (db::DCplxTrans ())) {

    if (QMessageBox::warning (QApplication::activeWindow (),
      QObject::tr ("Merge Mode Is Not Available"),
      tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("Merge mode is not supported for the specified transformation (%s).\nSimple mode will be used instead.\nPress 'Ok' to continue.")), global_trans.to_string ())),
      QMessageBox::Ok | QMessageBox::Cancel,
      QMessageBox::Ok) != QMessageBox::Ok) {
      return;
    }

    m_cell_mapping = StreamImportData::Simple;

  }

  for (size_t file_index = 0; file_index < m_files.size (); ++file_index) {

    std::string file = m_files [file_index];

    //  Prepare the layout to read
    db::Layout source;

    //  Load the layout
    {
      tl::InputStream stream (file);
      db::Reader reader (stream);

      tl::log << tl::to_string (QObject::tr ("Loading file: ")) << file;
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Loading file: ")) + file);
      reader.read (source, m_options);
    }

    //  Locate the top cell in the source file
    db::cell_index_type source_topcell;
    std::vector <db::cell_index_type> source_cells;

    if (m_cell_mapping != StreamImportData::Extra || !m_topcell.empty ()) {

      if (m_topcell.empty ()) {

        db::Layout::top_down_const_iterator t = source.begin_top_down ();
        if (t == source.end_top_down ()) {
          throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Source layout '%s' does not have a top cell")), file));
        }

        source_topcell = *t;

        ++t;
        if (t != source.end_top_cells ()) {
          throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Source layout '%s' does not have a unique top cell - specify one explicitly")), file));
        }

      } else {

        std::pair<bool, db::cell_index_type> t = source.cell_by_name (m_topcell.c_str ());
        if (! t.first) {
          throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Source layout '%s' does not have a cell named '%s'")), file, m_topcell));
        }

        source_topcell = t.second;

      }

      source_cells.push_back (source_topcell);

    } else {

      // collect source cells
      for (db::Layout::top_down_const_iterator t = source.begin_top_down (); t != source.end_top_cells (); ++t) {
        source_cells.push_back (*t);
      }

    }

    //  Create a layer map
    std::map <unsigned int, unsigned int> layer_map;
    for (db::Layout::layer_iterator l = source.begin_layers (); l != source.end_layers (); ++l) {

      db::LayerProperties lp (*(*l).second);
      if (m_layer_mapping == StreamImportData::Offset) {
        lp = m_layer_offset.apply (lp);
      }

      bool layer_found = false;
      for (db::Layout::layer_iterator ll = target.begin_layers (); ll != target.end_layers () && ! layer_found; ++ll) {
        if ((*ll).second->log_equal (lp)) {
          layer_map.insert (std::make_pair ((*l).first, (*ll).first));
          layer_found = true;
        }
      }

      if (! layer_found) {
        unsigned int new_layer = target.insert_layer (lp);
        layer_map.insert (std::make_pair ((*l).first, new_layer));
        new_layers.push_back (new_layer);
      }

    }

    //  Computes the final global transformation
    db::DCplxTrans gt = global_trans;

    //  Create a cell map
    std::map <db::cell_index_type, db::cell_index_type> cell_map;

    if (m_cell_mapping == StreamImportData::Simple) {

      cell_map.insert (std::make_pair (source_topcell, target_cell_index));

    } else if (m_cell_mapping == StreamImportData::Extra) {

      //  create new top cells for each source top cell
      for (std::vector<db::cell_index_type>::const_iterator t = source_cells.begin (); t != source_cells.end (); ++t) {
        db::cell_index_type new_top = target.add_cell (source.cell_name (*t));
        cell_map.insert (std::make_pair (*t, new_top));
      }

    } else if (m_cell_mapping == StreamImportData::Instantiate) {

      //  Create a new top cell for importing into and use the cell reference to produce the first part of the transformation

      db::cell_index_type new_top = target.add_cell (source.cell_name (source_topcell));
      cell_map.insert (std::make_pair (source_topcell, new_top));

      db::ICplxTrans gt_dbu = db::VCplxTrans (1.0 / target.dbu ()) * gt * db::CplxTrans (source.dbu ());
      target.cell (target_cell_index).insert (db::CellInstArray (new_top, gt_dbu * db::ICplxTrans (1.0 / gt.mag ())));

      gt = db::DCplxTrans (gt.mag ());

    } else if (m_cell_mapping == StreamImportData::Merge) {

      //  Create the cell mapping
      //  TODO: Currently no merging is provided for non-unity transformations
      tl_assert (gt.equal (db::DCplxTrans ()));

      db::CellMapping cm;
      cm.create_from_geometry (target, target_cell_index, source, source_topcell);
      cell_map.insert (cm.begin (), cm.end ());

    }

    //  And actually merge
    db::merge_layouts (target, source, db::VCplxTrans (1.0 / target.dbu ()) * gt * db::CplxTrans (source.dbu ()), source_cells, cell_map, layer_map);

  }
}

}

