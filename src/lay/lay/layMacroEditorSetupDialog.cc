
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "layMacroEditorSetupDialog.h"

#include "tlString.h"

#include <cstdio>

namespace lay
{

static void
update_item (QListWidgetItem *item, QTextCharFormat format)
{
  item->setData (Qt::FontRole, format.font ());
  item->setData (Qt::ForegroundRole, format.foreground ());
  item->setData (Qt::BackgroundRole, format.background ());
}

MacroEditorSetupDialog::MacroEditorSetupDialog (QWidget *parent)
  : QDialog (parent), mp_data (0)
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
}

void 
MacroEditorSetupDialog::color_changed (QColor)
{
  commit_attributes (styles_list->currentItem ());
}

void 
MacroEditorSetupDialog::cb_changed (int)
{
  commit_attributes (styles_list->currentItem ());
}

void 
MacroEditorSetupDialog::update_font ()
{
  QFont f;
  f.setFamily (font_sel->currentFont().family ());
  f.setPointSize (font_size->value ());
  f.setFixedPitch (true);
  styles_list->setFont (f);
}

int
MacroEditorSetupDialog::exec_dialog (MacroEditorSetupDialogData &data)
{
  mp_data = &data;

  tab_width->setValue (data.tab_width);
  indent->setValue (data.indent);
  save_all_cb->setChecked (data.save_all_on_run);
  stop_on_exception->setChecked (data.stop_on_exception);
  watch_files->setChecked (data.file_watcher_enabled);

  if (data.font_size <= 0) {
    data.font_size = font ().pointSize ();
    data.font_family = "Monospace";
  }

  QFont f;
  f.setFamily (tl::to_qstring (data.font_family));
  font_sel->setCurrentFont (f);
  font_size->setValue (data.font_size);

  styles_list->blockSignals (true);

  styles_list->clear ();

  std::map <int, QString> basic_names;

  for (GenericSyntaxHighlighterAttributes::const_iterator a = data.basic_attributes.begin (); a != data.basic_attributes.end (); ++a) {
    QListWidgetItem *item = new QListWidgetItem (styles_list);
    QString n = tl::to_qstring (tl::to_string (QObject::tr ("(basic)")) + " ") + a->first;
    item->setText (n);
    item->setData (Qt::UserRole, -1);
    item->setData (Qt::UserRole + 1, a->second);
    basic_names.insert (std::make_pair (a->second, n));
    update_item (item, data.basic_attributes.format_for (a->second));
  }

  int na = 0;
  for (std::vector <std::pair <std::string, GenericSyntaxHighlighterAttributes> >::const_iterator sa = data.specific_attributes.begin (); sa != data.specific_attributes.end (); ++sa, ++na) {

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

  int r = QDialog::exec ();
  if (r) {

    if (styles_list->currentItem ()) {
      commit_attributes (styles_list->currentItem ());
    }

    data.tab_width = tab_width->value ();
    data.indent = indent->value ();
    data.save_all_on_run = save_all_cb->isChecked ();
    data.stop_on_exception = stop_on_exception->isChecked ();
    data.file_watcher_enabled = watch_files->isChecked ();

    data.font_family = tl::to_string (font_sel->currentFont ().family ());
    data.font_size = font_size->value ();

  }

  return r;
}

void 
MacroEditorSetupDialog::current_attribute_changed (QListWidgetItem *current, QListWidgetItem *previous)
{
  if (previous) {
    commit_attributes (previous);
  }

  update_attributes (current);
}

void 
MacroEditorSetupDialog::commit_attributes (QListWidgetItem *to_item)
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

  //  update all list styles (because we may modifiy a basic entry, we have to update dependent ones as well)
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
MacroEditorSetupDialog::update_attributes (QListWidgetItem *from_item)
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

