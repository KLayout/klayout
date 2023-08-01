
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


#ifndef HDR_rdb
#define HDR_rdb

#include "rdbCommon.h"

#include "dbTrans.h"
#include "gsi.h"
#include "tlObject.h"
#include "tlObjectCollection.h"
#include "tlPixelBuffer.h"

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>

#if defined(HAVE_QT)
class QImage;
#endif

namespace tl
{
  class Extractor;
}

namespace db
{
  class Shape;
}

namespace rdb
{

typedef size_t id_type;

class References;
class Categories;
class Database;
class Cells;
class Cell;
class Items;

/**
 *  @brief A report item's category
 *
 *  An item is member of exactly one category. This can be a check for example. 
 *  A category is described by a name and a description string. An Id is provided
 *  to reference this category from actual report items.
 *  Categories can be organized hierarchically for which a category collection 
 *  is provided and member of the individual category.
 *
 *  A category can only be created by the database object, since the
 *  database object issues id's.
 */
class RDB_PUBLIC Category
  : public tl::Object
{
public:
  /**
   *  @brief The default constructor with a category reference
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  Use Database::create_category instead.
   */
  Category (Categories *categories);

  /**
   *  @brief The default constructor
   */
  Category ();

  /**
   *  @brief Destructor
   */
  ~Category ();

  /**
   *  @brief The Id of the category (getter)
   */
  id_type id () const
  {
    return m_id;
  }

  /**
   *  @brief The Id of the category (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_id (id_type id) 
  {
    m_id = id;
  }

  /**
   *  @brief The name string (getter)
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief The the name string (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_name (const std::string &d) 
  {
    m_name = d;
  }

  /**
   *  @brief Get the path of this category
   *
   *  The path contains all components leading to this category starting from a top level
   *  category. The path string can be used with category_by_name to localize the current
   *  category.
   */
  std::string path () const;

  /**
   *  @brief The description string (getter)
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief The description string (setter)
   */
  void set_description (const std::string &d) 
  {
    m_description = d;
  }

  /**
   *  @brief The collection of sub-categories (const access)
   */
  const Categories &sub_categories () const;

  /**
   *  @brief The collection of sub-categories
   */
  Categories &sub_categories ();

  /**
   *  @brief The parent (owner) of this category (getter)
   */
  const Category *parent () const
  {
    return mp_parent;
  }

  /**
   *  @brief The parent (owner) of this category (getter)
   */
  Category *parent () 
  {
    return mp_parent;
  }

  /**
   *  @brief Import categories as sub categories.
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  The Category object will take over ownership over the sub categories.
   */
  void import_sub_categories (Categories *categories); 

  /**
   *  @brief Report the number of items
   */
  size_t num_items () const
  {
    return m_num_items;
  }

  /**
   *  @brief Report the number of items visited
   */
  size_t num_items_visited () const
  {
    return m_num_items_visited;
  }

  /**
   *  @brief Get the database reference
   */
  Database *database ()
  {
    return mp_database;
  }

  /**
   *  @brief Get the database reference (const version)
   */
  const Database *database () const
  {
    return mp_database;
  }

private:
  friend class Database;
  friend class Categories;

  id_type m_id;
  std::string m_name, m_description;
  Category *mp_parent;
  Categories *mp_sub_categories;
  size_t m_num_items;
  size_t m_num_items_visited;
  Database *mp_database;

  /**
   *  @brief Default constructor
   *
   *  Creates a category object with empty name and description.
   *  This constructor is private to allow only the database to produce a 
   *  category object.
   */
  Category (const std::string &name);

  /**
   *  @brief The parent (owner) of this category (setter)
   */
  void set_parent (Category *parent_category)
  {
    mp_parent = parent_category;
  }

  /**
   *  @brief Add an offset to the number of items visited
   */
  void add_to_num_items_visited (int d)
  {
    m_num_items_visited += d;
  }
  
  /**
   *  @brief Add an offset to the number of items 
   */
  void add_to_num_items (int d)
  {
    m_num_items += d;
  }

  /**
   *  @brief Reset the number of items
   */
  void reset_num_items ()
  {
    m_num_items = 0;
    m_num_items_visited = 0;
  }
  
  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database);

  //  no copying, no default ctor
  Category (const Category &d);
  Category &operator= (const Category &d);
};

/**
 *  @brief The collection of categories
 *
 *  A generic collection of categories used for the root node and 
 *  sub-category nodes.
 */
class RDB_PUBLIC Categories 
{
public:
  typedef tl::shared_collection<Category>::const_iterator const_iterator;
  typedef tl::shared_collection<Category>::iterator iterator;

  /**
   *  @brief Constructor with a database reference
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Categories (Database *database)
    : mp_database (database)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Constructor with a parent category reference
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Categories (Category *category)
    : mp_database (category->database ())
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Iterate the categories inside this collection (begin iterator)
   */
  const_iterator begin () const 
  { 
    return m_categories.begin (); 
  }

  /**
   *  @brief Iterate the categories inside this collection (end iterator)
   */
  const_iterator end () const 
  { 
    return m_categories.end (); 
  }

  /**
   *  @brief Iterate the categories inside this collection (begin iterator)
   */
  iterator begin () 
  { 
    return m_categories.begin (); 
  }

  /**
   *  @brief Iterate the categories inside this collection (end iterator)
   */
  iterator end () 
  { 
    return m_categories.end (); 
  }

