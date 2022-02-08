
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "edtPCellParametersPage.h"
#include "edtPropertiesPageUtils.h"
#include "layWidgets.h"
#include "layQtTools.h"
#include "layLayoutView.h"
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

PCellParametersPage::PCellParametersPage (QWidget *parent, bool dense)
  : QFrame (parent), m_dense (dense), dm_parameter_changed (this, &PCellParametersPage::do_parameter_changed)
{
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
  setLayout (frame_layout);

  mp_update_frame = new QFrame ();
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
  mp_changed_icon->setPixmap (QPixmap (":/warn.png"));
  update_frame_layout->addWidget (mp_changed_icon, 0, 0, 1, 1);

  mp_update_button = new QToolButton (mp_update_frame);
  mp_update_button->setText (tr ("Update"));
  connect (mp_update_button, SIGNAL (clicked()), this, SLOT (update_button_pressed ()));
  update_frame_layout->addWidget (mp_update_button, 0, 1, 1, 1);

  mp_changed_label = new QLabel (mp_update_frame);
  mp_changed_label->setText (tr ("Update needed"));
  update_frame_layout->addWidget (mp_changed_label, 0, 2, 1, 1);

  update_frame_layout->setColumnStretch (2, 1);

  mp_error_frame = new QFrame ();
  mp_error_frame->setFrameShape (QFrame::NoFrame);
  frame_layout->addWidget (mp_error_frame, 1, 0, 1, 1);

  QGridLayout *error_frame_layout = new QGridLayout (mp_error_frame);
  mp_error_frame->setLayout (error_frame_layout);
  if (m_dense) {
    error_frame_layout->setContentsMargins (4, 4, 4, 4);
    error_frame_layout->setHorizontalSpacing (6);
    error_frame_layout->setVerticalSpacing (2);
  }

  mp_error_icon = new QLabel (mp_update_frame);
  mp_error_icon->setPixmap (QPixmap (":/warn.png"));
  error_frame_layout->addWidget (mp_error_icon, 1, 0, 1, 1);

  mp_error_label = new QLabel (mp_update_frame);
  mp_error_label->setWordWrap (true);
  palette = mp_error_label->palette ();
  palette.setColor (QPalette::WindowText, Qt::red);
  mp_error_label->setPalette (palette);
  font = mp_error_label->font ();
  font.setBold (true);
  mp_error_label->setFont (font);
  error_frame_layout->addWidget (mp_error_label, 1, 1, 1, 2);

  error_frame_layout->setColumnStretch (2, 1);
}

bool
PCellParametersPage::lazy_evaluation ()
{
  return mp_pcell_decl.get () && mp_pcell_decl->wants_lazy_evaluation ();
}

