
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


#include "gsiDecl.h"

#include "dbTilingProcessor.h"
#include "dbMatrix.h"

namespace gsi
{

class DoubleCollectingTileOutputReceiver
  : public db::TileOutputReceiver
{
public:
  DoubleCollectingTileOutputReceiver (double *value)
    : mp_value (value)
  {
    //  .. nothing yet ..
  }

  virtual void begin (size_t /*nx*/, size_t /*ny*/, const db::DPoint & /*p0*/, double /*dx*/, double /*dy*/, const db::DBox & /*frame*/)
  {
    if (mp_value) {
      *mp_value = 0.0;
    }
  }

  virtual void put (size_t /*ix*/, size_t /*iy*/, const db::Box & /*tile*/, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool /*clip*/) 
  {
    if (mp_value) {
      *mp_value += obj.to_double ();
    }
  }

private:
  double *mp_value;
};

struct TPEvent
{
  TPEvent (size_t _ix, size_t _iy, const db::Box &_tile, size_t _id, const tl::Variant &_obj, double _dbu, const db::ICplxTrans &_trans, bool _clip) 
  {
    ix = _ix;
    iy = _iy;
    tile = _tile;
    id = _id;
    obj = _obj;
    dbu = _dbu;
    trans = _trans;
    clip = _clip;
  }

