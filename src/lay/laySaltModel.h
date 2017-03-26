
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
   *  @brief Constructor
   */
  SaltModel (QObject *parent, lay::Salt *salt);

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  QVariant data (const QModelIndex &index, int role) const;

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
   *  @brief Updates the model
   *  Needs to be called when the salt has changed.
   */
  void update ();

  /**
   *  @brief Sets or resets the "marked" flag on the grain with the given name
   */
  void set_marked (const std::string &name, bool marked);

  /**
   *  @brief Installs a message on the grain with the given name
   *  Installing an empty message basically removes the message.
   */
  void set_message (const std::string &name, const std::string &message);

public:
  lay::Salt *mp_salt;
  std::set<std::string> m_marked;
  std::map<std::string, std::string> m_messages;

  bool is_marked (const std::string &name) const;
};

// --------------------------------------------------------------------------------------

/**
 *  @brief A delegate displaying the summary of a grain
 */
class SaltItemDelegate
  : public QStyledItemDelegate
{
public:
  SaltItemDelegate (QObject *parent);

  void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
