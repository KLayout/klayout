
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


#ifndef HDR_dbLayoutQuery
#define HDR_dbLayoutQuery

#include <vector>
#include <map>
#include <set>
#include <string>

#include "dbLayout.h"
#include "dbLayoutContextHandler.h"
#include "tlExpression.h"
#include "tlProgress.h"

namespace tl
{
  class Variant;
}

namespace db
{

/**
 *  @brief Enum to identify the type of a property
 */
enum LayoutQueryPropertyType
{
  LQ_none = 0,
  LQ_variant,
  LQ_shape,
  LQ_trans,
  LQ_dtrans,
  LQ_layer,
  LQ_instance,
  LQ_cell,
  LQ_point,
  LQ_dpoint,
  LQ_box,
  LQ_dbox,
  LQ_polygon,
  LQ_path,
  LQ_edge,
  LQ_text
};

class FilterStateBase;
class LayoutQuery;

/**
 *  @brief A base class for a filter component
 *
 *  A filter component is one stage in the query path. It defines what items to look for below a given item. 
 *  This is the base class for all filters.
 */
class DB_PUBLIC FilterBase
{
public:
  /**
   *  @brief Constructor
   */
  FilterBase (LayoutQuery *q);

  /**
   *  @brief Destructor
   */
  virtual ~FilterBase () { }

  /**
   *  @brief Create the state object for this filter
   *
   *  This method must be implemented by a filter to provide the state object.
   *  The state object is basically the selector and acts as a iterator for the property that this
   *  filter represents.
   *  This method is provided for implementation by FilterBracket mainly. 
   *  Custom classes should implement do_create_state.
   */
  virtual FilterStateBase *create_state (const std::vector<FilterStateBase *> &followers, db::Layout *layout, tl::Eval &eval, bool single) const;

  /**
   *  @brief Clones this instance
   *
   *  This method must be implemented by derived classes to create a copy of themselves.
   */
  virtual FilterBase *clone (LayoutQuery *q) const;

  /**
   *  @brief Dumps the content (for debugging purposes)
   */
  virtual void dump (unsigned int l) const;

  /**
   *  @brief Gets the follower filters (const version)
   */
  const std::vector<FilterBase *> &followers () const
  {
    return m_followers;
  }

  /**
   *  @brief Gets the follower filters (non-const version)
   */
  std::vector<FilterBase *> &followers () 
  {
    return m_followers;
  }

  /** 
   *  @brief Connect this filter to the given follower filter
   *
   *  Connections form a graph of filters that are evaluated to render the sequence of results.
   */
  void connect (FilterBase *follower);

  /** 
   *  @brief Connect this filter to the given follower filters
   */
  void connect (const std::vector<FilterBase *> &followers);

protected:
  /**
   *  @brief Create the state object for this filter
   *
   *  This method must be implemented by a filter to provide the state object.
   *  The state object is basically the selector and acts as a iterator for the property that this
   *  filter represents.
   *
   *  @param layout The layout that this query refers to.
   *  @param eval The expression context to use for late evaluation of the filter's arguments
   */
  virtual FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const;

