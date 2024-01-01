
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#if defined(HAVE_QT)

#ifndef HDR_layNetInfoDialog
#define HDR_layNetInfoDialog

#include "ui_NetInfoDialog.h"

#include "dbLayoutToNetlist.h"
#include "tlObjectCollection.h"

#include <QDialog>

namespace Ui
{
  class NetInfoDialog;
}

namespace lay
{

/**
 *  @brief A dialog showing the details of a net
 */
class NetInfoDialog
  : public QDialog
{
  Q_OBJECT

public:
  NetInfoDialog (QWidget *parent);
  ~NetInfoDialog ();

  void set_nets (const db::LayoutToNetlist *l2ndb, const std::vector<const db::Net *> &nets);

private slots:
  void detailed_checkbox_clicked ();

protected:
  void showEvent (QShowEvent *);

private:
  tl::weak_ptr<db::LayoutToNetlist> mp_l2ndb;
  tl::weak_collection<db::Net> mp_nets;
  bool m_needs_update;
  Ui::NetInfoDialog *ui;

  void update_info_text ();
  void needs_update ();
};

}

#endif

#endif  //  defined(HAVE_QT)
