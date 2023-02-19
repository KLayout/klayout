
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


#include "layLayerProperties.h"
#include "layLayoutViewBase.h"
#include "layConverters.h"
#include "tlXMLParser.h"
#include "tlException.h"
#include "tlExpression.h"
#include "tlAssert.h"
#include "tlInternational.h"

#include <algorithm>

namespace lay
{

// -------------------------------------------------------------
//  LayerProperties implementation

/**
 *  @brief The brightness correction
 *
 *  The brightness is a logarithmic scaling of the rgb values 
 *  towards black (x < 0) or white (x > 0). A brightness correction
 *  of 128 reduces the intensity (in case of correction to black)
 *  by a factor of 2, a correction of 256 by a factor of 4.
 *  All channels are scaled the same way in order not to change the
 *  color but the brightness alone.
 */
tl::color_t
LayerProperties::brighter (tl::color_t in, int x)
{
  //  shortcut for no correction
  if (x == 0) {
    return in;
  }
  
  int r = (in >> 16) & 0xff;
  int g = (in >> 8) & 0xff;
  int b = in & 0xff;

  static double f = log (2.0) / 128.0;

  if (x < 0) {
    x = int (256.0 * exp (f * x) + 0.5);
    r = (x * r) / 256;
    g = (x * g) / 256;
    b = (x * b) / 256;
  } else {
    x = int (256.0 * exp (f * -x) + 0.5);
    r = 255 - (x * (255 - r)) / 256;
    g = 255 - (x * (255 - g)) / 256;
    b = 255 - (x * (255 - b)) / 256;
  }

  return (r << 16) + (g << 8) + b;
}

LayerProperties::LayerProperties ()
  : m_gen_id (0),
    m_frame_color (0),
    m_frame_color_real (0), 
    m_fill_color (0),
    m_fill_color_real (0),
    m_frame_brightness (0),
    m_frame_brightness_real (0),
    m_fill_brightness (0),
    m_fill_brightness_real (0),
    m_dither_pattern (-1), 
    m_dither_pattern_real (-1), 
    m_line_style (-1),
    m_line_style_real (-1),
    m_valid (true),
    m_valid_real (true),
    m_visible (true),
    m_visible_real (true),
    m_transparent (false), 
    m_transparent_real (false), 
    m_width (-1),
    m_width_real (-1),
    m_marked (false),
    m_marked_real (false),
    m_xfill (false),
    m_xfill_real (false),
    m_animation (0),
    m_animation_real (0),
    m_source (),
    m_source_real (),
    m_layer_index (-1),
    m_cellview_index (-1),
    m_inv_prop_set (false),
    m_realize_needed_source (true),
    m_realize_needed_visual (true)
{
  // .. nothing yet ..
}

LayerProperties::LayerProperties (const LayerProperties &d)
  : gsi::ObjectBase (d), 
    m_gen_id (0),
    m_frame_color (0), 
    m_frame_color_real (0), 
    m_fill_color (0),
    m_fill_color_real (0),
    m_frame_brightness (0),
    m_frame_brightness_real (0),
    m_fill_brightness (0),
    m_fill_brightness_real (0),
    m_dither_pattern (-1), 
    m_dither_pattern_real (-1), 
    m_line_style (-1),
    m_line_style_real (-1),
    m_valid (true),
    m_valid_real (true),
    m_visible (true),
    m_visible_real (true),
    m_transparent (false), 
    m_transparent_real (false), 
    m_width (-1),
    m_width_real (-1),
    m_marked (false),
    m_marked_real (false),
    m_xfill (false),
    m_xfill_real (false),
    m_animation (0),
    m_animation_real (0),
    m_source (),
    m_source_real (),
    m_layer_index (-1),
    m_cellview_index (-1),
    m_inv_prop_set (false),
    m_realize_needed_source (true),
    m_realize_needed_visual (true)
{
  operator= (d);
}

LayerProperties &
LayerProperties::operator= (const LayerProperties &d)
{
  if (&d != this) {

    refresh ();
    d.ensure_realized ();

    int flags = 0;

    if (m_frame_color != d.m_frame_color ||
        m_fill_color != d.m_fill_color ||
        m_frame_brightness != d.m_frame_brightness ||
        m_fill_brightness != d.m_fill_brightness ||
        m_dither_pattern != d.m_dither_pattern ||
        m_line_style != d.m_line_style ||
        m_valid != d.m_valid ||
        m_visible != d.m_visible ||
        m_transparent != d.m_transparent ||
        m_width != d.m_width ||
        m_marked != d.m_marked ||
        m_xfill != d.m_xfill ||
        m_animation != d.m_animation) {
      m_frame_color = d.m_frame_color;
      m_fill_color = d.m_fill_color;
      m_frame_brightness = d.m_frame_brightness;
      m_fill_brightness = d.m_fill_brightness;
      m_dither_pattern = d.m_dither_pattern;
      m_line_style = d.m_line_style;
      m_valid = d.m_valid;
      m_visible = d.m_visible;
      m_transparent = d.m_transparent;
      m_width = d.m_width;
      m_marked = d.m_marked;
      m_xfill = d.m_xfill;
      m_animation = d.m_animation;
      flags += nr_visual;
    }

    if (m_source != d.m_source) {
      m_source = d.m_source;
      flags += nr_source;
    }

    if (m_name != d.m_name) {
      m_name = d.m_name;
      flags += nr_meta;
    }

    if (flags) {
      need_realize (flags, true /*force on children*/);
    }

  }
  return *this;
}

LayerProperties::~LayerProperties ()
{
  //  .. nothing yet ..
}

bool 
LayerProperties::operator== (const LayerProperties &d) const
{
  ensure_realized ();
  d.ensure_realized ();

  //  do not consider the derived and "real" properties  - these is not really a property
  return m_frame_color == d.m_frame_color && 
         m_fill_color == d.m_fill_color && 
         m_frame_brightness == d.m_frame_brightness &&
         m_fill_brightness == d.m_fill_brightness &&
         m_dither_pattern == d.m_dither_pattern && 
         m_line_style == d.m_line_style &&
         m_valid == d.m_valid &&
         m_visible == d.m_visible && 
         m_transparent == d.m_transparent && 
         m_width == d.m_width &&
         m_marked == d.m_marked &&
         m_xfill == d.m_xfill &&
         m_animation == d.m_animation &&
         m_name == d.m_name && 
         m_source == d.m_source;
}

bool  
LayerProperties::is_visual () const
{
  return valid (true) && visible (true) && (layer_index () >= 0 || is_cell_box_layer ());
}

tl::color_t
LayerProperties::eff_frame_color (bool real) const
{
  return brighter (frame_color (real) & 0xffffff, frame_brightness (real));
}

tl::color_t
LayerProperties::eff_fill_color (bool real) const
{
  return brighter (fill_color (real) & 0xffffff, fill_brightness (real));
}

tl::color_t
LayerProperties::eff_frame_color_brighter (bool real, int plus_brightness) const
{
  return brighter (frame_color (real) & 0xffffff, frame_brightness (real) + plus_brightness);
}

tl::color_t
LayerProperties::eff_fill_color_brighter (bool real, int plus_brightness) const
{
  return brighter (fill_color (real) & 0xffffff, fill_brightness (real) + plus_brightness);
}

void
LayerProperties::merge_visual (const LayerProperties *d) const
{
  if (!d || !d->has_frame_color (true)) {
    m_frame_color_real = m_frame_color;
  } else {
    m_frame_color_real = d->m_frame_color_real;
  }
  if (!d || !d->has_fill_color (true)) {
    m_fill_color_real = m_fill_color;
  } else {
    m_fill_color_real = d->m_fill_color_real;
  }

  m_frame_brightness_real = m_frame_brightness;
  if (d) {
    m_frame_brightness_real += d->m_frame_brightness_real;
  }
  m_fill_brightness_real = m_fill_brightness;
  if (d) {
    m_fill_brightness_real += d->m_fill_brightness_real;
  }

  if (!d || !d->has_dither_pattern (true)) {
    m_dither_pattern_real = m_dither_pattern;
  } else {
    m_dither_pattern_real = d->m_dither_pattern_real;
  }

  if (!d || !d->has_line_style (true)) {
    m_line_style_real = m_line_style;
  } else {
    m_line_style_real = d->m_line_style_real;
  }

  m_valid_real = m_valid && (!d || d->m_valid_real);
  m_visible_real = m_visible && (!d || d->m_visible_real);
  m_xfill_real = m_xfill || (d && d->m_xfill_real);
  m_transparent_real = m_transparent || (d && d->m_transparent_real);
  m_marked_real = m_marked || (d && d->m_marked_real);

  m_width_real = m_width;
  if (d && d->m_width_real > m_width) {
    m_width_real = d->m_width_real;
  }

  m_animation_real = m_animation;
  if (d && m_animation_real == 0) {
    m_animation_real = d->m_animation_real;
  }
}

void
LayerProperties::merge_source (const LayerProperties *d) const
{
  m_source_real = m_source;
  if (d) {
    m_source_real += d->m_source_real;
  }
}

void
LayerProperties::ensure_realized () const
{
  refresh ();
  if (m_realize_needed_source) {
    realize_source ();
    m_realize_needed_source = false;
  }
  if (m_realize_needed_visual) {
    realize_visual ();
    m_realize_needed_visual = false;
  }
}

void
LayerProperties::ensure_source_realized () const
{
  refresh ();
  if (m_realize_needed_source) {
    realize_source ();
    m_realize_needed_source = false;
  }
}

void
LayerProperties::ensure_visual_realized () const
{
  refresh ();
  if (m_realize_needed_visual) {
    realize_visual ();
    m_realize_needed_visual = false;
  }
}

LayerProperties
LayerProperties::flat () const
{
  ensure_realized ();

  //  initialize the result object with the "real" properties
  LayerProperties r;
  r.m_frame_color = r.m_frame_color_real = m_frame_color_real;
  r.m_fill_color = r.m_fill_color_real = m_fill_color_real;
  r.m_frame_brightness = r.m_frame_brightness_real = m_frame_brightness_real;
  r.m_fill_brightness = r.m_fill_brightness_real = m_fill_brightness_real;
  r.m_dither_pattern = r.m_dither_pattern_real = m_dither_pattern_real;
  r.m_line_style = r.m_line_style_real = m_line_style_real;
  r.m_valid = r.m_valid_real = m_valid_real;
  r.m_visible = r.m_visible_real = m_visible_real;
  r.m_transparent = r.m_transparent_real = m_transparent_real;
  r.m_width = r.m_width_real = m_width_real;
  r.m_marked = r.m_marked_real = m_marked_real;
  r.m_xfill = r.m_xfill_real = m_xfill_real;
  r.m_animation = r.m_animation_real = m_animation_real;
  r.m_name = m_name;
  r.m_source = r.m_source_real = m_source_real;
  r.m_layer_index = m_layer_index;
  r.m_cellview_index = m_cellview_index;
  r.m_trans = m_trans;
  r.m_hier_levels = m_hier_levels;
  r.m_prop_set = m_prop_set;
  r.m_inv_prop_set = m_inv_prop_set;
  r.m_realize_needed_source = r.m_realize_needed_visual = false;

  return r;
}

class LayerSourceEval 
  : public tl::Eval
{
public:
  LayerSourceEval (const lay::LayerProperties &lp, const lay::LayoutViewBase *view, bool real)
    : m_lp (lp), mp_view (view), m_real (real)
  { 
    // .. nothing yet ..
  }

  const lay::ParsedLayerSource &source () const
  {
    return m_lp.source (m_real);
  }

  const lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

private:
  const lay::LayerProperties &m_lp;
  const lay::LayoutViewBase *mp_view;
  bool m_real;
};

class LayerSourceEvalFunction
  : public tl::EvalFunction
{
public:
  LayerSourceEvalFunction (char function, const LayerSourceEval *eval)
    : m_function (function), mp_eval (eval)
  {
    // .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv) const
  {
    if (vv.size () != 0) {
      throw tl::EvalError (tl::to_string (tr ("Layer source function must not have arguments")), context);
    }

    out = tl::Variant ();

    if (m_function == 'N') {
      if (mp_eval->source ().has_name ()) {
        out = mp_eval->source ().name ();
      }
    } else if (m_function == 'L') {
      if (mp_eval->source ().layer () >= 0) {
        out = mp_eval->source ().layer ();
      }
    } else if (m_function == 'D') {
      if (mp_eval->source ().datatype () >= 0) {
        out = mp_eval->source ().datatype ();
      }
    } else if (m_function == 'I') {
      if (mp_eval->source ().layer_index () >= 0) {
        out = mp_eval->source ().layer_index ();
      }
    } else if (m_function == 'C') {
      if (mp_eval->source ().cv_index () >= 0) {
        out = mp_eval->source ().cv_index ();
      }
    } else if (m_function == 'S') {
      out = mp_eval->source ().display_string (mp_eval->view ());
    } else if (m_function == 'T') {
      const lay::CellView &cv = mp_eval->view ()->cellview (mp_eval->source ().cv_index ());
      if (cv.is_valid ()) {
        out = cv->name ();
      }
    }
  }

private:
  char m_function;
  const LayerSourceEval *mp_eval;
};

std::string
LayerProperties::display_string (const lay::LayoutViewBase *view, bool real, bool always_show_source) const
{
  refresh ();

  try {

    std::string ret;

    if (! m_name.empty ()) {

      if (m_name.find ("$") == std::string::npos) {
        ret = m_name;
      } else {

        if (m_realize_needed_source) {
          realize_source ();
        }

        LayerSourceEval eval (*this, view, real);
        eval.define_function ("N", new LayerSourceEvalFunction ('N', &eval)); // layer name
        eval.define_function ("L", new LayerSourceEvalFunction ('L', &eval)); // layer number
        eval.define_function ("D", new LayerSourceEvalFunction ('D', &eval)); // datatype
        eval.define_function ("I", new LayerSourceEvalFunction ('I', &eval)); // layer index
        eval.define_function ("C", new LayerSourceEvalFunction ('C', &eval)); // cv index
        eval.define_function ("S", new LayerSourceEvalFunction ('S', &eval)); // layer source
        eval.define_function ("T", new LayerSourceEvalFunction ('T', &eval)); // title

        ret = eval.interpolate (m_name);

      }

      if (always_show_source || view->always_show_source ()) {
        ret += " - ";
        ret += source (real).display_string (view);
      }

    } else {
      ret = source (real).display_string (view);
    }

    return ret;

  } catch (tl::Exception &ex) {
    return ex.msg ();
  }
}

void
LayerProperties::set_xfill (bool xf)
{
  refresh ();
  if (xf != m_xfill) {
    m_xfill = xf;
    need_realize (nr_visual);
  }
}

void
LayerProperties::set_source (const std::string &s)
{
  set_source (lay::ParsedLayerSource (s));
}

void 
LayerProperties::set_source (const lay::ParsedLayerSource &s)
{
  refresh ();
  if (m_source != s) {
    m_source = s;
    need_realize (nr_source);
  }
}

void
LayerProperties::realize_visual () const
{
  //  do as much as we can. The node implementation will provide a 
  //  parent and a view if possible.
  merge_visual (0);
}

void
LayerProperties::realize_source () const
{
  //  do as much as we can. The node implementation will provide a 
  //  parent and a view if possible.
  merge_source (0);
  do_realize (0);
}

void
LayerProperties::touch ()
{
  if (++m_gen_id == 0) {
    ++m_gen_id;
  }
}

void
LayerProperties::need_realize (unsigned int flags, bool /*force*/)
{
  touch ();

  if ((flags & nr_source) != 0) {
    m_realize_needed_source = true;
  }
  if ((flags & nr_visual) != 0) {
    m_realize_needed_visual = true;
  }
}

void
LayerProperties::expanded_state_changed ()
{
  //  .. no effect ..
}

void 
LayerProperties::do_realize (const LayoutViewBase *view) const
{
  m_layer_index = -1;
  m_cellview_index = -1;
  m_trans.clear ();
  m_inv_prop_set = true;
  m_prop_set.clear ();
  m_hier_levels = m_source_real.hier_levels ();

  if (view) {

    if (m_source_real.cv_index () < 0) {
      if (view->cellviews () > 0) {
        m_cellview_index = 0;
      }
    } else if (m_source_real.cv_index () < int (view->cellviews ())) {
      m_cellview_index = m_source_real.cv_index ();
    } 

    if (m_cellview_index >= 0) {

      const lay::CellView &cv = view->cellview (m_cellview_index);

      //  retrieve the property selector, if one is present
      if (! m_source_real.property_selector ().is_null ()) {
        m_inv_prop_set = m_source_real.property_selector ().matching (cv->layout ().properties_repository (), m_prop_set);
      }

      //  compute the effective transformation in database units
      m_trans = m_source_real.trans ();

      if (m_source_real.special_purpose () == ParsedLayerSource::SP_None) {

        m_layer_index = m_source_real.layer_index ();

        //  lookup the layer with the given name/layer/datatype
        if (m_layer_index < 0 && ! m_source_real.is_wildcard_layer ()) {
          m_layer_index = cv->layout ().get_layer_maybe (m_source_real.layer_props ());
        }

      }

    }

  }

  if (m_trans.empty ()) {
    m_trans.push_back (db::DCplxTrans ());
  }
}


// -------------------------------------------------------------
//  LayerPropertiesNode implementation

static unsigned int s_unique_id = 0;

LayerPropertiesNode::LayerPropertiesNode ()
  : LayerProperties (),
    m_list_index (0), m_expanded (false)
{
  m_id = ++s_unique_id;
}

LayerPropertiesNode::~LayerPropertiesNode ()
{
  //  .. nothing yet ..
}

LayerPropertiesNode::LayerPropertiesNode (const LayerProperties &d)
  : LayerProperties (d),
    m_list_index (0), m_expanded (false)
{
  m_id = ++s_unique_id;
}

LayerPropertiesNode::LayerPropertiesNode (const LayerPropertiesNode &d)
  : LayerProperties (d), tl::Object (),
    m_list_index (0),
    m_expanded (d.m_expanded),
    m_children (d.m_children),
    m_id (d.m_id)
{
  for (iterator c = m_children.begin (); c != m_children.end (); ++c) {
    c->set_parent (this);
  }
}

LayerPropertiesNode &
LayerPropertiesNode::operator= (const LayerPropertiesNode &d)
{
  if (&d != this) {

    LayerProperties::operator= (d);

    m_children = d.m_children;
    m_expanded = d.m_expanded;
    m_id = d.m_id;

    for (iterator c = m_children.begin (); c != m_children.end (); ++c) {
      c->set_parent (this);
    }

    need_realize (nr_hierarchy, true);

  }
  return *this;
}

bool 
LayerPropertiesNode::operator== (const LayerPropertiesNode &d) const
{
  if (! LayerProperties::operator== (d)) {
    return false;
  }
  return m_children == d.m_children && m_expanded == d.m_expanded;
}

LayoutViewBase *LayerPropertiesNode::view() const
{
  return const_cast<lay::LayoutViewBase *> (mp_view.get ());
}

void
LayerPropertiesNode::set_expanded (bool ex)
{
  if (expanded () != ex) {
    m_expanded = ex;
    expanded_state_changed ();
  }
}

unsigned int
LayerPropertiesNode::list_index () const
{
  return m_list_index;
}

void 
LayerPropertiesNode::realize_visual () const
{
  //  make sure the parents are realized
  if (mp_parent && mp_parent->realize_needed_visual ()) {
    mp_parent->realize_visual ();
  }
  merge_visual (mp_parent.get ());
}

void 
LayerPropertiesNode::realize_source () const
{
  //  make sure the parents are realized
  if (mp_parent && mp_parent->realize_needed_source ()) {
    mp_parent->realize_source ();
  }
  merge_source (mp_parent.get ());
  do_realize (mp_view.get ());
}

void
LayerPropertiesNode::expanded_state_changed ()
{
  touch ();
}

void
LayerPropertiesNode::need_realize (unsigned int flags, bool force)
{
  LayerProperties::need_realize (flags);

  if ((flags & (nr_visual + nr_source)) != 0 && (force || ! realize_needed_visual () || ! realize_needed_source ())) {
    for (iterator c = m_children.begin (); c != m_children.end (); ++c) {
      c->need_realize (flags, force);
    }
  }

  //  Propagate the status change to the parents on hierarchy change.
  //  This is important to make any references to parent nodes aware of this
  //  change.
  LayerPropertiesNode *p = const_cast<LayerPropertiesNode *> (parent ());
  while (p) {
    p->touch ();
    p = const_cast<LayerPropertiesNode *> (p->parent ());
  }
}

void
LayerPropertiesNode::set_parent (const LayerPropertiesNode *parent)
{
  mp_parent.reset (const_cast<LayerPropertiesNode *>(parent));
}

db::DBox 
LayerPropertiesNode::bbox () const
{
  tl_assert (mp_view);
  lay::CellView cv = mp_view->cellview (cellview_index ());
  
  if (! cv.is_valid ()) {

    return db::DBox ();

  } else if (is_cell_box_layer ()) {

    db::DBox b;
    double dbu = cv->layout ().dbu ();
    for (std::vector<db::DCplxTrans>::const_iterator t = trans ().begin (); t != trans ().end (); ++t) {
      b += (*t * db::CplxTrans (dbu) * cv.context_trans ()) * cv.cell ()->bbox ();
    }
    return b;

  } else {

    db::DBox b;
    double dbu = cv->layout ().dbu ();
    for (std::vector<db::DCplxTrans>::const_iterator t = trans ().begin (); t != trans ().end (); ++t) {
      b += (*t * db::CplxTrans (dbu) * cv.context_trans ()) * cv.cell ()->bbox (layer_index ());
    }
    return b;

  }
}

void 
LayerPropertiesNode::attach_view (LayoutViewBase *view, unsigned int list_index)
{
  mp_view.reset (view);
  m_list_index = list_index;

  for (iterator c = m_children.begin (); c != m_children.end (); ++c) {
    c->attach_view (view, list_index);
  }
  //  Attachment of a view is a strong indication that something significant changed - 
  //  recompute the source specifications on next request.
  m_realize_needed_source = true;
}

LayerPropertiesNode &
LayerPropertiesNode::insert_child (const iterator &iter, const LayerPropertiesNode &child)
{
  refresh ();
  iterator i = m_children.insert (iter, child);
  i->set_parent (this);
  need_realize (nr_hierarchy, true);
  return *i;
}

void 
LayerPropertiesNode::erase_child (const iterator &iter)
{
  refresh ();
  m_children.erase (iter);
  need_realize (nr_hierarchy, true);
}

void 
LayerPropertiesNode::add_child (const LayerPropertiesNode &child)
{
  refresh ();
  m_children.push_back (child);
  m_children.back ().set_parent (this);
  need_realize (nr_hierarchy, true);
}

// -------------------------------------------------------------
//  LayerPropertiesConstIterator implementation

LayerPropertiesConstIterator::LayerPropertiesConstIterator ()
  : m_uint (0), m_list ()
{
  //  .. nothing yet ..
}

LayerPropertiesConstIterator::LayerPropertiesConstIterator (const lay::LayerPropertiesNode *node)
  : m_uint (0), m_list ()
{
  if (!node) {
    return;
  }

  //  determine the position of the layer properties in the hierarchy of nodes

  std::vector<size_t> child_indexes;

  while (node->parent ()) {
    size_t index = 0;
    bool found = false;
    for (lay::LayerPropertiesNode::const_iterator c = node->parent ()->begin_children (); c != node->parent ()->end_children (); ++c, ++index) {
      if (&*c == node) {
        found = true;
        break;
      }
    }
    if (!found) {
      return;
    }
    child_indexes.push_back (index);
    node = node->parent ();
  }

  if (!node->view ()) {
    return;
  }

  {
    const lay::LayerPropertiesList &list = node->view ()->get_properties (node->list_index ());
    size_t index = 0;
    bool found = false;
    for (lay::LayerPropertiesList::const_iterator c = list.begin_const (); c != list.end_const (); ++c, ++index) {
      if (&*c == node) {
        found = true;
        break;
      }
    }
    if (!found) {
      return;
    }
    child_indexes.push_back (index);
  }

  //  unfold the final iterator by recursing down the hierarchy path

  lay::LayerPropertiesConstIterator iter = node->view()->begin_layers ();
  while (!child_indexes.empty () && !iter.at_end () && !iter.is_null ()) {
    iter.to_sibling (child_indexes.back ());
    child_indexes.pop_back ();
    if (! child_indexes.empty ()) {
      iter = iter.first_child ();
    }
  }

  *this = iter;
}

LayerPropertiesConstIterator::LayerPropertiesConstIterator (const LayerPropertiesList &list, bool last)
    //  NOTE: there should be some "const_weak_ptr"
  : m_uint (0), m_list (const_cast<LayerPropertiesList *> (&list))
{
  if (last) {
    m_uint = (list.end_const () - list.begin_const ()) + 1;
  } else {
    m_uint = 1;
  }
}

LayerPropertiesConstIterator::LayerPropertiesConstIterator (const LayerPropertiesList &list, size_t uint)
    //  NOTE: there should be some "const_weak_ptr"
  : m_uint (uint), m_list (const_cast<LayerPropertiesList *> (&list))
{
  //  .. nothing yet ..
}

LayerPropertiesConstIterator::LayerPropertiesConstIterator (const LayerPropertiesConstIterator &d)
  : tl::Object (), m_uint (d.m_uint), m_list (d.m_list), mp_obj (d.mp_obj)
{
  //  .. nothing yet ..
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::operator= (const LayerPropertiesConstIterator &d)
{
  if (this != &d) {
    m_uint = d.m_uint;
    m_list = d.m_list;
    mp_obj = d.mp_obj;
  }
  return *this;
}

bool 
LayerPropertiesConstIterator::operator< (const LayerPropertiesConstIterator &d) const
{
  tl_assert (m_list);
  tl_assert (m_list == d.m_list);

  size_t uint = m_uint;
  size_t duint = d.m_uint;
  if (uint == duint) {
    return false;
  }

  if (!m_list) {
    return false;
  }

  LayerPropertiesList::const_iterator iter = m_list->begin_const ();
  size_t n = ((m_list->end_const () - m_list->begin_const ()) + 2);

  while (true) {
    size_t rem = uint % n;
    size_t drem = duint % n;
    if (rem != drem) {
      return rem < drem;
    }
    uint /= n;
    duint /= n;
    if (uint == 0 || duint == 0) {
      return uint < duint;
    }
    n = ((iter[rem - 1].end_children () - iter[rem - 1].begin_children ()) + 2);
    iter = iter[rem - 1].begin_children ();
  }
}

std::pair<size_t, size_t>
LayerPropertiesConstIterator::factor () const
{
  tl_assert (m_list);
  
  //  with this definition, the 0 iterator can act as the "root"
  if (m_uint == 0) {
    return std::make_pair (size_t (1), size_t (1));
  }

  LayerPropertiesList::const_iterator iter = m_list->begin_const ();
  size_t uint = m_uint;
  size_t n = ((m_list->end_const () - m_list->begin_const ()) + 2);
  size_t f = 1;

  while (uint > n) {
    size_t rem = uint % n;
    uint /= n;
    f *= n;
    tl_assert (rem < n - 1 && rem > 0);
    n = ((iter[rem - 1].end_children () - iter[rem - 1].begin_children ()) + 2);
    iter = iter[rem - 1].begin_children ();
  }

  return std::make_pair (f, n);
}

bool 
LayerPropertiesConstIterator::at_end () const
{
  if (! m_list) {
    return true;
  } else {
    std::pair <size_t, size_t> f = factor ();
    return (m_uint / f.first == f.second - 1);
  }
}

bool 
LayerPropertiesConstIterator::at_top () const
{
  tl_assert (m_list);
  return m_uint < size_t ((m_list->end_const () - m_list->begin_const ()) + 2);
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::up ()
{
  m_uint %= factor ().first;
  mp_obj.reset (0);
  return *this;
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::next_sibling (ptrdiff_t n)
{
  m_uint += factor ().first * n;
  mp_obj.reset (0);
  return *this;
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::to_sibling (size_t n)
{
  std::pair <size_t, size_t> f = factor ();
  m_uint = (m_uint % f.first) + (1 + n) * f.first;
  mp_obj.reset (0);
  return *this;
}

size_t 
LayerPropertiesConstIterator::num_siblings () const
{
  std::pair <size_t, size_t> f = factor ();
  return f.second - 2;
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::down_first_child ()
{
  std::pair <size_t, size_t> f = factor ();
  m_uint += f.first * f.second;
  mp_obj.reset (0);
  return *this;
}

LayerPropertiesConstIterator &
LayerPropertiesConstIterator::down_last_child ()
{
  std::pair <size_t, size_t> f = factor ();
  const LayerPropertiesNode *o = obj ();
  m_uint += f.first * f.second * ((o->end_children () - o->begin_children ()) + 1);
  mp_obj.reset (0);
  return *this;
}

std::pair <const LayerPropertiesNode *, size_t> 
LayerPropertiesConstIterator::parent_obj () const
{
  tl_assert (m_list);

  size_t uint = m_uint;
  LayerPropertiesList::const_iterator iter = m_list->begin_const ();
  size_t n = ((m_list->end_const () - m_list->begin_const ()) + 2);
  const LayerPropertiesNode *ret = 0;

  while (uint > n) {
    size_t rem = uint % n;
    tl_assert (rem > 0);
    tl_assert (rem < n - 1);
    ret = &iter[rem - 1];
    uint /= n;
    n = ((ret->end_children () - ret->begin_children ()) + 2);
    iter = ret->begin_children ();
  }

  tl_assert (uint > 0);
  return std::make_pair (ret, uint - 1);
}

void
LayerPropertiesConstIterator::invalidate () 
{
  mp_obj.reset (0);

  //  the iterator may be parked at a position behind the last element.
  //  Move one step further in this case.
  std::pair <size_t, size_t> f = factor ();
  if (m_uint / f.first >= f.second - 1 && !at_top ()) {
    up ();
    inc (1);
  }
}

void
LayerPropertiesConstIterator::set_obj () const
{
  if (is_null () || !m_list) {

    mp_obj.reset (0);

  } else {

    tl_assert (m_list);

    size_t uint = m_uint;
    LayerPropertiesList::const_iterator iter = m_list->begin_const ();
    size_t n = ((m_list->end_const () - m_list->begin_const ()) + 2);

    while (uint > n) {
      size_t rem = uint % n;
      tl_assert (rem > 0);
      tl_assert (rem < n - 1);
      uint /= n;
      n = ((iter[rem - 1].end_children () - iter[rem - 1].begin_children ()) + 2);
      iter = iter[rem - 1].begin_children ();
    }

    mp_obj.reset (const_cast<lay::LayerPropertiesNode *> (&iter[uint - 1]));

  }
}

void 
LayerPropertiesConstIterator::inc (unsigned int d)
{
  if (d == 0) {
    return;
  } else if (d == 1) {

    if (obj ()->has_children ()) {
      down_first_child ();
    } else {
      while (true) {
        std::pair <size_t, size_t> f = factor ();
        m_uint += f.first;
        mp_obj.reset (0);
        if (m_uint / f.first < f.second - 1) {
          break;
        } else if (at_top ()) {
          break;
        } else {
          up ();
        }
      }
    }

  } else {
    //  :KLUDGE: this is pretty slow ..
    while (d-- > 0) {
      inc (1);
    }
  }
}

size_t 
LayerPropertiesConstIterator::child_index () const
{
  std::pair <size_t, size_t> f = factor ();
  return ((m_uint / f.first) % f.second) - 1;
}

// -------------------------------------------------------------
//  LayerPropertiesList implementation

LayerPropertiesList::LayerPropertiesList ()
  : tl::Object (), m_list_index (0)
{
  //  .. nothing yet ..  
}

LayerPropertiesList::~LayerPropertiesList ()
{
  //  .. nothing yet ..  
}

LayerPropertiesList::LayerPropertiesList (const LayerPropertiesList &d)
  : tl::Object (), m_list_index (0)
{
  operator= (d);
}

LayerPropertiesList &
LayerPropertiesList::operator= (const LayerPropertiesList &d)
{
  if (&d != this) {
    m_layer_properties = d.m_layer_properties;
    m_dither_pattern = d.m_dither_pattern;
    m_line_styles = d.m_line_styles;
    m_name = d.m_name;
  }
  return *this;
}

bool 
LayerPropertiesList::operator== (const LayerPropertiesList &d) const
{
  if (m_dither_pattern != d.m_dither_pattern) {
    return false;
  }
  if (m_line_styles != d.m_line_styles) {
    return false;
  }
  return m_layer_properties == d.m_layer_properties;
}

void 
LayerPropertiesList::translate_cv_references (int cv_index)
{
  for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {
    if (l->source (false).cv_index () >= 0) {
      ParsedLayerSource new_source (l->source (false));
      new_source.cv_index (cv_index);
      l->set_source (new_source);
    }
  }
}

static bool has_cv_ref (const LayerPropertiesNode &node, int cv_ref)
{
  if (! node.has_children ()) {

    return node.source (true).cv_index () == cv_ref && (node.is_cell_box_layer () || node.is_standard_layer ());

  } else {

    for (LayerPropertiesNode::const_iterator c = node.begin_children (); c != node.end_children (); ++c) {
      if (! has_cv_ref (*c, cv_ref)) {
        return false;
      }
    }

    return true;

  }
}

void 
LayerPropertiesList::remove_cv_references (int cv_index, bool except)
{
  std::vector<LayerPropertiesIterator> cv_ref;

  for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {
    if (has_cv_ref (*l, cv_index) != except) {
      cv_ref.push_back (l);
    }
  }

  std::sort (cv_ref.begin (), cv_ref.end (), CompareLayerIteratorBottomUp ());

  for (std::vector<LayerPropertiesIterator>::const_iterator ll = cv_ref.begin (); ll != cv_ref.end (); ++ll) {
    erase (*ll);
  }
}

static bool has_wildcard_layout (const LayerPropertiesNode &node, bool any)
{
  if (! node.has_children ()) {

    return node.source (true).cv_index () < 0 && (node.is_cell_box_layer () || node.is_standard_layer ());

  } else if (any) {
  
    for (LayerPropertiesNode::const_iterator c = node.begin_children (); c != node.end_children (); ++c) {
      if (has_wildcard_layout (*c, true)) {
        return true;
      }
    }

    return false;

  } else {

    for (LayerPropertiesNode::const_iterator c = node.begin_children (); c != node.end_children (); ++c) {
      if (! has_wildcard_layout (*c, false)) {
        return false;
      }
    }

    return true;

  }
}

static LayerPropertiesNode expand_wildcard_layout (const LayerPropertiesNode &source, int new_cv_index)
{
  //  this creates the node, not the children:
  LayerPropertiesNode new_node ((const LayerProperties &) source);

  if (! source.has_children ()) {

    //  a wildcard layout: need to replace the cv index:
    ParsedLayerSource new_source (new_node.source (false));
    new_source.cv_index (new_cv_index);
    new_node.set_source (new_source);

  } else {

    for (LayerPropertiesNode::const_iterator l = source.begin_children (); l != source.end_children (); ++l) {
      if (has_wildcard_layout (*l, true /*any*/)) {
        //  need to add a new child node
        new_node.add_child (expand_wildcard_layout (*l, new_cv_index));
      }
    }

  }

  return new_node;
}

static std::vector<LayerPropertiesNode> 
expand_wildcard_layers (const LayerPropertiesNode &lp, const LayerPropertiesList &current_props, lay::LayoutViewBase *view, unsigned int list_index)
{
  std::vector<LayerPropertiesNode> new_props;

  int cv_index = lp.source (true).cv_index ();
  if (cv_index >= 0 && cv_index < int (view->cellviews ())) {

    //  determine the layers not assigned so far.
    //  NOTE: we use lay::ParsedLayerSource, but in a normalized form that does not 
    //  include transformations and such. Hence we really figure out which layer is 
    //  missing or not.

    std::set <lay::ParsedLayerSource> present;
    for (LayerPropertiesConstIterator l = current_props.begin_const_recursive (); ! l.at_end (); ++l) {
      if (! l->has_children ()) {
        lay::ParsedLayerSource src = l->source (true /*real*/);
        if (src.cv_index () == cv_index) {
          present.insert (lay::ParsedLayerSource (src.layer_props (), cv_index));
        }
      }
    }

    std::vector <lay::ParsedLayerSource> actual;
    const db::Layout &layout = view->cellview (cv_index)->layout ();
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        actual.push_back (lay::ParsedLayerSource (layout.get_properties (l), cv_index));
      }
    }

    std::sort (actual.begin (), actual.end ());

    for (std::vector <lay::ParsedLayerSource>::const_iterator a = actual.begin (); a != actual.end (); ++a) {

      if (present.find (*a) == present.end ()) {

        //  NOTE: initialization through LayerProperties creates a new ID
        lay::LayerPropertiesNode node ((const LayerProperties &) lp);
        node.attach_view (view, list_index);

        //  Build a new ParsedLayerSource combining the transformation, hierarchy levels and 
        //  property selections from the wildcard one and the requested layer source
        lay::ParsedLayerSource src (*a);
        src += lp.source (true /*real*/);
        node.set_source (src);

        new_props.push_back (node);

      }

    }

  }

  return new_props;
}

void 
LayerPropertiesList::append (const LayerPropertiesList &other)
{
  {
    lay::DitherPattern dp (other.dither_pattern ());

    std::map <unsigned int, unsigned int> index_map;
    dp.merge (dither_pattern (), index_map);

    //  remap the dither pattern index
    for (lay::LayerPropertiesIterator l = begin_recursive (); l != end_recursive (); ++l) {
      int dpi = l->dither_pattern (false /*local*/);
      std::map <unsigned int, unsigned int>::iterator m = index_map.find ((unsigned int) dpi);
      if (m != index_map.end ()) {
        l->set_dither_pattern (int (m->second));
      }
    }

    set_dither_pattern (dp);
  }

  {
    lay::LineStyles ls (other.line_styles ());

    std::map <unsigned int, unsigned int> index_map;
    ls.merge (line_styles (), index_map);

    //  remap the line style index
    for (lay::LayerPropertiesIterator l = begin_recursive (); l != end_recursive (); ++l) {
      int lsi = l->line_style (false /*local*/);
      std::map <unsigned int, unsigned int>::iterator m = index_map.find ((unsigned int) lsi);
      if (m != index_map.end ()) {
        l->set_line_style (int (m->second));
      }
    }

    set_line_styles (ls);
  }

  for (lay::LayerPropertiesList::const_iterator l = other.begin_const (); l != other.end_const (); ++l) {
    push_back (*l);
  }
}

void
LayerPropertiesList::expand (const std::map<int, int> &map_cv_index, bool add_default)
{
  tl_assert (view () != 0);

  //  Add a default element if required unless there already is one at top level.
  //  If there already is one, this one will be ignored.
  if (add_default) {
    push_back (LayerPropertiesNode ());
  }

  //  Apply cv mapping 
  if (! map_cv_index.empty ()) {

    std::set<int> cvrefs_to_erase;

    for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {

      ParsedLayerSource new_source (l->source (false));

      std::map<int, int>::const_iterator m = map_cv_index.end ();
      if (new_source.cv_index () >= 0) {
        m = map_cv_index.find (new_source.cv_index ());
      }
      if (m == map_cv_index.end () && !l->has_children ()) {
        m = map_cv_index.find (-1);
      }

      if (m != map_cv_index.end ()) {
        if (m->second == -2) {
          //  mapping to -2 means: remove
          cvrefs_to_erase.insert (new_source.cv_index ());
        } else {
          new_source.cv_index (m->second);
          l->set_source (new_source);
        }
      }

    }

    //  erase the items specified to removal (cv mapping to -1)
    for (std::set<int>::const_iterator cv = cvrefs_to_erase.begin (); cv != cvrefs_to_erase.end (); ++cv) {
      remove_cv_references (*cv, false);
    }

  }

  //  Test if any layer has a wildcard layout spec
  bool lywc = false;
  for (const_iterator l = begin (); l != end () && !lywc; ++l) {
    lywc = has_wildcard_layout (*l, true /*any*/);
  }

  if (lywc) {

    //  If that is the case, iterate over all layouts (outer iteration) and create the 
    //  wildcarded 

    std::vector <lay::LayerPropertiesNode> new_nodes;

    for (unsigned int cv_index = 0; cv_index < view ()->cellviews (); ++cv_index) {
      for (const_iterator l = begin (); l != end (); ++l) {
        if (has_wildcard_layout (*l, true /*any*/)) {
          new_nodes.push_back (expand_wildcard_layout (*l, cv_index));
        }
      }
    }

    for (std::vector <lay::LayerPropertiesNode>::const_iterator n = new_nodes.begin (); n != new_nodes.end (); ++n) {
      push_back (*n);
      back ().attach_view (view (), list_index ());
    }

    //  Remove all layers which have been expanded and their parents
    //  if necessary

    std::vector<LayerPropertiesIterator> expanded;

    for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {
      if (has_wildcard_layout (*l, false /*all*/)) {
        expanded.push_back (l);
      }
    }

    std::sort (expanded.begin (), expanded.end (), CompareLayerIteratorBottomUp ());

    for (std::vector<LayerPropertiesIterator>::const_iterator ll = expanded.begin (); ll != expanded.end (); ++ll) {
      erase (*ll);
    }

  }

  //  Expand layer wildcard layers

  std::vector<LayerPropertiesIterator> lwc;

  //  find the wildcard layers
  for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {
    if (! l->has_children () && l->source (true).is_wildcard_layer ()) {
      lwc.push_back (l);
    }
  }

  //  after this sort we can modify the entries without invalidating the following iterators
  std::sort (lwc.begin (), lwc.end (), CompareLayerIteratorBottomUp ());

  for (std::vector<LayerPropertiesIterator>::const_iterator ll = lwc.begin (); ll != lwc.end (); ++ll) {
    LayerPropertiesIterator pos = *ll;
    //  Note: expand_wildcard_layers will recompute the already present layers on every call. Thus, only the
    //  first matching wildcard is effective.
    std::vector <LayerPropertiesNode> new_nodes = expand_wildcard_layers (*pos, *this, view (), list_index ());
    for (std::vector <LayerPropertiesNode>::const_iterator n = new_nodes.begin (); n != new_nodes.end (); ++n) {
      insert (pos, *n);
      pos.next_sibling ();
    }
    erase (pos);
  }

  //  Assign default colors and stipples for layers without any ...
  
  int stipple_index = 0;
  for (LayerPropertiesIterator l = begin_recursive (); !l.at_end (); ++l) {

    if (! l->has_children ()) {

      if (l->frame_color (true) == 0) {
        l->set_frame_color (view ()->get_palette ().luminous_color_by_index (l->source (true /*real*/).color_index ()));
      }

      if (l->fill_color (true) == 0) {
        l->set_fill_color (view ()->get_palette ().luminous_color_by_index (l->source (true /*real*/).color_index ()));
      }

      if (l->dither_pattern (true) < 0) {
        l->set_dither_pattern (view ()->get_stipple_palette ().standard_stipple_by_index (stipple_index));
      }

      ++stipple_index;

    }

  }
}

LayerPropertiesConstIterator 
LayerPropertiesList::begin_const_recursive () const
{
  return LayerPropertiesConstIterator (*this);
}

LayerPropertiesConstIterator 
LayerPropertiesList::end_const_recursive () const
{
  return LayerPropertiesConstIterator (*this, true);
}

LayerPropertiesIterator 
LayerPropertiesList::begin_recursive () 
{
  return LayerPropertiesIterator (*this);
}

LayerPropertiesIterator 
LayerPropertiesList::end_recursive () 
{
  return LayerPropertiesIterator (*this, true);
}

LayerPropertiesList::const_iterator 
LayerPropertiesList::begin_const () const
{
  return m_layer_properties.begin ();
}

LayerPropertiesList::const_iterator 
LayerPropertiesList::end_const () const
{
  return m_layer_properties.end ();
}

LayerPropertiesList::iterator 
LayerPropertiesList::begin () 
{
  return m_layer_properties.begin ();
}

LayerPropertiesList::iterator
LayerPropertiesList::end () 
{
  return m_layer_properties.end ();
}

LayerPropertiesNode &
LayerPropertiesList::back () 
{
  return m_layer_properties.back ();
}

const LayerPropertiesNode &
LayerPropertiesList::back () const
{
  return m_layer_properties.back ();
}

/**
 *  @brief A helper class for XML parser: convert a string to a color and vice versa
 */
struct UIntColorConverter 
  : private ColorConverter
{
  std::string to_string (const tl::color_t &c) const
  {
    if (c == 0) {
      return "";
    } else {
      return ColorConverter::to_string (tl::Color (c | 0xff000000));
    }
  }

  void from_string (const std::string &s, tl::color_t &c) const
  {
    if (s.empty ()) {
      c = 0;
    } else {
      tl::Color qc;
      ColorConverter::from_string (s, qc);
      c = qc.rgb () | 0xff000000;
    }
  }
};

/**
 *  @brief A helper class for XML parser: convert a string to a integer index and vice versa (with -1 being a blank string)
 */
struct WidthConverter 
{
  std::string to_string (int b) const
  {
    if (b < 0) {
      return "";
    } else {
      return tl::to_string (b);
    }
  }

  void from_string (const std::string &s, int &b) const
  {
    if (s.empty ()) {
      b = -1;
    } else {
      tl::from_string (s.c_str (), b);
    }
  }
};

/**
 *  @brief A helper class for XML parser: convert a string to a integer index and vice versa
 */
struct DitherPatternIndexConverter
{
  std::string to_string (int b) const
  {
    if (b < 0) {
      return "";
    } else if (b < std::distance (lay::DitherPattern::default_pattern ().begin (), lay::DitherPattern::default_pattern ().begin_custom ())) {
      //  intrinsic pattern
      return "I" + tl::to_string (b);
    } else {
      //  custom pattern
      return "C" + tl::to_string (b - std::distance (lay::DitherPattern::default_pattern ().begin (), lay::DitherPattern::default_pattern ().begin_custom ()));
    }
  }

  void from_string (const std::string &s, int &b) const
  {
    if (s.empty ()) {
      b = -1;
    } else if (s[0] == 'I') {
      tl::from_string (s.c_str () + 1, b);
    } else if (s[0] == 'C') {
      tl::from_string (s.c_str () + 1, b);
      b = b + std::distance (lay::DitherPattern::default_pattern ().begin (), lay::DitherPattern::default_pattern ().begin_custom ());
    } else {
      tl::from_string (s, b);
      if (b >= 16) {
        //  backward compatibility to older versions
        b = b - 16 + std::distance (lay::DitherPattern::default_pattern ().begin (), lay::DitherPattern::default_pattern ().begin_custom ());
      }
    }
  }
};

/**
 *  @brief A helper class for XML parser: convert a style string to a integer index and vice versa
 */
struct LineStyleIndexConverter
{
  std::string to_string (int b) const
  {
    if (b < 0) {
      return "";
    } else if (b < std::distance (lay::LineStyles::default_style ().begin (), lay::LineStyles::default_style ().begin_custom ())) {
      //  intrinsic pattern
      return "I" + tl::to_string (b);
    } else {
      //  custom pattern
      return "C" + tl::to_string (b - std::distance (lay::LineStyles::default_style ().begin (), lay::LineStyles::default_style ().begin_custom ()));
    }
  }

  void from_string (const std::string &s, int &b) const
  {
    if (s.empty ()) {
      b = -1;
    } else if (s[0] == 'I') {
      tl::from_string (s.c_str () + 1, b);
    } else if (s[0] == 'C') {
      tl::from_string (s.c_str () + 1, b);
      b = b + std::distance (lay::LineStyles::default_style ().begin (), lay::LineStyles::default_style ().begin_custom ());
    } else {
      tl::from_string (s, b);
      if (b >= 16) {
        //  backward compatibility to older versions
        b = b - 16 + std::distance (lay::LineStyles::default_style ().begin (), lay::LineStyles::default_style ().begin_custom ());
      }
    }
  }
};

static const tl::XMLElementList layer_element = tl::XMLElementList (
  //  HINT: these make_member calls want to be qualified: otherwise an internal error
  //  was observed ..
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::expanded,             &LayerPropertiesNode::set_expanded,         "expanded") +
  tl::make_member<tl::color_t, LayerPropertiesNode>  (&LayerPropertiesNode::frame_color_loc,      &LayerPropertiesNode::set_frame_color_code, "frame-color",        UIntColorConverter ()) +
  tl::make_member<tl::color_t, LayerPropertiesNode>  (&LayerPropertiesNode::fill_color_loc,       &LayerPropertiesNode::set_fill_color_code,  "fill-color",         UIntColorConverter ()) +
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::frame_brightness_loc, &LayerPropertiesNode::set_frame_brightness, "frame-brightness") + 
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::fill_brightness_loc,  &LayerPropertiesNode::set_fill_brightness,  "fill-brightness") + 
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::dither_pattern_loc,   &LayerPropertiesNode::set_dither_pattern,   "dither-pattern",     DitherPatternIndexConverter ()) + 
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::line_style_loc,       &LayerPropertiesNode::set_line_style,       "line-style",         LineStyleIndexConverter ()) +
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::valid_loc,            &LayerPropertiesNode::set_valid,            "valid") +
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::visible_loc,          &LayerPropertiesNode::set_visible,          "visible") + 
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::transparent_loc,      &LayerPropertiesNode::set_transparent,      "transparent") + 
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::width_loc,            &LayerPropertiesNode::set_width,            "width",              WidthConverter ()) + 
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::marked_loc,           &LayerPropertiesNode::set_marked,           "marked") + 
  tl::make_member<bool, LayerPropertiesNode>         (&LayerPropertiesNode::xfill_loc,            &LayerPropertiesNode::set_xfill,            "xfill") +
  tl::make_member<int, LayerPropertiesNode>          (&LayerPropertiesNode::animation_loc,        &LayerPropertiesNode::set_animation,        "animation") +
  tl::make_member<std::string, LayerPropertiesNode>  (&LayerPropertiesNode::name,                 &LayerPropertiesNode::set_name,             "name") +
  tl::make_member<std::string, LayerPropertiesNode>  (&LayerPropertiesNode::source_string_loc,    &LayerPropertiesNode::set_source,           "source") +
  tl::make_element<LayerPropertiesNode, LayerPropertiesNode::const_iterator, LayerPropertiesNode> (&LayerPropertiesNode::begin_children, &LayerPropertiesNode::end_children,     &LayerPropertiesNode::add_child, "group-members", &layer_element)
);