  /**
   *  @brief Register a new property
   *
   *  Derived classes can use this method to register properties.
   *  These properties are made available to the query.
   *  The method will return a property ID.
   *  Filter specific state objects must report a value if they are asked for the property 
   *  with the returned property ID.
   *
   *  @param name The property name
   *  @param type The property type
   */
  unsigned int register_property (const std::string &name, LayoutQueryPropertyType type);

private:
  std::vector<FilterBase *> m_followers;
  LayoutQuery *mp_q;
};

/** 
 *  @brief A filter bracket
 *
 *  A bracket is a bracket around a filter graph. In addition, brackets can specify a multiplicity
 *  (loopmin to loopmax). 
 *  A bracket defines two virtual nodes: the entry and the exit node. 
 *  The entry node is the input of the filter and internally connected to the inputs of the children.
 *  The exit node is the output of the filter and the node the children connect to.
 */
class DB_PUBLIC FilterBracket : 
  public FilterBase
{
public:
  /**
   *  @brief Constructor (multiplicity 1)
   */
  FilterBracket (LayoutQuery *q);

  /**
   *  @brief Constructor (multiplicity loopmin..loopmax)
   */
  FilterBracket (LayoutQuery *q, unsigned int loopmin, unsigned int loopmax);

  /**
   *  @brief Destructor
   */
  ~FilterBracket ();

  /**
   *  @brief Set the min multiplicity explicitly
   */
  void set_loopmin (unsigned int v)
  {
    m_loopmin = v;
  }

  /**
   *  @brief Set the max multiplicity explicitly
   */
  void set_loopmax (unsigned int v)
  {
    m_loopmax = v;
  }

  /**
   *  @brief Gets the children (const version)
   */
  const std::vector<FilterBase *> &children () const;

  /**
   *  @brief Adds a filter to the children
   */
  void add_child (FilterBase *follower);

  /**
   *  @brief Adds a filter with the entry node (make it input)
   */
  void connect_entry (FilterBase *child);

  /**
   *  @brief Adds a filter to the exit node (make it output)
   */
  void connect_exit (FilterBase *child);

  /**
   *  @brief Implementation of create_state
   */
  virtual FilterStateBase *create_state (const std::vector<FilterStateBase *> &followers, db::Layout *layout, tl::Eval &eval, bool single) const;

  /**
   *  @brief Implementation of clone
   */
  virtual FilterBase *clone (LayoutQuery *q) const;

  /**
   *  @brief Implementation of dump
   */
  virtual void dump (unsigned int l) const;

  /**
   *  @brief Optimize the bracket - reduce the complexity where possible
   */
  void optimize ();

private:
  std::vector<FilterBase *> m_children;
  FilterBase m_initial, m_closure;
  unsigned int m_loopmin, m_loopmax;

  FilterStateBase *create_state_helper (std::map<const FilterBase *, FilterStateBase *> &fmap, const FilterBase *child, FilterStateBase *closure_state, db::Layout *layout, tl::Eval &eval) const;
};

/**
 *  @brief A structure representing optimization hints for the states
 */
struct DB_PUBLIC FilterStateObjectives
{
public:
  typedef std::set<db::cell_index_type>::const_iterator cell_iterator;

  FilterStateObjectives ();

  FilterStateObjectives &operator+= (const FilterStateObjectives &other);

  void set_wants_all_cells (bool f);
  bool wants_all_cells () const
  {
    return m_wants_all_cells;
  }

  void request_cell (db::cell_index_type ci);
  bool wants_cell (db::cell_index_type ci) const;

  cell_iterator begin_cells () const
  {
    return m_wants_cells.begin ();
  }

  cell_iterator end_cells () const
  {
    return m_wants_cells.end ();
  }

  static FilterStateObjectives everything ();

private:
  bool m_wants_all_cells;
  std::set<db::cell_index_type> m_wants_cells;
};

/**
 *  @brief A base class for the state objects
 *
 *  See \FilterBase for a brief description of the state objects.
 */
class DB_PUBLIC FilterStateBase 
{
public:
  /**
   *  @brief Constructor
   *
   *  @param filter The filter that this base is derived from
   *  @param layout The layout that is subject of the query.
   */
  FilterStateBase (const FilterBase *filter, db::Layout *layout, tl::Eval &eval);

  /**
   *  @brief Destructor
   */
  virtual ~FilterStateBase () { }

  /**
   *  @brief Initializes the filter state object
   *
   *  This method is called after the state graph has been created. It will initialize all followers
   *  and the call the virtual "do_init" method.
   */
  void init (bool recursive = true);

  /**
   *  @brief Reset the iterator
   *
   *  This method can be overloaded to reset the iterator for a new sequence.
   *  Make sure that the base implementation is called.
   */
  virtual void reset (FilterStateBase *previous) 
  {
    mp_previous = previous;
  }

