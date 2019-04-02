
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "gsiSignals.h"
#include "antObject.h"
#include "antService.h"
#include "antPlugin.h"
#include "layLayoutView.h"

namespace gsi
{

class AnnotationRef;

static int style_ruler ()       { return int (ant::Object::STY_ruler); }
static int style_arrow_end ()   { return int (ant::Object::STY_arrow_end); }
static int style_arrow_start () { return int (ant::Object::STY_arrow_start); }
static int style_arrow_both ()  { return int (ant::Object::STY_arrow_both); }
static int style_line ()        { return int (ant::Object::STY_line); }

static int outline_diag ()      { return int (ant::Object::OL_diag); }
static int outline_xy ()        { return int (ant::Object::OL_xy); }
static int outline_diag_xy ()   { return int (ant::Object::OL_diag_xy); }
static int outline_yx ()        { return int (ant::Object::OL_yx); }
static int outline_diag_yx ()   { return int (ant::Object::OL_diag_yx); }
static int outline_box ()       { return int (ant::Object::OL_box); }

static int angle_any ()         { return int (lay::AC_Any); }
static int angle_diagonal ()    { return int (lay::AC_Diagonal); }
static int angle_ortho ()       { return int (lay::AC_Ortho); }
static int angle_horizontal ()  { return int (lay::AC_Horizontal); }
static int angle_vertical ()    { return int (lay::AC_Vertical); }
static int angle_global ()      { return int (lay::AC_Global); }

static int pos_auto ()          { return int (ant::Object::POS_auto); }
static int pos_p1 ()            { return int (ant::Object::POS_p1); }
static int pos_p2 ()            { return int (ant::Object::POS_p2); }
static int pos_center ()        { return int (ant::Object::POS_center); }

static int align_auto ()        { return int (ant::Object::AL_auto); }
static int align_center ()      { return int (ant::Object::AL_center); }
static int align_left ()        { return int (ant::Object::AL_left); }
static int align_bottom ()      { return int (ant::Object::AL_bottom); }
static int align_down ()        { return int (ant::Object::AL_down); }
static int align_right ()       { return int (ant::Object::AL_right); }
static int align_top ()         { return int (ant::Object::AL_top); }
static int align_up ()          { return int (ant::Object::AL_up); }

static void clear_annotations (lay::LayoutView *view);
static void insert_annotation (lay::LayoutView *view, AnnotationRef *obj);
static void erase_annotation (lay::LayoutView *view, int id);
static void replace_annotation (lay::LayoutView *view, int id, const AnnotationRef &obj);

/**
 *  @brief An extension of the ant::Object that provides "live" updates of the view
 */
class AnnotationRef
  : public ant::Object
{
public:
  AnnotationRef ()
    : ant::Object ()
  {
    //  .. nothing yet ..
  }

  AnnotationRef (const ant::Object &other, lay::LayoutView *view)
    : ant::Object (other), mp_view (view)
  {
    //  .. nothing yet ..
  }

  AnnotationRef (const AnnotationRef &other)
    : ant::Object (other), mp_view (other.mp_view)
  {
    //  .. nothing yet ..
  }

  AnnotationRef &operator= (const AnnotationRef &other)
  {
    //  NOTE: assignment changes the properties, not the reference
    if (this != &other) {
      ant::Object::operator= (other);
    }
    return *this;
  }

  bool operator== (const AnnotationRef &other) const
  {
    return ant::Object::operator== (other);
  }

  bool operator!= (const AnnotationRef &other) const
  {
    return ant::Object::operator!= (other);
  }

  void detach ()
  {
    mp_view.reset (0);
  }

  bool is_valid () const
  {
    return (mp_view && id () >= 0);
  }

  void erase ()
  {
    if (mp_view && id () >= 0) {
      erase_annotation (mp_view.get (), id ());
      detach ();
    }
  }

  template <class T>
  AnnotationRef transformed (const T &t) const
  {
    return AnnotationRef (ant::Object::transformed<T> (t), const_cast<lay::LayoutView *> (mp_view.get ()));
  }

  void set_view (lay::LayoutView *view)
  {
    mp_view.reset (view);
  }

protected:
  void property_changed ()
  {
    if (mp_view && id () >= 0) {
      replace_annotation (mp_view.get (), id (), *this);
    }
  }

private:
  tl::weak_ptr<lay::LayoutView> mp_view;
};

static void clear_annotations (lay::LayoutView *view)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    ant_service->clear_rulers ();
  }
}

static void insert_annotation (lay::LayoutView *view, AnnotationRef *obj)
{
  if (obj->is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("The object is already inserted into a view - detach the object first or create a different object.")));
  }

  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    int id = ant_service->insert_ruler (*obj, false /*do not observe the ruler count limit*/);
    obj->id (id);
    obj->set_view (view);
  }
}

