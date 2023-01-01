
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#ifndef HDR_tlDeferredExecutionQt
#define HDR_tlDeferredExecutionQt

#include "tlCommon.h"
#include "tlDeferredExecution.h"

#include <QObject>
#include <QTimer>
#include <QMutex>

#include <list>

namespace tl
{

/**
 *  @brief The deferred method scheduler
 */
class TL_PUBLIC DeferredMethodSchedulerQt
  : public QObject, public DeferredMethodScheduler
{
Q_OBJECT
public:
  /**
   *  @brief Constructor
   */
  DeferredMethodSchedulerQt ();

  /**
   *  @brief Destructor
   */
  ~DeferredMethodSchedulerQt ();

protected:
  /**
   *  @brief Reimplementation of the interface: queue an event
   *  In effect, the event should later trigger a call to do_execute ().
   */
  void queue_event ();

private slots:
  void timer ();

private:
  QTimer m_timer, m_fallback_timer;
  int m_event_type;

  virtual bool event (QEvent *event);
};

}

#endif