void
PCellParametersPage::setup (lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters)
{
  mp_pcell_decl.reset (const_cast<db::PCellDeclaration *> (pcell_decl));  //  no const weak_ptr ...
  mp_view = view;
  m_cv_index = cv_index;
  m_parameters = parameters;

  if (mp_parameters_area) {
    delete mp_parameters_area;
  }

  m_widgets.clear ();

  mp_parameters_area = new QScrollArea (this);
  mp_parameters_area->setFrameShape (QFrame::NoFrame);
  QGridLayout *frame_layout = dynamic_cast<QGridLayout *> (QFrame::layout ());
  frame_layout->addWidget (mp_parameters_area, 2, 0, 1, 1);
  frame_layout->setRowStretch (2, 1);

  QFrame *fi = new QFrame (mp_parameters_area);
  QWidget *inner_frame = fi;
  fi->setFrameShape (QFrame::NoFrame);
  setFrameShape (QFrame::NoFrame);

  QGridLayout *inner_grid = new QGridLayout (inner_frame);
  inner_frame->setLayout (inner_grid);
  if (m_dense) {
    inner_grid->setContentsMargins (4, 4, 4, 4);
    inner_grid->setHorizontalSpacing (6);
    inner_grid->setVerticalSpacing (2);
  }

  QWidget *main_frame = inner_frame;
  QGridLayout *main_grid = inner_grid;

  int main_row = 0;
  int row = 0;
  std::string group_title;

  int r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {

    if (p->is_hidden () || p->get_type () == db::PCellParameterDeclaration::t_shape) {
      m_widgets.push_back (0);
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
        QGroupBox *gb = new QGroupBox (main_frame);
        gb->setTitle (tl::to_qstring (gt));
        main_grid->addWidget (gb, main_row, 0, 1, 2);
        
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
        inner_frame = main_frame;
        row = main_row;

      }

      group_title = gt;

    } 

    inner_grid->addWidget (new QLabel (tl::to_qstring (description), inner_frame), row, 0);

    tl::Variant value;
    if (r < int (parameters.size ())) {
      value = parameters [r];
    } else {
      value = p->get_default ();
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

          QLineEdit *le = new QLineEdit (f);
          le->setEnabled (! p->is_readonly ());
          hb->addWidget (le);
          le->setMaximumWidth (150);
          le->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (le);

          QLabel *ul = new QLabel (f);
          hb->addWidget (ul, 1);
          ul->setText (tl::to_qstring (p->get_unit ()));

          inner_grid->addWidget (f, row, 1);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_string:
      case db::PCellParameterDeclaration::t_shape:
      case db::PCellParameterDeclaration::t_list:
        {
          QLineEdit *le = new QLineEdit (inner_frame);
          le->setEnabled (! p->is_readonly ());
          le->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (le);
          inner_grid->addWidget (le, row, 1);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_layer:
        {
          lay::LayerSelectionComboBox *ly = new lay::LayerSelectionComboBox (inner_frame);
          ly->setEnabled (! p->is_readonly ());
          ly->set_no_layer_available (true);
          ly->set_view (mp_view, m_cv_index, true /*all layers*/);
          ly->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (ly);
          inner_grid->addWidget (ly, row, 1);

          connect (ly, SIGNAL (activated (int)), this, SLOT (parameter_changed ()));
        }
        break;

      case db::PCellParameterDeclaration::t_boolean:
        {
          QCheckBox *cbx = new QCheckBox (inner_frame);
          //  this makes the checkbox not stretch over the full width - better when navigating with tab
          cbx->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred));
          cbx->setEnabled (! p->is_readonly ());
          cbx->setObjectName (tl::to_qstring (p->get_name ()));
          m_widgets.push_back (cbx);
          inner_grid->addWidget (cbx, row, 1);

          connect (cbx, SIGNAL (stateChanged (int)), this, SLOT (parameter_changed ()));
        }
        break;

      default:
        m_widgets.push_back (0);
        break;
      }

    } else {

      QComboBox *cb = new QComboBox (inner_frame);
      cb->setObjectName (tl::to_qstring (p->get_name ()));

      int i = 0;
      for (std::vector<tl::Variant>::const_iterator c = p->get_choices ().begin (); c != p->get_choices ().end (); ++c, ++i) {
        if (i < int (p->get_choice_descriptions ().size ())) {
          cb->addItem (tl::to_qstring (p->get_choice_descriptions () [i]));
        } else {
          cb->addItem (tl::to_qstring (c->to_string ()));
        }
      }

      connect (cb, SIGNAL (activated (int)), this, SLOT (parameter_changed ()));

      cb->setEnabled (! p->is_readonly ());
      cb->setMinimumContentsLength (30);
      cb->setSizeAdjustPolicy (QComboBox::AdjustToMinimumContentsLengthWithIcon);
      m_widgets.push_back (cb);
      inner_grid->addWidget (cb, row, 1);

    }

    set_value (*p, m_widgets.back (), value);

    ++row;
    if (inner_frame == main_frame) {
      ++main_row;
    }

  }

  mp_parameters_area->setWidget (main_frame);
  main_frame->show ();

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
  dm_parameter_changed ();
}

