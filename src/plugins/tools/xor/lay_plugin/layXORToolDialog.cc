
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


#include "layXORToolDialog.h"
#include "layXORProgress.h"
#include "antService.h"
#include "rdb.h"
#include "rdbUtils.h"
#include "dbShapeProcessor.h"
#include "dbRecursiveShapeIterator.h"
#include "dbClip.h"
#include "dbLayoutUtils.h"
#include "dbRegion.h"
#include "dbDeepShapeStore.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "tlThreadedWorkers.h"
#include "tlEnv.h"
#include "tlExceptions.h"
#include "tlMath.h"
#include "layCellView.h"
#include "layLayoutView.h"
#include "layApplication.h"
#include "layMainWindow.h"

#include "ui_XORToolDialog.h"

#include <stdio.h>

#include <QMessageBox>

namespace lay
{

bool merge_before_bool ()
{
  //  $KLAYOUT_XOR_MERGE_BEFORE_BOOLEAN
  return tl::app_flag ("xor-merge-before-boolean");
}

std::string cfg_xor_input_mode ("xor-input-mode");
std::string cfg_xor_output_mode ("xor-output-mode");
std::string cfg_xor_nworkers ("xor-num-workers");
std::string cfg_xor_layer_offset ("xor-layer-offset");
std::string cfg_xor_axorb ("xor-axorb");
std::string cfg_xor_anotb ("xor-anotb");
std::string cfg_xor_bnota ("xor-bnota");
std::string cfg_xor_summarize ("xor-summarize");
std::string cfg_xor_tolerances ("xor-tolerances");
std::string cfg_xor_deep ("xor-deep");
std::string cfg_xor_tiling ("xor-tiling");
std::string cfg_xor_tiling_heal ("xor-tiling-heal");
std::string cfg_xor_region_mode ("xor-region-mode");

//  Note: this enum must match with the order of the combo box entries in the 
//  dialog implementation
enum input_mode_t { IMAll = 0, IMVisible, IMSpecific };

//  Note: this enum must match with the order of the combo box entries in the 
//  dialog implementation
enum output_mode_t { OMMarkerDatabase = 0, OMNewLayout, OMNewLayersA, OMNewLayersB, OMNone };

//  Note: this enum must match with the order of the combo box entries in the 
//  dialog implementation
enum region_mode_t { RMAll = 0, RMVisible, RMRulers };

struct InputModeConverter
{
  std::string to_string (input_mode_t t)
  {
    switch (t) {
    case IMAll:
      return "all";
    case IMVisible:
      return "visible";
    case IMSpecific:
      return "specific";
    default:
      return "";
    }
  }

  void from_string (const std::string &s, input_mode_t &t) 
  {
    t = IMAll;
    if (s == "visible") {
      t = IMVisible;
    } else if (s == "specific") {
      t = IMSpecific;
    }
  }
};

struct OutputModeConverter
{
  std::string to_string (output_mode_t t)
  {
    switch (t) {
    case OMMarkerDatabase:
      return "rdb";
    case OMNewLayout:
      return "layout";
    case OMNewLayersA:
      return "layers-a";
    case OMNewLayersB:
      return "layers-b";
    default:
      return "";
    }
  }

  void from_string (const std::string &s, output_mode_t &t) 
  {
    t = OMMarkerDatabase;
    if (s == "layout") {
      t = OMNewLayout;
    } else if (s == "layers-a") {
      t = OMNewLayersA;
    } else if (s == "layers-b") {
      t = OMNewLayersB;
    }
  }
};

struct RegionModeConverter
{
  std::string to_string (region_mode_t t)
  {
    switch (t) {
    case RMAll:
      return "all";
    case RMVisible:
      return "visible";
    case RMRulers:
      return "rulers";
    default:
      return "";
    }
  }

  void from_string (const std::string &s, region_mode_t &t) 
  {
    t = RMAll;
    if (s == "all") {
      t = RMAll;
    } else if (s == "visible") {
      t = RMVisible;
    } else if (s == "rulers") {
      t = RMRulers;
    }
  }
};

XORToolDialog::XORToolDialog (QWidget *parent)
  : QDialog (parent), mp_view (0)
{
  mp_ui = new Ui::XORToolDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->input_layers_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (input_changed (int)));
  connect (mp_ui->output_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (output_changed (int)));
  connect (mp_ui->deep, SIGNAL (clicked ()), this, SLOT (deep_changed ()));

  input_changed (0);
  output_changed (0);
}

XORToolDialog::~XORToolDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

int 
XORToolDialog::exec_dialog (lay::LayoutViewBase *view)
{
  mp_view = view;

  if (view != mp_ui->layouta->layout_view () || view != mp_ui->layoutb->layout_view ()) {

    mp_ui->layouta->set_layout_view (view);
    mp_ui->layoutb->set_layout_view (view);

    if (view->cellviews () >= 2) {
      mp_ui->layouta->set_current_cv_index (0);
      mp_ui->layoutb->set_current_cv_index (1);
    }

  } else {
    //  force update of the layer list
    //  TODO: the controls should register a listener for the view so this activity is not necessary:
    mp_ui->layouta->set_layout_view (view);
    mp_ui->layoutb->set_layout_view (view);
  }

  //  take current settings from the configurations
  lay::Dispatcher *config_root = lay::Dispatcher::instance ();

  input_mode_t im = IMAll;
  if (config_root->config_get (cfg_xor_input_mode, im, InputModeConverter ())) {
    mp_ui->input_layers_cbx->setCurrentIndex ((int) im);
  }

  output_mode_t om = OMMarkerDatabase;
  if (config_root->config_get (cfg_xor_output_mode, om, OutputModeConverter ())) {
    mp_ui->output_cbx->setCurrentIndex ((int) om);
  }

  region_mode_t rm = RMAll;
  if (config_root->config_get (cfg_xor_region_mode, rm, RegionModeConverter ())) {
    mp_ui->region_cbx->setCurrentIndex ((int) rm);
  }

  int nw = 1;
  if (config_root->config_get (cfg_xor_nworkers, nw)) {
    mp_ui->threads->setValue (nw);
  }

  std::string lo;
  if (config_root->config_get (cfg_xor_layer_offset, lo)) {
    mp_ui->layer_offset_le->setText (tl::to_qstring (lo));
  }

  bool f = false;
  if (config_root->config_get (cfg_xor_axorb, f)) {
    mp_ui->axorb_cb->setChecked (f);
  }
  if (config_root->config_get (cfg_xor_anotb, f)) {
    mp_ui->anotb_cb->setChecked (f);
  }
  if (config_root->config_get (cfg_xor_bnota, f)) {
    mp_ui->bnota_cb->setChecked (f);
  }

  bool summarize = false;
  if (config_root->config_get (cfg_xor_summarize, summarize)) {
    mp_ui->summarize_cb->setChecked (summarize);
  }

  std::string tol;
  if (config_root->config_get (cfg_xor_tolerances, tol)) {
    mp_ui->tolerances->setText (tl::to_qstring (tol));
  }

  bool deep = false;
  if (config_root->config_get (cfg_xor_deep, deep)) {
    mp_ui->deep->setChecked (deep);
  }
  deep_changed ();

  std::string tiling;
  if (config_root->config_get (cfg_xor_tiling, tiling)) {
    mp_ui->tiling->setText (tl::to_qstring (tiling));
  }

  bool heal = false;
  if (config_root->config_get (cfg_xor_tiling_heal, heal)) {
    mp_ui->heal_cb->setChecked (heal);
  }

  int ret = QDialog::exec ();

  if (ret) {
    run_xor ();
  }

  mp_view = 0;
  return ret;
}

