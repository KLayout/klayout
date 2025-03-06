
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

#include "edtPCellParametersPage.h"
#include "edtPropertiesPageUtils.h"
#include "edtConfig.h"
#include "layWidgets.h"
#include "layQtTools.h"
#include "layLayoutViewBase.h"
#include "layDispatcher.h"
#include "layBusy.h"
#include "tlScriptError.h"

#include <QFrame>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QScrollBar>
#include <QScrollArea>
#include <QToolButton>

namespace edt
{

static void set_value (const db::PCellParameterDeclaration &p, QWidget *widget, const tl::Variant &value)
{
  if (p.get_choices ().empty ()) {

    switch (p.get_type ()) {
      
    case db::PCellParameterDeclaration::t_int:
      {
        QLineEdit *le = dynamic_cast<QLineEdit *> (widget);
        if (le) {
          le->blockSignals (true);
          le->setText (value.cast<int> ().to_qstring ());
          le->blockSignals (false);
        }
      }
      break;

    case db::PCellParameterDeclaration::t_double:
      {
        QLineEdit *le = dynamic_cast<QLineEdit *> (widget);
        if (le) {
          le->blockSignals (true);
          le->setText (value.cast<double> ().to_qstring ());
          le->blockSignals (false);
        }
      }
      break;

    case db::PCellParameterDeclaration::t_string:
      {
        QLineEdit *le = dynamic_cast<QLineEdit *> (widget);
        if (le) {
          le->blockSignals (true);
          le->setText (value.to_qstring ());
          le->blockSignals (false);
        }
      }
      break;

    case db::PCellParameterDeclaration::t_list:
      {
        QLineEdit *le = dynamic_cast<QLineEdit *> (widget);
        if (le) {
          le->blockSignals (true);
          le->setText (value.to_qstring ());
          le->blockSignals (false);
        }
      }
      break;

    case db::PCellParameterDeclaration::t_layer:
      {
        lay::LayerSelectionComboBox *ly = dynamic_cast<lay::LayerSelectionComboBox *> (widget);
        if (ly) {

          db::LayerProperties lp;
          if (value.is_user<db::LayerProperties> ()) {
            lp = value.to_user<db::LayerProperties> ();
          } else if (value.is_nil ()) {
            //  empty LayerProperties
          } else {
            std::string s = value.to_string ();
            tl::Extractor ex (s.c_str ());
            lp.read (ex);
          }

          ly->blockSignals (true);
          ly->set_current_layer (lp);
          ly->blockSignals (false);

        }
      }
      break;

    case db::PCellParameterDeclaration::t_boolean:
      {
        QCheckBox *cbx = dynamic_cast<QCheckBox *> (widget);
        if (cbx) {
          cbx->blockSignals (true);
          cbx->setChecked (value.to_bool ());
          cbx->blockSignals (false);
        }
      }
      break;

    default:
      break;
    }

  } else {

    QComboBox *cb = dynamic_cast<QComboBox *> (widget);
    if (cb) {
      int i = 0;
      for (std::vector<tl::Variant>::const_iterator c = p.get_choices ().begin (); c != p.get_choices ().end (); ++c, ++i) {
        if (*c == value) {
          cb->blockSignals (true);
          cb->setCurrentIndex (i);
          cb->blockSignals (false);
        }
      }
    }

  }
}

PCellParametersPage::PCellParametersPage (QWidget *parent, lay::Dispatcher *dispatcher, bool dense)
  : QFrame (parent), mp_dispatcher (dispatcher), m_dense (dense), m_show_parameter_names (false), m_lazy_evaluation (-1), dm_parameter_changed (this, &PCellParametersPage::do_parameter_changed)
{
  if (mp_dispatcher) {
    mp_dispatcher->config_get (cfg_edit_pcell_show_parameter_names, m_show_parameter_names);
    mp_dispatcher->config_get (cfg_edit_pcell_lazy_eval_mode, m_lazy_evaluation);
  }

  init ();
}

void
PCellParametersPage::init ()
{
  QPalette palette;
  QFont font;

  mp_pcell_decl.reset (0);
  mp_view = 0;
  m_cv_index = 0;
  mp_parameters_area = 0;

  QGridLayout *frame_layout = new QGridLayout (this);
  //  spacing and margin for tool windows
  frame_layout->setContentsMargins (0, 0, 0, 0);
  frame_layout->setHorizontalSpacing (0);
  frame_layout->setVerticalSpacing (0);
  setLayout (frame_layout);

  mp_update_frame = new QFrame (this);
  mp_update_frame->setFrameShape (QFrame::NoFrame);
  frame_layout->addWidget (mp_update_frame, 0, 0, 1, 1);

  QGridLayout *update_frame_layout = new QGridLayout (mp_update_frame);
  mp_update_frame->setLayout (update_frame_layout);
  if (m_dense) {
    update_frame_layout->setContentsMargins (4, 4, 4, 4);
    update_frame_layout->setHorizontalSpacing (6);
    update_frame_layout->setVerticalSpacing (2);
  }

  mp_changed_icon = new QLabel (mp_update_frame);
  mp_changed_icon->setPixmap (QPixmap (":/warn_16px@2x.png"));
  update_frame_layout->addWidget (mp_changed_icon, 0, 0, 1, 1);

  mp_update_button = new QToolButton (mp_update_frame);
  mp_update_button->setText (tr ("Update"));
  connect (mp_update_button, SIGNAL (clicked()), this, SLOT (update_button_pressed ()));
  update_frame_layout->addWidget (mp_update_button, 0, 1, 1, 1);

  mp_changed_label = new QLabel (mp_update_frame);
  mp_changed_label->setText (tr ("Update needed"));
  update_frame_layout->addWidget (mp_changed_label, 0, 2, 1, 1);

  update_frame_layout->setColumnStretch (2, 1);

  mp_error_frame = new QFrame (this);
  mp_error_frame->setFrameShape (QFrame::NoFrame);
  frame_layout->addWidget (mp_error_frame, 1, 0, 1, 1);

  QGridLayout *error_frame_layout = new QGridLayout (mp_error_frame);
  mp_error_frame->setLayout (error_frame_layout);
  if (m_dense) {
    error_frame_layout->setContentsMargins (4, 4, 4, 4);
    error_frame_layout->setHorizontalSpacing (6);
    error_frame_layout->setVerticalSpacing (2);
  }

  mp_error_icon = new QLabel (mp_error_frame);
  mp_error_icon->setPixmap (QPixmap (":/warn_16px@2x.png"));
  error_frame_layout->addWidget (mp_error_icon, 1, 0, 1, 1);

  mp_error_label = new QLabel (mp_error_frame);
  mp_error_label->setWordWrap (true);
  palette = mp_error_label->palette ();
  palette.setColor (QPalette::WindowText, Qt::red);
  mp_error_label->setPalette (palette);
  font = mp_error_label->font ();
  font.setBold (true);
  mp_error_label->setFont (font);
  error_frame_layout->addWidget (mp_error_label, 1, 1, 1, 2);

  error_frame_layout->setColumnStretch (2, 1);

  QFrame *options_frame = new QFrame (this);
  options_frame->setFrameShape (QFrame::NoFrame);
  frame_layout->addWidget (options_frame, 3, 0, 1, 1);

  QHBoxLayout *options_frame_layout = new QHBoxLayout (options_frame);
  options_frame->setLayout (options_frame_layout);
  if (m_dense) {
    options_frame_layout->setContentsMargins (4, 4, 4, 4);
  }

  QToolButton *dot_menu_button = new QToolButton (options_frame);
  dot_menu_button->setText (tr ("Options "));
  dot_menu_button->setAutoRaise (true);
  dot_menu_button->setPopupMode (QToolButton::InstantPopup);
  dot_menu_button->setToolButtonStyle (Qt::ToolButtonTextOnly);
  options_frame_layout->addWidget (dot_menu_button);
  options_frame_layout->addStretch ();

  QMenu *dot_menu = new QMenu (dot_menu_button);
  dot_menu_button->setMenu (dot_menu);
  mp_show_parameter_names_action = new QAction (dot_menu);
  dot_menu->addAction (mp_show_parameter_names_action);
  mp_show_parameter_names_action->setText (tr ("Show parameter names"));
  mp_show_parameter_names_action->setCheckable (true);
  mp_show_parameter_names_action->setChecked (m_show_parameter_names);
  connect (mp_show_parameter_names_action, SIGNAL (triggered (bool)), this, SLOT (show_parameter_names (bool)));

  QMenu *lazy_eval_menu = new QMenu (dot_menu);
  lazy_eval_menu->setTitle (tr ("Lazy PCell evaluation"));
  dot_menu->addMenu (lazy_eval_menu);

  mp_auto_lazy_eval_action = new QAction (lazy_eval_menu);
  lazy_eval_menu->addAction (mp_auto_lazy_eval_action);
  mp_auto_lazy_eval_action->setText (tr ("As requested by PCell"));
  mp_auto_lazy_eval_action->setCheckable (true);
  mp_auto_lazy_eval_action->setChecked (m_lazy_evaluation < 0);
  connect (mp_auto_lazy_eval_action, SIGNAL (triggered ()), this, SLOT (lazy_eval_mode_slot ()));

  mp_always_lazy_eval_action = new QAction (lazy_eval_menu);
  lazy_eval_menu->addAction (mp_always_lazy_eval_action);
  mp_always_lazy_eval_action->setText (tr ("Always"));
  mp_always_lazy_eval_action->setCheckable (true);
  mp_always_lazy_eval_action->setChecked (m_lazy_evaluation > 0);
  connect (mp_always_lazy_eval_action, SIGNAL (triggered ()), this, SLOT (lazy_eval_mode_slot ()));

  mp_never_lazy_eval_action = new QAction (lazy_eval_menu);
  lazy_eval_menu->addAction (mp_never_lazy_eval_action);
  mp_never_lazy_eval_action->setText (tr ("Never"));
  mp_never_lazy_eval_action->setCheckable (true);
  mp_never_lazy_eval_action->setChecked (m_lazy_evaluation == 0);
  connect (mp_never_lazy_eval_action, SIGNAL (triggered ()), this, SLOT (lazy_eval_mode_slot ()));
}

bool
PCellParametersPage::lazy_evaluation ()
{
  if (m_lazy_evaluation < 0) {
    return mp_pcell_decl.get () && mp_pcell_decl->wants_lazy_evaluation ();
  } else {
    return m_lazy_evaluation > 0;
  }
}

void
PCellParametersPage::lazy_eval_mode_slot ()
{
  if (sender () == mp_always_lazy_eval_action) {
    lazy_eval_mode (1);
  } else if (sender () == mp_never_lazy_eval_action) {
    lazy_eval_mode (0);
  } else if (sender () == mp_auto_lazy_eval_action) {
    lazy_eval_mode (-1);
  }
}

void
PCellParametersPage::lazy_eval_mode (int mode)
{
  if (mode == m_lazy_evaluation) {
    return;
  }

  mp_never_lazy_eval_action->setChecked (mode == 0);
  mp_always_lazy_eval_action->setChecked (mode > 0);
  mp_auto_lazy_eval_action->setChecked (mode < 0);

  m_lazy_evaluation = mode;

  if (mp_dispatcher) {
    mp_dispatcher->config_set (cfg_edit_pcell_lazy_eval_mode, m_lazy_evaluation);
  }

  setup (mp_view, m_cv_index, mp_pcell_decl.get (), get_parameters ());
}

void
PCellParametersPage::show_parameter_names (bool f)
{
  if (m_show_parameter_names == f) {
    return;
  }

  m_show_parameter_names = f;
  mp_show_parameter_names_action->setChecked (f);

  if (mp_dispatcher) {
    mp_dispatcher->config_set (cfg_edit_pcell_show_parameter_names, m_show_parameter_names);
  }

  setup (mp_view, m_cv_index, mp_pcell_decl.get (), get_parameters ());
}

void
PCellParametersPage::setup (lay::LayoutViewBase *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters)
{
  mp_pcell_decl.reset (const_cast<db::PCellDeclaration *> (pcell_decl));  //  no const weak_ptr ...
  mp_view = view;
  m_cv_index = cv_index;
  m_states = db::ParameterStates ();
  m_initial_states = db::ParameterStates ();

  if (mp_parameters_area) {
    delete mp_parameters_area;
  }

  m_widgets.clear ();
  m_icon_widgets.clear ();
  m_all_widgets.clear ();

  mp_parameters_area = new QScrollArea (this);
  mp_parameters_area->setFrameShape (QFrame::NoFrame);
  mp_parameters_area->setWidgetResizable (true);
  QGridLayout *frame_layout = dynamic_cast<QGridLayout *> (QFrame::layout ());
  frame_layout->addWidget (mp_parameters_area, 2, 0, 1, 1);
  frame_layout->setRowStretch (2, 1);

  mp_main_frame = new QFrame (mp_parameters_area);
  mp_main_frame->setFrameShape (QFrame::NoFrame);
  setFrameShape (QFrame::NoFrame);

  QGridLayout *main_grid = new QGridLayout (mp_main_frame);
  mp_main_frame->setLayout (main_grid);
  if (m_dense) {
    main_grid->setContentsMargins (4, 4, 4, 4);
    main_grid->setHorizontalSpacing (6);
    main_grid->setVerticalSpacing (2);
  }

  if (! mp_pcell_decl) {
    mp_parameters_area->setWidget (mp_main_frame);
    update_current_parameters ();
    return;
  }

  QWidget *inner_frame = mp_main_frame;
  QGridLayout *inner_grid = main_grid;

  int main_row = 0;
  int row = 0;
  std::string group_title;

  int r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {

    tl::Variant value;
    if (r < int (parameters.size ())) {
      value = parameters [r];
    } else {
      value = p->get_default ();
    }

    db::ParameterState &ps = m_states.parameter (p->get_name ());
    ps.set_value (value);
    ps.set_readonly (p->is_readonly ());
    ps.set_visible (! p->is_hidden ());

    m_all_widgets.push_back (std::vector<QWidget *> ());

    if (p->get_type () == db::PCellParameterDeclaration::t_shape) {
      m_widgets.push_back (0);
      m_icon_widgets.push_back (0);
      continue;
    }

    std::string gt, description;
    size_t tab = p->get_description ().find ("\t");
    if (tab != std::string::npos) {
      gt = std::string (p->get_description (), 0, tab);
      description = std::string (p->get_description (), tab + 1, std::string::npos);
    } else {
      description = p->get_description ();
    }

    if (gt != group_title) {

      if (! gt.empty ()) {

        //  create a new group
        QGroupBox *gb = new QGroupBox (mp_main_frame);
        mp_groups.push_back (gb);
        gb->setTitle (tl::to_qstring (gt));
        main_grid->addWidget (gb, main_row, 0, 1, 3);
        
        inner_grid = new QGridLayout (gb);
        if (m_dense) {
          inner_grid->setContentsMargins (4, 4, 4, 4);
          inner_grid->setHorizontalSpacing (6);
          inner_grid->setVerticalSpacing (2);
        }
        gb->setLayout (inner_grid);
        inner_frame = gb;

        row = 0;
        ++main_row;

      } else {

        //  back to the main group
        inner_grid = main_grid;
        inner_frame = mp_main_frame;
        row = main_row;

      }

      group_title = gt;

    } 

    QLabel *icon_label = new QLabel (QString (), inner_frame);
    inner_grid->addWidget (icon_label, row, 0);
    m_icon_widgets.push_back (icon_label);
    m_all_widgets.back ().push_back (icon_label);

    std::string range;

    if (! p->min_value ().is_nil () || ! p->max_value ().is_nil ()) {
      range = tl::sprintf (
                " [%s, %s]" ,
                p->min_value ().is_nil () ? "-\u221e" /*infinity*/ : p->min_value ().to_string (),
                p->max_value ().is_nil () ? "\u221e"  /*infinity*/ : p->max_value ().to_string ()
              );
    }

    if (p->get_type () != db::PCellParameterDeclaration::t_callback) {

      std::string leader;
      if (m_show_parameter_names) {
        leader = tl::sprintf ("[%s] ", p->get_name ());
      }

      QLabel *l = new QLabel (tl::to_qstring (leader + description + range), inner_frame);
      inner_grid->addWidget (l, row, 1);
      m_all_widgets.back ().push_back (l);

    } else if (m_show_parameter_names) {

      QLabel *l = new QLabel (tl::to_qstring (tl::sprintf ("[%s]", p->get_name ())), inner_frame);
      inner_grid->addWidget (l, row, 1);
      m_all_widgets.back ().push_back (l);

    }

    if (p->get_choices ().empty ()) {

      switch (p->get_type ()) {
        
      case db::PCellParameterDeclaration::t_int:
      case db::PCellParameterDeclaration::t_double:
        {
          QFrame *f = new QFrame (inner_frame);
          QHBoxLayout *hb = new QHBoxLayout (f);
          hb->setContentsMargins (0, 0, 0, 0);
          f->setLayout (hb);
          f->setFrameShape (QFrame::NoFrame);
          QSizePolicy sp = f->sizePolicy ();
          sp.setHorizontalStretch (1);
          f->setSizePolicy (sp);

          QLineEdit *le = new QLineEdit (f);
          hb->addWidget (le);
          le->setMaximumWidth (150);
          le->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (le);

          if (! p->get_unit ().empty ()) {
            QLabel *ul = new QLabel (f);
            hb->addWidget (ul, 1);
            ul->setText (tl::to_qstring (p->get_unit ()));
          }

          hb->addStretch (1);

          inner_grid->addWidget (f, row, 2);
          m_all_widgets.back ().push_back (f);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_callback:
        {
          QPushButton *pb = new QPushButton (inner_frame);
          pb->setObjectName (tl::to_qstring (p->get_name ()));
          pb->setText (tl::to_qstring (description));
          QSizePolicy sp = pb->sizePolicy ();
          sp.setHorizontalPolicy (QSizePolicy::Fixed);
          sp.setHorizontalStretch (1);
          pb->setSizePolicy (sp);
          m_widgets.push_back (pb);

          inner_grid->addWidget (pb, row, 2);
          m_all_widgets.back ().push_back (pb);

          connect (pb, SIGNAL (clicked ()), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_string:
      case db::PCellParameterDeclaration::t_shape:
      case db::PCellParameterDeclaration::t_list:
        {
          QLineEdit *le = new QLineEdit (inner_frame);
          le->setObjectName (tl::to_qstring (p->get_name ()));
          QSizePolicy sp = le->sizePolicy ();
          sp.setHorizontalStretch (1);
          le->setSizePolicy (sp);
          m_widgets.push_back (le);
          inner_grid->addWidget (le, row, 2);
          m_all_widgets.back ().push_back (le);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_layer:
        {
          QFrame *f = new QFrame (inner_frame);
          QHBoxLayout *hb = new QHBoxLayout (f);
          hb->setContentsMargins (0, 0, 0, 0);
          f->setLayout (hb);
          f->setFrameShape (QFrame::NoFrame);
          QSizePolicy sp = f->sizePolicy ();
          sp.setHorizontalStretch (1);
          f->setSizePolicy (sp);

          lay::LayerSelectionComboBox *ly = new lay::LayerSelectionComboBox (f);
          hb->addWidget (ly);
          ly->set_no_layer_available (true);
          ly->set_view (mp_view, m_cv_index, true /*all layers*/);
          ly->setObjectName (tl::to_qstring (p->get_name ()));
          sp = ly->sizePolicy ();
          sp.setHorizontalPolicy (QSizePolicy::Fixed);
          ly->setSizePolicy (sp);
          m_widgets.push_back (ly);

          hb->addStretch (1);

          inner_grid->addWidget (f, row, 2);
          m_all_widgets.back ().push_back (f);

          connect (ly, SIGNAL (activated (int)), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_boolean:
        {
          QCheckBox *cbx = new QCheckBox (inner_frame);
          //  this makes the checkbox not stretch over the full width - better when navigating with tab
          QSizePolicy sp = cbx->sizePolicy ();
          sp.setHorizontalStretch (1);
          cbx->setSizePolicy (sp);
          cbx->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (cbx);
          inner_grid->addWidget (cbx, row, 2);
          m_all_widgets.back ().push_back (cbx);

          connect (cbx, SIGNAL (stateChanged (int)), this, SLOT (parameter_changed ()));
        }
        break;

      default:
        m_widgets.push_back (0);
        break;
      }

    } else {

      QFrame *f = new QFrame (inner_frame);
      QHBoxLayout *hb = new QHBoxLayout (f);
      hb->setContentsMargins (0, 0, 0, 0);
      f->setLayout (hb);
      f->setFrameShape (QFrame::NoFrame);
      QSizePolicy sp = f->sizePolicy ();
      sp.setHorizontalStretch (1);
      f->setSizePolicy (sp);

      QComboBox *cb = new QComboBox (f);
      hb->addWidget (cb);
      cb->setObjectName (tl::to_qstring (p->get_name ()));
      cb->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred);
      cb->setSizeAdjustPolicy (QComboBox::AdjustToContents);

      int i = 0;
      for (std::vector<tl::Variant>::const_iterator c = p->get_choices ().begin (); c != p->get_choices ().end (); ++c, ++i) {
        if (i < int (p->get_choice_descriptions ().size ())) {
          cb->addItem (tl::to_qstring (p->get_choice_descriptions () [i]));
        } else {
          cb->addItem (tl::to_qstring (c->to_string ()));
        }
      }

      connect (cb, SIGNAL (activated (int)), this, SLOT (parameter_changed ()));

      m_widgets.push_back (cb);

      hb->addStretch (1);

      inner_grid->addWidget (f, row, 2);
      m_all_widgets.back ().push_back (f);

    }

    ++row;
    if (inner_frame == mp_main_frame) {
      ++main_row;
    }

  }

  //  adds some default buffer space
  main_grid->setRowStretch (main_row, 1);

  //  initial callback

  try {
    mp_pcell_decl->callback (mp_view->cellview (m_cv_index)->layout (), std::string (), m_states);
  } catch (tl::Exception &ex) {
    //  potentially caused by script errors in callback implementation
    tl::error << ex.msg ();
  } catch (std::runtime_error &ex) {
    tl::error << ex.what ();
  } catch (...) {
    //  ignore other errors
  }

  m_initial_states = m_states;
  mp_error_frame->hide ();

  update_widgets_from_states (m_states, lazy_evaluation ());

  mp_parameters_area->setWidget (mp_main_frame);
  mp_main_frame->show ();

  update_current_parameters ();
}

PCellParametersPage::State
PCellParametersPage::get_state ()
{
  State s;

  s.valid = true;
  s.vScrollPosition = mp_parameters_area->verticalScrollBar ()->value ();
  s.hScrollPosition = mp_parameters_area->horizontalScrollBar ()->value ();

  if (focusWidget ()) {
    s.focusWidget = focusWidget ()->objectName ();
  }

  return s;
}

void
PCellParametersPage::set_state (const State &s)
{
  if (s.valid) {

    mp_parameters_area->verticalScrollBar ()->setValue (s.vScrollPosition);
    mp_parameters_area->horizontalScrollBar ()->setValue (s.hScrollPosition);

    if (! s.focusWidget.isEmpty ()) {
      QWidget *c = findChild<QWidget *> (s.focusWidget);
      if (c) {
        c->setFocus ();
      }
    }

  }
}

void
PCellParametersPage::parameter_changed ()
{
  if (! mp_pcell_decl) {
    return;
  }
  if (! mp_view->cellview (m_cv_index).is_valid ()) {
    return;
  }
  if (lay::BusySection::is_busy ()) {
    //  ignore events for example during debugger execution
    return;
  }

  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  const db::PCellParameterDeclaration *pd = 0;
  for (auto w = m_widgets.begin (); w != m_widgets.end (); ++w) {
    if (*w == sender ()) {
      pd = &pcp [w - m_widgets.begin ()];
      break;
    }
  }

  try {

    db::ParameterStates states = m_states;

    bool edit_error = false;
    //  Silent and without coerce - this will be done later in do_parameter_changed().
    //  This is just about providing the inputs for the callback.
    get_parameters_internal (states, edit_error);

    //  Note: checking for is_busy prevents callbacks during debugger execution
    if (! edit_error) {
      mp_pcell_decl->callback (mp_view->cellview (m_cv_index)->layout (), pd ? pd->get_name () : std::string (), states);
      m_states = states;
    }

  } catch (tl::Exception &ex) {
    //  potentially caused by script errors in callback implementation
    tl::error << ex.msg ();
  } catch (std::runtime_error &ex) {
    tl::error << ex.what ();
  } catch (...) {
    //  ignore other errors
  }

  dm_parameter_changed ();
}

void
PCellParametersPage::deleteLater ()
{
  dm_parameter_changed.cancel ();
  QFrame::deleteLater ();
}

void
PCellParametersPage::do_parameter_changed ()
{
  bool ok = true;
  db::ParameterStates states = m_states;
  get_parameters (states, &ok);   //  includes coerce
  if (ok) {
    update_widgets_from_states (states, lazy_evaluation ());
    if (! lazy_evaluation ()) {
      emit edited ();
    }
  }
}

void
PCellParametersPage::update_button_pressed ()
{
  if (update_current_parameters ()) {
    emit edited ();
  }
}

bool
PCellParametersPage::update_current_parameters ()
{
  bool ok = true;
  db::ParameterStates states = m_states;
  get_parameters (states, &ok);   //  includes coerce
  if (ok) {
    m_current_states = states;
    mp_update_frame->hide ();
  }

  return ok;
}

void
PCellParametersPage::get_parameters_internal (db::ParameterStates &states, bool &edit_error)
{
  edit_error = false;

  int r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {

    db::ParameterState &ps = states.parameter (p->get_name ());

    if (! ps.is_visible () || ! ps.is_enabled () || ps.is_readonly () || p->get_type () == db::PCellParameterDeclaration::t_shape) {
      continue;
    }

    if (p->get_choices ().empty ()) {

      switch (p->get_type ()) {

      case db::PCellParameterDeclaration::t_int:
        {
          QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
          if (le) {

            try {

              int v = 0;
              tl::from_string_ext (tl::to_string (le->text ()), v);

              ps.set_value (tl::Variant (v));
              lay::indicate_error (le, (tl::Exception *) 0);

              check_range(tl::Variant (v), *p);

            } catch (tl::Exception &ex) {

              lay::indicate_error (le, &ex);
              edit_error = true;

            }

          }
        }
        break;

      case db::PCellParameterDeclaration::t_double:
        {
          QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
          if (le) {

            try {

              double v = 0;
              tl::from_string_ext (tl::to_string (le->text ()), v);

              ps.set_value (tl::Variant (v));
              lay::indicate_error (le, (tl::Exception *) 0);

              check_range(tl::Variant (v), *p);

            } catch (tl::Exception &ex) {

              lay::indicate_error (le, &ex);
              edit_error = true;

            }

          }
        }
        break;

      case db::PCellParameterDeclaration::t_string:
        {
          QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
          if (le) {
            ps.set_value (tl::Variant (tl::to_string (le->text ())));
          }
        }
        break;

      case db::PCellParameterDeclaration::t_list:
        {
          QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
          if (le) {
            std::vector<std::string> values = tl::split (tl::to_string (le->text ()), ",");
            ps.set_value (tl::Variant (values.begin (), values.end ()));
          }
        }
        break;

      case db::PCellParameterDeclaration::t_layer:
        {
          lay::LayerSelectionComboBox *ly = dynamic_cast<lay::LayerSelectionComboBox *> (m_widgets [r]);
          if (ly) {
            ps.set_value (tl::Variant (ly->current_layer_props ()));
          }
        }
        break;
      case db::PCellParameterDeclaration::t_boolean:
        {
          QCheckBox *cbx = dynamic_cast<QCheckBox *> (m_widgets [r]);
          if (cbx) {
            ps.set_value (tl::Variant (cbx->isChecked ()));
          }
        }
        break;

      default:
        break;
      }

    } else {

      QComboBox *cb = dynamic_cast<QComboBox*> (m_widgets [r]);
      if (cb && cb->currentIndex () >= 0 && cb->currentIndex () < int (p->get_choices ().size ())) {
        ps.set_value (p->get_choices () [cb->currentIndex ()]);
      }

    }

  }
}

void
PCellParametersPage::get_parameters (db::ParameterStates &states, bool *ok)
{
  try {

    if (! mp_pcell_decl) {
      throw tl::Exception (tl::to_string (tr ("PCell no longer valid.")));
    }

    bool edit_error = false;
    mp_error_frame->hide ();

    get_parameters_internal (states, edit_error);

    if (edit_error) {
      throw tl::Exception (tl::to_string (tr ("There are errors. See the highlighted edit fields for details.")));
    }

    //  coerces the parameters and writes the changed values back
    if (mp_view->cellview (m_cv_index).is_valid ()) {

      auto parameters = parameter_from_states (states);
      auto before_coerce = parameters;
      mp_pcell_decl->coerce_parameters (mp_view->cellview (m_cv_index)->layout (), parameters);

      if (parameters != before_coerce) {
        states_from_parameters (states, parameters);
        set_parameters_internal (states, lazy_evaluation ());
      }

    }

    if (ok) {
      *ok = true;
    }

  } catch (tl::ScriptError &ex) {

    if (ok) {
      mp_error_label->setText (tl::to_qstring (ex.basic_msg ()));
      mp_error_label->setToolTip (tl::to_qstring (ex.msg ()));
      mp_error_frame->show ();
      *ok = false;
    } else {
      throw;
    }

  } catch (tl::Exception &ex) {

    if (ok) {
      mp_error_label->setText (tl::to_qstring (ex.msg ()));
      mp_error_frame->show ();
      *ok = false;
    } else {
      throw;
    }

  }
}

std::vector<tl::Variant>
PCellParametersPage::get_parameters (bool *ok)
{
  db::ParameterStates states = m_states;
  get_parameters (states, ok);

  return parameter_from_states (states);
}

void
PCellParametersPage::set_parameters (const std::vector<tl::Variant> &parameters)
{
  if (! mp_pcell_decl) {
    return;
  }

  states_from_parameters (m_states, parameters);

  try {
    if (mp_view->cellview (m_cv_index).is_valid ()) {
      mp_pcell_decl->callback (mp_view->cellview (m_cv_index)->layout (), std::string (), m_states);
    }
  } catch (tl::Exception &ex) {
    //  potentially caused by script errors in callback implementation
    tl::error << ex.msg ();
  } catch (std::runtime_error &ex) {
    tl::error << ex.what ();
  } catch (...) {
    //  ignore other errors
  }

  m_initial_states = m_states;
  mp_error_frame->hide ();

  update_widgets_from_states (m_states, false);
}

void
PCellParametersPage::update_widgets_from_states (const db::ParameterStates &states, bool tentatively)
{
  if (! mp_pcell_decl) {
    return;
  }

  size_t i = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end () && i < m_widgets.size (); ++p, ++i) {

    const std::string &name = p->get_name ();
    const std::string &static_tooltip = p->get_tooltip ();
    const db::ParameterState &ps = states.parameter (name);

    if (m_widgets [i]) {
      QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [i]);
      if (le) {
        le->setEnabled (ps.is_enabled ());
        le->setReadOnly (ps.is_readonly ());
      } else {
        m_widgets [i]->setEnabled (ps.is_enabled () && ! ps.is_readonly ());
      }
    }

    for (auto w = m_all_widgets [i].begin (); w != m_all_widgets [i].end (); ++w) {
      if (*w != m_widgets [i]) {
        (*w)->setEnabled (ps.is_enabled ());
      }
      if (*w != m_icon_widgets [i]) {
        (*w)->setVisible (ps.is_visible ());
      }
      if (ps.tooltip ().empty ()) {
        (*w)->setToolTip (tl::to_qstring (static_tooltip));
      } else {
        (*w)->setToolTip (tl::to_qstring (ps.tooltip ()));
      }
    }

    if (m_icon_widgets [i]) {

      static QPixmap error (":/error_16px@2x.png");
      static QPixmap info (":/info_16px@2x.png");
      static QPixmap warning (":/warn_16px@2x.png");

      switch (ps.icon ()) {
      case db::ParameterState::NoIcon:
      default:
        m_icon_widgets [i]->setPixmap (QPixmap ());
        m_icon_widgets [i]->hide ();
        break;
      case db::ParameterState::InfoIcon:
        m_icon_widgets [i]->setPixmap (info);
        m_icon_widgets [i]->setVisible (ps.is_visible ());
        break;
      case db::ParameterState::WarningIcon:
        m_icon_widgets [i]->setPixmap (warning);
        m_icon_widgets [i]->setVisible (ps.is_visible ());
        break;
      case db::ParameterState::ErrorIcon:
        m_icon_widgets [i]->setPixmap (error);
        m_icon_widgets [i]->setVisible (ps.is_visible ());
        break;
      }

    }

  }

  set_parameters_internal (states, tentatively);

  //  QGridLayouts are bad in handling nested QFrame (or QGroupBox) with their own layouts,
  //  so we help a little here:
  for (auto g = mp_groups.begin (); g != mp_groups.end (); ++g) {
    (*g)->resize (QSize ((*g)->width (), (*g)->sizeHint ().height ()));
  }
}

void
PCellParametersPage::set_parameters_internal (const db::ParameterStates &states, bool tentatively)
{
  if (! mp_pcell_decl) {
    return;
  }

  //  write the changed value back
  size_t r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {
    if (m_widgets [r]) {
      set_value (*p, m_widgets [r], states.parameter (p->get_name ()).value ());
    }
  }

  bool update_needed = false;

  if (! tentatively) {
    m_current_states = states;
  } else {
    update_needed = ! m_current_states.values_are_equal (states);
  }

  mp_update_frame->setVisible (update_needed);
}

std::vector<tl::Variant>
PCellParametersPage::parameter_from_states (const db::ParameterStates &states) const
{
  std::vector<tl::Variant> parameters;
  if (mp_pcell_decl) {

    const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
    for (auto p = pcp.begin (); p != pcp.end (); ++p) {
      if (! states.has_parameter (p->get_name ())) {
        parameters.push_back (p->get_default ());
      } else {
        parameters.push_back (states.parameter (p->get_name ()).value ());
      }
    }

  }

  return parameters;
}

void
PCellParametersPage::states_from_parameters (db::ParameterStates &states, const std::vector<tl::Variant> &parameters)
{
  if (! mp_pcell_decl) {
    return;
  }

  size_t r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {
    db::ParameterState &ps = states.parameter (p->get_name ());
    if (r < parameters.size ()) {
      ps.set_value (parameters [r]);
    } else {
      ps.set_value (p->get_default ());
    }
  }
}

void
PCellParametersPage::check_range (const tl::Variant &value, const db::PCellParameterDeclaration &decl)
{
  if (! decl.min_value ().is_nil () && value < decl.min_value ()) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("The value is lower than the minimum allowed value: given value is %s, minimum value is %s")), value.to_string (), decl.min_value ().to_string ()));
  }

  if (! decl.max_value ().is_nil () && ! (value < decl.max_value () || value == decl.max_value ())) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("The value is higher than the maximum allowed value: given value is %s, maximum value is %s")), value.to_string (), decl.max_value ().to_string ()));
  }
}

}

#endif
