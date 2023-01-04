
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


#ifndef _HDR_tlEvents
#define _HDR_tlEvents

#include "tlObject.h"
#include "tlException.h"

#include <vector>
#include <algorithm>

namespace tl
{

/**
 *  @brief A helper function to handle event exceptions of tl::Exception type
 */
TL_PUBLIC void handle_event_exception (tl::Exception &ex);

/**
 *  @brief A helper function to handle event exceptions of std::exception type
 */
TL_PUBLIC void handle_event_exception (std::exception &ex);

/**
 *  @brief A framework of observer and observables
 *
 *  This framework competes with the signal/event concept of Qt. It is provided because
 *  the latter has some weaknesses, namely:
 *
 *   - Signals/slots cannot be a defined for templates
 *   - The involved objects need to be derived from QObject
 *   - The Qt concept requires the meta object compiler
 *
 *  the framework provided herein is based on C++ meta programming alone and avoids
 *  some of these issues.
 *
 *  Events are used the following way:
 *   - Instantiate an event object inside the observable class
 *   - Expose this event object
 *   - Use add/remove to attach or detach receivers
 *
 *  @code
 *
 *  //  Object with event
 *  class Observed
 *  {
 *  public:
 *    tl::event<Observed *, int> &event () { return m_event; }
 *    void trigger_event ()
 *    {
 *      //  issue event
 *      m_event (this, -1);
 *    }
 *  private:
 *    tl::event<Observed *, int> m_event;
 *  };
 *
 *  //  Observer
 *  class Observer : public tl::Object
 *  {
 *  public:
 *    void receives_event (Observed *, int) { ... }
 *  };
 *
 *  //  Connect this
 *  Observed x;
 *  Observer y;
 *  x.event ().add (&y, &Observed::receives_event);
 *  x.trigger_event ();
 *
 *  @endcode
 *
 *  Events can be given a disambiguation or client data parameter:
 *
 *  @code
 *
 *  //  Observer
 *  class Observer : public tl::Object
 *  {
 *  public:
 *    //  The first parameter is the client data parameter which is given as
 *    //  third parameter in "add"
 *    void receives_event (int, Observed *, int) { ... }
 *  };
 *
 *  //  Connect this
 *  Observed x;
 *  Observer y;
 *  x.event ().add (&y, &Observed::receives_event, 1);
 *  x.trigger_event ();
 *
 *  @endcode
 */

template <class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE event_function_base;
template <class T, class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE event_function;
template <class T, class D, class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE event_function_with_data;
template <class T, class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE generic_event_function;
template <class T, class D, class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE generic_event_function_with_data;
template <class A1 = void, class A2 = void, class A3 = void, class A4 = void, class A5 = void> class TL_PUBLIC_TEMPLATE event;
typedef event<> Event;

#define  _COUNT 0
#define _TMPLARGS
#define _TMPLARGLIST
#define _TMPLARGLISTP void, void, void, void, void
#define _CALLARGS
#define _CALLARGLIST
#define _CALLARGPTRS  0

#include "tlEventsVar.h"

#undef _VOIDLIST
#undef _CALLARGPTRS
#undef _CALLARGLIST
#undef _CALLARGS
#undef _TMPLARGLISTP
#undef _TMPLARGLIST
#undef _TMPLARGS
#undef _COUNT

#define  _COUNT 1
#define _TMPLARGS     class A1
#define _TMPLARGLIST  A1
#define _TMPLARGLISTP A1, void, void, void, void
#define _CALLARGS     a1
#define _CALLARGLIST  A1 a1
#define _CALLARGPTRS  (void *)&a1

#include "tlEventsVar.h"

#undef _VOIDLIST
#undef _CALLARGPTRS
#undef _CALLARGLIST
#undef _CALLARGS
#undef _TMPLARGLISTP
#undef _TMPLARGLIST
#undef _TMPLARGS
#undef _COUNT

#define  _COUNT 2
#define _TMPLARGS     class A1, class A2
#define _TMPLARGLIST  A1, A2
#define _TMPLARGLISTP A1, A2, void, void, void
#define _CALLARGS     a1, a2
#define _CALLARGLIST  A1 a1, A2 a2
#define _VOIDLIST     void, void, void
#define _CALLARGPTRS  (void *)&a1, (void *)&a2

#include "tlEventsVar.h"

#undef _VOIDLIST
#undef _CALLARGPTRS
#undef _CALLARGLIST
#undef _CALLARGS
#undef _TMPLARGLISTP
#undef _TMPLARGLIST
#undef _TMPLARGS
#undef _COUNT

#define  _COUNT 3
#define _TMPLARGS     class A1, class A2, class A3
#define _TMPLARGLIST  A1, A2, A3
#define _TMPLARGLISTP A1, A2, A3, void, void
#define _CALLARGS     a1, a2, a3
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3
#define _VOIDLIST     void, void
#define _CALLARGPTRS  (void *)&a1, (void *)&a2, (void *)&a3

#include "tlEventsVar.h"

#undef _VOIDLIST
#undef _CALLARGPTRS
#undef _CALLARGLIST
#undef _CALLARGS
#undef _TMPLARGLISTP
#undef _TMPLARGLIST
#undef _TMPLARGS
#undef _COUNT

#define  _COUNT 4
#define _TMPLARGS     class A1, class A2, class A3, class A4
#define _TMPLARGLIST  A1, A2, A3, A4
#define _TMPLARGLISTP A1, A2, A3, A4, void
#define _CALLARGS     a1, a2, a3, a4
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4
#define _VOIDLIST     void
#define _CALLARGPTRS  (void *)&a1, (void *)&a2, (void *)&a3, (void *)&a4

#include "tlEventsVar.h"

#undef _VOIDLIST
#undef _CALLARGPTRS
#undef _CALLARGLIST
#undef _CALLARGS
#undef _TMPLARGLISTP
#undef _TMPLARGLIST
#undef _TMPLARGS
#undef _COUNT

}

#endif
