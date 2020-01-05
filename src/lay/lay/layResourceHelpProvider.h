
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_layResourceHelpProvider
#define HDR_layResourceHelpProvider

#include "layHelpProvider.h"

#include <vector>

namespace lay
{

/**
 *  @brief Implements a help provider for the generated documentation
 */
class ResourceHelpProvider
  : public HelpProvider
{
public:
  ResourceHelpProvider (const char *folder, const std::string &title);

  std::string folder () const
  {
    return m_folder;
  }

  std::string title () const
  {
    return m_title;
  }

  virtual QDomDocument get (const std::string &path) const;

private:
  std::string m_folder, m_title;
};

}

#endif

