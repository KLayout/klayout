
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


#include "laySearchReplaceDialog.h"
#include "laySearchReplacePropertiesWidgets.h"
#include "laySearchReplaceConfigPage.h"
#include "layConfigurationDialog.h"
#include "layQtTools.h"

#include "layMainWindow.h"
#include "tlExceptions.h"
#include "layMarker.h"
#include "layFileDialog.h"
#include "layQtTools.h"

#include "dbLayoutQuery.h"
#include "dbLayoutUtils.h"
#include "tlLog.h"
#include "tlProgress.h"
#include "tlTimer.h"
#include "rdbUtils.h"
#include "edtService.h"

#include <QInputDialog>
#include <QHeaderView>
#include <QRegExp>
#include <QClipboard>
#include <QMimeData>

#include <fstream>

namespace lay
{

// --------------------------------------------------------------------------------
//  SearchReplaceResults implementation

SearchReplaceResults::SearchReplaceResults ()
  : m_data_columns (1), m_last_column_count (0), m_has_more (false)
{
  //  .. nothing yet ..
}

void
SearchReplaceResults::has_more (bool hm)
{
  m_has_more = hm;
}

void 
SearchReplaceResults::clear ()
{
  m_data_result.clear ();
  m_shape_result.clear ();
  m_inst_result.clear ();
  m_cell_result.clear ();
  m_data_columns = 1;
  m_has_more = false;
}

void
SearchReplaceResults::set_data_column_headers (const tl::Variant &v)
{
  m_data_column_headers = v;
  if (v.is_list ()) {
    m_data_columns = std::max (v.get_list ().size (), m_data_columns);
  }
}

void 
SearchReplaceResults::push_back (const tl::Variant &v)
{
  m_data_result.push_back (v);
  if (v.is_list ()) {
    m_data_columns = std::max (v.get_list ().size (), m_data_columns);
  }
}

void 
SearchReplaceResults::push_back (const QueryShapeResult &v)
{
  m_shape_result.push_back (v);
}

void 
SearchReplaceResults::push_back (const QueryInstResult &v)
{
  m_inst_result.push_back (v);
}

void 
SearchReplaceResults::push_back (const QueryCellResult &v)
{
  m_cell_result.push_back (v);
}

void 
SearchReplaceResults::begin_changes (const db::Layout *layout)
{
#if QT_VERSION >= 0x040600
  beginResetModel ();
#endif

  //  In order to be independent from the layout object we save the mapping tables here
  m_lp_map.clear ();
  m_cellname_map.clear ();

  if (layout) {

    for (db::cell_index_type ci = 0; ci < layout->cells (); ++ci) {
      if (layout->is_valid_cell_index (ci)) {
        m_cellname_map.insert (std::make_pair (ci, std::string (layout->cell_name (ci))));
      }
    }

    for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {
      m_lp_map.insert (std::make_pair ((*l).first, *(*l).second));
    }

  }
}

void 
SearchReplaceResults::end_changes ()
{
#if QT_VERSION >= 0x040600
  endResetModel ();
#else
  reset ();
#endif
}

size_t
SearchReplaceResults::size () const
{
  return std::max (std::max (m_cell_result.size (), m_data_result.size ()), std::max (m_shape_result.size (), m_inst_result.size ())) + (m_has_more ? 1 : 0);
}

int 
SearchReplaceResults::columnCount (const QModelIndex & /*parent*/) const
{
  //  Note: keep last column count for empty model to avoid resize events for the header
  if (! m_data_result.empty ()) {
    m_last_column_count = int (m_data_columns);
  } else if (! m_shape_result.empty ()) {
    m_last_column_count = 5;
  } else if (! m_inst_result.empty ()) {
    m_last_column_count = 4;
  } else if (! m_cell_result.empty ()) {
    m_last_column_count = 2;
  }
  return m_last_column_count;
}

QVariant
SearchReplaceResults::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole) {
    if (! m_data_result.empty ()) {
      if (m_data_column_headers.is_list ()) {
        if (section < int (m_data_column_headers.get_list ().size ())) {
          return QVariant (m_data_column_headers.get_list () [section].to_string ());
        } else {
          return QVariant (QString ());
        }
      } else if (section == 0) {
        return QVariant (QObject::tr ("Value"));
      } else {
        return QVariant (QString ());
      }
    } else if (! m_shape_result.empty ()) {
      if (section == 0) {
        return QVariant (QObject::tr ("Shape"));
      } else if (section == 1) {
        return QVariant (QObject::tr ("Layer"));
      } else if (section == 2) {
        return QVariant (QObject::tr ("Cell"));
      } else if (section == 3) {
        return QVariant (QObject::tr ("As Seen in Top"));
      } else if (section == 4) {
        return QVariant (QObject::tr ("Top Cell"));
      }
    } else if (! m_inst_result.empty ()) {
      if (section == 0) {
        return QVariant (QObject::tr ("Instance"));
      } else if (section == 1) {
        return QVariant (QObject::tr ("Parent Cell"));
      } else if (section == 2) {
        return QVariant (QObject::tr ("As Seen in Top"));
      } else if (section == 3) {
        return QVariant (QObject::tr ("Top Cell"));
      }
    } else if (! m_cell_result.empty ()) {
      if (section == 0) {
        return QVariant (QObject::tr ("Cell"));
      } else if (section == 1) {
        return QVariant (QObject::tr ("Parent Cell"));
      }
    }
  }
  return QVariant ();
}

static std::string instance_to_string (const db::Instance &inst, const db::ICplxTrans &t = db::ICplxTrans ())
{
  if (inst.is_null ()) {
    return std::string ();
  }

  db::CellInstArray ci = inst.cell_inst ();

  std::string r;
  double dbu = 1.0;
  if (inst.instances () && inst.instances ()->cell () && inst.instances ()->cell ()->layout ()) {
    r = inst.instances ()->cell ()->layout ()->cell (ci.object ().cell_index ()).get_qualified_name ();
    dbu = inst.instances ()->cell ()->layout ()->dbu ();
  }

  r += " " + (db::CplxTrans (dbu) * t * ci.complex_trans () * db::CplxTrans (1.0 / dbu)).to_string ();

  db::Vector a, b;
  unsigned long amax, bmax;
  if (ci.is_regular_array (a, b, amax, bmax)) {
    r += " array=(" + (db::CplxTrans (dbu) * t * a).to_string () + "," + (db::CplxTrans (dbu) * t * b).to_string () + " " + tl::to_string (amax) + "x" + tl::to_string (bmax) + ")";
  }

  return r;
}

static std::string shape_to_string (const db::Shape &shape, const db::ICplxTrans &t = db::ICplxTrans ())
{
  double dbu = 1.0;
  if (shape.shapes () && shape.shapes ()->cell () && shape.shapes ()->cell ()->layout ()) {
    dbu = shape.shapes ()->cell ()->layout ()->dbu ();
  }

  if (shape.is_text ()) {

    db::Text text;
    shape.text (text);

    return "text " + text.transformed (db::CplxTrans (dbu) * t).to_string ();

  } else if (shape.is_polygon ()) {

    db::Polygon polygon;
    shape.polygon (polygon);

    return "polygon " + polygon.transformed (db::CplxTrans (dbu) * t).to_string ();

  } else if (shape.is_path ()) {

    db::Path path;
    shape.path (path);

    return "path " + path.transformed (db::CplxTrans (dbu) * t).to_string ();

  } else if (shape.is_box ()) {

    db::Box box;
    shape.box (box);

    if (t.is_ortho ()) {
      return "box " + box.transformed (db::CplxTrans (dbu) * t).to_string ();
    } else {
      return "polygon " + db::Polygon (box).transformed (db::CplxTrans (dbu) * t).to_string ();
    }

  } else {
    return std::string ();
  }
}

