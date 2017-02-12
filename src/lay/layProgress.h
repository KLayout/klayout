
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

  virtual void set_progress_can_cancel (bool f) = 0;
  virtual void set_progress_text (const std::string &text) = 0;
  virtual void set_progress_value (double v, const std::string &value) = 0;
  virtual void show_progress_bar (bool show) = 0;
};

class LAY_PUBLIC ProgressReporter 
  : public tl::ProgressAdaptor
{
public:
  ProgressReporter ();
  virtual ~ProgressReporter ();

  virtual void register_object (tl::Progress *progress);
  virtual void unregister_object (tl::Progress *progress);
  virtual void trigger (tl::Progress *progress);
  virtual void yield (tl::Progress *progress);

  void signal_break ();
  void set_progress_bar (lay::ProgressBar *pb);

private:
  std::list <tl::Progress *> mp_objects;
  tl::Clock m_start_time;
  lay::ProgressBar *mp_pb;
  bool m_pw_visible;

  void process_events ();
  void update_and_yield ();
};

}

#endif

