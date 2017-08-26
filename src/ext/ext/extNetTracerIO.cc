
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#include "extNetTracerIO.h"
#include "extNetTracerConfig.h"

#include "layConfigurationDialog.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layFinder.h"
#include "layLayoutView.h"
#include "layTechSetupDialog.h"
#include "layFileDialog.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlXMLWriter.h"
#include "tlUtils.h"
#include "gsiDecl.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QItemDelegate>
#include <QHeaderView>
#include <QPainter>

#include <fstream>
#include <sstream>

namespace ext
{

std::string net_tracer_component_name ("connectivity");

// -----------------------------------------------------------------------------------
//  NetTracerConnectionInfo implementation

NetTracerConnectionInfo::NetTracerConnectionInfo ()
{
  // .. nothing yet ..
}

NetTracerConnectionInfo::NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &lb)
  : m_la (la), m_via (), m_lb (lb)
{
  // .. nothing yet ..
}

NetTracerConnectionInfo::NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &via, const NetTracerLayerExpressionInfo &lb)
  : m_la (la), m_via (via), m_lb (lb)
{
  // .. nothing yet ..
}

static int get_layer_id (const NetTracerLayerExpressionInfo &e, const db::Layout &layout, const NetTracerTechnologyComponent &tech, NetTracerData *data) 
{ 
  std::auto_ptr<NetTracerLayerExpression> expr_in (NetTracerLayerExpressionInfo::compile (e.to_string ()).get (layout, tech));
  int l = expr_in->alias_for ();
  if (l < 0 && data) {
    l = data->find_symbol (e.to_string ());
    if (l < 0) {
      return int (data->register_logical_layer (expr_in.release (), 0));
    } 
  } 
  return l;
}

NetTracerConnection 
NetTracerConnectionInfo::get (const db::Layout &layout, const NetTracerTechnologyComponent &tech, NetTracerData &data) const
{
  int la = get_layer_id (m_la, layout, tech, &data);
  int lb = get_layer_id (m_lb, layout, tech, &data);

  if (! m_via.to_string ().empty ()) {
    int via = get_layer_id (m_via, layout, tech, &data);
    return NetTracerConnection (la, via, lb);
  } else {
    return NetTracerConnection (la, lb);
  }
}

std::string 
NetTracerConnectionInfo::to_string () const
{
  std::string res;
  res += m_la.to_string ();
  res += ",";
  res += m_via.to_string ();
  res += ",";
  res += m_lb.to_string ();

  return res;
}

void 
NetTracerConnectionInfo::parse (tl::Extractor &ex)
{
  m_la = NetTracerLayerExpressionInfo::parse (ex);
  ex.expect (",");
  m_via = NetTracerLayerExpressionInfo::parse (ex);
  ex.expect (",");
  m_lb = NetTracerLayerExpressionInfo::parse (ex);
}

// -----------------------------------------------------------------------------------
//  NetTracerSymbolInfo implementation

NetTracerSymbolInfo::NetTracerSymbolInfo ()
{
  // .. nothing yet ..
}

NetTracerSymbolInfo::NetTracerSymbolInfo (const db::LayerProperties &symbol, const std::string &expression)
  : m_symbol (symbol), m_expression (expression)
{
  // .. nothing yet ..
}

std::string 
NetTracerSymbolInfo::to_string () const
{
  std::string res;
  res += m_symbol.to_string ();
  res += "=";
  res += tl::to_quoted_string(m_expression);

  return res;
}

void 
NetTracerSymbolInfo::parse (tl::Extractor &ex)
{
  m_symbol.read (ex);
  ex.expect ("=");
  ex.read_word_or_quoted (m_expression);
}

// -----------------------------------------------------------------------------------------
//  NetTracerLayerExpressionInfo implementation

NetTracerLayerExpressionInfo::NetTracerLayerExpressionInfo ()
  : mp_a (0), mp_b (0), m_op (NetTracerLayerExpression::OPNone)
{
  //  .. nothing yet ..
}

NetTracerLayerExpressionInfo::~NetTracerLayerExpressionInfo ()
{
  delete mp_a; 
  mp_a = 0;
  delete mp_b;
  mp_b = 0;
}