void 
XORToolDialog::accept ()
{
BEGIN_PROTECTED

  bool axorb = mp_ui->axorb_cb->isChecked ();
  bool anotb = mp_ui->anotb_cb->isChecked ();
  bool bnota = mp_ui->bnota_cb->isChecked ();
  if (axorb + anotb + bnota == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No mode selected")));  
  }

  int cv_index_a = mp_ui->layouta->current_cv_index ();
  int cv_index_b = mp_ui->layoutb->current_cv_index ();

  const lay::CellView &cva = mp_view->cellview (cv_index_a);
  const lay::CellView &cvb = mp_view->cellview (cv_index_b);

  if (&cva->layout () == &cvb->layout () && cva.cell_index () == cvb.cell_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Trying to perform an XOR between identical layouts")));  
  }

  if (!cva.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("First layout is not a valid input")));  
  }
  if (!cvb.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Second layout is not a valid input")));  
  }

  {
    std::string text (tl::to_string (mp_ui->tolerances->text ()));
    tl::Extractor ex (text.c_str ());
    while (! ex.at_end ()) {
      double t;
      if (! ex.try_read (t) || t < -1e-6) {
        break;
      }
      ex.test (",");
    }
  }

  {
    std::string text (tl::to_string (mp_ui->tiling->text ()));
    tl::Extractor ex (text.c_str ());
    double t = 0.0;
    if (ex.try_read (t)) {
      if (t < 0.001) {
        throw tl::Exception (tl::to_string (QObject::tr ("Invalid tile size (invalid text or negative)")));  
      }
    }
  }
  
  lay::Dispatcher *config_root = lay::Dispatcher::instance ();

  config_root->config_set (cfg_xor_input_mode, InputModeConverter ().to_string ((input_mode_t) mp_ui->input_layers_cbx->currentIndex ()));
  config_root->config_set (cfg_xor_output_mode, OutputModeConverter ().to_string ((output_mode_t) mp_ui->output_cbx->currentIndex ()));
  config_root->config_set (cfg_xor_region_mode, RegionModeConverter ().to_string ((region_mode_t) mp_ui->region_cbx->currentIndex ()));
  config_root->config_set (cfg_xor_axorb, mp_ui->axorb_cb->isChecked ());
  config_root->config_set (cfg_xor_anotb, mp_ui->anotb_cb->isChecked ());
  config_root->config_set (cfg_xor_bnota, mp_ui->bnota_cb->isChecked ());
  config_root->config_set (cfg_xor_nworkers, mp_ui->threads->value ());
  config_root->config_set (cfg_xor_layer_offset, tl::to_string (mp_ui->layer_offset_le->text ()));
  config_root->config_set (cfg_xor_summarize, mp_ui->summarize_cb->isChecked ());
  config_root->config_set (cfg_xor_tolerances, tl::to_string (mp_ui->tolerances->text ()));
  config_root->config_set (cfg_xor_deep, mp_ui->deep->isChecked ());
  config_root->config_set (cfg_xor_tiling, tl::to_string (mp_ui->tiling->text ()));
  config_root->config_set (cfg_xor_tiling_heal, mp_ui->heal_cb->isChecked ());
  config_root->config_end ();

  QDialog::accept ();

END_PROTECTED
}

void
XORToolDialog::deep_changed ()
{
  bool deep = mp_ui->deep->isChecked ();
  mp_ui->tiling->setEnabled (!deep);
  mp_ui->heal_cb->setEnabled (!deep);
}

void
XORToolDialog::input_changed (int /*index*/)
{
  // .. nothing yet ..
}

void
XORToolDialog::output_changed (int index)
{
  bool enabled = (index == 2 || index == 3);
  mp_ui->layer_offset_lbl->setEnabled (enabled);
  mp_ui->layer_offset_le->setEnabled (enabled);
}


