
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

#include "gsiQt.h"
#include "tlObject.h"

#include <QMetaType>

namespace qt_gsi
{

/**
 *  @brief A tiny object being both a gsi::ObjectBase for lifetime monitoring and tl::Object for shared pointer management
 *
 *  See QtWatcher for details.
 */
class QtLifetimeMonitor
  : public tl::Object, public gsi::ObjectBase
{
  //  .. nothing yet ..
};

/**
 *  @brief A helper object that is attached to a QObject to monitor its lifetime
 *
 *  This object will be put into the properties table of the QObject (more precisely: a copy).
 *  When the QObject is destroyed, its properties are destroyed too and through reference
 *  counting the destruction of the QObject is detected. The monitoring itself is implemented
 *  through a gsi::ObjectBase object which plugs seamlessly into the gsi type system:
 *
 *     QObject -> QtWatcher (+ temp copies) -> gsi::ObjectBase -> plugs into script objects
 *
 *  NOTE: the initial idea was to use child objects attached to QObject. However, these
 *  interfere too much with Qt's internals - for example, attaching a child object emits
 *  a QChildEvent. That gets filtered in some eventFilter function which itself may cause
 *  QObjects to be decorated with helper objects ... resulting in a infinte recursion.
 *
 *  Decoration by properties is a leaner solution.
 */
class QtWatcher
{
public:
  QtWatcher ()
  {
    //  .. nothing yet ..
  }

  QtWatcher (QtLifetimeMonitor *monitor)
    : mp_monitor (monitor)
  {
    //  .. nothing yet ..
  }

  gsi::ObjectBase *gsi_object ()
  {
    if (mp_monitor) {
      return static_cast<gsi::ObjectBase *> (mp_monitor.get ());
    } else {
      return 0;
    }
  }

private:
  QtWatcher (size_t id, gsi::ObjectBase *obj);

  tl::shared_ptr<QtLifetimeMonitor> mp_monitor;
};

}

Q_DECLARE_METATYPE (qt_gsi::QtWatcher)

namespace qt_gsi
{

/**
 *  @brief Attaches a watcher object to a native QObject
 */
gsi::ObjectBase *get_watcher_object (QObject *qobject, bool required)
{
  const char *watcher_prop_name = "_gsi_qt::watcher";

  QVariant prop = qobject->property (watcher_prop_name);
  if (prop.isValid ()) {

    return prop.value<QtWatcher> ().gsi_object ();

  } else if (required) {

    QtWatcher w (new QtLifetimeMonitor ());
    qobject->setProperty (watcher_prop_name, QVariant::fromValue<QtWatcher> (w));
    return w.gsi_object ();

  } else {

    return 0;

  }
}

}