  /**
   *  @brief Find a category by name
   *
   *  The name is actually a path expression which specifies the category starting from the given 
   *  node with a '.' separated path notation. I.e. 'a.b' is the sub-category 'b' of category 'a'.
   *  If no such category can be found, 0 is returned.
   */
  const Category *category_by_name (const char *path) const
  {
    return (const_cast <Categories *> (this)->category_by_name (path));
  }

  /**
   *  @brief Find a category by name (non-const version)
   *
   *  The name is actually a path expression which specifies the category starting from the given 
   *  node with a '.' separated path notation. I.e. 'a.b' is the sub-category 'b' of category 'a'.
   *  If no such category can be found, 0 is returned.
   */
  Category *category_by_name (const char *path);

  /**
   *  @brief Clear the collection of categories
   */
  void clear ();

  /**
   *  @brief Import a category
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  This will take over ownership over the category.
   */
  void import_category (Category *category); 

  /**
   *  @brief Gets the database reference
   */
  Database *database ()
  {
    return mp_database.get ();
  }

public:
  friend class Database;
  friend class Category;

  tl::shared_collection<Category> m_categories;
  std::map <std::string, Category *> m_categories_by_name; 
  tl::weak_ptr<Database> mp_database;

  Categories ()
    : mp_database (0)
  {
    // .. nothing yet ..
  }

  void add_category (Category *cath);

  void set_database (Database *database);
};

/**
 *  @brief This is the base class of any value associated with a marker item
 *  A value has a value (as the name says) and an optional tag id. Tag id's identify
 *  the value and make the value a named one.
 */
class RDB_PUBLIC ValueBase 
{
public:
  ValueBase ()
  {
    //  .. nothing yet ..
  }

  virtual ~ValueBase ()
  {
    //  .. nothing yet ..
  }

  virtual std::string to_string () const = 0;

  virtual std::string to_display_string () const = 0;

  virtual bool is_shape () const = 0;

  virtual ValueBase *clone () const = 0;

  virtual int type_index () const = 0;

  virtual bool compare (const ValueBase *other) const = 0;

  static bool compare (const ValueBase *a, const ValueBase *b);

  static ValueBase *create_from_string (const std::string &s);

  static ValueBase *create_from_string (tl::Extractor &ex);

  static ValueBase *create_from_shape (const db::Shape &shape, const db::CplxTrans &trans);
};

/**
 *  @brief A type identifier
 *
 *  This template function associates a type with an integer for the value.
 */
template <class T>
int type_index_of ();

/**
 *  @brief A generic class wrapped into a ValueBase
 *
 *  Using this class, any value can be stored inside the collection of Values.
 */
template <class C>
class RDB_PUBLIC_TEMPLATE Value
  : public ValueBase
{
public:
  Value ()
    : m_value ()
  {
    //  .. nothing yet ..
  }

  Value (const C &value)
    : m_value (value)
  {
    //  .. nothing yet ..
  }

  void set_value (const C &value)
  {
    m_value = value;
  }

  const C &value () const
  {
    return m_value;
  }

  C &value () 
  {
    return m_value;
  }

  int type_index () const 
  { 
    return type_index_of<C> (); 
  }

  bool compare (const ValueBase *other) const 
  {
    return m_value < static_cast<const Value<C> *> (other)->m_value;
  }

  bool is_shape () const;

  std::string to_string () const;

  std::string to_display_string () const;

  ValueBase *clone () const
  {
    return new Value<C> (m_value);
  }

private:
  C m_value;
};

/**
 *  @brief Type bindings
 */
template <class T>
RDB_PUBLIC_TEMPLATE ValueBase *make_value (const T &value)
{
  return new Value<T> (value);
}

/**
 *  @brief A class encapsulating ValueBase pointer
 */
class RDB_PUBLIC ValueWrapper 
{
public:
  /**
   *  @brief Default constructor
   */
  ValueWrapper ()
    : mp_ptr (0), m_tag_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from an existing object
   */
  ValueWrapper (ValueBase *ptr)
    : mp_ptr (ptr), m_tag_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy constructor
   */
  ValueWrapper (const ValueWrapper &d)
    : mp_ptr (d.mp_ptr ? d.mp_ptr->clone () : 0), m_tag_id (d.m_tag_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~ValueWrapper ()
  {
    set (0);
  }

  /**
   *  @brief Assignment
   */
  ValueWrapper &operator= (const ValueWrapper &d)
  {
    if (this != &d) {
      if (mp_ptr) {
        delete mp_ptr;
      }
      mp_ptr = (d.mp_ptr ? d.mp_ptr->clone () : 0);
      m_tag_id = d.m_tag_id;
    }

    return *this;
  }

  /**
   *  @brief Get the pointer
   */
  ValueBase *get () 
  {
    return mp_ptr;
  }

  /**
   *  @brief Get the pointer (const)
   */
  const ValueBase *get () const
  {
    return mp_ptr;
  }

  /**
   *  @brief Set the pointer
   */
  void set (ValueBase *ptr)
  {
    if (mp_ptr) {
      delete mp_ptr;
    }
    mp_ptr = ptr;
  }

  void set_tag_id (id_type id)
  {
    m_tag_id = id;
  }

  id_type tag_id () const
  {
    return m_tag_id;
  }

  /**
   *  @brief Convert the values collection to a string 
   */
  std::string to_string (const Database *rdb = 0) const;

  /**
   *  @brief Fill the values collection from the string
   */
  void from_string (Database *rdb, const std::string &s);  

  /**
   *  @brief Fill the values collection from an extractor
   */
  void from_string (Database *rdb, tl::Extractor &ex);  

private:
  ValueBase *mp_ptr;
  id_type m_tag_id;
};

/**
 *  @brief A collection of value objects for a RDB item
 */
class RDB_PUBLIC Values
{
public:
  typedef std::list<ValueWrapper>::const_iterator const_iterator;
  typedef std::list<ValueWrapper>::iterator iterator;

