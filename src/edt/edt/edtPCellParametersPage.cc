
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "layWidgets.h"
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

static void set_value (const db::PCellParameterDeclaration &p, const db::Layout * /*layout*/, QWidget *widget, const tl::Variant &value)
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

PCellParametersPage::PCellParametersPage (QWidget *parent, const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters)
  : QFrame (parent)
{
  init ();
  setup (layout, view, cv_index, pcell_decl, parameters);
}

PCellParametersPage::PCellParametersPage (QWidget *parent)
  : QFrame (parent)
{
  init ();
}

void
PCellParametersPage::init ()
{
  mp_pcell_decl = 0;
  mp_layout = 0;
  mp_view = 0;
  m_cv_index = 0;
  mp_parameters_area = 0;

  QGridLayout *frame_layout = new QGridLayout (this);
  setLayout (frame_layout);

  mp_error_icon = new QLabel (this);
  mp_error_icon->setPixmap (QPixmap (":/warn.png"));
  mp_error_icon->hide ();
  frame_layout->addWidget (mp_error_icon, 1, 0, 1, 1);

  mp_error_label = new QLabel (this);
  QPalette palette = mp_error_label->palette ();
  palette.setColor (QPalette::Foreground, Qt::red);
  mp_error_label->setPalette (palette);
  QFont font = mp_error_label->font ();
  font.setBold (true);
  mp_error_label->setFont (font);
  mp_error_label->hide ();
  frame_layout->addWidget (mp_error_label, 1, 1, 1, 1);
  frame_layout->setColumnStretch (1, 1);
}

void
PCellParametersPage::setup (const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters)
{
  mp_pcell_decl = pcell_decl;
  mp_layout = layout;
  mp_view = view;
  m_cv_index = cv_index;
  m_parameters = parameters;

  if (mp_parameters_area) {
    delete mp_parameters_area;
  }

  m_widgets.clear ();

  mp_parameters_area = new QScrollArea (this);
  QGridLayout *frame_layout = dynamic_cast<QGridLayout *> (QFrame::layout ());
  frame_layout->addWidget (mp_parameters_area, 0, 0, 1, 2);
  frame_layout->setRowStretch (0, 1);

  QFrame *fi = new QFrame (mp_parameters_area);
  QWidget *inner_frame = fi;
  fi->setFrameShape (QFrame::NoFrame);
  setFrameShape (QFrame::NoFrame);

  QGridLayout *inner_grid = new QGridLayout (inner_frame);
  inner_frame->setLayout (inner_grid);

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
          hb->setMargin (0);
          f->setLayout (hb);

          QLineEdit *le = new QLineEdit (f);
          le->setEnabled (! p->is_readonly ());
          hb->addWidget (le);
          le->setMaximumWidth (150);
          m_widgets.push_back (le);

          QLabel *ul = new QLabel (f);
          hb->addWidget (ul, 1);
          ul->setText (tl::to_qstring (p->get_unit ()));

          inner_grid->addWidget (f, row, 1);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (activated ()));
        }
        break;

      case db::PCellParameterDeclaration::t_string:
      case db::PCellParameterDeclaration::t_shape:
      case db::PCellParameterDeclaration::t_list:
        {
          QLineEdit *le = new QLineEdit (inner_frame);
          le->setEnabled (! p->is_readonly ());
          m_widgets.push_back (le);
          inner_grid->addWidget (le, row, 1);

          connect (le, SIGNAL (editingFinished ()), this, SLOT (activated ()));
        }
        break;

      case db::PCellParameterDeclaration::t_layer:
        {
          lay::LayerSelectionComboBox *ly = new lay::LayerSelectionComboBox (inner_frame);
          ly->setEnabled (! p->is_readonly ());
          ly->set_no_layer_available (true);
          ly->set_view (mp_view, m_cv_index, true /*all layers*/);
          m_widgets.push_back (ly);
          inner_grid->addWidget (ly, row, 1);
        }
        break;

      case db::PCellParameterDeclaration::t_boolean:
        {
          QCheckBox *cbx = new QCheckBox (inner_frame);
          cbx->setEnabled (! p->is_readonly ());
          m_widgets.push_back (cbx);
          inner_grid->addWidget (cbx, row, 1);

          connect (cbx, SIGNAL (stateChanged (int)), this, SLOT (activated ()));
        }
        break;

      default:
        m_widgets.push_back (0);
        break;
      }

    } else {

      QComboBox *cb = new QComboBox (inner_frame);

      int i = 0;
      for (std::vector<tl::Variant>::const_iterator c = p->get_choices ().begin (); c != p->get_choices ().end (); ++c, ++i) {
        if (i < int (p->get_choice_descriptions ().size ())) {
          cb->addItem (tl::to_qstring (p->get_choice_descriptions () [i]));
        } else {
          cb->addItem (tl::to_qstring (c->to_string ()));
        }
      }

      connect (cb, SIGNAL (activated (int)), this, SLOT (activated ()));
      cb->setEnabled (! p->is_readonly ());
      cb->setMinimumContentsLength (30);
      cb->setSizeAdjustPolicy (QComboBox::AdjustToMinimumContentsLengthWithIcon);
      m_widgets.push_back (cb);
      inner_grid->addWidget (cb, row, 1);

    }

    set_value (*p, mp_layout, m_widgets.back (), value);

    ++row;
    if (inner_frame == main_frame) {
      ++main_row;
    }

  }

  mp_parameters_area->setWidget (main_frame);
  main_frame->show ();

  //  does a first coerce and update
  get_parameters ();
}

