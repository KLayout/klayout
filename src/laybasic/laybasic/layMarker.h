
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



#ifndef HDR_layMarker
#define HDR_layMarker

#include "laybasicCommon.h"

#include "layViewObject.h"
#include "layViewOp.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "dbInstances.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbPolygon.h"
#include "dbText.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbArray.h"
#include "gsi.h"

namespace lay
{

class LayoutViewBase;

/**
 *  @brief The marker base class
 *
 *  This base class defines the properties common to all marker objects
 */

class LAYBASIC_PUBLIC MarkerBase
  : public lay::ViewObject
{
public: 
  /** 
   *  @brief The constructor 
   */ 
  MarkerBase (lay::LayoutViewBase *view);

  /**
   *  @brief Get the color by which the marker is drawn
   *
   *  If the color is invalid, the marker is drawn with the canvases foreground color.
   */
  tl::Color get_color () const
  {
    return m_color;
  }

  /**
   *  @brief Set the color by which the marker is drawn
   *
   *  If the color is invalid, the marker is drawn with the canvases foreground color.
   */
  void set_color (tl::Color color);

  /**
   *  @brief Get the color by which the marker's frame is drawn
   *
   *  If the color is invalid, the marker's frame is drawn with the fill color.
   */
  tl::Color get_frame_color () const
  {
    return m_frame_color;
  }

  /**
   *  @brief Set the frame color by which the marker is drawn
   *
   *  If the color is invalid, the marker's frame is drawn with the fill color.
   */
  void set_frame_color (tl::Color color);

  /**
   *  @brief Get the line width with which the marker is drawn
   */
  int get_line_width () const
  {
    return m_line_width;
  }

  /**
   *  @brief Set the line width with which the marker is drawn
   *
   *  The maximum line width is 15 and 13 if a halo is enabled.
   */
  void set_line_width (int lw);

  /**
   *  @brief Get the vertex size with which the marker is drawn
   */
  int get_vertex_size () const
  {
    return m_vertex_size;
  }

  /**
   *  @brief Set the vertex size with which the marker is drawn
   *
   *  The maximum vertex size is 15 and 13 if a halo is enabled.
   */
  void set_vertex_size (int vs);

  /**
   *  @brief Get the vertex shape with which the marker is drawn
   */
  lay::ViewOp::Shape get_vertex_shape () const
  {
    return m_vertex_shape;
  }

  /**
   *  @brief Set the vertex shape with which the marker is drawn
   */
  void set_vertex_shape (lay::ViewOp::Shape);

  /**
   *  @brief Get the frame pattern index for the marker
   */
  int get_frame_pattern () const
  {
    return m_frame_pattern;
  }

  /**
   *  @brief Set the frame pattern index for the marker
   *
   *  The default pattern is 0 (solid)
   */
  void set_frame_pattern (int index);

  /**
   *  @brief Get the stipple pattern index for the marker
   */
  int get_dither_pattern () const
  {
    return m_dither_pattern;
  }

  /**
   *  @brief Set the stipple pattern index for the marker
   *
   *  A pattern index of -1 indicates that no pattern is set.
   */
  void set_dither_pattern (int index);

  /**
   *  @brief Get the line style index for the marker
   */
  int get_line_style () const
  {
    return m_line_style;
  }

  /**
   *  @brief Set the line style index for the marker
   *
   *  A line style index of -1 indicates that no line style is set.
   */
  void set_line_style (int index);

  /**
   *  @brief Get the halo flag
   *
   *  See "set_halo" for a description of the halo flag.
   */
  int get_halo () const
  {
    return m_halo;
  }

  /**
   *  @brief Set the halo flag
   *
   *  If the halo is enabled (1), a 1 pixel halo is drawn around that marker this giving
   *  it a better visibility in front of complex drawing. If disabled (0), no halo is drawn.
   *  A negative value (-1) instructs the marker to use the global setting.
   */
  void set_halo (int halo);

  /**
   *  @brief Gets a value indicating whether text drawing is enabled
   *
   *  If this value is false, texts are never drawn. If true (the default), texts are drawn as usual.
   *  This is specifically useful for disabling cell boxes of instances.
   */
  bool is_text_enabled () const
  {
    return m_text_enabled;
  }

  /**
   *  @brief Sets a value indicating whether text drawing is enabled
   */
  void set_text_enabled (bool en);

  /**
   *  @brief Gets the bounding box
   */
  virtual db::DBox bbox () const = 0;

protected:
  void get_bitmaps (const Viewport &vp, ViewObjectCanvas &canvas, lay::CanvasPlane *&fill, lay::CanvasPlane *&frame, lay::CanvasPlane *&vertex, lay::CanvasPlane *&text);

  lay::LayoutViewBase *view ()
  {
    return mp_view;
  }

private:
  tl::Color m_color;
  tl::Color m_frame_color;
  char m_line_width, m_vertex_size, m_halo;
  bool m_text_enabled;
  lay::ViewOp::Shape m_vertex_shape;
  int m_line_style, m_dither_pattern, m_frame_pattern;
  lay::LayoutViewBase *mp_view;
};

/**
 *  @brief The generic marker object
 *
 *  This marker object is the base for shape and instance markers. 
 *  This object is the base for specialized markers, i.e. instance and shape markers.
 *  The basic functionality is to keep a set of transformations.
 *  certain instance object in the layout. It is given an instance reference
 *  thus pointing to a instance rather than keeping a copy of it.
 */

class LAYBASIC_PUBLIC GenericMarkerBase
  : public MarkerBase
{
public: 
  /** 
   *  @brief The constructor 
   */ 
  GenericMarkerBase (lay::LayoutViewBase *view, unsigned int cv_index);

  /**
   *  @brief The destructor
   */
  ~GenericMarkerBase ();

  /**
   *  @brief Set the transformation
   *  The transformation is applied in database unit space.
   */
  void set (const db::ICplxTrans &t1);

  /**
   *  @brief Set the transformation
   *  The transformation is applied in database unit space.
   */
  void set (const db::DCplxTrans &t1);

  /**
   *  @brief Set a transformation and a vector of relative transformations
   *  The transformations from the trans vector are applied in micron unit space after the global
   *  transformation (t1) and after application of the database unit.
   *  t1 is given in database unit space.
   */
  void set (const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set a transformation and a vector of relative transformations
   *  The transformations from the trans vector are applied in micron unit space after the global
   *  transformation (t1) and after application of the database unit.
   *  t1 is given in database unit space.
   */
  void set (const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Gets the global transformation
   *  This will be the first part of the transformation including the DBU scaling.
   */
  const db::CplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Sets the global transformation
   *  This method sets the transformation including the DBU scaling.
   */
  void set_trans (const db::CplxTrans &trans);

  /**
   *  @brief Obtain the transformation vector (0 if none is set)
   */
  const std::vector<db::DCplxTrans> *trans_vector () const
  {
    return mp_trans_vector;
  }

  /**
   *  @brief Get the cellview index that this marker refers to
   */
  unsigned int cv_index () const
  {
    return m_cv_index;
  }

  /**
   *  @brief Gets the view object
   */
  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  /**
   *  @brief Gets the bounding box
   */
  db::DBox bbox () const;

  /**
   *  @brief Gets the database unit
   */
  double dbu () const;

  /**
   *  @brief Gets the layout object
   */
  const db::Layout *layout () const;

private:
  db::CplxTrans m_trans;
  std::vector<db::DCplxTrans> *mp_trans_vector;
  lay::LayoutViewBase *mp_view;
  unsigned int m_cv_index;

  /**
   *  @brief Gets the item's box
   */
  virtual db::DBox item_bbox () const = 0;
};

/**
 *  @brief The shape marker object
 *
 *  The marker is a visual object that "marks" (highlights) a 
 *  certain shape object in the layout.
 */

class LAYBASIC_PUBLIC ShapeMarker
  : public GenericMarkerBase
{
public: 
  /** 
   *  @brief The constructor 
   */ 
  ShapeMarker (lay::LayoutViewBase *view, unsigned int cv_index);

  /**
   *  @brief The destructor
   */
  ~ShapeMarker ();

  /**
   *  @brief Set the shape the marker is to display
   *
   *  The marker just stores the shape which is a proxy. The actual object
   *  must be stored somewhere else.
   */
  void set (const db::Shape &shape, const db::ICplxTrans &trans);

  /**
   *  @brief Set the shape the marker is to display
   *
   *  The marker just stores the shape which is a proxy. The actual object
   *  must be stored somewhere else.
   */
  void set (const db::Shape &shape, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Gets the shape
   */
  const db::Shape &shape () const
  {
    return m_shape;
  }

private:
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas);

  virtual db::DBox item_bbox () const;

  db::Shape m_shape; 
};

/**
 *  @brief The instance marker object
 *
 *  The marker is a visual object that "marks" (highlights) a 
 *  certain instance object in the layout. It is given an instance reference
 *  thus pointing to a instance rather than keeping a copy of it.
 */

class LAYBASIC_PUBLIC InstanceMarker
  : public GenericMarkerBase
{
public: 
  /** 
   *  @brief The constructor 
   *
   *  @param view The view the marker is intended for
   *  @param cv_index The cell view index of the layout that this marker is intended for
   *  @param draw_outline True to have instances drawing their outline
   *  @param max_shapes The maximum number of shapes to draw for instances (just a box is drawn if more shapes are present)
   */ 
  InstanceMarker (lay::LayoutViewBase *view, unsigned int cv_index, bool draw_outline = true, size_t max_shapes = 0);

  /**
   *  @brief The destructor
   */
  ~InstanceMarker ();

  /**
   *  @brief Gets the instance
   */
  const db::Instance &instance () const
  {
    return m_inst;
  }

  /**
   *  @brief Set the instance the marker is to display
   *
   *  The marker just stores the instance which is a proxy. The actual object
   *  must be stored somewhere else.
   */
  void set (const db::Instance &inst, const db::ICplxTrans &trans);

  /**
   *  @brief Set the instance the marker is to display
   *
   *  The marker just stores the instance which is a proxy. The actual object
   *  must be stored somewhere else.
   */
  void set (const db::Instance &inst, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the outline drawing flag (for instances)
   */
  void set_draw_outline (bool d);

  /**
   *  @brief Get The outline drawing flag
   */
  bool draw_outline () const
  {
    return m_draw_outline;
  }

  /**
   *  @brief Set the maximum number of shapes to draw to instances
   */
  void set_max_shapes (size_t s);

  /**
   *  @brief Get the maximum number of shapes to draw to instances
   */
  size_t max_shapes () const
  {
    return m_max_shapes;
  }

private:
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas);

  virtual db::DBox item_bbox () const;

  bool m_draw_outline;
  size_t m_max_shapes;
  db::Instance m_inst; 
};

/**
 *  @brief The marker object
 *
 *  The marker is a visual object that "marks" (highlights) a 
 *  certain area of the layout.
 *
 *  The objects can be given as either integer or float types.
 *  Both the integer and the float types are given in database units.
 *  A transformation can be specified that will be applied to the object
 *  This transformation (t1) is given in database units.
 *  Ultimately a vector of transformations can be given that is
 *  applied to the object multiple times. This transformation vector
 *  is to be given in micron units and will usually be taken from the
 *  view.
 *
 *  The DMarker object is a marker object supporting pure micron-unit
 *  markers.
 */

class LAYBASIC_PUBLIC Marker
  : public GenericMarkerBase
{
public: 
  /** 
   *  The constructor
   *
   *  @param view The view the marker is intended for
   *  @param cv_index The cell view index of the layout that this marker is intended for
   *  @param draw_outline True to have instances drawing their outline
   *  @param max_shapes The maximum number of shapes to draw for instances (just a box is drawn if more shapes are present)
   */ 
  Marker (lay::LayoutViewBase *view, unsigned int cv_index, bool draw_outline = true, size_t max_shapes = 0);

  /**
   *  @brief The destructor
   */
  ~Marker ();

  /**
   *  @brief Set the marker to display nothing.
   */
  void set ();

  /**
   *  @brief Set the box the marker is to display
   */
  void set (const db::Box &box, const db::ICplxTrans &t1);

  /**
   *  @brief Set the box the marker is to display
   */
  void set (const db::Box &box, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point box the marker is to display
   */
  void set (const db::DBox &box, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point box the marker is to display
   */
  void set (const db::DBox &box, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the polygon the marker is to display
   */
  void set (const db::Polygon &poly, const db::ICplxTrans &t1);

  /**
   *  @brief Set the polygon the marker is to display
   */
  void set (const db::Polygon &poly, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point polygon the marker is to display
   */
  void set (const db::DPolygon &poly, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point polygon the marker is to display
   */
  void set (const db::DPolygon &poly, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the polygon reference the marker is to display
   */
  void set (const db::PolygonRef &poly_ref, const db::ICplxTrans &t1);

  /**
   *  @brief Set the polygon reference the marker is to display
   */
  void set (const db::PolygonRef &poly_ref, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the edge pair the marker is to display
   */
  void set (const db::EdgePair &edge_pair, const db::ICplxTrans &t1);

  /**
   *  @brief Set the edge pair the marker is to display
   */
  void set (const db::EdgePair &edge_pair, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point edge pair the marker is to display
   */
  void set (const db::DEdgePair &edge_pair, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point edge pair the marker is to display
   */
  void set (const db::DEdgePair &edge_pair, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the edge the marker is to display
   */
  void set (const db::Edge &edge, const db::ICplxTrans &t1);

  /**
   *  @brief Set the edge the marker is to display
   */
  void set (const db::Edge &edge, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point edge the marker is to display
   */
  void set (const db::DEdge &edge, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point edge the marker is to display
   */
  void set (const db::DEdge &edge, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the path the marker is to display
   */
  void set (const db::Path &path, const db::ICplxTrans &t1);

  /**
   *  @brief Set the path the marker is to display
   */
  void set (const db::Path &path, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point path the marker is to display
   */
  void set (const db::DPath &path, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point path the marker is to display
   *  
   *  This variant draws the path multiple times at the given transformations.
   */
  void set (const db::DPath &path, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the text the marker is to display
   */
  void set (const db::Text &text, const db::ICplxTrans &t1);

  /**
   *  @brief Set the text the marker is to display
   *  
   *  This variant draws the text multiple times at the given transformations.
   */
  void set (const db::Text &text, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the floating-point text the marker is to display
   */
  void set (const db::DText &text, const db::DCplxTrans &t1);

  /**
   *  @brief Set the floating-point text the marker is to display
   *  
   *  This variant draws the text multiple times at the given transformations.
   */
  void set (const db::DText &text, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the instance the marker is to display
   */
  void set (const db::CellInstArray &instance, const db::ICplxTrans &t1);

  /**
   *  @brief Set the instance the marker is to display
   */
  void set (const db::CellInstArray &instance, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans);

  /**
   *  @brief Set the outline drawing flag (for instances)
   */
  void set_draw_outline (bool d);

  /**
   *  @brief Get The outline drawing flag
   */
  bool draw_outline () const
  {
    return m_draw_outline;
  }

  /**
   *  @brief Set the maximum number of shapes to draw to instances
   */
  void set_max_shapes (size_t s);

  /**
   *  @brief Get the maximum number of shapes to draw to instances
   */
  size_t max_shapes () const
  {
    return m_max_shapes;
  }

protected:
  virtual db::DBox item_bbox () const;
  
private:
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas);

  void draw (lay::Renderer &r, const db::CplxTrans &t, lay::CanvasPlane *fill, lay::CanvasPlane *contour, lay::CanvasPlane *vertex, lay::CanvasPlane *text);
  void remove_object ();

  bool m_draw_outline;
  size_t m_max_shapes;

  enum { 
    None, Box, DBox, Polygon, PolygonRef, DPolygon, EdgePair, DEdgePair, Edge, DEdge, Path, DPath, Text, DText, Instance
  } m_type;

  union {
    db::Box *box;
    db::DBox *dbox;
    db::Polygon *polygon;
    db::DPolygon *dpolygon;
    db::PolygonRef *polygon_ref;
    db::EdgePair *edge_pair;
    db::DEdgePair *dedge_pair;
    db::Edge *edge;
    db::DEdge *dedge;
    db::Path *path;
    db::DPath *dpath;
    db::Text *text;
    db::DText *dtext;
    db::CellInstArray *inst;
    void *any;
  } m_object;
};

/**
 *  @brief The floating-point coordinate marker object
 *
 *  The marker is a visual object that "marks" (highlights) a 
 *  certain area of the layout. In contrast to the "Marker" object, this
 *  object accepts objects with coordinates in floating-point coordinates in micron values.
 *  It does not need a DBU value hence no cellview to display itself.
 */

class LAYBASIC_PUBLIC DMarker
  : public MarkerBase
{
public: 
  /** 
   *  @brief The constructor 
   */ 
  DMarker (lay::LayoutViewBase *view);

  /**
   *  @brief The destructor
   */
  ~DMarker ();

  /**
   *  @brief Set the box the marker is to display
   *
   *  If the box is empty, no marker is drawn.
   */
  void set (const db::DBox &box);

  /**
   *  @brief Set the polygon the marker is to display
   */
  void set (const db::DPolygon &poly);

  /**
   *  @brief Set the edge pair the marker is to display
   */
  void set (const db::DEdgePair &edge_pair);

  /**
   *  @brief Set the edge the marker is to display
   */
  void set (const db::DEdge &edge);

  /**
   *  @brief Set the path the marker is to display
   */
  void set (const db::DPath &path);

  /**
   *  @brief Set the text the marker is to display
   */
  void set (const db::DText &text);

  /**
   *  @brief Gets the bounding box
   */
  virtual db::DBox bbox () const;
  
private:
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas);

  void remove_object ();

  enum { 
    None, Box, Polygon, EdgePair, Edge, Path, Text
  } m_type;

  union {
    db::DBox *box;
    db::DPolygon *polygon;
    db::DEdgePair *edge_pair;
    db::DEdge *edge;
    db::DPath *path;
    db::DText *text;
    void *any;
  } m_object;

  lay::LayoutViewBase *mp_view;
};

}

#endif