  /**
   *  @brief The default constructor
   */
  Values ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy constructor
   */
  Values (const Values &d)
  {
    operator= (d);
  }

  /**
   *  @brief Assignment 
   */
  Values &operator= (const Values &d);

  /**
   *  @brief The const iterator (begin)
   */
  const_iterator begin () const
  {
    return m_values.begin ();
  }

  /**
   *  @brief The const iterator (end)
   */
  const_iterator end () const
  {
    return m_values.end ();
  }

  /**
   *  @brief The non-const iterator (begin)
   */
  iterator begin () 
  {
    return m_values.begin ();
  }

  /**
   *  @brief The non-const iterator (end)
   */
  iterator end ()
  {
    return m_values.end ();
  }

  /**
   *  @brief Add a new value
   *
   *  The values collection will become owner of the pointer given.
   */
  void add (ValueBase *value, id_type tag_id = 0)
  {
    m_values.push_back (ValueWrapper ());
    m_values.back ().set (value);
    m_values.back ().set_tag_id (tag_id);
  }

  /**
   *  @brief Add a new value from a wrapper
   */
  void add (const ValueWrapper &value)
  {
    m_values.push_back (value);
  }

  /**
   *  @brief Swaps the values with other values
   */
  void swap (Values &other)
  {
    m_values.swap (other.m_values);
  }

  /**
   *  @brief Convert the values collection to a string 
   */
  std::string to_string (const Database *rdb) const;

  /**
   *  @brief Fill the values collection from the string
   */
  void from_string (Database *rdb, const std::string &s);  

private:
  std::list <ValueWrapper> m_values;
};

/**
 *  @brief A report item
 *
 *  A report item is one information item in the report. 
 *  The value of a report item is manyfold. Values can be keyed, 
 *  i.e. multiple values can be present with different keys.
 *  Each value can be of different types where the type is specified by a type Id.
 */
class RDB_PUBLIC Item
  : public tl::Object
{
public:
  /**
   *  @brief Construct an item from a items reference
   */
  Item (Items *items);

  /**
   *  @brief Copy constructor 
   */
  Item (const Item &d);

  /**
   *  @brief Assignment 
   */
  Item &operator= (const Item &d);

  /**
   *  @brief Destructor
   */
  ~Item ();

  /**
   *  @brief Get the cell that this item is located inside (by id)
   */
  id_type cell_id () const
  {
    return m_cell_id;
  }

  /**
   *  @brief The cell id (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_cell_id (id_type id) 
  {
    m_cell_id = id;
  }

  /**
   *  @brief Get the cell that this item is located inside (by name or name:variant combination)
   */
  std::string cell_qname () const;

  /**
   *  @brief The cell name or name:variant combination (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_cell_qname (const std::string &qname);

  /**
   *  @brief Get the category that this item is located inside (by id)
   */
  id_type category_id () const
  {
    return m_category_id;
  }

  /**
   *  @brief The category Id of the category (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_category_id (id_type id) 
  {
    m_category_id = id;
  }

  /**
   *  @brief Get the category that this item is located inside (by name)
   */
  std::string category_name () const;

  /**
   *  @brief The category name (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_category_name (const std::string &name);

  /**
   *  @brief The list of values of this item
   */
  const Values &values () const
  {
    return m_values;
  }

  /**
   *  @brief The list of values of this item (non-const version)
   */
  Values &values ()
  {
    return m_values;
  }

  /**
   *  @brief Add a value in a generic way
   */
  template <class V>
  ValueBase *add_value (const V &v, id_type tag_id = 0)
  {
    ValueBase *value = new Value<V> (v);
    values ().add (value, tag_id);
    return value;
  }

  /**
   *  @brief Set the list of values
   */
  void set_values (const Values &values)
  {
    m_values = values;
  }

  /**
   *  @brief Set the multiplicity of the item
   */
  void set_multiplicity (size_t n)
  {
    m_multiplicity = n;
  }

  /**
   *  @brief Get the multiplicity of this item
   */
  size_t multiplicity () const
  {
    return m_multiplicity;
  }

  /**
   *  @brief Returns true if the item was visited already.
   *
   *  This property should be changed with rdb::Database::set_item_visited.
   */
  bool visited () const
  {
    return m_visited;
  }

  /**
   *  @brief Directly set the visited flag
   *
   *  This method must not be used for items already in the database to keep
   *  the database consistent. Use rdb::Database::set_item_visited instead.
   */
  void set_visited (bool v)
  {
    m_visited = v;
  }

  /**
   *  @brief Add a tag to this item
   */
  void add_tag (id_type tag_id);

  /**
   *  @brief Remove a tag from this item
   */
  void remove_tag (id_type tag_id);

  /**
   *  @brief Remove all tags of this item
   */
  void remove_tags ();

  /**
   *  @brief Check, if this item has the given tag
   */
  bool has_tag (id_type tag_id) const;

  /**
   *  @brief Get the tags for this item by string
   */
  std::string tag_str () const;

  /**
   *  @brief Set the tags for this item from a string
   */
  void set_tag_str (const std::string &tags);

#if defined(HAVE_QT)
  /**
   *  @brief Gets the image object attached to this item
   *
   *  @return The image object or an empty image if no image is attached
   */
  QImage image () const;

  /**
   *  @brief Set the image for this item
   */
  void set_image (const QImage &image);
#endif

#if defined(HAVE_PNG)
  /**
   *  @brief Gets the image as a tl::PixelBuffer object
   */
  tl::PixelBuffer image_pixels () const;

  /**
   *  @brief Sets the image from a tl::PixelBuffer object
   */
  void set_image (const tl::PixelBuffer &image);
#endif

  /**
   *  @brief Gets a value indicating whether the item has an image
   */
  bool has_image () const;

  /**
   *  @brief Gets the image as a string (PNG, base64 coded)
   *
   *  Note: if neither PNG support for Qt are compiled in, this string will be empty
   */
  std::string image_str () const;

  /**
   *  @brief Gets the image from a string (PNG, base64 coded)
   *
   *  If the image string is empty, the image will be cleared.
   *  If the image string is not valid, the image will also be cleared.
   */
  void set_image_str (const std::string &s);

  /**
   *  @brief Get the database reference
   */
  Database *database ()
  {
    return mp_database;
  }

  /**
   *  @brief Get the database reference (const version)
   */
  const Database *database () const
  {
    return mp_database;
  }

private:
  friend class Database;
  friend class Items;

  Values m_values;
  id_type m_cell_id;
  id_type m_category_id;
  size_t m_multiplicity;
  bool m_visited;
  std::vector <bool> m_tag_ids;
  Database *mp_database;
  std::string m_image_str;

  Item ();

  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database)
  {
    mp_database = database;
  }
};

