
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


#include "gtfUiDialog.h"
#include "ui_gtfUiDialogUI.h"
#include "tlLog.h"

#include <iostream>
#include <sstream>
#include <functional>

#include <QScrollBar>
#include <QPainter>
#include <QHeaderView>
#include <QTreeView>
#include <QTreeWidget>

namespace gtf
{

static QBrush left_diff_brush (QColor (192, 64, 64));
static QBrush left_diff_brush_dep (QColor (192, 64, 64), Qt::Dense4Pattern);
static QBrush right_diff_brush (QColor (64, 192, 64));
static QBrush right_diff_brush_dep (QColor (64, 192, 64), Qt::Dense4Pattern);

// --------------------------------------------------------------------------
//  diff implementation

/**
 *  @brief diff two sequences [b1,e1) [b2,e2)
 *
 *  The output is delivered to these output iterators:
 *    "common" will receive all elements that are common to stream 1 and 2,
 *    "o1" those that only appear in stream 1, "o2" only those that appear in stream 2,
 *    "d" will receive std::pair's of elements that differ between 1 and 2.
 *  Two parameters govern the algorithm:
 *    "max_lookahead": tell how far to look into the future to find "synchronized" parts.
 *    "min_sync": how many elements must be equal (in sequence) to resync.
 */
template <class I, class O1, class O2, class O3, class O4, class EQ> 
void diff (I b1, I e1, I b2, I e2, O1 common, O2 o1, O3 o2, O4 d, EQ equal = std::equal_to<typename std::iterator_traits<I>::value_type> (), unsigned long max_lookahead = 100, unsigned long min_sync = 3)
{
  I i1 = b1;
  I i2 = b2;

  while (i1 != e1 && i2 != e2) {

    if (equal (*i1, *i2)) {

      *common = std::make_pair (*i1, *i2);
      ++common;
      ++i1;
      ++i2;

    } else {

      bool sync1 = false;
      unsigned long s1 = 1;
      for (I ii1 = i1; ++ii1 != e1 && s1 < max_lookahead; ++s1) {
        unsigned long s = min_sync;
        for (I j1 = ii1, j2 = i2; s > 0 && j1 != e1 && j2 != e2 && equal (*j1, *j2); ++j1, ++j2) {
          --s;
        }
        if (! s) {
          sync1 = true;
          break;
        }
      }

      bool sync2 = false;
      unsigned long s2 = 1;
      for (I ii2 = i2; ++ii2 != e2 && s2 < max_lookahead; ++s2) {
        unsigned long s = min_sync;
        for (I j1 = i1, j2 = ii2; s > 0 && j1 != e1 && j2 != e2 && equal (*j1, *j2); ++j1, ++j2) {
          --s;
        }
        if (! s) {
          sync2 = true;
          break;
        }
      }

      if (sync1 && (!sync2 || s1 < s2)) {
        while (s1 > 0) {
          *o1 = *i1;
          ++o1;
          ++i1;
          --s1;
        }
      } else if (sync2 && (!sync1 || s2 < s1)) {
        while (s2 > 0) {
          *o2 = *i2;
          ++o2;
          ++i2;
          --s2;
        }
      } else { 
        *d = std::make_pair (*i1, *i2);
        ++d;
        ++i1;
        ++i2;
      }

    }

  }

  while (i1 != e1) {
    *o1 = *i1;
    ++i1;
    ++o1;
  }
    
  while (i2 != e2) {
    *o2 = *i2;
    ++i2;
    ++o2;
  }
}

// --------------------------------------------------------------------------
//  StripedBar definition and implementation

StripedBar::StripedBar (QWidget *parent)
  : QFrame (parent), mp_tv (0)
{
  QObjectList cc = children ();
  for (QObjectList::const_iterator child = cc.begin (); child != cc.end (); ++child) {
    QWidget *ww = dynamic_cast <QWidget *> (*child);
    if (ww) {
      delete ww;
    }
  }

  //  ...
}

void
StripedBar::paintEvent (QPaintEvent *e)
{
  QFrame::paintEvent (e);

  if (mp_tv) {

    QPainter painter (this);

    QAbstractItemModel *model = mp_tv->model ();

    int fw = 0;
    int lw = lineWidth ();
    int y = lw;
    int h = 1;
    int htot = height () - 2 * lw;
    int wtot = width () - 2 * (lw + fw);

    int count = 0;
    QModelIndex mi (model->index (0, 1));
    while (mi.isValid ()) {
      ++count;
      mi = mp_tv->indexBelow (mi);
    }

    int ytop = -1;
    int ybottom = -1;
    int index = 0;
    QModelIndex col0 (model->index (0, 0));
    while (col0.isValid ()) {

      QModelIndex col1 (model->index (col0.row (), 1, model->parent (col0)));
      y = (index * htot) / count;
      h = ((index + 1) * htot) / count - y;
      if (h == 0) {
        h = 1;
      }

      QRect r (mp_tv->visualRect (col0));
      if (r.bottom () >= 0 && r.top () < height ()) {
        if (ytop < 0) {
          ytop = y;
        }
        ybottom = y + h;
      }

      if (model->data (col0, Qt::UserRole).toBool ()) {
        painter.fillRect (lw + fw, y, wtot / 2, h, left_diff_brush);
      }

      if (model->data (col1, Qt::UserRole).toBool ()) {
        painter.fillRect (lw + fw + wtot / 2, y, wtot / 2, h, right_diff_brush);
      }

      col0 = mp_tv->indexBelow (col0);

      ++index;

    }

    if (ytop >= 0) {
      painter.fillRect (lw, ytop, width () - 2 * lw, ybottom - ytop, QColor (128, 128, 128, 128));
    }

  }
}

void 
StripedBar::set_treeview (QTreeView *tv)
{
  mp_tv = tv;
  connect (mp_tv->verticalScrollBar () /* Qt4.2 only*/, SIGNAL (valueChanged (int)), this, SLOT (force_update (int)));
  connect (mp_tv, SIGNAL (expanded (const QModelIndex &)), this, SLOT (force_update (const QModelIndex &)));
  connect (mp_tv, SIGNAL (collapsed (const QModelIndex &)), this, SLOT (force_update (const QModelIndex &)));
}

void 
StripedBar::force_update (int)
{
  update ();
}

void
StripedBar::force_update (const QModelIndex &)
{
  update ();
}

// --------------------------------------------------------------------------
//  UiDialog implementation

static void diff_log_event_list (QTreeWidget *tv, QTreeWidgetItem *parent, const tl::Variant &dleft, const tl::Variant &dright);
static void diff_log_event_data (QTreeWidget *tv, QTreeWidgetItem *parent, const tl::Variant &data_left, const tl::Variant &data_right);
static void add_log_event_list (QTreeWidget *tv, int column, QTreeWidgetItem *parent, const tl::Variant &dlist);
static void add_log_event_data (QTreeWidget *tv, int column, QTreeWidgetItem *parent, const tl::Variant &data);

void 
expand_path (QTreeWidget *tv, QTreeWidgetItem *item)
{
  tv->expandItem (item);
  if (item->parent ()) {
    expand_path (tv, item->parent ());
  }
}

static std::string
log_event_to_text (const gtf::LogEventBase *e)
{
  std::string t = e->name ();
  /* too much:
  std::vector< std::pair<std::string, std::string> > attrs;
  e->attributes (attrs);
  for (std::vector< std::pair<std::string, std::string> >::const_iterator a = attrs.begin (); a != attrs.end (); ++a) {
    t += " ";
    t += a->first;
    t += ":";
    t += a->second;
  }
  */
  return t;
}

struct make_entry_both 
{
  make_entry_both (QTreeWidget *tv) : mp_tv (tv) { }

