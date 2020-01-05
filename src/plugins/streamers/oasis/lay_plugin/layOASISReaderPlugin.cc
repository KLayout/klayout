
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


#include "dbOASIS.h"
#include "dbOASISReader.h"
#include "dbLoadLayoutOptions.h"
#include "layOASISReaderPlugin.h"
#include "layStream.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  OASISReaderPluginDeclaration definition and implementation

class OASISReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  OASISReaderPluginDeclaration () 
    : StreamReaderPluginDeclaration (db::OASISReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget * /*parent*/) const
  {
    return 0;
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::OASISReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::OASISReaderPluginDeclaration (), 10000, "OASISReader");

}