/**
 *  @brief An item reference 
 *
 *  This is basically a wrapper for a pointer that correctly 
 *  maps const * and non-const * values through the operator->
 *  overloads.
 */
class RDB_PUBLIC ItemRef
{
public:
  /**
   *  @brief Constructor
   */
  ItemRef (Item *item)
    : mp_item (item)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief non-const access
   */
  Item *operator-> ()
  {
    return mp_item;
  }

  /**
   *  @brief non-const reference access
   */
  Item &operator* ()
  {
    return *mp_item;
  }

  /**
   *  @brief const access
   */
  const Item *operator-> () const
  {
    return mp_item;
  }

  /**
   *  @brief const reference access
   */
  const Item &operator* () const
  {
    return *mp_item;
  }

public:
  Item *mp_item;
};

/**
 *  @brief A container for items
 *
 *  This container is owned by the database.
 */
class RDB_PUBLIC Items
{
public:
  typedef std::list<Item>::const_iterator const_iterator;
  typedef std::list<Item>::iterator iterator;

  /**
   *  @brief Construct an item list with a database reference
   */
  Items (Database *database)  
    : mp_database (database)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Iterate the items inside this collection (begin iterator)
   */
  const_iterator begin () const 
  { 
    return m_items.begin (); 
  }

  /**
   *  @brief Iterate the items inside this collection (end iterator)
   */
  const_iterator end () const 
  { 
    return m_items.end (); 
  }

  /**
   *  @brief Iterate the items inside this collection (non-const begin iterator)
   */
  iterator begin ()
  { 
    return m_items.begin (); 
  }

  /**
   *  @brief Iterate the items inside this collection (non-const end iterator)
   */
  iterator end ()
  { 
    return m_items.end (); 
  }

  /**
   *  @brief Add an item
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void add_item (const Item &item)
  {
    m_items.push_back (item);
    m_items.back ().set_database (mp_database);
  }

  /**
   *  @brief Get a reference to the last item
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Item &back () 
  {
    return m_items.back ();
  }

  /**
   *  @brief Get the database reference
   */
  Database *database () 
  {
    return mp_database;
  }

private:
  friend class Cell;
  friend class Database;

  std::list <Item> m_items;
  Database *mp_database;

  Items (const Items &d);
  Items &operator= (const Items &d);

  Items ()
    : mp_database (0)
  {
    // .. nothing yet ..
  }

  void set_database (Database *database)
  {
    mp_database = database;
  }
};

/**
 *  @brief A reference description
 *
 *  A reference describes how a cell is located inside a parent cell.
 *  The reference is not meant to replace a full hierarchy graph. Instead
 *  it is intended as a hint how to display a marker in a top level context
 */
