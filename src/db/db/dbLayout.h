
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


#ifndef HDR_dbLayout
#define HDR_dbLayout

#include "dbCommon.h"

#include "dbArray.h"
#include "dbPropertiesRepository.h"
#include "dbObject.h"
#include "dbText.h"
#include "dbCell.h"
#include "dbLayoutStateModel.h"
#include "dbLayoutLayers.h"
#include "dbLayerProperties.h"
#include "dbMetaInfo.h"
#include "dbCellInst.h"
#include "tlException.h"
#include "tlVector.h"
#include "tlString.h"
#include "tlThreads.h"
#include "tlObject.h"
#include "tlUniqueId.h"
#include "gsi.h"

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <list>
#include <vector>


namespace db
{

class Manager;
class MemStatistics;
class PCellVariant;
class PCellDeclaration;
class PCellHeader;
class Library;
class LibraryProxy;
class CellMapping;
class LayerMapping;
class Region;
class Edges;
class EdgePairs;
class Texts;
class Technology;
class CellMapping;
class LayerMapping;
class VariantsCollectorBase;

template <class Coord> class generic_repository;
typedef generic_repository<db::Coord> GenericRepository;

/**
 *  @brief The cell iterator
 *
 *  Since we are using a custom cell list, we have to provide our own iterator
 */
template <class C>
class DB_PUBLIC_TEMPLATE cell_list_iterator
{
public:
  typedef C cell_type;
  typedef cell_type value_type;
  typedef cell_type *pointer; 
  typedef cell_type &reference; 
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default constructor
   */
  cell_list_iterator ()
    : mp_cell (0)
  { }

  /**
   *  @brief Constructor pointing to a certain element
   */
  cell_list_iterator (cell_type *cell)
    : mp_cell (cell)
  { }

  /**
   *  @brief Equality
   */
  bool operator== (const cell_list_iterator &d) const
  {
    return mp_cell == d.mp_cell;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const cell_list_iterator &d) const
  {
    return mp_cell != d.mp_cell;
  }

  /**
   *  @brief Increment
   */
  cell_list_iterator &operator++ () 
  {
    mp_cell = mp_cell->mp_next;
    return *this;
  }

  /**
   *  @brief Decrement
   */
  cell_list_iterator &operator-- () 
  {
    mp_cell = mp_cell->mp_last;
    return *this;
  }

  /**
   *  @brief Access
   */
  cell_type &operator* () const
  {
    return *mp_cell;
  }

  /**
   *  @brief Access
   */
  cell_type *operator-> () const
  {
    return mp_cell;
  }

private:
  cell_type *mp_cell;
};

/**
 *  @brief The cell iterator
 *
 *  Since we are using a custom cell list, we have to provide our own iterator
 */
template <class C>
class cell_list_const_iterator
{
public:
  typedef C cell_type;
  typedef const cell_type value_type;
  typedef const cell_type *pointer; 
  typedef const cell_type &reference; 
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default constructor
   */
  cell_list_const_iterator ()
    : mp_cell (0)
  { }

  /**
   *  @brief Constructor pointing to a certain element
   */
  cell_list_const_iterator (const cell_type *cell)
    : mp_cell (cell)
  { }

  /**
   *  @brief Default constructor
   */
  cell_list_const_iterator (cell_list_iterator<cell_type> iter)
    : mp_cell (iter.operator-> ())
  { }

  /**
   *  @brief Equality
   */
  bool operator== (const cell_list_const_iterator &d) const
  {
    return mp_cell == d.mp_cell;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const cell_list_const_iterator &d) const
  {
    return mp_cell != d.mp_cell;
  }

  /**
   *  @brief Increment
   */
  cell_list_const_iterator &operator++ () 
  {
    mp_cell = mp_cell->mp_next;
    return *this;
  }

  /**
   *  @brief Decrement
   */
  cell_list_const_iterator &operator-- () 
  {
    mp_cell = mp_cell->mp_last;
    return *this;
  }

  /**
   *  @brief Access
   */
  const cell_type &operator* () const
  {
    return *mp_cell;
  }

  /**
   *  @brief Access
   */
  const cell_type *operator-> () const
  {
    return mp_cell;
  }

private:
  const cell_type *mp_cell;
};

/**
 *  @brief The cell list 
 */
template <class C>
class cell_list
{
public:
  typedef C cell_type;
  typedef cell_list_iterator<cell_type> iterator;
  typedef cell_list_const_iterator<cell_type> const_iterator;

  /**
   *  @brief The default constructor
   */
  cell_list ()
    : mp_first (0), mp_last (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~cell_list ()
  {
    clear ();
  }

  /**
   *  @brief Clear the cell list
   */
  void clear ()
  {
    while (! empty ()) {
      erase (begin ());
    }
  }

  /**
   *  @brief Push back a new pointer
   *
   *  The ownership over that pointer is transferred to the CellList.
   */
  void push_back_ptr (cell_type *new_cell)
  {
    new_cell->mp_last = mp_last;
    new_cell->mp_next = 0;
    if (mp_last) {
      mp_last->mp_next = new_cell;
    } else {
      mp_first = new_cell;
    }
    mp_last = new_cell;
  }
  
  /**
   *  @brief empty predicate
   *
   *  @return true, if the list is empty
   */
  bool empty () const
  {
    return mp_last == 0 && mp_first == 0;
  }
  
  /**
   *  @brief begin iterator
   */
  iterator begin () 
  {
    return iterator (mp_first);
  }
  
  /**
   *  @brief end iterator
   */
  iterator end () 
  {
    return iterator (0);
  }
  
  /**
   *  @brief begin iterator (const)
   */
  const_iterator begin () const
  {
    return const_iterator (mp_first);
  }
  
  /**
   *  @brief end iterator (const)
   */
  const_iterator end () const
  {
    return const_iterator (0);
  }

  /**
   *  @brief Take out an element from the list
   *
   *  This removes the element from the list but does not
   *  destroy it. The ownership is given back to the 
   *  caller.
   */
  cell_type *take (iterator iter) 
  {
    cell_type *cell = &(*iter);
    
    if (cell->mp_last) {
      cell->mp_last->mp_next = cell->mp_next;
    } else {
      mp_first = cell->mp_next;
    }

    if (cell->mp_next) {
      cell->mp_next->mp_last = cell->mp_last;
    } else {
      mp_last = cell->mp_last;
    }

    cell->mp_last = 0;
    cell->mp_next = 0;

    return cell;
  }

  /**
   *  @brief Erase an element from the list
   *
   *  This will destroy the underlying cell object.
   */
  void erase (iterator iter)
  {
    delete take (iter);
  }

private:
  //  No copy, no assignment (because the is no good cell copy constructor)
  cell_list &operator= (const cell_list &d);
  cell_list (const cell_list &d);

  cell_type *mp_first, *mp_last;
};

/**
 *  @brief An interface that is used to map layer between libraries and PCells and the layout
 */
class ImportLayerMapping
{
public:
  /**
   *  @brief Destructor
   */
  virtual ~ImportLayerMapping () { }

  /**
   *  @brief Perform the mapping, i.e. deliver a layer index for a given LayerProperties information
   *
   *  This method can return false in the first member of the returned pair to indicate that no mapping shall
   *  be performed. Otherwise it must return the layer index in the second member.
   */
  virtual std::pair <bool, unsigned int> map_layer (const LayerProperties &lprops) = 0;
};

/**
 *  @brief A binary object representing context information for regenerating library proxies and PCells
 */
struct DB_PUBLIC LayoutOrCellContextInfo
{
  std::string lib_name;
  std::string cell_name;
  std::string pcell_name;
  std::map<std::string, tl::Variant> pcell_parameters;
  std::map<std::string, std::pair<tl::Variant, std::string> > meta_info;

