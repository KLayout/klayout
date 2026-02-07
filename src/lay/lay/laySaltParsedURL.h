
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

#ifndef HDR_laySaltParsedURL
#define HDR_laySaltParsedURL

#include "layCommon.h"
#include "laySaltGrain.h"

namespace lay
{

/**
 *  @brief An enum describing the protocol to use for download
 */
enum Protocol {
  DefaultProtocol = 0,
  WebDAV = 1,
  Git = 2
};

/**
 *  @brief A class representing a SaltGrain URL
 *
 *  The URL is parsed into protocol, branch, URL and subfolder if applicable.
 *  Some heuristics is applied to decompose parts.
 *
 *  SVN URLs:
 *    https://server.com/repo/trunk                               -> protocol=DefaultProtocol, url="https://server.com/repo/trunk", branch="", subfolder=""
 *    svn+https://server.com/repo/trunk                           -> protocol=WebDAV, url="https://server.com/repo/trunk", branch="", subfolder=""
 *
 *  Git URL heuristics:
 *    git+https://server.com/repo.git                             -> protocol=Git, url="https://server.com/repo.git", branch="", subfolder=""
 *    git+https://server.com/repo.git/sub/folder                  -> protocol=Git, url="https://server.com/repo.git", branch="", subfolder="sub/folder"
 *    git+https://server.com/repo+sub/folder                      -> protocol=Git, url="https://server.com/repo", branch="", subfolder="sub/folder"
 *    git+https://server.com/repo.git[v1.0]                       -> protocol=Git, url="https://server.com/repo.git", branch="v1.0", subfolder=""
 *    git+https://server.com/repo.git/sub/folder[refs/tags/1.0]   -> protocol=Git, url="https://server.com/repo.git", branch="refs/tags/1.0", subfolder="sub/folder"
 *    git+https://server.com/repo.git/trunk                       -> protocol=Git, url="https://server.com/repo.git", branch="HEAD", subfolder=""
 *    git+https://server.com/repo.git/trunk/sub/folder            -> protocol=Git, url="https://server.com/repo.git", branch="HEAD", subfolder="sub/folder"
 *    git+https://server.com/repo.git/branches/release            -> protocol=Git, url="https://server.com/repo.git", branch="refs/heads/release", subfolder=""
 *    git+https://server.com/repo.git/tags/1.9                    -> protocol=Git, url="https://server.com/repo.git", branch="refs/tags/1.9", subfolder=""
 *    git+https://server.com/repo.git/tags/1.9/sub/folder         -> protocol=Git, url="https://server.com/repo.git", branch="refs/tags/1.9", subfolder="sub/folder"
 */

class LAY_PUBLIC SaltParsedURL
{
public:
  /**
   *  @brief Constructor: creates an URL from the given generic URL string
   *
   *  This will decompose the URL into the parts and fill protocol, branch and subfolder fields.
   */
  SaltParsedURL (const std::string &url);

  /**
   *  @brief Gets the basic URL
   */
  const std::string &url () const
  {
    return m_url;
  }

  /**
   *  @brief Gets the subfolder string
   */
  const std::string &subfolder () const
  {
    return m_subfolder;
  }

  /**
   *  @brief Gets the branch string
   */
  const std::string &branch () const
  {
    return m_branch;
  }

  /**
   *  @brief Gets the protocol
   */
  lay::Protocol protocol () const
  {
    return m_protocol;
  }

private:
  std::string m_url;
  std::string m_branch;
  std::string m_subfolder;
  lay::Protocol m_protocol;
};

}

#endif