class RDB_PUBLIC Reference
{
public:
  /**
   *  @brief Constructs a reference with a unit transformation and a empty parent cell id
   */
  Reference ()
    : m_trans (), m_parent_cell_id (0), mp_database (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructs a reference in the context of a References collection
   */
  Reference (References *references);

  /**
   *  @brief Constructs a reference with the given transformation and the given parent cell id
   *
   *  @param trans The transformation which transforms anything of this cell into the parent cell.
   */
  Reference (const db::DCplxTrans &trans, id_type parent_cell_id)
    : m_trans (trans), m_parent_cell_id (parent_cell_id), mp_database (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Set the parent cell id
   */
  void set_parent_cell_id (id_type id)
  {
    m_parent_cell_id = id;
  }

  /**
   *  @brief Get the parent cell id
   */
  id_type parent_cell_id () const
  {
    return m_parent_cell_id;
  }

  /**
   *  @brief Set the parent cell name (plus optionally the variant id separated by a colon - i.e. cell:var1)
   */
  void set_parent_cell_qname (const std::string &qname);

  /**
   *  @brief Get the transformation as string (plus optionally the variant id separated by a colon - i.e. cell:var1)
   */
  std::string parent_cell_qname () const;

  /**
   *  @brief Set the transformation
   */
  void set_trans (const db::DCplxTrans &trans)
  {
    m_trans = trans;
  }

  /**
   *  @brief Get the transformation
   */
  const db::DCplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Set the transformation as string
   */
  void set_trans_str (const std::string &str);

  /**
   *  @brief Get the transformation as string
   */
  std::string trans_str () const;

  /**
   *  @brief Get the database reference
   */
  Database *database ()
  {
    return mp_database;
  }

  /**
   *  @brief Get the database reference (const version)
   */
  const Database *database () const
  {
    return mp_database;
  }

private:
  friend class References;

  db::DCplxTrans m_trans;
  id_type m_parent_cell_id;
  Database *mp_database;

  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database)
  {
    mp_database = database;
  }
};

/**
 *  @brief A collection of references 
 */
class RDB_PUBLIC References
{
public:
  typedef std::vector<Reference>::const_iterator const_iterator;
  typedef std::vector<Reference>::iterator iterator;

  /**
   *  @brief Create a References object for a cell
   */
  References (Cell *cell);

  /**
   *  @brief Add a reference to this collection
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void insert (const Reference &ref)
  {
    m_references.push_back (ref);
    m_references.back ().set_database (mp_database);
  }

  /**
   *  @brief Begin iterator (const)
   */
  const_iterator begin () const
  {
    return m_references.begin ();
  }

  /**
   *  @brief End iterator (const)
   */
  const_iterator end () const
  {
    return m_references.end ();
  }

  /**
   *  @brief Begin iterator (non-const)
   */
  iterator begin () 
  {
    return m_references.begin ();
  }

  /**
   *  @brief End iterator (non-const)
   */
  iterator end () 
  {
    return m_references.end ();
  }

  /**
   *  @brief Clear this collection
   */
  void clear ()
  {
    m_references.clear ();
  }

  /**
   *  @brief Get the database reference
   */
  Database *database () 
  {
    return mp_database;
  }

private:
  friend class Database;
  friend class Cell;

  std::vector<Reference> m_references;
  Database *mp_database;

  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database);

  References ();

  //  no copying, no default ctor
  References (const References &d);
  References &operator= (const References &d);
};

/**
 *  @brief A class describing a cell
 *
 *  A cell has a collection of items, a unique id and a name.
 *  A cell can only be create by the database.
 */
class RDB_PUBLIC Cell
  : public tl::Object
{
public:
  typedef std::vector<Reference> references_list_type;
  typedef std::vector<Reference>::const_iterator reference_iterator;

  /**
   *  @brief The default constructor
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cell ();

  /**
   *  @brief The constructor taking the database reference from the Cells collection
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cell (Cells *cells);

  /**
   *  @brief the constructor from an id and a name
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cell (id_type id, const std::string &name);

  /**
   *  @brief the constructor from an id and a name and a variant id
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cell (id_type id, const std::string &name, const std::string &variant);

  /**
   *  @brief Cell destructor
   */
  ~Cell ();

  /**
   *  @brief Get the cell id
   */
  id_type id () const
  {
    return m_id;
  }

  /**
   *  @brief The Id of the category (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_id (id_type id) 
  {
    m_id = id;
  }

  /**
   *  @brief Get the cell name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Set the name string (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_name (const std::string &d) 
  {
    m_name = d;
  }

  /**
   *  @brief Get the variant id
   */
  const std::string &variant () const
  {
    return m_variant;
  }

  /**
   *  @brief Set the variant string (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_variant (const std::string &v) 
  {
    m_variant = v;
  }

  /**
   *  @brief Get the qualified name (name plus optionally the variant id separated by a colon)
   */
  std::string qname () const;

  /**
   *  @brief Report the number of items in total
   */
  size_t num_items () const
  {
    return m_num_items;
  }

  /**
   *  @brief Report the number of items visited
   */
  size_t num_items_visited () const
  {
    return m_num_items_visited;
  }

  /**
   *  @brief The reference collection (const)
   */
  const References &references () const
  {
    return m_references;
  }

  /**
   *  @brief The reference collection (non-const)
   */
  References &references () 
  {
    return m_references;
  }

  /**
   *  @brief Import a set of references
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void import_references (const References &references);

  /**
   *  @brief Get one example transformation leading from this cell to a given parent cell 
   *
   *  This method will try to determine one path from the given cell to the given parent
   *  cell and return the accumulated transformation for this path. 
   *  If no path is found, the first parameter of the returned pair is false, otherwise it's
   *  true.
   */
  std::pair<bool, db::DCplxTrans> path_to (id_type parent_cell_id, const rdb::Database *db) const;

  /**
   *  @brief Get the database reference
   */
  Database *database () 
  {
    return mp_database;
  }

  /**
   *  @brief Get the database reference (const version)
   */
  const Database *database () const
  {
    return mp_database;
  }

private:
  friend class Database;
  friend class Cells;

  id_type m_id;
  std::string m_name;
  std::string m_variant;
  size_t m_num_items;
  size_t m_num_items_visited;
  References m_references;
  Database *mp_database;

  Cell (const Cell &d);
  Cell &operator= (const Cell &d);

  /**
   *  @brief Add an offset to the number of items visited
   */
  void add_to_num_items_visited (int d)
  {
    m_num_items_visited += d;
  }

  /**
   *  @brief Add an offset to the number of items 
   */
  void add_to_num_items (int d)
  {
    m_num_items += d;
  }

  /**
   *  @brief Reset the number of items
   */
  void reset_num_items ()
  {
    m_num_items = 0;
    m_num_items_visited = 0;
  }
  
  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database)
  {
    mp_database = database;
    m_references.set_database (database);
  }

  std::pair<bool, db::DCplxTrans> path_to (id_type parent_cell_id, const Database *db, std::set <id_type> &visited, const db::DCplxTrans &trans) const;
};

