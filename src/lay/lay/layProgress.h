
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


#ifndef HDR_layProgress
#define HDR_layProgress

#include "layCommon.h"

#include <string>
#include <list>
#include <map>
#include <set>

#include "tlProgress.h"
#include "tlObject.h"
#include "tlTimer.h"

namespace lay
{

class ProgressReporter;

/**
 *  @brief This interface provides the actual implementation of the progress bar
 */
class LAY_PUBLIC ProgressBar
  : public tl::Object
{
public:
  virtual ~ProgressBar () { }

  virtual void update_progress (tl::Progress *progress) = 0;
  virtual bool progress_wants_widget () const { return false; }
  virtual void progress_add_widget (QWidget * /*widget*/) { }
  virtual void progress_remove_widget () { }
  virtual QWidget *progress_get_widget () const { return 0; }
  virtual void show_progress_bar (bool show) = 0;
};

class LAY_PUBLIC ProgressReporter 
  : public QObject, public tl::ProgressAdaptor
{
public:
  ProgressReporter ();
  virtual ~ProgressReporter ();

  virtual void register_object (tl::Progress *progress);
  virtual void unregister_object (tl::Progress *progress);
  virtual void trigger (tl::Progress *progress);
  virtual void yield (tl::Progress *progress);
  virtual bool eventFilter (QObject *dest, QEvent *event);

  void set_progress_bar (lay::ProgressBar *pb);

private:
  lay::ProgressBar *mp_pb;
  bool m_pw_visible;
  std::map<tl::Progress *, tl::Clock> m_queued;
  std::set<tl::Progress *> m_active;

  void process_events ();
  void update_and_yield ();
  void set_visible (bool vis);
};

/**
 *  @brief Marks a widget as alive
 *  "alive" widgets receive input events also while a progress reporter is shown
 */
LAY_PUBLIC void mark_widget_alive (QWidget *w, bool alive);

}

#endif