PCellParametersPage::State
PCellParametersPage::get_state ()
{
  State s;
  s.valid = true;
  s.vScrollPosition = mp_parameters_area->verticalScrollBar ()->value ();
  s.hScrollPosition = mp_parameters_area->horizontalScrollBar ()->value ();
  return s;
}

void
PCellParametersPage::set_state (const State &s)
{
  if (s.valid) {
    mp_parameters_area->verticalScrollBar ()->setValue (s.vScrollPosition);
    mp_parameters_area->horizontalScrollBar ()->setValue (s.hScrollPosition);
  }
}

void  
PCellParametersPage::activated ()
{
  //  does a coerce and update
  get_parameters ();
}

void  
PCellParametersPage::clicked ()
{
  //  does a coerce and update
  get_parameters ();
}

std::vector<tl::Variant> 
PCellParametersPage::get_parameters () 
{
  std::vector<tl::Variant> parameters;

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
              int v = 0;
              tl::from_string (tl::to_string (le->text ()), v);
              parameters.back () = tl::Variant (v);
            }
          }
          break;

        case db::PCellParameterDeclaration::t_double:
          {
            QLineEdit *le = dynamic_cast<QLineEdit *> (m_widgets [r]);
            if (le) {
              double v = 0;
              tl::from_string (tl::to_string (le->text ()), v);
              parameters.back () = tl::Variant (v);
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

  try {

    //  coerce the parameters
    mp_pcell_decl->coerce_parameters (*mp_layout, parameters);
    set_parameters (parameters);

  } catch (tl::ScriptError &ex) {

    mp_error_label->setText (tl::to_qstring (ex.basic_msg ()));
    mp_error_label->setToolTip (tl::to_qstring (ex.msg ()));
    mp_error_icon->show ();
    mp_error_label->show ();

  } catch (tl::Exception &ex) {

    mp_error_label->setText (tl::to_qstring (ex.msg ()));
    mp_error_icon->show ();
    mp_error_label->show ();

  }

  return parameters;
}

void 
PCellParametersPage::set_parameters (const  std::vector<tl::Variant> &parameters) 
{
  //  write the changed value back
  size_t r = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = mp_pcell_decl->parameter_declarations ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++r) {
    if (r < parameters.size () && m_widgets [r]) {
      set_value (*p, mp_layout, m_widgets [r], parameters [r]);
    }
  }
}

}


