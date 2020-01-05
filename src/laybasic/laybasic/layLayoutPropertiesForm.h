
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_layLayoutPropertiesForm
#define HDR_layLayoutPropertiesForm

#include <QDialog>
#include "ui_LayoutProperties.h"
#include "layCellView.h"

#include <vector>

namespace lay {

class LayoutView;

class LAYBASIC_PUBLIC LayoutPropertiesForm
  : public QDialog, private Ui::LayoutPropertiesForm
{
  Q_OBJECT 

public:
  LayoutPropertiesForm (QWidget *parent, lay::LayoutView *view, const char *name);

  void accept ();
  void commit ();

public slots:
  void layout_selected (int);

private slots:
  void prop_pb_clicked ();

private:
  std::vector <lay::LayoutHandleRef> m_handles;
  lay::LayoutView *mp_view;
  int m_index;
  bool m_editable;
};

}

#endif

