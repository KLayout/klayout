
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_layHelpProvider
#define HDR_layHelpProvider

#include <QDomDocument>
#include <string>

namespace lay
{

class HelpSource;

/**
 *  @brief A provider for documentation in the help system
 *
 *  A help provider is responsible for providing documention of a certain 
 *  category. That can be written documentation or generated documentation.
 *  Each help provider provides documents under a certain folder, i.e.
 *  "doc/..".
 *  It must be able to deliver a keyword list for the search system and 
 *  a DOM model for a given URL below that folder.
 */
class HelpProvider
{
public:
  /**
   *  @brief Constructor
   */
  HelpProvider ();

  /**
   *  @brief Destructor
   */
  virtual ~HelpProvider () { }

  /**
   *  @brief Gets the main entry page for this category
   *
   *  @return The documentation path for the main entry point for this provider.
   */
  virtual std::string index (lay::HelpSource *src) const
  {
    return "/" + folder (src) + "/index.xml";
  }

  /**
   *  @brief Gets the DOM for a given URL
   *
   *  The DOM is the document in XML form which can be converted to HTML form for example
   *  or scanned for keywords.
   */
  virtual QDomDocument get (lay::HelpSource * /*src*/, const std::string & /*path*/) const
  {
    return QDomDocument ();
  }
  
  /**
   *  @brief Delivers the folder name below which the help documents of this provider are located
   *
   *  If this string is "doc" for example, all help documents will be looked up under
   *  "doc/...".
   */
  virtual std::string folder (lay::HelpSource * /*src*/) const = 0;

  /**
   *  @brief Gets the title for this category
   */
  virtual std::string title (lay::HelpSource * /*src*/) const = 0;
};

}

#endif

