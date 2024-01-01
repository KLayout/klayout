
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


#include "layMacroVariableView.h"

#include "tlException.h"
#include "tlScriptError.h"
#include "tlString.h"
#include "gsiInspector.h"

#include <cstdio>

namespace 
{

/**
 *  @brief A placeholder item that is expanded on demand
 */
class PlaceholderItem
  : public QTreeWidgetItem
{
public:
  PlaceholderItem (gsi::Inspector *inspector)
    : mp_inspector (inspector)
  {
    // .. nothing yet ..
  }

  gsi::Inspector *inspector ()
  {
    return mp_inspector.get ();
  }

private:
  std::unique_ptr<gsi::Inspector> mp_inspector;
};

}

namespace lay
{

/**
 *  @brief Converts a tl::Variant to a nice string
 */
QString 
pretty_print (const tl::Variant &v) 
{
  if (v.is_nil ()) {

    return QObject::tr ("(nil)");

  } else if (v.is_double ()) {

    return tl::to_qstring (tl::sprintf ("%.12g", v.to_double ()));

  } else if (v.is_char ()) {

    std::string details = tl::sprintf ("#%d (0x%x)", v.to_int (), v.to_uint ());
    return tl::to_qstring (std::string ("'") + v.to_string () + "' " + details);

  } else if (v.is_ulong () || v.is_long () || v.is_ulonglong () || v.is_longlong ()) {

    std::string details = tl::sprintf (" (0x%llx)", v.to_ulonglong ());
    return tl::to_qstring (v.to_string () + details);

  } else {
    return tl::to_qstring (v.to_parsable_string ());
  }
}

/**
 *  @brief Return an inspector's description 
 *
 *  This function also adds an error catch to show evaluation errors
 */
static QString
inspector_description (const gsi::Inspector *inspector)
{
  try {
    return tl::to_qstring (inspector->description ());
  } catch (tl::ScriptError &ex) {
    return QObject::tr ("Error") + QString::fromUtf8 (": ") + tl::to_qstring (ex.basic_msg ());
  } catch (tl::Exception &ex) {
    return QObject::tr ("Error") + QString::fromUtf8 (": ") + tl::to_qstring (ex.msg ());
  } catch (...) {
    return QObject::tr ("Error (unspecific)");
  }
}

/**
 *  @brief Return an inspected value
 *
 *  This function also adds an error catch to show evaluation errors
 */
static QString
inspector_value (const gsi::Inspector *inspector, int index)
{
  try {
    return pretty_print (inspector->value (index));
  } catch (tl::ScriptError &ex) {
    return QObject::tr ("Error") + QString::fromUtf8 (": ") + tl::to_qstring (ex.basic_msg ());
  } catch (tl::Exception &ex) {
    return QObject::tr ("Error") + QString::fromUtf8 (": ") + tl::to_qstring (ex.msg ());
  } catch (...) {
    return QObject::tr ("Error (unspecific)");
  }
}

/**
 *  @brief Helper: updates a text and makes it bold if it changed
 */
static void
update_value (QTreeWidgetItem *item, const QString &text, bool fresh)
{
  int column = 1;

  QFont f (item->font (column));
  if (! fresh && item->text (column) != text) {
    f.setWeight (QFont::Bold);
  } else {
    f.setWeight (QFont::Normal);
  }
  item->setFont (column, f);
  item->setText (column, text);
  item->setToolTip (column, text);
}

MacroVariableView::MacroVariableView (QWidget *parent)
  : QTreeWidget (parent), m_show_all (false)
{
  connect (this, SIGNAL (itemExpanded (QTreeWidgetItem *)), this, SLOT (expanded (QTreeWidgetItem *)));
}

void MacroVariableView::set_inspector (gsi::Inspector *inspector)
{
  if (inspector != mp_inspector.get ()) {
    bool fresh = (! inspector || ! mp_inspector.get () || ! mp_inspector->equiv (inspector));
    if (fresh) {
      clear ();
    }
    mp_inspector.reset (inspector);
    if (inspector) {
      sync (fresh);
    }
  }
}

void MacroVariableView::sync (bool fresh)
{
  sync (invisibleRootItem (), mp_inspector.get (), fresh);
}

void MacroVariableView::expanded (QTreeWidgetItem *item)
{
  //  Replace the (single) placeholder item by the complete list of items
  if (item->childCount () > 0) {
    PlaceholderItem *ph = dynamic_cast<PlaceholderItem *> (item->child (0));
    if (ph) {
      std::unique_ptr<QTreeWidgetItem> ph_taken (item->takeChild (0));
      sync (item, ph->inspector (), true);
    }
  }
}

void MacroVariableView::set_show_all (bool show_all)
{
  if (m_show_all != show_all) {
    m_show_all = show_all;
    if (mp_inspector.get ()) {
      sync (true);
    }
  }
}

void MacroVariableView::sync_item (QTreeWidgetItem *parent, gsi::Inspector *inspector, const QString &key, size_t index, int pos, bool fresh)
{
  if (pos == parent->childCount ()) {

    QTreeWidgetItem *item = new QTreeWidgetItem ();
    item->setText (0, key);
    QFont f (item->font (0));
    f.setWeight (QFont::Bold);
    item->setFont (0, f);
    parent->addChild (item);

    if (inspector->has_children (index)) {
      gsi::Inspector *ci = inspector->child_inspector (index);
      item->addChild (new PlaceholderItem (ci));
      update_value (item, inspector_description (ci), fresh);
    } else {
      update_value (item, inspector_value (inspector, int (index)), fresh);
    }

  } else if (parent->child (pos)->text (0) != key) {

    QTreeWidgetItem *item = new QTreeWidgetItem ();
    item->setText (0, key);
    QFont f (item->font (0));
    f.setWeight (QFont::Bold);
    item->setFont (0, f);
    parent->insertChild (pos, item);

    if (inspector->has_children (index)) {
      gsi::Inspector *ci = inspector->child_inspector (index);
      item->addChild (new PlaceholderItem (ci));
      update_value (item, inspector_description (ci), fresh);
    } else {
      update_value (item, inspector_value (inspector, int (index)), fresh);
    }

  } else {

    QTreeWidgetItem *item = parent->child (pos);

    if (inspector->has_children (index)) {

      std::unique_ptr<gsi::Inspector> ci (inspector->child_inspector (index));
      update_value (item, inspector_description (ci.get ()), false);
      if (item->isExpanded ()) {
        sync (item, ci.get (), fresh);
      } else if (item->childCount () == 0) {
        item->addChild (new PlaceholderItem (ci.release ()));
      }

    } else {

      update_value (item, inspector_value (inspector, int (index)), false);
      while (item->childCount () > 0) {
        delete item->takeChild (0);
      }

    }

  }
}

void MacroVariableView::sync (QTreeWidgetItem *parent, gsi::Inspector *inspector, bool fresh)
{
  if (inspector->has_keys ()) {

    //  collect all top-level items
    std::map<QString, size_t> keys;

    for (size_t n = inspector->count (); n-- > 0; ) {
      gsi::Inspector::Visibility vis = inspector->visibility (n);
      if (vis == gsi::Inspector::Always || (m_show_all && vis == gsi::Inspector::IfRequested)) {
        QString k = tl::to_qstring (inspector->key (n));
        if (k.isEmpty ()) {
          k = pretty_print (inspector->keyv (n));
        }
        keys.insert (std::make_pair (k, n));
      }
    }

    //  delete all items which are no longer present
    for (int i = 0; i < parent->childCount (); ++i) {
      QString key = parent->child (i)->text (0);
      if (keys.find (key) == keys.end ()) {
        delete parent->takeChild (i);
        --i;
      }
    }

    //  insert or update new items
    int i = 0;
    for (std::map<QString, size_t>::const_iterator k = keys.begin (); k != keys.end (); ++k, ++i) {
      sync_item (parent, inspector, k->first, k->second, i, fresh);
    }

  } else {

    size_t n = inspector->count ();

    //  delete all items which are no longer present
    while (size_t (parent->childCount ()) > n) {
      delete parent->takeChild (int (n));
    }

    //  insert or update new items
    for (size_t i = 0; i < n; ++i) {
      sync_item (parent, inspector, QString::fromUtf8("[%1]").arg (i), i, int (i), fresh);
    }

  }
}

}

