
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

#ifndef HDR_laySelectLineStyleForm
#define HDR_laySelectLineStyleForm

#include <QDialog>

#include "layLineStyles.h"

class QListWidgetItem;

namespace Ui
{
  class SelectLineStyleForm;
}

namespace lay
{

class SelectLineStyleForm
  : public QDialog
{
  Q_OBJECT 

public:
  SelectLineStyleForm (QWidget *parent, const lay::LineStyles &styles, bool include_nil = false);

  ~SelectLineStyleForm ();

  int selected () const
  {
    return m_selected;
  }

  void set_selected (int selected);
  
public slots:
  void sel_changed (QListWidgetItem *current, QListWidgetItem *); 

protected:
  void update ();

private:
  Ui::SelectLineStyleForm *mp_ui;
  int m_selected;
  lay::LineStyles m_styles;
  bool m_include_nil;
};

}

#endif

#endif  //  defined(HAVE_QT)
