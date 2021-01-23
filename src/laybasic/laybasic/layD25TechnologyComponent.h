
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_layD25TechnologyComponent
#define HDR_layD25TechnologyComponent

#include "ui_D25TechnologyComponentEditor.h"
#include "layTechnology.h"
#include "layGenericSyntaxHighlighter.h"

#include <memory>

namespace lay {

class D25TechnologyComponentEditor
  : public lay::TechnologyComponentEditor,
    public Ui::D25TechnologyComponentEditor
{
Q_OBJECT

public:
  D25TechnologyComponentEditor (QWidget *parent);

  void commit ();
  void setup ();

private slots:
  void cursor_position_changed ();

private:
  std::unique_ptr<lay::GenericSyntaxHighlighterAttributes> mp_hl_attributes, mp_hl_basic_attributes;
};

}

#endif