static const tl::XMLElementList layer_prop_list = tl::XMLElementList (
  tl::make_element<LayerPropertiesNode, LayerPropertiesList::const_iterator, LayerPropertiesList> (&LayerPropertiesList::begin_const, &LayerPropertiesList::end_const, &LayerPropertiesList::push_back, "properties", &layer_element) +
  tl::make_member (&LayerPropertiesList::name, &LayerPropertiesList::set_name, "name") +
  tl::make_element (&LayerPropertiesList::begin_custom_dither_pattern, &LayerPropertiesList::end_custom_dither_pattern, &LayerPropertiesList::push_custom_dither_pattern, "custom-dither-pattern", 
    tl::make_element (&lay::DitherPatternInfo::to_strings, &lay::DitherPatternInfo::from_strings, "pattern",
      tl::make_member<std::string, std::vector<std::string>::const_iterator, std::vector<std::string> > (&std::vector<std::string>::begin, &std::vector<std::string>::end, &std::vector<std::string>::push_back, "line")
    ) +
    tl::make_member (&lay::DitherPatternInfo::order_index, &lay::DitherPatternInfo::set_order_index, "order") +
    tl::make_member (&lay::DitherPatternInfo::name, &lay::DitherPatternInfo::set_name, "name") 
  ) +
  tl::make_element (&LayerPropertiesList::begin_custom_line_styles, &LayerPropertiesList::end_custom_line_styles, &LayerPropertiesList::push_custom_line_style, "custom-line-style",
    tl::make_member (&lay::LineStyleInfo::to_string, &lay::LineStyleInfo::from_string, "pattern") +
    tl::make_member (&lay::LineStyleInfo::order_index, &lay::LineStyleInfo::set_order_index, "order") +
    tl::make_member (&lay::LineStyleInfo::name, &lay::LineStyleInfo::set_name, "name")
  )
);