  void operator= (std::pair<const gtf::LogEventBase *, const gtf::LogEventBase *> e) const
  {
    QTreeWidgetItem *item = new QTreeWidgetItem ();
    item->setText (0, log_event_to_text (e.first).c_str ());
    item->setText (1, log_event_to_text (e.second).c_str ());
    add_log_event_data (mp_tv, -1, item, e.first->data ());
    item->setData (0, Qt::UserRole + 2, QVariant::fromValue ((void *)e.first));
    item->setData (1, Qt::UserRole + 2, QVariant::fromValue ((void *)e.second));
    mp_tv->addTopLevelItem (item);
  }

  make_entry_both &operator* () { return *this; }
  make_entry_both &operator++ (int) { return *this; }
  make_entry_both &operator++ () { return *this; }

private:
  QTreeWidget *mp_tv;
};

struct make_entry_left 
{
  make_entry_left (QTreeWidget *tv) : mp_tv (tv) { }

  void operator= (const gtf::LogEventBase *e) const
  {
    QTreeWidgetItem *item = new QTreeWidgetItem ();
    std::string t = log_event_to_text (e);
    item->setText (0, t.c_str ());
    add_log_event_data (mp_tv, 0, item, e->data ());
    item->setData (0, Qt::BackgroundRole, QVariant (left_diff_brush));
    item->setData (0, Qt::UserRole, QVariant (true));
    item->setData (0, Qt::UserRole + 2, QVariant::fromValue ((void *)e));
    mp_tv->addTopLevelItem (item);
    mp_tv->expandItem (item);
  }

