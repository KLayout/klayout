
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


#include "ui_ReplacePropertiesBox.h"
#include "ui_ReplacePropertiesInstance.h"
#include "ui_ReplacePropertiesPath.h"
#include "ui_ReplacePropertiesShape.h"
#include "ui_ReplacePropertiesText.h"
#include "ui_SearchPropertiesBox.h"
#include "ui_SearchPropertiesInstance.h"
#include "ui_SearchPropertiesPath.h"
#include "ui_SearchPropertiesShape.h"
#include "ui_SearchPropertiesText.h"
#include "ui_SearchReplaceDialog.h"

#include "laySearchReplacePropertiesWidgets.h"
#include "layMainWindow.h"

#include "dbStreamLayers.h"

#include "tlInternational.h"

#include <cctype>

namespace lay
{

// ----------------------------------------------------------------------------
//  Some definitions

static const char *cfg_suffix_instance_cellname_op = "-instance-cellname-op";
static const char *cfg_suffix_instance_cellname_value = "-instance-cellname-value";

static const char *cfg_suffix_shape_layer = "-shape-layer";
static const char *cfg_suffix_shape_area_op = "-shape-area-op";
static const char *cfg_suffix_shape_area_value = "-shape-area-value";
static const char *cfg_suffix_shape_perimeter_op = "-shape-perimeter-op";
static const char *cfg_suffix_shape_perimeter_value = "-shape-perimeter-value";

static const char *cfg_suffix_polygon_layer = "-polygon-layer";
static const char *cfg_suffix_polygon_area_op = "-polygon-area-op";
static const char *cfg_suffix_polygon_area_value = "-polygon-area-value";
static const char *cfg_suffix_polygon_perimeter_op = "-polygon-perimeter-op";
static const char *cfg_suffix_polygon_perimeter_value = "-polygon-perimeter-value";

static const char *cfg_suffix_box_layer = "-box-layer";
static const char *cfg_suffix_box_width_op = "-box-width-op";
static const char *cfg_suffix_box_width_value = "-box-width-value";
static const char *cfg_suffix_box_height_op = "-box-height-op";
static const char *cfg_suffix_box_height_value = "-box-height-value";

static const char *cfg_suffix_path_layer = "-path-layer";
static const char *cfg_suffix_path_width_op = "-path-width-op";
static const char *cfg_suffix_path_width_value = "-path-width-value";
static const char *cfg_suffix_path_length_op = "-path-length-op";
static const char *cfg_suffix_path_length_value = "-path-length-value";

static const char *cfg_suffix_text_layer = "-text-layer";
static const char *cfg_suffix_text_string_op = "-text-string-op";
static const char *cfg_suffix_text_string_value = "-text-string-value";
static const char *cfg_suffix_text_orientation_op = "-text-orientation-op";
static const char *cfg_suffix_text_orientation_value = "-text-orientation-value";
static const char *cfg_suffix_text_size_op = "-text-size-op";
static const char *cfg_suffix_text_size_value = "-text-size-value";

// ----------------------------------------------------------------------------
//  Some utilities

static std::string 
escape_string (const std::string &s, bool process_substring_refs = false, bool *has_substring_refs = 0)
{
  std::string r = "\"";

  for (const char *cp = s.c_str (); *cp; ++cp) {
    if (*cp == '"') {
      r += "\\\"";
    } else if (*cp == '\\' && isdigit (cp[1]) && process_substring_refs) {
      //  $1, $2 ... are the substring references
      ++cp;
      r += "\"+$";
      r += *cp;
      r += "+\"";
      if (has_substring_refs) {
        *has_substring_refs = true;
      }
    } else if (*cp == '\\') {
      r += "\\\\";
    } else {
      r += *cp;
    }
  }

  r += "\"";

  //  remove neutral components (i.e. ""+$2+"abc" -> $2+"abc")
  std::string rr;
  for (const char *cp = r.c_str (); *cp; ) {
    if ((cp[0] == '+' && cp[1] == '"' && cp[2] == '"') || (cp[0] == '"' && cp[1] == '"' && cp[2] == '+')) {
      cp += 3;
    } else {
      rr += *cp++;
    }
  }

  return rr;
}

static void 
configure_cbx (QComboBox *cbx, const std::string &value)
{
  cbx->setCurrentIndex (cbx->findText (QString (tl::to_qstring (value))));
}

static void
add_layer_clause (std::string &expr, LayerSelectionComboBox *layer)
{
  db::LayerMap lm;
  //  NOTE: going the way through the LayerMap object to obtain the string
  //  makes this code compatible with the query parser which uses the 
  //  LayerMap object too.
  lm.map (layer->current_layer_props (), 0);
  std::string v = lm.mapping_str (0);
  if (! v.empty ()) {
    expr += " on layer " + v;
  }
}

static void
add_numerical_condition (std::string &expr, QComboBox *op, QLineEdit *value, const char *attribute, const char *unit = 0)
{
  std::string v = tl::to_string (value->text ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += " && ";
    }

    double vv;
    tl::Extractor (v.c_str ()).read (vv);
    expr += attribute;
    expr += " ";
    expr += tl::to_string (op->currentText());
    expr += " " + tl::to_string (vv);
    if (unit) {
      expr += " ";
      expr += unit;
    }

  }
}