  size_t ix, iy;
  db::Box tile;
  size_t id;
  tl::Variant obj;
  double dbu;
  db::ICplxTrans trans;
  bool clip;
};

class TileOutputReceiver_Impl 
  : public db::TileOutputReceiver
{
public:
  TileOutputReceiver_Impl ()
  {
    m_mt_mode = false;
  }

  virtual void begin (size_t nx, size_t ny, const db::DPoint &p0, double dx, double dy, const db::DBox &frame)
  { 
    m_mt_mode = (processor () && processor ()->threads () >= 1);
    m_events.clear ();
    if (begin_cb.can_issue ()) {
      begin_cb.issue<db::TileOutputReceiver, size_t, size_t, const db::DPoint &, double, double, const db::DBox &> (&db::TileOutputReceiver::begin, nx, ny, p0, dx, dy, frame);
    } else {
      db::TileOutputReceiver::begin (nx, ny, p0, dx, dy, frame);
    }
  }

  virtual void put_red (size_t /*ix*/, size_t /*iy*/, const db::Box & /*tile*/, const tl::Variant & /*obj*/, double /*dbu*/, bool /*clip*/) 
  { 
    //  .. nothing yet ..
  }

  virtual void put (size_t ix, size_t iy, const db::Box &tile, size_t id, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip) 
  { 
    if (m_mt_mode) {
      //  store the events so we can later flush them at once in the main thread
      //  (the worker threads are not ruby-initialized, hence we cannot call ruby code from them)
      m_events.push_back (TPEvent (ix, iy, tile, id, obj, dbu, trans, clip));
    } else {
      do_put (ix, iy, tile, id, obj, dbu, trans, clip);
    }
  }

  void do_put (const TPEvent &e)
  {
    do_put(e.ix, e.iy, e.tile, e.id, e.obj, e.dbu, e.trans, e.clip);
  }

  void do_put (size_t ix, size_t iy, const db::Box &tile, size_t id, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip) 
  {
    if (put_cb.can_issue ()) {
      put_cb.issue<TileOutputReceiver_Impl, size_t, size_t, const db::Box &, const tl::Variant &, double, bool> (&TileOutputReceiver_Impl::put_red, ix, iy, tile, obj, dbu, clip);
    } else {
      TileOutputReceiver::put (ix, iy, tile, id, obj, dbu, trans, clip);
    }
  }

  virtual void finish (bool success)
  { 
    //  flush stored events now.
    for (std::vector<TPEvent>::const_iterator e = m_events.begin (); e != m_events.end (); ++e) {
      do_put (*e);
    }
    m_events.clear ();
    if (finish_cb.can_issue ()) {
      finish_cb.issue<db::TileOutputReceiver, bool> (&db::TileOutputReceiver::finish, success);
    } else {
      db::TileOutputReceiver::finish (success);
    }
  }

  gsi::Callback begin_cb;
  gsi::Callback put_cb;
  gsi::Callback finish_cb;

private:
  std::vector<TPEvent> m_events;
  bool m_mt_mode;
};

gsi::Class<db::TileOutputReceiver> decl_TileOutputReceiverBase ("db", "TileOutputReceiverBase",
  gsi::method ("processor", &TileOutputReceiver_Impl::processor,
    "@brief Gets the processor the receiver is attached to\n"
    "\n"
    "This attribute is set before begin and can be nil if the receiver is not attached to "
    "a processor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ),
  "@hide\n@alias TileOutputReceiver"
);

DB_PUBLIC
gsi::Class<db::TileOutputReceiver> &dbdecl_TileOutputReceiverBase ()
{
  return decl_TileOutputReceiverBase;
}

gsi::Class<TileOutputReceiver_Impl> decl_TileOutputReceiver (decl_TileOutputReceiverBase, "db", "TileOutputReceiver",
  gsi::callback ("begin", &TileOutputReceiver_Impl::begin, &TileOutputReceiver_Impl::begin_cb, gsi::arg ("nx"), gsi::arg ("ny"), gsi::arg ("p0"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("frame"),
    "@brief Initiates the delivery\n"
    "This method is called before the first tile delivers its data.\n"
    "\n"
    "@param nx The number of tiles in x direction\n"
    "@param ny The number of tiles in y direction\n"
    "@param p0 The initial point\n"
    "@param dx The tile's x dimension\n"
    "@param dy The tile's y dimension\n"
    "@param frame The overall frame that is the basis of the tiling"
    "\n"
    "The tile's coordinates will be p0+(ix*dx,iy*dy)..p0+((ix+1)*dx,(iy+1)*dy) \n"
    "where ix=0..nx-1, iy=0..ny-1.\n"
    "\n"
    "All coordinates are given in micron. If tiles are not used, nx and ny are 0.\n"
    "\n"
    "The frame parameter has been added in version 0.25."
  ) +
  gsi::callback ("put", &TileOutputReceiver_Impl::put_red, &TileOutputReceiver_Impl::put_cb, gsi::arg ("ix"), gsi::arg ("iy"), gsi::arg ("tile"), gsi::arg ("obj"), gsi::arg ("dbu"), gsi::arg ("clip"),
    "@brief Delivers data for one tile\n"
    "\n"
    "When the script's \"_output\" function is called, the data will be delivered through this\n"
    "method. \"obj\" is the data passed as the second argument to _output.\n"
    "The interpretation of the object remains subject to the implementation.\n"
    "\n"
    "The obj and clip parameters are taken from the _output method call inside the script.\n"
    "If clip is set to true, this usually means that output shall be clipped to the tile.\n"
    "\n"
    "@param ix The x index of the tile\n"
    "@param iy The y index of the tile\n"
    "@param tile The tile's box\n"
    "@param obj The object which is delivered\n"
    "@param dbu The database unit\n"
    "@param clip True if clipping at the tile box is requested\n"
  ) +
  gsi::callback ("finish", &TileOutputReceiver_Impl::finish, &TileOutputReceiver_Impl::finish_cb, gsi::arg ("success"),
    "@brief Indicates the end of the execution\n"
    "\n"
    "This method is called when the tiling processor has finished the last tile and script item.\n"
    "The success flag is set to true, if every tile has finished successfully. Otherwise, this value is false.\n"
    "\n"
    "The success flag has been added in version 0.25."
  ),
  "@brief A receiver abstraction for the tiling processor.\n"
  "\n"
  "The tiling processor (\\TilingProcessor) is a framework for executing sequences of operations "
  "on tiles of a layout or multiple layouts. The \\TileOutputReceiver class is used to specify an "
  "output channel for the tiling processor. See \\TilingProcessor#output for more details.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

static void tp_output (db::TilingProcessor *proc, const std::string &name, db::TileOutputReceiver *rec)
{
  rec->gsi::ObjectBase::keep ();
  proc->output (name, 0, rec, db::ICplxTrans ());
}

static void tp_output_layout1 (db::TilingProcessor *proc, const std::string &name, db::Layout &layout, db::cell_index_type cell, const db::LayerProperties &lp)
{
  proc->output (name, layout, cell, lp);
}

static void tp_output_layout2 (db::TilingProcessor *proc, const std::string &name, db::Layout &layout, db::cell_index_type cell, const unsigned int layer_index)
{
  proc->output (name, layout, cell, layer_index);
}

static void tp_output_region (db::TilingProcessor *proc, const std::string &name, db::Region &region)
{
  proc->output (name, region);
}

static void tp_output_edges (db::TilingProcessor *proc, const std::string &name, db::Edges &edges)
{
  proc->output (name, edges);
}

static void tp_output_edge_pairs (db::TilingProcessor *proc, const std::string &name, db::EdgePairs &edge_pairs)
{
  proc->output (name, edge_pairs);
}

static void tp_output_texts (db::TilingProcessor *proc, const std::string &name, db::Texts &texts)
{
  proc->output (name, texts);
}

static void tp_output_double (db::TilingProcessor *proc, const std::string &name, double *v)
{
  proc->output (name, 0, new DoubleCollectingTileOutputReceiver (v), db::ICplxTrans ());
}

static void tp_input2 (db::TilingProcessor *proc, const std::string &name, const db::RecursiveShapeIterator &iter)
{
  proc->input (name, iter);
}

static void tp_input3 (db::TilingProcessor *proc, const std::string &name, const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
{
  proc->input (name, iter, trans);
}

static void tp_input4 (db::TilingProcessor *proc, const std::string &name, const db::Layout &layout, db::cell_index_type ci, const db::LayerProperties &lp)
{
  if (! lp.is_null ()) {
    //  if we have a layer with the requested properties already, return this.
    for (db::Layout::layer_iterator li = layout.begin_layers (); li != layout.end_layers (); ++li) {
      if ((*li).second->log_equal (lp)) {
        proc->input (name, db::RecursiveShapeIterator (layout, layout.cell (ci), (*li).first));
        return;
      }
    }
  }
  proc->input (name, db::RecursiveShapeIterator ());
}

static void tp_input5 (db::TilingProcessor *proc, const std::string &name, const db::Layout &layout, db::cell_index_type ci, unsigned int li)
{
  proc->input (name, db::RecursiveShapeIterator (layout, layout.cell (ci), li));
}

static void tp_input6 (db::TilingProcessor *proc, const std::string &name, const db::Layout &layout, db::cell_index_type ci, const db::LayerProperties &lp, const db::ICplxTrans &trans)
{
  if (! lp.is_null ()) {
    //  if we have a layer with the requested properties already, return this.
    for (db::Layout::layer_iterator li = layout.begin_layers (); li != layout.end_layers (); ++li) {
      if ((*li).second->log_equal (lp)) {
        proc->input (name, db::RecursiveShapeIterator (layout, layout.cell (ci), (*li).first), trans);
        return;
      }
    }
  }
  proc->input (name, db::RecursiveShapeIterator (), trans);
}

static void tp_input7 (db::TilingProcessor *proc, const std::string &name, const db::Layout &layout, db::cell_index_type ci, unsigned int li, const db::ICplxTrans &trans)
{
  proc->input (name, db::RecursiveShapeIterator (layout, layout.cell (ci), li), trans);
}

static void tp_input8 (db::TilingProcessor *proc, const std::string &name, const db::Region &region)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = region.begin_iter ();
  proc->input (name, it.first, it.second, db::TilingProcessor::TypeRegion, region.merged_semantics ());
}

static void tp_input9 (db::TilingProcessor *proc, const std::string &name, const db::Region &region, const db::ICplxTrans &trans)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = region.begin_iter ();
  proc->input (name, it.first, trans * it.second, db::TilingProcessor::TypeRegion, region.merged_semantics ());
}

static void tp_input10 (db::TilingProcessor *proc, const std::string &name, const db::Edges &edges)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edges.begin_iter ();
  proc->input (name, it.first, it.second, db::TilingProcessor::TypeEdges, edges.merged_semantics ());
}

static void tp_input11 (db::TilingProcessor *proc, const std::string &name, const db::Edges &edges, const db::ICplxTrans &trans)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edges.begin_iter ();
  proc->input (name, it.first, trans * it.second, db::TilingProcessor::TypeEdges, edges.merged_semantics ());
}

static void tp_input12 (db::TilingProcessor *proc, const std::string &name, const db::EdgePairs &edge_pairs)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edge_pairs.begin_iter ();
  proc->input (name, it.first, it.second, db::TilingProcessor::TypeEdgePairs);
}

