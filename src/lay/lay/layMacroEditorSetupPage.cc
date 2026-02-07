
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


#include "layMacroEditorSetupPage.h"
#include "layMacroEditorPage.h"
#include "layMacroEditorDialog.h"
#include "layGenericSyntaxHighlighter.h"
#include "layDispatcher.h"
#include "layQtTools.h"

#include "lymMacro.h"

#include "tlString.h"

#include <vector>
#include <cstdio>

namespace lay
{

struct MacroEditorSetupDialogData
  : public QObject
{
  MacroEditorSetupDialogData (QObject *parent)
    : QObject(parent),
      basic_attributes (0), tab_width (8), indent (2), save_all_on_run (true), stop_on_exception (true), file_watcher_enabled (true), font_size (0)
  {
  }

  GenericSyntaxHighlighterAttributes basic_attributes;
  std::vector <std::pair <std::string, GenericSyntaxHighlighterAttributes> > specific_attributes;
  int tab_width;
  int indent;
  bool save_all_on_run;
  bool stop_on_exception;
  bool file_watcher_enabled;
  std::string font_family;
  int font_size;
  std::set <std::string> ignore_exceptions_list;

  void setup (lay::Dispatcher *root)
  {
    lay::MacroEditorHighlighters highlighters (this);
    std::string styles;
    root->config_get (cfg_macro_editor_styles, styles);
    highlighters.load (styles);

    if (highlighters.basic_attributes ()) {
      basic_attributes.assign (*highlighters.basic_attributes ());
    }
    for (lay::MacroEditorHighlighters::const_iterator a = highlighters.begin (); a != highlighters.end (); ++a) {
      specific_attributes.push_back (std::make_pair (a->first, GenericSyntaxHighlighterAttributes (& basic_attributes)));
      specific_attributes.back ().second.assign (a->second);
    }

    root->config_get (cfg_macro_editor_save_all_on_run, save_all_on_run);
    root->config_get (cfg_macro_editor_file_watcher_enabled, file_watcher_enabled);
    root->config_get (cfg_macro_editor_stop_on_exception, stop_on_exception);
    root->config_get (cfg_macro_editor_tab_width, tab_width);
    root->config_get (cfg_macro_editor_indent, indent);
    root->config_get (cfg_macro_editor_font_family, font_family);
    root->config_get (cfg_macro_editor_font_size, font_size);

    std::string il;
    root->config_get (cfg_macro_editor_ignore_exception_list, il);
    ignore_exceptions_list.clear ();
    tl::Extractor ex (il.c_str ());
    while (! ex.at_end ()) {
      std::string f;
      ex.read_word_or_quoted (f);
      ex.test (";");
      ignore_exceptions_list.insert (f);
    }
  }

  void commit (lay::Dispatcher *root)
  {
    lay::MacroEditorHighlighters highlighters (this);

    if (highlighters.basic_attributes ()) {
      highlighters.basic_attributes ()->assign (basic_attributes);
    }

    for (MacroEditorHighlighters::iterator a = highlighters.begin (); a != highlighters.end (); ++a) {
      for (std::vector< std::pair<std::string, GenericSyntaxHighlighterAttributes> >::const_iterator i = specific_attributes.begin (); i != specific_attributes.end (); ++i) {
        if (i->first == a->first) {
          a->second.assign (i->second);
          break;
        }
      }
    }

    //  write configuration

    root->config_set (cfg_macro_editor_styles, highlighters.to_string ());
    root->config_set (cfg_macro_editor_save_all_on_run, save_all_on_run);
    root->config_set (cfg_macro_editor_file_watcher_enabled, file_watcher_enabled);
    root->config_set (cfg_macro_editor_stop_on_exception, stop_on_exception);
    root->config_set (cfg_macro_editor_tab_width, tab_width);
    root->config_set (cfg_macro_editor_indent, indent);
    root->config_set (cfg_macro_editor_font_family, font_family);
    root->config_set (cfg_macro_editor_font_size, font_size);

    std::string il;
    for (std::set<std::string>::const_iterator i = ignore_exceptions_list.begin (); i != ignore_exceptions_list.end (); ++i) {
      if (! il.empty ()) {
        il += ";";
      }
      il += tl::to_quoted_string (*i);
    }
    root->config_set (cfg_macro_editor_ignore_exception_list, il);

  }
};

static void
update_item (QListWidgetItem *item, QTextCharFormat format)
{
  item->setData (Qt::FontRole, format.font ());
  item->setData (Qt::ForegroundRole, format.foreground ());
  item->setData (Qt::BackgroundRole, format.background ());
}

MacroEditorSetupPage::MacroEditorSetupPage (QWidget *parent)
  : lay::ConfigPage (parent),
    mp_data (new MacroEditorSetupDialogData (this))
{
  setupUi (this);

  connect (styles_list, SIGNAL (currentItemChanged (QListWidgetItem *, QListWidgetItem *)), this, SLOT (current_attribute_changed (QListWidgetItem *, QListWidgetItem *)));
  connect (bold_cb, SIGNAL (stateChanged (int)), this, SLOT (cb_changed (int)));
  connect (italic_cb, SIGNAL (stateChanged (int)), this, SLOT (cb_changed (int)));
  connect (underline_cb, SIGNAL (stateChanged (int)), this, SLOT (cb_changed (int)));
  connect (strikeout_cb, SIGNAL (stateChanged (int)), this, SLOT (cb_changed (int)));
  connect (text_color_button, SIGNAL (color_changed (QColor)), this, SLOT (color_changed (QColor)));
  connect (background_color_button, SIGNAL (color_changed (QColor)), this, SLOT (color_changed (QColor)));
  connect (font_sel, SIGNAL (currentFontChanged (const QFont &)), this, SLOT (update_font ()));
  connect (font_size, SIGNAL (valueChanged (int)), this, SLOT (update_font ()));
  connect (clear_el, SIGNAL (clicked ()), this, SLOT (clear_exception_list ()));
}

MacroEditorSetupPage::~MacroEditorSetupPage ()
{
  //  .. nothing yet ..
}

void 
MacroEditorSetupPage::color_changed (QColor)
{
  commit_attributes (styles_list->currentItem ());
}

void 
MacroEditorSetupPage::cb_changed (int)
{
  commit_attributes (styles_list->currentItem ());
}

void
MacroEditorSetupPage::clear_exception_list ()
{
  mp_data->ignore_exceptions_list.clear ();
  update_ignore_exception_list ();
}

void
MacroEditorSetupPage::update_ignore_exception_list ()
{
  exception_list->clear ();
  for (std::set<std::string>::const_iterator i = mp_data->ignore_exceptions_list.begin (); i != mp_data->ignore_exceptions_list.end (); ++i) {
    exception_list->addItem (tl::to_qstring (*i));
  }
}

void 
MacroEditorSetupPage::update_font ()
{
  QFont f;
  f.setFamily (font_sel->currentFont().family ());
  f.setPointSize (font_size->value ());
  f.setFixedPitch (true);
  styles_list->setFont (f);
}

void
MacroEditorSetupPage::setup (Dispatcher *root)
{
  delete mp_data;
  mp_data = new MacroEditorSetupDialogData (this);
  mp_data->setup (root);

  update_ignore_exception_list ();

  tab_width->setValue (mp_data->tab_width);
  indent->setValue (mp_data->indent);
  save_all_cb->setChecked (mp_data->save_all_on_run);
  stop_on_exception->setChecked (mp_data->stop_on_exception);
  watch_files->setChecked (mp_data->file_watcher_enabled);

  if (mp_data->font_size <= 0) {
    mp_data->font_size = font ().pointSize ();
    mp_data->font_family = tl::to_string (monospace_font ().family ());
  }

  QFont f;
  f.setFamily (tl::to_qstring (mp_data->font_family));
  font_sel->setCurrentFont (f);
  font_size->setValue (mp_data->font_size);

  styles_list->blockSignals (true);

  styles_list->clear ();

  std::map <int, QString> basic_names;

  for (GenericSyntaxHighlighterAttributes::const_iterator a = mp_data->basic_attributes.begin (); a != mp_data->basic_attributes.end (); ++a) {
    QListWidgetItem *item = new QListWidgetItem (styles_list);
    QString n = tl::to_qstring (tl::to_string (QObject::tr ("(basic)")) + " ") + a->first;
    item->setText (n);
    item->setData (Qt::UserRole, -1);
    item->setData (Qt::UserRole + 1, a->second);
    basic_names.insert (std::make_pair (a->second, n));
    update_item (item, mp_data->basic_attributes.format_for (a->second));
  }

  int na = 0;
  for (std::vector <std::pair <std::string, GenericSyntaxHighlighterAttributes> >::const_iterator sa = mp_data->specific_attributes.begin (); sa != mp_data->specific_attributes.end (); ++sa, ++na) {

    QString l = tl::to_qstring ("(" + sa->first + ") ");

    for (GenericSyntaxHighlighterAttributes::const_iterator a = sa->second.begin (); a != sa->second.end (); ++a) {
      QListWidgetItem *item = new QListWidgetItem (styles_list);
      item->setData (Qt::UserRole, na);
      item->setData (Qt::UserRole + 1, a->second);
      std::map <int, QString>::const_iterator bn = basic_names.find (sa->second.basic_id (a->second));
      if (bn != basic_names.end ()) {
        item->setText (l + a->first + QObject::tr (" - based on ") + bn->second);
      } else {
        item->setText (l + a->first);
      }
      update_item (item, sa->second.format_for (a->second));
    }

  }

  styles_list->blockSignals (false);

  update_attributes (styles_list->currentItem ());
  update_font ();
}

void
MacroEditorSetupPage::commit (Dispatcher *root)
{
  if (styles_list->currentItem ()) {
    commit_attributes (styles_list->currentItem ());
  }

  mp_data->tab_width = tab_width->value ();
  mp_data->indent = indent->value ();
  mp_data->save_all_on_run = save_all_cb->isChecked ();
  mp_data->stop_on_exception = stop_on_exception->isChecked ();
  mp_data->file_watcher_enabled = watch_files->isChecked ();

  mp_data->font_family = tl::to_string (font_sel->currentFont ().family ());
  mp_data->font_size = font_size->value ();

  mp_data->commit (root);
}

void 
MacroEditorSetupPage::current_attribute_changed (QListWidgetItem *current, QListWidgetItem *previous)
{
  if (previous) {
    commit_attributes (previous);
  }

  update_attributes (current);
}

void 
MacroEditorSetupPage::commit_attributes (QListWidgetItem *to_item)
{
  if (! to_item) {
    return;
  }

  GenericSyntaxHighlighterAttributes *attributes = 0;

  int ai = to_item->data (Qt::UserRole).toInt ();
  if (ai < 0) {
    attributes = &mp_data->basic_attributes;
  } else if (ai < int (mp_data->specific_attributes.size ())) {
    attributes = &mp_data->specific_attributes [ai].second;
  }

  if (attributes) {

    int id = to_item->data (Qt::UserRole + 1).toInt ();
    QTextCharFormat style = attributes->specific_style (id);

    if (underline_cb->checkState () == Qt::PartiallyChecked) {
      style.clearProperty (QTextFormat::FontUnderline);
    } else {
      style.setProperty (QTextFormat::FontUnderline, underline_cb->checkState () == Qt::Checked);
    }

    if (italic_cb->checkState () == Qt::PartiallyChecked) {
      style.clearProperty (QTextFormat::FontItalic);
    } else {
      style.setProperty (QTextFormat::FontItalic, italic_cb->checkState () == Qt::Checked);
    }

    if (strikeout_cb->checkState () == Qt::PartiallyChecked) {
      style.clearProperty (QTextFormat::FontStrikeOut);
    } else {
      style.setProperty (QTextFormat::FontStrikeOut, strikeout_cb->checkState () == Qt::Checked);
    }

    if (bold_cb->checkState () == Qt::PartiallyChecked) {
      style.clearProperty (QTextFormat::FontWeight);
    } else {
      style.setProperty (QTextFormat::FontWeight, bold_cb->checkState () == Qt::Checked ? QFont::Bold : QFont::Normal);
    }

    if (text_color_button->get_color () == QColor ()) {
      style.clearProperty (QTextFormat::ForegroundBrush);
    } else {
      style.setProperty (QTextFormat::ForegroundBrush, QBrush (text_color_button->get_color ()));
    }

    if (background_color_button->get_color () == QColor ()) {
      style.clearProperty (QTextFormat::BackgroundBrush);
    } else {
      style.setProperty (QTextFormat::BackgroundBrush, QBrush (background_color_button->get_color ()));
    }

    attributes->set_style (id, style);

  }

  //  update all list styles (because we may modify a basic entry, we have to update dependent ones as well)
  for (int i = 0; i < styles_list->count (); ++i) {

    QListWidgetItem *item = styles_list->item (i);

    int ai = item->data (Qt::UserRole).toInt ();
    if (ai < 0) {
      attributes = &mp_data->basic_attributes;
    } else if (ai < int (mp_data->specific_attributes.size ())) {
      attributes = &mp_data->specific_attributes [ai].second;
    }

    if (attributes) {
      int id = item->data (Qt::UserRole + 1).toInt ();
      update_item (item, attributes->format_for (id));
    }

  }

}

void 
MacroEditorSetupPage::update_attributes (QListWidgetItem *from_item)
{
  if (from_item) {

    text_color_button->setEnabled (true);
    background_color_button->setEnabled (true);
    bold_cb->setEnabled (true);
    italic_cb->setEnabled (true);
    underline_cb->setEnabled (true);
    strikeout_cb->setEnabled (true);

    GenericSyntaxHighlighterAttributes *attributes = 0;

    int ai = from_item->data (Qt::UserRole).toInt ();
    if (ai < 0) {
      attributes = &mp_data->basic_attributes;
    } else if (ai < int (mp_data->specific_attributes.size ())) {
      attributes = &mp_data->specific_attributes [ai].second;
    }

    if (attributes) {

      int id = from_item->data (Qt::UserRole + 1).toInt ();
      QTextCharFormat style = attributes->specific_style (id);

      if (style.hasProperty (QTextFormat::FontUnderline)) {
        underline_cb->setCheckState (style.boolProperty (QTextFormat::FontUnderline) ? Qt::Checked : Qt::Unchecked);
      } else {
        underline_cb->setCheckState (Qt::PartiallyChecked);
      }

      if (style.hasProperty (QTextFormat::FontStrikeOut)) {
        strikeout_cb->setCheckState (style.boolProperty (QTextFormat::FontStrikeOut) ? Qt::Checked : Qt::Unchecked);
      } else {
        strikeout_cb->setCheckState (Qt::PartiallyChecked);
      }

      if (style.hasProperty (QTextFormat::FontItalic)) {
        italic_cb->setCheckState (style.boolProperty (QTextFormat::FontItalic) ? Qt::Checked : Qt::Unchecked);
      } else {
        italic_cb->setCheckState (Qt::PartiallyChecked);
      }

      if (style.hasProperty (QTextFormat::FontWeight)) {
        bold_cb->setCheckState (style.intProperty (QTextFormat::FontWeight) == QFont::Bold ? Qt::Checked : Qt::Unchecked);
      } else {
        bold_cb->setCheckState (Qt::PartiallyChecked);
      }

      if (style.hasProperty (QTextFormat::ForegroundBrush)) {
        text_color_button->set_color (style.brushProperty (QTextFormat::ForegroundBrush).color ());
      } else {
        text_color_button->set_color (QColor ());
      }

      if (style.hasProperty (QTextFormat::BackgroundBrush)) {
        background_color_button->set_color (style.brushProperty (QTextFormat::BackgroundBrush).color ());
      } else {
        background_color_button->set_color (QColor ());
      }

    }

  } else {

    text_color_button->setEnabled (false);
    text_color_button->set_color (QColor ());
    background_color_button->setEnabled (false);
    background_color_button->set_color (QColor ());
    bold_cb->setCheckState (Qt::PartiallyChecked);
    bold_cb->setEnabled (false);
    italic_cb->setCheckState (Qt::PartiallyChecked);
    italic_cb->setEnabled (false);
    underline_cb->setCheckState (Qt::PartiallyChecked);
    underline_cb->setEnabled (false);
    strikeout_cb->setCheckState (Qt::PartiallyChecked);
    strikeout_cb->setEnabled (false);

  }
}

}