void
PCellParametersPage::do_parameter_changed ()
{
  //  does a coerce and update
  bool ok = false;
  std::vector<tl::Variant> parameters = get_parameters (&ok);
  if (ok && ! lazy_evaluation ()) {
    emit edited ();
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
  bool ok = false;
  std::vector<tl::Variant> parameters = get_parameters (&ok);
  if (ok) {
    m_current_parameters = parameters;
    mp_update_frame->hide ();
  }

  return ok;
}

std::vector<tl::Variant> 
PCellParametersPage::get_parameters (bool *ok)
{
  std::vector<tl::Variant> parameters;

  try {

    if (! mp_pcell_decl) {
      throw tl::Exception (tl::to_string (tr ("PCell no longer valid.")));
    }

    bool edit_error = true;

    int r = 0;
    const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
    for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {

      if (p->is_hidden () || p->get_type () == db::PCellParameterDeclaration::t_shape) {

        if (r < (int) m_parameters.size ()) {
          parameters.push_back (m_parameters [r]);
        } else {
          parameters.push_back (p->get_default ());
        }

      } else {

        parameters.push_back (tl::Variant ());

        if (p->get_choices ().empty ()) {

          switch (p->get_type ()) {

          case db::PCellParameterDeclaration::t_int:
            {
              QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
              if (le) {

                try {

                  int v = 0;
                  tl::from_string_ext (tl::to_string (le->text ()), v);

                  parameters.back () = tl::Variant (v);
                  lay::indicate_error (le, (tl::Exception *) 0);

                } catch (tl::Exception &ex) {

                  lay::indicate_error (le, &ex);
                  edit_error = false;

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

                  parameters.back () = tl::Variant (v);
                  lay::indicate_error (le, (tl::Exception *) 0);

                } catch (tl::Exception &ex) {

                  lay::indicate_error (le, &ex);
                  edit_error = false;

                }

              }
            }
            break;

          case db::PCellParameterDeclaration::t_string:
            {
              QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
              if (le) {
                parameters.back () = tl::Variant (tl::to_string (le->text ()));
              }
            }
            break;

          case db::PCellParameterDeclaration::t_list:
            {
              QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
              if (le) {
                std::vector<std::string> values = tl::split (tl::to_string (le->text ()), ",");
                parameters.back () = tl::Variant (values.begin (), values.end ());
              }
            }
            break;

          case db::PCellParameterDeclaration::t_layer:
            {
              lay::LayerSelectionComboBox *ly = dynamic_cast<lay::LayerSelectionComboBox *> (m_widgets [r]);
              if (ly) {
                parameters.back () = tl::Variant (ly->current_layer_props ());
              }
            }
            break;
          case db::PCellParameterDeclaration::t_boolean:
            {
              QCheckBox *cbx = dynamic_cast<QCheckBox *> (m_widgets [r]);
              if (cbx) {
                parameters.back () = tl::Variant (cbx->isChecked ());
              }
            }
            break;

          default:
            break;
          }

        } else {

          QComboBox *cb = dynamic_cast<QComboBox*> (m_widgets [r]);
          if (cb && cb->currentIndex () >= 0 && cb->currentIndex () < int (p->get_choices ().size ())) {
            parameters.back () = p->get_choices () [cb->currentIndex ()];
          }

        }

      }

    }

    if (! edit_error) {
      throw tl::Exception (tl::to_string (tr ("There are errors. See the highlighted edit fields for details.")));
    }

    //  coerce the parameters
    if (mp_view->cellview (m_cv_index).is_valid ()) {
      mp_pcell_decl->coerce_parameters (mp_view->cellview (m_cv_index)->layout (), parameters);
    }
    set_parameters_internal (parameters, lazy_evaluation ());

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

  return parameters;
}

void
PCellParametersPage::set_parameters (const std::vector<tl::Variant> &parameters)
{
  m_parameters = parameters;
  set_parameters_internal (parameters, false);
}

void
PCellParametersPage::set_parameters_internal (const std::vector<tl::Variant> &parameters, bool tentatively)
{
  if (! mp_pcell_decl) {
    return;
  }

  //  write the changed value back
  size_t r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {
    if (r < parameters.size () && m_widgets [r]) {
      set_value (*p, m_widgets [r], parameters [r]);
    }
  }

  mp_error_frame->hide ();

  bool update_needed = false;

  if (! tentatively) {
    m_current_parameters = parameters;
  } else {
    update_needed = (m_current_parameters != parameters);
  }

  mp_update_frame->setVisible (update_needed);
}

}


