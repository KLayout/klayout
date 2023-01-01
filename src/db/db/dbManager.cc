
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


#include "dbObject.h"
#include "dbStatic.h"
#include "tlException.h"
#include "tlAssert.h"
#include "tlProgress.h"
#include "tlLog.h"

#include <cstdio>

namespace db
{

Manager::Manager (bool enabled)
  : m_transactions (),
    m_current (m_transactions.begin ()), 
    m_opened (false), m_replay (false),
    m_enabled (enabled)
{
  //  .. nothing yet ..
}

Manager::~Manager ()
{
  clear ();

  for (std::vector<db::Object *>::const_iterator obj = m_id_table.begin (); obj != m_id_table.end (); ++obj)
  {
    if (*obj) {
      (*obj)->manager (0);
    }
  }

  m_id_table.clear ();
  m_unused_ids.clear ();
}

db::Object *
Manager::object_by_id (ident_t id)
{
  if (id >= m_id_table.size ()) {
    return 0;
  } else {
    return m_id_table [id];
  }
}

void 
Manager::release_object (ident_t id)
{
  m_id_table [id] = 0;
  m_unused_ids.push_back (id);
}

Manager::ident_t 
Manager::next_id (db::Object *obj)
{
  if (m_unused_ids.size () > 0) {
    ident_t id = m_unused_ids.back ();
    m_unused_ids.pop_back ();
    m_id_table [id] = obj;
    return id;
  } else {
    m_id_table.push_back (obj);
    return ident_t (m_id_table.size ()) - 1;
  }
}

void
Manager::clear ()
{
  tl_assert (! m_replay);
  m_opened = false;
  erase_transactions (m_transactions.begin (), m_transactions.end ());
  m_current = m_transactions.begin ();
}

void
Manager::erase_transactions (transactions_t::iterator from, transactions_t::iterator to)
{
  for (transactions_t::iterator i = from; i != to; ++i) {
    for (operations_t::iterator o = i->first.begin (); o != i->first.end (); ++o) {
      delete o->second;
    }
  }
  m_transactions.erase (from, to);
}

Manager::transaction_id_t 
Manager::transaction (const std::string &description, transaction_id_t join_with)
{
  if (m_enabled) {

    //  close transactions that are still open (was an assertion before)
    if (m_opened) {
      tl::warn << tl::to_string (tr ("Transaction still opened: ")) << m_current->second;
      commit ();
    }

    tl_assert (! m_replay);

    if (! m_transactions.empty () && reinterpret_cast<transaction_id_t> (& m_transactions.back ()) == join_with) {
      m_transactions.back ().second = description;
    } else {
      //  delete all following transactions and add a new one
      erase_transactions (m_current, m_transactions.end ());
      m_transactions.push_back (transaction_t (operations_t (), description));
    }
    m_current = m_transactions.end ();
    --m_current;
    m_opened = true;
  
  }

  size_t id = m_transactions.empty () ? 0 : reinterpret_cast<transaction_id_t> (& m_transactions.back ());
  return id;
}

Manager::transaction_id_t 
Manager::last_transaction_id () const
{
  return m_transactions.empty () ? 0 : reinterpret_cast<transaction_id_t> (& m_transactions.back ());
}

Manager::transaction_id_t
Manager::transaction_id_for_undo () const
{
  transactions_t::iterator c = m_current;
  if (c == m_transactions.begin ()) {
    return 0;
  } else {
    --c;
    return reinterpret_cast<transaction_id_t> (c.operator-> ());
  }
}

Manager::transaction_id_t
Manager::transaction_id_for_redo () const
{
  if (m_current == m_transactions.end ()) {
    return 0;
  } else {
    return reinterpret_cast<transaction_id_t> (m_current.operator-> ());
  }
}

void
Manager::cancel ()
{
  //  equivalent to commit and undo. But takes care that an empty commit is not followed by undo
  //  (which would undo the previous transaction!)
  if (m_enabled) {

    tl_assert (m_opened);
    tl_assert (! m_replay);
    m_opened = false;

    if (m_current->first.begin () != m_current->first.end ()) {
      ++m_current;
      undo ();
    }

    //  wipe following history as we don't want the cancelled operation to be redoable
    erase_transactions (m_current, m_transactions.end ());
    m_current = m_transactions.end ();

  }
}

void 
Manager::commit ()
{
  if (m_enabled) {

    tl_assert (m_opened);
    tl_assert (! m_replay);
    m_opened = false;

    //  delete transactions that are empty
    if (m_current->first.begin () != m_current->first.end ()) {
      ++m_current;
    } else {
      erase_transactions (m_current, m_transactions.end ());
      m_current = m_transactions.end ();
    }

  }
}

void 
Manager::undo ()
{
  //  anything to undo?
  if (m_current == m_transactions.begin ()) {
    return;
  }

  tl_assert (! m_opened);
  tl_assert (! m_replay);

  m_replay = true;
  --m_current;

  tl::RelativeProgress progress (tl::to_string (tr ("Undoing")), m_current->first.size (), 10);

  try {

    for (operations_t::reverse_iterator o = m_current->first.rbegin (); o != m_current->first.rend (); ++o) {

      tl_assert (o->second->is_done ());
      db::Object *obj = object_by_id (o->first);
      tl_assert (obj != 0);
      obj->undo (o->second);
      o->second->set_done (false);

      ++progress;

    }

    m_replay = false;

  } catch (...) {
    m_replay = false;
    clear ();
  }
}

void 
Manager::redo ()
{
  //  anything to redo?
  if (m_current == m_transactions.end ()) {
    return;
  }

  tl_assert (! m_opened);
  tl_assert (! m_replay);

  tl::RelativeProgress progress (tl::to_string (tr ("Redoing")), m_current->first.size (), 10);

  try {

    m_replay = true;
    for (operations_t::iterator o = m_current->first.begin (); o != m_current->first.end (); ++o) {

      tl_assert (! o->second->is_done ());
      db::Object *obj = object_by_id (o->first);
      tl_assert (obj != 0);
      obj->redo (o->second);
      o->second->set_done (true);

      ++progress;

    }
    ++m_current;
    m_replay = false;

  } catch (...) {
    m_replay = false;
    clear ();
  }
}

std::pair<bool, std::string> 
Manager::available_undo () const
{
  if (m_opened || m_current == m_transactions.begin ()) {
    return std::make_pair (false, std::string (""));
  } else {
    transactions_t::const_iterator t = m_current;
    --t;
    return std::make_pair (true, t->second);
  }
}

std::pair<bool, std::string> 
Manager::available_redo () const
{
  if (m_opened || m_current == m_transactions.end ()) {
    return std::make_pair (false, std::string (""));
  } else {
    return std::make_pair (true, m_current->second);
  }
}

db::Op *
Manager::last_queued (db::Object *object) 
{
  tl_assert (m_opened);
  tl_assert (! m_replay);

  if (m_current == m_transactions.end () || m_current->first.empty () || (object && m_current->first.back ().first != object->id ())) {
    return 0;
  } else {
    return m_current->first.back ().second;
  }
}

void 
Manager::queue (db::Object *object, db::Op *op)
{
  tl_assert (! m_replay);

  //  when not open, ignore that call
  if (! m_opened) {
    delete op;
  } else {

    //  implicitly call redo if the operation was not in done state before.
    if (! op->is_done ()) {
      object->redo (op);
      op->set_done (true);
    }

    m_current->first.push_back (std::make_pair (object->id (), op));

  }
}


} // namespace db