  make_entry_left &operator* () { return *this; }
  make_entry_left &operator++ (int) { return *this; }
  make_entry_left &operator++ () { return *this; }

private:
  QTreeWidget *mp_tv;
};

struct make_entry_right 
{
  make_entry_right (QTreeWidget *tv) : mp_tv (tv) { }

  void operator= (const gtf::LogEventBase *e) const
  {
    QTreeWidgetItem *item = new QTreeWidgetItem ();
    std::string t = log_event_to_text (e);
    item->setText (1, t.c_str ());
    add_log_event_data (mp_tv, 1, item, e->data ());
    item->setData (1, Qt::BackgroundRole, QVariant (right_diff_brush));
    item->setData (1, Qt::UserRole, QVariant (true));
    item->setData (1, Qt::UserRole + 2, QVariant::fromValue ((void *)e));
    mp_tv->addTopLevelItem (item);
    mp_tv->expandItem (item);
  }

  make_entry_right &operator* () { return *this; }
  make_entry_right &operator++ (int) { return *this; }
  make_entry_right &operator++ () { return *this; }

private:
  QTreeWidget *mp_tv;
};

struct make_entry_diff 
{
  make_entry_diff (QTreeWidget *tv) : mp_tv (tv) { }

  void operator= (std::pair<const gtf::LogEventBase *, const gtf::LogEventBase *> e) const
  {
    QTreeWidgetItem *item = new QTreeWidgetItem ();
    std::string tleft = log_event_to_text (e.first);
    std::string tright = log_event_to_text (e.second);
    bool dep = e.first->equals (*e.second);
    item->setText (0, tleft.c_str ());
    item->setData (0, Qt::BackgroundRole, QVariant (dep ? left_diff_brush_dep : left_diff_brush));
    item->setData (0, Qt::UserRole, QVariant (true));
    item->setData (0, Qt::UserRole + 2, QVariant::fromValue ((void *)e.first));
    item->setText (1, tright.c_str ());
    item->setData (1, Qt::BackgroundRole, QVariant (dep ? right_diff_brush_dep : right_diff_brush));
    item->setData (1, Qt::UserRole, QVariant (true));
    item->setData (1, Qt::UserRole + 2, QVariant::fromValue ((void *)e.second));
    diff_log_event_data (mp_tv, item, e.first->data (), e.second->data ());
    mp_tv->addTopLevelItem (item);
    mp_tv->expandItem (item);
  }