static void tp_input13 (db::TilingProcessor *proc, const std::string &name, const db::EdgePairs &edge_pairs, const db::ICplxTrans &trans)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edge_pairs.begin_iter ();
  proc->input (name, it.first, trans * it.second, db::TilingProcessor::TypeEdgePairs);
}

static void tp_input14 (db::TilingProcessor *proc, const std::string &name, const db::Texts &texts)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = texts.begin_iter ();
  proc->input (name, it.first, it.second, db::TilingProcessor::TypeTexts);
}

static void tp_input15 (db::TilingProcessor *proc, const std::string &name, const db::Texts &texts, const db::ICplxTrans &trans)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = texts.begin_iter ();
  proc->input (name, it.first, trans * it.second, db::TilingProcessor::TypeTexts);
}

Class<db::TilingProcessor> decl_TilingProcessor ("db", "TilingProcessor",
  method_ext ("input", &tp_input2, gsi::arg ("name"), gsi::arg ("iter"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "recursive shape iterator, hence from a hierarchy of shapes from a layout.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input3, gsi::arg ("name"), gsi::arg ("iter"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "recursive shape iterator, hence from a hierarchy of shapes from a layout.\n"
    "In addition, a transformation can be specified which will be applied to the shapes before they are used.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input4, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("lp"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "layout and the hierarchy below the cell with the given cell index.\n"
    "\"lp\" is a \\LayerInfo object specifying the input layer.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input5, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "layout and the hierarchy below the cell with the given cell index.\n"
    "\"layer\" is the layer index of the input layer.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input6, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("lp"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "layout and the hierarchy below the cell with the given cell index.\n"
    "\"lp\" is a \\LayerInfo object specifying the input layer.\n"
    "In addition, a transformation can be specified which will be applied to the shapes before they are used.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input7, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a "
    "layout and the hierarchy below the cell with the given cell index.\n"
    "\"layer\" is the layer index of the input layer.\n"
    "In addition, a transformation can be specified which will be applied to the shapes before they are used.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input8, gsi::arg ("name"), gsi::arg ("region"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a \\Region object. "
    "Regions don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Region object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Region object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input9, gsi::arg ("name"), gsi::arg ("region"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from a \\Region object. "
    "Regions don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Region object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Region object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant allows one to specify an additional transformation too. It has been introduced in version 0.23.2.\n"
  ) + 
  method_ext ("input", &tp_input10, gsi::arg ("name"), gsi::arg ("edges"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\Edges object. "
    "Edge collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Edges object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Edges object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
  ) + 
  method_ext ("input", &tp_input11, gsi::arg ("name"), gsi::arg ("edges"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\Edges object. "
    "Edge collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Edges object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Edges object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant allows one to specify an additional transformation too. It has been introduced in version 0.23.2.\n"
    "\n"
  ) + 
  method_ext ("input", &tp_input12, gsi::arg ("name"), gsi::arg ("edge_pairs"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\EdgePairs object. "
    "Edge pair collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the EdgePairs object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the EdgePairs object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method_ext ("input", &tp_input13, gsi::arg ("name"), gsi::arg ("edge_pairs"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\EdgePairs object. "
    "Edge pair collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the EdgePairs object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the EdgePairs object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method_ext ("input", &tp_input14, gsi::arg ("name"), gsi::arg ("texts"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\Texts object. "
    "Text collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Texts object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Texts object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method_ext ("input", &tp_input15, gsi::arg ("name"), gsi::arg ("texts"), gsi::arg ("trans"),
    "@brief Specifies input for the tiling processor\n"
    "This method will establish an input channel for the processor. This version receives input from an \\Texts object. "
    "Text collections don't always come with a database unit, hence a database unit should be specified with the \\dbu= method unless "
    "a layout object is specified as input too.\n"
    "\n"
    "Caution: the Texts object must stay valid during the lifetime of the tiling processor. Take care to store it in "
    "a variable to prevent early destruction of the Texts object. Not doing so may crash the application.\n"
    "\n"
    "The name specifies the variable under which the input can be used in the scripts."
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  method ("var", &db::TilingProcessor::var, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Defines a variable for the tiling processor script\n"
    "\n"
    "The name specifies the variable under which the value can be used in the scripts."
  ) + 
  method_ext ("output", &tp_output, gsi::arg ("name"), gsi::arg ("rec"),
    "@brief Specifies output for the tiling processor\n"
    "This method will establish an output channel for the processor. For that it registers an output receiver "
    "which will receive data from the scripts. The scripts call the _output function to deliver data.\n"
    "\"name\" will be name of the variable which must be passed to the first argument of the _output function "
    "in order to address this channel.\n"
    "\n"
    "Please note that the tiling processor will destroy the receiver object when it is freed itself. Hence "
    "if you need to address the receiver object later, make sure that the processor is still alive, i.e. by "
    "assigning the object to a variable.\n"
    "\n"
    "The following code uses the output receiver. It takes the shapes of a layer from a layout, "
    "computes the area of each tile and outputs the area to the custom receiver:\n"
    "\n"
    "@code\n"
    "layout = ... # the layout\n"
    "cell = ... # the top cell's index\n"
    "layout = ... # the input layer\n"
    "\n"
    "class MyReceiver < RBA::TileOutputReceiver\n"
    "  def put(ix, iy, tile, obj, dbu, clip)\n"
    "    puts \"got area for tile #{ix+1},#{iy+1}: #{obj.to_s}\"\n"
    "  end\n"
    "end\n"
    "\n"
    "tp = RBA::TilingProcessor::new\n"
    "\n"
    "# register the custom receiver\n"
    "tp.output(\"my_receiver\", MyReceiver::new)\n"
    "tp.input(\"the_input\", layout.begin_shapes(cell, layer))\n"
    "tp.tile_size(100, 100)  # 100x100 um tile size\n"
    "# The script clips the input at the tile and computes the (merged) area:\n"
    "tp.queue(\"_output(my_receiver, (the_input & _tile).area)\")\n"
    "tp.execute(\"Job description\")\n"
    "@/code\n"
  ) + 
  method_ext ("output", &tp_output_layout1, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("lp"),
    "@brief Specifies output to a layout layer\n"
    "This method will establish an output channel to a layer in a layout. The output sent to that channel "
    "will be put into the specified layer and cell. In this version, the layer is specified through a \\LayerInfo "
    "object, i.e. layer and datatype number. If no such layer exists, it will be created.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param layout The layout to which the data is sent\n"
    "@param cell The index of the cell to which the data is sent\n"
    "@param lp The layer specification where the output will be sent to\n"
  ) +
  method_ext ("output", &tp_output_layout2, gsi::arg ("name"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer_index"),
    "@brief Specifies output to a layout layer\n"
    "This method will establish an output channel to a layer in a layout. The output sent to that channel "
    "will be put into the specified layer and cell. In this version, the layer is specified through a layer index, "
    "hence it must be created before.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param layout The layout to which the data is sent\n"
    "@param cell The index of the cell to which the data is sent\n"
    "@param layer_index The layer index where the output will be sent to\n"
  ) +
  method_ext ("output", &tp_output_region, gsi::arg ("name"), gsi::arg ("region"),
    "@brief Specifies output to a \\Region object\n"
    "This method will establish an output channel to a \\Region object. The output sent to that channel "
    "will be put into the specified region.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "Edges sent to this channel are discarded. Edge pairs are converted to polygons.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param region The \\Region object to which the data is sent\n"
  ) +
  method_ext ("output", &tp_output_edges, gsi::arg ("name"), gsi::arg ("edges"),
    "@brief Specifies output to an \\Edges object\n"
    "This method will establish an output channel to an \\Edges object. The output sent to that channel "
    "will be put into the specified edge collection.\n"
    "'Solid' objects such as polygons will be converted to edges by resolving their hulls into edges. "
    "Edge pairs are resolved into single edges.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param edges The \\Edges object to which the data is sent\n"
  ) +
  method_ext ("output", &tp_output_edge_pairs, gsi::arg ("name"), gsi::arg ("edge_pairs"),
    "@brief Specifies output to an \\EdgePairs object\n"
    "This method will establish an output channel to an \\EdgePairs object. The output sent to that channel "
    "will be put into the specified edge pair collection.\n"
    "Only \\EdgePair objects are accepted. Other objects are discarded.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param edge_pairs The \\EdgePairs object to which the data is sent\n"
  ) +
  method_ext ("output", &tp_output_texts, gsi::arg ("name"), gsi::arg ("texts"),
    "@brief Specifies output to an \\Texts object\n"
    "This method will establish an output channel to an \\Texts object. The output sent to that channel "
    "will be put into the specified edge pair collection.\n"
    "Only \\Text objects are accepted. Other objects are discarded.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
    "\n"
    "@param name The name of the channel\n"
    "@param texts The \\Texts object to which the data is sent\n"
    "\n"
    "This variant has been introduced in version 0.27."
  ) +
  method_ext ("output", &tp_output_double, gsi::arg ("name"), gsi::arg ("sum"),
    "@brief Specifies output to single value\n"
    "This method will establish an output channel which sums up float data delivered by calling the _output function.\n"
    "In order to specify the target for the data, a \\Value object must be provided for the \"sum\" parameter.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
  ) +
  method ("scale_to_dbu?", &db::TilingProcessor::scale_to_dbu,
    "@brief Gets a valid indicating whether automatic scaling to database unit is enabled\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) + 
  method ("scale_to_dbu=", &db::TilingProcessor::set_scale_to_dbu, gsi::arg ("en"),
    "@brief Enables or disabled automatic scaling to database unit\n"
    "\n"
    "If automatic scaling to database unit is enabled, the input is automatically scaled to the "
    "database unit set inside the tile processor. This is the default.\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) + 
  method ("dbu", &db::TilingProcessor::dbu,
    "@brief Gets the database unit under which the computations will be done\n"
  ) + 
  method ("dbu=", &db::TilingProcessor::set_dbu, gsi::arg ("u"),
    "@brief Sets the database unit under which the computations will be done\n"
    "\n"
    "All data used within the scripts will be brought to that database unit. If none is given "
    "it will be the database unit of the first layout given or 1nm if no layout is specified.\n"
  ) + 
  method ("frame=", &db::TilingProcessor::set_frame, gsi::arg ("frame"),
    "@brief Sets the layout frame\n"
    "\n"
    "The layout frame is the box (in micron units) taken into account for computing\n"
    "the tiles if the tile counts are not given. If the layout frame is not set or\n"
    "set to an empty box, the processor will try to derive the frame from the given\n"
    "inputs.\n"
    "\n"
    "This method has been introduced in version 0.25."
 ) +
  method ("tile_size", &db::TilingProcessor::tile_size, gsi::arg ("w"), gsi::arg ("h"),
    "@brief Sets the tile size\n"
    "\n"
    "Specifies the size of the tiles to be used. If no tile size is specified, tiling won't be used "
    "and all computations will be done on the whole layout.\n"
    "\n"
    "The tile size is given in micron.\n"
  ) + 
  method ("tiles", &db::TilingProcessor::tiles, gsi::arg ("nw"), gsi::arg ("nh"),
    "@brief Sets the tile count\n"
    "\n"
    "Specifies the number of tiles to be used. If no tile number is specified, the number of tiles "
    "required is computed from the layout's dimensions and the tile size. If a number is given, but "
    "no tile size, the tile size will be computed from the layout's dimensions.\n"
  ) + 
  method ("tile_origin", &db::TilingProcessor::tile_origin, gsi::arg ("xo"), gsi::arg ("yo"),
    "@brief Sets the tile origin\n"
    "\n"
    "Specifies the origin (lower left corner) of the tile field. If no origin is specified, the "
    "tiles are centered to the layout's bounding box. Giving the origin together with the "
    "tile count and dimensions gives full control over the tile array.\n"
    "\n"
    "The tile origin is given in micron.\n"
  ) + 
  method ("tile_border", &db::TilingProcessor::tile_border, gsi::arg ("bx"), gsi::arg ("by"),
    "@brief Sets the tile border\n"
    "\n"
    "Specifies the tile border. The border is a margin that is considered when fetching shapes. "
    "By specifying a border you can fetch shapes into the tile's data which are outside the "
    "tile but still must be considered in the computations (i.e. because they might grow into the tile).\n"
    "\n"
    "The tile border is given in micron.\n"
  ) + 
  method ("threads=", &db::TilingProcessor::set_threads, gsi::arg ("n"),
    "@brief Specifies the number of threads to use\n"
  ) + 
  method ("threads", &db::TilingProcessor::threads,
    "@brief Gets the number of threads to use\n"
  ) + 
  method ("queue", &db::TilingProcessor::queue, gsi::arg ("script"),
    "@brief Queues a script for parallel execution\n"
    "\n"
    "With this method, scripts are registered that are executed in parallel on each tile.\n"
    "The scripts have \"Expressions\" syntax and can make use of several predefined variables and functions.\n"
    "See the \\TilingProcessor class description for details.\n"
  ) + 
  method ("execute", &db::TilingProcessor::execute, gsi::arg ("desc"),
    "@brief Runs the job\n"
    "\n"
    "This method will initiate execution of the queued scripts, once for every tile. The desc is a text "
    "shown in the progress bar for example.\n"
  ),
  "@brief A processor for layout which distributes tasks over tiles\n"
  "\n"
  "The tiling processor executes one or several scripts on one or multiple layouts providing "
  "a tiling scheme. In that scheme, the processor divides the original layout into rectangular tiles "
  "and executes the scripts on each tile separately. The tiling processor allows one to specify multiple, "
  "independent scripts which are run separately on each tile. It can make use of multi-core CPU's by "
  "supporting multiple threads running the tasks in parallel (with respect to tiles and scripts).\n"
  "\n"
  "Tiling a optional - if no tiles are specified, the tiling processing basically operates flat and "
  "parallelization extends to the scripts only.\n"
  "\n"
  "Tiles can be overlapping to gather input from neighboring tiles into the current tile. "
  "In order to provide that feature, a border can be specified which gives the amount by which "
  "the search region is extended beyond the border of the tile. To specify the border, use the "
  "\\TilingProcessor#tile_border method.\n"
  "\n"
  "The basis of the tiling processor are \\Region objects and expressions. Expressions are a built-in "
  "simple language to form simple scripts. Expressions allow access to the objects and methods built "
  "into KLayout. Each script can consist of multiple operations. Scripts are specified using \\TilingProcessor#queue.\n"
  "\n"
  "Input is provided to the script through "
  "variables holding a \\Region object each. From outside the tiling processor, input is specified "
  "with the \\TilingProcessor#input method. This method is given a name and a \\RecursiveShapeIterator object "
  "which delivers the data for the input. On the script side, a \\Region object is provided through a variable "
  "named like the first argument of the \"input\" method.\n"
  "\n"
  "Inside the script the following functions are provided:\n"
  "\n"
  "@ul\n"
  "@li\"_dbu\" delivers the database unit used for the computations @/li\n"
  "@li\"_tile\" delivers a region containing a mask for the tile (a rectangle) or nil if no tiling is used @/li\n"
  "@li\"_output\" is used to deliver output (see below) @/li\n"
  "@/ul\n"
  "\n"
  "Output can be obtained from the tiling processor by registering a receiver with a channel. A channel is basically "
  "a name. Inside the script, the name describes a variable which can be used as the first argument of the "
  "\"_output\" function to identify the channel. A channel is registers using the \\TilingProcessor#output method. "
  "Beside the name, a receiver must be specified. A receiver is either another layout (a cell of that), a report database "
  "or a custom receiver implemented through the \\TileOutputReceiver class.\n"
  "\n"
  "The \"_output\" function expects two or three parameters: one channel id (the variable that was defined by the name "
  "given in the output method call) and an object to output (a \\Region, \\Edges, \\EdgePairs or a geometrical primitive such "
  "as \\Polygon or \\Box). In addition, a boolean argument can be given indicating whether clipping at the tile shall be "
  "applied. If clipping is requested (the default), the shapes will be clipped at the tile's box.\n"
  "\n"
  "The tiling can be specified either through a tile size, a tile number or both. If a tile size is specified with the "
  "\\TilingProcessor#tile_size method, the tiling processor will compute the number of tiles required. If the tile "
  "count is given (through \\TilingProcessor#tiles), the tile size will be computed. If both are given, the tiling "
  "array is fixed and the array is centered around the original layout's center. If the tiling origin is given as well, "
  "the tiling processor will use the given array without any modifications.\n"
  "\n"
  "Once the tiling processor has been set up, the operation can be launched using \\TilingProcessor#execute.\n"
  "\n"
  "This is some sample code. It performs two XOR operations between two layouts and delivers the results to a "
  "report database:\n"
  "\n"
  "@code\n"
  "ly1 = ... # first layout\n"
  "ly2 = ... # second layout\n"
  "\n"
  "rdb = RBA::ReportDatabase::new(\"xor\")\n"
  "output_cell = rdb.create_cell(ly1.top_cell.name)\n"
  "output_cat1 = rbd.create_category(\"XOR 1-10\")\n"
  "output_cat2 = rbd.create_category(\"XOR 2-11\")\n"
  "\n"
  "tp = RBA::TilingProcessor::new\n"
  "tp.input(\"a1\", ly1, ly1.top_cell.cell_index, RBA::LayerInfo::new(1, 0))\n"
  "tp.input(\"a2\", ly1, ly1.top_cell.cell_index, RBA::LayerInfo::new(2, 0))\n"
  "tp.input(\"b1\", ly2, ly2.top_cell.cell_index, RBA::LayerInfo::new(11, 0))\n"
  "tp.input(\"b2\", ly2, ly2.top_cell.cell_index, RBA::LayerInfo::new(12, 0))\n"
  "tp.output(\"o1\", rdb, output_cell, output_cat1)\n"
  "tp.output(\"o2\", rdb, output_cell, output_cat2)\n"
  "tp.queue(\"_output(o1, a1 ^ b1)\")\n"
  "tp.queue(\"_output(o2, a2 ^ b2)\")\n"
  "tp.tile_size(50.0, 50.0)\n"
  "tp.execute(\"Job description\")\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}
