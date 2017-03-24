
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

public:
  lay::Salt *mp_salt;
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
