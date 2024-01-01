
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


#include "tlTimer.h"
#include "tlStream.h"

#include "dbReader.h"
#include "dbStream.h"
#include "dbLEFImporter.h"
#include "dbDEFImporter.h"
#include "dbLEFDEFImporter.h"

#include "layPlugin.h"
#include "layStream.h"
#include "layLEFDEFImportDialogs.h"

namespace db
{

// ---------------------------------------------------------------
//  LEFDEFPluginDeclaration definition and implementation

class LEFDEFPluginDeclaration
  : public lay::StreamReaderPluginDeclaration
{
public:
  LEFDEFPluginDeclaration ()
    : lay::StreamReaderPluginDeclaration (LEFDEFReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  lay::StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new lay::LEFDEFReaderOptionsEditor (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new LEFDEFReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new LEFDEFPluginDeclaration (), 10001, "LEFDEFReader");

}

