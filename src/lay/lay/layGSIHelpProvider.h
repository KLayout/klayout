
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


#ifndef HDR_layGSIHelpProvider
#define HDR_layGSIHelpProvider

#include "layHelpProvider.h"

#include <vector>

namespace lay
{

/**
 *  @brief Implements a help provider for the generated documentation
 */
class GSIHelpProvider
  : public HelpProvider
{
public:
  GSIHelpProvider ();

  virtual std::string folder (lay::HelpSource *src) const;
  virtual std::string title (lay::HelpSource *src) const;
  virtual void toc (lay::HelpSource *src, std::vector<std::string> &t);
  virtual QDomDocument get (lay::HelpSource *src, const std::string &u) const;

private:
  std::string produce_class_doc (const std::string &cls) const;
  std::string produce_class_index (HelpSource *src, const char *module_name) const;
};

}

#endif