//  declaration of the layer properties file XML structure
static const tl::XMLStruct <LayerPropertiesList>
layer_prop_list_structure ("layer-properties", &layer_prop_list);

//  declaration of the layer properties file XML structure for a multi-tab file
static const tl::XMLStruct <std::vector<LayerPropertiesList> >
layer_prop_lists_structure ("layer-properties-tabs", 
  tl::make_element<LayerPropertiesList, std::vector<LayerPropertiesList>::const_iterator, std::vector<LayerPropertiesList> > (&std::vector<LayerPropertiesList>::begin, &std::vector<LayerPropertiesList>::end, &std::vector<LayerPropertiesList>::push_back, "layer-properties", &layer_prop_list) 
);

const tl::XMLElementList *
LayerPropertiesList::xml_format () 
{
  return &layer_prop_list;
}

void 
LayerPropertiesList::load (tl::XMLSource &stream)
{
  layer_prop_list_structure.parse (stream, *this); 
}

void 
LayerPropertiesList::save (tl::OutputStream &os) const
{
  layer_prop_list_structure.write (os, *this);
}

void 
LayerPropertiesList::load (tl::XMLSource &stream, std::vector <lay::LayerPropertiesList> &properties_lists)
{
  try {
    // "old" way
    lay::LayerPropertiesList properties_list;
    layer_prop_list_structure.parse (stream, properties_list); 
    properties_lists.push_back (properties_list);
  } catch (tl::Exception &ex) {
    try {
      // "new" way
      stream.reset ();
      layer_prop_lists_structure.parse (stream, properties_lists);
    } catch (tl::Exception &) {
      //  the first exception is likely to be the root cause, so let's rather throw this one
      throw ex;
    }
  }
}
  
