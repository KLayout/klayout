
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#if defined(HAVE_QT)

#ifndef HDR_layEditorOptionsPages
#define HDR_layEditorOptionsPages

#include "laybasicCommon.h"
#include "layEditorOptionsPage.h"

#include <QFrame>
#include <QDialog>

#include <vector>
#include <string>

class QTabWidget;
class QLabel;
class QDialogButtonBox;
class QAbstractButton;

namespace lay
{

class PluginDeclaration;
class Dispatcher;
class Plugin;
class EditorOptionsModalPages;

/**
 *  @brief The object properties tab widget
 */
class LAYBASIC_PUBLIC EditorOptionsPages
  : public QFrame
{
Q_OBJECT

public:
  EditorOptionsPages (QWidget *parent, const std::vector<lay::EditorOptionsPage *> &pages, lay::Dispatcher *root);
  ~EditorOptionsPages ();

  void unregister_page (lay::EditorOptionsPage *page);
  void activate_page (lay::EditorOptionsPage *page);
  void activate (const lay::Plugin *plugin);
  void focusInEvent (QFocusEvent *event);
  void make_page_current (lay::EditorOptionsPage *page);
  bool exec_modal (lay::EditorOptionsPage *page);

  const std::vector <lay::EditorOptionsPage *> &pages () const
  {
    return m_pages;
  }

  bool has_content () const;
  bool has_modal_content () const;
  void do_apply (bool modal);

public slots:
  void apply ();
  void setup ();

private:
  std::vector <lay::EditorOptionsPage *> m_pages;
  lay::Dispatcher *mp_dispatcher;
  QTabWidget *mp_pages;
  EditorOptionsModalPages *mp_modal_pages;

  void update (lay::EditorOptionsPage *page);
};

/**
 *  @brief The object properties modal page dialog
 */
class LAYBASIC_PUBLIC EditorOptionsModalPages
  : public QDialog
{
Q_OBJECT

public:
  EditorOptionsModalPages (EditorOptionsPages *parent);
  ~EditorOptionsModalPages ();

  int count ();
  int current_index ();
  void set_current_index (int index);
  void add_page (EditorOptionsPage *page);
  void remove_page (int index);
  EditorOptionsPage *widget (int index);

private slots:
  void accept ();
  void reject ();
  void clicked (QAbstractButton *button);

private:
  EditorOptionsPages *mp_parent;
  QTabWidget *mp_pages;
  QFrame *mp_single_page_frame;
  EditorOptionsPage *mp_single_page;
  QDialogButtonBox *mp_button_box;

  void update_title ();
};

}

#endif

#endif  //  defined(HAVE_QT)