NetTracerLayerExpressionInfo::NetTracerLayerExpressionInfo (const NetTracerLayerExpressionInfo &other)
  : m_expression (other.m_expression), m_a (other.m_a), m_b (other.m_b), mp_a (0), mp_b (0), m_op (other.m_op)
{
  if (other.mp_a) {
    mp_a = new NetTracerLayerExpressionInfo (*other.mp_a);
  }
  if (other.mp_b) {
    mp_b = new NetTracerLayerExpressionInfo (*other.mp_b);
  }
}

NetTracerLayerExpressionInfo &
NetTracerLayerExpressionInfo::operator= (const NetTracerLayerExpressionInfo &other)
{
  if (this != &other) {

    m_expression = other.m_expression;

    delete mp_a; 
    mp_a = 0;
    delete mp_b;
    mp_b = 0;

    m_a = other.m_a; 
    m_b = other.m_b; 
    m_op = other.m_op;

    if (other.mp_a) {
      mp_a = new NetTracerLayerExpressionInfo (*other.mp_a);
    }
    if (other.mp_b) {
      mp_b = new NetTracerLayerExpressionInfo (*other.mp_b);
    }

  }

  return *this;
}

void 
NetTracerLayerExpressionInfo::merge (NetTracerLayerExpression::Operator op, const NetTracerLayerExpressionInfo &other)
{
  if (m_op != NetTracerLayerExpression::OPNone) {
    NetTracerLayerExpressionInfo *e = new NetTracerLayerExpressionInfo (*this);
    *this = NetTracerLayerExpressionInfo ();
    mp_a = e;
  }

  m_op = op;

  if (other.m_op == NetTracerLayerExpression::OPNone) {
    if (other.mp_a) {
      mp_b = new NetTracerLayerExpressionInfo (*other.mp_a);
    } else {
      m_b = other.m_a;
    }
  } else {
    mp_b = new NetTracerLayerExpressionInfo (other);
  }
}

NetTracerLayerExpressionInfo 
NetTracerLayerExpressionInfo::parse_add (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e = parse_mult (ex);
  while (true) {
    if (ex.test ("+")) {
      NetTracerLayerExpressionInfo ee = parse_mult (ex);
      e.merge (NetTracerLayerExpression::OPOr, ee);
    } else if (ex.test ("-")) {
      NetTracerLayerExpressionInfo ee = parse_mult (ex);
      e.merge (NetTracerLayerExpression::OPNot, ee);
    } else {
      break;
    }
  }

  return e;
}

NetTracerLayerExpressionInfo 
NetTracerLayerExpressionInfo::parse_mult (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e = parse_atomic (ex);
  while (true) {
    if (ex.test ("*")) {
      NetTracerLayerExpressionInfo ee = parse_atomic (ex);
      e.merge (NetTracerLayerExpression::OPAnd, ee);
    } else if (ex.test ("^")) {
      NetTracerLayerExpressionInfo ee = parse_atomic (ex);
      e.merge (NetTracerLayerExpression::OPXor, ee);
    } else {
      break;
    }
  }

  return e;
}

NetTracerLayerExpressionInfo 
NetTracerLayerExpressionInfo::parse_atomic (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e;
  if (ex.test ("(")) {
    e = parse_add (ex);
    ex.expect (")");
  } else {
    e.m_a.read (ex);
  }
  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::parse (tl::Extractor &ex)
{
  const char *start = ex.skip ();
  NetTracerLayerExpressionInfo e = parse_add (ex);
  e.m_expression = std::string (start, ex.get () - start);
  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::compile (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  const char *start = ex.skip ();
  NetTracerLayerExpressionInfo e = parse_add (ex);
  e.m_expression = std::string (start, ex.get () - start);
  ex.expect_end ();
  return e;
}

NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get_expr (const db::LayerProperties &lp, const db::Layout &layout, const NetTracerTechnologyComponent &tech, const std::set<std::string> &used_symbols) const
{
  for (NetTracerTechnologyComponent::const_symbol_iterator s = tech.begin_symbols (); s != tech.end_symbols (); ++s) {
    if (s->symbol ().log_equal (lp)) {
      std::set<std::string> us = used_symbols;
      if (! us.insert (s->symbol ().to_string ()).second) {
        throw tl::Exception (tl::to_string (QObject::tr ("Recursive expression through symbol %s")), s->symbol ());
      }
      return NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, tech, us);
    }
  }

  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->log_equal (lp)) {
      return new NetTracerLayerExpression ((*l).first);
    }
  }

  return new NetTracerLayerExpression (-1);
}
 
NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get (const db::Layout &layout, const NetTracerTechnologyComponent &tech) const
{
  std::set<std::string> us;
  return get (layout, tech, us);
}

NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get (const db::Layout &layout, const NetTracerTechnologyComponent &tech, const std::set<std::string> &used_symbols) const
{
  NetTracerLayerExpression *e = 0;

  if (mp_a) {
    e = mp_a->get (layout, tech, used_symbols);
  } else {
    e = get_expr (m_a, layout, tech, used_symbols);
  }

  if (m_op != NetTracerLayerExpression::OPNone) {
    if (mp_b) {
      e->merge (m_op, mp_b->get (layout, tech, used_symbols));
    } else {
      e->merge (m_op, get_expr (m_b, layout, tech, used_symbols));
    }
  }

  return e;
}

// -----------------------------------------------------------------------------------
//  NetTracerTechnologyComponent implementation

NetTracerTechnologyComponent::NetTracerTechnologyComponent ()
  : lay::TechnologyComponent (net_tracer_component_name, tl::to_string (QObject::tr ("Connectivity")))
{
  // .. nothing yet ..
}

NetTracerTechnologyComponent::NetTracerTechnologyComponent (const NetTracerTechnologyComponent &d)
  : lay::TechnologyComponent (net_tracer_component_name, tl::to_string (QObject::tr ("Connectivity")))
{
  m_connections = d.m_connections;
  m_symbols = d.m_symbols;
}

NetTracerData
NetTracerTechnologyComponent::get_tracer_data (const db::Layout &layout) const
{
  //  test run on the expressions to verify their syntax
  int n = 1;
  for (NetTracerTechnologyComponent::const_iterator c = begin (); c != end (); ++c, ++n) {
    if (c->layer_a ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Missing first layer specification on connectivity specification #%d")), n);
    }
    if (c->layer_b ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Missing second layer specification on connectivity specification #%d")), n);
    }
  }

  n = 1;
  for (NetTracerTechnologyComponent::const_symbol_iterator s = begin_symbols (); s != end_symbols (); ++s, ++n) {
    if (s->symbol ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Missing symbol name on symbol specification #%d")), n);
    }
    if (s->expression ().empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Missing expression on symbol specification #%d")), n);
    }
    try {
      std::auto_ptr<NetTracerLayerExpression> expr_in (NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, *this));
    } catch (tl::Exception &ex) {
      throw tl::Exception (tl::to_string (QObject::tr ("Error compiling expression '%s' (symbol #%d): %s")), s->expression (), n, ex.msg ());
    }
  }

  NetTracerData data;

  //  register a logical layer for each original one as alias and one for each expression with a new ID
  for (ext::NetTracerTechnologyComponent::const_symbol_iterator s = begin_symbols (); s != end_symbols (); ++s) {
    ext::NetTracerLayerExpression *expr = ext::NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, *this);
    data.register_logical_layer (expr, s->symbol ().to_string ().c_str ());
  }

  for (ext::NetTracerTechnologyComponent::const_iterator c = begin (); c != end (); ++c) {
    data.add_connection (c->get (layout, *this, data));
  }

  return data;
}

lay::TechnologyComponentEditor *
NetTracerTechnologyComponent::create_editor (QWidget *parent) 
{
  return new NetTracerTechComponentEditor (parent);
}

// -----------------------------------------------------------------------------------------
//  NetTracerConnectivityColumnDelegate definition and implementation