void 
LayerPropertiesList::save (tl::OutputStream &os, const std::vector <lay::LayerPropertiesList> &properties_lists)
{
  layer_prop_lists_structure.write (os, properties_lists); 
}
  
void 
LayerPropertiesList::attach_view (lay::LayoutViewBase *view, unsigned int list_index)
{
  mp_view.reset (view);
  m_list_index = list_index;

  //  HINT: this method has the side effect of recomputing the layer source parameters.
  //  The action must not be suppressed if view == mp_view
  for (layer_list::iterator c = m_layer_properties.begin (); c != m_layer_properties.end (); ++c) {
    c->attach_view (view, list_index);
  }
}

lay::LayoutViewBase *
LayerPropertiesList::view () const
{
  return const_cast<lay::LayoutViewBase *> (mp_view.get ());
}

unsigned int
LayerPropertiesList::list_index () const
{
  return m_list_index;
}

void 
LayerPropertiesList::push_back (const LayerPropertiesNode &d)
{
  m_layer_properties.push_back (d);
}

void 
LayerPropertiesList::set_dither_pattern (const lay::DitherPattern &pattern) 
{
  m_dither_pattern = pattern;
}

void
LayerPropertiesList::set_line_styles (const lay::LineStyles &styles)
{
  m_line_styles = styles;
}