QVariant 
SearchReplaceResults::data (const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {

    if (index.row () < int (m_data_result.size ())) {

      const tl::Variant &v = m_data_result [index.row ()];
      if (index.column () == 0 && ! v.is_list ()) {

        return QVariant (tl::to_qstring (v.to_string ()));

      } else if (index.column () < int (v.get_list ().size ())) {

        return QVariant (tl::to_qstring (v.get_list () [index.column ()].to_string ()));

      } 

    } else if (index.row () < int (m_shape_result.size ())) {

      const QueryShapeResult &result = m_shape_result [index.row ()];

      if (index.column () == 0) {

        return QVariant (tl::to_qstring (shape_to_string (result.shape)));

      } else if (index.column () == 1) {

        unsigned int layer = result.layer_index;
        std::map<unsigned int, db::LayerProperties>::const_iterator lm = m_lp_map.find (layer);
        if (lm != m_lp_map.end ()) {
          return QVariant (tl::to_qstring (lm->second.to_string ()));
        }

      } else if (index.column () == 2) {

        db::cell_index_type cell_index = result.cell_index;
        std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (cell_index);
        if (cn != m_cellname_map.end ()) {
          return QVariant (tl::to_qstring (cn->second));
        }

      } else if (index.column () == 3) {

        if (result.trans != db::ICplxTrans ()) {
          return QVariant (tl::to_qstring (shape_to_string (result.shape, result.trans)));
        }

      } else if (index.column () == 4) {

        if (result.initial_cell_index != result.cell_index) {
          db::cell_index_type cell_index = result.initial_cell_index;
          std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (cell_index);
          if (cn != m_cellname_map.end ()) {
            return QVariant (tl::to_qstring (cn->second));
          }
        }

      } 

    } else if (index.row () < int (m_inst_result.size ())) {

      const QueryInstResult &result = m_inst_result [index.row ()];

      if (index.column () == 0) {

        return QVariant (tl::to_qstring (instance_to_string (result.inst)));

      } else if (index.column () == 1) {

        db::cell_index_type cell_index = result.cell_index;
        std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (cell_index);
        if (cn != m_cellname_map.end ()) {
          return QVariant (tl::to_qstring (cn->second));
        }

      } else if (index.column () == 2) {

        if (result.trans != db::ICplxTrans ()) {
          return QVariant (tl::to_qstring (instance_to_string (result.inst, result.trans)));
        }

      } else if (index.column () == 3) {

        if (result.initial_cell_index != result.cell_index) {
          db::cell_index_type cell_index = result.initial_cell_index;
          std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (cell_index);
          if (cn != m_cellname_map.end ()) {
            return QVariant (tl::to_qstring (cn->second));
          }
        }

      } 

    } else if (index.row () < int (m_cell_result.size ())) {

      if (index.column () == 0 || index.column () == 1) {

        db::cell_index_type cell_index;
        if (index.column () == 0) {
          cell_index = m_cell_result [index.row ()].cell_index;
        } else {
          cell_index = m_cell_result [index.row ()].parent_cell_index;
        }

        std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (cell_index);
        if (cn != m_cellname_map.end ()) {
          return QVariant (tl::to_qstring (cn->second));
        }

      } 

    } else if (m_has_more) {

      if (index.column () == 0) {
        return QVariant (tl::to_qstring ("..."));
      }

    }

  }

  return QVariant ();
}

Qt::ItemFlags 
SearchReplaceResults::flags (const QModelIndex & /*index*/) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool
SearchReplaceResults::hasChildren (const QModelIndex &parent) const
{
  return ! parent.isValid ();
}

bool
SearchReplaceResults::hasIndex (int row, int /*column*/, const QModelIndex &parent) const
{
  return ! parent.isValid () && row < int (size ());
}

QModelIndex 
SearchReplaceResults::index (int row, int column, const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return createIndex (row, column);
  } else {
    return QModelIndex ();
  }
}

QModelIndex 
SearchReplaceResults::parent (const QModelIndex & /*index*/) const
{
  return QModelIndex ();
}

int 
SearchReplaceResults::rowCount (const QModelIndex &parent) const
{
  return parent.isValid () ? 0 : int (size ());
}

static std::string 
escape_csv (const std::string &s)
{
  if (s.find (",") != std::string::npos) {
    std::string r = "\"";
    for (const char *c = s.c_str (); *c; ++c) {
      if (*c == '\"') {
        r += "\"\"";
      } else {
        r += *c;
      }
    }
    r += "\"";
    return r;
  } else {
    return s;
  }
}

void
SearchReplaceResults::select_items (lay::LayoutViewBase *view, int cv_index, const std::set<int> *rows)
{
  const lay::CellView &cv = view->cellview (cv_index);
  const db::Layout &layout = cv->layout ();

  std::vector<edt::Service::objects::value_type> sel;

  int n_rows = int (size ());
  for (int r = 0; r < n_rows; ++r) {

    if (rows && rows->find (r) == rows->end ()) {
      continue;
    }

    if (r < int (shapes ().size ())) {

      const SearchReplaceResults::QueryShapeResult &sr = shapes () [r];
      if (! sr.shape.is_null () && layout.is_valid_cell_index (sr.initial_cell_index)) {

        std::vector<db::InstElement> path;
        if (db::find_path (layout, sr.initial_cell_index, cv.cell_index (), path)) {

          sel.push_back (edt::Service::objects::value_type ());
          sel.back ().set_cv_index (cv_index);
          sel.back ().set_layer (sr.layer_index);
          sel.back ().set_shape (sr.shape);
          sel.back ().set_topcell (cv.cell_index ());
          sel.back ().assign_path (path.begin (), path.end ());
          if (sr.inst_elements.has_value ()) {
            sel.back ().add_path (sr.inst_elements->begin (), sr.inst_elements->end ());
          }

        }

      }

    } else if (r < int (instances ().size ())) {

      const SearchReplaceResults::QueryInstResult &ir = instances () [r];
      if (! ir.inst.is_null () && layout.is_valid_cell_index (ir.initial_cell_index)) {

        std::vector<db::InstElement> path;
        if (db::find_path (layout, ir.initial_cell_index, cv.cell_index (), path)) {

          sel.push_back (edt::Service::objects::value_type ());
          sel.back ().set_cv_index (cv_index);
          sel.back ().set_topcell (cv.cell_index ());
          sel.back ().assign_path (path.begin (), path.end ());
          if (ir.inst_elements.has_value ()) {
            sel.back ().add_path (ir.inst_elements->begin (), ir.inst_elements->end ());
          }

        }

      }

    }

  }

  edt::set_object_selection (view, sel);
}

void
SearchReplaceResults::export_csv_to_clipboard (const std::set<int> *rows)
{
  tl::OutputMemoryStream buffer;

  {
    tl::OutputStream os (buffer, true /* as text */);
    export_csv (os, rows);
  }

#if QT_VERSION >= 0x050000
  QClipboard *clipboard = QGuiApplication::clipboard();
#else
  QClipboard *clipboard = QApplication::clipboard();
#endif
  QMimeData *data = new QMimeData ();
  data->setData (QString::fromUtf8 ("text/csv"), QByteArray (buffer.data (), buffer.size ()));
  data->setText (QString::fromUtf8 (buffer.data (), buffer.size ()));
  clipboard->setMimeData (data);
}

void
SearchReplaceResults::export_csv (const std::string &file, const std::set<int> *rows)
{
  tl::OutputStream os (file, tl::OutputStream::OM_Auto, true /* as text */);
  export_csv (os, rows);
}

void
SearchReplaceResults::export_csv (tl::OutputStream &os, const std::set<int> *rows)
{
  QModelIndex parent;

  size_t n_columns = columnCount (parent);
  size_t n_rows = rowCount (parent);

  for (size_t c = 0; c < n_columns; ++c) {
    if (c) {
      os << ",";
    }
    os << escape_csv (tl::to_string (headerData (int (c), Qt::Horizontal, Qt::DisplayRole).toString ()));
  }
  os << "\n";

  for (size_t r = 0; r < n_rows; ++r) {

    if (! rows || rows->find (r) != rows->end ()) {

      for (size_t c = 0; c < n_columns; ++c) {
        if (c) {
          os << ",";
        }
        //  TODO: optimize
        os << escape_csv (tl::to_string (data (index (int (r), int (c), parent), Qt::DisplayRole).toString ()));
      }

      os << "\n";

    }

  }
}

void  
SearchReplaceResults::export_layout (db::Layout &layout, const std::set<int> *rows)
{
  if (! m_data_result.empty () || ! m_cell_result.empty () || ! m_inst_result.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Query produces something other than shapes - such results cannot be converted to layout currently.")));
  }

  db::Cell &top_cell = layout.cell (layout.add_cell ("RESULTS"));
  db::LayerMap insert_lm;

  int r = 0;
  for (std::vector<QueryShapeResult>::const_iterator s = m_shape_result.begin (); s != m_shape_result.end (); ++s, ++r) {

    if (rows && rows->find (r) == rows->end ()) {
      continue;
    }

    unsigned int layer = s->layer_index;
    std::map<unsigned int, db::LayerProperties>::const_iterator lm = m_lp_map.find (layer);
    if (lm != m_lp_map.end ()) {

      std::pair<bool, unsigned int> ll = insert_lm.first_logical (lm->second);
      if (! ll.first) {
        layer = layout.insert_layer (lm->second);
        insert_lm.map (lm->second, layer, lm->second);
      } else {
        layer = ll.second;
      }

      tl::ident_map<db::Layout::properties_id_type> pm;
      top_cell.shapes (layer).insert (s->shape, db::ICplxTrans (s->trans), pm);

    }

  }
}

