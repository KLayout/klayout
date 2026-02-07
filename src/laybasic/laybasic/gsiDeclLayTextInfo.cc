
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


#include "gsiDecl.h"
#include "layTextInfo.h"
#include "layLayoutViewBase.h"
#include "layEditorUtils.h"
#include "layMarker.h"
#include "dbShape.h"

namespace gsi
{

class TextInfo
{
public:
  TextInfo (lay::LayoutViewBase *view)
    : mp_view (view), m_textinfo (view), m_tv (view)
  {
    m_border = lay::marker_text_border_in_pixels ();
  }

  void set_border (double pixels)
  {
    m_border = pixels;
  }

  double get_border () const
  {
    return m_border;
  }

  db::Box bbox_from_shape (const db::Shape &shape)
  {
    if (! mp_view || ! shape.is_text ()) {
      return db::Box ();
    }

    const db::Shapes *shapes = shape.shapes ();
    if (! shapes) {
      return db::Box ();
    }

    const db::Cell *cell = shapes->cell ();
    if (! cell) {
      return db::Box ();
    }

    const db::Layout *layout = cell->layout ();
    if (! layout) {
      return db::Box ();
    }

    int layer_index = -1;
    for (auto l = layout->begin_layers (); l != layout->end_layers () && layer_index < 0; ++l) {
      if (&cell->shapes ((*l).first) == shapes) {
        layer_index = int ((*l).first);
      }
    }

    if (layer_index < 0) {
      return db::Box ();
    }

    int cv_index = -1;
    for (unsigned int i = 0; i < mp_view->cellviews () && cv_index < 0; ++i) {
      const lay::CellView &cv = mp_view->cellview (i);
      if (cv.is_valid () && &cv->layout () == layout) {
        cv_index = int (i);
      }
    }

    if (cv_index < 0) {
      return db::Box ();
    }

    db::Text t;
    shape.text (t);
    return bbox_from_text (t, (unsigned int) cv_index, (unsigned int) layer_index);
  }

  db::Box bbox_from_text (const db::Text &text, unsigned int cv_index, int layer)
  {
    if (! mp_view) {
      return db::Box ();
    }

    const lay::CellView &cv = mp_view->cellview (cv_index);
    if (! cv.is_valid ()) {
      return db::Box ();
    }

    db::CplxTrans dbu_trans (cv->layout ().dbu ());
    return dbu_trans.inverted () * bbox_from_dtext (dbu_trans * text, cv_index, layer);
  }