class NetTracerConnectivityColumnDelegate
  : public QItemDelegate
{
public:
  NetTracerConnectivityColumnDelegate (QWidget *parent, NetTracerTechnologyComponent *data)
    : QItemDelegate (parent), mp_data (data)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const 
  {
    return new QLineEdit (parent);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {
      int n = index.model ()->data (index, Qt::UserRole).toInt ();
      if (mp_data->size () > size_t (n)) { 
        if (index.column () == 0) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].layer_a ().to_string ()));
        } else if (index.column () == 1) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].via_layer ().to_string ()));
        } else if (index.column () == 2) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].layer_b ().to_string ()));
        }
      }
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {

      int n = model->data (index, Qt::UserRole).toInt ();
      if (mp_data->size () > size_t (n)) { 

        NetTracerLayerExpressionInfo expr;

        std::string text = tl::to_string (editor->text ());
        tl::Extractor ex (text.c_str ());
        bool error = false;
        try {
          expr = NetTracerLayerExpressionInfo::compile (text);
        } catch (...) {
          error = true;
        }

        if (error) {
          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
        } else if ((index.column () == 0 || index.column () == 2) && expr.to_string ().empty ()) {
          model->setData (index, QVariant (QObject::tr ("Enter expression")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
        } else if (index.column () == 1 && expr.to_string ().empty ()) {
          model->setData (index, QVariant (QObject::tr ("None")), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);
        } else {
          model->setData (index, QVariant (tl::to_qstring (expr.to_string ())), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);
        }

        if (index.column () == 0) {
          mp_data->begin () [n].set_layer_a (expr);
        } else if (index.column () == 1) {
          mp_data->begin () [n].set_via_layer (expr);
        } else if (index.column () == 2) {
          mp_data->begin () [n].set_layer_b (expr);
        }

      }

    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const 
  {
    QWidget *editor = createEditor (0, option, index);
    QSize size = editor->sizeHint ();
    delete editor;
    return size - QSize (2, 2);
  }

private:
  NetTracerTechnologyComponent *mp_data;
};

// -----------------------------------------------------------------------------------------
//  NetTracerConnectivitySymbolColumnDelegate definition and implementation

class NetTracerConnectivitySymbolColumnDelegate
  : public QItemDelegate
{
public:
  NetTracerConnectivitySymbolColumnDelegate (QWidget *parent, NetTracerTechnologyComponent *data)
    : QItemDelegate (parent), mp_data (data)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const 
  {
    return new QLineEdit (parent);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {
      int n = index.model ()->data (index, Qt::UserRole).toInt ();
      if (mp_data->symbols () > size_t (n)) { 
        if (index.column () == 0) {
          editor->setText (tl::to_qstring (mp_data->begin_symbols () [n].symbol ().to_string ()));
        } else if (index.column () == 1) {
          editor->setText (tl::to_qstring (mp_data->begin_symbols () [n].expression ()));
        }
      }
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {

      int n = model->data (index, Qt::UserRole).toInt ();
      if (mp_data->symbols () > size_t (n)) { 

        std::string text = tl::to_string (editor->text ());

        if (index.column () == 0 && text.empty ()) {

          model->setData (index, QVariant (QObject::tr ("Enter symbol")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);

        } else if (index.column () == 1 && text.empty ()) {

          model->setData (index, QVariant (QObject::tr ("Enter expression")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);

        } else if (index.column () == 1) {

          bool ok = true;
          //  check the expression
          try {
            NetTracerLayerExpressionInfo::compile (text);
          } catch (tl::Exception &) {
            ok = false;
          }

          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);

          if (! ok) {
            model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
            model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
          } else {
            model->setData (index, QVariant (), Qt::ForegroundRole);
            model->setData (index, QVariant (), Qt::BackgroundRole);
          }

        } else {

          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);

        }

        if (index.column () == 0) {
          db::LayerProperties lp;
          tl::Extractor ex (text.c_str ());
          lp.read (ex);
          mp_data->begin_symbols () [n].set_symbol (lp);
        } else if (index.column () == 1) {
          mp_data->begin_symbols () [n].set_expression (text);
        }

      }

    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const 
  {
    QWidget *editor = createEditor (0, option, index);
    QSize size = editor->sizeHint ();
    delete editor;
    return size - QSize (2, 2);
  }

private:
  NetTracerTechnologyComponent *mp_data;
};

// -----------------------------------------------------------------------------------
//  NetTracerTechComponentEditor implementation

NetTracerTechComponentEditor::NetTracerTechComponentEditor (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  Ui::NetTracerTechComponentEditor::setupUi (this);

  connect (add_conductor_pb, SIGNAL (clicked ()), this, SLOT (add_clicked ()));
  connect (del_conductor_pb, SIGNAL (clicked ()), this, SLOT (del_clicked ()));
  connect (move_conductor_up_pb, SIGNAL (clicked ()), this, SLOT (move_up_clicked ()));
  connect (move_conductor_down_pb, SIGNAL (clicked ()), this, SLOT (move_down_clicked ()));
  connect (add_symbol_pb, SIGNAL (clicked ()), this, SLOT (symbol_add_clicked ()));
  connect (del_symbol_pb, SIGNAL (clicked ()), this, SLOT (symbol_del_clicked ()));
  connect (move_symbol_up_pb, SIGNAL (clicked ()), this, SLOT (symbol_move_up_clicked ()));
  connect (move_symbol_down_pb, SIGNAL (clicked ()), this, SLOT (symbol_move_down_clicked ()));

  lay::activate_help_links (symbol_help_label);
  lay::activate_help_links (help_label);

  connectivity_table->horizontalHeader ()->setHighlightSections (false);
  connectivity_table->horizontalHeader ()->setStretchLastSection (true);
  connectivity_table->verticalHeader ()->hide ();
  symbol_table->horizontalHeader ()->setHighlightSections (false);
  symbol_table->horizontalHeader ()->setStretchLastSection (true);
  symbol_table->verticalHeader ()->hide ();
}

void 
NetTracerTechComponentEditor::commit ()
{
  NetTracerTechnologyComponent *data = dynamic_cast <NetTracerTechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  *data = m_data;
}

void 
NetTracerTechComponentEditor::setup ()
{
  NetTracerTechnologyComponent *data = dynamic_cast <NetTracerTechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  m_data = *data;

  for (int c = 0; c < 3; ++c) {
    if (connectivity_table->itemDelegateForColumn (c) != 0) {
      delete connectivity_table->itemDelegateForColumn (c);
    }
    connectivity_table->setItemDelegateForColumn (c, new NetTracerConnectivityColumnDelegate (connectivity_table, &m_data));
  }

  for (int c = 0; c < 2; ++c) {
    if (symbol_table->itemDelegateForColumn (c) != 0) {
      delete symbol_table->itemDelegateForColumn (c);
    }
    symbol_table->setItemDelegateForColumn (c, new NetTracerConnectivitySymbolColumnDelegate (symbol_table, &m_data));
  }

  update ();
}

void 
NetTracerTechComponentEditor::add_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_conductor_pb->setFocus (); 

  int row = connectivity_table->currentItem () ? connectivity_table->row (connectivity_table->currentItem ()) : -1;
  if (row < 0) {
    m_data.add (NetTracerConnectionInfo ());
    row = int (m_data.size () - 1);
  } else {
    row += 1;
    m_data.insert (m_data.begin () + row, NetTracerConnectionInfo ());
  }

  update ();
  connectivity_table->setCurrentItem (connectivity_table->item (row, 0));
}

void 
NetTracerTechComponentEditor::del_clicked ()
{
  //  removes focus from the tree view - commits the data
  del_conductor_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  connectivity_table->setCurrentIndex (QModelIndex ());

  int offset = 0;
  for (std::set<int>::const_iterator r = selected_rows.begin (); r != selected_rows.end (); ++r) {
    m_data.erase (m_data.begin () + (*r - offset));
    ++offset;
  }

  update ();
}

void 
NetTracerTechComponentEditor::move_up_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_conductor_up_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = connectivity_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  connectivity_table->setCurrentIndex (QModelIndex ());

  int n = 0;
  for (NetTracerTechnologyComponent::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {
    if (selected_rows.find (n + 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin () [n + 1], m_data.begin () [n]);
      selected_rows.erase (n + 1);
      selected_rows.insert (n);
      if (n_current == n + 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerTechComponentEditor::move_down_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_conductor_down_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = connectivity_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  connectivity_table->setCurrentIndex (QModelIndex ());

  int n = m_data.size ();
  for (NetTracerTechnologyComponent::iterator l = m_data.end (); l != m_data.begin (); ) {
    --l;
    --n;
    if (selected_rows.find (n - 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin () [n - 1], m_data.begin () [n]);
      selected_rows.erase (n - 1);
      selected_rows.insert (n);
      if (n_current == n - 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerTechComponentEditor::symbol_add_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_symbol_pb->setFocus (); 

  int row = symbol_table->currentItem () ? symbol_table->row (symbol_table->currentItem ()) : -1;
  if (row < 0) {
    m_data.add_symbol (NetTracerSymbolInfo ());
    row = int (m_data.symbols () - 1);
  } else {
    row += 1;
    m_data.insert_symbol (m_data.begin_symbols () + row, NetTracerSymbolInfo ());
  }

  update ();
  symbol_table->setCurrentItem (symbol_table->item (row, 0));
}

void 
NetTracerTechComponentEditor::symbol_del_clicked ()
{
  //  removes focus from the tree view - commits the data
  del_symbol_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  symbol_table->setCurrentIndex (QModelIndex ());

  int offset = 0;
  for (std::set<int>::const_iterator r = selected_rows.begin (); r != selected_rows.end (); ++r) {
    m_data.erase_symbol (m_data.begin_symbols () + (*r - offset));
    ++offset;
  }

  update ();
}

void 
NetTracerTechComponentEditor::symbol_move_up_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_symbol_up_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = symbol_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  symbol_table->setCurrentIndex (QModelIndex ());

  int n = 0;
  for (NetTracerTechnologyComponent::symbol_iterator l = m_data.begin_symbols (); l != m_data.end_symbols (); ++l, ++n) {
    if (selected_rows.find (n + 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin_symbols () [n + 1], m_data.begin_symbols () [n]);
      selected_rows.erase (n + 1);
      selected_rows.insert (n);
      if (n_current == n + 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerTechComponentEditor::symbol_move_down_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_symbol_down_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = symbol_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  symbol_table->setCurrentIndex (QModelIndex ());

  int n = m_data.symbols ();
  for (NetTracerTechnologyComponent::symbol_iterator l = m_data.end_symbols (); l != m_data.begin_symbols (); ) {
    --l;
    --n;
    if (selected_rows.find (n - 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin_symbols () [n - 1], m_data.begin_symbols () [n]);
      selected_rows.erase (n - 1);
      selected_rows.insert (n);
      if (n_current == n - 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void
NetTracerTechComponentEditor::update ()
{
  QStringList labels;
  int n;

  connectivity_table->clear ();
  connectivity_table->setRowCount (m_data.size ());

  connectivity_table->setColumnCount (3);
  labels.clear ();
  labels << QObject::tr ("Conductor 1");
  labels << QObject::tr ("Via (optional)");
  labels << QObject::tr ("Conductor 2");
  connectivity_table->setHorizontalHeaderLabels (labels);

  n = 0;
  for (NetTracerTechnologyComponent::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {

    for (int c = 0; c < 3; ++c) {

      QTableWidgetItem *item = new QTableWidgetItem ();
      item->setFlags (item->flags () | Qt::ItemIsEditable);
      connectivity_table->setItem (n, c, item);

      item->setData (Qt::ForegroundRole, QVariant ());

      if (c == 0) {
        if (l->layer_a ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter layer")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->layer_a ().to_string ())));
        }
      } else if (c == 1) {
        if (l->via_layer ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("None")));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->via_layer ().to_string ())));
        }
      } else if (c == 2) {
        if (l->layer_b ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter layer")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->layer_b ().to_string ())));
        }
      }

      item->setData (Qt::UserRole, QVariant (n));

    }

  }

  connectivity_table->clearSelection ();

  symbol_table->clear ();
  symbol_table->setRowCount (m_data.symbols ());

  symbol_table->setColumnCount (2);
  labels.clear ();
  labels << QObject::tr ("Symbol");
  labels << QObject::tr ("Expression");
  symbol_table->setHorizontalHeaderLabels (labels);

  n = 0;
  for (NetTracerTechnologyComponent::symbol_iterator l = m_data.begin_symbols (); l != m_data.end_symbols (); ++l, ++n) {

    for (int c = 0; c < 2; ++c) {

      QTableWidgetItem *item = new QTableWidgetItem ();
      item->setFlags (item->flags () | Qt::ItemIsEditable);
      symbol_table->setItem (n, c, item);

      item->setData (Qt::ForegroundRole, QVariant ());
      item->setData (Qt::BackgroundRole, QVariant ());

      if (c == 0) {

        if (l->symbol ().log_equal (db::LayerProperties ())) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter symbol")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->symbol ().to_string ())));
        }

      } else if (c == 1) {

        if (l->expression ().empty ()) {

          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter expression")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));

        } else {

          bool ok = true;
          //  check the expression
          try {
            NetTracerLayerExpressionInfo::compile (l->expression ());
          } catch (tl::Exception &) {
            ok = false;
          }

          if (! ok) {
            item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
            item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
          }

          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->expression ())));

        }

      }

      item->setData (Qt::UserRole, QVariant (n));

    }

  }

  symbol_table->clearSelection ();
}

// -----------------------------------------------------------------------------------
//  Net implementation

Net::Net ()
  : m_dbu (0.001), m_incomplete (true), m_trace_path (false)
{
  //  .. nothing yet ..
}

Net::Net (const NetTracer &tracer, const db::ICplxTrans &trans, const db::Layout &layout, db::cell_index_type cell_index, const std::string &layout_filename, const std::string &layout_name, const NetTracerData &data)
  : m_name (tracer.name ()), m_incomplete (tracer.incomplete ())
{
  m_dbu = layout.dbu ();
  m_top_cell_name = layout.cell_name (cell_index);
  m_layout_filename = layout_filename;
  m_layout_name = layout_name;

  size_t n = 0;
  for (NetTracer::iterator s = tracer.begin (); s != tracer.end (); ++s) {
    ++n;
  }
  m_net_shapes.reserve (n);

  for (NetTracer::iterator s = tracer.begin (); s != tracer.end (); ++s) {

    //  TODO: should reset propery ID:
    tl::ident_map<db::properties_id_type> pm;
    db::Shape new_shape = m_shapes.insert (s->shape (), trans, pm);
    m_net_shapes.push_back (*s);
    m_net_shapes.back ().shape (new_shape);

    if (m_cell_names.find (s->cell_index ()) == m_cell_names.end ()) {
      m_cell_names.insert (std::make_pair (s->cell_index (), layout.cell_name (s->cell_index ())));
    }

    if (m_layers.find (s->layer ()) == m_layers.end ()) {

      unsigned int l = s->layer ();
      db::LayerProperties lp;
      db::LayerProperties lprep;

      if (layout.is_valid_layer (l)) {

        lp = layout.get_properties (l);
        lprep = lp;

      } else {

        int lrep = data.expression (l).representative_layer ();
        if (layout.is_valid_layer (lrep)) {
          lprep = layout.get_properties (lrep);
        }

        for (std::map<std::string, unsigned int>::const_iterator sy = data.symbols ().begin (); sy != data.symbols ().end (); ++sy) {
          if (sy->second == l) {
            tl::Extractor ex (sy->first.c_str ());
            lp.read (ex);
            break;
          }
        }

      }

      define_layer (l, lp, lprep);

    }

  }
}

std::vector<unsigned int>
Net::export_net (db::Layout &layout, db::Cell &export_cell)
{
  std::vector<unsigned int> new_layers;
  std::map<unsigned int, unsigned int> layer_map;

  for (iterator net_shape = begin (); net_shape != end (); ++net_shape) {

    if (net_shape->is_pseudo ()) {
      continue;
    }

    std::map<unsigned int, unsigned int>::const_iterator lm = layer_map.find (net_shape->layer ());
    if (lm == layer_map.end ()) {

      int layer_index = -1;
      for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
        if ((*l).second->log_equal (representative_layer_for (net_shape->layer ()))) {
          layer_index = int ((*l).first);
          break;
        }
      }

      if (layer_index < 0) {
        layer_index = int (layout.insert_layer (representative_layer_for (net_shape->layer ())));
        new_layers.push_back (layer_index);
      }

      lm = layer_map.insert (std::make_pair (net_shape->layer (), (unsigned int)layer_index)).first;

    }

    tl::ident_map<db::properties_id_type> pm;
    export_cell.shapes (lm->second).insert (net_shape->shape (), db::ICplxTrans (net_shape->trans ()), pm);

  }

  return new_layers;
}

const std::string &
Net::cell_name (db::cell_index_type cell_index) const
{
  std::map <unsigned int, std::string>::const_iterator cn = m_cell_names.find (cell_index);
  if (cn != m_cell_names.end ()) {
    return cn->second;
  } else {
    static std::string n;
    return n;
  }
}

db::LayerProperties 
Net::representative_layer_for (unsigned int log_layer) const
{
  std::map <unsigned int, std::pair <db::LayerProperties, db::LayerProperties> >::const_iterator l = m_layers.find (log_layer);
  if (l != m_layers.end ()) {
    return l->second.second;
  } else {
    return db::LayerProperties ();
  }
}

db::LayerProperties 
Net::layer_for (unsigned int log_layer) const
{
  std::map <unsigned int, std::pair <db::LayerProperties, db::LayerProperties> >::const_iterator l = m_layers.find (log_layer);
  if (l != m_layers.end ()) {
    return l->second.first;
  } else {
    return db::LayerProperties ();
  }
}

void 
Net::define_layer (unsigned int l, const db::LayerProperties &lp, const db::LayerProperties &lp_representative)
{
  m_layers.insert (std::make_pair (l, std::make_pair (lp, lp_representative)));
}

}