class XORJob
  : public tl::JobBase
{
public:
  enum EmptyLayerHandling
  {
    EL_optimize,        // copy the non-empty contributions of a or b
    EL_summarize,       // print a message about leaving away some operations and don't do anything
    EL_process          // include in processing - the non-empty layer will be merged
  };

  XORJob (int nworkers, 
          output_mode_t output_mode, 
          db::BooleanOp::BoolOp op,
          EmptyLayerHandling el_handling,
          double dbu,
          const lay::CellView &cva, const lay::CellView &cvb,
          const std::vector <db::Coord> &tolerances,
          const std::vector <rdb::Category *> &sub_categories,
          const std::vector <std::vector <rdb::Category *> > &layer_categories,
          const std::vector <db::Cell *> &sub_cells,
          const std::vector <std::vector <unsigned int> > &sub_output_layers,
          rdb::Database *rdb,
          rdb::Cell *rdb_cell)
    : tl::JobBase (nworkers),
      m_output_mode (output_mode),
      m_op (op),
      m_el_handling (el_handling),
      m_has_tiles (false),
      m_tile_heal (false),
      m_dbu (dbu),
      m_cva (cva),
      m_cvb (cvb),
      m_tolerances (tolerances),
      m_sub_categories (sub_categories),
      m_layer_categories (layer_categories),
      m_sub_cells (sub_cells),
      m_sub_output_layers (sub_output_layers),
      m_rdb (rdb),
      m_rdb_cell (rdb_cell),
      m_progress (0),
      m_nx (0), m_ny (0)
  {
  }

  output_mode_t output_mode () const
  {
    return m_output_mode;
  }

  db::BooleanOp::BoolOp op () const
  {
    return m_op;
  }

  EmptyLayerHandling el_handling () const
  {
    return m_el_handling;
  }

  bool has_tiles () const
  {
    return m_has_tiles;
  }

  void set_tiles (bool ht, int nx, int ny, bool heal)
  {
    m_has_tiles = ht;
    m_nx = ht ? nx : 0;
    m_ny = ht ? ny : 0;
    m_tile_heal = heal;
  }

  double dbu () const
  {
    return m_dbu;
  }

  const lay::CellView &cva () const
  {
    return m_cva;
  }

  const lay::CellView &cvb () const
  {
    return m_cvb;
  }

  const std::vector <db::Coord> &tolerances () const
  {
    return m_tolerances;
  }

  const std::vector <std::vector <unsigned int> > &sub_output_layers () const
  {
    return m_sub_output_layers;
  }

  void next_progress () 
  {
    QMutexLocker locker (&m_mutex);
    ++m_progress;
  }

  void add_results (const db::LayerProperties &lp, db::Coord tol, size_t n, size_t ix, size_t iy)
  {
    QMutexLocker locker (&m_mutex);

    std::vector<std::vector<size_t> > &cc = m_results [std::make_pair (lp, tol)];
    if (cc.size () <= ix) {
      cc.resize (ix + 1, std::vector<size_t> ());
    }
    if (cc [ix].size () <= iy) {
      cc [ix].resize (iy + 1, 0);
    }

    if (n == missing_in_a || n == missing_in_b) {
      cc[ix][iy] = n;
    } else {
      //  NOTE: we will not get a "normal" n after missing_in_a or missing_in_b
      cc[ix][iy] += n;
    }
  }

  void update_progress (XORProgress &progress)
  {
    unsigned int p;
    {
      QMutexLocker locker (&m_mutex);
      p = m_progress;
      progress.configure (m_dbu, int (m_nx), int (m_ny), m_tolerances);
      progress.merge_results (m_results);
    }    

    progress.set (p, true /*force yield*/);
  }

  void issue_string (unsigned int tol_index, unsigned int layer_index, const std::string &s)
  {
    QMutexLocker locker (&m_mutex);

    if (m_output_mode == OMMarkerDatabase) {

      rdb::Category *layercat = m_layer_categories[tol_index][layer_index];

      rdb::Item *item = m_rdb->create_item (m_rdb_cell->id (), layercat->id ());
      item->values ().add (new rdb::Value <std::string> (s));

    }
  }

  void issue_region (unsigned int tol_index, unsigned int layer_index, const db::Region &region)
  {
    QMutexLocker locker (&m_mutex);
    db::CplxTrans trans (dbu ());

    if (m_output_mode == OMMarkerDatabase) {

      rdb::Category *layercat = m_layer_categories[tol_index][layer_index];

      std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = region.begin_iter ();
      rdb::scan_layer (layercat, m_rdb_cell, trans * it.second, it.first, false);

    } else {

      db::Cell *output_cell = m_sub_cells[tol_index];
      unsigned int output_layer = m_sub_output_layers[tol_index][layer_index];

      region.insert_into (output_cell->layout (), output_cell->cell_index (), output_layer);

    }
  }

  void issue_polygon (unsigned int tol_index, unsigned int layer_index, const db::Polygon &polygon, bool touches_border = false)
  {
    QMutexLocker locker (&m_mutex);
    db::CplxTrans trans (dbu ());

    if (m_tile_heal && touches_border) {

      //  save for merging later
      m_polygons_to_heal [std::make_pair (tol_index, layer_index)].insert (polygon);

    } else if (m_output_mode == OMMarkerDatabase) {

      rdb::Category *layercat = m_layer_categories[tol_index][layer_index];

      rdb::Item *item = m_rdb->create_item (m_rdb_cell->id (), layercat->id ());
      item->values ().add (new rdb::Value <db::DPolygon> (polygon.transformed (trans)));

    } else {

      db::Cell *subcell = 0;
      unsigned int layout_layer = 0;

      subcell = m_sub_cells[tol_index];
      layout_layer = m_sub_output_layers[tol_index][layer_index];

      double factor = 1.0;
      if (subcell->layout ()) {
        factor = dbu () / subcell->layout ()->dbu ();
      }
      if (tl::equal (factor, 1.0)) {
        subcell->shapes (layout_layer).insert (polygon);
      } else {
        subcell->shapes (layout_layer).insert (db::Polygon (polygon * factor));
      }

    }
  }

  void finish ()
  {
    //  merge the polygons to heal and re-issue (this time without healing)
    for (std::map<std::pair<size_t, size_t>, db::Region>::iterator p = m_polygons_to_heal.begin (); p != m_polygons_to_heal.end (); ++p) {
      for (db::Region::const_iterator mp = p->second.begin_merged (); !mp.at_end (); ++mp) {
        issue_polygon ((unsigned int) p->first.first, (unsigned int) p->first.second, *mp, false);
      }
    }
  }

  virtual tl::Worker *create_worker ();

private:
  output_mode_t m_output_mode;
  db::BooleanOp::BoolOp m_op;
  EmptyLayerHandling m_el_handling;
  bool m_has_tiles;
  bool m_tile_heal;
  double m_dbu;
  lay::CellView m_cva;
  lay::CellView m_cvb;
  std::vector <db::Coord> m_tolerances;
  std::vector <rdb::Category *> m_sub_categories;
  std::vector <std::vector <rdb::Category *> > m_layer_categories;
  std::vector <db::Cell *> m_sub_cells;
  std::vector <std::vector <unsigned int> > m_sub_output_layers;
  rdb::Database *m_rdb;
  rdb::Cell *m_rdb_cell;
  unsigned int m_progress;
  QMutex m_mutex;
  std::string m_result_string;
  size_t m_nx, m_ny;
  std::map<std::pair<db::LayerProperties, db::Coord>, std::vector<std::vector<size_t> > > m_results;
  std::map<std::pair<size_t, size_t>, db::Region> m_polygons_to_heal;
};

class XORTask
  : public tl::Task
{
public:
  XORTask (bool deep, const std::string &tile_desc, const db::Box &clip_box, const db::Box &region_a, const db::Box &region_b, unsigned int layer_index, const db::LayerProperties &lp, const std::vector<unsigned int> &la, const std::vector<unsigned int> &lb, int ix, int iy)
    : m_deep (deep), m_tile_desc (tile_desc), m_clip_box (clip_box), m_region_a (region_a), m_region_b (region_b), m_layer_index (layer_index), m_lp (lp), m_la (la), m_lb (lb), m_ix (ix), m_iy (iy)
  {
    //  .. nothing yet ..
  }

  bool deep () const
  {
    return m_deep;
  }

  const std::string &tile_desc () const
  {
    return m_tile_desc;
  }
  
  const db::Box &clip_box () const
  {
    return m_clip_box;
  }
  
  const db::Box &region_a () const
  {
    return m_region_a;
  }
  
  const db::Box &region_b () const
  {
    return m_region_b;
  }

  const std::vector<unsigned int> &la () const 
  {
    return m_la;
  }

  const std::vector<unsigned int> &lb () const 
  {
    return m_lb;
  }

  unsigned int layer_index () const
  {
    return m_layer_index;
  }

  const db::LayerProperties &lp () const
  {
    return m_lp;
  }

  int ix () const
  {
    return m_ix;
  }

  int iy () const
  {
    return m_iy;
  }

private:
  bool m_deep;
  std::string m_tile_desc;
  db::Box m_clip_box, m_region_a, m_region_b;
  unsigned int m_layer_index;
  db::LayerProperties m_lp;
  std::vector<unsigned int> m_la, m_lb;
  int m_ix, m_iy;
};

class XORWorker
  : public tl::Worker
{
public:
  XORWorker (XORJob *job)
    : tl::Worker (), mp_job (job)
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task) 
  {
    XORTask *xor_task = dynamic_cast <XORTask *> (task);
    if (xor_task) {
      do_perform (xor_task);
    }
  }

private:
  XORJob *mp_job;

  void do_perform (const XORTask *task);
  void do_perform_tiled (const XORTask *task);
  void do_perform_deep (const XORTask *task);
};

