
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


#include <map>

#include <QMessageBox>
#include <QScrollArea>
#include <QHeaderView>

#include "laySettingsForm.h"

#include "layMainWindow.h"
#include "layApplication.h"
#include "layPluginConfigPage.h"
#include "tlExceptions.h"
#include "tlLog.h"
#include "dbHershey.h"

namespace lay
{

// -------------------------------------------------------------

SettingsForm::SettingsForm (QWidget *parent, lay::Dispatcher *dispatcher, const char *name)
  : QDialog (parent), Ui::SettingsForm (),
    mp_dispatcher (dispatcher), m_finalize_recursion (false)
{ 
  setObjectName (QString::fromUtf8 (name));

  Ui::SettingsForm::setupUi (this);

  //  signals and slots connections
  connect (reset_pb, SIGNAL (clicked ()), this, SLOT (reset_clicked ()));
  connect (ok_button, SIGNAL (clicked ()), this, SLOT (ok_clicked ()));
  connect (cancel_button, SIGNAL (clicked ()), this, SLOT (reject ()));
  connect (apply_button, SIGNAL (clicked ()), this, SLOT (apply_clicked ()));
  connect (items_tree, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (item_changed (QTreeWidgetItem *, QTreeWidgetItem *)));

  items_tree->header ()->hide ();

  //  Collect all configuration pages
  std::vector <std::pair <std::string, lay::ConfigPage *> > pages;
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {

    std::string config_title;
    lay::ConfigPage *config_page = cls->config_page (0, config_title);
    if (config_page) {
      pages.push_back (std::make_pair (config_title, config_page));
    }

    std::vector <std::pair <std::string, lay::ConfigPage *> > pp = cls->config_pages (0);
    pages.insert (pages.end (), pp.begin (), pp.end ());

  }

  //  Create an empty dummy page as page 0
  QScrollArea *page0 = new QScrollArea (this);
  page0->setWidget (new QFrame (page0));
  pages_stack->addWidget (page0);

  //  Create the pages in the stack widget
  std::map <std::string, int> stack_headers;
  std::vector <QTreeWidgetItem *> items;
  for (std::vector <std::pair <std::string, lay::ConfigPage *> >::iterator p = pages.begin (); p != pages.end (); ++p) {

    std::string config_title = p->first;
    lay::ConfigPage *config_page = p->second;

    //  ignore ones without a title
    if (config_page && config_title.empty ()) {
      delete config_page;
      config_page = 0;
    } 

    if (config_page) {

      //  override external settings
      if (config_page->layout () == 0) {
        tl::warn << "No layout in configuration page " << p->first;
      } else {
        config_page->layout ()->setContentsMargins (0, 0, 0, 0);
      }

      m_config_pages.push_back (config_page);

      std::map <std::string, int>::const_iterator t = stack_headers.find (config_title);
      if (t == stack_headers.end ()) {

        //  not there yet ..
        QScrollArea *page = new QScrollArea (this);
        QFrame *page_frame = new QFrame (page);
        page->setWidget (page_frame);
        page->setWidgetResizable (true);
        QVBoxLayout *layout = new QVBoxLayout (page_frame);
        config_page->setParent (page_frame);
        layout->addWidget (config_page);

        int index = pages_stack->addWidget (page);
        stack_headers.insert (std::make_pair (config_title, index));

        QTreeWidgetItem *parent = 0;

        //  add the entry in the items tree
        while (! config_title.empty ()) {

          size_t sep = config_title.find ("|");
          std::string subtitle;
          if (sep != std::string::npos) {
            subtitle = std::string (config_title, 0, sep);
            config_title = std::string (config_title, sep + 1);
          } else {
            subtitle = config_title;
            config_title.clear ();
          }

          int count = parent ? parent->childCount () : items_tree->topLevelItemCount ();
          QTreeWidgetItem *child = 0;
          for (int i = 0; i < count && child == 0; ++i) {
            child = parent ? parent->child (i) : items_tree->topLevelItem (i);
            if (child != 0 && child->text (0) != tl::to_qstring (subtitle)) {
              child = 0;
            }
          }

          int current_index = config_title.empty () ? index : 0;

          if (!child) {

            if (parent) {
              child = new QTreeWidgetItem (parent);
            } else {
              child = new QTreeWidgetItem (items_tree);
            }

            child->setText (0, tl::to_qstring (subtitle));
            child->setData (0, Qt::UserRole, QVariant (current_index));

            items.push_back (child);

            //  initially expand all
            items_tree->expandItem (child);

          } else if (child->data (0, Qt::UserRole).toInt () == 0 && current_index > 0) {
            child->setData (0, Qt::UserRole, QVariant (current_index));
          }

          parent = child;

        }

      } else {

        //  already there - add a new item ..
        QScrollArea *page = dynamic_cast<QScrollArea *> (pages_stack->widget (t->second));
        if (page) {
          QFrame *page_frame = dynamic_cast<QFrame *> (page->widget ());
          if (page_frame) {
            QLayout *layout = page_frame->layout ();
            config_page->setParent (page_frame);
            layout->addWidget (config_page);
          }
        }
   
      }

    }

  }

  for (std::vector <QTreeWidgetItem *>::iterator i = items.begin (); i != items.end (); ++i) {
    if ((*i)->data (0, Qt::UserRole).toInt () == 0) { 
      (*i)->setFlags ((*i)->flags () & ~Qt::ItemIsSelectable);
    }
  }

  for (std::map <std::string, int>::const_iterator t = stack_headers.begin (); t != stack_headers.end (); ++t) {
    QScrollArea *page = dynamic_cast<QScrollArea *> (pages_stack->widget (t->second));
    if (page) {
      QFrame *page_frame = dynamic_cast<QFrame *> (page->widget ());
      if (page_frame) {
        QVBoxLayout *layout = dynamic_cast <QVBoxLayout *> (page_frame->layout ());
        if (layout) {
          layout->addStretch (0);
        }
      }
    }
  }

  items_tree->setCurrentItem (items_tree->topLevelItem (0));
}

void
SettingsForm::item_changed (QTreeWidgetItem *current, QTreeWidgetItem *)
{
  int index = -1;
  if (current) {
    index = current->data (0, Qt::UserRole).toInt ();
  }
  
  if (index < 0 || index >= pages_stack->count ()) {
    index = 0;
  }

  pages_stack->setCurrentIndex (index);

  if (index == 0 && current && current->childCount () > 0) {
    items_tree->setCurrentItem (current->child (0));
  }
}

void 
SettingsForm::setup ()
{
  //  recursion sentinel
  if (m_finalize_recursion || ! isVisible ()) {
    return;
  }

  //  setup the custom config pages
  for (std::vector <lay::ConfigPage *>::iterator cp = m_config_pages.begin (); cp != m_config_pages.end (); ++cp) {
    (*cp)->setup (mp_dispatcher);
  }
}

void 
SettingsForm::commit ()
{
  //  commit the custom config pages
  for (std::vector <lay::ConfigPage *>::iterator cp = m_config_pages.begin (); cp != m_config_pages.end (); ++cp) {
    (*cp)->commit (mp_dispatcher);
  }

  m_finalize_recursion = true;
  try {
    //  config_end will make the main window call setup on the settings form. 
    //  the recursion sentinel takes care of that.
    mp_dispatcher->config_end ();
    m_finalize_recursion = false;
  } catch (...) {
    m_finalize_recursion = false;
    throw;
  }
}

void
SettingsForm::reset_clicked ()
{
  if (QMessageBox::question (this, 
    QObject::tr ("Confirm Reset"),
    QObject::tr ("Are you sure to reset the configuration?\nThis operation will clear all custom settings and cannot be undone."),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No) == QMessageBox::Yes) {

    BEGIN_PROTECTED

    lay::ApplicationBase::instance ()->reset_config ();
    setup ();

    END_PROTECTED

  }
}

void 
SettingsForm::ok_clicked ()
{
  BEGIN_PROTECTED

  commit ();
  accept ();

  END_PROTECTED
}

void 
SettingsForm::apply_clicked ()
{
  BEGIN_PROTECTED

  commit ();

  END_PROTECTED
}

}