void  
SearchReplaceResults::export_rdb (rdb::Database &rdb, double dbu, const std::set<int> *rows)
{
  if (! m_cell_result.empty ()) {

    throw tl::Exception (tl::to_string (QObject::tr ("Query produces cells - such results cannot be exported to a report database.")));

  } else if (! m_data_result.empty ()) {

    rdb::Category *cat = rdb.create_category ("data");
    rdb::Cell *cell = rdb.create_cell (rdb.top_cell_name ());

    int r = 0;
    for (std::vector<tl::Variant>::const_iterator v = m_data_result.begin (); v != m_data_result.end (); ++v, ++r) {

      if (rows && rows->find (r) == rows->end ()) {
        continue;
      }

      rdb::Item *item = rdb.create_item (cell->id (), cat->id ());

      if (! v->is_list ()) {
        rdb::add_item_value (item, *v, dbu);
      } else {
        for (std::vector<tl::Variant>::const_iterator i = v->get_list ().begin (); i != v->get_list ().end (); ++i) {
          rdb::add_item_value (item, *i, dbu);
        }
      } 

    }

  } else if (! m_inst_result.empty ()) {

    rdb::Category *cat = rdb.create_category ("instances");
    rdb::Cell *rdb_top_cell = rdb.create_cell (rdb.top_cell_name ());

    std::map<std::pair<db::cell_index_type, db::CplxTrans>, rdb::Cell *> cells_by_variant;
    for (std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.begin (); cn != m_cellname_map.end (); ++cn) {
      if (cn->second == rdb.top_cell_name ()) {
        cells_by_variant.insert (std::make_pair (std::make_pair (cn->first, db::CplxTrans ()), rdb_top_cell));
      }
    }

    int r = 0;
    for (std::vector<QueryInstResult>::const_iterator i = m_inst_result.begin (); i != m_inst_result.end (); ++i, ++r) {

      if (rows && rows->find (r) == rows->end ()) {
        continue;
      }

      std::map<std::pair<db::cell_index_type, db::CplxTrans>, rdb::Cell *>::const_iterator v = cells_by_variant.find (std::make_pair (i->cell_index, db::CplxTrans (i->trans)));
      if (v == cells_by_variant.end ()) {
        std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (i->cell_index);
        if (cn != m_cellname_map.end ()) {
          v = cells_by_variant.insert (std::make_pair (std::make_pair (i->cell_index, db::CplxTrans (i->trans)), rdb.create_cell (cn->second))).first;
          v->second->references ().insert (rdb::Reference (db::CplxTrans (dbu) * i->trans * db::VCplxTrans (1.0 / dbu), rdb_top_cell->id ()));
        }
      }
       
      if (v != cells_by_variant.end ()) {

        db::Box inst_bbox = i->inst.bbox ();
        rdb::Item *item = rdb.create_item (v->second->id (), cat->id ());
        item->add_value (inst_bbox.transformed (db::CplxTrans (dbu)));
        item->add_value (i->inst.to_string (true));

      }

    }

  } else if (! m_shape_result.empty ()) {

    rdb::Cell *rdb_top_cell = rdb.create_cell (rdb.top_cell_name ());

    //  create categories
    std::map<unsigned int, rdb::Category *> categories;

    int r = 0;
    for (std::vector<QueryShapeResult>::const_iterator s = m_shape_result.begin (); s != m_shape_result.end (); ++s, ++r) {

      if (rows && rows->find (r) == rows->end ()) {
        continue;
      }

      unsigned int layer = s->layer_index;
      std::map<unsigned int, db::LayerProperties>::const_iterator lm = m_lp_map.find (layer);
      if (lm != m_lp_map.end ()) {
        std::map<unsigned int, rdb::Category *>::const_iterator cm = categories.find (layer);
        if (cm == categories.end ()) {
          rdb::Category *cat = rdb.create_category (lm->second.to_string ()); 
          categories.insert (std::make_pair (layer, cat));
        }
      }

    }

    std::map<std::pair<db::cell_index_type, db::CplxTrans>, rdb::Cell *> cells_by_variant;
    for (std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.begin (); cn != m_cellname_map.end (); ++cn) {
      if (cn->second == rdb.top_cell_name ()) {
        cells_by_variant.insert (std::make_pair (std::make_pair (cn->first, db::CplxTrans ()), rdb_top_cell));
      }
    }

    r = 0;
    for (std::vector<QueryShapeResult>::const_iterator s = m_shape_result.begin (); s != m_shape_result.end (); ++s, ++r) {

      if (rows && rows->find (r) == rows->end ()) {
        continue;
      }

      unsigned int layer = s->layer_index;
      std::map<unsigned int, rdb::Category *>::const_iterator cm = categories.find (layer);
      if (cm != categories.end ()) {

        std::map<std::pair<db::cell_index_type, db::CplxTrans>, rdb::Cell *>::const_iterator v = cells_by_variant.find (std::make_pair (s->cell_index, db::CplxTrans (s->trans)));
        if (v == cells_by_variant.end ()) {
          std::map<db::cell_index_type, std::string>::const_iterator cn = m_cellname_map.find (s->cell_index);
          if (cn != m_cellname_map.end ()) {
            v = cells_by_variant.insert (std::make_pair (std::make_pair (s->cell_index, db::CplxTrans (s->trans)), rdb.create_cell (cn->second))).first;
            v->second->references ().insert (rdb::Reference (db::CplxTrans (dbu) * s->trans * db::VCplxTrans (1.0 / dbu), rdb_top_cell->id ()));
          }
        }
         
        if (v != cells_by_variant.end ()) {

          if (s->shape.is_polygon ()) {

            db::Polygon poly;
            s->shape.polygon (poly);
            rdb::Item *item = rdb.create_item (v->second->id (), cm->second->id ());
            item->values ().add (new rdb::Value <db::DPolygon> (poly.transformed (db::CplxTrans (dbu))));

          } else if (s->shape.is_path ()) {

            db::Path path;
            s->shape.path (path);
            rdb::Item *item = rdb.create_item (v->second->id (), cm->second->id ());
            item->values ().add (new rdb::Value <db::DPath> (path.transformed (db::CplxTrans (dbu))));

          } else if (s->shape.is_box ()) {

            db::Box box;
            s->shape.box (box);
            rdb::Item *item = rdb.create_item (v->second->id (), cm->second->id ());
            item->values ().add (new rdb::Value <db::DBox> (box.transformed (db::CplxTrans (dbu))));

          } else if (s->shape.is_text ()) {

            db::Text text;
            s->shape.text (text);
            rdb::Item *item = rdb.create_item (v->second->id (), cm->second->id ());
            item->values ().add (new rdb::Value <db::DText> (text.transformed (db::CplxTrans (dbu))));

          } else if (s->shape.is_edge ()) {

            db::Edge edge;
            s->shape.edge (edge);
            rdb::Item *item = rdb.create_item (v->second->id (), cm->second->id ());
            item->values ().add (new rdb::Value <db::DEdge> (edge.transformed (db::CplxTrans (dbu))));

          }

        }

      }

    }

  }
}

// --------------------------------------------------------------------------------
//  SearchReplaceDialog implementation

static const char *cfg_sr_mru = "sr-mru";
static const char *cfg_sr_saved = "sr-saved";
static const char *cfg_sr_mode = "sr-mode";
static const char *cfg_sr_object = "sr-object";
static const char *cfg_sr_ctx = "sr-ctx";

static const char *mode_values[] = { "find", "delete", "replace", "custom" };
static const int find_mode_index = 0;
static const int delete_mode_index = 1;
static const int replace_mode_index = 2;
static const int custom_mode_index = 3;

static const char *ctx_values[] = { "current-cell", "current-cell-hierarchy", "all-cells" };

static void
fill_ctx_cbx (QComboBox *cbx)
{
  //  Note: see also SearchReplaceDialog::cell_expr()
  cbx->clear ();
  cbx->addItem (QObject::tr ("Current cell"));
  cbx->addItem (QObject::tr ("Current cell and below"));
  cbx->addItem (QObject::tr ("All cells"));
}