static void
add_string_condition (std::string &expr, QComboBox *op, QLineEdit *value, const char *attribute)
{
  std::string v = tl::to_string (value->text ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += " && ";
    }

    expr += attribute;
    expr += " ";
    expr += tl::to_string (op->currentText ());
    expr += " " + escape_string (v);

  }
}

static void
add_orientation_condition (std::string &expr, QComboBox *op, QComboBox *value, const char *attribute)
{
  std::string v = tl::to_string (value->currentText ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += " && ";
    }

    expr += attribute;
    expr += " ";
    expr += tl::to_string (op->currentText ());
    expr += " Trans." + v + ".rot";

  }
}

static void
add_layer_assignment (std::string &expr, LayerSelectionComboBox *value, const char *attribute)
{
  std::string v = value->current_layer_props ().to_string ();
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += "; ";
    }

    expr += attribute;
    expr += " = ";

    db::LayerProperties lp;
    tl::Extractor ex (v.c_str ());
    lp.read (ex);

    expr += "<" + lp.to_string () + ">";

  }
}

static void
add_cell_index_assignment (std::string &expr, QLineEdit *value, const char *attribute)
{
  std::string v = tl::to_string (value->text ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += "; ";
    }

    expr += attribute;

    bool needs_dynamic_eval = false;
    std::string cstr = escape_string (v, true, &needs_dynamic_eval);
    if (needs_dynamic_eval) {
      expr += " = layout.cell_by_name(";
      expr += cstr;
      expr += ")";
    } else {
      expr += " = <<";
      expr += cstr;
      expr += ">>";
    }

  }
}

static void
add_numerical_assignment (std::string &expr, QLineEdit *value, const char *attribute, const char *unit)
{
  std::string v = tl::to_string (value->text ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += "; ";
    }

    double vv;
    tl::Extractor (v.c_str ()).read (vv);
    expr += attribute;
    expr += " = ";
    expr += " " + tl::to_string (vv);

    if (unit) {
      expr += " ";
      expr += unit;
    }

  }
}

static void
add_string_assignment (std::string &expr, QLineEdit *value, const char *attribute)
{
  std::string v = tl::to_string (value->text ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += "; ";
    }

    expr += attribute;
    expr += " = ";
    expr += escape_string (v, true /*=process substring refs*/);

  }
}

static void
add_orientation_assignment (std::string &expr, QComboBox *value, const char *attribute)
{
  std::string v = tl::to_string (value->currentText ());
  if (! v.empty ()) {

    if (! expr.empty ()) {
      expr += "; ";
    }

    expr += attribute;
    expr += " = Trans.";
    expr += v;
    expr += ".rot";

  }
}