void
XORWorker::do_perform (const XORTask *xor_task)
{
  if (xor_task->deep ()) {
    do_perform_deep (xor_task);
  } else {
    do_perform_tiled (xor_task);
  }
}

void
XORWorker::do_perform_deep (const XORTask *xor_task)
{
  db::DeepShapeStore dss;
  db::Region rr;

  unsigned int tol_index = 0;
  for (std::vector <db::Coord>::const_iterator t = mp_job->tolerances ().begin (); t != mp_job->tolerances ().end (); ++t, ++tol_index) {

    const std::vector<unsigned int> &la = xor_task->la ();
    const std::vector<unsigned int> &lb = xor_task->lb ();

    if ((!la.empty () && !lb.empty ()) || mp_job->el_handling () != XORJob::EL_summarize) {

      if (tl::verbosity () >= 10) {
        tl::info << "XOR tool (hierarchical): layer " << xor_task->lp ().to_string () << ", tolerance " << *t * mp_job->dbu ();
      }

      tl::SelfTimer timer (tl::verbosity () >= 11, "Elapsed time");

      if (tol_index == 0) {

        if ((!la.empty () && !lb.empty ()) || mp_job->el_handling () == XORJob::EL_process) {

          tl::SelfTimer timer (tl::verbosity () >= 21, "Boolean part");

          db::RecursiveShapeIterator s_a (mp_job->cva ()->layout (), mp_job->cva ()->layout ().cell (mp_job->cva ().cell_index ()), la, xor_task->region_a ());
          db::RecursiveShapeIterator s_b (mp_job->cvb ()->layout (), mp_job->cvb ()->layout ().cell (mp_job->cvb ().cell_index ()), lb, xor_task->region_b ());

          s_a.set_for_merged_input (true);
          s_b.set_for_merged_input (true);

          db::Region ra (s_a, dss, db::ICplxTrans (mp_job->cva ()->layout ().dbu () / mp_job->dbu ()));
          db::Region rb (s_b, dss, db::ICplxTrans (mp_job->cvb ()->layout ().dbu () / mp_job->dbu ()));

          if (mp_job->op () == db::BooleanOp::Xor) {
            rr = ra ^ rb;
          } else if (mp_job->op () == db::BooleanOp::ANotB) {
            rr = ra - rb;
          } else if (mp_job->op () == db::BooleanOp::BNotA) {
            rr = rb - ra;
          }

        } else if (mp_job->op () == db::BooleanOp::Xor ||
                   (mp_job->op () == db::BooleanOp::ANotB && !la.empty ()) ||
                   (mp_job->op () == db::BooleanOp::BNotA && !lb.empty ())) {

          tl::SelfTimer timer (tl::verbosity () >= 21, "Boolean part (shortcut)");

          db::RecursiveShapeIterator s;
          db::ICplxTrans dbu_scale;

          if (!la.empty ()) {
            s = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la, xor_task->region_a ());
            dbu_scale = db::ICplxTrans (mp_job->cva ()->layout ().dbu () / mp_job->dbu ());
          } else if (!lb.empty ()) {
            s = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb, xor_task->region_b ());
            dbu_scale = db::ICplxTrans (mp_job->cvb ()->layout ().dbu () / mp_job->dbu ());
          }

          s.set_for_merged_input (true);

          rr = db::Region (s, dss, dbu_scale);

        }

      }

      if (*t > 0) {
        tl::SelfTimer timer (tl::verbosity () >= 21, "Sizing part");
        rr.size (-((*t + 1) / 2), (unsigned int)2);
        rr.size (((*t + 1) / 2), (unsigned int)2);
      }

      //  TODO: no clipping for hierarchical mode yet
      mp_job->issue_region (tol_index, xor_task->layer_index (), rr);

      mp_job->add_results (xor_task->lp (), *t, rr.count (), xor_task->ix (), xor_task->iy ());

    } else if (mp_job->op () == db::BooleanOp::Xor ||
               (mp_job->op () == db::BooleanOp::ANotB && !la.empty ()) ||
               (mp_job->op () == db::BooleanOp::BNotA && !lb.empty ())) {

      if (!la.empty ()) {
        mp_job->issue_string (tol_index, xor_task->layer_index (), tl::to_string (QObject::tr ("Layer not present at all in layout B")));
        mp_job->add_results (xor_task->lp (), *t, missing_in_b, 0, 0);
      }

      if (!lb.empty ()) {
        mp_job->issue_string (tol_index, xor_task->layer_index (), tl::to_string (QObject::tr ("Layer not present at all in layout A")));
        mp_job->add_results (xor_task->lp (), *t, missing_in_a, 0, 0);
      }

    }

    mp_job->next_progress ();

  }
}