LayerPropertiesNode &
LayerPropertiesList::insert (const LayerPropertiesIterator &iter, const LayerPropertiesNode &node)
{
  tl_assert (! iter.is_null ());

  LayerPropertiesNode *ret = 0;

  LayerPropertiesIterator parent = iter.parent ();

  if (parent.is_null ()) {
    if (iter.child_index () > m_layer_properties.size ()) {
      throw tl::Exception (tl::to_string (tr ("Iterator is out of range in LayerPropertiesList::insert")));
    }
    ret = &*(m_layer_properties.insert (m_layer_properties.begin () + iter.child_index (), node));
  } else {
    if (iter.child_index () > size_t (parent->end_children () - parent->begin_children ())) {
      throw tl::Exception (tl::to_string (tr ("Iterator is out of range in LayerPropertiesList::insert")));
    }
    ret = &(parent->insert_child (parent->begin_children () + iter.child_index (), node));
  }

  //  attach the new object to the view
  ret->attach_view (view (), list_index ());

  return *ret;
}
  
void
LayerPropertiesList::erase (const LayerPropertiesIterator &iter)
{
  tl_assert (! iter.is_null ());

  std::pair <LayerPropertiesNode *, size_t> pp = iter.parent_obj ();

  if (pp.first == 0) {
    if (pp.second >= m_layer_properties.size ()) {
      throw tl::Exception (tl::to_string (tr ("Iterator is out of range in LayerPropertiesList::erase")));
    }
    m_layer_properties.erase (m_layer_properties.begin () + pp.second);
  } else {
    if (pp.second >= size_t (pp.first->end_children () - pp.first->begin_children ())) {
      throw tl::Exception (tl::to_string (tr ("Iterator is out of range in LayerPropertiesList::erase")));
    }
    pp.first->erase_child (pp.first->begin_children () + pp.second);
  }
}

