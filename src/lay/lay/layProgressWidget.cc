
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


#include "layProgressWidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QListView>

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
    m_value (0.0), m_width (200), m_length (0), m_fw (1), m_bw (0)
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
#if QT_VERSION >= 0x60000
  return QSize (fm.horizontalAdvance (QString::fromUtf8("100%")) * 4, fm.height () + 2);
#else
  return QSize (fm.width (QString::fromUtf8("100%")) * 4, fm.height () + 2);
#endif
}

QSize
ProgressBarWidget::minimumSizeHint () const
{
  return QSize (50, 1);
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

ProgressWidget::ProgressWidget (ProgressReporter *pr, QWidget *parent, bool fw)
  : QFrame (parent),
    mp_widget (0), mp_pr (pr), m_log_file (0, true), m_log_visible (false)
{
  QVBoxLayout *top_layout = new QVBoxLayout (this);
  top_layout->addStretch (1);

  mp_log_frame = new QFrame (this);
  mp_log_frame->setFrameShape (QFrame::NoFrame);
  mp_log_frame->hide ();
  top_layout->addWidget (mp_log_frame);

  QVBoxLayout *log_layout = new QVBoxLayout (mp_log_frame);

  mp_log_label = new QLabel (mp_log_frame);
  mp_log_label->setText (QString ());
  mp_log_label->setSizePolicy (QSizePolicy (QSizePolicy::Ignored, QSizePolicy::Preferred));
  log_layout->addWidget (mp_log_label);

  QListView *log_view = new QListView (this);
  log_view->setModel (&m_log_file);
  log_view->setUniformItemSizes (true);
  log_layout->addWidget (log_view);

  QFrame *attn_frame = new QFrame (this);
  attn_frame->setFrameShape (QFrame::NoFrame);
  attn_frame->hide ();
  log_layout->addWidget (attn_frame);

  QHBoxLayout *attn_layout = new QHBoxLayout (attn_frame);
  attn_layout->setContentsMargins (0, 0, 0, 0);

  QLabel *attn_label1 = new QLabel (attn_frame);
  attn_label1->setPixmap (QPixmap (QString::fromUtf8 (":/warn_16px@2x.png")));
  attn_layout->addWidget (attn_label1);

  QLabel *attn_label2 = new QLabel (attn_frame);
  attn_label2->setText (tr ("There are errors or warnings"));
  attn_layout->addWidget (attn_label2);

  attn_layout->addStretch (1);

  connect (&m_log_file, SIGNAL (layoutChanged ()), log_view, SLOT (scrollToBottom ()));
  connect (&m_log_file, SIGNAL (attention_changed (bool)), attn_frame, SLOT (setVisible (bool)));

  QFrame *bar_frame = new QFrame (this);
  top_layout->addWidget (bar_frame);

  top_layout->addStretch (1);

  //  this does not allow the label to control the overall size, so a long string does not hurt:
  bar_frame->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Preferred);

  QGridLayout *layout = new QGridLayout (bar_frame);
  mp_layout = layout;

  layout->setSpacing (4);
  layout->setContentsMargins (0, 0, 0, 0);

  int col = 0;

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, col, 1, 1);
  m_left_col = col++;

  mp_label = new QLabel (bar_frame);
  layout->setColumnStretch(col, 2);
  layout->addWidget (mp_label, 0, col++, 1, 1);

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, col++, 1, 1);

  mp_progress_bar_frame = new QFrame (bar_frame);
  mp_progress_bar_frame->setFrameStyle (QFrame::Box | QFrame::Plain);
  mp_progress_bar_frame->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget (mp_progress_bar_frame, 0, col, 1, 1);

  QGridLayout *pbf_layout = new QGridLayout (mp_progress_bar_frame);
  mp_progress_bar_frame->setLayout (pbf_layout);
  pbf_layout->setContentsMargins (0, 0, 0, 0);
  pbf_layout->setSpacing (0);

  mp_progress_bar1 = new ProgressBarWidget (mp_progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar1, 0, 2, 1, 1);
  mp_progress_bar2 = new ProgressBarWidget (mp_progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar2, 0, 1, 1, 1);
  mp_progress_bar3 = new ProgressBarWidget (mp_progress_bar_frame);
  pbf_layout->addWidget (mp_progress_bar3, 0, 0, 1, 1);

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, col++, 1, 1);

  mp_cancel_button = new QToolButton (bar_frame);
  mp_cancel_button->setText (QObject::tr ("Cancel"));
  layout->addWidget (mp_cancel_button, 0, col++, 1, 1);

  layout->addItem (new QSpacerItem (8, 8, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, col, 1, 1);
  m_right_col = col++;

  layout->addItem (new QSpacerItem (10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0, 1, col);

  m_widget_col = col;

  connect (mp_cancel_button, SIGNAL (clicked ()), this, SLOT (signal_break ()));

  set_full_width (fw);
}

void
ProgressWidget::set_log_visible (tl::Progress *progress)
{
  if ((progress != 0) != m_log_visible) {
    m_log_visible = (progress != 0);
    mp_log_frame->setVisible (m_log_visible);
    mp_log_label->setText (progress ? tl::to_qstring (progress->desc ()) : QString ());
    set_full_width (m_full_width);
  }
}
void
ProgressWidget::set_full_width (bool fw)
{
  m_full_width = fw;

  bool f = (fw || m_log_visible);
  mp_layout->setColumnStretch (m_left_col, f ? 0 : 1);
  mp_layout->setColumnStretch (m_right_col, f ? 0 : 1);
}

bool
ProgressWidget::full_width () const
{
  return m_full_width;
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
  lay::ProgressBarWidget *progress_bars[] = { mp_progress_bar1, mp_progress_bar2, mp_progress_bar3 };

  if (! progress || progress->is_abstract ()) {

    if (! progress) {
      m_log_file.clear ();
    }
    m_log_file.set_max_entries (progress ? 1000 : 0);

    set_log_visible (progress);

    mp_progress_bar_frame->hide ();
    mp_cancel_button->setEnabled (true);
    mp_label->setText (QString ());

    return;

  }

  bool can_cancel = false;
  std::string text;

  if (progress) {
    can_cancel = progress->can_cancel ();
    text = progress->desc ();
  }

  mp_cancel_button->setEnabled (can_cancel);
  mp_label->setText (tl::to_qstring (text));

  for (size_t i = 0; i < sizeof (progress_bars) / sizeof (progress_bars[0]); ++i) {

    lay::ProgressBarWidget *pb = progress_bars[i];

    if (progress) {

      pb->show ();

      std::string value = progress->formatted_value ();
      double v = progress->value ();
      pb->set_value (v, value);

      if (progress->final ()) {
        progress = 0;
      } else {
        progress = progress->next ();
      }

    } else {
      pb->hide ();
    }

  }

  mp_progress_bar_frame->show ();

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