/**
 *  @brief A collection of cells 
 */
class RDB_PUBLIC Cells
{
public:
  typedef tl::shared_collection<Cell>::const_iterator const_iterator;
  typedef tl::shared_collection<Cell>::iterator iterator;

  /**
   *  @brief The default Constructor
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cells ()
    : mp_database (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The default Constructor
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Cells (Database *database)
    : mp_database (database)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Add a cell to this collection
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void add_cell (Cell *cell)
  {
    m_cells.push_back (cell);
    cell->set_database (mp_database.get ());
  }

  /**
   *  @brief Begin iterator (const)
   */
  const_iterator begin () const
  {
    return m_cells.begin ();
  }

  /**
   *  @brief End iterator (const)
   */
  const_iterator end () const
  {
    return m_cells.end ();
  }

  /**
   *  @brief Begin iterator (non-const)
   */
  iterator begin () 
  {
    return m_cells.begin ();
  }

  /**
   *  @brief End iterator (non-const)
   */
  iterator end () 
  {
    return m_cells.end ();
  }

  /**
   *  @brief Clear this collection
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void clear ()
  {
    m_cells.clear ();
  }

  /**
   *  @brief Import a cell
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void import_cell (const Cell &cell);

  /**
   *  @brief Get the database reference
   */
  const Database *database () const
  {
    return mp_database.get ();
  }

  /**
   *  @brief Get the database reference (non-const)
   */
  Database *database () 
  {
    return mp_database.get ();
  }

private:
  friend class Database;

  tl::shared_collection<Cell> m_cells;
  tl::weak_ptr<Database> mp_database;

  /**
   *  @brief Set the database reference
   */
  void set_database (Database *database)
  {
    mp_database = database;
  }
};

/**
 *  @brief Represents a tag
 *
 *  A tag has a name and a description. A tag is identifies with an Id.
 */
class RDB_PUBLIC Tag
{
public:
  /**
   *  @brief Default constructor
   */
  Tag ()
    : m_id (0), m_is_user_tag (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The constructor
   */
  Tag (id_type id, const std::string &name, bool user_tag = false)
    : m_id (id), m_is_user_tag (user_tag), m_name (name)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Set the description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Get the description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Gets a flag indicating whether the tag is a user tag or a system tag
   *  
   *  If this flag is false, the tag is a system tag used for tagging "waived" and
   *  similar conditions. Otherwise it is a user tag which can be used freely to 
   *  tag arbitrary conditions.
   */
  bool is_user_tag () const
  {
    return m_is_user_tag;
  }

  /**
   *  @brief Sets a flag indicating whether the tag is a user tag or a system tag
   *
   *  See \is_user_tag for details.
   */
  void set_user_tag (bool user) 
  {
    m_is_user_tag = user;
  }

  /**
   *  @brief Gets the id
   */
  id_type id () const
  {
    return m_id;
  }

  /**
   *  @brief The Id of the category (setter)
   *
   *  This method must not be used for items in the database to keep the database consistent.
   */
  void set_id (id_type id) 
  {
    m_id = id;
  }

  /**
   *  @brief Gets the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the name
   *
   *  The name of the tag must not be changed when the tag is already part of a Tags collection.
   *  Otherwise, the tag collection becomes inconsistent.
   */
  void set_name (const std::string &name) 
  {
    m_name = name;
  }

private:
  id_type m_id;
  bool m_is_user_tag;
  std::string m_name, m_description;
};

/**
 *  @brief Represents a tag collection
 */
class RDB_PUBLIC Tags
{
public:
  typedef std::vector <rdb::Tag> tag_list_type;
  typedef tag_list_type::const_iterator const_iterator;

  /**
   *  @brief Default constructor for the tags list 
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  Tags ();

  /**
   *  @brief Get the tag for a name (const version)
   */
  const Tag &tag (const std::string &name, bool user_tag = false) const;

  /**
   *  @brief Get the tag for a name (non-const version)
   */
  Tag &tag (const std::string &name, bool user_tag = false);

  /**
   *  @brief Get the tag for an id
   */
  const Tag &tag (id_type id) const;

  /**
   *  @brief Get the tag for an id
   */
  Tag &tag (id_type id);

  /**
   *  @brief Determine if a tag with that name already exists
   */
  bool has_tag (const std::string &name, bool user_tag = false) const;

  /**
   *  @brief Get the tag iterator (begin)
   */
  const_iterator begin_tags () const
  {
    return m_tags.begin ();
  }

  /**
   *  @brief Get the tag iterator (end)
   */
  const_iterator end_tags () const
  {
    return m_tags.end ();
  }

  /**
   *  @brief Import a tag
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  This will assign a new id to the tag and replace any tag with that 
   *  name.
   */
  void import_tag (const Tag &tag);