// -------------------------------------------------------------
//  LayerPropertiesNodeRef implementation

LayerPropertiesNodeRef::LayerPropertiesNodeRef (LayerPropertiesNode *node)
  : m_iter (node), m_synched_gen_id (0)
{
  if (node) {

    //  NOTE: we do assignment before we set the iterator reference - hence there won't be
    //  updates triggered.
    LayerPropertiesNode::operator= (*node);

    //  Makes ourself a perfect copy of the original (including reference into the view)
    attach_view (node->view (), node->list_index ());
    set_parent (node->parent ());

    mp_node.reset (node);

  }
}

LayerPropertiesNodeRef::LayerPropertiesNodeRef (const LayerPropertiesConstIterator &iter)
  : m_iter (iter), m_synched_gen_id (0)
{
  if (!iter.at_end () && !iter.is_null ()) {

    const lay::LayerPropertiesNode *node = iter.operator-> ();

    //  NOTE: we do assignment before we set the iterator reference - hence there won't be
    //  updates triggered.
    LayerPropertiesNode::operator= (*node);

    //  Makes ourself a perfect copy of the original (including reference into the view)
    attach_view (node->view (), node->list_index ());
    set_parent (node->parent ());

    mp_node.reset (const_cast<lay::LayerPropertiesNode *> (node));

  }
}