  make_entry_diff &operator* () { return *this; }
  make_entry_diff &operator++ (int) { return *this; }
  make_entry_diff &operator++ () { return *this; }

private:
  QTreeWidget *mp_tv;
};

struct ptr_eq
{
  bool operator() (const gtf::LogEventBase *a, const gtf::LogEventBase *b) const
  {
    return *a == *b;
  }
};

static void
add_log_event (QTreeWidget *tv, int column, QTreeWidgetItem *parent, const tl::Variant &d, QTreeWidgetItem *p = 0)
{
  if (! p) {
    p = new QTreeWidgetItem (parent);
  }
  std::string t;
  if (d.is_list ()) {
    t = "block";
    add_log_event_list (tv, column, p, d);
  } else if (d.is_user () && d.user_type () == 1) {
    t = "img";
    if (column < 0) {
      p->setData (0, Qt::UserRole + 1, d.to_user<QImage> ());
      p->setData (1, Qt::UserRole + 1, d.to_user<QImage> ());
    } else {
      p->setData (column, Qt::UserRole + 1, d.to_user<QImage> ());
    }
  } else if (d.is_long ()) {
    t = "int ";
    t += d.to_string ();
  } else if (d.is_string ()) {
    t = "string \"";
    t += d.to_string ();
    t += "\"";
  }
  if (column < 0) {
    p->setText (0, t.c_str ());
    p->setText (1, t.c_str ());
  } else {
    p->setText (column, t.c_str ());
    if (column == 0) {
      p->setData (0, Qt::BackgroundRole, QVariant (left_diff_brush));
      p->setData (0, Qt::UserRole, QVariant (true));
      expand_path (tv, p);
    } else {
      p->setData (1, Qt::BackgroundRole, QVariant (right_diff_brush));
      p->setData (1, Qt::UserRole, QVariant (true));
      expand_path (tv, p);
    }
  }
}

static void
add_log_event_list (QTreeWidget *tv, int column, QTreeWidgetItem *parent, const tl::Variant &dlist)
{
  for (tl::Variant::iterator d = dlist.begin (); d != dlist.end (); ++d) {
    add_log_event (tv, column, parent, *d);
  }
}

static void
add_log_event_data (QTreeWidget *tv, int column, QTreeWidgetItem *parent, const tl::Variant &data) 
{
  if (! data.is_nil ()) {
    tl::Variant ddummy = tl::Variant::empty_list ();
    const tl::Variant *d = &data;
    if (! d->is_list ()) {
      ddummy.push (data);
      d = &ddummy;
    }
    add_log_event_list (tv, column, parent, *d);
  }
}

struct enter_data 
{
  enter_data (QTreeWidget *tv, QTreeWidgetItem *item, int column) : mp_tv (tv), mp_item (item), m_column (column) { }

  enter_data &operator= (const tl::Variant &data) { add_log_event (mp_tv, m_column, mp_item, data); return *this; }
  enter_data &operator= (const std::pair<tl::Variant, tl::Variant> &data) { return *this = data.first; }

  enter_data &operator* () { return *this; }
  enter_data &operator++ () { return *this; }
  enter_data &operator++ (int) { return *this; }

private:
  QTreeWidget *mp_tv;
  QTreeWidgetItem *mp_item;
  int m_column;
};

struct enter_data_diff 
{
  enter_data_diff (QTreeWidget *tv, QTreeWidgetItem *item) : mp_tv (tv), mp_item (item) { }

  enter_data_diff &operator= (const std::pair<tl::Variant, tl::Variant> &data) 
  { 
    QTreeWidgetItem *p = new QTreeWidgetItem (mp_item);
    if (data.first.is_list () && data.second.is_list ()) {
      p->setText (0, "block");
      p->setData (0, Qt::BackgroundRole, QVariant (left_diff_brush_dep));
      p->setData (0, Qt::UserRole, QVariant (true));
      p->setText (1, "block");
      p->setData (1, Qt::BackgroundRole, QVariant (right_diff_brush_dep));
      p->setData (1, Qt::UserRole, QVariant (true));
      diff_log_event_list (mp_tv, p, data.first, data.second);
    } else if (data.first.is_list ()) {
      p->setText (0, "block");
      p->setData (0, Qt::BackgroundRole, QVariant (left_diff_brush));
      p->setData (0, Qt::UserRole, QVariant (true));
      expand_path (mp_tv, p);
      add_log_event_list (mp_tv, 0, mp_item, data.first); 
      add_log_event (mp_tv, 1, mp_item, data.second, p); 
    } else if (data.second.is_list ()) {
      p->setText (1, "block");
      p->setData (1, Qt::BackgroundRole, QVariant (right_diff_brush));
      p->setData (1, Qt::UserRole, QVariant (true));
      expand_path (mp_tv, p);
      add_log_event_list (mp_tv, 1, mp_item, data.second); 
      add_log_event (mp_tv, 0, mp_item, data.first, p); 
    } else {
      add_log_event (mp_tv, 0, mp_item, data.first, p); 
      add_log_event (mp_tv, 1, mp_item, data.second, p); 
    }
    return *this;
  }