  /**
   *  @brief Increment the iterator
   *
   *  Implement this method to increment the iterator to the next state.
   *  Skip is a flag passed from the query iterator's next function. It indicates
   *  whether an associated operation (i.e. delete or expression evaluation) shall
   *  be skipped.
   */
  virtual void next (bool skip) = 0;

  /**
   *  @brief End test of the iterator 
   *
   *  Implement this method to return true, if the iterator is at the end of the sequence.
   */
  virtual bool at_end () = 0;

  /**
   *  @brief Implementation of the get method (override to implement the actual getter)
   */
  virtual bool get_property (unsigned int id, tl::Variant &v) 
  {
    return mp_previous && mp_previous->get_property (id, v);
  }

  /**
   *  @brief Gets the child state for the current state
   *
   *  This method returns the currently active child (follower) state.
   */
  FilterStateBase *child () const;

  /**
   *  @brief Get the layout object that this query refers to
   */
  db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Gets the FilterBase object that this state is for
   */
  const FilterBase *filter () const
  {
    return mp_filter;
  }

  /**
   *  @brief Returns the expression evaluation object
   */
  tl::Eval &eval () const
  {
    return *mp_eval;
  }

  /**
   *  @brief Add a new follower state.
   */
  void connect (FilterStateBase *follower);

  /**
   *  @brief Add new follower states.
   */
  void connect (const std::vector<FilterStateBase *> &followers);

  /**
   *  @brief Gets the list of followers (const).
   */
  const std::vector<FilterStateBase *> &followers () const
  {
    return m_followers;
  }

  /**
   *  @brief Gets the previous state
   */
  FilterStateBase *previous () const
  {
    return mp_previous;
  }

  /**
   *  @brief A dump method (for debugging).
   */
  virtual void dump () const;

protected:
  /**
   *  @brief Performs the actual initialization
   *
   *  The main intention of this method is to specify optimization hints
   *  for the objectives. Before this method is called the objectives are
   *  initialized as the common objectives of all followers.
   */
  virtual void do_init ();

  FilterStateObjectives &objectives ()
  {
    return m_objectives;
  }

private:
  friend class LayoutQueryIterator;

  FilterStateBase *mp_previous;
  //  Hint: this member is mutable because the child method may dynamically add items to create
  //  a recursion
  mutable std::vector <FilterStateBase *> m_followers;
  const FilterBase *mp_filter;
  db::Layout *mp_layout;
  size_t m_follower;
  tl::Eval *mp_eval;
  FilterStateObjectives m_objectives;

  void proceed (bool skip);
};

/**
 *  @brief the layout query
 */
class DB_PUBLIC LayoutQuery
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates a query from the given string
   */
  LayoutQuery (const std::string &query);

  /**
   *  @brief Destructor
   */
  ~LayoutQuery ();

  /**
   *  @brief Gets the root bracket of the query
   */
  FilterBracket &root ()
  { 
    return *mp_root; 
  }
  
  /**
   *  @brief Gets the root bracket of the query (const)
   */
  const FilterBracket &root () const 
  { 
    return *mp_root; 
  }

  /**
   *  @brief Used by the filters to register a property.
   */
  unsigned int register_property (const std::string &name, LayoutQueryPropertyType type);

  /**
   *  @brief Gets the number of registered properties.
   */
  unsigned int properties () const
  {
    return (unsigned int) m_properties.size ();
  }

  /**
   *  @brief Gets the property name for the property with the given id.
   */
  const std::string &property_name (unsigned int id) const;

  /**
   *  @brief Gets the property type for the property with the given id.
   */
  LayoutQueryPropertyType property_type (unsigned int id) const;

  /**
   *  @brief Gets a value that indicates whether the property with the given name exists.
   */
  bool has_property (const std::string &name) const;

  /**
   *  @brief Finds the id for the property with the given name.
   */
  unsigned int property_by_name (const std::string &name) const;

  /**
   *  @brief Executes the query
   *
   *  This method can be used to execute "active" queries such 
   *  as "delete" or "with ... do".
   *  It is basically equivalent to iterating over the query until it is
   *  done.
   *
   *  The context provides a way to define variables and functions.
   */
  void execute (db::Layout &layout, tl::Eval *context = 0);
  
