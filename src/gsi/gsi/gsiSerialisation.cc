
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


#include "gsi.h"
#include "gsiSerialisation.h"

namespace gsi
{

class AdaptorSynchronizer
{
public:
  AdaptorSynchronizer (AdaptorBase *src, AdaptorBase *target, tl::Heap *heap)
    : mp_src (src), mp_target (target), mp_heap (heap)
  {
    //  .. nothing yet ..
  }

  ~AdaptorSynchronizer ()
  {
    mp_src->copy_to (mp_target, *mp_heap);
    delete mp_src;
    delete mp_target;
    mp_src = 0;
  }

private:
  AdaptorBase *mp_src, *mp_target;
  tl::Heap *mp_heap;
};

AdaptorBase::AdaptorBase ()
{
  //  .. nothing yet ..
}

AdaptorBase::~AdaptorBase ()
{
  //  .. nothing yet ..
}

void AdaptorBase::tie_copies (AdaptorBase *target, tl::Heap &heap) 
{
  std::unique_ptr<AdaptorBase> t (target);
  copy_to (target, heap);

  //  This object (which will be destroyed before this is responsible for copying back 
  //  the contents of target into this once the heap goes out of scope)
  heap.push (new AdaptorSynchronizer (t.release (), this, &heap));
}

}