  db::DBox bbox_from_dtext (const db::DText &dtext, int cv_index, int layer)
  {
    if (! mp_view) {
      return db::DBox ();
    }

    db::DCplxTrans tv_trans, ctx_trans;

    if (cv_index >= 0 && layer >= 0) {

      const lay::CellView &cv = mp_view->cellview (cv_index);
      if (cv.is_valid () && cv->layout ().is_valid_layer (layer)) {

        db::CplxTrans dbu_trans (cv->layout ().dbu ());
        ctx_trans = dbu_trans * cv.context_trans () * dbu_trans.inverted ();

        const std::vector<db::DCplxTrans> *tv_list = m_tv.per_cv_and_layer (cv_index, layer);
        if (tv_list != 0 && ! tv_list->empty ()) {
          tv_trans = tv_list->front ();
        }

      }

    }

    db::DCplxTrans vp_trans = db::DCplxTrans (double (mp_view->canvas ()->oversampling ())) * mp_view->viewport ().trans () * tv_trans;
    db::DBox box = m_textinfo.bbox (ctx_trans * dtext, vp_trans);

    double b = m_border / vp_trans.mag ();
    return box.enlarged (db::DVector (b, b));
  }

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  lay::TextInfo m_textinfo;
  lay::TransformationVariants m_tv;
  double m_border;
};

gsi::TextInfo *new_textinfo (lay::LayoutViewBase *view)
{
  return new gsi::TextInfo (view);
}

Class<gsi::TextInfo> decl_TextInfo ("lay", "TextInfo",
  gsi::constructor ("new", &new_textinfo, gsi::arg ("view"),
    "@brief Creates a TextInfo object for a given layout view\n"
  ) +
  gsi::method ("border=", &gsi::TextInfo::set_border, gsi::arg ("pixels"),
    "@brief Sets the border in pixels\n"
    "This attribute adds a border between the bounding box edges and the character polygons "
    "for better readability of the text when a box is drawn around them. The value is given in "
    "screen pixels. The default value is the one used for the markers in the application."
  ) +
  gsi::method ("border", &gsi::TextInfo::get_border,
    "@brief Gets the border in pixels\n"
    "See \\border= for details about this attribute."
  ) +
  gsi::method ("bbox", &gsi::TextInfo::bbox_from_shape, gsi::arg ("shape"),
    "@brief Obtains the bounding box for the given text-type shape\n"
    "\n"
    "If the shape is not a text object or it is not part of a layout shown in the layout view, this "
    "method will return an empty box. Otherwise, it will return a \\Box object, representing the bounding box "
    "of the text object, including the label's character representation.\n"
    "\n"
    "The bounding box is given as an equivalent integer-unit \\Box object, when placed in the same cell and on the same layer as the original text object."
  ) +
  gsi::method ("bbox", &gsi::TextInfo::bbox_from_text, gsi::arg ("text"), gsi::arg ("cv_index"), gsi::arg ("layer_index", -1),
    "@brief Obtains the bounding box for the given text object\n"
    "\n"
    "This method returns a \\Box object, representing the bounding box of the integer-unit \\Text object.\n"
    "The cellview index needs to be specified, while the layer index is optional. The layer index is the layer where the text object "
    "lives in the layout, given by the cellview index. Without a layer, the bounding box computation will not take into account potential "
    "additional transformations implied by transformations present the layer view specification.\n"
    "\n"
    "The bounding box is given as an equivalent integer-unit \\Box object, when placed in the same cell and on the same layer as the original text object."
  ) +
  gsi::method ("bbox", &gsi::TextInfo::bbox_from_dtext, gsi::arg ("dtext"), gsi::arg ("cv_index", -1), gsi::arg ("layer_index", -1),
    "@brief Obtains the bounding box for the given micrometer-unit text object\n"
    "\n"
    "This method returns a \\DBox object, representing the bounding box of the micrometer-unit \\DText object.\n"
    "The cellview and layer index is optional. Without a layer and cellview index, the bounding box computation will not take into account potential "
    "additional transformations implied by transformations present the layer view specification.\n"
    "\n"
    "The bounding box is given as an equivalent micrometer-unit \\DBox object, when placed in the same cell and on the same layer as the original text object."
  ),
  "@brief A utility class for generating text bounding boxes including the glyph polygons\n"
  "\n"
  "The geometry database regards text objects as point-like, hence the natural bounding box of a "
  "text object is a single point. To obtain the visual bounding box, you can use the \\TextInfo object. "
  "It is created from a layout view and allows computing bounding boxes from \\Text, \\DText or \\Shape objects which "
  "include the visual representation of the text.\n"
  "\n"
  "That bounding box is given in the equivalent space of the original text object - i.e. when it is placed into the same cell "
  "and on the same layer than the original text.\n"
  "\n"
  "This computation is not trivial, because there are fonts that do not scale with zoom level. Hence, the equivalent bounding "
  "bounding box depends on the zoom factor and other transformations that control the rendering of the text. Also, a number of "
  "settings from the layout view - specifically default font or default text height influence the appearance of the characters "
  "and need to be considered. The TextInfo object takes care of these things.\n"
  "\n"
  "It does not take care however of transformations applied inside the hierarchy. Specifically, when a text object is not "
  "in the current top cell, different instantiation paths may exist that render different bounding boxes. Hence there not a single "
  "equivalent bounding box for a text object not inside the top cell. It is recommended to first transform the texts into the top "
  "cell before computing the bounding boxes.\n"
  "\n"
  "Here is some sample code that places boxes over each selected text object. These boxes are identical to the selection markers "
  "of the texts, but this identity quickly vanishes if you zoom in or out:\n"
  "\n"
  "@code\n"
  "begin\n"
  "\n"
  "  view = RBA::LayoutView.current\n"
  "  # Provide undo support, so it is more convenient to try out\n"
  "  view.transaction(\"Generate true label bounding boxes\")\n"
  "\n"
  "  textinfo = RBA::TextInfo::new(view)\n"
  "  \n"
  "  view.each_object_selected do |sel|\n"
  "  \n"
  "    # Ignore selected objects which are not texts\n"
  "    sel.shape.is_text? || next\n"
  "    \n"
  "    # Transform the text to top level  \n"
  "    tl_text = sel.trans * sel.shape.text\n"
  "    \n"
  "    # Compute the bounding box\n"
  "    bbox = textinfo.bbox(tl_text, sel.cv_index, sel.layer)\n"
  "    \n"
  "    # Place boxes over the original texts\n"
  "    # Note that 'ctx_cell' is the true origin of the selection path, hence the one that 'sel.trans' applies to\n"
  "    view.cellview(sel.cv_index).ctx_cell.shapes(sel.layer).insert(bbox)\n"
  "    \n"
  "  end\n"
  "\n"
  "ensure\n"
  "  view.commit\n"
  "\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.30.5."
);

}