  /**
   *  @brief Clear the collection of tags
   */
  void clear ();

private:
  friend class Database;

  mutable std::map <std::pair<std::string, bool>, id_type> m_ids_for_names;
  mutable std::vector <Tag> m_tags;
};

/**
 *  @brief The database object
 */
class RDB_PUBLIC Database 
  : public gsi::ObjectBase,
    public tl::Object
{
public:
  typedef Items::const_iterator const_item_iterator;
  typedef Items::iterator item_iterator;
  typedef std::list<ItemRef>::const_iterator const_item_ref_iterator;
  typedef std::list<ItemRef>::iterator item_ref_iterator;
  typedef Cells::const_iterator const_cell_iterator;
  typedef Cells::iterator cell_iterator;

  /**
   *  @brief Default constructor
   */
  Database ();

  /**
   *  @brief Destructor
   */
  ~Database ();

  /**
   *  @brief Get the database description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Set the database description
   */
  void set_description (const std::string &description)
  {
    set_modified ();
    m_description = description;
  }

  /**
   *  @brief Get the database original file
   *
   *  The original file describes what original file the marker database
   *  was derived from.
   */
  const std::string &original_file () const
  {
    return m_original_file;
  }

  /**
   *  @brief Set the database original file
   */
  void set_original_file (const std::string &original_file)
  {
    set_modified ();
    m_original_file = original_file;
  }

  /**
   *  @brief Get the database name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Set the database name
   */
  void set_name (const std::string &name)
  {
    m_name = name;
  }

  /**
   *  @brief Get the file name
   */
  const std::string &filename () const
  {
    return m_filename;
  }

  /**
   *  @brief Set the file name
   */
  void set_filename (const std::string &filename)
  {
    set_modified ();
    m_filename = filename;
  }

  /**
   *  @brief Get the generator name
   */
  const std::string &generator () const
  {
    return m_generator;
  }

  /**
   *  @brief Set the generator name
   */
  void set_generator (const std::string &generator)
  {
    set_modified ();
    m_generator = generator;
  }

  /**
   *  @brief Set the top cell name 
   */
  void set_top_cell_name (const std::string &topcell)
  {
    set_modified ();
    m_topcell = topcell;
  }

  /**
   *  @brief Return the top cell name
   */
  const std::string &top_cell_name () const
  {
    return m_topcell;
  }

  /*
   *  @brief Get the reference to the tags collection (const version)
   */
  const Tags &tags () const
  {
    return m_tags;
  }

  /**
   *  @brief Import tags
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void import_tags (const Tags &tags);

  /**
   *  @brief Get the reference to the categories collection (const version)
   */
  const Categories &categories () const
  {
    return *mp_categories;
  }

  /**
   *  @brief Import categories
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  This will take over the ownership over the Categories object.
   */
  void import_categories (Categories *categories);

  /**
   *  @brief Create a category and register it 
   */
  Category *create_category (const std::string &name);

  /**
   *  @brief Create a category as a subcategory and register it 
   */
  Category *create_category (Category *parent, const std::string &name);

  /**
   *  @brief Create a category as a subcategory in the container and register it 
   *
   *  Hint: this method does not set the parent properly and must not be used
   *  under normal circumstances. It is provided as a internal method and
   *  to be used in the persistency code.
   */
  Category *create_category (Categories *container, const std::string &name);

  /**
   *  @brief Get the category pointer for a category name (const version)
   *
   *  This method returns 0 if the category name is invalid.
   */
  const Category *category_by_name (const std::string &name) const
  {
    return const_cast<Database *> (this)->category_by_name_non_const (name);
  }

  /**
   *  @brief Get the category pointer for a category id (const version)
   *
   *  This method returns 0 if the category is invalid.
   */
  const Category *category_by_id (id_type id) const
  {
    return const_cast<Database *> (this)->category_by_id_non_const (id);
  }

  /**
   *  @brief Access to the cell collection (const)
   */
  const Cells &cells () const
  {
    return m_cells;
  }

  /**
   *  @brief Import cells
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   */
  void import_cells (const Cells &cells);

  /**
   *  @brief Create a cell and register it
   *
   *  If a cell with that name already exists, a variant is created
   */
  Cell *create_cell (const std::string &name)
  {
    return create_cell (name, std::string ());
  }

  /**
   *  @brief Create a cell variant and register it
   *
   *  A cell with name name/variant combination must not exist already.
   *  If the variant string is empty, this method behaves the same as the 
   *  method without variant.
   */
  Cell *create_cell (const std::string &name, const std::string &variant);

  /**
   *  @brief Get all variants registered for a given cell name (not qname!)
   *
   *  @return a vector of id's corresponding to the given variants or an empty vector if the name is not valid or the cell has no variants
   */
  const std::vector <id_type> &variants (const std::string &name);

  /**
   *  @brief Get the cell pointer for a cell name or name:variant combination (const version)
   *
   *  This method returns 0 if the cell name or name:variant combination is invalid.
   */
  const Cell *cell_by_qname (const std::string &name) const
  {
    return const_cast<Database *> (this)->cell_by_qname_non_const (name);
  }

  /**
   *  @brief Get the cell pointer for a cell id (const version)
   *
   *  This method returns 0 if the cell id is invalid.
   */
  const Cell *cell_by_id (id_type id) const
  {
    return const_cast<Database *> (this)->cell_by_id_non_const (id);
  }

  /**
   *  @brief Report the number of items in total
   */
  size_t num_items () const
  {
    return m_num_items;
  }

  /**
   *  @brief Report the number of items visited
   */
  size_t num_items_visited () const
  {
    return m_num_items_visited;
  }

  /**
   *  @brief Report the number of items for a given cell and category id
   */
  size_t num_items (id_type cell_id, id_type category_id) const;

  /**
   *  @brief Report the number of items visited
   */
  size_t num_items_visited (id_type cell_id, id_type category_id) const;

  /**
   *  @brief Create a new item for the given cell and category (both given by id)
   */
  Item *create_item (id_type cell_id, id_type category_id);

  /**
   *  @brief Set a tag's description
   */
  void set_tag_description (id_type tag_id, const std::string &description);

  /**
   *  @brief Set an item to visited state or reset the state
   */
  void set_item_visited (const Item *item, bool visited);

  /**
   *  @brief Add a tag to the given item
   */
  void add_item_tag (const Item *item, id_type tag);

  /**
   *  @brief Remove a tag from the given item
   */
  void remove_item_tag (const Item *item, id_type tag);

#if defined(HAVE_QT)
  /**
   *  @brief Set the image of an item
   */
  void set_item_image (const Item *item, const QImage &image);
#endif

  /**
   *  @brief Set the image string of an item
   */
  void set_item_image_str (const Item *item, const std::string &image_str);

  /**
   *  @brief Set the multiplicity of an item
   */
  void set_item_multiplicity (const Item *item, size_t n);

  /**
   *  @brief Get the items collection (const version)
   */
  const Items &items () const
  {
    return *mp_items;
  }

  /**
   *  @brief Set the items collection
   *
   *  This method is provided for persistency application only. It should not be used otherwise.
   *  This will take ownership over the items collection.
   */
  void set_items (Items *items);

  /**
   *  @brief Get an iterator pair that delivers the const items (ItemRef) for a given cell
   */
  std::pair<const_item_ref_iterator, const_item_ref_iterator> items_by_cell (id_type cell_id) const; 

  /**
   *  @brief Get an iterator that delivers the const items (ItemRef) for a given category
   */
  std::pair<const_item_ref_iterator, const_item_ref_iterator> items_by_category (id_type category_id) const; 

  /**
   *  @brief Get an iterator that delivers the const items (ItemRef) for a given cell and category
   */
  std::pair<const_item_ref_iterator, const_item_ref_iterator> items_by_cell_and_category (id_type cell_id, id_type category_id) const; 

  /**
   *  @brief Returns true, if the database was modified
   */
  bool is_modified () const
  {
    return m_modified;
  }

  /**
   *  @brief Reset the modified file
   */
  void reset_modified () 
  {
    m_modified = false;
  }

  /**
   *  @brief Save the database to a file
   */
  void save (const std::string &filename);

  /**
   *  @brief Load the database from a file
   *
   *  @brief This clears the existing database.
   */
  void load (const std::string &filename);

private:
  std::string m_generator;
  std::string m_filename;
  std::string m_description;
  std::string m_original_file;
  std::string m_name;
  std::string m_topcell;
  id_type m_next_id;
  Categories *mp_categories;
  Tags m_tags;
  std::map <std::string, Cell *> m_cells_by_qname;
  std::map <std::string, std::vector <id_type> > m_cell_variants;
  std::map <id_type, Cell *> m_cells_by_id;
  std::map <id_type, Category *> m_categories_by_id;
  std::map <std::pair <id_type, id_type>, std::list<ItemRef> > m_items_by_cell_and_category_id;
  std::map <std::pair <id_type, id_type>, size_t> m_num_items_by_cell_and_category;
  std::map <std::pair <id_type, id_type>, size_t> m_num_items_visited_by_cell_and_category;
  std::map <id_type, std::list<ItemRef> > m_items_by_cell_id;
  std::map <id_type, std::list<ItemRef> > m_items_by_category_id;
  Items *mp_items;
  Cells m_cells;
  size_t m_num_items;
  size_t m_num_items_visited;
  bool m_modified;

  void clear ();

  void set_modified () 
  {
    m_modified = true;
  }

  /**
   *  @brief Get the items collection (non-const version)
   */
  Items &items_non_const () 
  {
    return *mp_items;
  }

  /**
   *  @brief Get the reference to the tags collection (non-const version)
   */
  Tags &tags_non_const ()
  {
    return m_tags;
  }

  /**
   *  @brief Get the reference to the categories collection (non-const version)
   */
  Categories &categories_non_const ()
  {
    return *mp_categories;
  }

  /**
   *  @brief Get the category pointer for a category name
   *
   *  This method returns 0 if the category name is invalid.
   */
  Category *category_by_name_non_const (const std::string &name);

  /**
   *  @brief Get the category pointer for a category id
   *
   *  This method returns 0 if the category is invalid.
   */
  Category *category_by_id_non_const (id_type id);

  /**
   *  @brief Access to the cell collection
   */
  Cells &cells_non_const ()
  {
    return m_cells;
  }

  /**
   *  @brief Get the cell pointer for a cell name or name:variant combination (non-const version)
   *
   *  This method returns 0 if the cell name or name:variant combination is invalid.
   */
  Cell *cell_by_qname_non_const (const std::string &qname);

  /**
   *  @brief Get the cell pointer for a cell id (non-const version)
   *
   *  This method returns 0 if the cell id is invalid.
   */
  Cell *cell_by_id_non_const (id_type id);
};

}

#endif
