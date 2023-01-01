
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
#include "dbManager.h"

namespace gsi
{

static bool has_undo(const db::Manager *manager)
{
  return manager->available_undo ().first;
}

static std::string transaction_for_undo(const db::Manager *manager)
{
  return manager->available_undo ().second;
}

static bool has_redo(const db::Manager *manager)
{
  return manager->available_redo ().first;
}

static std::string transaction_for_redo(const db::Manager *manager)
{
  return manager->available_redo ().second;
}

static db::Manager::transaction_id_t transaction1(db::Manager *manager, const std::string &description)
{
  return manager->transaction (description);
}

static db::Manager::transaction_id_t transaction2(db::Manager *manager, const std::string &description, db::Manager::transaction_id_t id)
{
  return manager->transaction (description, id);
}

Class<db::Manager> decl_Manager ("db", "Manager",
  gsi::method_ext ("transaction", &transaction1, gsi::arg ("description"),
    "@brief Begin a transaction\n"
    "\n"
    "\n"
    "This call will open a new transaction. A transaction consists\n"
    "of a set of operations issued with the 'queue' method.\n"
    "A transaction is closed with the 'commit' method.\n"
    "\n"
    "@param description The description for this transaction.\n"
    "\n"
    "@return The ID of the transaction (can be used to join other transactions with this one)\n"
  ) +
  gsi::method_ext ("transaction", &transaction2, gsi::arg ("description"), gsi::arg ("join_with"),
    "@brief Begin a joined transaction\n"
    "\n"
    "\n"
    "This call will open a new transaction and join if with the previous transaction.\n"
    "The ID of the previous transaction must be equal to the ID given with 'join_with'.\n"
    "\n"
    "This overload was introduced in version 0.22.\n"
    "\n"
    "@param description The description for this transaction (ignored if joined).\n"
    "@param description The ID of the previous transaction.\n"
    "\n"
    "@return The ID of the new transaction (can be used to join more)\n"
  ) +
  gsi::method ("commit", &db::Manager::commit,
    "@brief Close a transaction.\n"
  ) +
  gsi::method ("undo", &db::Manager::undo,
    "@brief Undo the current transaction\n"
    "\n"
    "The current transaction is undone with this method.\n"
    "The 'has_undo' method can be used to determine whether\n"
    "there are transactions to undo.\n"
  ) +
  gsi::method ("redo", &db::Manager::redo,
    "@brief Redo the next available transaction\n"
    "\n"
    "The next transaction is redone with this method.\n"
    "The 'has_redo' method can be used to determine whether\n"
    "there are transactions to undo.\n"
  ) +
  gsi::method_ext ("has_undo?", &has_undo,
    "@brief Determine if a transaction is available for 'undo'\n"
    "\n"
    "@return True, if a transaction is available.\n"
  ) +
  gsi::method_ext ("transaction_for_undo", &transaction_for_undo,
    "@brief Return the description of the next transaction for 'undo'\n"
  ) +
  gsi::method_ext ("has_redo?", &has_redo,
    "@brief Determine if a transaction is available for 'redo'\n"
    "\n"
    "@return True, if a transaction is available.\n"
  ) +
  gsi::method_ext ("transaction_for_redo", &transaction_for_redo,
    "@brief Return the description of the next transaction for 'redo'\n"
  ),
  "@brief A transaction manager class\n"
  "\n"
  "Manager objects control layout and potentially other objects in the layout database "
  "and queue operations to form transactions. A transaction is a sequence of "
  "operations that can be undone or redone.\n"
  "\n"
  "In order to equip a layout object with undo/redo support, instantiate the layout object "
  "with a manager attached and embrace the operations to undo/redo with transaction/commit calls.\n"
  "\n"
  "The use of transactions is subject to certain constraints, i.e. transacted sequences may not be "
  "mixed with non-transacted ones.\n"
  "\n"
  "This class has been introduced in version 0.19.\n"
);

}

