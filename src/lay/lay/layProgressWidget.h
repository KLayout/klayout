
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


#ifndef HDR_layProgressWidget
#define HDR_layProgressWidget

#include <QFrame>
#include <QToolButton>
#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>

#include "layProgress.h"

class QToolButton;
class QLabel;
class QToolButton;
class QGridLayout;

namespace tl
{
  class Progress;
}

namespace lay
{

class ProgressBarWidget;

class ProgressWidget
  : public QFrame
{
Q_OBJECT

public:
  ProgressWidget (ProgressReporter *pr, QWidget *parent, bool full_width = false);

  void set_progress (tl::Progress *progress);
  void add_widget (QWidget *widget);
  void remove_widget ();
  QWidget *get_widget () const;

  QSize sizeHint () const;

public slots:
  void signal_break ();

private:
  QLabel *mp_label;
  ProgressBarWidget *mp_progress_bar1, *mp_progress_bar2, *mp_progress_bar3;
  QWidget *mp_widget;
  int m_widget_col;
  QGridLayout *mp_layout;
  QToolButton *mp_cancel_button;
  ProgressReporter *mp_pr;
};

}

#endif

