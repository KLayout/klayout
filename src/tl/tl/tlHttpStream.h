
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#ifndef HDR_tlHttpStream
#define HDR_tlHttpStream

#include "tlHttpStreamCurl.h"
#include "tlHttpStreamQt.h"

#include "tlObject.h"

namespace tl
{

/**
 *  @brief A callback interface to provide the authentication data
 */
class TL_PUBLIC HttpCredentialProvider
  : public tl::Object
{
public:
  HttpCredentialProvider () { }
  virtual ~HttpCredentialProvider () { }

  /**
   *  @brief Gets the user name and password for the given URL and authentication realm
   */
  virtual bool user_password (const std::string &url, const std::string &realm, bool proxy, int attempt, std::string &user, std::string &passwd) = 0;
};

/**
 *  @brief An exception class for HTTP errors
 */
class TL_PUBLIC HttpErrorException
  : public tl::Exception
{
public:
  HttpErrorException (const std::string &f, int en, const std::string &url)
    : tl::Exception (tl::to_string (QObject::tr ("Error %d: %s, fetching %s")), en, f, url)
  { }
};

}

#endif