LayerPropertiesNodeRef::LayerPropertiesNodeRef ()
  : m_synched_gen_id (0)
{
  //  .. nothing yet ..
}

LayerPropertiesNodeRef::LayerPropertiesNodeRef (const LayerPropertiesNodeRef &other)
  : LayerPropertiesNode (other), m_iter (other.m_iter), mp_node (other.mp_node), m_synched_gen_id (0)
{
  attach_view (other.view (), other.list_index ());
  set_parent (other.parent ());
}

LayerPropertiesNodeRef &LayerPropertiesNodeRef::operator= (const LayerPropertiesNodeRef &other)
{
  if (this != &other) {

    m_synched_gen_id = other.gen_id ();

    mp_node = other.mp_node;
    m_iter = other.m_iter;
    attach_view (other.view (), other.list_index ());
    set_parent (other.parent ());

    //  NOTE: this will update the view
    LayerPropertiesNode::operator= (other);

  }
  return *this;
}

void
LayerPropertiesNodeRef::erase ()
{
  if (is_valid ()) {
    view ()->delete_layer ((unsigned int) list_index (), m_iter);
    //  detach from everthing
    *this = LayerPropertiesNodeRef ();
  }
}

const lay::LayerPropertiesConstIterator &
LayerPropertiesNodeRef::iter () const
{
  return m_iter;
}

