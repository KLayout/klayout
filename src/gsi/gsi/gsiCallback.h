
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


#ifndef _HDR_gsiCallback
#define _HDR_gsiCallback

#include "gsiSerialisation.h"

namespace gsi
{

/**
 *  @brief Call target (the scripting client's implementation) base class
 *
 *  This class is specialized to implement the actual call process later.
 */
struct GSI_PUBLIC Callee
  : public tl::Object
{
  Callee () { }
  virtual ~Callee () { }

  virtual void call (int id, SerialArgs &args, SerialArgs &ret) const = 0;
  virtual bool can_call () const { return true; }
};

/**
 *  @brief Callback connector object
 *
 *  This object holds information about the actual implementation of the callback 
 *  on the scripting client's side.
 */
struct Callback
{
  Callback () 
    : id (-1), callee (0), argsize (0), retsize (0)
  { 
    //  .. nothing yet ..
  }

  Callback (int i, Callee *c, unsigned int as, unsigned int rs) 
    : id (i), callee (c), argsize (as), retsize (rs)
  { 
    //  .. nothing yet ..
  }

  void call_int (SerialArgs &args, SerialArgs &ret) const
  {
    if (callee) {
      callee->call (id, args, ret);
    }
  }

  bool can_issue () const
  {
    return callee && callee->can_call ();
  } 

// 0 arguments

#define _TMPLARGPART  
#define _FUNCARGLIST  
#define _COMMA
#define _CALLARGLIST  
#define _CALLARGS  
#define _SETVALUE     

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGLIST
#undef _CALLARGS
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 1 argument

#define _TMPLARGPART  class A1
#define _FUNCARGLIST  A1
#define _COMMA        ,
#define _CALLARGLIST  A1 a1
#define _CALLARGS     a1
#define _SETVALUE     args.write<A1> (a1); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 2 arguments

#define _TMPLARGPART  class A1, class A2
#define _FUNCARGLIST  A1, A2
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2
#define _CALLARGS     a1, a2
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 3 arguments

#define _TMPLARGPART  class A1, class A2, class A3
#define _FUNCARGLIST  A1, A2, A3
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3
#define _CALLARGS     a1, a2, a3
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 4 arguments

#define _TMPLARGPART  class A1, class A2, class A3, class A4
#define _FUNCARGLIST  A1, A2, A3, A4
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4
#define _CALLARGS     a1, a2, a3, a4
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \
                      args.write<A4> (a4); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 5 arguments

#define _TMPLARGPART  class A1, class A2, class A3, class A4, class A5
#define _FUNCARGLIST  A1, A2, A3, A4, A5
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5
#define _CALLARGS     a1, a2, a3, a4, a5
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \
                      args.write<A4> (a4); \
                      args.write<A5> (a5); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 6 arguments

#define _TMPLARGPART  class A1, class A2, class A3, class A4, class A5, class A6
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6
#define _CALLARGS     a1, a2, a3, a4, a5, a6
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \
                      args.write<A4> (a4); \
                      args.write<A5> (a5); \
                      args.write<A6> (a6); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 7 arguments

#define _TMPLARGPART  class A1, class A2, class A3, class A4, class A5, class A6, class A7
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7
#define _CALLARGS     a1, a2, a3, a4, a5, a6, a7
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \
                      args.write<A4> (a4); \
                      args.write<A5> (a5); \
                      args.write<A6> (a6); \
                      args.write<A7> (a7); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

// 8 arguments

#define _TMPLARGPART  class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8
#define _COMMA        ,
#define _CALLARGLIST  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8
#define _CALLARGS     a1, a2, a3, a4, a5, a6, a7, a8
#define _SETVALUE     args.write<A1> (a1); \
                      args.write<A2> (a2); \
                      args.write<A3> (a3); \
                      args.write<A4> (a4); \
                      args.write<A5> (a5); \
                      args.write<A6> (a6); \
                      args.write<A7> (a7); \
                      args.write<A8> (a8); \

#include "gsiCallbackVar.h"

#undef _SETVALUE
#undef _COMMA
#undef _CALLARGS
#undef _CALLARGLIST
#undef _FUNCARGLIST
#undef _TMPLARGPART

  int id;
  tl::weak_ptr<Callee> callee;
  unsigned int argsize;
  unsigned int retsize;
};

}

#endif