static void erase_annotation (lay::LayoutView *view, int id)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    for (ant::AnnotationIterator a = ant_service->begin_annotations (); ! a.at_end (); ++a) {
      if (a->id () == id) {
        ant_service->delete_ruler (a.current ());
        break;
      }
    }
  }
}

static void replace_annotation (lay::LayoutView *view, int id, const AnnotationRef &obj)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    for (ant::AnnotationIterator a = ant_service->begin_annotations (); ! a.at_end (); ++a) {
      if (a->id () == id) {
        ant_service->change_ruler (a.current (), obj);
        break;
      }
    }
  }
}

static int get_style (const AnnotationRef *obj)
{
  return int (obj->style ());
}

static void set_style (AnnotationRef *obj, int style)
{
  obj->style (AnnotationRef::style_type (style));
}

static int get_outline (const AnnotationRef *obj)
{
  return int (obj->outline ());
}

static void set_outline (AnnotationRef *obj, int outline)
{
  obj->outline (AnnotationRef::outline_type (outline));
}

static int get_angle_constraint (const AnnotationRef *obj)
{
  return int (obj->angle_constraint ());
}

static void set_angle_constraint (AnnotationRef *obj, int angle_constraint)
{
  obj->angle_constraint (lay::angle_constraint_type (angle_constraint));
}

static int get_main_position (const AnnotationRef *obj)
{
  return int (obj->main_position ());
}

static void set_main_position (AnnotationRef *obj, int pos)
{
  obj->set_main_position ((ant::Object::position_type) pos);
}

static int get_main_xalign (const AnnotationRef *obj)
{
  return int (obj->main_xalign ());
}

static void set_main_xalign (AnnotationRef *obj, int align)
{
  obj->set_main_xalign ((ant::Object::alignment_type) align);
}

static int get_main_yalign (const AnnotationRef *obj)
{
  return int (obj->main_yalign ());
}

static void set_main_yalign (AnnotationRef *obj, int align)
{
  obj->set_main_yalign ((ant::Object::alignment_type) align);
}

static int get_xlabel_xalign (const AnnotationRef *obj)
{
  return int (obj->xlabel_xalign ());
}

static void set_xlabel_xalign (AnnotationRef *obj, int align)
{
  obj->set_xlabel_xalign ((ant::Object::alignment_type) align);
}

static int get_xlabel_yalign (const AnnotationRef *obj)
{
  return int (obj->xlabel_yalign ());
}

static void set_xlabel_yalign (AnnotationRef *obj, int align)
{
  obj->set_xlabel_yalign ((ant::Object::alignment_type) align);
}

static int get_ylabel_xalign (const AnnotationRef *obj)
{
  return int (obj->ylabel_xalign ());
}

static void set_ylabel_xalign (AnnotationRef *obj, int align)
{
  obj->set_ylabel_xalign ((ant::Object::alignment_type) align);
}

static int get_ylabel_yalign (const AnnotationRef *obj)
{
  return int (obj->ylabel_yalign ());
}

static void set_ylabel_yalign (AnnotationRef *obj, int align)
{
  obj->set_ylabel_yalign ((ant::Object::alignment_type) align);
}

/**
 *  @brief An alternative iterator that returns "live" AnnotationRef objects
 */
struct AnnotationRefIterator
  : public ant::AnnotationIterator
{
public:
  typedef AnnotationRef reference;

  AnnotationRefIterator ()
    : ant::AnnotationIterator ()
  {
    //  .. nothing yet ..
  }

  AnnotationRefIterator (const ant::AnnotationIterator &iter, lay::LayoutView *view)
    : ant::AnnotationIterator (iter), mp_view (view)
  {
    //  .. nothing yet ..
  }

  reference operator* () const
  {
    return reference (ant::AnnotationIterator::operator* (), const_cast<lay::LayoutView * >(mp_view.get ()));
  }

private:
  tl::weak_ptr<lay::LayoutView> mp_view;
};

static AnnotationRefIterator begin_annotations (lay::LayoutView *view)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    return AnnotationRefIterator (ant_service->begin_annotations (), view);
  } else {
    return AnnotationRefIterator ();
  }
}

static AnnotationRef get_annotation (lay::LayoutView *view, int id)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  if (ant_service) {
    for (AnnotationRefIterator iter (ant_service->begin_annotations (), view); !iter.at_end(); ++iter) {
      if ((*iter).id () == id) {
        return *iter;
      }
    }
  }
  return AnnotationRef ();
}

static tl::Event &get_annotations_changed_event (lay::LayoutView *view)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  return ant_service->annotations_changed_event;
}