SearchReplaceDialog::SearchReplaceDialog (lay::Dispatcher *root, LayoutViewBase *view)
  : lay::Browser (root, view),
    Ui::SearchReplaceDialog (),
    mp_view (view),
    m_current_mode (0),
    m_window (FitMarker),
    m_window_dim (0.0),
    m_max_item_count (0),
    m_last_query_cv_index (0)
{
  setObjectName (QString::fromUtf8 ("search_replace_dialog"));

  Ui::SearchReplaceDialog::setupUi (this);

  connect (find_all_button, SIGNAL (clicked ()), this, SLOT (find_all_button_clicked ())); 
  connect (delete_button, SIGNAL (clicked ()), this, SLOT (delete_button_clicked ())); 
  connect (delete_all_button, SIGNAL (clicked ()), this, SLOT (delete_all_button_clicked ())); 
  connect (replace_button, SIGNAL (clicked ()), this, SLOT (replace_button_clicked ())); 
  connect (replace_all_button, SIGNAL (clicked ()), this, SLOT (replace_all_button_clicked ())); 
  connect (execute_all_button, SIGNAL (clicked ()), this, SLOT (execute_all_button_clicked ())); 
  connect (add_saved_button, SIGNAL (clicked ()), this, SLOT (add_saved_button_clicked ())); 
  connect (replace_saved_button, SIGNAL (clicked ()), this, SLOT (replace_saved_button_clicked ())); 
  connect (delete_saved_button, SIGNAL (clicked ()), this, SLOT (delete_saved_button_clicked ())); 
  connect (rename_saved_button, SIGNAL (clicked ()), this, SLOT (rename_saved_button_clicked ())); 
  connect (configure_button, SIGNAL (clicked ()), this, SLOT (configure_button_clicked ())); 
  connect (mode_tab, SIGNAL (currentChanged (int)), this, SLOT (tab_index_changed (int))); 
  connect (saved_queries, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (saved_query_double_clicked ())); 
  connect (recent_queries, SIGNAL (activated (int)), this, SLOT (recent_query_index_changed (int)));
  connect (cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_exec ()));
  connect (delete_selected_button, SIGNAL (clicked ()), this, SLOT (execute_selected_button_clicked ()));
  connect (replace_selected_button, SIGNAL (clicked ()), this, SLOT (execute_selected_button_clicked ()));

  activate_help_links (hint_label1);
  activate_help_links (hint_label2);
  activate_help_links (hint_label3);
  activate_help_links (hint_label4);

  fill_ctx_cbx (find_context);
  fill_ctx_cbx (delete_context);
  fill_ctx_cbx (replace_context);

  results->setModel (&m_model);
  results->header ()->show ();
  results->header ()->setStretchLastSection (false);

  connect (results->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (result_selection_changed ()));
  connect (results->header (), SIGNAL (sectionCountChanged (int, int)), this, SLOT (header_columns_changed (int, int)));

  QMenu *menu = new QMenu (this);
  menu->addAction (QObject::tr ("Copy to clipboard"), this, SLOT (export_csv_to_clipboard ()));
  menu->addAction (QObject::tr ("To CSV file"), this, SLOT (export_csv ()));
  menu->addAction (QObject::tr ("To report database"), this, SLOT (export_rdb ()));
  menu->addAction (QObject::tr ("To layout"), this, SLOT (export_layout ()));
  menu->addAction (QObject::tr ("To selection"), this, SLOT (select_items ()));
  export_b->setMenu (menu);

  QAction *action;

  action = new QAction (QObject::tr ("Copy to clipboard"), results);
  connect (action, SIGNAL (triggered ()), this, SLOT (sel_export_csv_to_clipboard ()));
  results->addAction (action);

  action = new QAction (QObject::tr ("Export to CSV file"), results);
  connect (action, SIGNAL (triggered ()), this, SLOT (sel_export_csv ()));
  results->addAction (action);

  action = new QAction (QObject::tr ("Export to report database"), results);
  connect (action, SIGNAL (triggered ()), this, SLOT (sel_export_rdb ()));
  results->addAction (action);

  action = new QAction (QObject::tr ("Export to layout"), results);
  connect (action, SIGNAL (triggered ()), this, SLOT (sel_export_layout ()));
  results->addAction (action);

  action = new QAction (QObject::tr ("Export to selection"), results);
  connect (action, SIGNAL (triggered ()), this, SLOT (sel_select_items ()));
  results->addAction (action);

  results->setContextMenuPolicy (Qt::ActionsContextMenu);

  bool editable = view->is_editable ();
  mode_tab->setTabEnabled (replace_mode_index, editable);
  mode_tab->setTabEnabled (delete_mode_index, editable);

  if (editable) {
    setWindowTitle (tr ("Search And Replace"));
  } else {
    setWindowTitle (tr ("Search"));
  }
}

SearchReplaceDialog::~SearchReplaceDialog ()
{
  remove_markers ();
}

static void 
save_states (QStackedWidget *sw, const std::string &pfx, lay::Dispatcher *config_root)
{
  for (int i = 0; i < sw->count (); ++i) {
    SearchReplacePropertiesWidget *pw = dynamic_cast<SearchReplacePropertiesWidget *> (sw->widget (i));
    if (pw) {
      pw->save_state (pfx, config_root);
    }
  }
}

static void 
restore_states (QStackedWidget *sw, const std::string &pfx, lay::Dispatcher *config_root)
{
  for (int i = 0; i < sw->count (); ++i) {
    SearchReplacePropertiesWidget *pw = dynamic_cast<SearchReplacePropertiesWidget *> (sw->widget (i));
    if (pw) {
      pw->restore_state (pfx, config_root);
    }
  }
}

static int 
ctx_to_index (const std::string &ctx)
{
  for (int i = 0; i < int (sizeof (ctx_values) / sizeof (ctx_values [0])); ++i) {
    if (ctx_values [i] == ctx) {
      return i;
    }
  }
  return -1;
}

static std::string 
ctx_from_index (int index)
{
  if (index >= 0 && index < int (sizeof (ctx_values) / sizeof (ctx_values [0]))) {
    return ctx_values [index];
  } else {
    return std::string ();
  }
}

void 
SearchReplaceDialog::restore_state ()
{
  lay::Dispatcher *config_root = root ();

  restore_states (find_properties, "sr-find", config_root);
  restore_states (delete_properties, "sr-find", config_root);
  restore_states (find_replace_properties, "sr-find", config_root);
  restore_states (replace_properties, "sr-replace", config_root);

  std::string v;

  if (config_root->config_get (cfg_sr_mru, v)) {
    m_mru.clear ();
    tl::Extractor ex (v.c_str ());
    while (! ex.at_end ()) {
      std::string vv;
      ex.read_quoted (vv);
      m_mru.push_back (vv);
      ex.test (";");
    }
  }

  if (config_root->config_get (cfg_sr_saved, v)) {
    m_saved.clear ();
    tl::Extractor ex (v.c_str ());
    while (! ex.at_end ()) {
      SavedQuery sq;
      ex.read_quoted (sq.description);
      ex.test (":");
      ex.read_quoted (sq.text);
      m_saved.push_back (sq);
      ex.test (";");
    }
  }

  m_current_mode = 0;
  mode_tab->blockSignals (true);
  mode_tab->setCurrentIndex (m_current_mode);
  if (config_root->config_get (cfg_sr_mode, v)) {
    for (int i = 0; i < int (sizeof (mode_values) / sizeof (mode_values [0])); ++i) {
      if (v == mode_values [i]) {
        if (mode_tab->isTabEnabled (i)) {
          m_current_mode = i;
          mode_tab->setCurrentIndex (m_current_mode);
        }
        break;
      }
    }
  }
  mode_tab->blockSignals (false);

  if (config_root->config_get (cfg_sr_object, v)) {
    find_objects->setCurrentIndex (index_from_find_object_id (v));
    delete_objects->setCurrentIndex (index_from_find_object_id (v));
    replace_objects->setCurrentIndex (index_from_find_object_id (v));
  }

  if (config_root->config_get (cfg_sr_ctx, v)) {
    find_context->setCurrentIndex (ctx_to_index (v));
    delete_context->setCurrentIndex (ctx_to_index (v));
    replace_context->setCurrentIndex (ctx_to_index (v));
  }

  update_mru_list ();
  update_saved_list ();
}

void
SearchReplaceDialog::save_state ()
{
  lay::Dispatcher *config_root = root ();

  config_root->config_set (cfg_sr_window_state, lay::save_dialog_state (this));

  int m = mode_tab->currentIndex ();

  if (m == find_mode_index) {
    save_states (find_properties, "sr-find", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (find_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (find_context->currentIndex ()));
  } else if (m == delete_mode_index) {
    save_states (delete_properties, "sr-find", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (delete_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (delete_context->currentIndex ()));
  } else if (m == replace_mode_index) {
    save_states (find_replace_properties, "sr-find", config_root);
    save_states (replace_properties, "sr-replace", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (replace_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (replace_context->currentIndex ()));
  }

  {
    std::string v;
    for (std::vector<std::string>::const_iterator i = m_mru.begin (); i != m_mru.end (); ++i) {
      if (! v.empty ()) {
        v += ";";
      }
      v += tl::to_quoted_string (*i);
    }
    config_root->config_set (cfg_sr_mru, v);
  }

  {
    std::string v;
    for (std::vector<SavedQuery>::const_iterator i = m_saved.begin (); i != m_saved.end (); ++i) {
      if (! v.empty ()) {
        v += ";";
      }
      v += tl::to_quoted_string (i->description);
      v += ":";
      v += tl::to_quoted_string (i->text);
    }
    config_root->config_set (cfg_sr_saved, v);
  }

  if (m >= 0 && m < int (sizeof (mode_values) / sizeof (mode_values [0]))) {
    config_root->config_set (cfg_sr_mode, mode_values [m]);
  }
}

void
SearchReplaceDialog::sel_select_items ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  std::set<int> rows;
  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    rows.insert (s->row ());
  }

  m_model.select_items (view (), cv_index, &rows);

END_PROTECTED
}

void
SearchReplaceDialog::select_items ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  db::LayoutQuery lq (m_last_query);

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
  progress.set_unit (100000);
  progress.set_format ("Processing ..");

  db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

  if (tl::verbosity () >= 10) {
    tl::log << tl::to_string (QObject::tr ("Running query: ")) << m_last_query;
  }

  SearchReplaceResults model;
  model.begin_changes (& cv->layout ());
  query_to_model (model, lq, iq, std::numeric_limits<size_t>::max (), true, true /*with paths*/);
  model.end_changes ();
  model.select_items (view (), cv_index);

END_PROTECTED
}

void
SearchReplaceDialog::sel_export_csv ()
{
BEGIN_PROTECTED

  std::set<int> rows;
  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    rows.insert (s->row ());
  }

  std::string fn;

  lay::FileDialog file_dialog (this, tl::to_string (QObject::tr ("Export CSV")), tl::to_string (QObject::tr ("CSV Files (*.csv);;All Files (*)")), "csv");
  if (! file_dialog.get_save (fn)) {
    return;
  }

  m_model.export_csv (fn, &rows);

END_PROTECTED
}

