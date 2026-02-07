
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_tlHash
#define HDR_tlHash

#include <unordered_map>
#include <unordered_set>

#include <set>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <functional>
#include <stdint.h>

//  for std::hash of QString and QByteArray
#if defined(HAVE_QT)
# include <QString>
# include <QByteArray>
#endif

#include "tlSList.h"

/**
 *  This header defines some generic hash functions
 *  for use with std::unordered_map and std::unordered_set
 */

namespace tl
{
  inline size_t hcombine (size_t h1, size_t h2)
  {
    return (h1 << 4) ^ (h1 >> 4) ^ h2;
  }

  template <class T>
  inline size_t hfunc (const T &t)
  {
    std::hash <T> hf;
    return hf (t);
  }

  template <class T>
  inline size_t hfunc (const T &t, size_t h)
  {
    std::hash <T> hf;
    return hcombine (h, hf (t));
  }

  template <class T>
  size_t hfunc_iterable (const T &o, size_t h)
  {
    for (auto i = o.begin (); i != o.end (); ++i) {
      h = hfunc (*i, h);
    }
    return h;
  }

  /**
   *  @brief Generic hash for a pair of objects
   */

  template <class T1, class T2>
  size_t hfunc (const std::pair <T1, T2> &o, size_t h)
  {
    return hfunc (o.first, hfunc (o.second, h));
  }

  template <class T1, class T2>
  size_t hfunc (const std::pair <T1, T2> &o)
  {
    return hfunc (o.first, hfunc (o.second));
  }

  /**
   *  @brief Generic hash for an unordered set
   */

  template <class T>
  size_t hfunc (const std::unordered_set <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const std::unordered_set <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for a vector
   */

  template <class T>
  size_t hfunc (const std::vector <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const std::vector <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for a list
   */

  template <class T>
  size_t hfunc (const std::list <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const std::list <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for a tl::slist
   */

  template <class T>
  size_t hfunc (const tl::slist <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const tl::slist <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for an ordered set
   */

  template <class T>
  size_t hfunc (const std::set <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const std::set <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for std::multiset
   */

  template <class T>
  size_t hfunc (const std::multiset <T> &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <class T>
  size_t hfunc (const std::multiset <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for an unordered map
   */

  template <class T1, class T2>
  size_t hfunc (const std::unordered_map<T1, T2> &o, size_t h)
  {
    for (typename std::unordered_map<T1, T2>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (i->first, hfunc (i->second, h));
    }
    return h;
  }

  template <class T1, class T2>
  size_t hfunc (const std::unordered_map<T1, T2> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for an ordered map
   */

  template <class T1, class T2>
  size_t hfunc (const std::map<T1, T2> &o, size_t h)
  {
    for (typename std::map<T1, T2>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (i->first, hfunc (i->second, h));
    }
    return h;
  }

  template <class T1, class T2>
  size_t hfunc (const std::map<T1, T2> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for std::multimap
   */

  template <class T1, class T2>
  size_t hfunc (const std::multimap<T1, T2> &o, size_t h)
  {
    for (typename std::multimap<T1, T2>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (i->first, hfunc (i->second, h));
    }
    return h;
  }

  template <class T1, class T2>
  size_t hfunc (const std::multimap<T1, T2> &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Create a pointer hash from the pointer's value
   */
  template <class X>
  struct ptr_hash_from_value
  {
    size_t operator() (const X *ptr) const
    {
      return ptr ? std::hash<X> () (*ptr) : 0;
    }
  };

#if defined(HAVE_QT)

  /**
   *  @brief Generic hash for QChar
   */

  template <>
  inline size_t hfunc (const QChar &o, size_t h)
  {
    return hcombine (h, size_t (o.unicode ()));
  }

  template <>
  inline size_t hfunc (const QChar &o)
  {
    return size_t (o.unicode ());
  }

  /**
   *  @brief Generic hash for QString
   */

  template <>
  inline size_t hfunc (const QString &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <>
  inline size_t hfunc (const QString &o)
  {
    return hfunc (o, size_t (0));
  }

  /**
   *  @brief Generic hash for QByteArray
   */

  template <>
  inline size_t hfunc (const QByteArray &o, size_t h)
  {
    return hfunc_iterable (o, h);
  }

  template <>
  inline size_t hfunc (const QByteArray &o)
  {
    return hfunc (o, size_t (0));
  }

#endif

#if defined(HAVE_64BIT_COORD)

  inline size_t hfunc (__int128 v)
  {
    return hcombine (hfunc (uint64_t (v)), hfunc (uint64_t (v >> 64)));
  }

  inline size_t hfunc (__int128 v, size_t h)
  {
    return hcombine (hfunc (v), h);
  }

#endif

}

//  provide some missing std::hash implementations based on our tl::hfunc

namespace std
{

  template <class T1, class T2>
  struct hash <std::pair <T1, T2> >
  {
    size_t operator() (const std::pair<T1, T2> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <std::unordered_set <T> >
  {
    size_t operator() (const std::unordered_set<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <std::vector <T> >
  {
    size_t operator() (const std::vector<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <std::list <T> >
  {
    size_t operator() (const std::list<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <tl::slist <T> >
  {
    size_t operator() (const tl::slist<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <std::set <T> >
  {
    size_t operator() (const std::set<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T>
  struct hash <std::multiset <T> >
  {
    size_t operator() (const std::multiset<T> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T1, class T2>
  struct hash <std::unordered_map<T1, T2> >
  {
    size_t operator() (const std::unordered_map<T1, T2> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T1, class T2>
  struct hash <std::map<T1, T2> >
  {
    size_t operator() (const std::map<T1, T2> &o) const
    {
      return tl::hfunc (o);
    }
  };

  template <class T1, class T2>
  struct hash <std::multimap<T1, T2> >
  {
    size_t operator() (const std::multimap<T1, T2> &o) const
    {
      return tl::hfunc (o);
    }
  };

}

#endif