static tl::Event &get_annotation_selection_changed_event (lay::LayoutView *view)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  return ant_service->annotation_selection_changed_event;
}

static tl::event<int> &get_annotation_changed_event (lay::LayoutView *view)
{
  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  return ant_service->annotation_changed_event;
}

static int ruler_mode_normal ()
{
  return ant::Template::RulerNormal;
}

static int ruler_mode_single_click ()
{
  return ant::Template::RulerSingleClick;
}

static int ruler_mode_auto_metric ()
{
  return ant::Template::RulerAutoMetric;
}

static void register_annotation_template (const ant::Object &a, const std::string &title, int mode)
{
  ant::Template t;

  t.angle_constraint (a.angle_constraint ());
  t.category (a.category ());
  t.fmt (a.fmt ());
  t.fmt_x (a.fmt_x ());
  t.fmt_y (a.fmt_y ());
  t.set_main_position (a.main_position ());
  t.set_main_xalign (a.main_xalign ());
  t.set_main_yalign (a.main_yalign ());
  t.set_xlabel_xalign (a.xlabel_xalign ());
  t.set_xlabel_yalign (a.xlabel_yalign ());
  t.set_ylabel_xalign (a.ylabel_xalign ());
  t.set_ylabel_yalign (a.ylabel_yalign ());
  t.outline (a.outline ());
  t.style (a.style ());
  t.title (title);

  t.set_mode (ant::Template::ruler_mode_type (mode));

  if (ant::PluginDeclaration::instance ()) {
    ant::PluginDeclaration::instance ()->register_annotation_template (t);
  }
}

//  NOTE: ant::Object is available as "BasicAnnotation" to allow binding for other methods.
gsi::Class<ant::Object> decl_BasicAnnotation ("lay", "BasicAnnotation", gsi::Methods (), "@hide\n@alias Annotation");