void
XORWorker::do_perform_tiled (const XORTask *xor_task)
{
  db::ShapeProcessor sp (true);

  //  prepare a layout for the results
  db::Layout xor_results;
  xor_results.dbu (mp_job->dbu ());
  db::Cell &xor_results_cell = xor_results.cell (xor_results.add_cell ());
  xor_results.insert_layer (0);

  unsigned int tol_index = 0;
  for (std::vector <db::Coord>::const_iterator t = mp_job->tolerances ().begin (); t != mp_job->tolerances ().end (); ++t, ++tol_index) {

    const std::vector<unsigned int> &la = xor_task->la ();
    const std::vector<unsigned int> &lb = xor_task->lb ();

    if ((!la.empty () && !lb.empty ()) || mp_job->el_handling () != XORJob::EL_summarize) {

      if (tl::verbosity () >= (mp_job->has_tiles () ? 20 : 10)) {
        tl::info << "XOR tool: layer " << xor_task->lp ().to_string () << ", tolerance " << *t * mp_job->dbu () << ", tile " << xor_task->tile_desc ();
      }

      tl::SelfTimer timer (tl::verbosity () >= (mp_job->has_tiles () ? 21 : 11), "Elapsed time");

      if (tol_index == 0) {

        if ((!la.empty () && !lb.empty ()) || mp_job->el_handling () == XORJob::EL_process) {

          tl::SelfTimer timer (tl::verbosity () >= 31, "Boolean part");
          size_t n;

          if (! merge_before_bool ()) {

            //  Straightforward implementation
            sp.clear ();

            db::CplxTrans dbu_scale_a (mp_job->cva ()->layout ().dbu () / xor_results.dbu ());
            db::CplxTrans dbu_scale_b (mp_job->cvb ()->layout ().dbu () / xor_results.dbu ());

            n = 0;
            db::RecursiveShapeIterator s_a;
            if (mp_job->has_tiles ()) {
              s_a = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la, xor_task->region_a ());
            } else {
              s_a = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la);
            }
            s_a.set_for_merged_input (true);
            for ( ; ! s_a.at_end (); ++s_a, ++n) {
              sp.insert (s_a.shape (), dbu_scale_a * s_a.trans (), n * 2);
            }

            n = 0;
            db::RecursiveShapeIterator s_b;
            if (mp_job->has_tiles ()) {
              s_b = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb, xor_task->region_b ());
            } else {
              s_b = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb);
            }
            s_b.set_for_merged_input (true);
            for (; ! s_b.at_end (); ++s_b, ++n) {
              sp.insert (s_b.shape (), dbu_scale_b * s_b.trans (), n * 2 + 1);
            }

            db::BooleanOp bool_op (mp_job->op ());
            db::ShapeGenerator sg (xor_results_cell.shapes (0), true /*clear shapes*/);
            db::PolygonGenerator out (sg, false /*don't resolve holes*/, false /*no min. coherence*/);
            sp.process (out, bool_op);

          } else {

            //  This implementation is faster when a lot of overlapping shapes are involved
            db::Layout merge_helper;
            merge_helper.dbu (mp_job->dbu ());
            db::Cell &merge_helper_cell = merge_helper.cell (merge_helper.add_cell ());
            merge_helper.insert_layer (0);
            merge_helper.insert_layer (1);

            //  This implementation is faster when a lot of overlapping shapes are involved
            if (!la.empty ()) {

              sp.clear ();

              db::CplxTrans dbu_scale (mp_job->cva ()->layout ().dbu () / xor_results.dbu ());

              n = 0;
              db::RecursiveShapeIterator s;
              if (mp_job->has_tiles ()) {
                s = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la, xor_task->region_a ());
              } else {
                s = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la);
              }
              s.set_for_merged_input (true);
              for ( ; ! s.at_end (); ++s, ++n) {
                sp.insert (s.shape (), dbu_scale * s.trans (), n);
              }

              db::MergeOp op (0);
              db::ShapeGenerator sg (merge_helper_cell.shapes (0), true /*clear shapes*/);
              db::PolygonGenerator out (sg, false /*don't resolve holes*/, false /*no min. coherence*/);
              sp.process (out, op);

            }

            if (!lb.empty ()) {

              sp.clear ();

              db::CplxTrans dbu_scale (mp_job->cvb ()->layout ().dbu () / xor_results.dbu ());

              n = 0;
              db::RecursiveShapeIterator s;
              if (mp_job->has_tiles ()) {
                s = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb, xor_task->region_b ());
              } else {
                s = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb);
              }
              s.set_for_merged_input (true);
              for ( ; ! s.at_end (); ++s, ++n) {
                sp.insert (s.shape (), dbu_scale * s.trans (), n);
              }

              db::MergeOp op (0);
              db::ShapeGenerator sg (merge_helper_cell.shapes (1), true /*clear shapes*/);
              db::PolygonGenerator out (sg, false /*don't resolve holes*/, false /*no min. coherence*/);
              sp.process (out, op);

            }

            sp.boolean (merge_helper, merge_helper_cell, 0,
                        merge_helper, merge_helper_cell, 1,
                        xor_results_cell.shapes (0), mp_job->op (), true, false, true);

          }

        } else if (mp_job->op () == db::BooleanOp::Xor || 
                   (mp_job->op () == db::BooleanOp::ANotB && !la.empty ()) ||
                   (mp_job->op () == db::BooleanOp::BNotA && !lb.empty ())) {

          db::RecursiveShapeIterator s;
          db::CplxTrans dbu_scale;

          if (!la.empty ()) {
            if (mp_job->has_tiles ()) {
              s = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la, xor_task->region_a ());
            } else {
              s = db::RecursiveShapeIterator (mp_job->cva ()->layout (), *mp_job->cva ().cell (), la);
            }
            dbu_scale = db::CplxTrans (mp_job->cva ()->layout ().dbu () / xor_results.dbu ());
          } else if (!lb.empty ()) {
            if (mp_job->has_tiles ()) {
              s = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb, xor_task->region_b ());
            } else {
              s = db::RecursiveShapeIterator (mp_job->cvb ()->layout (), *mp_job->cvb ().cell (), lb);
            }
            dbu_scale = db::CplxTrans (mp_job->cvb ()->layout ().dbu () / xor_results.dbu ());
          }

          s.set_for_merged_input (true);

          for (; ! s.at_end (); ++s) {
            if (s->is_polygon () || s->is_box () || s->is_path ()) {
              db::Polygon p;
              s->polygon (p);
              p.transform (dbu_scale * s.trans ());
              xor_results_cell.shapes (0).insert (p);
            }
          }

        }

      }

      if (*t > 0) {
        tl::SelfTimer timer (tl::verbosity () >= (mp_job->has_tiles () ? 31 : 21), "Sizing part");
        sp.size (xor_results, xor_results_cell, 0, xor_results_cell.shapes (0), -((*t + 1) / 2), (unsigned int)2, false);
        sp.size (xor_results, xor_results_cell, 0, xor_results_cell.shapes (0), ((*t + 1) / 2), (unsigned int)2, false);
      }

      size_t n = 0;

      for (db::Shapes::shape_iterator s = xor_results_cell.shapes (0).begin (db::ShapeIterator::All); ! s.at_end (); ++s) {

        if (mp_job->has_tiles ()) {

          std::vector <db::Polygon> clipped_poly;
          clip_poly (s->polygon (), xor_task->clip_box (), clipped_poly, false /*don't resolve holes*/);
          db::Box inner = xor_task->clip_box ().enlarged (db::Vector (-1, -1));

          for (std::vector <db::Polygon>::const_iterator cp = clipped_poly.begin (); cp != clipped_poly.end (); ++cp) {

            mp_job->issue_polygon (tol_index, xor_task->layer_index (), *cp, !cp->box ().inside (inner));
            ++n;
          }
        
        } else {
          mp_job->issue_polygon (tol_index, xor_task->layer_index (), s->polygon ());
          ++n;
        }

      }

      mp_job->add_results (xor_task->lp (), *t, n, xor_task->ix (), xor_task->iy ());

    } else if (mp_job->op () == db::BooleanOp::Xor || 
               (mp_job->op () == db::BooleanOp::ANotB && !la.empty ()) ||
               (mp_job->op () == db::BooleanOp::BNotA && !lb.empty ())) {

      if (!la.empty ()) {
        mp_job->issue_string (tol_index, xor_task->layer_index (), tl::to_string (QObject::tr ("Layer not present at all in layout B")));
        mp_job->add_results (xor_task->lp (), *t, missing_in_b, 0, 0);
      }

      if (!lb.empty ()) {
        mp_job->issue_string (tol_index, xor_task->layer_index (), tl::to_string (QObject::tr ("Layer not present at all in layout A")));
        mp_job->add_results (xor_task->lp (), *t, missing_in_a, 0, 0);
      }

    }
    
    mp_job->next_progress ();

  }

}

tl::Worker *
XORJob::create_worker ()
{
  return new XORWorker (this);
}

