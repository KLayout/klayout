
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


#include "layMacroPropertiesDialog.h"
#include "layMainWindow.h"
#include "layQtTools.h"
#include "lymMacroInterpreter.h"
#include "tlString.h"

#include <QKeySequence>

namespace lay
{

MacroPropertiesDialog::MacroPropertiesDialog (QWidget *parent)
  : QDialog (parent)
{
  setupUi (this);

  connect (shortcut, SIGNAL (editingFinished ()), this, SLOT (shortcut_edited ()));
  activate_help_links (helpLabel);
}

int 
MacroPropertiesDialog::exec_dialog (lym::Macro *macro)
{
  update (macro);
  int ret = QDialog::exec ();
  if (ret) {
    commit (macro);
  }
  return ret;
}

void
MacroPropertiesDialog::shortcut_edited ()
{
  QKeySequence ks (shortcut->text ());
  shortcut->setText (ks.toString ());
}

void 
MacroPropertiesDialog::update (const lym::Macro *macro)
{
  std::string ip = "-";
  if (macro->interpreter () == lym::Macro::Ruby) {
    ip = "Ruby";
  } else if (macro->interpreter () == lym::Macro::Python) {
    ip = "Python";
  } else if (macro->interpreter () == lym::Macro::DSLInterpreter) {
    ip = lym::MacroInterpreter::description (macro->dsl_interpreter ());
  } 
  interpreterLabel->setText (tl::to_qstring (ip));

  propertiesFrame->setEnabled (! macro->is_readonly ());
  description->setText (tl::to_qstring (macro->description ()));
  version->setText (tl::to_qstring (macro->version ()));
  priority->setText (tl::to_qstring (tl::to_string (macro->priority ())));
  prolog->setText (tl::to_qstring (macro->prolog ()));
  epilog->setText (tl::to_qstring (macro->epilog ()));
  autorun->setChecked (macro->is_autorun ());
  autorunEarly->setChecked (macro->is_autorun_early ());
  shortcut->setText (tl::to_qstring (macro->shortcut ()));
  showmenu->setChecked (macro->show_in_menu ());
  groupName->setText (tl::to_qstring (macro->group_name ()));
  menuPath->setText (tl::to_qstring (macro->menu_path ()));
}

void 
MacroPropertiesDialog::commit (lym::Macro *macro)
{
  macro->set_description (tl::to_string (description->text ()));
  macro->set_version (tl::to_string (version->text ()));
  macro->set_prolog (tl::to_string (prolog->text ()));
  macro->set_epilog (tl::to_string (epilog->text ()));
  macro->set_autorun (autorun->isChecked ());
  macro->set_autorun_early (autorunEarly->isChecked ());
  macro->set_shortcut (tl::to_string (shortcut->text ()));
  macro->set_show_in_menu (showmenu->isChecked ());
  macro->set_group_name (tl::to_string (groupName->text ()));
  macro->set_menu_path (tl::to_string (menuPath->text ()));

  int p = 0;
  tl::from_string (tl::to_string (priority->text ()), p);
  macro->set_priority (p);
}

}