void
SearchReplaceDialog::export_csv ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  std::string fn;

  lay::FileDialog file_dialog (this, tl::to_string (QObject::tr ("Export CSV")), tl::to_string (QObject::tr ("CSV Files (*.csv);;All Files (*)")), "csv");
  if (! file_dialog.get_save (fn)) {
    return;
  }

  db::LayoutQuery lq (m_last_query);

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
  progress.set_unit (100000);
  progress.set_format ("Processing ..");

  db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

  if (tl::verbosity () >= 10) {
    tl::log << tl::to_string (QObject::tr ("Running query: ")) << m_last_query;
  }

  SearchReplaceResults model;
  model.begin_changes (& cv->layout ());
  query_to_model (model, lq, iq, std::numeric_limits<size_t>::max (), true);
  model.end_changes ();
  model.export_csv (fn);

END_PROTECTED
}

void
SearchReplaceDialog::sel_export_csv_to_clipboard ()
{
BEGIN_PROTECTED

  std::set<int> rows;
  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    rows.insert (s->row ());
  }

  m_model.export_csv_to_clipboard (&rows);

END_PROTECTED
}

void
SearchReplaceDialog::export_csv_to_clipboard ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  db::LayoutQuery lq (m_last_query);

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
  progress.set_unit (100000);
  progress.set_format ("Processing ..");

  db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

  if (tl::verbosity () >= 10) {
    tl::log << tl::to_string (QObject::tr ("Running query: ")) << m_last_query;
  }

  SearchReplaceResults model;
  model.begin_changes (& cv->layout ());
  query_to_model (model, lq, iq, std::numeric_limits<size_t>::max (), true);
  model.end_changes ();
  model.export_csv_to_clipboard ();

END_PROTECTED
}

void
SearchReplaceDialog::sel_export_rdb ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  std::set<int> rows;
  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    rows.insert (s->row ());
  }

  std::unique_ptr<rdb::Database> rdb (new rdb::Database ());

  rdb->set_description (tl::to_string (QObject::tr ("Query results: ")) + m_last_query);
  rdb->set_name ("query_results");
  rdb->set_generator ("query: " + m_last_query);
  rdb->set_top_cell_name (cv->layout ().cell_name (cv.cell_index ()));

  m_model.export_rdb (*rdb, cv->layout ().dbu (), &rows);

  int rdb_index = mp_view->add_rdb (rdb.release ());
  mp_view->open_rdb_browser (rdb_index, cv_index);

END_PROTECTED
}

void
SearchReplaceDialog::export_rdb ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  std::unique_ptr<rdb::Database> rdb (new rdb::Database ());

  rdb->set_description (tl::to_string (QObject::tr ("Query results: ")) + m_last_query);
  rdb->set_name ("query_results");
  rdb->set_generator ("query: " + m_last_query);
  rdb->set_top_cell_name (cv->layout ().cell_name (cv.cell_index ()));

  db::LayoutQuery lq (m_last_query);

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
  progress.set_unit (100000);
  progress.set_format ("Processing ..");

  db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

  if (tl::verbosity () >= 10) {
    tl::log << tl::to_string (QObject::tr ("Running query: ")) << m_last_query;
  }

  SearchReplaceResults model;
  model.begin_changes (& cv->layout ());
  query_to_model (model, lq, iq, std::numeric_limits<size_t>::max (), true);
  model.end_changes ();
  model.export_rdb (*rdb, cv->layout ().dbu ());

  int rdb_index = mp_view->add_rdb (rdb.release ());
  mp_view->open_rdb_browser (rdb_index, cv_index);

END_PROTECTED
}

void
SearchReplaceDialog::sel_export_layout ()
{
BEGIN_PROTECTED

  std::set<int> rows;
  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    rows.insert (s->row ());
  }

  std::unique_ptr <lay::LayoutHandle> handle (new lay::LayoutHandle (new db::Layout (mp_view->manager ()), std::string ()));
  handle->rename ("query_results");
  m_model.export_layout (handle->layout (), &rows);
  mp_view->add_layout (handle.release (), true);

END_PROTECTED
}

void
SearchReplaceDialog::export_layout ()
{
BEGIN_PROTECTED

  int cv_index = m_last_query_cv_index;
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  db::LayoutQuery lq (m_last_query);

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
  progress.set_unit (100000);
  progress.set_format ("Processing ..");

  db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

  if (tl::verbosity () >= 10) {
    tl::log << tl::to_string (QObject::tr ("Running query: ")) << m_last_query;
  }

  SearchReplaceResults model;
  model.begin_changes (& cv->layout ());
  query_to_model (model, lq, iq, std::numeric_limits<size_t>::max (), true);
  model.end_changes ();

  std::unique_ptr <lay::LayoutHandle> handle (new lay::LayoutHandle (new db::Layout (mp_view->manager ()), std::string ()));
  handle->rename ("query_results");
  model.export_layout (handle->layout ());
  mp_view->add_layout (handle.release (), true);

END_PROTECTED
}

static void 
sync_cbx (QComboBox *cbx, QStackedWidget *sw)
{
  cbx->clear ();
  for (int i = 0; i < sw->count (); ++i) {
    SearchPropertiesWidget *pw = dynamic_cast<SearchPropertiesWidget *> (sw->widget (i));
    tl_assert (pw != 0);
    cbx->addItem (tl::to_qstring (pw->description ()));
  }
}

void 
SearchReplaceDialog::activated ()
{
  cancel ();

  m_find_query.clear ();

  m_model.begin_changes (0);
  m_model.clear ();
  m_model.end_changes ();

  int cv_index = mp_view->active_cellview_index ();

  lay::CellView cv = mp_view->cellview (cv_index);
  if (cv.is_valid ()) {

    fill_find_pages (find_properties, mp_view, cv_index);
    sync_cbx (find_objects, find_properties);
    find_objects->setCurrentIndex (0);

    fill_find_pages (delete_properties, mp_view, cv_index);
    sync_cbx (delete_objects, delete_properties);
    delete_objects->setCurrentIndex (0);

    fill_replace_pages (replace_properties, mp_view, cv_index);
    fill_find_pages (find_replace_properties, mp_view, cv_index);
    sync_cbx (replace_objects, find_replace_properties);
    replace_objects->setCurrentIndex (0);

    restore_state ();

  }
}

void 
SearchReplaceDialog::deactivated ()
{
  cancel ();
  save_state ();
  remove_markers ();

  m_model.begin_changes (0);
  m_model.clear ();
  m_model.end_changes ();
}

bool 
SearchReplaceDialog::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;

  if (name == cfg_sr_window_state) {

    lay::restore_dialog_state (this, value);

  } else if (name == cfg_sr_window_mode) {

    window_type window = m_window;
    SearchReplaceWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_sr_window_dim) {

    lay::Margin wdim = lay::Margin::from_string (value);
    if (wdim != m_window_dim) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_sr_max_item_count) {

    unsigned int mic = m_max_item_count;
    tl::from_string (value, mic);
    need_update = lay::test_and_set (m_max_item_count, mic);

  } else {
    taken = false;
  }

  if (isVisible () && need_update && ! m_find_query.empty ()) {
    try {
      update_results (m_find_query);
    } catch (...) { }
  }

  return taken;
}

static std::string 
cell_expr (int ctx, const lay::CellView &cv)
{
  std::string ce;
  if (ctx == 0) {
    ce = "cell ";
    ce += tl::to_word_or_quoted_string (cv->layout ().cell_name (cv.cell_index ()));
  } else if (ctx == 1) {
    ce = "instances of ";
    ce += tl::to_word_or_quoted_string (cv->layout ().cell_name (cv.cell_index ()));
    ce += "..";
  } else {
    ce = "cells *";
  }

  return ce;
}

std::string
SearchReplaceDialog::build_find_expression (QStackedWidget *prop_page, QComboBox *context)
{
  const lay::CellView &cv = mp_view->cellview (mp_view->active_cellview_index ());
  if (! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout loaded")));
  }

  std::string expr;

  lay::SearchPropertiesWidget *p = dynamic_cast<lay::SearchPropertiesWidget *> (prop_page->currentWidget ());
  if (p) {
    expr += p->search_expression (cell_expr (context->currentIndex (), cv));
  }

  return expr;
}

std::string
SearchReplaceDialog::build_delete_expression ()
{
  const lay::CellView &cv = mp_view->cellview (mp_view->active_cellview_index ());
  if (! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout loaded")));
  }

  std::string expr;

  lay::SearchPropertiesWidget *p = dynamic_cast<lay::SearchPropertiesWidget *> (delete_properties->currentWidget ());
  if (p) {
    expr = "delete ";
    expr += p->search_expression (cell_expr (delete_context->currentIndex (), cv));
  }

  return expr;
}

std::string
SearchReplaceDialog::build_replace_expression ()
{
  const lay::CellView &cv = mp_view->cellview (mp_view->active_cellview_index ());
  if (! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout loaded")));
  }

  std::string expr;

  lay::SearchPropertiesWidget *pf = dynamic_cast<lay::SearchPropertiesWidget *> (find_replace_properties->currentWidget ());
  lay::ReplacePropertiesWidget *pr = dynamic_cast<lay::ReplacePropertiesWidget *> (replace_properties->currentWidget ());
  if (pf && pr) {
    expr = "with ";
    expr += pf->search_expression (cell_expr (replace_context->currentIndex (), cv));
    expr += " do ";
    std::string re = pr->replace_expression ();
    if (re.empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("No replacement action specified - replace operation wouldn't do anything")));
    }
    expr += re;
  }

  return expr;
}