  static LayoutOrCellContextInfo deserialize (std::vector<std::string>::const_iterator from, std::vector<std::string>::const_iterator to);
  void serialize (std::vector<std::string> &strings);

  bool has_proxy_info () const;
  bool has_meta_info () const;
};

/**
 *  @brief The layout object
 *
 *  The layout object basically wraps the cell graphs and
 *  adds functionality for managing cell names and layer names.
 */

class DB_PUBLIC Layout 
  : public db::Object,
    public db::LayoutStateModel,
    public gsi::ObjectBase,
    public tl::Object,
    public tl::UniqueId
{
public:
  typedef db::Box box_type;
  typedef db::CellInst cell_inst_type;
  typedef db::Cell cell_type;
  typedef db::cell_list<cell_type> cell_list;
  typedef db::PCellVariant pcell_variant_type;
  typedef db::PCellHeader pcell_header_type;
  typedef db::PCellDeclaration pcell_declaration_type;
  typedef db::LibraryProxy lib_proxy_type;
  typedef cell_list::iterator iterator;
  typedef cell_list::const_iterator const_iterator;
  typedef tl::vector<db::cell_index_type> cell_index_vector;
  typedef cell_index_vector::reverse_iterator bottom_up_iterator;
  typedef cell_index_vector::const_reverse_iterator bottom_up_const_iterator;
  typedef cell_index_vector::iterator top_down_iterator;
  typedef cell_index_vector::const_iterator top_down_const_iterator;
  typedef tl::vector<cell_type *> cell_ptr_vector;
  typedef db::properties_id_type properties_id_type;
  typedef db::pcell_id_type pcell_id_type;
  typedef std::map<std::string, pcell_id_type> pcell_name_map;
  typedef pcell_name_map::const_iterator pcell_iterator;
  typedef std::map<std::pair<lib_id_type, cell_index_type>, cell_index_type> lib_proxy_map;
  typedef LayerIterator layer_iterator;
  typedef size_t meta_info_name_id_type;
  typedef std::map<meta_info_name_id_type, MetaInfo> meta_info_map;
  typedef meta_info_map::const_iterator meta_info_iterator;

  /**
   *  @brief A helper functor to compare "const char *" by the content
   */
  struct name_cmp_f 
  {
    bool operator() (const char *a, const char *b) const {
      return strcmp (a, b) < 0;
    }
  };

  typedef std::map<const char *, cell_index_type, name_cmp_f> cell_map_type;

  /**
   *  @brief Standard constructor
   *
   *  The editable mode will be taken from db::default_editable_mode.
   */
  explicit Layout (db::Manager *manager = 0);

  /**
   *  @brief Standard constructor which allows one to specify editable mode
   */
  explicit Layout (bool editable, db::Manager *manager = 0);

  /**
   *  @brief The copy ctor
   *
   *  This copy constructor inherits the attachment to a manager.
   *  For copying without attachment, create a layout without a manager attached
   *  and use the assignment operator.
   */
  Layout (const Layout &d);

  /**
   *  @brief Destructor
   */
  ~Layout ();

  /**
   *  @brief Assignment operator
   */
  Layout &operator= (const Layout &d);

  /**
   *  @brief Specifies, if the layout participates in cleanup
   *
   *  "cleanup" will be called to get rid of top level proxies.
   *  This flag controls whether cleanup happens or not. Library
   *  layouts for example must not loose proxies as they might
   *  themselves be referenced.
   *
   *  The default is OFF.
   */
  void do_cleanup (bool f)
  {
    m_do_cleanup = f;
  }

  /**
   *  @brief Clears the layout
   */
  void clear ();

  /**
   *  @brief Gets the technology name the layout is associated with
   */
  const std::string &technology_name () const
  {
    return m_tech_name;
  }

  /**
   *  @brief Gets the library the layout lives in or NULL if the layout is not part of a library
   */
  Library *library () const
  {
    return mp_library;
  }

  /**
   *  @brief Sets the library pointer
   */
  void set_library (db::Library *library)
  {
    mp_library = library;
  }

  /**
   *  @brief Gets the technology object the layout is associated with or null if no valid technology is associated
   */
  const db::Technology *technology () const;

  /**
   *  @brief Changes the technology, the layout is associated with
   *  Changing the layout may re-assess all the library references as libraries can be technology specific
   */
  void set_technology_name (const std::string &tech);

  /**
   *  @brief Changes the technology name
   *  This method will only change the technology name, but does not re-assess the library links.
   *  It's provided mainly to support undo/redo and testing.
   */
  void set_technology_name_without_update (const std::string &tech);

  /**
   *  @brief Accessor to the array repository
   */
  ArrayRepository &array_repository ()
  {
    return m_array_repository;
  }

  /**
   *  @brief Accessor to the string repository
   */
  StringRepository &string_repository ()
  {
    return m_string_repository;
  }

  /**
   *  @brief Accessor to the string repository (const version)
   */
  const StringRepository &string_repository () const
  {
    return m_string_repository;
  }

  /**
   *  @brief Accessor to the shape repository
   */
  GenericRepository &shape_repository ()
  {
    return m_shape_repository;
  }

  /**
   *  @brief Accessor to the shape repository (const version)
   */
  const GenericRepository &shape_repository () const
  {
    return m_shape_repository;
  }

  /**
   *  @brief Accessor to the properties repository
   */
  PropertiesRepository &properties_repository ()
  {
    return m_properties_repository;
  }

  /**
   *  @brief Accessor to the properties repository
   */
  const PropertiesRepository &properties_repository () const
  {
    return m_properties_repository;
  }

  /**
   *  @brief Gets the lock for the layout object
   *  This is a generic lock that can be used to lock modifications against multiple threads.
   */
  tl::Mutex &lock ()
  {
    return m_lock;
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

  /**
   *  @brief Sets the properties ID
   */
  void prop_id (db::properties_id_type id);

  /**
   *  @brief Gets the properties ID
   */
  db::properties_id_type prop_id () const
  {
    return m_prop_id;
  }

  /**
   *  @brief Query, if a cell with a given name is present
   *
   *  Since internally just a "const char *" is required, the interface
   *  does not use std::string for the name to avoid unnecessary string
   *  allocations.
   *
   *  @param s The name of the cell
   *  @return true, if the cell is present
   */
  bool has_cell (const char *name);

  /**
   *  @brief Obtain the index of a cell with the given name
   *
   *  Since internally just a "const char *" is required, the interface
   *  does not use std::string for the name to avoid unnecessary string
   *  allocations.
   *
   *  @param name The name of the cell
   *  @return A pair, telling if the cell was present (first) and
   *          it's index if it is present (second)
   */ 
  std::pair<bool, cell_index_type> cell_by_name (const char *name) const;

  /** 
   *  @brief Tell the name of a cell with the given index
   *  
   *  @param index The index of the cell
   *  @return The name of the cell
   */
  const char *cell_name (cell_index_type index) const;

  /**
   *  @brief Return the display name for the given cell
   *
   *  This method is forwarded to the respective method of the cell.
   *  The display name is a nicely formatted name reflecting the
   *  library source and PCell parameters (if supported by the 
   *  PCell implementation).
   */
  std::string display_name (cell_index_type cell_index) const;

  /**
   *  @brief Return the basic name for the given cell 
   *
   *  This method is forwarded to the respective method of the cell.
   *  The basic name is the "original"  cell name within the library or
   *  of the PCell. The actual cell name may differ from that by
   *  a unique extension.
   */
  std::string basic_name (cell_index_type cell_index) const;

  /** 
   *  @brief Add a cell object with the given ID and name
   *
   *  This method is basically supposed to be used for "undo" and "redo". 
   *  It is not intended for general use. It requires that the cell was already 
   *  created before and has gone "invalid" in between. The cell object's ownership is transferred to 
   *  the layout. 
   *
   *  @param id The name of the new cell
   *  @param name The name of the new cell
   *  @param cell The pointer of the cell object to transfer
   */
  void insert_cell (cell_index_type ci, const std::string &name, db::Cell *cell);

  /** 
   *  @brief Take a cell object with the given ID and name
   *
   *  This method is basically supposed to be used for "undo" and "redo". 
   *  It is not intended for general use. It takes out the cell object and released ownership
   *  within the layout object.
   *
   *  @param id The name of the new cell
   *  @return The pointer to the "free" cell object
   */
  db::Cell *take_cell (cell_index_type ci);

  /**
   *  @brief Uniquify the given name by appending a suitable suffix
   *
   *  @param name The input name 
   *  @return A similar name that is unique
   */
  std::string uniquify_cell_name (const char *name) const;

  /** 
   *  @brief Add a cell with a given name
   *
   *  Since internally just a "const char *" is required, the interface
   *  does not use std::string for the name to avoid unnecessary string
   *  allocations.
   *  If the name pointer is 0, the cell is given a unique name.
   *
   *  @param s The name of the new cell
   *  @return The index of the new cell
   */
  cell_index_type add_cell (const char *name = 0);

  /**
   *  @brief Adds a cell using another cell as a template
   *
   *  This method will use the name of the other cell and initialize the
   *  new cell with the meta info from the other cell.
   */
  cell_index_type add_cell (const db::Layout &other, db::cell_index_type ci);

  /**
   *  @brief Add a cell without a name
   *
   *  The cell is created, but cannot be found by name. The name returned is an empty string.
   *  The cell is created with the purpose of being renamed later.
   *
   *  @return The index of the new cell
   */
  cell_index_type add_anonymous_cell ();

  /**
   *  @brief Rename a cell
   *
   *  Rename the cell with the given id.
   *
   *  @param id The index of the cell to rename
   *  @param name The new name of the cell
   */
  void rename_cell (cell_index_type id, const char *name);

  /**
   *  @brief Delete a cell 
   *
   *  This deletes a cell but not the sub cells of the cell.
   *  These subcells will likely become new top cells unless they are used
   *  otherwise.
   *  All instances of this cell are deleted as well.
   *  Hint: to delete multiple cells, use "delete_cells" which is 
   *  far more efficient in this case.
   *
   *  @param id The index of the cell to delete
   */
  void delete_cell (cell_index_type id);

  /**
   *  @brief Delete multiple cells
   *
   *  This deletes the cells but not the sub cells of these cells.
   *  These subcells will likely become new top cells unless they are used
   *  otherwise.
   *  All instances of these cells are deleted as well.
   *
   *  @param from A begin type iterator that delivers the ids of the cells to delete
   *  @param to A end type iterator that delivers the ids of the cells to delete
   */
  template <class Iter>
  void delete_cells (Iter from, Iter to)
  {
    std::set<cell_index_type> cells_to_delete;
    cells_to_delete.insert (from, to);
    delete_cells (cells_to_delete);
  }

  /**
   *  @brief Delete multiple cells
   *
   *  This deletes the cells but not the sub cells of these cells.
   *  These subcells will likely become new top cells unless they are used
   *  otherwise.
   *  All instances of these cells are deleted as well.
   *
   *  @param cells_to_delete A set containing the ids of the cells to delete
   */
  void delete_cells (const std::set<cell_index_type> &cells_to_delete);

  /** 
   *  @brief Convert a PCell variant to a static cell
   *
   *  @param cell_index The index of the PCell
   *  @return The index of the new cell or cell_index if the cell is not a PCell or library proxy
   */
  db::cell_index_type convert_cell_to_static (db::cell_index_type cell_index);

  /** 
   *  @brief Get a PCell variant
   *
   *  @param pcell_id The Id of the PCell declaration
   *  @param parameters The PCell parameters
   *  @return The index of the new cell
   */
  cell_index_type get_pcell_variant (pcell_id_type pcell_id, const std::vector<tl::Variant> &parameters);

  /**
   *  @brief Gets a PCell variant 
   *
   *  Unlike the first version, this one allows specification of the parameters through a 
   *  key/value dictionary. Parameters not listed there are replaced by their defaults.
   *
   *  @param pcell_id The Id of the PCell declaration
   *  @param parameters A dictionary of key/value value pairs for the parameters
   *  @return The index of the new cell
   */
  cell_index_type get_pcell_variant_dict (pcell_id_type pcell_id, const std::map<std::string, tl::Variant> &p);

  /** 
   *  @brief Get a PCell variant and replace the given cell
   *
   *  @param pcell_id The Id of the PCell declaration
   *  @param parameters The PCell parameters
   *  @param cell_index The cell index which is to be replaced by the PCell variant proxy
   *  @param layer_mapping The optional layer mapping object that maps the PCell layers to the layout's layers
   *  @param retain_layout Set to true for not using update() on the PCell but to retain existing layout (conservative approach)
   */
  void get_pcell_variant_as (pcell_id_type pcell_id, const std::vector<tl::Variant> &parameters, cell_index_type cell_index, ImportLayerMapping *layer_mapping = 0, bool retain_layout = false);

  /** 
   *  @brief Get the PCell variant cell of a existing cell with new parameters
   *
   *  This method is intended for internal use.
   *
   *  @param cell_index The current PCell variant proxy cell (must be a PCellVariant)
   *  @param new_parameters The new parameters
   *  @return The cell index of the new PCell variant proxy cell.
   */
  cell_index_type get_pcell_variant_cell (cell_index_type cell_index, const std::vector<tl::Variant> &new_parameters);

  /**
   *  @brief Get the PCell header of the given PCell Id
   *
   *  This method returns 0, if the id is not a valid one.
   *
   *  @param pcell_id The Id of the PCell.
   */
  const pcell_header_type *pcell_header (pcell_id_type pcell_id) const;

  /**
   *  @brief Get the PCell header of the given PCell Id (non-const version)
   *
   *  This method returns 0, if the id is not a valid one.
   *
   *  @param pcell_id The Id of the PCell.
   */
  pcell_header_type *pcell_header (pcell_id_type pcell_id);

  /**
   *  @brief Get the PCell declaration of the given PCell Id
   *
   *  This method returns 0, if the id is not a valid one.
   *
   *  @param pcell_id The Id of the PCell.
   */
  const pcell_declaration_type *pcell_declaration (pcell_id_type pcell_id) const;

  /**
   *  @brief Get the PCell Id for a given PCell name
   *
   *  If a PCell with that name does not exist, a PCell will be created.
   *
   *  @param name The name of the PCell in the scope of this layout.
   *  @return A pair of an id and a boolean which is true, if the PCell with that name exists.
   */
  std::pair<bool, pcell_id_type> pcell_by_name (const char *name) const;

  /**
   *  @brief Register a pcell declaration
   *
   *  If a PCell with that name is already registered, the definition will be overwritten, but 
   *  no update of any cell content is performed.
   *  The layout object becomes owner of the declaration object and will delete it when it is destroyed.
   */
  pcell_id_type register_pcell (const std::string &name, pcell_declaration_type *declaration);

  /** 
   *  @brief The PCell iterator (begin)
   *
   *  This iterator delivers the PCell names and ID's. iterator->first is the PCell name and iterator->second
   *  the PCell ID.
   */
  pcell_iterator begin_pcells () const
  {
    return m_pcell_ids.begin ();
  }

  /**
   *  @brief The PCell iterator (end)
   */
  pcell_iterator end_pcells () const
  {
    return m_pcell_ids.end ();
  }

  /**
   *  @brief Test, if the given cell is a PCell variant
   *
   *  The return value is a pair of a boolean which is true when the instance is a PCell
   *  instance and the PCell id of the instance if this is the case.
   *
   *  @param ref The cell to test
   *  @return See above
   */
  std::pair<bool, db::pcell_id_type> is_pcell_instance (cell_index_type cell_index) const;

  /**
   *  @brief Returns the library where the cell is finally defined 
   *
   *  In the first part of the returned pair, this method returns the pointer to the library 
   *  where the cell is defined. If the cell is not a library proxy, the return value is 0.
   *  If the cell is defined in a library, the second part of the returned pair will contain
   *  the cell index inside this library. 
   */
  std::pair<db::Library *, db::cell_index_type> defining_library (cell_index_type cell_index) const;

  /**
   *  @brief Gets the PCell declaration object for a PCell instance
   *
   *  This method determines the PCell declaration object for a given PCell variant cell.
   *  Note, that the declaration may originate from a different layout than this, if the PCell
   *  is imported from a library.
   *
   *  The cell given cell is not a PCell variant cell, 0 is returned.
   */
  const Layout::pcell_declaration_type *pcell_declaration_for_pcell_variant (cell_index_type cell_index) const;

  /**
   *  @brief Get the PCell parameters of a PCell instance
   *
   *  For the order of the parameters, ask the PCell declaration (available trough Layout::pcell_declaration
   *  from the PCell id or from Layout::pcell_declaration_for_pcell_variant from the cell_index).
   *
   *  @return A list of parameters in the order they are declared.
   */
  const std::vector<tl::Variant> &get_pcell_parameters (cell_index_type cell_index) const;

  /**
   *  @brief Gets a PCell parameter of a PCell instance
   *
   *  @return The value of the PCell parameter with the given name or a nil variant if there is no such parameter
   */
  tl::Variant get_pcell_parameter (cell_index_type cell_index, const std::string &name) const;

  /**
   *  @brief Get the PCell parameters of a PCell instance as a name to value map
   *
   *  @return A map with the parameter names and their values
   */
  std::map<std::string, tl::Variant> get_named_pcell_parameters (cell_index_type cell_index) const;

  /**
   *  @brief Get the proxy cell (index) for a given library an cell index (inside that library)
   *
   *  This method caches library proxies and delivers an existing one, if a
   *  proxy for that library/cell configuration is registered.
   *  If that is not the case, a new proxy will be created.
   */
  cell_index_type get_lib_proxy (Library *lib, cell_index_type cell_index);

  /**
   *  @brief Get the proxy cell (index) for a given library an cell index (inside that library)
   *
   *  @param retain_layout Set to true for not using update() on the PCell but to retain existing layout (conservative approach)
   *
   *  This method replaces the cell with the given target cell index by a library.
   */
  void get_lib_proxy_as (Library *lib, cell_index_type cell_index, cell_index_type target_cell_index, ImportLayerMapping *layer_mapping = 0, bool retain_layout = false);

  /**
   *  @brief Creates a cold proxy representing the given context information
   */
  cell_index_type create_cold_proxy (const db::LayoutOrCellContextInfo &info);

  /**
   *  @brief Subsitutes the given cell by a cold proxy representing the given context information
   */
  void create_cold_proxy_as (const db::LayoutOrCellContextInfo &info, cell_index_type cell_index);

  /**
   *  @brief Gets a value indicating whether layout context info is provided / needed
   */
  bool has_context_info() const;

  /**
   *  @brief Gets a value indicating whether layout context info is provided / needed
   */
  bool has_context_info(cell_index_type cell_index) const;

  /**
   *  @brief Get the context information for the layout (for writing into a file)
   *
   *  The context information is a sequence of strings which is pushed onto the given
   *  vector. It can be used to fill meta information with fill_meta_info_from_context.
   */
  bool get_context_info (std::vector <std::string> &strings) const;

  /**
   *  @brief Gets the context information as a binary object
   */
  bool get_context_info (LayoutOrCellContextInfo &context_info) const;

  /**
   *  @brief Fills the layout's meta information from the context
   */
  void fill_meta_info_from_context (std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to);

  /**
   *  @brief Fills the layout's meta information from the binary context
   */
  void fill_meta_info_from_context (const LayoutOrCellContextInfo &context_info);

  /**
   *  @brief Get the context information for a given cell (for writing into a file)
   *
   *  The context information is a sequence of strings which is pushed onto the given
   *  vector. It can be used to recover a respective proxy cell with the recover_proxy method
   *  or to fill meta information using fill_meta_info_from_context.
   */
  bool get_context_info (cell_index_type cell_index, std::vector <std::string> &context_info) const;

  /**
   *  @brief Gets the context information as a binary object
   */
  bool get_context_info (cell_index_type cell_index, LayoutOrCellContextInfo &context_info) const;

  /**
   *  @brief Fills the layout's meta information from the context
   */
  void fill_meta_info_from_context (cell_index_type cell_index, std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to);

  /**
   *  @brief Fills the layout's meta information from the binary context
   */
  void fill_meta_info_from_context (cell_index_type cell_index, const LayoutOrCellContextInfo &context_info);

  /**
   *  @brief Recover a proxy cell from the given context info.
   *
   *  Creates a proxy cell from the context information given by two iterators into a string list.
   *  If no cell can be created from the context information, null is returned. 
   *
   *  @param from The begin iterator for the strings from which to recover the cell
   *  @param to The end iterator for the strings from which to recover the cell
   */
  db::Cell *recover_proxy (std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to);

  /**
   *  @brief Recover a proxy cell from the given binary context info object.
   */
  db::Cell *recover_proxy (const LayoutOrCellContextInfo &context_info);

  /**
   *  @brief Recover a proxy cell from the given context info.
   *
   *  Creates a proxy cell from the context information given by two iterators into a string list.
   *  If no cell can be created from the context information, null is returned. 
   *  This creates the proxy as a cell with the given cell index.
   *
   *  @param cell_index The cell which to prepare
   *  @param from The begin iterator for the strings from which to recover the cell
   *  @param to The end iterator for the strings from which to recover the cell
   *  @param layer_mapping The optional layer mapping object that maps the PCell layers to the layout's layers
   *  @return true, if the proxy cell could be created
   */
  bool recover_proxy_as (cell_index_type cell_index, std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to, ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Recover a proxy cell from the given binary context info object
   *
   *  See the string-based version of "recover_proxy_as" for details.
   */
  bool recover_proxy_as (cell_index_type cell_index, const LayoutOrCellContextInfo &context_info, ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Restores proxies as far as possible
   *
   *  This feature can be used after a library update to make sure that proxies are updated.
   *  Library updates may enabled lost connections which are help in cold proxies. This method will recover
   *  these connections.
   */
  void restore_proxies(ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Replaces the given cell index with the new cell
   */
  void replace_cell (cell_index_type target_cell_index, db::Cell *new_cell, bool retain_layout);

  /**
   *  @brief Replaces all instances of src_cell_index by target_cell_index
   */
  void replace_instances_of (cell_index_type src_cell_index, cell_index_type target_cell_index);

  /**
   *  @brief Delete a cell plus the subcells not used otherwise
   *
   *  All subcells referenced directly or indirectly but not used otherwise
   *  are deleted as well. This basically prunes the cell tree by this cell.
   *  All instances of this cell are deleted as well.
   *
   *  @param id The index of the cell to delete
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  void prune_cell (cell_index_type id, int levels = -1);

  /**
   *  @brief Delete cells plus their subcells not used otherwise
   *
   *  All subcells referenced directly or indirectly but not used otherwise
   *  are deleted as well. This basically prunes the cell tree by this cell.
   *  All instances of this cell are deleted as well.
   *  This method is more efficient than calling prune_cell multiple times.
   *
   *  @param from A begin iterator delivering the cell id's to delete
   *  @param to An end iterator delivering the cell id's to delete
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  template <class Iter>
  void prune_cells (Iter from, Iter to, int levels = -1)
  {
    std::set<cell_index_type> cells_to_delete;
    cells_to_delete.insert (from, to);
    prune_cells (cells_to_delete, levels);
  }

  /**
   *  @brief Delete cells plus their subcells not used otherwise
   *
   *  All subcells referenced directly or indirectly but not used otherwise
   *  are deleted as well. This basically prunes the cell tree by this cell.
   *  All instances of this cell are deleted as well.
   *  This method is more efficient than calling prune_cell multiple times.
   *
   *  @param cells A set of cell id's to prune
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  void prune_cells (const std::set<cell_index_type> &cells, int levels = -1);

  /**
   *  @brief Delete the subcells of the given cell which are not used otherwise
   *
   *  All subcells of the given cell which are referenced directly or indirectly but not used otherwise
   *  are deleted. 
   *
   *  @param id The index whose subcells to delete
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  void prune_subcells (cell_index_type id, int levels = -1);

  /**
   *  @brief Delete the subcells of the given cells which are not used otherwise
   *
   *  All subcells referenced directly or indirectly but not used otherwise
   *  are deleted as well.
   *  This method is more efficient than calling prune_subcells for single cells multiple times.
   *
   *  @param from A begin iterator delivering the cell id's to delete
   *  @param to An end iterator delivering the cell id's to delete
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  template <class Iter>
  void prune_subcells (Iter from, Iter to, int levels = -1)
  {
    std::set<cell_index_type> cells_to_delete;
    cells_to_delete.insert (from, to);
    prune_subcells (cells_to_delete, levels);
  }

  /**
   *  @brief Delete the subcells of the given cells which are not used otherwise
   *
   *  All subcells referenced directly or indirectly but not used otherwise
   *  are deleted as well.
   *  This method is more efficient than calling prune_subcells for single cells multiple times.
   *
   *  @param cells A set of cell id's to prune
   *  @param levels The number of hierarchy levels to look for (-1: all, 0: none, 1: one level etc.)
   */
  void prune_subcells (const std::set<cell_index_type> &cells, int levels = -1);

  /**
   *  @brief Flatten a cell into another cell
   *
   *  @param source_cell Where to take the content from
   *  @param target_cell Where to put the content to
   *  @param t The transformation to apply (can be unity for first level)
   *  @param levels The number of levels to flatten or -1 for all levels (0: just copy and transform if source cell != target cell, 1: flatten on level of instances etc.)
   */
  void flatten (const db::Cell &source_cell, db::Cell &target_cell, const db::ICplxTrans &t, int levels);

  /**
   *  @brief Flatten a cell 
   *
   *  This convenience method is a special version of the other flatten method.
   *
   *  @param cell The cell to flatten
   *  @param levels The number of levels to flatten or -1 for all levels (0: nothing, 1: flatten on level of instances etc.)
   *  @param prune If true, remove all cells which became orphans by the flattening
   */
  void flatten (db::Cell &cell, int levels, bool prune = false);

  /**
   *  @brief Inserts a region (potentially hierarchical) into the given cell and layer
   *
   *  If the region is flat (conceptionally), it will be put into the cell.
   *  If the region is hierarchical, a cell hierarchy will be built below the
   *  given cell.
   */
  void insert (db::cell_index_type cell, int layer, const db::Region &region);

  /**
   *  @brief Inserts a edge collection (potentially hierarchical) into the given cell and layer
   *
   *  If the edge collection is flat (conceptionally), it will be put into the cell.
   *  If the edge collection is hierarchical, a cell hierarchy will be built below the
   *  given cell.
   */
  void insert (db::cell_index_type cell, int layer, const db::Edges &edges);

  /**
   *  @brief Inserts a edge pair collection (potentially hierarchical) into the given cell and layer
   *
   *  If the edge pair collection is flat (conceptionally), it will be put into the cell.
   *  If the edge pair collection is hierarchical, a cell hierarchy will be built below the
   *  given cell.
   */
  void insert (db::cell_index_type cell, int layer, const db::EdgePairs &edge_pairs);

  /**
   *  @brief Inserts a text collection (potentially hierarchical) into the given cell and layer
   *
   *  If the text collection is flat (conceptionally), it will be put into the cell.
   *  If the text collection is hierarchical, a cell hierarchy will be built below the
   *  given cell.
   */
  void insert (db::cell_index_type cell, int layer, const db::Texts &texts);

  /**
   *  @brief Delete a cell plus all subcells 
   *
   *  All subcells referenced directly or indirectly are deleted as well.
   *  All instances of these cells are deleted as well.
   *
   *  @param id The index of the cell to delete
   */
  void delete_cell_rec (cell_index_type id);

  /**
   *  @brief Update the parent/child relationships between the cells
   *
   *  Basically this method needs to be called if changes have been
   *  made to the graph. It will update the parent instance lists
   *  from the child instance lists.
   */
  void update_relations ();

  /**
   *  @brief Transforms the layout with the given transformation
   */
  template <class T>
  void transform (const T &t)
  {
    for (iterator c = begin (); c != end (); ++c) {
      c->transform_into (t);
    }
  }

  /**
   *  @brief Return the number of cells
   *
   *  @return The number of cells (the maximum cell index)
   */
  cell_index_type cells () const
  {
    return (cell_index_type) m_cell_ptrs.size ();
  }
  
  /**
   *  @brief Address a cell by index
   *
   *  @param i The cell index
   *  @return A reference to the cell
   */
  cell_type &cell (cell_index_type i)
  {
    return *(m_cell_ptrs [i]);
  }

  /**
   *  @brief Address a cell by index (const version)
   *
   *  @param i The cell index
   *  @return A reference to the cell
   */
  const cell_type &cell (cell_index_type i) const
  {
    return *(m_cell_ptrs [i]);
  }

  /**
   *  @brief Reserve space for the given number of cells
   */
  void reserve (size_t n)
  {
    m_cell_ptrs.reserve (n);
  }

  /**  
   *  @brief Swap layers
   *
   *  Swaps the shapes of both layers.
   */
  void swap_layers (unsigned int a, unsigned int b);

  /**  
   *  @brief Move a layer
   *
   *  Move a layer from the source to the target. The target is not cleared before, so that this method 
   *  merges shapes from the source with the target layer. The source layer is empty after that operation.
   */
  void move_layer (unsigned int src, unsigned int dest);

  /**
   *  @brief Move a layer (selected shape types only)
   *
   *  Move a layer from the source to the target. The target is not cleared before, so that this method
   *  merges shapes from the source with the target layer. The source layer is empty after that operation.
   */
  void move_layer (unsigned int src, unsigned int dest, unsigned int flags);

  /**
   *  @brief Copy a layer
   *
   *  Copy a layer from the source to the target. The target is not cleared before, so that this method 
   *  merges shapes from the source with the target layer.
   */
  void copy_layer (unsigned int src, unsigned int dest);

  /**
   *  @brief Copy a layer (selected shape types only)
   *
   *  Copy a layer from the source to the target. The target is not cleared before, so that this method
   *  merges shapes from the source with the target layer.
   */
  void copy_layer (unsigned int src, unsigned int dest, unsigned int flags);

  /**
   *  @brief Clear a layer
   *
   *  Clears the layer: removes all shapes.
   */
  void clear_layer (unsigned int n);

  /**
   *  @brief Clear a layer (selected shapes only)
   *
   *  Clears the layer: removes the shapes of the type given the flags (ShapeIterator::shapes_type)
   */
  void clear_layer (unsigned int n, unsigned int flags);

  /**
   *  @brief Delete a layer
   *
   *  This does free the shapes of the cells and remembers the
   *  layer's index for recycling.
   */
  void delete_layer (unsigned int n);

  /**
   *  @brief Copies the shapes of certain cells from the given source layout into this layout
   *
   *  The affected cells are derived from the cell mapping object.
   */
  void copy_tree_shapes (const db::Layout &source_layout, const db::CellMapping &cm);

  /**
   *  @brief Copies the shapes of certain cells from the given source layout into this layout using the given layer mapping
   *
   *  The affected cells are derived from the cell mapping object.
   */
  void copy_tree_shapes (const db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm);

  /**
   *  @brief Moves the shapes of certain cells from the given source layout into this layout
   *
   *  The affected cells are derived from the cell mapping object.
   */
  void move_tree_shapes (db::Layout &source_layout, const db::CellMapping &cm);

  /**
   *  @brief Moves the shapes of certain cells from the given source layout into this layout using the given layer mapping
   *
   *  The affected cells are derived from the cell mapping object.
   */
  void move_tree_shapes (db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm);

  /**
   *  @brief Return true, if the cell index is a valid one
   */
  bool is_valid_cell_index (cell_index_type ci) const;

  /**
   *  @brief Returns true, if a layer index is a valid index for a normal layout layer
   */
  bool is_valid_layer (unsigned int n) const
  {
    return m_layers.layer_state (n) == db::LayoutLayers::Normal;
  }

  /**
   *  @brief Returns true, if a layer index is a free (unused) layer
   */
  bool is_free_layer (unsigned int n) const
  {
    return m_layers.layer_state (n) == db::LayoutLayers::Free;
  }

  /**
   *  @brief Returns true, if a layer index is a special layer index
   */
  bool is_special_layer (unsigned int n) const
  {
    return m_layers.layer_state (n) == db::LayoutLayers::Special;
  }

  /**
   *  @brief Query the number of layers defined so far
   *  
   *  TODO: the list of 0 to nlayers-1 also contains the free layers -
   *  we should get a vector containing the layers that are actually
   *  allocated.
   */
  unsigned int layers () const
  {
    return m_layers.layers ();
  }

  /**
   *  @brief The iterator of valid layers: begin 
   */
  layer_iterator begin_layers () const
  {
    return m_layers.begin_layers ();
  }

  /**
   *  @brief The iterator of valid layers: end 
   */
  layer_iterator end_layers () const
  {
    return m_layers.end_layers ();
  }

  /**
   *  @brief Reserve space for n layers
   */
  void reserve_layers (unsigned int n)
  {
    m_layers.reserve_layers (n);
  }

  /**
   *  @brief begin iterator of the unsorted cell list
   */
  iterator begin () 
  {
    return m_cells.begin ();
  }
  
  /**
   *  @brief end iterator of the unsorted cell list
   */
  iterator end () 
  {
    return m_cells.end ();
  }
  
  /**
   *  @brief begin iterator of the unsorted const cell list
   */
  const_iterator begin () const
  {
    return m_cells.begin ();
  }
  
  /**
   *  @brief end iterator of the unsorted const cell list
   */
  const_iterator end () const
  {
    return m_cells.end ();
  }

  /**
   *  @brief begin iterator of the bottom-up sorted cell list
   *
   *  In bottom-up traversal a cell is not delivered before
   *  the last child cell of this cell has been delivered.
   *  The bottom-up iterator does not deliver cells but cell
   *  indices actually.
   */
  bottom_up_iterator begin_bottom_up () 
  {
    update ();
    return m_top_down_list.rbegin ();
  }
  
  /**
   *  @brief end iterator of the bottom-up sorted cell list
   */
  bottom_up_iterator end_bottom_up () 
  {
    update ();
    return m_top_down_list.rend ();
  }
  
  /**
   *  @brief begin iterator of the const bottom-up sorted cell list
   *
   *  See the non-const version for a detailed description.
   */
  bottom_up_const_iterator begin_bottom_up () const
  {
    update ();
    return m_top_down_list.rbegin ();
  }
  
  /**
   *  @brief end iterator of the const bottom-up sorted cell list
   */
  bottom_up_const_iterator end_bottom_up () const
  {
    update ();
    return m_top_down_list.rend ();
  }
  
  /**
   *  @brief begin iterator of the top-down sorted cell list
   *
   *  The top-down cell list has the property of delivering all
   *  cells before they are instantiated. In addition the first
   *  cells are all top cells. There is at least one top cell.
   *  The top-down iterator does not deliver cells but cell
   *  indices actually.
   */
  top_down_iterator begin_top_down () 
  {
    update ();
    return m_top_down_list.begin ();
  }
  
  /**
   *  @brief end iterator of the top-down sorted cell list
   */
  top_down_iterator end_top_down () 
  {
    update ();
    return m_top_down_list.end ();
  }
  
  /**
   *  @brief begin iterator of the const top-down sorted cell list
   *
   *  See the non-const version for a detailed description.
   */
  top_down_const_iterator begin_top_down () const
  {
    update ();
    return m_top_down_list.begin ();
  }
  
  /**
   *  @brief end iterator of the const top-down sorted cell list
   */
  top_down_const_iterator end_top_down () const
  {
    update ();
    return m_top_down_list.end ();
  }
  
  /**
   *  @brief end iterator of the top cells
   * 
   *  The begin iterator is identical to begin_top_down()
   */
  top_down_iterator end_top_cells ();
  
  /**
   *  @brief end iterator of the top cells
   * 
   *  The begin iterator is identical to begin_top_down()
   */
  top_down_const_iterator end_top_cells () const;
  
  /**
   *  @brief Provide a const version of the update method
   *
   *  This pseudo-const version is required in order to automatically call
   *  update within the begin method for example.
   */
  void update () const;

  /**
   *  @brief Forces an update even if the layout is under construction
   *
   *  This method behaves like "update" but forces and update even if the 
   *  "under_construction" state is active. This allows one to do the update
   *  in certain stages without triggering the update automatically and
   *  too frequently.
   */
  void force_update ();

  /**
   *  @brief Cleans up the layout
   *
   *  This method removes proxy objects which are no longer used.
   *  It can be given a list of cells which need to be kept.
   */
  void cleanup (const std::set<db::cell_index_type> &keep = std::set<db::cell_index_type> ());

  /**
   *  @brief Calls "update" on all cells of the layout
   *
   *  This will update PCells stored inside this layout, but will *not* update
   *  PCells which are imported from a library.
   */
  void refresh ();

  /**
   *  @brief Implementation of the undo operations
   */
  virtual void undo (db::Op *op);

  /**
   *  @brief Implementation of the redo operations
   */
  virtual void redo (db::Op *op);

  /** 
   *  @brief Database unit read accessor
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Database unit write accessor
   */
  void dbu (double d);

  /**
   *  @brief Insert a new layer with the given properties
   */
  unsigned int insert_layer (const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Insert a new layer with the given properties at the given index
   */
  void insert_layer (unsigned int index, const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Gets or creates a layer with the given properties
   *
   *  If there already is a layer matching the given properties, it's index will be
   *  returned. Otherwise a new layer with these properties is created.
   */
  unsigned int get_layer (const db::LayerProperties &props);

  /**
   *  @brief Gets the layer with the given properties or -1 if such a layer does not exist
   */
  int get_layer_maybe (const db::LayerProperties &props) const
  {
    return m_layers.get_layer_maybe (props);
  }

  /**
   *  @brief Insert a new special layer with the given properties
   *
   *  A special layers is used for example to represent rulers.
   */
  unsigned int insert_special_layer (const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Insert a new layer with the given properties at the given index
   *
   *  A special layers is used for example to represent rulers.
   */
  void insert_special_layer (unsigned int index, const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Gets the guiding shape layer
   *
   *  The guiding shape layer is used to store the guiding shapes of PCells
   */
  unsigned int guiding_shape_layer () const
  {
    return m_layers.guiding_shape_layer ();
  }

  /**
   *  @brief Gets the waste layer
   *
   *  The waste layer is used to store shapes that should not be visible and can be cleared at any time.
   */
  unsigned int waste_layer () const
  {
    return m_layers.waste_layer ();
  }

  /**
   *  @brief Gets the error layer
   *
   *  The error layer is used to display error messages.
   */
  unsigned int error_layer () const
  {
    return m_layers.error_layer ();
  }

  /**
   *  @brief Set the properties for a specified layer
   */
  void set_properties (unsigned int i, const LayerProperties &props);

  /**
   *  @brief Get the properties for a specified layer
   */
  const LayerProperties &get_properties (unsigned int i) const
  {
    return m_layers.get_properties (i);
  }

  /**
   *  @brief Signal the start of an operation bringing the layout into invalid state
   *
   *  This method should be called whenever the layout is
   *  about to be brought into an invalid state. After calling
   *  this method, "under_construction" returns false which 
   *  tells foreign code (such as update which might be called
   *  asynchronously for example because of a repaint event)
   *  not to use this layout object.
   *  This state is cancelled by the end_changes () method.
   *  The start_changes () method can be called multiple times
   *  and must be cancelled the same number of times.
   *  If a transaction is used to bracket the start and end of
   *  an operation, the start/end_changes method pair does not
   *  need to be called.
   */
  void start_changes ()
  {
    ++m_invalid;
  }

  /**
   *  @brief Cancel the "in changes" state (see "start_changes")
   */
  void end_changes ()
  {
    if (m_invalid > 0) {
      --m_invalid;
      if (! m_invalid) {
        update ();
      }
    }
  }

  /**
   *  @brief Cancel the "in changes" state (see "start_changes")
   *  This version does not force an update
   */
  void end_changes_no_update ()
  {
    if (m_invalid > 0) {
      --m_invalid;
    }
  }

  /**
   *  @brief Tell if the layout object is under construction
   *
   *  A layout object is either under construction if 
   *  the layout is brought into invalid state by "start_changes".
   */
  bool under_construction () const
  {
    return m_invalid > 0;
  }

  /**
   *  @brief Register a library proxy
   *
   *  This method is used by LibraryProxy to register itself.
   */
  void register_lib_proxy (db::LibraryProxy *lib_proxy);

  /**
   *  @brief Unregister a library proxy
   *
   *  This method is used by LibraryProxy to unregister itself.
   */
  void unregister_lib_proxy (db::LibraryProxy *lib_proxy);

  /**
   *  @brief Gets the editable status of this layout
   *
   *  If this value is true, the layout is editable, i.e. shapes and properties
   *  can be changed.
   */
  bool is_editable () const
  {
    return m_editable;
  }

  /**
   *  @brief Delivers the meta information (begin iterator)
   *
   *  Meta information is additional data that usually is set by the
   *  layout reader and holds information that was obtained when reading
   *  the file. Meta information is format specific.
   */
  meta_info_iterator begin_meta () const
  {
    return m_meta_info.begin ();
  }

  /**
   *  @brief Delivers the meta information (end iterator)
   */
  meta_info_iterator end_meta () const
  {
    return m_meta_info.end ();
  }

  /**
   *  @brief Delivers the meta information (begin iterator) per cell
   */
  meta_info_iterator begin_meta (db::cell_index_type ci) const;

  /**
   *  @brief Delivers the meta information (end iterator) per cell
   */
  meta_info_iterator end_meta (db::cell_index_type ci) const;

  /**
   *  @brief Gets the meta informatio name by ID
   */
  const std::string &meta_info_name (meta_info_name_id_type name_id) const;

  /**
   *  @brief Gets the meta information name ID for a specific string
   */
  meta_info_name_id_type meta_info_name_id (const std::string &name) const;

  /**
   *  @brief Gets the meta information name ID for a specific string (const version)
   */
  meta_info_name_id_type meta_info_name_id (const std::string &name);

  /**
   *  @brief Clears the meta information
   */
  void clear_meta ();

  /**
   *  @brief Adds meta information
   *  The given meta information object is added to the meta information list.
   *  If a meta info object with the same name already exists it is overwritten.
   */
  void add_meta_info (const std::string &name, const MetaInfo &i)
  {
    add_meta_info (meta_info_name_id (name), i);
  }

  /**
   *  @brief Adds meta information (variant with name ID)
   */
  void add_meta_info (meta_info_name_id_type name_id, const MetaInfo &i);

  /**
   *  @brief Adds meta information from a sequence
   */
  template <class I>
  void add_meta_info (const I &b, const I &e)
  {
    for (I i = b; i != e; ++i) {
      m_meta_info.insert (b, e);
    }
  }

  /**
   *  @brief Removes the meta information object with the given name
   *  The method will do nothing if no object with that name exists.
   */
  void remove_meta_info (const std::string &name)
  {
    remove_meta_info (meta_info_name_id (name));
  }

  /**
   *  @brief Removes the meta information object with the given name ID
   */
  void remove_meta_info (meta_info_name_id_type name_id);

  /**
   *  @brief Gets the meta info value for a meta info object with the given name
   *  If no object with that name exists, an empty string is returned
   */
  const MetaInfo &meta_info (const std::string &name) const
  {
    return meta_info (meta_info_name_id (name));
  }

  /**
   *  @brief Gets the meta info value for a meta info object with the given name ID
   */
  const MetaInfo &meta_info (meta_info_name_id_type name_id) const;

  /**
   *  @brief Gets a value indicating whether a meta info with the given name is present
   */
  bool has_meta_info (const std::string &name) const
  {
    return has_meta_info (meta_info_name_id (name));
  }

  /**
   *  @brief Gets a value indicating whether a meta info with the given name is present
   */
  bool has_meta_info (meta_info_name_id_type name_id) const;

  /**
   *  @brief Clears the meta information for a specific cell
   */
  void clear_meta (db::cell_index_type ci);

  /**
   *  @brief Adds meta information for a given cell
   *  The given meta information object is to the meta information list for the given cell.
   *  If a meta info object with the same name already exists it is overwritten.
   */
  void add_meta_info (db::cell_index_type ci, const std::string &name, const MetaInfo &i)
  {
    add_meta_info (ci, meta_info_name_id (name), i);
  }

  /**
   *  @brief Adds meta information for a given cell (version with name ID)
   *  The given meta information object is appended at the end of the meta information list.
   *  If a meta info object with the same name already exists it is overwritten.
   */
  void add_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id, const MetaInfo &i);

  /**
   *  @brief Adds meta information from a sequence
   */
  template <class I>
  void add_meta_info (db::cell_index_type ci, const I &b, const I &e)
  {
    for (I i = b; i != e; ++i) {
      m_meta_info_by_cell [ci].insert (b, e);
    }
  }

  /**
   *  @brief Gets a value indicating whether a meta info with the given name is present for the given cell
   */
  bool has_meta_info (db::cell_index_type ci, const std::string &name) const
  {
    return has_meta_info (ci, meta_info_name_id (name));
  }

  /**
   *  @brief Gets a value indicating whether a meta info with the given name is present for the given cell
   */
  bool has_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id) const;

  /**
   *  @brief Removes the meta information object with the given name from the given cell
   *  The method will do nothing if no object with that name exists.
   */
  void remove_meta_info (db::cell_index_type ci, const std::string &name)
  {
    remove_meta_info (ci, meta_info_name_id (name));
  }

  /**
   *  @brief Removes the meta information object with the given name ID from the given cell
   *  The method will do nothing if no object with that name exists.
   */
  void remove_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id);

  /**
   *  @brief Gets the meta info value for a meta info object with the given name for the given cell
   *  If no object with that name exists, an empty string is returned
   */
  const MetaInfo &meta_info (db::cell_index_type ci, const std::string &name) const
  {
    return meta_info (ci, meta_info_name_id (name));
  }

  /**
   *  @brief Gets the meta info value for a meta info object with the given name ID for the given cell
   *  If no object with that name exists, an empty string is returned
   */
  const MetaInfo &meta_info (db::cell_index_type ci, meta_info_name_id_type name_id) const;

  /**
   *  @brief This event is triggered when the technology changes
   */
  tl::Event technology_changed_event;

  /**
   *  @brief This event is raised when cell variants are built
   *  It will specify a list of cells with their new variants.
   */
  tl::event<const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > *> variants_created_event;

protected:
  /**
   *  @brief Establish the graph's internals according to the dirty flags
   *
   *  This reimplements the LayoutStateModel interface.
   *  Calling do_update investigates the dirty flags and issues update
   *  calls on the cells and it's components as far as necessary.
   *  This will guarantee mainly that region queries can be performed
   *  on all levels of the graph.
   */
  virtual void do_update ();

private:
  db::Library *mp_library;
  cell_list m_cells;
  size_t m_cells_size;
  cell_ptr_vector m_cell_ptrs;
  cell_index_vector m_free_cell_indices;
  mutable unsigned int m_invalid;
  cell_index_vector m_top_down_list;
  size_t m_top_cells;
  LayoutLayers m_layers;
  std::vector<const char *> m_cell_names;
  cell_map_type m_cell_map;
  double m_dbu;
  db::properties_id_type m_prop_id;
  StringRepository m_string_repository;
  GenericRepository m_shape_repository;
  PropertiesRepository m_properties_repository;
  ArrayRepository m_array_repository;
  std::vector<pcell_header_type *> m_pcells;
  pcell_name_map m_pcell_ids;
  lib_proxy_map m_lib_proxy_map;
  bool m_do_cleanup;
  bool m_editable;
  std::map<std::string, meta_info_name_id_type> m_meta_info_name_map;
  std::vector<std::string> m_meta_info_names;
  meta_info_map m_meta_info;
  std::map<db::cell_index_type, meta_info_map> m_meta_info_by_cell;

  std::string m_tech_name;
  tl::Mutex m_lock;

  /**
   *  @brief Sort the cells topologically
   *
   *  Establish a sorting order top-down. The sorted 
   *  cell list can be retrieved using the begin_bottom_up 
   *  ..end_bottom_up iterator pair. Alternatively the
   *  begin_top_down..end_top_down iterator pair can be used
   *  to retrieve the cell indices in top-down order after
   *  the cells have been sorted bottom-up.
   *  This method must be called whenever the graph has changed.
   *  Before this method can be called, update_relations must have
   *  been called.
   *
   *  @return true if the cell graph is OK, false if recursive
   */
  bool topological_sort ();

  /**
   *  @brief Register a cell name for the cell index 
   */
  void register_cell_name (const char *name, cell_index_type ci);

  /**
   *  @brief Allocate a cell index for a new cell
   */
  cell_index_type allocate_new_cell ();

  /**
   *  @brief Implementation of prune_cell and prune_subcells
   */
  void do_prune_cell_or_subcell (cell_index_type id, int levels, bool subcells);

  /**
   *  @brief Implementation of prune_cells and some prune_subcells variants
   */
  void do_prune_cells_or_subcells (const std::set<cell_index_type> &ids, int levels, bool subcells);

  /**
   *  @brief Recovers a proxy without considering the library from context_info
   */
  db::Cell *recover_proxy_no_lib (const LayoutOrCellContextInfo &context_info);
};

/**
 *  @brief Collect memory statistics
 */
inline void
mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const Layout &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A nice helper class that employs RAII for locking the layout against updates
 *
 *  If a layout shall be locked against internal updates temporarily, use this locker:
 *  @code
 *  Layout *ly = ...;
 *  {
 *    db::LayoutLocker locker (ly);
 *    //  the layout is not updated here
 *    ... modify the layout
 *  }
 *  //  now only the layout gets updated
 *  @endcode
 */
class DB_PUBLIC LayoutLocker
{
public:
  explicit LayoutLocker (db::Layout *layout = 0, bool no_update = false)
    : mp_layout (layout), m_no_update (no_update)
  {
    if (mp_layout.get ()) {
      mp_layout->start_changes ();
    }
  }

  ~LayoutLocker ()
  {
    set (0, false);
  }

  LayoutLocker (const LayoutLocker &other)
    : mp_layout (other.mp_layout), m_no_update (other.m_no_update)
  {
    if (mp_layout.get ()) {
      mp_layout->start_changes ();
    }
  }

  LayoutLocker &operator= (const LayoutLocker &other)
  {
    if (this == &other) {
      return *this;
    }

    set (const_cast<db::Layout *> (other.mp_layout.get ()), other.m_no_update);
    return *this;
  }

private:
  tl::weak_ptr<db::Layout> mp_layout;
  bool m_no_update;

  void set (db::Layout *layout, bool no_update)
  {
    if (mp_layout.get ()) {
      if (m_no_update) {
        mp_layout->end_changes_no_update ();
      } else {
        mp_layout->end_changes ();
      }
    }
    mp_layout = layout;
    m_no_update = no_update;
    if (mp_layout.get ()) {
      mp_layout->start_changes ();
    }
  }

};

}

#endif