  enter_data_diff &operator* () { return *this; }
  enter_data_diff &operator++ () { return *this; }
  enter_data_diff &operator++ (int) { return *this; }

private:
  QTreeWidget *mp_tv;
  QTreeWidgetItem *mp_item;
};

static void
diff_log_event_list (QTreeWidget *tv, QTreeWidgetItem *parent, const tl::Variant &dleft, const tl::Variant &dright)
{
  enter_data both (tv, parent, -1);
  enter_data left (tv, parent, 0);
  enter_data right (tv, parent, 1);
  enter_data_diff delta (tv, parent);

  diff (dleft.begin (), dleft.end (), dright.begin (), dright.end (), both, left, right, delta, std::equal_to<tl::Variant> ());
}

static void
diff_log_event_data (QTreeWidget *tv, QTreeWidgetItem *parent, const tl::Variant &data_left, const tl::Variant &data_right) 
{
  tl::Variant ddummyleft = tl::Variant::empty_list ();
  const tl::Variant *dleft = &data_left;
  if (dleft->is_nil ()) {
    dleft = &ddummyleft;
  } else if (! dleft->is_list ()) {
    ddummyleft.push (data_left);
    dleft = &ddummyleft;
  }

  tl::Variant ddummyright = tl::Variant::empty_list ();
  const tl::Variant *dright = &data_right;
  if (dright->is_nil ()) {
    dright = &ddummyright;
  } else if (! dright->is_list ()) {
    ddummyright.push (data_right);
    dright = &ddummyright;
  }

  diff_log_event_list (tv, parent, *dleft, *dright);
}

UiDialog::UiDialog ()
{
  mp_ui = new Ui_GtfUiDialog ();
  mp_ui->setupUi (this);
  mp_ui->striped_bar->set_treeview (mp_ui->log_list);
  mp_ui->log_list->header ()->setResizeMode (QHeaderView::Stretch);
  mp_ui->au_event_list->header ()->setResizeMode (QHeaderView::ResizeToContents);
  mp_ui->curr_event_list->header ()->setResizeMode (QHeaderView::ResizeToContents);
  mp_ui->golden_img_frame->setWidget (mp_ui->golden_lbl);
  mp_ui->delta_img_frame->setWidget (mp_ui->delta_lbl);
  mp_ui->current_img_frame->setWidget (mp_ui->current_lbl);

  connect (mp_ui->actionExit, SIGNAL (triggered ()), qApp, SLOT (quit ()));
  connect (mp_ui->log_list, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (item_selected (QTreeWidgetItem *, QTreeWidgetItem *)));

  // ...

}

UiDialog::~UiDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
UiDialog::open_files (const std::string &fn_au, const std::string &fn_current)
{
  m_fn_au = fn_au;
  m_fn_current = fn_current;

  if (tl::verbosity () >= 10) {
    tl::info << "Reading golden file: " << fn_au;
  }
  m_au_events.load (fn_au, true /*no spontaneous*/);

  if (tl::verbosity () >= 10) {
    tl::info << "Reading current file: " << fn_current;
  }
  m_current_events.load (fn_current, true /*no spontaneous*/);

  make_entry_both b (mp_ui->log_list);
  make_entry_left l (mp_ui->log_list);
  make_entry_right r (mp_ui->log_list);
  make_entry_diff d (mp_ui->log_list);
  diff (m_au_events.begin (), m_au_events.end (), m_current_events.begin (), m_current_events.end (), b, l, r, d, ptr_eq ());
}

void 
UiDialog::item_selected (QTreeWidgetItem *current, QTreeWidgetItem *)
{
  if (current->data (0, Qt::UserRole + 1) != QVariant () || 
      current->data (1, Qt::UserRole + 1) != QVariant ()) {

    mp_ui->compare_stck->setCurrentIndex (1);

    QImage img_left;
    if (current->data (0, Qt::UserRole + 1).type () == QVariant::Image) {
      img_left = current->data (0, Qt::UserRole + 1).value<QImage> ();
    }
    QImage img_right;
    if (current->data (1, Qt::UserRole + 1).type () == QVariant::Image) {
      img_right = current->data (1, Qt::UserRole + 1).value<QImage> ();
    }

    if (! img_left.isNull ()) {
      QPixmap pixmap;
      pixmap = img_left; // Qt 4.6.0 workaround
      mp_ui->golden_lbl->setPixmap (pixmap);
      mp_ui->golden_lbl->resize (img_left.size ());
    } else {
      mp_ui->golden_lbl->setPixmap (QPixmap ());
      mp_ui->golden_lbl->setText ("");
    }
    
    if (! img_right.isNull ()) {
      QPixmap pixmap;
      pixmap = img_right; // Qt 4.6.0 workaround
      mp_ui->current_lbl->setPixmap (pixmap);
      mp_ui->current_lbl->resize (img_right.size ());
    } else {
      mp_ui->current_lbl->setPixmap (QPixmap ());
      mp_ui->current_lbl->setText ("");
    }

    if (! img_left.isNull () && ! img_right.isNull ()) {

      int w = std::min (img_left.width (), img_right.width ());
      int h = std::min (img_left.height (), img_right.height ());
      QImage delta (w, h, QImage::Format_RGB32);
      
      for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
          QRgb p1 = img_left.pixel (x, y);
          QRgb p2 = img_right.pixel (x, y);
          QRgb px = p1 ^ p2;
          delta.setPixel (x, y, px);
        }
      }

      QPixmap pixmap;
      pixmap = delta; // Qt 4.6.0 workaround
      mp_ui->delta_lbl->setPixmap (pixmap);
      mp_ui->delta_lbl->resize (delta.size ());

    } else {
      mp_ui->delta_lbl->setPixmap (QPixmap ());
      mp_ui->delta_lbl->setText ("");
    }

  } else if (current->data (0, Qt::UserRole + 2) != QVariant () || 
             current->data (1, Qt::UserRole + 2) != QVariant ()) {

    mp_ui->compare_stck->setCurrentIndex (2);

    std::vector< std::pair<std::string, std::string> > attrs_left, attrs_right;

    mp_ui->au_event_list->clear ();
    mp_ui->curr_event_list->clear ();

    const LogEventBase *eleft = 0;
    const LogEventBase *eright = 0;

    if (current->data (0, Qt::UserRole + 2) != QVariant ()) {
      eleft = (const LogEventBase *) current->data (0, Qt::UserRole + 2).value<void *> ();
      eleft->attributes (attrs_left);
    }

    if (current->data (1, Qt::UserRole + 2) != QVariant ()) {
      eright = (const LogEventBase *) current->data (1, Qt::UserRole + 2).value<void *> ();
      eright->attributes (attrs_right);
    }

    bool same_type = false;
    if (eleft != 0 && eright != 0 && std::string (eleft->name ()) == eright->name ()) {
      same_type = true;
    }

    std::vector< std::pair<std::string, std::string> >::const_iterator l = attrs_left.begin ();
    std::vector< std::pair<std::string, std::string> >::const_iterator r = attrs_right.begin ();

    while (l != attrs_left.end () || r != attrs_right.end ()) {
      if (l != attrs_left.end ()) {
        QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->au_event_list);
        item->setText (0, l->first.c_str ());
        item->setText (1, l->second.c_str ());
        if (! same_type || (r != attrs_right.end () && l->second != r->second)) {
          item->setData (1, Qt::BackgroundRole, QVariant (left_diff_brush));
        }
      }
      if (r != attrs_right.end ()) {
        QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->curr_event_list);
        item->setText (0, r->first.c_str ());
        item->setText (1, r->second.c_str ());
        if (! same_type || (l != attrs_left.end () && l->second != r->second)) {
          item->setData (1, Qt::BackgroundRole, QVariant (right_diff_brush));
        }
      }
      if (r != attrs_right.end ()) {
        ++r;
      }
      if (l != attrs_left.end ()) {
        ++l;
      }
    }

    QTreeWidgetItem *item;

    if (eleft) {
      item = new QTreeWidgetItem (mp_ui->au_event_list);
      item->setText (0, "XML line");
      item->setText (1, tl::to_string (eleft->xml_line ()).c_str ());
    }
    if (eright) {
      item = new QTreeWidgetItem (mp_ui->curr_event_list);
      item->setText (0, "XML line");
      item->setText (1, tl::to_string (eright->xml_line ()).c_str ());
    }

  } else {
    mp_ui->compare_stck->setCurrentIndex (0);
  }

}

}


