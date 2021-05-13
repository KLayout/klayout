
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


#include "laybasicConfig.h"
#include "dbD25TechnologyComponent.h"
#include "layD25TechnologyComponent.h"
#include "layQtTools.h"

#include <QResource>
#include <QBuffer>

namespace lay
{

D25TechnologyComponentEditor::D25TechnologyComponentEditor (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  setupUi (this);

  src_te->setFont (monospace_font ());

  activate_help_links (label);

  QResource res (tl::to_qstring (":/syntax/d25_text.xml"));
  QByteArray data ((const char *) res.data (), int (res.size ()));
  if (res.isCompressed ()) {
    data = qUncompress (data);
  }

  QBuffer input (&data);
  input.open (QIODevice::ReadOnly);
  mp_hl_basic_attributes.reset (new GenericSyntaxHighlighterAttributes ());
  mp_hl_attributes.reset (new GenericSyntaxHighlighterAttributes (mp_hl_basic_attributes.get ()));
  lay::GenericSyntaxHighlighter *hl = new GenericSyntaxHighlighter (src_te, input, mp_hl_attributes.get ());
  input.close ();

  hl->setDocument (src_te->document ());

  connect (src_te, SIGNAL (cursorPositionChanged ()), this, SLOT (cursor_position_changed ()));
}

void
D25TechnologyComponentEditor::cursor_position_changed ()
{
  int line = src_te->textCursor ().block ().firstLineNumber () + 1;
  lnum_label->setText (tl::to_qstring (tl::sprintf (tl::to_string (tr ("Line %d")), line)));
}

void
D25TechnologyComponentEditor::commit ()
{
  db::D25TechnologyComponent *data = dynamic_cast <db::D25TechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  std::string src = tl::to_string (src_te->toPlainText ());

  //  test-compile before setting it
  db::D25TechnologyComponent tc;
  tc.set_src (src);
  tc.compile_from_source ();

  data->set_src (src);
}

void
D25TechnologyComponentEditor::setup ()
{
  db::D25TechnologyComponent *data = dynamic_cast <db::D25TechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  src_te->setPlainText (tl::to_qstring (data->src ()));
}

class D25TechnologyComponentEditorProvider
  : public lay::TechnologyEditorProvider
{
public:
  virtual lay::TechnologyComponentEditor *create_editor (QWidget *parent) const
  {
    return new D25TechnologyComponentEditor (parent);
  }
};

static tl::RegisteredClass<lay::TechnologyEditorProvider> editor_decl (new D25TechnologyComponentEditorProvider (), 3100, "d25");

} // namespace lay