void 
XORToolDialog::run_xor ()
{
  input_mode_t input_mode = (input_mode_t) mp_ui->input_layers_cbx->currentIndex ();
  output_mode_t output_mode = (output_mode_t) mp_ui->output_cbx->currentIndex ();
  region_mode_t region_mode = (region_mode_t) mp_ui->region_cbx->currentIndex ();

  int nworkers = mp_ui->threads->value ();

  db::LayerOffset layer_offset;
  if (output_mode == OMNewLayersA || output_mode == OMNewLayersB) {
    std::string lo = tl::to_string (mp_ui->layer_offset_le->text ());
    tl::Extractor ex (lo.c_str ());
    layer_offset.read (ex);
  }

  bool axorb = mp_ui->axorb_cb->isChecked ();
  bool anotb = mp_ui->anotb_cb->isChecked ();
  bool bnota = mp_ui->bnota_cb->isChecked ();

  bool deep = mp_ui->deep->isChecked ();

  bool summarize = mp_ui->summarize_cb->isChecked ();
  //  TODO: make this a user interface feature later
  bool process_el = tl::app_flag ("always-do-xor");

  int cv_index_a = mp_ui->layouta->current_cv_index ();
  int cv_index_b = mp_ui->layoutb->current_cv_index ();

  lay::CellView cva = mp_view->cellview (cv_index_a);
  lay::CellView cvb = mp_view->cellview (cv_index_b);

  //  NOTE: basically we should take the common denominator rather than the minimum of the layout's DBU's. 
  //  But this could be a very small number resulting in coordinate overflow issues. 
  double dbu = std::min (cva->layout ().dbu (), cvb->layout ().dbu ());

  std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc> layers;

  for (db::Layout::layer_iterator la = cva->layout ().begin_layers (); la != cva->layout ().end_layers (); ++la) {
    layers[*(*la).second].first.push_back ((*la).first);
  }

  for (db::Layout::layer_iterator lb = cvb->layout ().begin_layers (); lb != cvb->layout ().end_layers (); ++lb) {
    layers[*(*lb).second].second.push_back ((*lb).first);
  }

  //  Keep only visible layers if requested. Treat invisible ones as empty.
  if (input_mode == IMVisible) {

    std::set<unsigned int> visible_layers_a;
    std::set<unsigned int> visible_layers_b;

    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); l != mp_view->end_layers (); ++l) {
      if (! l->has_children () && l->layer_index () >= 0 && l->cellview_index () == cv_index_a && l->visible (true)) {
        visible_layers_a.insert (l->layer_index ());
      }
      if (! l->has_children () && l->layer_index () >= 0 && l->cellview_index () == cv_index_b && l->visible (true)) {
        visible_layers_b.insert (l->layer_index ());
      }
    }

    for (std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc>::iterator lm = layers.begin (); lm != layers.end (); ) {

      std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc>::iterator lm_next = lm;
      ++lm_next;

      std::sort (lm->second.first.begin (), lm->second.first.end ());
      std::sort (lm->second.second.begin (), lm->second.second.end ());

      lm->second.first.erase (std::set_intersection (lm->second.first.begin (), lm->second.first.end (), visible_layers_a.begin (), visible_layers_a.end (), lm->second.first.begin ()), lm->second.first.end ());
      lm->second.second.erase (std::set_intersection (lm->second.second.begin (), lm->second.second.end (), visible_layers_b.begin (), visible_layers_b.end (), lm->second.second.begin ()), lm->second.second.end ());

      if (lm->second.first.empty () && lm->second.second.empty ()) {
        layers.erase (lm);
      }

      lm = lm_next;

    }

  }

  std::vector <db::Coord> tolerances;

  {
    std::string text (tl::to_string (mp_ui->tolerances->text ()));
    tl::Extractor ex (text.c_str ());
    while (! ex.at_end ()) {
      double t;
      if (! ex.try_read (t) || t < -1e-6) {
        break;
      }
      ex.test (",");
      tolerances.push_back (db::coord_traits <db::Coord>::rounded (t / dbu));
    }

    std::sort (tolerances.begin (), tolerances.end ());
    tolerances.erase (std::unique (tolerances.begin (), tolerances.end ()), tolerances.end ());

    if (tolerances.size () == 0) {
      tolerances.push_back (0);
    }
  }

  //  Create a map of new layers for original ones 
  std::map <db::LayerProperties, db::LayerProperties> new_layer_props;
  for (std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc>::iterator lm = layers.begin (); lm != layers.end (); ++lm) {
    new_layer_props.insert (std::make_pair (lm->first, lm->first));
  }

  double tile_size = 0; // in micron units
  bool tile_heal = mp_ui->heal_cb->isChecked ();

  if (! deep) {
    std::string text (tl::to_string (mp_ui->tiling->text ()));
    tl::Extractor ex (text.c_str ());
    double t = 0.0;
    if (ex.try_read (t)) {
      tile_size = t;
      if (tile_size < 1.0) {
        throw tl::Exception (tl::to_string (QObject::tr ("Invalid tile size (smaller than 1 micron or negative)")));  
      }
    }
  }
  
  std::string srca = cva->name () + ", Cell " + cva->layout ().cell_name (cva.cell_index ());
  std::string srcb = cvb->name () + ", Cell " + cvb->layout ().cell_name (cvb.cell_index ());

  //  Create the report database or identify the output layout
  rdb::Database *rdb = 0;
  rdb::Cell *rdb_cell = 0;
  int rdb_index = 0;

  int output_cv = -1;
  db::Layout *output_layout = 0;
  db::Cell *output_cell = 0;
  std::vector <unsigned int> output_layers;

  if (output_mode == OMMarkerDatabase) {

    rdb = new rdb::Database ();
    rdb->set_name ("XOR");
    rdb->set_top_cell_name (cva->layout ().cell_name (cva.cell_index ()));
    rdb_cell = rdb->create_cell (rdb->top_cell_name ());

    rdb_index = mp_view->add_rdb (rdb);

    rdb->set_description ("Comparison of '" + srca + "' vs. '" + srcb + "'");

  } else if (output_mode == OMNewLayout) {

    output_cv = mp_view->create_layout (true);
    output_layout = &mp_view->cellview (output_cv)->layout ();
    output_layout->dbu (dbu);

    for (std::map <db::LayerProperties, db::LayerProperties>::const_iterator lp = new_layer_props.begin (); lp != new_layer_props.end (); ++lp) {
      output_layers.push_back (output_layout->insert_layer (lp->second));
      lay::LayerProperties lay_lp;
      lay_lp.set_source (lay::ParsedLayerSource (lp->second, output_cv));
      mp_view->init_layer_properties (lay_lp);
      mp_view->insert_layer (mp_view->end_layers (), lay_lp);
    }

  } else if (output_mode == OMNewLayersA) {

    output_cv = cv_index_a;
    output_layout = &cva->layout ();
    output_cell = cva.cell ();

  } else if (output_mode == OMNewLayersB) {

    output_cv = cv_index_b;
    output_layout = &cvb->layout ();
    output_cell = cvb.cell ();

  }

  //  Clear undo buffers if layout is created.
  if (output_layout) {
    mp_view->manager ()->clear ();
  }

  std::vector<db::DBox> boxes; 

  db::DBox overall_box = (db::DBox (cva.cell ()->bbox ()) * cva->layout ().dbu ()) + (db::DBox (cvb.cell ()->bbox ()) * cvb->layout ().dbu ());

  if (region_mode == RMVisible) {
    overall_box &= mp_view->viewport ().box ();
    boxes.push_back (overall_box);
  } else if (region_mode == RMRulers) {
    ant::Service *ant_service = mp_view->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        boxes.push_back (overall_box & db::DBox (ant->p1 (), ant->p2 ()));
        ++ant;
      }
    }
  } else {
    boxes.push_back (overall_box);
  }

  bool was_cancelled = false;
  for (int mode = 0; mode < 3 && ! was_cancelled; ++mode) {

    rdb::Category *cat = 0; 
    db::BooleanOp::BoolOp op;
    std::string op_name;
    std::string op_desc;

    if (mode == 0 && axorb) {
      op = db::BooleanOp::Xor;
      op_name = "XOR";
      op_desc = "XOR between '" + srca + "' (Layout A) and '" + srcb + "' (Layout B)";
    } else if (mode == 1 && anotb) {
      op = db::BooleanOp::ANotB;
      op_name = "ANOTB";
      op_desc = "Geometry in '" + srca + "' (Layout A) but not in '" + srcb + "' (Layout B)";
    } else if (mode == 2 && bnota) {
      op = db::BooleanOp::BNotA;
      op_name = "BNOTA";
      op_desc = "Geometry in '" + srca + "' (Layout B) but not in '" + srcb + "' (Layout A)";
    } else {
      continue;
    }

    if (output_mode == OMMarkerDatabase) {
      cat = rdb->create_category (op_name);
      cat->set_description (op_desc);
    } else if (output_mode == OMNewLayout) {
      output_cell = &output_layout->cell (output_layout->add_cell (op_name.c_str ()));
    }

    std::vector <rdb::Category *> sub_categories;
    std::vector <std::vector <rdb::Category *> > layer_categories;
    std::vector <db::Cell *> sub_cells;
    std::vector <std::vector <unsigned int> > sub_output_layers;

    if (output_mode == OMMarkerDatabase) {

      //  create the categories for database output

      if (tolerances.size () == 1) {

        sub_categories.push_back (cat);

      } else {

        for (std::vector <db::Coord>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

          rdb::Category *subcat;
          subcat = rdb->create_category (cat, tl::sprintf ("Tol_%g", *t * dbu));
          subcat->set_description (tl::sprintf ("XOR tolerance (min width reported): %g um", *t * dbu));
          sub_categories.push_back (subcat);

        }

      }

      layer_categories.reserve (sub_categories.size ());
      for (size_t i = 0; i < sub_categories.size (); ++i) {

        layer_categories.push_back (std::vector <rdb::Category *> ());

        for (std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

          rdb::Category *layercat = rdb->create_category (sub_categories [i], l->first.to_string ());
          layercat->set_description ("Results for layer " + l->first.to_string ());
          layer_categories.back ().push_back (layercat);

        }

      }

    } else if (output_mode == OMNewLayout) {

      if (tolerances.size () == 1) {

        sub_cells.push_back (output_cell);
        sub_output_layers.push_back (output_layers);

      } else {

        for (std::vector <db::Coord>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

          sub_cells.push_back (&output_layout->cell (output_layout->add_cell (tl::sprintf ("%s_TOL_%g", op_name, *t * dbu).c_str ())));
          output_cell->insert (db::CellInstArray (db::CellInst (sub_cells.back ()->cell_index ()), db::Trans ()));

          sub_output_layers.push_back (output_layers);

        }

      }

    } else if (output_mode == OMNewLayersA || output_mode == OMNewLayersB) {

      if (tolerances.size () == 1) {

        db::LayerOffset o = layer_offset;
        if (! o.is_named ()) {
          o.name = "*_" + op_desc; // "_XOR" postfix by default
        }

        for (std::map <db::LayerProperties, db::LayerProperties>::iterator lp = new_layer_props.begin (); lp != new_layer_props.end (); ++lp) {
          if (lp->first.is_named ()) {
            lp->second.name = lp->first.name;
          }
          lp->second += o;
        }

        output_layers.clear ();
        for (std::map <db::LayerProperties, db::LayerProperties>::iterator lp = new_layer_props.begin (); lp != new_layer_props.end (); ++lp) {
          output_layers.push_back (output_layout->insert_layer (lp->second));
          lay::LayerProperties lay_lp;
          lay_lp.set_source (lay::ParsedLayerSource (lp->second, output_cv));
          mp_view->init_layer_properties (lay_lp);
          mp_view->insert_layer (mp_view->end_layers (), lay_lp);
        }

        sub_cells.push_back (output_cell);
        sub_output_layers.push_back (output_layers);

      } else {

        for (std::vector <db::Coord>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

          db::LayerOffset o = layer_offset;
          if (! o.is_named ()) {
            o.name = "*_" + op_desc + tl::sprintf ("_T%d", int (t - tolerances.begin ()) + 1); // "_XOR" postfix by default
          }

          for (std::map <db::LayerProperties, db::LayerProperties>::iterator lp = new_layer_props.begin (); lp != new_layer_props.end (); ++lp) {
            if (lp->first.is_named ()) {
              lp->second.name = lp->first.name;
            }
            lp->second += o;
          }

          output_layers.clear ();
          for (std::map <db::LayerProperties, db::LayerProperties>::iterator lp = new_layer_props.begin (); lp != new_layer_props.end (); ++lp) {
            output_layers.push_back (output_layout->insert_layer (lp->second));
            lay::LayerProperties lay_lp;
            lay_lp.set_source (lay::ParsedLayerSource (lp->second, output_cv));
            mp_view->init_layer_properties (lay_lp);
            mp_view->insert_layer (mp_view->end_layers (), lay_lp);
          }

          sub_cells.push_back (output_cell);
          sub_output_layers.push_back (output_layers);

        }

      }

    }

    size_t todo_count = 0;
    XORJob::EmptyLayerHandling el_handling = XORJob::EL_optimize;
    if (summarize && output_mode == OMMarkerDatabase) {
      el_handling = XORJob::EL_summarize;
    } else if (process_el) {
      el_handling = XORJob::EL_process;
    }
    XORJob job (nworkers, output_mode, op, el_handling, dbu, cva, cvb, tolerances, sub_categories, layer_categories, sub_cells, sub_output_layers, rdb, rdb_cell);

    //  NOTE: uses min of both DBUs (see issue #1743)
    double common_dbu = std::min (cva->layout ().dbu (), cvb->layout ().dbu ());

    for (std::vector<db::DBox>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {

      db::DBox box (tl::round_down (b->left (), common_dbu), tl::round_down (b->bottom (), common_dbu),
                    tl::round_up (b->right (), common_dbu), tl::round_up (b->top (), common_dbu));

      //  compute the tiles if required
      db::Box box_a, box_b, box_out;
      db::Coord box_width_a = 0, box_height_a = 0, box_width_out = 0;
      db::Coord box_width_b = 0, box_height_b = 0, box_height_out = 0;

      size_t ntiles_w = 1, ntiles_h = 1;
      if (box.empty ()) {

        ntiles_w = ntiles_h = 0;

      } else {

        box_a = db::Box (box * (1.0 / cva->layout ().dbu ()));
        box_b = db::Box (box * (1.0 / cvb->layout ().dbu ()));
        box_out = db::Box (box * (1.0 / dbu));

        if (tile_size > 0.0) {

          ntiles_w = std::max (size_t (1), size_t (floor (box.width () / tile_size + 0.5)));
          ntiles_h = std::max (size_t (1), size_t (floor (box.height () / tile_size + 0.5)));

          double box_width = tl::round_up (box.width () / ntiles_w, common_dbu);
          double box_height = tl::round_up (box.height () / ntiles_h, common_dbu);

          box_width_a  = db::coord_traits<db::Coord>::rounded (box_width / cva->layout ().dbu ());
          box_height_a  = db::coord_traits<db::Coord>::rounded (box_height / cva->layout ().dbu ());

          box_width_b  = db::coord_traits<db::Coord>::rounded (box_width / cvb->layout ().dbu ());
          box_height_b  = db::coord_traits<db::Coord>::rounded (box_height / cvb->layout ().dbu ());

          box_width_out  = db::coord_traits<db::Coord>::rounded (box_width / dbu);
          box_height_out  = db::coord_traits<db::Coord>::rounded (box_height / dbu);

        }

      }

      //  Enlarge the tiles by half the maximum tolerance
      db::Coord tile_enlargement = 0;
      for (std::vector <db::Coord>::iterator t = tolerances.begin (); t != tolerances.end (); ++t) {
        db::Coord enlargement = (*t + 1) / 2; // round up
        if (enlargement > tile_enlargement) {
          tile_enlargement = enlargement;
        }
      }

      db::Coord tile_enlargement_a = db::coord_traits<db::Coord>::rounded_up (tile_enlargement * dbu / cva->layout ().dbu ());
      db::Coord tile_enlargement_b = db::coord_traits<db::Coord>::rounded_up (tile_enlargement * dbu / cvb->layout ().dbu ());

      if (ntiles_w > 1 || ntiles_h > 1 || region_mode != RMAll /*enforces clip*/) {
        job.set_tiles (true, int (ntiles_w), int (ntiles_h), tile_heal);
      }

      //  create the XOR tasks
      for (size_t nw = 0; nw < ntiles_w; ++nw) {

        for (size_t nh = 0; nh < ntiles_h; ++nh) {

          db::Box clip_box (box_out.left () + db::Coord (nw * box_width_out),
                            box_out.bottom () + db::Coord (nh * box_height_out),
                            (nw == ntiles_w - 1) ? box_out.right () : box_out.left () + db::Coord ((nw + 1) * box_width_out),
                            (nh == ntiles_h - 1) ? box_out.top () : box_out.bottom () + db::Coord ((nh + 1) * box_height_out));

          db::Box region_a (box_a.left () + db::Coord (nw * box_width_a),
                            box_a.bottom () + db::Coord (nh * box_height_a),
                            (nw == ntiles_w - 1) ? box_a.right () : box_a.left () + db::Coord ((nw + 1) * box_width_a),
                            (nh == ntiles_h - 1) ? box_a.top () : box_a.bottom () + db::Coord ((nh + 1) * box_height_a));

          db::Box region_b (box_b.left () + db::Coord (nw * box_width_b),
                            box_b.bottom () + db::Coord (nh * box_height_b),
                            (nw == ntiles_w - 1) ? box_b.right () : box_b.left () + db::Coord ((nw + 1) * box_width_b),
                            (nh == ntiles_h - 1) ? box_b.top () : box_b.bottom () + db::Coord ((nh + 1) * box_height_b));

          region_a.enlarge (db::Vector (tile_enlargement_a, tile_enlargement_a));
          region_b.enlarge (db::Vector (tile_enlargement_b, tile_enlargement_b));

          std::string tile_desc = tl::sprintf ("%d/%d,%d/%d", int (nw + 1), ntiles_w, int (nh + 1), ntiles_h);

          unsigned int layer_index = 0;
          for (std::map<db::LayerProperties, std::pair<std::vector<unsigned int>, std::vector<unsigned int> >, db::LPLogicalLessFunc>::const_iterator l = layers.begin (); l != layers.end (); ++l, ++layer_index) {
            job.schedule (new XORTask (deep, tile_desc, clip_box, region_a, region_b, layer_index, l->first, l->second.first, l->second.second, int (nw), int (nh)));
          }

        }

      }

      todo_count += ntiles_w * ntiles_h * tolerances.size () * layers.size ();

    }

    bool was_cancelled = false;

    if (todo_count > 0) {

      tl::SelfTimer timer_tot (tl::verbosity () >= 11, "Total boolean time");

      //  TODO: there should be a general scheme of how thread-specific progress is merged
      //  into a global one ..
      XORProgress progress (tl::to_string (QObject::tr ("Performing ")) + op_name, todo_count, 1);

      //  We need to lock the layouts during the processing - in OMNewLayerA and OMNewLayerB mode
      //  we actually modify the layout we iterate over
      db::LayoutLocker locker_a (& cva->layout ());
      db::LayoutLocker locker_b (& cvb->layout ());

      try {

        job.start ();
        while (job.is_running ()) {
          //  This may throw an exception, if the cancel button has been pressed.
          job.update_progress (progress);
          job.wait (100);
        }

      } catch (tl::BreakException & /*ex*/) {
        job.terminate ();
        was_cancelled = true;
      } catch (tl::Exception &ex) {
        job.terminate ();
        throw ex;
      }

      if (job.has_error ()) {
        if (mp_view && output_mode == OMMarkerDatabase) {
          mp_view->remove_rdb (rdb_index);
        }
        throw tl::Exception (tl::to_string (QObject::tr ("Errors occurred during processing. First error message says:\n")) + job.error_messages ().front ());
      }

      //  apply healing if required
      job.finish ();

    }

    if (was_cancelled && output_mode == OMMarkerDatabase) {
      //  If the output mode is database, ask whether to keep the data collected so far.
      //  If the answer is yes, remove the RDB.
      //  Don't ask if the application has exit (window was closed)
      if (lay::ApplicationBase::instance ()->main_window () && !lay::ApplicationBase::instance ()->main_window ()->exited ()) {
        QMessageBox msgbox (QMessageBox::Question,
                            QObject::tr ("Keep Data For Cancelled Job"),
                            QObject::tr ("The job has been cancelled. Keep the data collected so far?"),
                            QMessageBox::Yes | QMessageBox::No);
        if (msgbox.exec () == QMessageBox::No) {
          if (mp_view) {
            mp_view->remove_rdb (rdb_index);
          }
          output_mode = OMNone;
        }
      }
    }

  }

  if (mp_view) {

    if (output_mode == OMMarkerDatabase) {
      mp_view->open_rdb_browser (rdb_index, cv_index_a);
    }

    mp_view->update_content ();

    if (output_mode == OMNewLayout && output_cell != 0 && output_cv >= 0) {
      mp_view->select_cell (output_cell->cell_index (), output_cv);
    }
  }
}

}