gsi::Class<AnnotationRef> decl_Annotation (decl_BasicAnnotation, "lay", "Annotation",
  gsi::method ("register_template", &gsi::register_annotation_template,
    gsi::arg ("annotation"), gsi::arg ("title"), gsi::arg ("mode", ruler_mode_normal (), "\\RulerModeNormal"),
    "@brief Registers the given annotation as a template\n"
    "@param title The title to use for the ruler template\n"
    "@param mode The mode the ruler will be created in (see Ruler... constants)\n"
    "\n"
    "In order to register a system template, the category string of the annotation should be "
    "a unique and non-empty string. The annotation is added to the list of annotation templates "
    "and becomes available as a new template in the ruler drop-down menu.\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  gsi::method ("RulerModeNormal", &gsi::ruler_mode_normal,
    "@brief Specifies normal ruler mode for the \\register_template method\n"
    "\n"
    "This constant has been introduced in version 0.25"
  ) +
  gsi::method ("RulerModeSingleClick", &gsi::ruler_mode_single_click,
    "@brief Specifies single-click ruler mode for the \\register_template method\n"
    "In single click-mode, a ruler can be placed with a single click and p1 will be == p2."
    "\n"
    "This constant has been introduced in version 0.25"
  ) +
  gsi::method ("RulerModeAutoMetric", &gsi::ruler_mode_auto_metric,
    "@brief Specifies auto-metric ruler mode for the \\register_template method\n"
    "In auto-metric mode, a ruler can be placed with a single click and p1/p2 will be determined from the neighborhood."
    "\n"
    "This constant has been introduced in version 0.25"
  ) +
  gsi::method ("StyleRuler|#style_ruler", &gsi::style_ruler,
    "@brief Gets the ruler style code for use the \\style method\n"
    "When this style is specified, the annotation will show a ruler with "
    "some ticks at distances indicating a decade of units and a suitable "
    "subdivision into minor ticks at intervals of 1, 2 or 5 units."
  ) +
  gsi::method ("StyleArrowEnd|#style_arrow_end", &gsi::style_arrow_end,
    "@brief Gets the end arrow style code for use the \\style method\n"
    "When this style is specified, an arrow is drawn pointing from the start to the end point."
  ) +
  gsi::method ("StyleArrowStart|#style_arrow_start", &gsi::style_arrow_start,
    "@brief Gets the start arrow style code for use the \\style method\n"
    "When this style is specified, an arrow is drawn pointing from the end to the start point."
  ) +
  gsi::method ("StyleArrowBoth|#style_arrow_both", &gsi::style_arrow_both,
    "@brief Gets the both arrow ends style code for use the \\style method\n"
    "When this style is specified, a two-headed arrow is drawn."
  ) +
  gsi::method ("StyleLine|#style_line", &gsi::style_line,
    "@brief Gets the line style code for use with the \\style method\n"
    "When this style is specified, plain line is drawn."
  ) +
  gsi::method ("OutlineDiag|#outline_diag", &gsi::outline_diag,
    "@brief Gets the diagonal output code for use with the \\outline method\n"
    "When this outline style is specified, a line connecting start and "
    "end points in the given style (ruler, arrow or plain line) is drawn."
  ) +
  gsi::method ("OutlineXY|#outline_xy", &gsi::outline_xy,
    "@brief Gets the xy outline code for use with the \\outline method\n"
    "When this outline style is specified, two lines are drawn: one horizontal from left "
    "to right and attached to the end of that a line from the bottom to the top. The lines "
    "are drawn in the specified style (see \\style method)."
  ) +
  gsi::method ("OutlineDiagXY|#outline_diag_xy", &gsi::outline_diag_xy,
    "@brief Gets the xy plus diagonal outline code for use with the \\outline method\n"
    "@brief outline_xy code used by the \\outline method\n"
    "When this outline style is specified, three lines are drawn: one horizontal from left "
    "to right and attached to the end of that a line from the bottom to the top. Another line "
    "is drawn connecting the start and end points directly. The lines "
    "are drawn in the specified style (see \\style method)."
  ) +
  gsi::method ("OutlineYX|#outline_yx", &gsi::outline_yx ,
    "@brief Gets the yx outline code for use with the \\outline method\n"
    "When this outline style is specified, two lines are drawn: one vertical from bottom "
    "to top and attached to the end of that a line from the left to the right. The lines "
    "are drawn in the specified style (see \\style method)."
  ) +
  gsi::method ("OutlineDiagYX|#outline_diag_yx", &gsi::outline_diag_yx ,
    "@brief Gets the yx plus diagonal outline code for use with the \\outline method\n"
    "When this outline style is specified, three lines are drawn: one vertical from bottom "
    "to top and attached to the end of that a line from the left to the right. Another line "
    "is drawn connecting the start and end points directly. The lines "
    "are drawn in the specified style (see \\style method)."
  ) +
  gsi::method ("OutlineBox|#outline_box", &gsi::outline_box,
    "@brief Gets the box outline code for use with the \\outline method\n"
    "When this outline style is specified, a box is drawn with the corners specified by the "
    "start and end point. All box edges are drawn in the style specified with the \\style "
    "attribute."
  ) +
  gsi::method ("AngleAny|#angle_any", &gsi::angle_any,
    "@brief Gets the any angle code for use with the \\angle_constraint method\n"
    "If this value is specified for the angle constraint, all angles will be allowed."
  ) +
  gsi::method ("AngleDiagonal|#angle_diagonal", &gsi::angle_diagonal,
    "@brief Gets the diagonal angle code for use with the \\angle_constraint method\n"
    "If this value is specified for the angle constraint, only multiples of 45 degree are allowed."
  ) +
  gsi::method ("AngleOrtho|#angle_ortho", &gsi::angle_ortho,
    "@brief Gets the ortho angle code for use with the \\angle_constraint method\n"
    "If this value is specified for the angle constraint, only multiples of 90 degree are allowed."
  ) +
  gsi::method ("AngleHorizontal|#angle_horizontal", &gsi::angle_horizontal,
    "@brief Gets the horizontal angle code for use with the \\angle_constraint method\n"
    "If this value is specified for the angle constraint, only horizontal rulers are allowed."
  ) +
  gsi::method ("AngleVertical|#angle_vertical", &gsi::angle_vertical,
    "@brief Gets the vertical angle code for use with the \\angle_constraint method\n"
    "If this value is specified for the angle constraint, only vertical rulers are allowed."
  ) +
  gsi::method ("AngleGlobal|#angle_global", &gsi::angle_global,
    "@brief Gets the global angle code for use with the \\angle_constraint method.\n"
    "This code will tell the ruler or marker to use the angle constraint defined globally."
  ) +
  gsi::method ("PositionAuto", &gsi::pos_auto,
    "@brief This code indicates automatic positioning.\n"
    "The main label will be put either to p1 or p2, whichever the annotation considers best.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("PositionP1", &gsi::pos_p1,
    "@brief This code indicates positioning of the main label at p1.\n"
    "The main label will be put to p1.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("PositionP2", &gsi::pos_p2,
    "@brief This code indicates positioning of the main label at p2.\n"
    "The main label will be put to p2.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("PositionCenter", &gsi::pos_center,
    "@brief This code indicates positioning of the main label at the mid point between p1 and p2.\n"
    "The main label will be put to the center point.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignAuto", &gsi::align_auto,
    "@brief This code indicates automatic alignment.\n"
    "This code makes the annotation align the label the way it thinks is best.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignCenter", &gsi::align_center,
    "@brief This code indicates automatic alignment.\n"
    "This code makes the annotation align the label centered. When used in a horizontal context, "
    "centering is in horizontal direction. If used in a vertical context, centering is in vertical direction.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignLeft", &gsi::align_left,
    "@brief This code indicates left alignment.\n"
    "If used in a horizontal context, this alignment code makes the label aligned at the left side - i.e. it will appear right of the reference point.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignBottom", &gsi::align_bottom,
    "@brief This code indicates bottom alignment.\n"
    "If used in a vertical context, this alignment code makes the label aligned at the bottom side - i.e. it will appear top of the reference point.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignDown", &gsi::align_down,
    "@brief This code indicates left or bottom alignment, depending on the context.\n"
    "This code is equivalent to \\AlignLeft and \\AlignBottom.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignRight", &gsi::align_right,
    "@brief This code indicates right alignment.\n"
    "If used in a horizontal context, this alignment code makes the label aligned at the right side - i.e. it will appear left of the reference point.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignTop", &gsi::align_top,
    "@brief This code indicates top alignment.\n"
    "If used in a vertical context, this alignment code makes the label aligned at the top side - i.e. it will appear bottom of the reference point.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("AlignUp", &gsi::align_up,
    "@brief This code indicates right or top alignment, depending on the context.\n"
    "This code is equivalent to \\AlignRight and \\AlignTop.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("detach", &AnnotationRef::detach,
    "@brief Detaches the annotation object from the view\n"
    "If the annotation object was inserted into the view, property changes will be "
    "reflected in the view. To disable this feature, 'detach' can be called after which "
    "the annotation object becomes inactive and changes will no longer be reflected in the view.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("delete", &AnnotationRef::erase,
    "@brief Deletes this annotation from the view\n"
    "If the annotation is an \"active\" one, this method will remove it from the view. "
    "This object will become detached and can still be manipulated, but without having an "
    "effect on the view."
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("p1", (const db::DPoint & (AnnotationRef::*) () const) &AnnotationRef::p1,
    "@brief Gets the first point of the ruler or marker\n"
    "The points of the ruler or marker are always given in micron units in floating-point "
    "coordinates.\n"
    "@return The first point\n"
  ) +
  gsi::method ("p2", (const db::DPoint & (AnnotationRef::*) () const) &AnnotationRef::p2,
    "@brief Gets the second point of the ruler or marker\n"
    "The points of the ruler or marker are always given in micron units in floating-point "
    "coordinates.\n"
    "@return The second point\n"
  ) +
  gsi::method ("p1=", (void (AnnotationRef::*) (const db::DPoint &)) &AnnotationRef::p1,
    "@brief Sets the first point of the ruler or marker\n"
    "The points of the ruler or marker are always given in micron units in floating-point "
    "coordinates.\n"
    "@args point\n"
  ) +
  gsi::method ("p2=", (void (AnnotationRef::*) (const db::DPoint &)) &AnnotationRef::p2,
    "@brief Sets the second point of the ruler or marker\n"
    "The points of the ruler or marker are always given in micron units in floating-point "
    "coordinates.\n"
    "@args point\n"
  ) +
  gsi::method ("box", &AnnotationRef::box,
    "@brief Gets the bounding box of the object (not including text)\n"
    "@return The bounding box\n"
  ) +
  gsi::method ("transformed", &AnnotationRef::transformed<db::DTrans>,
    "@brief Transforms the ruler or marker with the given simple transformation\n"
    "@args t\n"
    "@param t The transformation to apply\n"
    "@return The transformed object\n"
  ) +
  gsi::method ("transformed|#transformed_cplx", &AnnotationRef::transformed<db::DCplxTrans>,
    "@brief Transforms the ruler or marker with the given complex transformation\n"
    "@args t\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed object\n"
    "\n"
    "Starting with version 0.25, all overloads all available as 'transform'."
  ) +
  gsi::method ("transformed|#transformed_cplx", &AnnotationRef::transformed<db::ICplxTrans>,
    "@brief Transforms the ruler or marker with the given complex transformation\n"
    "@args t\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed object (in this case an integer coordinate object)\n"
    "\n"
    "This method has been introduced in version 0.18.\n"
    "\n"
    "Starting with version 0.25, all overloads all available as 'transform'."
  ) +
  gsi::method ("fmt=", (void (AnnotationRef::*) (const std::string &)) &AnnotationRef::fmt,
    "@brief Sets the format used for the label\n"
    "@args format\n"
    "@param format The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method ("fmt", (const std::string & (AnnotationRef::*) () const) &AnnotationRef::fmt,
    "@brief Returns the format used for the label\n"
    "@return The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method ("fmt_x=", (void (AnnotationRef::*) (const std::string &)) &AnnotationRef::fmt_x,
    "@brief Sets the format used for the x-axis label\n"
    "@args format\n"
    "X-axis labels are only used for styles that have a horizontal component. "
    "@param format The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method ("fmt_x", (const std::string & (AnnotationRef::*) () const) &AnnotationRef::fmt_x,
    "@brief Returns the format used for the x-axis label\n"
    "@return The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method ("fmt_y=", (void (AnnotationRef::*) (const std::string &)) &AnnotationRef::fmt_y,
    "@brief Sets the format used for the y-axis label\n"
    "@args format\n"
    "Y-axis labels are only used for styles that have a vertical component. "
    "@param format The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method ("fmt_y", (const std::string & (AnnotationRef::*) () const) &AnnotationRef::fmt_y,
    "@brief Returns the format used for the y-axis label\n"
    "@return The format string\n"
    "Format strings can contain placeholders for values and formulas for computing derived "
    "values. See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
    "more details."
  ) +
  gsi::method_ext ("style=", &gsi::set_style, 
    "@brief Sets the style used for drawing the annotation object\n"
    "@args style\n"
    "The Style... values can be used for defining the annotation object's style. The style determines "
    "if ticks or arrows are drawn."
  ) +
  gsi::method_ext ("style", &gsi::get_style,
    "@brief Returns the style of the annotation object\n"
  ) +
  gsi::method_ext ("outline=", &gsi::set_outline,
    "@brief Sets the outline style used for drawing the annotation object\n"
    "@args outline\n"
    "The Outline... values can be used for defining the annotation object's outline. The "
    "outline style determines what components are drawn. "
  ) +
  gsi::method_ext ("outline", &gsi::get_outline,
    "@brief Returns the outline style of the annotation object\n"
  ) +
  gsi::method ("category=", &ant::Object::set_category, gsi::arg ("cat"),
    "@brief Sets the category string of the annotation\n"
    "The category string is an arbitrary string that can be used by various consumers "
    "or generators to mark 'their' annotation.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method ("category", &ant::Object::category,
    "@brief Gets the category string\n"
    "See \\category= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_position=", &gsi::set_main_position, gsi::arg ("pos"),
    "@brief Sets the position of the main label\n"
    "This method accepts one of the Position... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_position", &gsi::get_main_position,
    "@brief Gets the position of the main label\n"
    "See \\main_position= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_xalign=", &gsi::set_main_xalign, gsi::arg ("align"),
    "@brief Sets the horizontal alignment type of the main label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_xalign", &gsi::get_main_xalign,
    "@brief Gets the horizontal alignment type of the main label\n"
    "See \\main_xalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_yalign=", &gsi::set_main_yalign, gsi::arg ("align"),
    "@brief Sets the vertical alignment type of the main label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("main_yalign", &gsi::get_main_yalign,
    "@brief Gets the vertical alignment type of the main label\n"
    "See \\main_yalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("xlabel_xalign=", &gsi::set_xlabel_xalign, gsi::arg ("align"),
    "@brief Sets the horizontal alignment type of the x axis label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("xlabel_xalign", &gsi::get_xlabel_xalign,
    "@brief Gets the horizontal alignment type of the x axis label\n"
    "See \\xlabel_xalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("xlabel_yalign=", &gsi::set_xlabel_yalign, gsi::arg ("align"),
    "@brief Sets the vertical alignment type of the x axis label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("xlabel_yalign", &gsi::get_xlabel_yalign,
    "@brief Gets the vertical alignment type of the x axis label\n"
    "See \\xlabel_yalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("ylabel_xalign=", &gsi::set_ylabel_xalign, gsi::arg ("align"),
    "@brief Sets the horizontal alignment type of the y axis label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("ylabel_xalign", &gsi::get_ylabel_xalign,
    "@brief Gets the horizontal alignment type of the y axis label\n"
    "See \\ylabel_xalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("ylabel_yalign=", &gsi::set_ylabel_yalign, gsi::arg ("align"),
    "@brief Sets the vertical alignment type of the y axis label\n"
    "This method accepts one of the Align... constants.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method_ext ("ylabel_yalign", &gsi::get_ylabel_yalign,
    "@brief Gets the vertical alignment type of the y axis label\n"
    "See \\ylabel_yalign= for details.\n"
    "\n"
    "This method has been introduced in version 0.25"
  ) +
  gsi::method ("snap=", (void (AnnotationRef::*) (bool)) &AnnotationRef::snap,
    "@brief Sets the 'snap to objects' attribute\n"
    "@args flag\n"
    "If this attribute is set to true, the ruler or marker snaps to other objects when moved. "
  ) +
  gsi::method ("snap?", (bool (AnnotationRef::*) () const) &AnnotationRef::snap,
    "@brief Returns the 'snap to objects' attribute\n"
  ) +
  gsi::method_ext ("angle_constraint=", &gsi::set_angle_constraint,
    "@brief Sets the angle constraint attribute\n"
    "@args flag\n"
    "This attribute controls if an angle constraint is applied when moving one of the ruler's "
    "points. The Angle... values can be used for this purpose." 
  ) +
  gsi::method_ext ("angle_constraint", &gsi::get_angle_constraint,
    "@brief Returns the angle constraint attribute\n"
    "See \\angle_constraint= for a more detailed description."
  ) +
  gsi::method ("text_x", (std::string (AnnotationRef::*)() const) &AnnotationRef::text_x,
    "@brief Returns the formatted text for the x-axis label"
  ) +
  gsi::method ("text_y", (std::string (AnnotationRef::*)() const) &AnnotationRef::text_y,
    "@brief Returns the formatted text for the y-axis label"
  ) +
  gsi::method ("text", (std::string (AnnotationRef::*)() const) &AnnotationRef::text,
    "@brief Returns the formatted text for the main label"
  ) +
  gsi::method ("id", (int (AnnotationRef::*)() const) &AnnotationRef::id,
    "@brief Returns the annotation's ID"
    "\n"
    "The annotation ID is an integer that uniquely identifies an annotation inside a view.\n"
    "The ID is used for replacing an annotation (see \\LayoutView#replace_annotation).\n"
    "\n"
    "This method was introduced in version 0.24."
  ) +
  gsi::method ("is_valid?", &AnnotationRef::is_valid,
    "@brief Returns a value indicating whether the object is a valid reference.\n"
    "If this value is true, the object represents an annotation on the screen. Otherwise, the "
    "object is a 'detached' annotation which does not have a representation on the screen.\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::method ("to_s", &AnnotationRef::to_string,
    "@brief Returns the string representation of the ruler"
    "\n"
    "This method was introduced in version 0.19."
  ) +
  gsi::method ("==", &AnnotationRef::operator==,
    "@brief Equality operator\n"
    "@args other"
  ) +
  gsi::method ("!=", &AnnotationRef::operator!=,
    "@brief Inequality operator\n"
    "@args other"
  ),
  "@brief A layout annotation (i.e. ruler)\n"
  "\n"
  "Annotation objects provide a way to attach measurements or descriptive information to a layout view. "
  "Annotation objects can appear as rulers for example. Annotation objects can be configured in "
  "different ways using the styles provided. By configuring an annotation object properly, it can appear "
  "as a rectangle or a plain line for example.\n"
  "See @<a href=\"/manual/ruler_properties.xml\">Ruler properties@</a> for "
  "more details about the appearance options.\n"
  "\n"
  "Annotations are inserted into a layout view using \\LayoutView#insert_annotation. Here is some sample code "
  "in Ruby:\n"
  "\n"
  "@code\n"
  "app = RBA::Application.instance\n"
  "mw = app.main_window\n"
  "view = mw.current_view\n"
  "\n"
  "ant = RBA::Annotation::new\n"
  "ant.p1 = RBA::DPoint::new(0, 0)\n"
  "ant.p2 = RBA::DPoint::new(100, 0)\n"
  "ant.style = RBA::Annotation::StyleRuler\n"
  "view.insert_annotation(ant)\n"
  "@/code\n"
  "\n"
  "Annotations can be retrieved from a view with \\LayoutView#each_annotation and all "
  "annotations can be cleared with \\LayoutView#clear_annotations.\n"
  "\n"
  "Starting with version 0.25, annotations are 'live' objects once they are inserted into the view. "
  "Changing properties of annotations will automatically update the view (however, that is not true the "
  "other way round).\n"
  "\n"
  "Here is some sample code of changing the style of all rulers to two-sided arrows:\n"
  "\n"
  "@code\n"
  "view = RBA::LayoutView::current\n"
  "\n"
  "begin\n"
  "\n"
  "  view.transaction(\"Restyle annotations\")\n"
  "\n"
  "  view.each_annotation do |a|\n"
  "    a.style = RBA::Annotation::StyleArrowBoth\n"
  "  end\n"
  "  \n"
  "ensure\n"
  "  view.commit\n"
  "end\n"
  "@/code\n"
);

static
gsi::ClassExt<lay::LayoutView> layout_view_decl (
  gsi::method_ext ("clear_annotations", &gsi::clear_annotations, 
    "@brief Clears all annotations on this view"
  ) +
  gsi::method_ext ("insert_annotation", &gsi::insert_annotation, gsi::arg ("obj"),
    "@brief Inserts an annotation object into the given view\n"
    "Inserts a new annotation into the view. Existing annotation will remain. Use \\clear_annotations to "
    "delete them before inserting new ones. Use \\replace_annotation to replace an existing one with a new one. "
    "\n"
    "Starting with version 0.25 this method modifies self's ID to reflect the ID of the ruler created. "
    "After an annotation is inserted into the view, it can be modified and the changes of properties will become "
    "reflected immediately in the view."
  ) +
  gsi::method_ext ("erase_annotation", &gsi::erase_annotation, gsi::arg ("id"),
    "@brief Erases the annotation given by the id\n"
    "Deletes an existing annotation given by the id parameter. The id of an annotation "
    "can be obtained through \\Annotation#id.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
    "Starting with version 0.25, the annotation's \\Annotation#delete method can also be used to delete an annotation."
  ) +
  gsi::method_ext ("replace_annotation", &gsi::replace_annotation, gsi::arg ("id"), gsi::arg ("obj"),
    "@brief Replaces the annotation given by the id with the new one\n"
    "Replaces an existing annotation given by the id parameter with the new one. The id of an annotation "
    "can be obtained through \\Annotation#id.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +
  gsi::method_ext ("annotation", &gsi::get_annotation, gsi::arg ("id"),
    "@brief Gets the annotation given by an ID\n"
    "Returns a reference to the annotation given by the respective ID or an invalid annotation if the ID is not valid.\n"
    "Use \\Annotation#is_valid? to determine whether the returned annotation is valid or not.\n"
    "\n"
    "The returned annotation is a 'live' object and changing it will update the view.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::event_ext ("on_annotations_changed", &get_annotations_changed_event,
    "@brief A event indicating that annotations have been added or removed\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::event_ext ("on_annotation_selection_changed", &get_annotation_selection_changed_event,
    "@brief A event indicating that the annotation selection has changed\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::event_ext ("on_annotation_changed", &get_annotation_changed_event, gsi::arg ("id"),
    "@brief A event indicating that an annotation has been modified\n"
    "The argument of the event is the ID of the annotation that was changed.\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::iterator_ext ("each_annotation", &gsi::begin_annotations,
    "@brief Iterates over all annotations attached to this view"
  ),
  ""
);

class AnnotationSelectionIterator 
{
public:
  typedef AnnotationRef value_type;
  typedef std::map<ant::Service::obj_iterator, unsigned int>::const_iterator iterator_type;
  typedef void pointer; 
  typedef const value_type &reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  AnnotationSelectionIterator (const std::vector<ant::Service *> &services) 
    : m_services (services), m_service (0)
  {
    if (! m_services.empty ()) {
      m_iter = m_services [m_service]->selection ().begin ();
      next ();
    }
  }

  bool at_end () const
  {
    return (m_service >= m_services.size ());
  }

  AnnotationSelectionIterator &operator++ ()
  {
    ++m_iter;
    next ();
    return *this;
  }

  value_type operator* () const
  {
    return value_type (*(static_cast<const ant::Object *> (m_iter->first->ptr ())), m_services[m_service]->view ());
  }

private:
  std::vector<ant::Service *> m_services;
  unsigned int m_service;
  iterator_type m_iter;

  void next ()
  {
    while (m_iter == m_services [m_service]->selection ().end ()) {
      ++m_service;
      if (m_service < m_services.size ()) {
        m_iter = m_services [m_service]->selection ().begin ();
      } else {
        break;
      }
    }
  }
};

//  extend the layout view by "edtService" specific methods 

static bool has_annotation_selection (const lay::LayoutView *view)
{
  std::vector<ant::Service *> ant_services = view->get_plugins <ant::Service> ();
  for (std::vector<ant::Service *>::const_iterator s = ant_services.begin (); s != ant_services.end (); ++s) {
    if ((*s)->selection_size () > 0) {
      return true;
    }
  }
  return false;
}

static AnnotationSelectionIterator begin_annotations_selected (const lay::LayoutView *view)
{
  return AnnotationSelectionIterator (view->get_plugins <ant::Service> ());
}


static
gsi::ClassExt<lay::LayoutView> layout_view_decl2 (
  gsi::method_ext ("has_annotation_selection?", &has_annotation_selection, 
    "@brief Returns true, if annotations (rulers) are selected in this view"
    "\n"
    "This method was introduced in version 0.19."
  ) +
  gsi::iterator_ext ("each_annotation_selected", &begin_annotations_selected,
    "@brief Iterate over each selected annotation objects, yielding a \\Annotation object for each of them"
    "\n"
    "This method was introduced in version 0.19."
  ),
  ""
);

}