// ----------------------------------------------------------------------------

class SearchInstanceProperties
  : public SearchPropertiesWidget,
    private Ui::SearchPropertiesInstance
{
public:
  SearchInstanceProperties (QStackedWidget *sw, lay::LayoutViewBase * /*view*/, int /*cv_index*/)
    : SearchPropertiesWidget (sw)
  {
    setupUi (this);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_instance_cellname_op, v)) {
      configure_cbx (instance_cellname_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_instance_cellname_value, v)) {
      instance_cellname_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_instance_cellname_op, tl::to_string (instance_cellname_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_instance_cellname_value, tl::to_string (instance_cellname_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "instances of ";
    //  the cell expression may start of "instances of " itself - remove that one in that case
    if (std::string (cell_expr, 0, r.size ()) == r) {
      r = cell_expr;
    } else {
      r += cell_expr;
    }
    r += ".*";

    std::string expr;
    add_string_condition (expr, instance_cellname_op, instance_cellname_value, "cell_name");
    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Instance"));
  }
};

// ----------------------------------------------------------------------------

class SearchShapeProperties
  : public SearchPropertiesWidget,
    protected Ui::SearchPropertiesShape
{
public:
  SearchShapeProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : SearchPropertiesWidget (sw)
  {
    setupUi (this);

    shape_layer->set_view (view, cv_index);
    shape_layer->set_no_layer_available (true);
    shape_layer->set_new_layer_enabled (false);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_shape_layer, v)) {
      configure_cbx (shape_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_shape_area_op, v)) {
      configure_cbx (shape_area_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_shape_area_value, v)) {
      shape_area_value->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_shape_perimeter_op, v)) {
      configure_cbx (shape_perimeter_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_shape_perimeter_value, v)) {
      shape_perimeter_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_shape_layer, tl::to_string (shape_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_shape_area_op, tl::to_string (shape_area_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_shape_area_value, tl::to_string (shape_area_value->text ()));
    config_root->config_set (pfx + cfg_suffix_shape_perimeter_op, tl::to_string (shape_perimeter_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_shape_perimeter_value, tl::to_string (shape_perimeter_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "shapes";

    add_layer_clause (r, shape_layer);

    r += " from ";
    r += cell_expr;

    std::string expr;
    add_numerical_condition (expr, shape_area_op, shape_area_value, "shape.area", "um2");
    add_numerical_condition (expr, shape_perimeter_op, shape_perimeter_value, "shape.perimeter", "um");

    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Shape"));
  }
};

// ----------------------------------------------------------------------------

class SearchPolygonProperties
  : public SearchShapeProperties
{
public:
  SearchPolygonProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : SearchShapeProperties (sw, view, cv_index)
  {
    //  .. nothing yet ..
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_polygon_layer, v)) {
      configure_cbx (shape_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_polygon_area_op, v)) {
      configure_cbx (shape_area_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_polygon_area_value, v)) {
      shape_area_value->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_polygon_perimeter_op, v)) {
      configure_cbx (shape_perimeter_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_polygon_perimeter_value, v)) {
      shape_perimeter_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_polygon_layer, tl::to_string (shape_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_polygon_area_op, tl::to_string (shape_area_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_polygon_area_value, tl::to_string (shape_area_value->text ()));
    config_root->config_set (pfx + cfg_suffix_polygon_perimeter_op, tl::to_string (shape_perimeter_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_polygon_perimeter_value, tl::to_string (shape_perimeter_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "polygons";

    add_layer_clause (r, shape_layer);

    r += " from ";
    r += cell_expr;

    std::string expr;
    add_numerical_condition (expr, shape_area_op, shape_area_value, "shape.area", "um2");
    add_numerical_condition (expr, shape_perimeter_op, shape_perimeter_value, "shape.perimeter", "um");

    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Polygon"));
  }
};

// ----------------------------------------------------------------------------

class SearchBoxProperties
  : public SearchPropertiesWidget,
    private Ui::SearchPropertiesBox
{
public:
  SearchBoxProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : SearchPropertiesWidget (sw)
  {
    setupUi (this);

    box_layer->set_view (view, cv_index);
    box_layer->set_no_layer_available (true);
    box_layer->set_new_layer_enabled (false);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_box_layer, v)) {
      configure_cbx (box_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_box_width_op, v)) {
      configure_cbx (box_width_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_box_width_value, v)) {
      box_width_value->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_box_height_op, v)) {
      configure_cbx (box_height_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_box_height_value, v)) {
      box_height_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_box_layer, tl::to_string (box_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_box_width_op, tl::to_string (box_width_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_box_width_value, tl::to_string (box_width_value->text ()));
    config_root->config_set (pfx + cfg_suffix_box_height_op, tl::to_string (box_height_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_box_height_value, tl::to_string (box_height_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "boxes";

    add_layer_clause (r, box_layer);

    r += " from ";
    r += cell_expr;

    std::string expr;
    add_numerical_condition (expr, box_area_op, box_area_value, "shape.area", "um2");
    add_numerical_condition (expr, box_perimeter_op, box_perimeter_value, "shape.perimeter", "um");
    add_numerical_condition (expr, box_width_op, box_width_value, "shape.box_width", "um");
    add_numerical_condition (expr, box_height_op, box_height_value, "shape.box_height", "um");

    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Box"));
  }
};

// ----------------------------------------------------------------------------

class SearchPathProperties
  : public SearchPropertiesWidget,
    private Ui::SearchPropertiesPath
{
public:
  SearchPathProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : SearchPropertiesWidget (sw)
  {
    setupUi (this);

    path_layer->set_view (view, cv_index);
    path_layer->set_no_layer_available (true);
    path_layer->set_new_layer_enabled (false);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_path_layer, v)) {
      configure_cbx (path_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_path_width_op, v)) {
      configure_cbx (path_width_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_path_width_value, v)) {
      path_width_value->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_path_length_op, v)) {
      configure_cbx (path_length_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_path_length_value, v)) {
      path_length_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_path_layer, tl::to_string (path_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_path_width_op, tl::to_string (path_width_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_path_width_value, tl::to_string (path_width_value->text ()));
    config_root->config_set (pfx + cfg_suffix_path_length_op, tl::to_string (path_length_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_path_length_value, tl::to_string (path_length_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "paths";

    add_layer_clause (r, path_layer);

    r += " from ";
    r += cell_expr;

    std::string expr;
    add_numerical_condition (expr, path_width_op, path_width_value, "shape.path_width", "um");
    add_numerical_condition (expr, path_length_op, path_length_value, "shape.path_length", "um");

    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Path"));
  }
};

// ----------------------------------------------------------------------------

class SearchTextProperties
  : public SearchPropertiesWidget,
    private Ui::SearchPropertiesText
{
public:
  SearchTextProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : SearchPropertiesWidget (sw)
  {
    setupUi (this);

    text_layer->set_view (view, cv_index);
    text_layer->set_no_layer_available (true);
    text_layer->set_new_layer_enabled (false);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_text_layer, v)) {
      configure_cbx (text_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_string_op, v)) {
      configure_cbx (text_string_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_string_value, v)) {
      text_string_value->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_text_orientation_op, v)) {
      configure_cbx (text_orientation_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_orientation_value, v)) {
      configure_cbx (text_orientation_value, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_size_op, v)) {
      configure_cbx (text_size_op, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_size_value, v)) {
      text_size_value->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_text_layer, tl::to_string (text_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_string_op, tl::to_string (text_string_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_string_value, tl::to_string (text_string_value->text ()));
    config_root->config_set (pfx + cfg_suffix_text_orientation_op, tl::to_string (text_orientation_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_orientation_value, tl::to_string (text_orientation_value->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_size_op, tl::to_string (text_size_op->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_size_value, tl::to_string (text_size_value->text ()));
  }

  std::string search_expression (const std::string &cell_expr) const
  {
    std::string r = "texts";

    add_layer_clause (r, text_layer);

    r += " from ";
    r += cell_expr;

    std::string expr;
    add_numerical_condition (expr, text_size_op, text_size_value, "shape.text_size", "um");
    add_string_condition (expr, text_string_op, text_string_value, "shape.text_string");
    add_orientation_condition (expr, text_orientation_op, text_orientation_value, "shape.text_rot");

    if (! expr.empty ()) {
      r += " where ";
      r += expr;
    }

    return r;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Text"));
  }
};

// ----------------------------------------------------------------------------

class ReplaceInstanceProperties
  : public ReplacePropertiesWidget,
    private Ui::ReplacePropertiesInstance
{
public:
  ReplaceInstanceProperties (QStackedWidget *sw, lay::LayoutViewBase * /*view*/, int /*cv_index*/)
    : ReplacePropertiesWidget (sw)
  {
    setupUi (this);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_instance_cellname_value, v)) {
      instance_cellname->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_instance_cellname_value, tl::to_string (instance_cellname->text ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_cell_index_assignment (expr, instance_cellname, "inst.cell_index");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Instance"));
  }
};

// ----------------------------------------------------------------------------

class ReplaceShapeProperties
  : public ReplacePropertiesWidget,
    protected Ui::ReplacePropertiesShape
{
public:
  ReplaceShapeProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : ReplacePropertiesWidget (sw)
  {
    setupUi (this);

    shape_layer->set_view (view, cv_index);
    shape_layer->set_no_layer_available (true);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_shape_layer, v)) {
      configure_cbx (shape_layer, v);
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_shape_layer, tl::to_string (shape_layer->currentText ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_layer_assignment (expr, shape_layer, "shape.layer");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Shape"));
  }
};

// ----------------------------------------------------------------------------

class ReplacePolygonProperties
  : public ReplaceShapeProperties
{
public:
  ReplacePolygonProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : ReplaceShapeProperties (sw, view, cv_index)
  {
    //  .. nothing yet ..
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_polygon_layer, v)) {
      configure_cbx (shape_layer, v);
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_polygon_layer, tl::to_string (shape_layer->currentText ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_layer_assignment (expr, shape_layer, "shape.layer");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Polygon"));
  }
};

// ----------------------------------------------------------------------------

class ReplaceBoxProperties
  : public ReplacePropertiesWidget,
    private Ui::ReplacePropertiesBox
{
public:
  ReplaceBoxProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : ReplacePropertiesWidget (sw)
  {
    setupUi (this);

    box_layer->set_view (view, cv_index);
    box_layer->set_no_layer_available (true);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_box_layer, v)) {
      configure_cbx (box_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_box_width_value, v)) {
      box_width->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_box_height_value, v)) {
      box_height->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_box_layer, tl::to_string (box_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_box_width_value, tl::to_string (box_width->text ()));
    config_root->config_set (pfx + cfg_suffix_box_height_value, tl::to_string (box_height->text ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_layer_assignment (expr, box_layer, "shape.layer");
    add_numerical_assignment (expr, box_width, "shape.box_width", "um");
    add_numerical_assignment (expr, box_height, "shape.box_height", "um");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Box"));
  }
};

// ----------------------------------------------------------------------------

class ReplacePathProperties
  : public ReplacePropertiesWidget,
    private Ui::ReplacePropertiesPath
{
public:
  ReplacePathProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : ReplacePropertiesWidget (sw)
  {
    setupUi (this);

    path_layer->set_view (view, cv_index);
    path_layer->set_no_layer_available (true);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_path_layer, v)) {
      configure_cbx (path_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_path_width_value, v)) {
      path_width->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_path_layer, tl::to_string (path_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_path_width_value, tl::to_string (path_width->text ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_layer_assignment (expr, path_layer, "shape.layer");
    add_numerical_assignment (expr, path_width, "shape.path_width", "um");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Path"));
  }
};

// ----------------------------------------------------------------------------

class ReplaceTextProperties
  : public ReplacePropertiesWidget,
    private Ui::ReplacePropertiesText
{
public:
  ReplaceTextProperties (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
    : ReplacePropertiesWidget (sw)
  {
    setupUi (this);

    text_layer->set_view (view, cv_index);
    text_layer->set_no_layer_available (true);
  }

  void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    std::string v;
    if (config_root->config_get (pfx + cfg_suffix_text_layer, v)) {
      configure_cbx (text_layer, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_string_value, v)) {
      text_string->setText (tl::to_qstring (v));
    }
    if (config_root->config_get (pfx + cfg_suffix_text_orientation_value, v)) {
      configure_cbx (text_orientation, v);
    }
    if (config_root->config_get (pfx + cfg_suffix_text_size_value, v)) {
      text_size->setText (tl::to_qstring (v));
    }
  }

  void save_state (const std::string &pfx, lay::Dispatcher *config_root) const
  {
    config_root->config_set (pfx + cfg_suffix_text_layer, tl::to_string (text_layer->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_string_value, tl::to_string (text_string->text ()));
    config_root->config_set (pfx + cfg_suffix_text_orientation_value, tl::to_string (text_orientation->currentText ()));
    config_root->config_set (pfx + cfg_suffix_text_size_value, tl::to_string (text_size->text ()));
  }

  std::string replace_expression () const
  {
    std::string expr;
    add_layer_assignment (expr, text_layer, "shape.layer");
    add_numerical_assignment (expr, text_size, "shape.text_size", "um");
    add_string_assignment (expr, text_string, "shape.text_string");
    add_orientation_assignment (expr, text_orientation, "shape.text_rot");
    return expr;
  }

  std::string description () const
  {
    return tl::to_string (QObject::tr ("Text"));
  }
};

// ----------------------------------------------------------------------------

void fill_find_pages (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
{
  while (sw->count () > 0) {
    sw->removeWidget (sw->widget (0));
  }
  sw->addWidget (new SearchInstanceProperties (sw, view, cv_index));
  sw->addWidget (new SearchShapeProperties (sw, view, cv_index));
  sw->addWidget (new SearchBoxProperties (sw, view, cv_index));
  sw->addWidget (new SearchPolygonProperties (sw, view, cv_index));
  sw->addWidget (new SearchPathProperties (sw, view, cv_index));
  sw->addWidget (new SearchTextProperties (sw, view, cv_index));
}

void fill_replace_pages (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index)
{
  while (sw->count () > 0) {
    sw->removeWidget (sw->widget (0));
  }
  sw->addWidget (new ReplaceInstanceProperties (sw, view, cv_index));
  sw->addWidget (new ReplaceShapeProperties (sw, view, cv_index));
  sw->addWidget (new ReplaceBoxProperties (sw, view, cv_index));
  sw->addWidget (new ReplacePolygonProperties (sw, view, cv_index));
  sw->addWidget (new ReplacePathProperties (sw, view, cv_index));
  sw->addWidget (new ReplaceTextProperties (sw, view, cv_index));
}

static const char *object_ids[] = { "instance", "shape", "box", "polygon", "path", "text" };

std::string index_to_find_object_id (int index)
{
  if (index >= 0 && index < int (sizeof (object_ids) / sizeof (object_ids [0]))) {
    return object_ids [index];
  } else {
    return std::string ();
  }
}

int index_from_find_object_id (const std::string &id)
{
  for (int i = 0; i < int (sizeof (object_ids) / sizeof (object_ids [0])); ++i) {
    if (object_ids [i] == id) {
      return i;
    }
  }
  return -1;
}

}

