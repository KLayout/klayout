
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


#include "rdb.h"
#include "rdbReader.h"
#include "tlString.h"
#include "tlAssert.h"
#include "tlStream.h"
#include "tlLog.h"
#include "tlBase64.h"
#include "dbPolygon.h"
#include "dbBox.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbShape.h"

#if defined(HAVE_QT)
#  include <QByteArray>
#  include <QBuffer>
#  include <QImage>
#endif

#include <limits>
#include <memory>

namespace rdb
{

// ------------------------------------------------------------------------------------------
//  Value implementation

//  type index specializations

template <> RDB_PUBLIC int type_index_of<double> ()        { return 0; }
template <> RDB_PUBLIC int type_index_of<std::string> ()   { return 1; }
template <> RDB_PUBLIC int type_index_of<db::DPolygon> ()  { return 2; }
template <> RDB_PUBLIC int type_index_of<db::DEdge> ()     { return 3; }
template <> RDB_PUBLIC int type_index_of<db::DEdgePair> () { return 4; }
template <> RDB_PUBLIC int type_index_of<db::DBox> ()      { return 5; }
template <> RDB_PUBLIC int type_index_of<db::DPath> ()     { return 6; }
template <> RDB_PUBLIC int type_index_of<db::DText> ()     { return 7; }

//  Explicit instantiations to make VC++ happy in debug mode
template class RDB_PUBLIC Value<double>;
template class RDB_PUBLIC Value<std::string>;
template class RDB_PUBLIC Value<db::DPolygon>;
template class RDB_PUBLIC Value<db::DEdge>;
template class RDB_PUBLIC Value<db::DEdgePair>;
template class RDB_PUBLIC Value<db::DBox>;
template class RDB_PUBLIC Value<db::DPath>;
template class RDB_PUBLIC Value<db::DText>;

//  to_string implementations

template <> RDB_PUBLIC std::string Value<double>::to_string () const
{
  return "float: " + tl::to_string (m_value);
}

template <> RDB_PUBLIC std::string Value<std::string>::to_string () const
{
  return "text: " + tl::to_word_or_quoted_string (m_value);
}

template <> RDB_PUBLIC std::string Value<db::DPolygon>::to_string () const
{
  return "polygon: " + m_value.to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DEdge>::to_string () const
{
  return "edge: " + m_value.to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DEdgePair>::to_string () const
{
  return "edge-pair: " + m_value.to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DBox>::to_string () const
{
  return "box: " + m_value.to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DPath>::to_string () const
{
  return "path: " + m_value.to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DText>::to_string () const
{
  return "label: " + m_value.to_string ();
}

//  to_display_string implementations

template <> RDB_PUBLIC std::string Value<double>::to_display_string () const
{
  return tl::to_string (m_value);
}

template <> RDB_PUBLIC std::string Value<std::string>::to_display_string () const
{
  return m_value;
}

template <> RDB_PUBLIC std::string Value<db::DPolygon>::to_display_string () const
{
  return to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DEdge>::to_display_string () const
{
  return to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DEdgePair>::to_display_string () const
{
  return to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DBox>::to_display_string () const
{
  return to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DPath>::to_display_string () const
{
  return to_string ();
}

template <> RDB_PUBLIC std::string Value<db::DText>::to_display_string () const
{
  return to_string ();
}

//  is_shape implementations

template <> RDB_PUBLIC bool Value<double>::is_shape () const
{
  return false;
}

template <> RDB_PUBLIC bool Value<std::string>::is_shape () const
{
  return false;
}

template <> RDB_PUBLIC bool Value<db::DPolygon>::is_shape () const
{
  return true;
}

template <> RDB_PUBLIC bool Value<db::DEdge>::is_shape () const
{
  return true;
}

template <> RDB_PUBLIC bool Value<db::DEdgePair>::is_shape () const
{
  return true;
}

template <> RDB_PUBLIC bool Value<db::DBox>::is_shape () const
{
  return true;
}

template <> RDB_PUBLIC bool Value<db::DPath>::is_shape () const
{
  return true;
}

template <> RDB_PUBLIC bool Value<db::DText>::is_shape () const
{
  return true;
}


bool ValueBase::compare (const ValueBase *a, const ValueBase *b) 
{
  //  compare is the intrinsic compare of equal type and type index for different types.
  if (a->type_index () == b->type_index ()) {
    return a->compare (b);
  } else {
    return a->type_index () < b->type_index ();
  }
}

ValueBase *
ValueBase::create_from_string (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  return create_from_string (ex);
}

ValueBase *
ValueBase::create_from_string (tl::Extractor &ex)
{
  if (ex.test ("polygon")) {

    ex.expect (":");

    db::DPolygon poly;
    ex.read (poly);

    return new Value <db::DPolygon> (poly);

  } else if (ex.test ("edge-pair")) {

    ex.expect (":");

    db::DEdgePair ep;
    ex.read (ep);

    return new Value <db::DEdgePair> (ep);

  } else if (ex.test ("edge")) {

    ex.expect (":");

    db::DEdge edge;
    ex.read (edge);

    return new Value <db::DEdge> (edge);

  } else if (ex.test ("box")) {

    ex.expect (":");

    db::DBox box;
    ex.read (box);

    return new Value <db::DBox> (box);

  } else if (ex.test ("path")) {

    ex.expect (":");

    db::DPath path;
    ex.read (path);

    return new Value <db::DPath> (path);

  } else if (ex.test ("label")) {

    ex.expect (":");

    db::DText text;
    ex.read (text);

    return new Value <db::DText> (text);

  } else if (ex.test ("text")) {

    ex.expect (":");

    std::string text;
    ex.read_word_or_quoted (text);

    return new Value <std::string> (text);

  } else if (ex.test ("float")) {

    ex.expect (":");

    double v = 0.0;
    ex.read (v);

    return new Value <double> (v);

  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid value string at '...%s'")), ex.skip ());
  }
}

ValueBase *
ValueBase::create_from_shape (const db::Shape &shape, const db::CplxTrans &trans)
{
  if (shape.is_polygon () || shape.is_box ()) {

    db::Polygon poly;
    shape.polygon (poly);
    return new rdb::Value <db::DPolygon> (poly.transformed (trans));

  } else if (shape.is_path ()) {

    db::Path path;
    shape.path (path);
    return new rdb::Value <db::DPath> (path.transformed (trans));

  } else if (shape.is_text ()) {

    db::Text text;
    shape.text (text);
    return new rdb::Value <db::DText> (text.transformed (trans));

  } else if (shape.is_edge ()) {

    db::Edge edge;
    shape.edge (edge);
    return new rdb::Value <db::DEdge> (edge.transformed (trans));

  } else if (shape.is_edge_pair ()) {

    db::EdgePair edge_pair;
    shape.edge_pair (edge_pair);
    return new rdb::Value <db::DEdgePair> (edge_pair.transformed (trans));

  } else {
    return 0;
  }
}

// ------------------------------------------------------------------------------------------
//  ValueWrapper implementation

std::string 
ValueWrapper::to_string (const Database *rdb) const
{
  std::string r;
  r.reserve (200);

  if (tag_id () > 0 && rdb) {
    r += "[";
    const Tag &tag = rdb->tags ().tag (tag_id ());
    if (tag.is_user_tag ()) {
      r += "#";
    }
    r += tl::to_word_or_quoted_string (tag.name ());
    r += "] ";
  }

  r += get ()->to_string ();

  return r;
}

void 
ValueWrapper::from_string (Database *rdb, const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  from_string (rdb, ex);
}

void 
ValueWrapper::from_string (Database *rdb, tl::Extractor &ex)
{
  id_type tag_id = 0;

  if (ex.test ("[")) {

    bool user_tag = ex.test ("#");

    std::string tn;  
    ex.read_word_or_quoted (tn);

    tag_id = rdb->tags ().tag (tn, user_tag).id ();

    ex.test ("]");

  }

  set (ValueBase::create_from_string (ex));
  set_tag_id (tag_id);
}

// ------------------------------------------------------------------------------------------
//  Values implementation

Values &
Values::operator= (const Values &d)
{
  m_values = d.m_values;
  return *this;
}

std::string 
Values::to_string (const Database *rdb) const
{
  std::string r;
  r.reserve (200);

  for (const_iterator v = begin (); v != end (); ++v) {

    if (! r.empty ()) {
      r += ";";
    }

    r += v->to_string (rdb);

  }

  return r;
}

void 
Values::from_string (Database *rdb, const std::string &s) 
{
  tl::Extractor ex (s.c_str ());

  while (! ex.at_end ()) {

    ValueWrapper v;
    v.from_string (rdb, ex);

    add (v);

    ex.test (";");

  }
}

// ------------------------------------------------------------------------------------------
//  Cell implementation

void 
Cells::import_cell (const Cell &c)
{
  Cell *cell;
  if (mp_database) {
    cell = mp_database->create_cell (c.name (), c.variant ());
  } else {
    cell = new Cell (0, c.name ());
    add_cell (cell);
  }

  for (References::const_iterator r = c.references ().begin (); r != c.references ().end (); ++r) {
    cell->references ().insert (*r);
  }
}
  
// ------------------------------------------------------------------------------------------
//  Cell implementation

Cell::Cell ()
  : m_id (0), m_num_items (0), m_num_items_visited (0), mp_database (0)
{
  //  .. nothing yet ..
}

Cell::Cell (Cells *cells)
  : m_id (0), m_num_items (0), m_num_items_visited (0), mp_database (cells->database ())
{
  m_references.set_database (mp_database);
}

Cell::Cell (id_type id, const std::string &name)
  : m_id (id), m_name (name), m_num_items (0), m_num_items_visited (0), mp_database (0)
{
  //  .. nothing yet ..
}

Cell::Cell (id_type id, const std::string &name, const std::string &variant)
  : m_id (id), m_name (name), m_variant (variant), m_num_items (0), m_num_items_visited (0), mp_database (0)
{
  //  .. nothing yet ..
}

Cell::~Cell ()
{
  //  .. nothing yet ..
}

std::pair<bool, db::DCplxTrans> 
Cell::path_to (id_type parent_cell_id, const Database *db) const
{
  if (parent_cell_id == id ()) {
    return std::make_pair (true, db::DCplxTrans ());
  } else {
    std::set <id_type> v;
    return path_to (parent_cell_id, db, v, db::DCplxTrans ());
  }
}

std::pair<bool, db::DCplxTrans> 
Cell::path_to (id_type parent_cell_id, const Database *db, std::set <id_type> &visited, const db::DCplxTrans &trans) const
{
  for (reference_iterator r = references ().begin (); r != references ().end (); ++r) {

    if (r->parent_cell_id () == parent_cell_id) {

      return std::make_pair (true, r->trans () * trans);

    } else if (visited.find (r->parent_cell_id ()) == visited.end ()) {

      visited.insert (r->parent_cell_id ());
      const Cell *rc = db->cell_by_id (r->parent_cell_id ());

      if (rc) {
        std::pair<bool, db::DCplxTrans> path = rc->path_to (parent_cell_id, db, visited, r->trans () * trans);
        if (path.first) {
          return path;
        }
      }

    }

  }

  return std::pair<bool, db::DCplxTrans> (false, db::DCplxTrans ());
}

void 
Cell::import_references (const References &references)
{
  for (References::const_iterator r = references.begin (); r != references.end (); ++r) {
    m_references.insert (*r);
  }
}
  
std::string 
Cell::qname () const
{
  if (m_variant.empty ()) {
    return m_name;
  } else {
    return m_name + ":" + m_variant;
  }
}

// ------------------------------------------------------------------------------------------
//  References implementation

References::References ()
  : mp_database (0)
{
  //  .. nothing yet ..
}

References::References (Cell *cell)
  : mp_database (cell->database ())
{
  //  .. nothing yet ..
}

void 
References::set_database (Database *database)
{
  mp_database = database;
  for (iterator r = begin (); r != end (); ++r) {
    r->set_database (database);
  }
}

// ------------------------------------------------------------------------------------------
//  Reference implementation

Reference::Reference (References *references)
  : m_trans (), m_parent_cell_id (0), mp_database (references->database ())
{
  //  .. nothing yet ..
}

void 
Reference::set_trans_str (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  ex.read (m_trans);
}

std::string 
Reference::trans_str () const
{
  return m_trans.to_string ();
}

void 
Reference::set_parent_cell_qname (const std::string &qname)
{
  tl_assert (mp_database != 0);
  const Cell *cell = mp_database->cell_by_qname (qname);
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("%s is not a valid cell name or name/variant combination")), qname);
  }
  m_parent_cell_id = cell->id ();
}

std::string 
Reference::parent_cell_qname () const
{
  tl_assert (mp_database != 0);
  const Cell *cell = mp_database->cell_by_id (m_parent_cell_id);
  tl_assert (cell != 0);
  return cell->qname ();
}

// ------------------------------------------------------------------------------------------
//  Category implementation

Category::Category (Categories *categories)
  : m_id (0), mp_parent (0), mp_sub_categories (0), m_num_items (0), m_num_items_visited (0), mp_database (categories->database ())
{
  // .. nothing yet ..
}

Category::Category ()
  : m_id (0), mp_parent (0), mp_sub_categories (0), m_num_items (0), m_num_items_visited (0), mp_database (0)
{
  // .. nothing yet ..
}

Category::Category (const std::string &name)
  : m_id (0), m_name (name), mp_parent (0), mp_sub_categories (0), m_num_items (0), m_num_items_visited (0), mp_database (0)
{
  // .. nothing yet ..
}

Category::~Category ()
{
  if (mp_sub_categories) {
    delete mp_sub_categories;
    mp_sub_categories = 0;
  }
}

const Categories &
Category::sub_categories () const
{
  if (! mp_sub_categories) {
    static Categories empty_categories;
    return empty_categories;
  } else {
    return *mp_sub_categories;
  }
}

Categories &
Category::sub_categories ()
{
  if (! mp_sub_categories) {
    mp_sub_categories = new Categories (this);
  }
  return *mp_sub_categories;
}

std::string 
Category::path () const
{
  std::vector <std::string> path_elements;
  const Category *c = this;
  while (c) {
    path_elements.push_back (c->name ());
    c = c->parent ();
  }

  std::reverse (path_elements.begin (), path_elements.end ());

  std::string r;
  for (std::vector <std::string>::const_iterator p = path_elements.begin (); p != path_elements.end (); ++p) {
    if (p != path_elements.begin ()) {
      r += ".";
    }
    r += tl::to_word_or_quoted_string (*p, "_$");
  }

  return r;
}

void 
Category::import_sub_categories (Categories *categories)
{
  if (mp_sub_categories) {
    delete mp_sub_categories;
  }

  mp_sub_categories = categories;

  if (mp_sub_categories != 0) {
    for (Categories::iterator c = mp_sub_categories->begin (); c != mp_sub_categories->end (); ++c) {
      c->set_parent (this);
    }
  }
}

void 
Category::set_database (Database *database)
{
  mp_database = database;
  if (mp_sub_categories) {
    mp_sub_categories->set_database (database);
  }
}

// ------------------------------------------------------------------------------------------
//  Categories implementation

void 
Categories::clear ()
{
  m_categories.clear ();
  m_categories_by_name.clear ();
}

void 
Categories::add_category (Category *cat)
{
  cat->set_database (mp_database.get ());
  m_categories.push_back (cat);
  m_categories_by_name.insert (std::make_pair (cat->name (), cat));
}

void  
Categories::set_database (Database *database)
{
  mp_database = database;
  for (iterator c = m_categories.begin (); c != m_categories.end (); ++c) {
    c->set_database (database);
  }
}

Category *
Categories::category_by_name (const char *path) 
{
  std::string component;
  tl::Extractor ex (path);
  ex.read_word_or_quoted (component, "_$");

  std::map <std::string, Category *>::iterator ref = m_categories_by_name.find (component);
  if (ref != m_categories_by_name.end ()) {
    if (ex.test (".")) {
      return ref->second->sub_categories ().category_by_name (ex.skip ());
    } else {
      return ref->second;
    }
  }

  return 0;
}

void 
Categories::import_category (Category *category)
{
  Category *cat;

  if (mp_database) {
    //  assigns an Id:
    cat = mp_database->create_category (this, category->name ());
  } else {
    //  here, Categories acts as a stupid container:
    cat = new Category (category->name ());
    add_category (cat);
  }

  cat->set_description (category->description ());
  cat->import_sub_categories (category->mp_sub_categories);

  category->mp_sub_categories = 0;
  delete category;
}

// ------------------------------------------------------------------------------------------
//  Tags implementation

Tags::Tags ()
{
  //  .. nothing yet ..
}

void 
Tags::clear ()
{
  m_ids_for_names.clear ();
  m_tags.clear ();
}

const Tag &
Tags::tag (const std::string &name, bool user_tag) const
{
  return (const_cast <Tags *> (this)->tag (name, user_tag));
}

Tag &
Tags::tag (const std::string &name, bool user_tag)
{
  std::map <std::pair<std::string, bool>, id_type>::const_iterator i = m_ids_for_names.find (std::make_pair (name, user_tag));
  if (i == m_ids_for_names.end ()) {
    i = m_ids_for_names.insert (std::make_pair (std::make_pair (name, user_tag), m_tags.size () + 1)).first;
    m_tags.push_back (Tag (i->second, name, user_tag));
  }
  return m_tags [i->second - 1];
}

const Tag &
Tags::tag (id_type id) const
{
  tl_assert (id - 1 < m_tags.size () && id > 0);
  return m_tags [id - 1];
}

Tag &
Tags::tag (id_type id)
{
  tl_assert (id - 1 < m_tags.size () && id > 0);
  return m_tags [id - 1];
}

void
Tags::import_tag (const Tag &t)
{
  Tag &tt = tag (t.name (), t.is_user_tag ());
  tt.set_description (t.description ());
}

bool 
Tags::has_tag (const std::string &name, bool user_tag) const
{
  return m_ids_for_names.find (std::make_pair (name, user_tag)) != m_ids_for_names.end ();
}

// ------------------------------------------------------------------------------------------
//  Item implementation

Item::Item (Items *items)
  : m_cell_id (0), m_category_id (0), m_multiplicity (1), m_visited (false), mp_database (items->database ())
{
  // .. nothing yet ..
}

Item::Item ()
  : m_cell_id (0), m_category_id (0), m_multiplicity (1), m_visited (false), mp_database (0)
{
  // .. nothing yet ..
}

Item::Item (const Item &d)
  : tl::Object (d), m_cell_id (0), m_category_id (0), m_multiplicity (1), m_visited (false), mp_database (d.mp_database)
{
  operator= (d);
}

Item::~Item ()
{
  // .. nothing yet ..
}

Item &Item::operator= (const Item &d)
{
  if (this != &d) {
    m_values = d.m_values;
    m_cell_id = d.m_cell_id;
    m_category_id = d.m_category_id;
    m_visited = d.m_visited;
    m_multiplicity = d.m_multiplicity;
    m_tag_ids = d.m_tag_ids;
    m_image_str = d.m_image_str;
  }

  return *this;
}

void 
Item::add_tag (id_type tag_id)
{
  if (m_tag_ids.size () <= tag_id) {
    m_tag_ids.resize (tag_id + 1, false);
  }
  m_tag_ids [tag_id] = true;
}

void 
Item::remove_tag (id_type tag_id)
{
  if (m_tag_ids.size () > tag_id) {
    m_tag_ids [tag_id] = false;
  }
}

void 
Item::remove_tags ()
{
  m_tag_ids = std::vector <bool> ();
}

bool 
Item::has_tag (id_type tag_id) const
{
  return m_tag_ids.size () > tag_id && m_tag_ids [tag_id];
}

std::string 
Item::cell_qname () const
{
  tl_assert (mp_database != 0);
  const Cell *cell = mp_database->cell_by_id (m_cell_id);
  tl_assert (cell != 0);
  return cell->qname ();
}

void 
Item::set_cell_qname (const std::string &qname)
{
  tl_assert (mp_database != 0);
  const Cell *cell = mp_database->cell_by_qname (qname);
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("%s is not a valid cell name or name/variant combination")), qname);
  }
  m_cell_id = cell->id ();
}

std::string
Item::category_name () const
{
  tl_assert (mp_database != 0);
  const Category *category = mp_database->category_by_id (m_category_id);
  tl_assert (category != 0);
  return category->path ();
}

void 
Item::set_category_name (const std::string &path)
{
  tl_assert (mp_database != 0);
  const Category *category = mp_database->categories ().category_by_name (path.c_str ());
  if (! category) {
    throw tl::Exception (tl::to_string (tr ("%s is not a valid category path")), path);
  }
  m_category_id = category->id ();
}

std::string 
Item::tag_str () const
{
  tl_assert (mp_database != 0);

  std::string r;
  r.reserve (200);

  if (! m_tag_ids.empty ()) {

    id_type tag_id = 0;
    for (std::vector<bool>::const_iterator t = m_tag_ids.begin (); t != m_tag_ids.end (); ++t, ++tag_id) {
      if (*t) {
        if (! r.empty ()) {
          r += ",";
        }
        const Tag &tag = mp_database->tags ().tag (tag_id);
        if (tag.is_user_tag ()) {
          r += "#";
        }
        r += tl::to_word_or_quoted_string (tag.name ());
      }
    }

  }

  return r;
}

void 
Item::set_tag_str (const std::string &tags)
{
  tl_assert (mp_database != 0);

  m_tag_ids.clear ();

  tl::Extractor ex (tags.c_str ());
  while (! ex.at_end ()) {
    ex.test (",");
    bool user_tag = ex.test ("#");
    std::string tag_name;
    ex.read_word_or_quoted (tag_name);
    add_tag (mp_database->tags ().tag (tag_name, user_tag).id ());
  }
}

#if defined(HAVE_QT)
void 
Item::set_image (const QImage &image)
{
  if (image.isNull ()) {

    m_image_str.clear ();

  } else {

    QByteArray img_data;
    QBuffer img_io_device (&img_data);
    image.save (&img_io_device, "PNG");

    m_image_str = std::string (img_data.toBase64 ().constData ());

  }
}

QImage
Item::image () const
{
  if (m_image_str.empty ()) {

    return QImage ();

  } else {

    QByteArray img_data (QByteArray::fromBase64 (QByteArray::fromRawData (m_image_str.c_str (), int (m_image_str.size ()))));

    QImage image;
    image.loadFromData (img_data);
    return image;

  }
}
#endif

#if defined(HAVE_PNG)

tl::PixelBuffer
Item::image_pixels () const
{
  std::vector<unsigned char> data = tl::from_base64 (m_image_str.c_str ());
  tl::InputStream stream (new tl::InputMemoryStream ((const char *) data.begin ().operator-> (), data.size ()));
  return tl::PixelBuffer::read_png (stream);
}

void
Item::set_image (const tl::PixelBuffer &image)
{
  tl::OutputMemoryStream mem;
  {
    tl::OutputStream stream (mem);
    image.write_png (stream);
  }
  m_image_str = tl::to_base64 ((const unsigned char *) mem.data (), mem.size ());
}

#endif

bool
Item::has_image () const
{
  return !m_image_str.empty ();
}

std::string
Item::image_str () const
{
  return m_image_str;
}

void
Item::set_image_str (const std::string &s)
{
  m_image_str = s;
}

// ------------------------------------------------------------------------------------------
//  Database implementation

Database::Database ()
  : m_next_id (0), m_num_items (0), m_num_items_visited (0), m_modified (true)
{
  m_cells.set_database (this);

  mp_items = new Items ();
  mp_items->set_database (this);

  mp_categories = new Categories ();
  mp_categories->set_database (this);
}

Database::~Database ()
{
  m_items_by_cell_id.clear ();
  m_items_by_cell_and_category_id.clear ();
  m_items_by_category_id.clear ();

  delete mp_items;
  mp_items = 0;

  delete mp_categories;
  mp_categories = 0;
}

void
Database::set_items (Items *items)
{
  set_modified ();

  delete mp_items;

  mp_items = items;
  mp_items->set_database (this);

  m_items_by_cell_and_category_id.clear ();
  m_num_items_by_cell_and_category.clear ();
  m_num_items_visited_by_cell_and_category.clear ();
  m_items_by_cell_id.clear ();
  m_items_by_category_id.clear ();
  m_num_items = 0;
  m_num_items_visited = 0;

  for (std::map <id_type, Category *>::iterator c = m_categories_by_id.begin (); c != m_categories_by_id.end (); ++c) {
    c->second->reset_num_items ();
  }

  for (std::map <id_type, Cell *>::iterator c = m_cells_by_id.begin (); c != m_cells_by_id.end (); ++c) {
    c->second->reset_num_items ();
  }

  for (Items::iterator i = mp_items->begin (); i != mp_items->end (); ++i) {

    ++m_num_items;

    if (i->visited ()) {
      ++m_num_items_visited;
    }

    id_type cell_id = i->cell_id ();
    Cell *cell = cell_by_id_non_const (cell_id);

    id_type category_id = i->category_id ();
    Category *category = category_by_id_non_const (category_id);

    if (cell != 0 && category != 0) {

      cell->add_to_num_items (1);

      m_items_by_cell_id.insert (std::make_pair (cell_id, std::list<ItemRef> ())).first->second.push_back (ItemRef (&*i));

      if (i->visited ()) {
        cell->add_to_num_items_visited (1);
      }

      m_items_by_category_id.insert (std::make_pair (category_id, std::list<ItemRef> ())).first->second.push_back (ItemRef (&*i));
      m_items_by_cell_and_category_id.insert (std::make_pair (std::make_pair (cell_id, category_id), std::list<ItemRef> ())).first->second.push_back (ItemRef (&*i));

      while (category) {

        m_num_items_by_cell_and_category.insert (std::make_pair (std::make_pair (cell_id, category->id ()), 0)).first->second += 1;
        category->add_to_num_items (1);

        if (i->visited ()) {
          m_num_items_visited_by_cell_and_category.insert (std::make_pair (std::make_pair (cell_id, category->id ()), 0)).first->second += 1;
          category->add_to_num_items_visited (1);
        }

        category = category->parent ();

      }

    }

  }
}

void
Database::import_tags (const Tags &tags)
{
  set_modified ();

  for (Tags::const_iterator t = tags.begin_tags (); t != tags.end_tags (); ++t) {
    m_tags.import_tag (*t);
  }
}

void 
Database::import_categories (Categories *categories)
{
  set_modified ();

  delete mp_categories;

  mp_categories = categories;
  mp_categories->set_database (this);
}

void 
Database::import_cells (const Cells &cells)
{
  set_modified ();

  for (Cells::const_iterator c = cells.begin (); c != cells.end (); ++c) {
    m_cells.import_cell (*c);
  }
}

Category *
Database::create_category (Category *parent, const std::string &name)
{
  set_modified ();

  Category *cat = create_category (&parent->sub_categories (), name);
  cat->set_parent (parent);
  return cat;
}

Category *
Database::create_category (const std::string &name)
{
  set_modified ();

  return create_category (mp_categories, name);
}

Category *
Database::create_category (Categories *container, const std::string &name)
{
  set_modified ();

  Category *cat = new Category (name);
  cat->set_id (++m_next_id);

  m_categories_by_id.insert (std::make_pair (m_next_id, cat));
  container->add_category (cat);

  return cat;
}

Category *
Database::category_by_name_non_const (const std::string &name) 
{
  return mp_categories->category_by_name (name.c_str ());
}

Category *
Database::category_by_id_non_const (id_type id) 
{
  std::map <id_type, Category *>::const_iterator c = m_categories_by_id.find (id);
  if (c != m_categories_by_id.end ()) {
    return c->second;
  } else {
    return 0;
  }
}

Cell *
Database::create_cell (const std::string &name, const std::string &variant)
{
  set_modified ();

  Cell *new_cell;

  if (variant.empty ()) {

    std::map <std::string, std::vector <id_type> >::iterator variant = m_cell_variants.find (name);

    //  If another cell with that name already exists, rename it to variant "cell:1":
    Cell *other_cell = cell_by_qname_non_const (name);
    if (other_cell != 0) {

      other_cell->set_variant ("1");
      variant = m_cell_variants.insert (std::make_pair (name, std::vector <id_type> ())).first;
      variant->second.push_back (other_cell->id ());
      m_cells_by_qname.erase (name);

      m_cells_by_qname.insert (std::make_pair (other_cell->qname (), other_cell));

    }

    //  If that cell name is marked for use with variants, create a new variant
    if (variant != m_cell_variants.end ()) {

      std::string qname;

      //  Try to find a suitable variant id
      unsigned int variant_index = 0;
      for (unsigned int n = (std::numeric_limits<unsigned int>::max () / 2) + 1; n > 0; n /= 2) {
        qname = name + ":" + tl::to_string (variant_index + n);
        if (m_cells_by_qname.find (qname) != m_cells_by_qname.end ()) {
          variant_index += n;
        }
      }

      new_cell = new Cell (++m_next_id, name, tl::to_string (variant_index + 1));

      variant->second.push_back (new_cell->id ());

    } else {

      new_cell = new Cell (++m_next_id, name);
      
    }

    m_cells.add_cell (new_cell);
    m_cells_by_id.insert (std::make_pair (new_cell->id (), new_cell));
    m_cells_by_qname.insert (std::make_pair (new_cell->qname (), new_cell));

  } else {
      
    new_cell = new Cell (++m_next_id, name, variant);
    m_cells.add_cell (new_cell);
    m_cells_by_id.insert (std::make_pair (new_cell->id (), new_cell));
    m_cells_by_qname.insert (std::make_pair (new_cell->qname (), new_cell));

    std::map <std::string, std::vector <id_type> >::iterator variant = m_cell_variants.insert (std::make_pair (name, std::vector <id_type> ())).first;

    //  If another cell with that name already exists, rename it to a suitable variant
    Cell *other_cell = cell_by_qname_non_const (name);
    if (other_cell != 0) {

      variant->second.push_back (other_cell->id ());

      m_cells_by_qname.erase (name);

      std::string qname;

      //  Try to find a suitable variant id
      unsigned int variant_index = 0;
      for (unsigned int n = (std::numeric_limits<unsigned int>::max () / 2) + 1; n > 0; n /= 2) {
        qname = name + ":" + tl::to_string (variant_index + n);
        if (m_cells_by_qname.find (qname) != m_cells_by_qname.end ()) {
          variant_index += n;
        }
      }

      other_cell->set_variant (tl::to_string (variant_index + 1));
      
      m_cells_by_qname.insert (std::make_pair (other_cell->qname (), other_cell));

    }

    variant->second.push_back (new_cell->id ());

  }

  return new_cell;
}

const std::vector <id_type> &
Database::variants (const std::string &name)
{
  std::map <std::string, std::vector <id_type> >::iterator variant = m_cell_variants.find (name);
  if (variant != m_cell_variants.end ()) {
    return variant->second;
  } else {
    static std::vector <id_type> empty_vector;
    return empty_vector;
  }
}

Cell *
Database::cell_by_qname_non_const (const std::string &qname)
{
  std::map <std::string, Cell *>::const_iterator c = m_cells_by_qname.find (qname);
  if (c != m_cells_by_qname.end ()) {
    return c->second;
  } else {
    return 0;
  }
}

Cell *
Database::cell_by_id_non_const (id_type id)
{
  std::map <id_type, Cell *>::const_iterator c = m_cells_by_id.find (id);
  if (c != m_cells_by_id.end ()) {
    return c->second;
  } else {
    return 0;
  }
}

void 
Database::set_tag_description (id_type tag_id, const std::string &description)
{
  set_modified ();
  tags_non_const ().tag (tag_id).set_description (description);
}

void  
Database::add_item_tag (const Item *item, id_type tag)
{
  set_modified ();
  const_cast <Item *> (item)->add_tag (tag);
}

void  
Database::remove_item_tag (const Item *item, id_type tag)
{
  set_modified ();
  const_cast <Item *> (item)->remove_tag (tag);
}

#if defined(HAVE_QT)
void 
Database::set_item_image (const Item *item, const QImage &image)
{
  set_modified ();
  const_cast <Item *> (item)->set_image (image);
}
#endif

void
Database::set_item_image_str (const Item *item, const std::string &image_str)
{
  set_modified ();
  const_cast <Item *> (item)->set_image_str (image_str);
}

void 
Database::set_item_multiplicity (const Item *item, size_t n)
{
  set_modified ();
  const_cast <Item *> (item)->set_multiplicity (n);
}

void 
Database::set_item_visited (const Item *item_c, bool visited)
{
  if (visited != item_c->visited ()) {

    Item *item = const_cast <Item *> (item_c);

    set_modified ();

    item->set_visited (visited);

    Cell *cell = cell_by_id_non_const (item->cell_id ());
    if (cell) {
      cell->add_to_num_items_visited (visited ? 1 : -1);
    }

    m_num_items_visited += (visited ? 1 : -1);

    rdb::Category *cat = category_by_id_non_const (item->category_id ());
    while (cat != 0) {
      cat->add_to_num_items_visited (visited ? 1 : -1);
      m_num_items_visited_by_cell_and_category.insert (std::make_pair (std::make_pair (item->cell_id (), cat->id ()), 0)).first->second += (visited ? 1 : -1);
      cat = cat->parent ();
    }

  }
}

Item *
Database::create_item (id_type cell_id, id_type category_id)
{
  set_modified ();

  m_num_items += 1;

  Cell *cell = cell_by_id_non_const (cell_id);
  tl_assert (cell != 0);

  cell->m_num_items += 1;

  Category *category = category_by_id_non_const (category_id);
  while (category != 0) {
    category->m_num_items += 1;
    m_num_items_by_cell_and_category.insert (std::make_pair (std::make_pair (cell_id, category->id ()), 0)).first->second += 1;
    category = category->parent ();
  }

  mp_items->add_item (Item ());
  Item *item = &mp_items->back ();
  item->set_cell_id (cell_id);
  item->set_category_id (category_id);

  m_items_by_cell_id.insert (std::make_pair (cell_id, std::list<ItemRef> ())).first->second.push_back (ItemRef (item));
  m_items_by_category_id.insert (std::make_pair (category_id, std::list<ItemRef> ())).first->second.push_back (ItemRef (item));
  m_items_by_cell_and_category_id.insert (std::make_pair (std::make_pair (cell_id, category_id), std::list<ItemRef> ())).first->second.push_back (ItemRef (item));

  return item;
}

static std::list<ItemRef> empty_list;

std::pair<Database::const_item_ref_iterator, Database::const_item_ref_iterator> 
Database::items_by_cell_and_category (id_type cell_id, id_type category_id) const
{
  std::map <std::pair <id_type, id_type>, std::list<ItemRef> >::const_iterator i = m_items_by_cell_and_category_id.find (std::make_pair (cell_id, category_id));
  if (i != m_items_by_cell_and_category_id.end ()) {
    return std::make_pair (i->second.begin (), i->second.end ());
  } else {
    return std::make_pair (empty_list.begin (), empty_list.end ());
  }
}

std::pair<Database::const_item_ref_iterator, Database::const_item_ref_iterator> 
Database::items_by_cell (id_type cell_id) const
{
  std::map <id_type, std::list<ItemRef> >::const_iterator i = m_items_by_cell_id.find (cell_id);
  if (i != m_items_by_cell_id.end ()) {
    return std::make_pair (i->second.begin (), i->second.end ());
  } else {
    return std::make_pair (empty_list.begin (), empty_list.end ());
  }
}

std::pair<Database::const_item_ref_iterator, Database::const_item_ref_iterator> 
Database::items_by_category (id_type category_id) const
{
  std::map <id_type, std::list<ItemRef> >::const_iterator i = m_items_by_category_id.find (category_id);
  if (i != m_items_by_category_id.end ()) {
    return std::make_pair (i->second.begin (), i->second.end ());
  } else {
    return std::make_pair (empty_list.begin (), empty_list.end ());
  }
}

size_t 
Database::num_items (id_type cell_id, id_type category_id) const
{
  std::map <std::pair <id_type, id_type>, size_t>::const_iterator n = m_num_items_by_cell_and_category.find (std::make_pair (cell_id, category_id));
  if (n != m_num_items_by_cell_and_category.end ()) {
    return n->second;
  } else {
    return 0;
  }
}

size_t 
Database::num_items_visited (id_type cell_id, id_type category_id) const
{
  std::map <std::pair <id_type, id_type>, size_t>::const_iterator n = m_num_items_visited_by_cell_and_category.find (std::make_pair (cell_id, category_id));
  if (n != m_num_items_visited_by_cell_and_category.end ()) {
    return n->second;
  } else {
    return 0;
  }
}

void
Database::clear ()
{
  set_modified ();

  m_generator = "";
  m_filename = "";
  m_description = "";
  m_name = "";
  m_topcell = "";
  m_next_id = 0;
  m_tags.clear ();
  m_cells_by_qname.clear ();
  m_cell_variants.clear ();
  m_cells_by_id.clear ();
  m_categories_by_id.clear ();
  m_items_by_cell_and_category_id.clear ();
  m_num_items_by_cell_and_category.clear ();
  m_num_items_visited_by_cell_and_category.clear ();
  m_items_by_cell_id.clear ();
  m_items_by_category_id.clear ();
  m_cells.clear ();
  m_num_items = 0;
  m_num_items_visited = 0;

  delete mp_items;
  mp_items = new Items ();
  mp_items->set_database (this);

  delete mp_categories;
  mp_categories = new Categories ();
  mp_categories->set_database (this);
}

void
Database::load (const std::string &fn)
{
  tl::log << "Loading RDB from " << fn;

  tl::InputStream stream (fn);
  rdb::Reader reader (stream);

  clear ();
  reader.read (*this);

  set_filename (stream.absolute_path ());
  set_name (stream.filename ());

  reset_modified ();

  if (tl::verbosity () >= 10) {
    tl::info << "Loaded RDB from " << fn;
  }
}

} // namespace rdb