bool
LayerPropertiesNodeRef::is_valid () const
{
  return !m_iter.is_null () && !m_iter.at_end () && view ();
}

void
LayerPropertiesNodeRef::need_realize (unsigned int flags, bool force)
{
  LayerPropertiesNode::need_realize (flags, force);
  if (is_valid ()) {

    if ((flags & (nr_visual + nr_source + nr_meta)) != 0) {
      view ()->set_properties ((unsigned int) list_index (), m_iter, *this);
    }
    if ((flags & nr_hierarchy) != 0) {
      view ()->replace_layer_node ((unsigned int) list_index (), m_iter, *this);
    }

    //  we're in sync now
    m_synched_gen_id = mp_node->gen_id ();

  } else if (mp_node) {

    //  fallback mode is to use the target node directly.
    *mp_node = *this;
    m_synched_gen_id = mp_node->gen_id ();

  }
}

void
LayerPropertiesNodeRef::expanded_state_changed ()
{
  LayerPropertiesNode::expanded_state_changed ();

  if (is_valid ()) {
    view ()->set_layer_node_expanded (m_iter, expanded ());
  }
}

void
LayerPropertiesNodeRef::refresh () const
{
  if (! mp_node.get () || m_synched_gen_id == mp_node->gen_id ()) {
    return;
  }

  LayerPropertiesNodeRef *non_const_this = const_cast<LayerPropertiesNodeRef *> (this);

  non_const_this->m_synched_gen_id = mp_node->gen_id ();
  non_const_this->LayerPropertiesNode::operator= (*mp_node);
}

} // namespace lay

