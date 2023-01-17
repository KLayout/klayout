
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


#include "dbShapes2.h"

namespace db
{

// -------------------------------------------------------------------------------
//  dereference operator

/**
 *  @brief Dereferences the given shape into the given Shapes container
 */
struct deref_into_shapes
{
  deref_into_shapes (Shapes *shapes) 
    : mp_shapes (shapes)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class PropIdMap>
  void op (const Sh &sh, PropIdMap & /*pm*/)
  {
    mp_shapes->insert (sh);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const db::text_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const db::polygon_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const db::path_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class ATrans, class PropIdMap>
  void op (const db::array<Sh, ATrans> &sh, PropIdMap & /*pm*/)
  {
    for (typename db::array<Sh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (*a * sh.object ());
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::array<db::text_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename db::array<db::text_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::array<db::polygon_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename db::array<db::polygon_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::array<db::path_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename db::array<db::path_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class PropIdMap>
  void op (const db::array<db::box<C, R>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    db::box<C> box (sh.object ()); // avoid problems with short boxes
    for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (box.transformed (*a));
    }
  }

  template <class Sh, class PropIdMap>
  void op (const db::object_with_properties<Sh> &sh, PropIdMap &pm)
  {
    mp_shapes->insert (db::object_with_properties<Sh> (sh, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::text_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::polygon_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::path_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class ATrans, class PropIdMap>
  void op (const db::object_with_properties<db::array<InnerSh, ATrans> > &sh, PropIdMap &pm)
  {
    for (typename db::array<InnerSh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<InnerSh> (*a * sh.object (), pm (sh.properties_id ())));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::box<C, R>, ATrans> > &sh, PropIdMap &pm)
  {
    db::box<C> box (sh.object ()); // avoid problems with short boxes
    for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<db::box<C> > (box.transformed (*a), pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::text_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename db::array<db::text_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::polygon_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename db::array<db::polygon_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::path_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename db::array<db::path_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh>
  void operator() (const Sh &shape)
  {
    tl::ident_map <db::properties_id_type> pm;
    op (shape, pm);
  }

  template <class Sh, class PropIdMap>
  void operator() (const Sh &shape, PropIdMap &pm)
  {
    op (shape, pm);
  }

private:
  Shapes *mp_shapes;
};

// -------------------------------------------------------------------------------
//  dereference and transform operator

/**
 *  @brief Dereferences the given shape and transforms it into the given Shapes container
 */
struct deref_and_transform_into_shapes
{
  deref_and_transform_into_shapes (Shapes *shapes) 
    : mp_shapes (shapes)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class Trans, class PropIdMap>
  void op (const Sh &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    mp_shapes->insert (sh.transformed (trans));
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const db::polygon_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const db::path_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const db::text_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const db::box<C, R> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (box.transformed (trans));
    } else {
      db::polygon<C> poly = db::polygon<C> (db::box<C> (sh));
      mp_shapes->insert (poly.transformed (trans));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class Trans, class PropIdMap>
  void op (const db::array<db::box<C, R>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh.object ()); // avoid problems with short boxes
      for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (box.transformed (trans * Trans (*a)));
      }
    } else {
      db::polygon<C> poly = db::polygon<C> (db::box<C> (sh.object ()));
      for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (poly.transformed (trans * Trans (*a)));
      }
    }
  }

  template <class Sh, class Trans, class ATrans, class PropIdMap>
  void op (const db::array<Sh, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    for (typename db::array<Sh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (sh.object ().transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::array<db::text_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::text_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::array<db::polygon_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::polygon_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::array<db::path_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::path_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const db::object_with_properties<Sh> &sh, const Trans &trans, PropIdMap &pm)
  {
    mp_shapes->insert (db::object_with_properties<Sh> (sh.transformed (trans), pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::box<C, R> > &sh, const Trans &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (db::object_with_properties<db::box<C> > (box.transformed (trans), pm (sh.properties_id ())));
    } else {
      db::polygon<C> poly  = db::polygon<C> (db::box<C> (sh));
      mp_shapes->insert (db::object_with_properties<db::polygon<C> > (poly.transformed (trans), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::text_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::polygon_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::path_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (db::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class Trans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::box<C, R>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh.object ()); // avoid problems with short boxes
      for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (db::object_with_properties<db::box<C> > (box.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
      }
    } else {
      db::polygon<C> poly  = db::polygon<C> (db::box<C> (sh.object ()));
      for (typename db::array<db::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (db::object_with_properties<db::polygon<C> > (poly.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
      }
    }
  }

  template <class Trans, class AObject, class ATrans, class InnerSh, class PropIdMap>
  void op (const db::object_with_properties<db::array<AObject, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst (sh.object ());
    for (typename db::array<AObject, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::text_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::text_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::polygon_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::polygon_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::path_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename db::array<db::path_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (db::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh, class T>
  void operator() (const Sh &shape, const T &trans)
  {
    tl::ident_map <db::properties_id_type> pm;
    op (shape, trans, pm);
  }

  template <class Sh, class T, class PropIdMap>
  void operator() (const Sh &shape, const T &trans, PropIdMap &pm)
  {
    op (shape, trans, pm);
  }

private:
  Shapes *mp_shapes;
};

// -------------------------------------------------------------------------------
//  translate operator

/**
 *  @brief Translates the given shape into the given Shapes container using the given array repository 
 */
struct translate_into_shapes
{
  translate_into_shapes (Shapes *shapes, GenericRepository &rep, ArrayRepository &array_rep) 
    : mp_shapes (shapes), m_shape_rep (rep), m_array_rep (array_rep)
  {
    //  .. nothing yet ..
  }

  template <class Sh>
  void operator() (const Sh &sh)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh, class PropIdMap>
  void operator() (const Sh &sh, PropIdMap & /*pm*/)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh>
  void operator() (const db::object_with_properties<Sh> &sh)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (db::object_with_properties<Sh> (new_shape, sh.properties_id ()));
  }

  template <class Sh, class PropIdMap>
  void operator() (const db::object_with_properties<Sh> &sh, PropIdMap &pm)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (db::object_with_properties<Sh> (new_shape, pm (sh.properties_id ())));
  }

private:
  db::Shapes *mp_shapes;
  GenericRepository &m_shape_rep;
  ArrayRepository &m_array_rep;
};

// -------------------------------------------------------------------------------
//  translate and transform operator

/**
 *  @brief Translates and transforms the given shape into the given Shapes container using the given array repository 
 */
struct translate_and_transform_into_shapes
{
  translate_and_transform_into_shapes (Shapes *shapes, GenericRepository &rep, ArrayRepository &array_rep) 
    : mp_shapes (shapes), m_shape_rep (rep), m_array_rep (array_rep)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class T, class PropIdMap>
  void op (const Sh &sh, const T &trans, PropIdMap & /*pm*/)
  {
    Sh new_shape;
    new_shape.translate (sh, trans, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh, class T, class PropIdMap>
  void op (const db::object_with_properties<Sh> &sh, const T &trans, PropIdMap &pm)
  {
    Sh new_shape;
    new_shape.translate (sh, trans, m_shape_rep, m_array_rep);
    mp_shapes->insert (db::object_with_properties<Sh> (new_shape, pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const box<C, R> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (box.transformed (trans));
    } else {
      db::polygon<C> poly = db::polygon<C> (db::box<C> (sh));
      mp_shapes->insert (poly.transformed (trans));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const db::object_with_properties<box<C, R> > &sh, const T &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      db::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (db::object_with_properties<db::box<C> > (box.transformed (trans), pm (sh.properties_id ())));
    } else {
      db::polygon<C> poly = db::polygon<C> (db::box<C> (sh));
      mp_shapes->insert (db::object_with_properties<db::polygon<C> > (poly.transformed (trans), pm (sh.properties_id ())));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class T, class PropIdMap>
  void op (const db::array<db::box<C, R>, ATrans> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      db::array<db::box<C, R>, ATrans> new_array;
      new_array.translate (sh, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (new_array);
    } else {
      //  Convert the box to a polygon reference and translate ..
      db::polygon_ref<db::polygon<C>, db::unit_trans<C> > poly_ref (db::polygon<C> (db::box<C> (sh.object ())), m_shape_rep);
      const db::basic_array<C> *ba = dynamic_cast <const db::basic_array<C> *> (sh.delegate ());
      db::array<db::polygon_ref<db::polygon<C>, db::unit_trans<C> >, db::disp_trans<C> > poly_array (poly_ref, db::disp_trans<C> (sh.front ()), ba ? ba->clone () : 0);
      db::array<db::polygon_ref<db::polygon<C>, db::unit_trans<C> >, db::disp_trans<C> > new_array;
      new_array.translate (poly_array, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (new_array);
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class T, class PropIdMap>
  void op (const db::object_with_properties<db::array<db::box<C, R>, ATrans> > &sh, const T &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      db::array<db::box<C, R>, ATrans> new_array;
      new_array.translate (sh, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (db::object_with_properties<db::array<db::box<C, R>, ATrans> > (new_array, pm (sh.properties_id ())));
    } else {
      //  Convert the box to a polygon reference and translate ..
      db::polygon_ref<db::polygon<C>, db::unit_trans<C> > poly_ref (db::polygon<C> (db::box<C> (sh.object ())), m_shape_rep);
      const db::basic_array<C> *ba = dynamic_cast <const db::basic_array<C> *> (sh.delegate ());
      db::array<db::polygon_ref<db::polygon<C>, db::unit_trans<C> >, db::disp_trans<C> > poly_array (poly_ref, db::disp_trans<C> (sh.front ()), ba ? ba->clone () : 0);
      db::array<db::polygon_ref<db::polygon<C>, db::unit_trans<C> >, db::disp_trans<C> > new_array;
      new_array.translate (poly_array, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (db::object_with_properties<db::array<db::polygon_ref<db::polygon<C>, db::unit_trans<C> >, db::disp_trans<C> > > (new_array, pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh, class T, class PropIdMap>
  void operator() (const Sh &shape, const T &trans, PropIdMap &pm)
  {
    op (shape, trans, pm);
  }

  template <class Sh, class T>
  void operator() (const Sh &shape, const T &trans)
  {
    tl::ident_map <db::properties_id_type> pm;
    op (shape, trans, pm);
  }

private:
  db::Shapes *mp_shapes;
  GenericRepository &m_shape_rep;
  ArrayRepository &m_array_rep;
};

// -------------------------------------------------------------------------------
//  LayerBase implementations

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_type::tag)
{
  return 1 << ShapeIterator::Polygon;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_ref_type::tag)
{
  return 1 << ShapeIterator::PolygonRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_ptr_array_type::tag)
{
  return 1 << ShapeIterator::PolygonPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_type::tag)
{
  return 1 << ShapeIterator::SimplePolygon;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_ref_type::tag)
{
  return 1 << ShapeIterator::SimplePolygonRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_ptr_array_type::tag)
{
  return 1 << ShapeIterator::SimplePolygonPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::edge_type::tag)
{
  return 1 << ShapeIterator::Edge;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::edge_pair_type::tag)
{
  return 1 << ShapeIterator::EdgePair;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::point_type::tag)
{
  return 1 << ShapeIterator::Point;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_type::tag)
{
  return 1 << ShapeIterator::Path;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_ref_type::tag)
{
  return 1 << ShapeIterator::PathRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_ptr_array_type::tag)
{
  return 1 << ShapeIterator::PathPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_type::tag)
{
  return 1 << ShapeIterator::Text;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_ref_type::tag)
{
  return 1 << ShapeIterator::TextRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_ptr_array_type::tag)
{
  return 1 << ShapeIterator::TextPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::box_type::tag)
{
  return 1 << ShapeIterator::Box;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::box_array_type::tag)
{
  return 1 << ShapeIterator::BoxArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::short_box_type::tag)
{
  return 1 << ShapeIterator::ShortBox;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::short_box_array_type::tag)
{
  return 1 << ShapeIterator::ShortBoxArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::user_object_type::tag)
{
  return 1 << ShapeIterator::UserObject;
}

/// @brief Internal: ShapeIterator masks per shape type
template <class Sh>
inline unsigned int iterator_type_mask (db::object_tag< db::object_with_properties<Sh> >)
{
  return iterator_type_mask (typename Sh::tag ()) | ShapeIterator::Properties;
}

template <class Sh, class StableTag>
LayerBase *
layer_class<Sh, StableTag>::clone () const
{
  layer_class<Sh, StableTag> *r = new layer_class<Sh, StableTag> ();
  r->m_layer = m_layer;
  return r;
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s);
  }
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, pm);
  }
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void
layer_class<Sh, StableTag>::insert_into (Shapes *target)
{
  target->insert (m_layer.begin (), m_layer.end ());
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_into (Shapes *target) 
{
  deref_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_into (Shapes *target, pm_delegate_type &pm) 
{
  deref_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const Trans &trans) 
{
  deref_and_transform_into_shapes deref_op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    deref_op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const Trans &trans, pm_delegate_type &pm) 
{
  deref_and_transform_into_shapes deref_op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    deref_op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const ICplxTrans &trans) 
{
  deref_and_transform_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const ICplxTrans &trans, pm_delegate_type &pm) 
{
  deref_and_transform_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
unsigned int 
layer_class<Sh, StableTag>::type_mask () const 
{
  return iterator_type_mask (typename Sh::tag ());
}

//  explicit instantiations

template class layer_class<db::Shape::polygon_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::polygon_ref_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_ref_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::polygon_ptr_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::path_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::path_ref_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::path_ptr_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::edge_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::point_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::edge_pair_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::text_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::text_ref_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::text_ptr_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::box_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::box_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::short_box_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::short_box_array_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::user_object_type, db::stable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag>;
template class layer_class<db::Shape::polygon_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::polygon_ref_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::path_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::path_ref_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::path_ptr_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::edge_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::edge_pair_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::point_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::text_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::text_ref_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::text_ptr_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::box_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::box_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::short_box_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::short_box_array_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag>;
template class layer_class<db::Shape::user_object_type, db::unstable_layer_tag>;
template class layer_class<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag>;

}