void
SearchReplaceDialog::update_saved_list ()
{
  saved_queries->clear ();

  for (std::vector<SavedQuery>::const_iterator s = m_saved.begin (); s != m_saved.end (); ++s) {
    saved_queries->addItem (tl::to_qstring (s->description));
  }
}

void
SearchReplaceDialog::update_mru_list ()
{
  recent_queries->blockSignals (true);
  recent_queries->clear ();

  for (std::vector<std::string>::const_iterator mru = m_mru.begin (); mru != m_mru.end (); ++mru) {
    QString text = tl::to_qstring (*mru);
    QString display_text = text.simplified ();
    int nmax = 50;
    if (display_text.size () > nmax) {
      display_text = display_text.left (nmax) + QString::fromUtf8 ("...");
    }
    recent_queries->addItem (display_text, QVariant (text));
  }

  recent_queries->setCurrentIndex (0);
  recent_queries->blockSignals (false);
}

void
SearchReplaceDialog::recent_query_index_changed (int index)
{
  if (index >= 0 && index < int (recent_queries->count ())) {
    custom_query->setText (recent_queries->itemData (index).toString ());
  }
}

void
SearchReplaceDialog::issue_query (const std::string &q, const std::set<size_t> *selected_items, bool with_results)
{
  detach_from_all_events ();  //  don't listen to layout events any longer

  remove_markers ();
  results->clearSelection ();

  int cv_index = mp_view->active_cellview_index ();
  const lay::CellView &cv = mp_view->cellview (cv_index);
  if (! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout loaded")));
  }

  m_last_query.clear ();
  m_last_query_cv_index = -1;

  //  Test-parse the query
  db::LayoutQuery lq (q);

  m_last_query = q;
  m_last_query_cv_index = cv_index;

  const size_t max_mru_length = 20;

  //  put the query into the MRU list
  for (int i = 0; i < int (m_mru.size ()); ++i) {
    if (m_mru[i] == q) {
      m_mru.erase (m_mru.begin () + i);
      --i;
    }
  }
  m_mru.insert (m_mru.begin (), q);
  while (m_mru.size () > max_mru_length) {
    m_mru.pop_back ();
  }

  update_mru_list ();

  if (with_results) {

    update_results (q);

  } else if (! selected_items) {

    db::LayoutQuery lq (q);

    if (tl::verbosity () >= 10) {
      tl::log << tl::to_string (QObject::tr ("Running full query (without results): ")) << q;
    }

    m_model.begin_changes (0);
    m_model.clear ();
    m_model.end_changes ();
  
    tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
    progress.set_unit (100000);
    progress.set_format ("Processing ..");

    db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);
    while (! iq.at_end ()) {
      ++iq;
    }

  } else {
    
    db::LayoutQuery lq (q + " pass");

    if (tl::verbosity () >= 10) {
      tl::log << tl::to_string (QObject::tr ("Running query on selection: ")) << q;
    }

    m_model.begin_changes (0);
    m_model.clear ();
    m_model.end_changes ();
  
    tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
    progress.set_unit (100000);
    progress.set_format ("Processing ..");

    size_t n = 0;
    for (db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress); ! iq.at_end (); ++n) {
      iq.next (selected_items->find (n) == selected_items->end ());
    }

  }
}

void
SearchReplaceDialog::cancel_exec ()
{
  execute_panel->hide ();
  remove_markers ();
  results->clearSelection ();

  m_execute_query.clear ();
  m_find_query.clear ();
}

void
SearchReplaceDialog::cancel ()
{
  detach_from_all_events ();  //  don't listen to layout events any longer

  execute_panel->hide ();
  remove_markers ();
  results->clearSelection ();

  m_model.begin_changes (0);
  m_model.clear ();
  m_model.end_changes ();

  results_stack->setCurrentIndex (mode_tab->currentIndex () + 1);  //  show hint
  export_b->setEnabled (false);

  m_execute_query.clear ();
  m_find_query.clear ();
}

void
SearchReplaceDialog::layout_changed ()
{
  //  cannot call detach_all inside signal handler currently
  cancel ();
}

void
SearchReplaceDialog::attach_layout (db::Layout *layout)
{
  layout->hier_changed_event.add (this, &SearchReplaceDialog::layout_changed);
  layout->bboxes_changed_any_event.add (this, &SearchReplaceDialog::layout_changed);
  layout->cell_name_changed_event.add (this, &SearchReplaceDialog::layout_changed);
  layout->layer_properties_changed_event.add (this, &SearchReplaceDialog::layout_changed);
}

void
SearchReplaceDialog::update_results (const std::string &q)
{
  detach_from_all_events ();  //  don't listen to layout events any longer

  remove_markers ();
  results->clearSelection ();

  const lay::CellView &cv = mp_view->cellview (mp_view->active_cellview_index ());
  if (! cv.is_valid ()) {

    m_model.begin_changes (0);
    m_model.clear ();
    m_model.end_changes ();

  } else {

    db::LayoutQuery lq (q);

    tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Running query")));
    progress.set_unit (100000);
    progress.set_format ("Processing ..");

    db::LayoutQueryIterator iq (lq, &cv->layout (), 0, &progress);

    if (tl::verbosity () >= 10) {
      tl::log << tl::to_string (QObject::tr ("Running query: ")) << q;
    }

    try {
      fill_model (lq, iq, &cv->layout (), true, true);
      attach_layout (&cv->layout ());
    } catch (...) {
      attach_layout (&cv->layout ());
      throw;
    }

  }
}

bool
SearchReplaceDialog::query_to_model (SearchReplaceResults &model, const db::LayoutQuery &lq, db::LayoutQueryIterator &iq, size_t max_item_count, bool all, bool with_path)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (QObject::tr ("Query run")));

  size_t n = 0;
  bool res = false;

  int data_prop_id = lq.has_property ("data") ? int (lq.property_by_name ("data")) : -1;
  int expressions_prop_id = lq.has_property ("expressions") ? int (lq.property_by_name ("expressions")) : -1;
  int shape_prop_id = lq.has_property ("shape") ? int (lq.property_by_name ("shape")) : -1;
  int layer_index_prop_id = lq.has_property ("layer_index") ? int (lq.property_by_name ("layer_index")) : -1;
  int instance_prop_id = lq.has_property ("inst") ? int (lq.property_by_name ("inst")) : -1;
  int inst_elements_prop_id = with_path && lq.has_property ("inst_elements") ? int (lq.property_by_name ("inst_elements")) : -1;
  int path_trans_prop_id = lq.has_property ("path_trans") ? int (lq.property_by_name ("path_trans")) : -1;
  int trans_prop_id = lq.has_property ("trans") ? int (lq.property_by_name ("trans")) : -1;
  int cell_index_prop_id = lq.has_property ("cell_index") ? int (lq.property_by_name ("cell_index")) : -1;
  int parent_cell_index_prop_id = lq.has_property ("parent_cell_index") ? int (lq.property_by_name ("parent_cell_index")) : -1;
  int initial_cell_index_prop_id = lq.has_property ("initial_cell_index") ? int (lq.property_by_name ("initial_cell_index")) : -1;

  tl::Variant ve;
  if (expressions_prop_id >= 0 && iq.get (expressions_prop_id, ve)) {
    model.set_data_column_headers (ve);
  }

  while (! iq.at_end ()) {

    if (++n > max_item_count) {
      model.has_more (true);
      break;
    }

    res = true;

    tl::Variant v;

    if (data_prop_id >= 0 && iq.get (data_prop_id, v)) {

      model.push_back (v);

    } else if (shape_prop_id >= 0) {

      db::Shape shape;
      int layer_index = 0;
      db::ICplxTrans trans;
      db::cell_index_type cell_index = std::numeric_limits<db::cell_index_type>::max ();
      db::cell_index_type initial_cell_index = std::numeric_limits<db::cell_index_type>::max ();

      if (iq.get (shape_prop_id, v)) {
        shape = v.to_user<db::Shape> ();
      }
      if (layer_index_prop_id >= 0 && iq.get (layer_index_prop_id, v)) {
        layer_index = v.to_int ();
      }
      if (cell_index_prop_id >= 0 && iq.get (cell_index_prop_id, v)) {
        cell_index = db::cell_index_type (v.to_int ());
      }
      if (path_trans_prop_id >= 0 && iq.get (path_trans_prop_id, v)) {
        trans = v.to_user<db::ICplxTrans> ();
        if (initial_cell_index_prop_id >= 0 && iq.get (initial_cell_index_prop_id, v)) {
          initial_cell_index = db::cell_index_type (v.to_int ());
        }
      }

      model.push_back (SearchReplaceResults::QueryShapeResult (shape, layer_index, trans, cell_index, initial_cell_index));

      if (inst_elements_prop_id >= 0 && iq.get (inst_elements_prop_id, v) && v.is_list ()) {
        try {
          std::vector<db::InstElement> inst_elements;
          for (auto i = v.begin (); i != v.end (); ++i) {
            inst_elements.push_back (i->to_user<db::InstElement> ());
          }
          model.shapes ().back ().inst_elements = std::move (inst_elements);
        } catch (...) {
          //  ignore conversion errors
        }
      }

    } else if (instance_prop_id >= 0) {

      db::Instance instance;
      db::ICplxTrans trans;
      db::cell_index_type cell_index = std::numeric_limits<db::cell_index_type>::max ();
      db::cell_index_type initial_cell_index = std::numeric_limits<db::cell_index_type>::max ();

      if (iq.get (instance_prop_id, v)) {
        instance = v.to_user<db::Instance> ();
      }
      if (parent_cell_index_prop_id >= 0 && iq.get (parent_cell_index_prop_id, v)) {
        cell_index = db::cell_index_type (v.to_int ());
      }
      if (path_trans_prop_id >= 0 && iq.get (path_trans_prop_id, v)) {
        trans = v.to_user<db::ICplxTrans> ();
        if (trans_prop_id >= 0 && iq.get (trans_prop_id, v)) {
          //  strip the first transformation (the one from the instance itself)
          trans = trans * v.to_user<db::ICplxTrans> ().inverted ();
        }
        if (initial_cell_index_prop_id >= 0 && iq.get (initial_cell_index_prop_id, v)) {
          initial_cell_index = db::cell_index_type (v.to_int ());
        }
      }

      model.push_back (SearchReplaceResults::QueryInstResult (instance, trans, cell_index, initial_cell_index));

      if (inst_elements_prop_id >= 0 && iq.get (inst_elements_prop_id, v) && v.is_list ()) {
        try {
          std::vector<db::InstElement> inst_elements;
          for (auto i = v.begin (); i != v.end (); ++i) {
            inst_elements.push_back (i->to_user<db::InstElement> ());
          }
          model.instances ().back ().inst_elements = std::move (inst_elements);
        } catch (...) {
          //  ignore conversion errors
        }
      }

    } else if (cell_index_prop_id >= 0) {

      db::cell_index_type cell_index = std::numeric_limits<db::cell_index_type>::max ();
      db::cell_index_type parent_cell_index = std::numeric_limits<db::cell_index_type>::max ();

      if (cell_index_prop_id >= 0 && iq.get (cell_index_prop_id, v)) {
        cell_index = db::cell_index_type (v.to_int ());
      }
      if (parent_cell_index_prop_id >= 0 && iq.get (parent_cell_index_prop_id, v)) {
        parent_cell_index = db::cell_index_type (v.to_int ());
      }

      model.push_back (SearchReplaceResults::QueryCellResult (cell_index, parent_cell_index));

    } else {
      break;
    }

    if (! all) {
      break;
    } 

    ++iq;

  }

  return res;
}

