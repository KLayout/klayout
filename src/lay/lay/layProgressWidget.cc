
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


#include "layProgressWidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>

#include <math.h>

namespace lay
{

// --------------------------------------------------------------------

class ProgressBarWidget
  : public QWidget
{
public:
  ProgressBarWidget (QWidget *parent, const char *name = "");

  void set_value (double v, const std::string &value);

  QSize sizeHint () const;
  QSize minimumSizeHint () const;

private:
  double m_value;
  std::string m_value_string;
  int m_width;
  int m_length;
  int m_fw;
  int m_bw;

  void paintEvent (QPaintEvent *event); 
  void resizeEvent (QResizeEvent *event); 
};

ProgressBarWidget::ProgressBarWidget (QWidget *parent, const char *name)
  : QWidget (parent),
    m_value (0.0), m_width (64), m_length (0), m_fw (1), m_bw (0)
{
  setObjectName (QString::fromUtf8 (name));
  setMinimumSize (64, 10);
  setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void
ProgressBarWidget::set_value (double v, const std::string &value)
{
  if (value != m_value_string) {
    update ();
    m_value_string = value;
  }

  m_value = v;
  int l = 0;
  if (m_width > 0) {
    l = int (floor ((v < 0.0 ? 0.0 : v) * 0.01 * (double (m_width - 2) - 1e-6))) % m_width;
  }
  if (l != m_length) {
    m_length = l;
    update ();
  }
}

QSize
ProgressBarWidget::sizeHint () const
{
  QFontMetrics fm (font ());
  return QSize (m_width, fm.height () + 2);
}

QSize
ProgressBarWidget::minimumSizeHint () const
{
  return QSize (m_width, 1);
}

void 
ProgressBarWidget::paintEvent (QPaintEvent *)
{
  QPainter painter (this);

  int right = width ();
  int bottom = height ();

  painter.fillRect (QRect (QPoint (m_fw, m_fw), QPoint (m_length + m_fw - 1, bottom - 1 - m_fw)), palette ().brush (QPalette::Highlight));
  painter.fillRect (QRect (QPoint (m_length + m_fw, m_fw), QPoint (right - 1 - m_fw, bottom - 1 - m_fw)), palette ().brush (QPalette::Base));
  painter.setPen (palette ().color (QPalette::Text));
  
  for (int d = 0; d < m_bw; ++d) {
    painter.drawRect (QRect (QPoint (d, d), QPoint (right - 1 - d, bottom - 1 - d)));
  }

  painter.setFont (font ());

  painter.setClipRect (QRect (QPoint (m_fw, m_fw), QPoint (m_length + m_fw - 1, bottom - m_fw)));
  painter.setPen (palette ().color (QPalette::HighlightedText));
  painter.drawText (rect (), Qt::AlignHCenter | Qt::AlignVCenter, tl::to_qstring (m_value_string));

  painter.setClipRect (QRect (QPoint (m_length + m_fw, 0), QPoint (right - m_fw, bottom - m_fw)));
  painter.setPen (palette ().color (QPalette::Text));
  painter.drawText (rect (), Qt::AlignHCenter | Qt::AlignVCenter, tl::to_qstring (m_value_string));
}

void
ProgressBarWidget::resizeEvent (QResizeEvent *)
{
  m_width = size ().width ();
  update ();
}

// --------------------------------------------------------------------

ProgressWidget::ProgressWidget (ProgressReporter *pr, QWidget *parent, bool full_width)
  : QFrame (parent),
    mp_widget (0), mp_pr (pr)
{
  QVBoxLayout *top_layout = new QVBoxLayout (this);
  top_layout->addStretch (1);

  QFrame *bar_frame = new QFrame (this);
  top_layout->addWidget (bar_frame);

  top_layout->addStretch (1);

  //  this does not allow the label to control the overall size, so a long string does not hurt:
  bar_frame->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Preferred);

  QGridLayout *layout = new QGridLayout (bar_frame);
  mp_layout = layout;

  layout->setSpacing (4);
  layout->setMargin (0);

  int col = 0;

  if (! full_width) {
    layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, col, 1, 1);
    layout->setColumnStretch (col++, 1);
  }

  mp_label = new QLabel (bar_frame);
  layout->addWidget (mp_label, 0, col++, 1, 1);

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, col++, 1, 1);

  QFrame *progress_bar_frame = new QFrame (bar_frame);
  progress_bar_frame->setFrameStyle (QFrame::Box | QFrame::Plain);
  progress_bar_frame->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget (progress_bar_frame, 0, col, 1, 1);
  layout->setColumnStretch(col++, 2);

  QGridLayout *pbf_layout = new QGridLayout (progress_bar_frame);
  progress_bar_frame->setLayout (pbf_layout);
  pbf_layout->setMargin (0);
  pbf_layout->setSpacing (0);

  mp_progress_bar1 = new ProgressBarWidget (progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar1, 0, 0, 1, 1);
  mp_progress_bar2 = new ProgressBarWidget (progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar2, 1, 0, 1, 1);
  mp_progress_bar3 = new ProgressBarWidget (progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar3, 2, 0, 1, 1);

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, col++, 1, 1);

  mp_cancel_button = new QToolButton (bar_frame);
  mp_cancel_button->setText (QObject::tr ("Cancel"));
  layout->addWidget (mp_cancel_button, 0, col++, 1, 1);

  if (! full_width) {
    layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, col, 1, 1);
    layout->setColumnStretch (col++, 1);
  }

  layout->addItem (new QSpacerItem (10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0, 1, col);

  m_widget_col = col;

  connect (mp_cancel_button, SIGNAL (clicked ()), this, SLOT (signal_break ()));
}

QWidget *
ProgressWidget::get_widget () const
{
  return mp_widget;
}

void
ProgressWidget::add_widget (QWidget *widget)
{
  remove_widget ();

  if (widget) {
    mp_widget = widget;
    widget->setParent(this);
    mp_layout->addWidget (widget, 2, 0, 1, m_widget_col);
  }
}

void
ProgressWidget::remove_widget ()
{
  if (mp_widget) {
    delete mp_widget;
    mp_widget = 0;
  }
}

void
ProgressWidget::set_progress (tl::Progress *progress)
{
  bool can_cancel = false;
  std::string text;

  if (progress) {
    can_cancel = progress->can_cancel ();
    text = progress->desc ();
  }

  mp_cancel_button->setEnabled (can_cancel);
  mp_label->setText (tl::to_qstring (text));

  lay::ProgressBarWidget *progress_bars[] = { mp_progress_bar1, mp_progress_bar2, mp_progress_bar3 };

  for (size_t i = 0; i < sizeof (progress_bars) / sizeof (progress_bars[0]); ++i) {

    lay::ProgressBarWidget *pb = progress_bars[i];

    if (progress) {

      pb->show ();

      std::string value = progress->formatted_value ();
      double v = progress->value ();
      pb->set_value (v, value);

      progress = progress->next ();

    } else {
      pb->hide ();
    }

  }

  //  according to the doc this should not be required, but without, the progress bar does not resize
  mp_progress_bar1->parentWidget ()->updateGeometry ();
}

void 
ProgressWidget::signal_break ()
{
  mp_pr->signal_break ();
}

QSize  
ProgressWidget::sizeHint () const
{
  return QSize (400, 50);
}

}

