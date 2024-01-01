
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

#ifndef HDR_layLayoutStatisticsForm
#define HDR_layLayoutStatisticsForm

#include <QDialog>
#include "ui_LayoutStatistics.h"
#include "layCellView.h"

#include <vector>

namespace lay {

class LayoutViewBase;
class StatisticsSource;

class LayoutStatisticsForm
  : public QDialog, private Ui::LayoutStatisticsForm
{
  Q_OBJECT 

public:
  LayoutStatisticsForm (QWidget *parent, lay::LayoutViewBase *view, const char *name);
  ~LayoutStatisticsForm ();

public slots:
  void layout_selected (int);

private:
  std::vector <lay::LayoutHandleRef> m_handles;
  StatisticsSource *mp_source;
};

}

#endif

#endif  //  defined(HAVE_QT)