bool
SearchReplaceDialog::fill_model (const db::LayoutQuery &lq, db::LayoutQueryIterator &iq, const db::Layout *layout, bool all, bool with_paths)
{
  bool res = false;

  try {

    m_model.begin_changes (layout);
    m_model.clear ();

    res = query_to_model (m_model, lq, iq, m_max_item_count, all, with_paths);

    m_model.end_changes ();
    results_stack->setCurrentIndex (0);
    export_b->setEnabled (true);

  } catch (...) {

    m_model.end_changes ();
    results_stack->setCurrentIndex (0);
    export_b->setEnabled (true);

    throw;

  }

  return res;
}

void
SearchReplaceDialog::header_columns_changed (int /*from*/, int /*to*/)
{
  results->header ()->resizeSections (QHeaderView::ResizeToContents);
}

void
SearchReplaceDialog::remove_markers ()
{
  for (std::vector<lay::MarkerBase *>::const_iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }
  mp_markers.clear ();
}

void
SearchReplaceDialog::result_selection_changed ()
{
  try {

    remove_markers ();

    int cv_index = m_last_query_cv_index;
    const lay::CellView &cv = mp_view->cellview (cv_index);
    if (! cv.is_valid ()) {
      return;
    }

    const db::Layout &layout = cv->layout ();

    //  collect the transformation variants for this cellview - this way we can paint
    //  the cell boxes for each global transformation
    std::vector<db::DCplxTrans> global_trans = view ()->cv_transform_variants (cv_index);
    std::map<unsigned int, std::vector<db::DCplxTrans> > tv_map = view ()->cv_transform_variants_by_layer (cv_index);

    db::DBox dbox;

    QModelIndexList sel = results->selectionModel ()->selectedRows (0);

    delete_selected_button->setEnabled (! sel.isEmpty ());
    replace_selected_button->setEnabled (! sel.isEmpty ());

    for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {

      int index = s->row ();
      if (index < 0) {

        //  .. ignore ..

      } else if (index < int (m_model.shapes ().size ())) {

        const SearchReplaceResults::QueryShapeResult &sr = m_model.shapes () [index];

        if (! sr.shape.is_null ()) {

          db::ICplxTrans tr_context;
          if (layout.is_valid_cell_index (sr.initial_cell_index)) {
            tr_context = db::find_layout_context (layout, sr.initial_cell_index, cv.cell_index ()).second;
          }

          //  transform the box into the cell view shown in micron space
          lay::ShapeMarker *marker = new lay::ShapeMarker (view (), cv_index);
          mp_markers.push_back (marker);

          std::map<unsigned int, std::vector<db::DCplxTrans> >::const_iterator tv = tv_map.find (sr.layer_index);
          if (tv != tv_map.end ()) {
            marker->set (sr.shape, tr_context * sr.trans, tv->second);
          } else {
            marker->set (sr.shape, tr_context * sr.trans);
          }

          dbox += marker->bbox ();

        }

      } else if (index < int (m_model.instances ().size ())) {

        const SearchReplaceResults::QueryInstResult &ir = m_model.instances () [index];

        if (! ir.inst.is_null ()) {

          db::ICplxTrans tr_context;
          if (layout.is_valid_cell_index (ir.initial_cell_index)) {
            tr_context = db::find_layout_context (layout, ir.initial_cell_index, cv.cell_index ()).second;
          }

          lay::InstanceMarker *marker = new lay::InstanceMarker (view (), cv_index);
          marker->set (ir.inst, tr_context * ir.trans, global_trans);
          mp_markers.push_back (marker);

          dbox += marker->bbox ();

        }

      } else if (index < int (m_model.cells ().size ())) {

        const SearchReplaceResults::QueryCellResult &ir = m_model.cells () [index];

        std::pair<bool, db::ICplxTrans> si = db::find_layout_context (layout, ir.cell_index, cv.cell_index ());
        if (si.first) {

          db::Box box = layout.cell (ir.cell_index).bbox ();

          lay::Marker *marker = new lay::Marker (view (), cv_index);
          marker->set (box, si.second, global_trans);
          mp_markers.push_back (marker);

          dbox += marker->bbox ();

        }

      } else if (index < int (m_model.data ().size ())) {

        db::DCplxTrans as_dbu = db::DCplxTrans (layout.dbu ()).inverted ();

        const tl::Variant &dr = m_model.data () [index];
        for (tl::Variant::const_iterator v = dr.begin (); v != dr.end (); ++v) {

          lay::Marker *marker = new lay::Marker (view (), cv_index);

          if (v->is_user<db::DBox> ()) {
            marker->set (v->to_user<db::DBox> (), as_dbu, global_trans);
          } else if (v->is_user<db::Box> ()) {
            marker->set (v->to_user<db::Box> (), db::ICplxTrans (), global_trans);
          } else if (v->is_user<db::DEdge> ()) {
            marker->set (v->to_user<db::DEdge> (), as_dbu, global_trans);
          } else if (v->is_user<db::Edge> ()) {
            marker->set (v->to_user<db::Edge> (), db::ICplxTrans (), global_trans);
          } else if (v->is_user<db::DPolygon> ()) {
            marker->set (v->to_user<db::DPolygon> (), as_dbu, global_trans);
          } else if (v->is_user<db::Polygon> ()) {
            marker->set (v->to_user<db::Polygon> (), db::ICplxTrans (), global_trans);
          } else if (v->is_user<db::DPath> ()) {
            marker->set (v->to_user<db::DPath> (), as_dbu, global_trans);
          } else if (v->is_user<db::Path> ()) {
            marker->set (v->to_user<db::Path> (), db::ICplxTrans (), global_trans);
          } else if (v->is_user<db::DPoint> ()) {
            db::DPoint p = v->to_user<db::DPoint> ();
            marker->set (db::DBox (p, p), as_dbu, global_trans);
          } else if (v->is_user<db::Point> ()) {
            db::Point p = v->to_user<db::Point> ();
            marker->set (db::Box (p, p), db::ICplxTrans (), global_trans);
          } else if (v->is_user<db::DVector> ()) {
            db::DPoint p = db::DPoint () + v->to_user<db::DVector> ();
            marker->set (db::DBox (p, p), as_dbu, global_trans);
          } else if (v->is_user<db::Vector> ()) {
            db::Point p = db::Point () + v->to_user<db::Vector> ();
            marker->set (db::Box (p, p), db::ICplxTrans (), global_trans);
          } else {
            delete marker;
            marker = 0;
          }

          if (marker) {
            mp_markers.push_back (marker);
            dbox += marker->bbox ();
          }

        }

      }

    }

    if (! dbox.empty ()) {

      double window_dim = m_window_dim.get (dbox);

      if (m_window == FitCell) {
        view ()->zoom_fit ();
      } else if (m_window == FitMarker) {
        view ()->zoom_box (dbox.enlarged (db::DVector (window_dim, window_dim)));
      } else if (m_window == Center) {
        view ()->pan_center (dbox.p1 () + (dbox.p2 () - dbox.p1 ()) * 0.5);
      } else if (m_window == CenterSize) {
        double w = std::max (dbox.width (), window_dim);
        double h = std::max (dbox.height (), window_dim);
        db::DPoint center (dbox.p1 () + (dbox.p2 () - dbox.p1 ()) * 0.5);
        db::DVector d (w * 0.5, h * 0.5);
        view ()->zoom_box (db::DBox (center - d, center + d));
      }

    }

  } catch (...) {
    //  .. ignore exceptions ..
  }
}

