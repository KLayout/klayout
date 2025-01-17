
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_laySaltModel
#define HDR_laySaltModel

#include "layCommon.h"

#include <QObject>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <string>
#include <set>
#include <map>
#include <vector>

namespace lay
{

class Salt;
class SaltGrain;

/**
 *  @brief A model representing the salt grains for a QListView
 */
class SaltModel
  : public QAbstractItemModel
{
Q_OBJECT

public:
  /**
   *  @brief An enum describing the severity of a message
   */
  enum Severity
  {
    None = 0,
    Info = 1,
    Warning = 2,
    Error = 3
  };

  /**
   *  @brief Constructor
   */
  SaltModel (QObject *parent, lay::Salt *salt, Salt *salt_filtered = 0, bool salt_exclude = false);

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  QVariant data (const QModelIndex &index, int role) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  Qt::ItemFlags flags (const QModelIndex &index) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  QModelIndex index (int row, int column, const QModelIndex &parent) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  QModelIndex parent (const QModelIndex & /*index*/) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  int columnCount(const QModelIndex & /*parent*/) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  int rowCount (const QModelIndex &parent) const;

  /**
   *  @brief Gets the grain pointer from a model index
   */
  SaltGrain *grain_from_index (const QModelIndex &index) const;

  /**
   *  @brief Marks the model as "under construction"
   *  This method can be called (multiple times) before update to mark the model
   *  as being under construction. update() will end this state.
   */
  void begin_update ();

  /**
   *  @brief Updates the model
   *  Needs to be called when the salt has changed.
   */
  void update ();

  /**
   *  @brief Sets the explanation text for an empty list (i.e. "there are no new packages")
   */
  void set_empty_explanation (const QString &text);

  /**
   *  @brief Sets or resets the "marked" flag on the grain with the given name
   */
  void set_marked (const std::string &name, bool marked);

  /**
   *  @brief Clears the marked state of all grains
   */
  void clear_marked ();

  /**
   *  @brief Sets the marked state of all grains
   */
  void mark_all ();

  /**
   *  @brief Enables or disables the grain with the given name
   */
  void set_enabled (const std::string &name, bool enabled);

  /**
   *  @brief Enables all grains
   */
  void enable_all ();

  /**
   *  @brief Installs a message on the grain with the given name
   *  Installing an empty message basically removes the message.
   */
  void set_message (const std::string &name, Severity severity, const std::string &message);

  /**
   *  @brief Removes a message
   */
  void reset_message (const std::string &name)
  {
    set_message (name, None, std::string ());
  }

  /**
   *  @brief Clears all messages
   */
  void clear_messages ();

  /**
   *  @brief Sets the display order
   *  Specifying a display order for a name will make the grain appear
   *  before or after other grains.
   *  "update" needs to be called before the order becomes active.
   *  Non-assigned items are considered to have order (0).
   */
  void set_order (const std::string &name, int order);

  /**
   *  @brief Resets any display order
   */
  void reset_order (const std::string &name);

  /**
   *  @brief Resets all display order specs
   */
  void clear_order ();

public:
  lay::Salt *mp_salt, *mp_salt_filtered;
  bool m_salt_exclude;
  std::set<std::string> m_marked;
  std::set<std::string> m_disabled;
  std::map<std::string, std::pair<Severity, std::string> > m_messages;
  std::map<std::string, int> m_display_order;
  std::vector<SaltGrain *> m_ordered_grains;
  bool m_in_update;
  QString m_empty_explanation;

  bool is_marked (const std::string &name) const;
  bool is_enabled (const std::string &name) const;
  void create_ordered_list ();
};

}

#endif
