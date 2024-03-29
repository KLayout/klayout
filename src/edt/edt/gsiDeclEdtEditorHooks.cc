
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


#include "gsiDecl.h"
#include "edtEditorHooks.h"
#include "layObjectInstPath.h"
#include "layLayoutView.h"

namespace gsi
{
gsi::Class<edt::EditorHooks> decl_EditorHooksBase ("lay", "EditorHooksBase",
  gsi::method ("begin_create", &edt::EditorHooks::begin_create, gsi::arg ("view"),
    "@brief Creation protocol - begin session\n"
    "This method is called to initiate an object creation session. The session is ended with "
    "\\end_create. Between these calls, new objects are announced with \\begin_new_objects, "
    "\\create and \\end_new_objects calls. These calls are repeated to indicate changes in the objects "
    "created.\n"
    "\n"
    "\\commit_create is called once before \\end_create to indicate that the last set of "
    "objects are committed to the database."
  ) +
  gsi::method ("begin_new_objects", &edt::EditorHooks::begin_new_objects,
    "@brief Creation protocol - begin new objects\n"
    "See \\begin_create for a description of the protocol."
  ) +
  gsi::method ("create", &edt::EditorHooks::create, gsi::arg ("object"), gsi::arg ("dbu"),
    "@brief Creation protocol - indicate a new object\n"
    "See \\begin_create for a description of the protocol."
  ) +
  gsi::method ("end_new_objects", &edt::EditorHooks::end_new_objects,
    "@brief Creation protocol - finish list of new objects\n"
    "See \\begin_create for a description of the protocol."
  ) +
  gsi::method ("commit_create", &edt::EditorHooks::commit_create,
    "@brief Creation protocol - commit new objects\n"
    "See \\begin_create for a description of the protocol."
  ) +
  gsi::method ("end_create", &edt::EditorHooks::end_create,
    "@brief Creation protocol - finish session\n"
    "See \\begin_create for a description of the protocol."
  ) +
  gsi::method ("begin_modify", &edt::EditorHooks::begin_modify, gsi::arg ("view"),
    "@brief Modification protocol - begin session\n"
    "This method is called to initiate an object modification session. The session is ended with "
    "\\end_modify. Between these calls, modified objects are announced with \\begin_modifications, "
    "\\modified and \\end_modifications calls. These calls are repeated to indicate changes in the objects "
    "modified while moving the mouse for example.\n"
    "\n"
    "\\commit_modify is called once before \\end_modify to indicate that the last set of "
    "objects are committed to the database."
  ) +
  gsi::method ("begin_modifications", &edt::EditorHooks::begin_modifications,
    "@brief Modification protocol - begin modifications\n"
    "See \\begin_modify for a description of the protocol."
  ) +
  gsi::method ("modified", &edt::EditorHooks::modified, gsi::arg ("object"), gsi::arg ("dbu"),
    "@brief Modification protocol - indicate a modified object\n"
    "See \\begin_modify for a description of the protocol."
  ) +
  gsi::method ("end_modifications", &edt::EditorHooks::end_modifications,
    "@brief Modification protocol - finish list of modifications\n"
    "See \\begin_modify for a description of the protocol."
  ) +
  gsi::method ("commit_modify", &edt::EditorHooks::commit_modify,
    "@brief Modification protocol - commit new objects\n"
    "See \\begin_modify for a description of the protocol."
  ) +
  gsi::method ("end_modify", &edt::EditorHooks::end_modify,
    "@brief Modification protocol - finish session\n"
    "See \\begin_modify for a description of the protocol."
  ) +
  gsi::method ("begin_edit", &edt::EditorHooks::begin_edit, gsi::arg ("view"),
    "@brief Editing protocol - begin session\n"
    "This method is called to initiate an object editing session. The session is ended with "
    "\\end_edit. Between these calls, edits are announced with \\begin_edits, "
    "\\transformed and \\end_edits calls. These calls are repeated to indicate changes in the objects "
    "modified while moving the mouse for example.\n"
    "\n"
    "\\commit_edit is called once before \\end_edit to indicate that the last set of "
    "objects are committed to the database."
  ) +
  gsi::method ("begin_edits", &edt::EditorHooks::begin_edits,
    "@brief Editing protocol - begin edits\n"
    "See \\begin_edit for a description of the protocol."
  ) +
  gsi::method ("transformed", &edt::EditorHooks::transformed, gsi::arg ("object"), gsi::arg ("trans"), gsi::arg ("dbu"),
    "@brief Editing protocol - indicate an object transformation\n"
    "See \\begin_edit for a description of the protocol."
  ) +
  gsi::method ("end_edits", &edt::EditorHooks::end_edits,
    "@brief Editing protocol - finish list of edits\n"
    "See \\begin_edit for a description of the protocol."
  ) +
  gsi::method ("commit_edit", &edt::EditorHooks::commit_edit,
    "@brief Editing protocol - commit new objects\n"
    "See \\begin_edit for a description of the protocol."
  ) +
  gsi::method ("end_edit", &edt::EditorHooks::end_edit,
    "@brief Editing protocol - finish session\n"
    "See \\begin_edit for a description of the protocol."
  ) +
  gsi::method ("technology=", &edt::EditorHooks::set_technology, gsi::arg ("technology"),
    "@brief sets the name of the technology the hooks are associated with\n"
    "This will clear all technology associations and associate the hooks with that technology only.\n"
  ) +
  gsi::method ("clear_technologies", &edt::EditorHooks::clear_technologies,
    "@brief Clears the list of technologies the hooks are associated with.\n"
    "See also \\add_technology.\n"
  ) +
  gsi::method ("add_technology", &edt::EditorHooks::add_technology, gsi::arg ("tech"),
    "@brief Additionally associates the hooks with the given technology.\n"
    "See also \\clear_technologies.\n"
  ) +
  gsi::method ("is_for_technology", &edt::EditorHooks::is_for_technology, gsi::arg ("tech"),
    "@brief Returns a value indicating whether the hooks are associated with the given technology.\n"
  ) +
  gsi::method ("for_technologies", &edt::EditorHooks::for_technologies,
    "@brief Returns a value indicating whether the hooks are associated with any technology.\n"
    "The method is equivalent to checking whether the \\technologies list is empty.\n"
  ) +
  gsi::method ("technologies", &edt::EditorHooks::get_technologies,
    "@brief Gets the list of technologies these hooks are associated with.\n"
  ) +
  gsi::method ("register", &edt::EditorHooks::register_editor_hook, gsi::arg ("hooks"),
    "@brief Registers the hooks in the system.\n"
    "The hooks will not be active before they are registered in the system. Registration will "
    "also transfer object ownership to the system."
  ),
  "@brief The base class for editor hooks\n"
  "Editor hooks allow implementing technology-specific callbacks into the editor "
  "for example to implement visual feedback about DRC rules.\n"
  "\n"
  "This class provides the basic interface. To implement callbacks, use the \\EditorHooks class."
  "\n"
  "The EditorHooksBase class has been introduced in version 0.29."
);

class EditorHooksImpl
  : public edt::EditorHooks
{
public:
  EditorHooksImpl () { }

  virtual void begin_create (lay::LayoutView *view)
  {
    if (f_begin_create.can_issue ()) {
      f_begin_create.issue<edt::EditorHooks, lay::LayoutView *> (&edt::EditorHooks::begin_create, view);
    } else {
      edt::EditorHooks::begin_create (view);
    }
  }

  virtual void begin_new_objects ()
  {
    if (f_begin_new_objects.can_issue ()) {
      f_begin_new_objects.issue<edt::EditorHooks> (&edt::EditorHooks::begin_new_objects);
    } else {
      edt::EditorHooks::begin_new_objects ();
    }
  }

  virtual void create (const lay::ObjectInstPath &object, double dbu)
  {
    if (f_create.can_issue ()) {
      f_create.issue<edt::EditorHooks, const lay::ObjectInstPath &, double> (&edt::EditorHooks::create, object, dbu);
    } else {
      edt::EditorHooks::create (object, dbu);
    }
  }

  virtual void end_new_objects ()
  {
    if (f_end_new_objects.can_issue ()) {
      f_end_new_objects.issue<edt::EditorHooks> (&edt::EditorHooks::end_new_objects);
    } else {
      edt::EditorHooks::end_new_objects ();
    }
  }

  virtual void commit_create ()
  {
    if (f_commit_create.can_issue ()) {
      f_commit_create.issue<edt::EditorHooks> (&edt::EditorHooks::commit_create);
    } else {
      edt::EditorHooks::commit_create ();
    }
  }

  virtual void end_create ()
  {
    if (f_end_create.can_issue ()) {
      f_end_create.issue<edt::EditorHooks> (&edt::EditorHooks::end_create);
    } else {
      edt::EditorHooks::end_create ();
    }
  }

  virtual void begin_modify (lay::LayoutView *view)
  {
    if (f_begin_modify.can_issue ()) {
      f_begin_modify.issue<edt::EditorHooks, lay::LayoutView *> (&edt::EditorHooks::begin_modify, view);
    } else {
      edt::EditorHooks::begin_modify (view);
    }
  }

  virtual void begin_modifications ()
  {
    if (f_begin_modifications.can_issue ()) {
      f_begin_modifications.issue<edt::EditorHooks> (&edt::EditorHooks::begin_modifications);
    } else {
      edt::EditorHooks::begin_modifications ();
    }
  }

  virtual void modified (const lay::ObjectInstPath &object, double dbu)
  {
    if (f_modified.can_issue ()) {
      f_modified.issue<edt::EditorHooks, const lay::ObjectInstPath &, double> (&edt::EditorHooks::modified, object, dbu);
    } else {
      edt::EditorHooks::modified (object, dbu);
    }
  }

  virtual void end_modifications ()
  {
    if (f_end_modifications.can_issue ()) {
      f_end_modifications.issue<edt::EditorHooks> (&edt::EditorHooks::end_modifications);
    } else {
      edt::EditorHooks::end_modifications ();
    }
  }

  virtual void commit_modify ()
  {
    if (f_commit_modify.can_issue ()) {
      f_commit_modify.issue<edt::EditorHooks> (&edt::EditorHooks::commit_modify);
    } else {
      edt::EditorHooks::commit_modify ();
    }
  }

  virtual void end_modify ()
  {
    if (f_end_modify.can_issue ()) {
      f_end_modify.issue<edt::EditorHooks> (&edt::EditorHooks::end_modify);
    } else {
      edt::EditorHooks::end_modify ();
    }
  }

  virtual void begin_edit (lay::LayoutView *view)
  {
    if (f_begin_edit.can_issue ()) {
      f_begin_edit.issue<edt::EditorHooks, lay::LayoutView *> (&edt::EditorHooks::begin_edit, view);
    } else {
      edt::EditorHooks::begin_edit (view);
    }
  }

  virtual void begin_edits ()
  {
    if (f_begin_edits.can_issue ()) {
      f_begin_edits.issue<edt::EditorHooks> (&edt::EditorHooks::begin_edits);
    } else {
      edt::EditorHooks::begin_edits ();
    }
  }

  virtual void transformed (const lay::ObjectInstPath &object, const db::DCplxTrans &trans, double dbu)
  {
    if (f_transformed.can_issue ()) {
      f_transformed.issue<edt::EditorHooks, const lay::ObjectInstPath &, const db::DCplxTrans &, double> (&edt::EditorHooks::transformed, object, trans, dbu);
    } else {
      edt::EditorHooks::transformed (object, trans, dbu);
    }
  }

  virtual void end_edits ()
  {
    if (f_end_edits.can_issue ()) {
      f_end_edits.issue<edt::EditorHooks> (&edt::EditorHooks::end_edits);
    } else {
      edt::EditorHooks::end_edits ();
    }
  }

  virtual void commit_edit ()
  {
    if (f_commit_edit.can_issue ()) {
      f_commit_edit.issue<edt::EditorHooks> (&edt::EditorHooks::commit_edit);
    } else {
      edt::EditorHooks::commit_edit ();
    }
  }

  virtual void end_edit ()
  {
    if (f_end_edit.can_issue ()) {
      f_end_edit.issue<edt::EditorHooks> (&edt::EditorHooks::end_edit);
    } else {
      edt::EditorHooks::end_edit ();
    }
  }

  gsi::Callback f_begin_create;
  gsi::Callback f_begin_new_objects;
  gsi::Callback f_create;
  gsi::Callback f_end_new_objects;
  gsi::Callback f_commit_create;
  gsi::Callback f_end_create;

  gsi::Callback f_begin_modify;
  gsi::Callback f_begin_modifications;
  gsi::Callback f_modified;
  gsi::Callback f_end_modifications;
  gsi::Callback f_commit_modify;
  gsi::Callback f_end_modify;

  gsi::Callback f_begin_edit;
  gsi::Callback f_begin_edits;
  gsi::Callback f_transformed;
  gsi::Callback f_end_edits;
  gsi::Callback f_commit_edit;
  gsi::Callback f_end_edit;
};

gsi::Class<EditorHooksImpl> decl_EditorHooks ("lay", "EditorHooks",
  callback ("begin_create", &EditorHooksImpl::begin_create, &EditorHooksImpl::f_begin_create,
    "@brief Creation protocol - begin session\n"
    "See \\EditorHooksBase for a description of the protocol"
  ),
  "@brief An implementation base class for editor hooks\n"
  // @@@
  "\n"
  "The EditorHooks class has been introduced in version 0.29."
);

}