void  
SearchReplaceDialog::find_all_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  m_find_query = build_find_expression (find_properties, find_context);
  issue_query (m_find_query, 0, true);

END_PROTECTED
}

void  
SearchReplaceDialog::delete_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  m_execute_query = build_delete_expression ();
  m_find_query = build_find_expression (delete_properties, delete_context);
  issue_query (m_find_query, 0, true);

  delete_selected_button->show ();
  delete_selected_button->setEnabled (false);
  replace_selected_button->hide ();
  execute_panel->show ();

END_PROTECTED
}

void  
SearchReplaceDialog::delete_all_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  if (mp_view->manager ()) {
    mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Delete all")));
  }
  mp_view->cancel ();
  issue_query (build_delete_expression (), 0, false);
  if (mp_view->manager ()) {
    mp_view->manager ()->commit ();
  }

END_PROTECTED
}

void  
SearchReplaceDialog::replace_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  m_execute_query = build_replace_expression ();
  m_find_query = build_find_expression (find_replace_properties, replace_context);
  issue_query (m_find_query, 0, true);

  delete_selected_button->hide ();
  replace_selected_button->show ();
  replace_selected_button->setEnabled (false);
  execute_panel->show ();

END_PROTECTED
}

void
SearchReplaceDialog::execute_selected_button_clicked ()
{
BEGIN_PROTECTED

  if (m_execute_query.empty ()) {
    return;
  }

  std::set<size_t> selected_items;

  QModelIndexList sel = results->selectionModel ()->selectedRows (0);
  for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    int index = s->row ();
    if (index >= 0) {
      selected_items.insert (size_t (index));
    }
  }

  if (! sel.empty ()) {

    if (mp_view->manager ()) {
      if (sender () == delete_selected_button) {
        mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Delete selected")));
      } else {
        mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Replace selected")));
      }
    }

    mp_view->cancel ();
    issue_query (m_execute_query, &selected_items, false);
    if (mp_view->manager ()) {
      mp_view->manager ()->commit ();
    }

    issue_query (m_find_query, 0, true);

  }

END_PROTECTED
}

void  
SearchReplaceDialog::replace_all_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  m_execute_query.clear ();
  m_find_query.clear ();

  if (mp_view->manager ()) {
    mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Replace all")));
  }
  mp_view->cancel ();
  issue_query (build_replace_expression (), 0, false);
  if (mp_view->manager ()) {
    mp_view->manager ()->commit ();
  }

END_PROTECTED
}

void  
SearchReplaceDialog::execute_all_button_clicked ()
{
BEGIN_PROTECTED

  cancel_exec ();

  m_execute_query.clear ();
  m_find_query.clear ();

  if (mp_view->manager ()) {
    mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Execute custom query")));
  }
  mp_view->cancel ();
  issue_query (tl::to_string (custom_query->toPlainText ()), 0, true);
  if (mp_view->manager ()) {
    mp_view->manager ()->commit ();
  }

END_PROTECTED
}

void  
SearchReplaceDialog::configure_button_clicked ()
{
  lay::ConfigurationDialog config_dialog (this, root (), "SearchReplacePlugin");
  config_dialog.exec ();
}

void
SearchReplaceDialog::tab_index_changed (int index)
{
  cancel ();

  lay::Dispatcher *config_root = root ();

  std::string v;

  //  share find settings between pages: first save
  if (m_current_mode == find_mode_index) {

    save_states (find_properties, "sr-find", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (find_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (find_context->currentIndex ()));

  } else if (m_current_mode == delete_mode_index) {

    save_states (delete_properties, "sr-find", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (delete_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (delete_context->currentIndex ()));

  } else if (m_current_mode == replace_mode_index) {

    save_states (find_replace_properties, "sr-find", config_root);
    config_root->config_set (cfg_sr_object, index_to_find_object_id (replace_objects->currentIndex ()));
    config_root->config_set (cfg_sr_ctx, ctx_from_index (replace_context->currentIndex ()));

  }

  if (index == find_mode_index) {

    restore_states (find_properties, "sr-find", config_root);
    if (config_root->config_get (cfg_sr_object, v)) {
      find_objects->setCurrentIndex (index_from_find_object_id (v));
    }
    if (config_root->config_get (cfg_sr_ctx, v)) {
      find_context->setCurrentIndex (ctx_to_index (v));
    }

  } else if (index == delete_mode_index) {

    restore_states (delete_properties, "sr-find", config_root);
    if (config_root->config_get (cfg_sr_object, v)) {
      delete_objects->setCurrentIndex (index_from_find_object_id (v));
    }
    if (config_root->config_get (cfg_sr_ctx, v)) {
      delete_context->setCurrentIndex (ctx_to_index (v));
    }

  } else if (index == replace_mode_index) {

    restore_states (find_replace_properties, "sr-find", config_root);
    if (config_root->config_get (cfg_sr_object, v)) {
      replace_objects->setCurrentIndex (index_from_find_object_id (v));
    }
    if (config_root->config_get (cfg_sr_ctx, v)) {
      replace_context->setCurrentIndex (ctx_to_index (v));
    }

  } else if (index == custom_mode_index) {

    //  update query on the custom query page

    if (m_current_mode == find_mode_index) {

      try {
        custom_query->setText (tl::to_qstring (build_find_expression (find_properties, find_context)));
      } catch (...) {
        //  ignore errors
        custom_query->setText (tl::to_qstring (""));
      }

    } else if (m_current_mode == delete_mode_index) {

      try {
        custom_query->setText (tl::to_qstring (build_delete_expression ()));
      } catch (...) {
        //  ignore errors
        custom_query->setText (tl::to_qstring (""));
      }

    } else if (m_current_mode == replace_mode_index) {

      try {
        custom_query->setText (tl::to_qstring (build_replace_expression ()));
      } catch (...) {
        //  ignore errors
        custom_query->setText (tl::to_qstring (""));
      }

    }

  }

  m_current_mode = index;
}

void  
SearchReplaceDialog::replace_saved_button_clicked ()
{
  int index = saved_queries->currentRow ();
  if (index >= 0 && index < int (m_saved.size ())) {
    m_saved [index].text = tl::to_string (custom_query->toPlainText ());
  }
}

void  
SearchReplaceDialog::add_saved_button_clicked ()
{
BEGIN_PROTECTED

  bool ok = false;
  QString desc = QInputDialog::getText (this, QObject::tr ("Enter Description"), 
                                              QObject::tr ("Enter a description text for the current query.\nThat text will be shown in the selection box."),
                                              QLineEdit::Normal, QString (), &ok);
  if (ok) {

    m_saved.push_back (SavedQuery ());
    m_saved.back ().description = tl::to_string (desc);
    m_saved.back ().text = tl::to_string (custom_query->toPlainText ());

    update_saved_list ();

    saved_queries->setCurrentRow (saved_queries->count () - 1);

  }

END_PROTECTED
}

void  
SearchReplaceDialog::delete_saved_button_clicked ()
{
  int index = saved_queries->currentRow ();
  if (index >= 0 && index < int (m_saved.size ())) {
    m_saved.erase (m_saved.begin () + index);
    update_saved_list ();
    saved_queries->setCurrentRow (std::min (saved_queries->count () - 1, index));
  }
}

void  
SearchReplaceDialog::rename_saved_button_clicked ()
{
  int index = saved_queries->currentRow ();
  if (index >= 0 && index < int (m_saved.size ())) {

    bool ok = false;
    QString desc = QInputDialog::getText (this, QObject::tr ("Enter Description"), 
                                                QObject::tr ("Enter a description text for the current query.\nThat text will be shown in the selection box."),
                                                QLineEdit::Normal, 
                                                tl::to_qstring (m_saved [index].description), 
                                                &ok);
    if (ok) {
      m_saved [index].description = tl::to_string (desc);
      update_saved_list ();
    }

  }
}

void
SearchReplaceDialog::saved_query_double_clicked ()
{
  int index = saved_queries->currentRow ();
  if (index >= 0 && index < int (m_saved.size ())) {
    custom_query->setText (tl::to_qstring (m_saved [index].text));
  }
}

void 
SearchReplaceDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "search_replace::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else {
    lay::Plugin::menu_activated (symbol);
  }
}

}