  /**
   *  @brief A dump method (for debugging)
   */
  void dump () const;
  
private:
  struct PropertyDescriptor
  {
    PropertyDescriptor (LayoutQueryPropertyType t, unsigned int i, const std::string &n)
      : type (t), id (i), name (n)
    {
    }

    LayoutQueryPropertyType type;
    unsigned int id;
    std::string name;
  };

  FilterBracket *mp_root;
  std::vector <PropertyDescriptor> m_properties;
  std::map <std::string, unsigned int> m_property_ids_by_name;

  //  no copying currently (requires a clone method for the filter object)
  LayoutQuery (const LayoutQuery &d);
  LayoutQuery &operator= (const LayoutQuery &d);
};

/**
 *  @brief An iterator used to iterate over the query
 */
class DB_PUBLIC LayoutQueryIterator
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   *
   *  @param q The query that this iterator walks over
   *  @param layout The layout to which the query is applied
   */
  LayoutQueryIterator (const LayoutQuery &q, db::Layout *layout, tl::Eval *parent_eval = 0, tl::AbsoluteProgress *progress = 0);

  /**
   *  @brief Constructor
   *
   *  @param q The query that this iterator walks over
   *  @param layout The layout to which the query is applied
   */
  LayoutQueryIterator (const LayoutQuery &q, const db::Layout *layout, tl::Eval *parent_eval = 0, tl::AbsoluteProgress *progress = 0);

  /**
   *  @brief Destructor
   */
  ~LayoutQueryIterator ();

  /**
   *  @brief Reset the iterator to the initial state 
   */
  void reset ();

  /**
   *  @brief Returns true if the iterator is at the end.
   */
  bool at_end () const;

  /**
   *  @brief Increment the iterator: deliver the next state
   */
  LayoutQueryIterator &operator++ ()
  {
    next (false);
    return *this;
  }

  /**
   *  @brief Increment the iterator: deliver the next state
   *
   *  This method is equivalent to operator++, but it allows one to specify
   *  a boolean parameter telling whether the operation requested shall be
   *  skipped. This only applies to queries with an action like "delete" or "with".
   */
  void next (bool skip);

  /**
   *  @brief Gets the query object.
   */
  const LayoutQuery *query () const
  {
    return mp_q.get ();
  }

  /**
   *  @brief Gets the layout object that this iterator runs for
   */
  const Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Gets a property for the current state (property is given by name).
   *
   *  @param name The name of the property
   *  @param v The value of the property
   *  @return True, if the property could be delivered.
   */
  bool get (const std::string &name, tl::Variant &v);

  /**
   *  @brief Gets a property for the current state (property is given by ID).
   *
   *  @param id The ID of the property
   *  @param v The value of the property
   *  @return True, if the property could be delivered.
   */
  bool get (unsigned int id, tl::Variant &v);

  /**
   *  @brief Get the eval object which provides access to the properties through expressions
   */
  tl::Eval &eval ()
  {
    return m_eval;
  }

  /**
   *  @brief Dump method (for debugging)
   */
  void dump () const;

private:
  FilterStateBase *mp_root_state;
  std::vector<FilterStateBase *> m_state;
  tl::weak_ptr<LayoutQuery> mp_q;
  db::Layout *mp_layout;
  tl::Eval m_eval;
  db::LayoutContextHandler m_layout_ctx;
  tl::AbsoluteProgress *mp_progress;
  bool m_initialized;

  void ensure_initialized ();
  void collect (FilterStateBase *state, std::set<FilterStateBase *> &states);
  void next_up (bool skip);
  bool next_down ();
  void cleanup ();
  void init ();

  //  no copying currently (requires a clone method for the state object)
  LayoutQueryIterator (const LayoutQueryIterator &i);
  LayoutQueryIterator &operator= (const LayoutQueryIterator &i);
};

}

#endif

