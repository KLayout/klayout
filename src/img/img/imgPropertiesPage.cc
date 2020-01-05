
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


#include "imgPropertiesPage.h"
#include "imgLandmarksDialog.h"
#include "layLayoutView.h"
#include "layFileDialog.h"
#include "tlExceptions.h"

namespace img
{

const double min_gamma = 0.3;
const double max_gamma = 3.0;

// -------------------------------------------------------------------------
//  PropertiesPage implementation

PropertiesPage::PropertiesPage (img::Service *service, db::Manager *manager, QWidget *parent)
  : lay::PropertiesPage (parent, manager, service), mp_service (service), mp_direct_image (0)
{
  mp_service->get_selection (m_selection);
  m_pos = m_selection.begin ();

  mp_service->clear_highlights ();

  init ();
}

PropertiesPage::PropertiesPage (QWidget *parent)
  : lay::PropertiesPage (parent, 0, 0), mp_service (0), mp_direct_image (0)
{
  init ();
}

PropertiesPage::~PropertiesPage ()
{
  if (mp_service) {

    mp_service->restore_highlights ();

    if (mp_direct_image) {
      delete mp_direct_image;
      mp_direct_image = 0;
    }

  }
}

void
PropertiesPage::init ()
{
  m_no_signals = false;

  setupUi (this);

  QAction *action;

  action = new QAction (QObject::tr ("Black To White"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (black_to_white ()));
  false_color_control->addAction (action);

  action = new QAction (QObject::tr ("White To Black"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (white_to_black ()));
  false_color_control->addAction (action);

  action = new QAction (QObject::tr ("Red To Blue"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (red_to_blue ()));
  false_color_control->addAction (action);

  action = new QAction (QObject::tr ("Blue To Red"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (blue_to_red ()));
  false_color_control->addAction (action);

  action = new QAction (this);
  action->setSeparator (true);
  false_color_control->addAction (action);

  action = new QAction (QObject::tr ("Reverse Color Order"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (reverse_color_order ()));
  false_color_control->addAction (action);

  color_pb->set_color (QColor ());

  connect (browse_pb, SIGNAL (clicked ()), this, SLOT (browse ()));
  connect (color_pb, SIGNAL (color_changed (QColor)), false_color_control, SLOT (set_current_color (QColor)));
  connect (false_color_control, SIGNAL (selection_changed (QColor)), color_pb, SLOT (set_color (QColor)));

  connect (brightness_slider, SIGNAL (valueChanged (int)), this, SLOT (brightness_slider_changed (int)));
  connect (brightness_sb, SIGNAL (valueChanged (int)), this, SLOT (brightness_spinbox_changed (int)));
  connect (contrast_slider, SIGNAL (valueChanged (int)), this, SLOT (contrast_slider_changed (int)));
  connect (contrast_sb, SIGNAL (valueChanged (int)), this, SLOT (contrast_spinbox_changed (int)));
  connect (gamma_slider, SIGNAL (valueChanged (int)), this, SLOT (gamma_slider_changed (int)));
  connect (gamma_sb, SIGNAL (valueChanged (double)), this, SLOT (gamma_spinbox_changed (double)));
  connect (r_slider, SIGNAL (valueChanged (int)), this, SLOT (red_slider_changed (int)));
  connect (r_sb, SIGNAL (valueChanged (double)), this, SLOT (red_spinbox_changed (double)));
  connect (g_slider, SIGNAL (valueChanged (int)), this, SLOT (green_slider_changed (int)));
  connect (g_sb, SIGNAL (valueChanged (double)), this, SLOT (green_spinbox_changed (double)));
  connect (b_slider, SIGNAL (valueChanged (int)), this, SLOT (blue_slider_changed (int)));
  connect (b_sb, SIGNAL (valueChanged (double)), this, SLOT (blue_spinbox_changed (double)));

  connect (false_color_control, SIGNAL (color_mapping_changed ()), this, SLOT (color_mapping_changed ()));
  connect (false_color_control, SIGNAL (selection_changed ()), this, SLOT (color_mapping_changed ()));
  connect (from_le, SIGNAL (returnPressed ()), this, SLOT (min_max_return_pressed ()));
  connect (to_le, SIGNAL (returnPressed ()), this, SLOT (min_max_return_pressed ()));
  connect (value_le, SIGNAL (returnPressed ()), this, SLOT (value_return_pressed ()));

  connect (reset_pb, SIGNAL (clicked ()), this, SLOT (reset_pressed ()));
  connect (preview_cbx, SIGNAL (clicked ()), this, SLOT (preview_checked ()));
  connect (define_landmarks_pb, SIGNAL (clicked ()), this, SLOT (define_landmarks_pressed ()));
}

void
PropertiesPage::invalidate ()
{
  if (mp_direct_image) {
    delete mp_direct_image;
    mp_direct_image = 0;
  }
}

void 
PropertiesPage::back ()
{
  m_pos = m_selection.end ();
  invalidate ();
}

void 
PropertiesPage::front ()
{
  m_pos = m_selection.begin ();
  invalidate ();
}

bool 
PropertiesPage::at_begin () const
{
  return (m_pos == m_selection.begin ());
}

bool 
PropertiesPage::at_end () const
{
  return (m_pos == m_selection.end ());
}

void 
PropertiesPage::operator-- ()
{
  --m_pos;
  invalidate ();
}

void 
PropertiesPage::operator++ ()
{
  ++m_pos;
  invalidate ();
}

void
PropertiesPage::leave ()
{
  mp_service->clear_highlights ();
}

void 
PropertiesPage::set_direct_image (img::Object *image)
{
  tl_assert (mp_service == 0);
  mp_direct_image = image;
}

bool 
PropertiesPage::readonly ()
{
  return false;
}

void 
PropertiesPage::min_max_return_pressed ()
{
BEGIN_PROTECTED

  value_le->setText (QString ());
  value_le->setEnabled (false);

  color_pb->setEnabled (false_color_control->has_selection ());

  double xmin, xmax;
  tl::from_string (tl::to_string (from_le->text ()), xmin);
  tl::from_string (tl::to_string (to_le->text ()), xmax);
  if (xmin >= xmax) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid data value range (min. value must be less than max. value)")));
  }

  if (false_color_control->has_selection () && false_color_control->selected_node () > 0 && false_color_control->selected_node () < int (false_color_control->nodes ().size ()) - 1) {

    double x = false_color_control->nodes () [false_color_control->selected_node ()].first;
    double xx = x * (xmax - xmin) + xmin;

    value_le->setText (tl::to_qstring (tl::sprintf ("%.4g", xx)));
    value_le->setEnabled (true);

  }

  recompute_histogram ();

  preview ();

END_PROTECTED
}

void
PropertiesPage::color_mapping_changed ()
{
  if (! m_no_signals) {

    value_le->setText (QString ());
    value_le->setEnabled (false);

    color_pb->setEnabled (false_color_control->has_selection ());

    try {

      if (false_color_control->has_selection () && false_color_control->selected_node () > 0 && false_color_control->selected_node () < int (false_color_control->nodes ().size ()) - 1) {

        double xmin, xmax;
        tl::from_string (tl::to_string (from_le->text ()), xmin);
        tl::from_string (tl::to_string (to_le->text ()), xmax);
        if (xmin >= xmax) {
          throw tl::Exception ("");
        }

        double x = false_color_control->nodes () [false_color_control->selected_node ()].first;
        double xx = x * (xmax - xmin) + xmin;

        value_le->setText (tl::to_qstring (tl::sprintf ("%.4g", xx)));
        value_le->setEnabled (true);

      }

    } catch (...) { }

    preview ();

  }
}

void
PropertiesPage::value_return_pressed ()
{
BEGIN_PROTECTED

  double xmin, xmax;
  tl::from_string (tl::to_string (from_le->text ()), xmin);
  tl::from_string (tl::to_string (to_le->text ()), xmax);
  if (xmin >= xmax) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid data value range (min. value must be less than max. value)")));
  }

  double x = 0.0;
  tl::from_string (tl::to_string (value_le->text ()), x);

  double xx = (x - xmin) / (xmax - xmin);
  if (xx < 0 || xx > 1.0) {
    throw tl::Exception (tl::to_string (QObject::tr ("The position entered (%g) must be between the minimum (%g) and maximum (%g) value")), x, xmin, xmax);
  }

  m_no_signals = true;
  false_color_control->set_current_position (xx);
  m_no_signals = false;

  preview ();

END_PROTECTED
}

inline double 
round_to_zero (double x)
{
  return 1e-6 * floor (0.5 + 1e6 * x);
}

void 
PropertiesPage::update ()
{
  m_no_signals = true;

  if (mp_service) {

    mp_service->highlight (std::distance (m_selection.begin (), m_pos));

    //  create a local copy in which we can apply modifications
    if (! mp_direct_image) {
      const img::Object *image = dynamic_cast <const img::Object *> ((*m_pos)->ptr ());
      mp_direct_image = new img::Object (*image);
    }

  }

  std::string mode;
  if (mp_direct_image->is_color ()) {
    mode = tl::to_string (QObject::tr ("color"));
  } else {
    mode = tl::to_string (QObject::tr ("mono"));
  }
  if (mp_direct_image->is_byte_data ()) {
    mode += tl::to_string (QObject::tr ("/8bit"));
  }

  file_name_lbl->setText (tl::to_qstring (mp_direct_image->filename ()));

  if (! mp_direct_image->is_empty ()) {
    file_info_lbl->setText (tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("%lu x %lu pixels (%s)")), mp_direct_image->width (), mp_direct_image->height (), mode)));
  } else {
    file_info_lbl->setText (QObject::tr ("No data loaded"));
  }

  data_mapping_tab_widget->setTabEnabled (0, !mp_direct_image->is_color ());

  db::Matrix3d matrix = mp_direct_image->matrix ();

  //  The observer distance for perspective distortion is the average of the images width and height.
  //  The coordinate space origin in the center of the image.
  double pw2 = std::max (0.5, 0.5 * matrix.mag_x () * mp_direct_image->width ());
  double ph2 = std::max (0.5, 0.5 * matrix.mag_y () * mp_direct_image->height ());
  double z = pw2 + ph2;

  width_le->setText (tl::to_qstring (tl::micron_to_string (matrix.mag_x ())));
  width_le->setCursorPosition (0);

  height_le->setText (tl::to_qstring (tl::micron_to_string (matrix.mag_y ())));
  height_le->setCursorPosition (0);

  x_offset_le->setText (tl::to_qstring (tl::micron_to_string (round_to_zero (matrix.disp ().x ()))));
  x_offset_le->setCursorPosition (0);

  y_offset_le->setText (tl::to_qstring (tl::micron_to_string (round_to_zero (matrix.disp ().y ()))));
  y_offset_le->setCursorPosition (0);

  angle_le->setText (tl::to_qstring (tl::to_string (round_to_zero (matrix.angle ()))));
  angle_le->setCursorPosition (0);

  shear_le->setText (tl::to_qstring (tl::to_string (round_to_zero (matrix.shear_angle ()))));
  shear_le->setCursorPosition (0);

  persp_tx_le->setText (tl::to_qstring (tl::to_string (round_to_zero (matrix.perspective_tilt_x (z)))));
  persp_tx_le->setCursorPosition (0);

  persp_ty_le->setText (tl::to_qstring (tl::to_string (round_to_zero (matrix.perspective_tilt_y (z)))));
  persp_ty_le->setCursorPosition (0);

  mirror_cbx->setChecked (matrix.is_mirror ());

  from_le->setText (tl::to_qstring (tl::to_string (mp_direct_image->min_value ())));
  from_le->setCursorPosition (0);

  to_le->setText (tl::to_qstring (tl::to_string (mp_direct_image->max_value ())));
  to_le->setCursorPosition (0);

  false_color_control->set_nodes (mp_direct_image->data_mapping ().false_color_nodes);

  brightness_slider->setValue (int (floor (mp_direct_image->data_mapping ().brightness * 100 + 0.5)));
  brightness_sb->setValue (int (floor (mp_direct_image->data_mapping ().brightness * 100 + 0.5)));

  contrast_slider->setValue (int (floor (mp_direct_image->data_mapping ().contrast * 100 + 0.5)));
  contrast_sb->setValue (int (floor (mp_direct_image->data_mapping ().contrast * 100 + 0.5)));

  gamma_sb->setValue (mp_direct_image->data_mapping ().gamma);
  r_sb->setValue (mp_direct_image->data_mapping ().red_gain);
  g_sb->setValue (mp_direct_image->data_mapping ().green_gain);
  b_sb->setValue (mp_direct_image->data_mapping ().blue_gain);

  m_no_signals = true;

  if (mp_direct_image->data_mapping ().gamma < 1.0) {
    gamma_slider->setValue (50 - int (0.5 + (1.0 / mp_direct_image->data_mapping ().gamma - 1.0) / (1.0 / min_gamma - 1.0) * 50.0));
  } else {
    gamma_slider->setValue (50 + int (0.5 + (mp_direct_image->data_mapping ().gamma - 1.0) / (max_gamma - 1.0) * 50.0));
  }

  r_slider->setValue (int (0.5 + mp_direct_image->data_mapping ().red_gain * 50.0));
  g_slider->setValue (int (0.5 + mp_direct_image->data_mapping ().green_gain * 50.0));
  b_slider->setValue (int (0.5 + mp_direct_image->data_mapping ().blue_gain * 50.0));

  m_no_signals = false;

  recompute_histogram ();
}

void
PropertiesPage::recompute_histogram ()
{
  std::vector <size_t> histogram;

  try {

    double xmin, xmax;
    tl::from_string (tl::to_string (from_le->text ()), xmin);
    tl::from_string (tl::to_string (to_le->text ()), xmax);
    if (xmin >= xmax) {
      throw tl::Exception ("");
    }

    if (! mp_direct_image->byte_data ()) {

      const float *data = mp_direct_image->float_data ();
      if (! data) {
        data = mp_direct_image->float_data (1); // for testing
      }

      if (data) {

        histogram.resize (256, 0);

        double s = double (histogram.size () - 1) / (xmax - xmin);

        for (size_t i = mp_direct_image->data_length (); i > 0; --i) {
          double hi = (*data++ - xmin) * s;
          if (hi >= 0 && hi < double (histogram.size ())) {
            histogram [size_t (hi)] += 1;
          }
        }

      }

    } else {

      const unsigned char *data = mp_direct_image->byte_data ();
      if (! data) {
        data = mp_direct_image->byte_data (1); // for testing
      }

      if (data) {

        histogram.resize (256, 0);

        double s = double (histogram.size () - 1) / (xmax - xmin);

        for (size_t i = mp_direct_image->data_length (); i > 0; --i) {
          double hi = (*data++ - xmin) * s;
          if (hi >= 0 && hi < double (histogram.size ())) {
            histogram [size_t (hi)] += 1;
          }
        }

      }

    }

  } catch (...) { }

  false_color_control->set_histogram (histogram);
}

void 
PropertiesPage::brightness_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;
  brightness_sb->setValue (value);
  preview ();
  m_no_signals = false;
}

void 
PropertiesPage::brightness_spinbox_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;
  brightness_slider->setValue (value);
  preview ();
  m_no_signals = false;
}

void 
PropertiesPage::contrast_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;
  contrast_sb->setValue (value);
  preview ();
  m_no_signals = false;
}

void 
PropertiesPage::contrast_spinbox_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;
  contrast_slider->setValue (value);
  preview ();
  m_no_signals = false;
}

void 
PropertiesPage::gamma_spinbox_changed (double value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  if (value < 1.0) {
    gamma_slider->setValue (50 - int (0.5 + (1.0 / value - 1.0) / (1.0 / min_gamma - 1.0) * 50.0));
  } else {
    gamma_slider->setValue (50 + int (0.5 + (value - 1.0) / (max_gamma - 1.0) * 50.0));
  }

  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::gamma_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  double gamma;
  if (value < 50) {
    gamma = 1.0 / ((50 - value) / 50.0 * (1.0 / min_gamma - 1.0) + 1.0);
  } else {
    gamma = (value - 50) / 50.0 * (max_gamma - 1.0) + 1.0;
  }

  gamma_sb->setValue (gamma);
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::red_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  double gain = value * 0.02;

  r_sb->setValue (gain);
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::red_spinbox_changed (double value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  r_slider->setValue (int (0.5 + value * 50.0));
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::green_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  double gain = value * 0.02;

  g_sb->setValue (gain);
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::green_spinbox_changed (double value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  g_slider->setValue (int (0.5 + value * 50.0));
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::blue_slider_changed (int value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  double gain = value * 0.02;

  b_sb->setValue (gain);
  preview ();

  m_no_signals = false;
}

void 
PropertiesPage::blue_spinbox_changed (double value)
{
  if (m_no_signals) {
    return;
  }

  m_no_signals = true;

  b_slider->setValue (int (0.5 + value * 50.0));
  preview ();

  m_no_signals = false;
}

void  
PropertiesPage::black_to_white ()
{
  std::vector <std::pair <double, QColor> > nodes;
  nodes.push_back (std::make_pair (0.0, QColor (0, 0, 0)));
  nodes.push_back (std::make_pair (1.0, QColor (255, 255, 255)));
  false_color_control->set_nodes (nodes);
}

void  
PropertiesPage::white_to_black ()
{
  std::vector <std::pair <double, QColor> > nodes;
  nodes.push_back (std::make_pair (0.0, QColor (255, 255, 255)));
  nodes.push_back (std::make_pair (1.0, QColor (0, 0, 0)));
  false_color_control->set_nodes (nodes);
}

void  
PropertiesPage::red_to_blue ()
{
  std::vector <std::pair <double, QColor> > nodes;
  nodes.push_back (std::make_pair (0.0, QColor (255, 0, 0)));
  nodes.push_back (std::make_pair (1.0, QColor (0, 0, 255)));
  false_color_control->set_nodes (nodes);

}

void  
PropertiesPage::blue_to_red ()
{
  std::vector <std::pair <double, QColor> > nodes;
  nodes.push_back (std::make_pair (0.0, QColor (0, 0, 255)));
  nodes.push_back (std::make_pair (1.0, QColor (255, 0, 0)));
  false_color_control->set_nodes (nodes);
}

void  
PropertiesPage::reverse_color_order ()
{
  std::vector <std::pair <double, QColor> > nodes (false_color_control->nodes ());
  for (size_t i = 0; i < nodes.size () / 2; ++i) {
    std::swap (nodes [i].second, nodes [nodes.size () - 1 - i].second);
  }
  false_color_control->set_nodes (nodes);
}

void 
PropertiesPage::apply ()
{
  db::Matrix3d matrix = mp_direct_image->matrix ();

  //  The observer distance for perspective distortion is the average of the images width and height.
  //  The coordinate space origin in the center of the image.
  double pw2 = std::max (0.5, 0.5 * matrix.mag_x () * mp_direct_image->width ());
  double ph2 = std::max (0.5, 0.5 * matrix.mag_y () * mp_direct_image->height ());
  double z = pw2 + ph2;

  double w = matrix.mag_x (), h = matrix.mag_y (), x = matrix.disp ().x (), y = matrix.disp ().y (), 
         a = matrix.angle (), sa = matrix.shear_angle (), tx = matrix.perspective_tilt_x (z), ty = matrix.perspective_tilt_y (z);

  bool mirror;

  if (width_le->text () != tl::to_qstring (tl::micron_to_string (matrix.mag_x ()))) {
    tl::from_string (tl::to_string (width_le->text ()), w);
  }
  if (height_le->text () != tl::to_qstring (tl::micron_to_string (matrix.mag_y ()))) {
    tl::from_string (tl::to_string (height_le->text ()), h);
  }
  if (x_offset_le->text () != tl::to_qstring (tl::micron_to_string (round_to_zero (matrix.disp ().x ())))) {
    tl::from_string (tl::to_string (x_offset_le->text ()), x);
  }
  if (y_offset_le->text () != tl::to_qstring (tl::micron_to_string (round_to_zero (matrix.disp ().y ())))) {
    tl::from_string (tl::to_string (y_offset_le->text ()), y);
  }
  if (angle_le->text () != tl::to_qstring (tl::to_string (round_to_zero (matrix.angle ())))) {
    tl::from_string (tl::to_string (angle_le->text ()), a);
  }
  if (shear_le->text () != tl::to_qstring (tl::to_string (round_to_zero (matrix.shear_angle ())))) {
    tl::from_string (tl::to_string (shear_le->text ()), sa);
  }
  if (persp_tx_le->text () != tl::to_qstring (tl::to_string (round_to_zero (matrix.perspective_tilt_x (z))))) {
    tl::from_string (tl::to_string (persp_tx_le->text ()), tx);
  }
  if (persp_ty_le->text () != tl::to_qstring (tl::to_string (round_to_zero (matrix.perspective_tilt_y (z))))) {
    tl::from_string (tl::to_string (persp_ty_le->text ()), ty);
  }

  mirror = mirror_cbx->isChecked ();

  if (w <= 0.0 || h <= 0.0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Pixel width or height must be positive, non-null values")));
  }

  if (sa <= -45 || sa >= 45) {
    throw tl::Exception (tl::to_string (QObject::tr ("The shear angle must be larger than -45 and less than 45 degree")));
  }

  if (tx <= -90 || tx >= 90 || ty <= -90 || ty >= 90) {
    throw tl::Exception (tl::to_string (QObject::tr ("The perspective tilt angles must be larger than -90 and less than 90 degree")));
  }

  //  Compute the new observer distance
  pw2 = std::max (0.5, 0.5 * w * mp_direct_image->width ());
  ph2 = std::max (0.5, 0.5 * h * mp_direct_image->height ());
  z = pw2 + ph2;

  matrix = db::Matrix3d::disp (db::DVector (x, y)) * db::Matrix3d::perspective (tx, ty, z) * db::Matrix3d::rotation (a) * db::Matrix3d::shear (sa) * db::Matrix3d::mag (w, h) * db::Matrix3d::mirror (mirror);
  mp_direct_image->set_matrix (matrix);

  double xmin, xmax;
  tl::from_string (tl::to_string (from_le->text ()), xmin);
  tl::from_string (tl::to_string (to_le->text ()), xmax);
  if (xmin >= xmax) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid data value range (min. value must be less than max. value)")));
  }

  mp_direct_image->set_min_value (xmin);
  mp_direct_image->set_max_value (xmax);

  img::DataMapping dm (mp_direct_image->data_mapping ());
  dm.brightness = brightness_sb->value () * 0.01;
  dm.contrast = contrast_sb->value () * 0.01;
  dm.gamma = gamma_sb->value ();
  dm.red_gain = r_sb->value ();
  dm.green_gain = g_sb->value ();
  dm.blue_gain = b_sb->value ();
  dm.false_color_nodes = false_color_control->nodes ();
  mp_direct_image->set_data_mapping (dm);

  if (mp_service) {
    mp_service->change_image (*m_pos, *mp_direct_image);
  }
}

void
PropertiesPage::browse ()
{
BEGIN_PROTECTED

  apply ();

  lay::FileDialog file_dialog (this, tl::to_string (QObject::tr ("Load Image File")), tl::to_string (QObject::tr ("All files (*)")));

  static std::string s_filename;
  std::string filename;
  if (mp_direct_image) {
    filename = mp_direct_image->filename ();
    if (filename.empty ()) {
      filename = s_filename;
    }
    if (file_dialog.get_open (filename)) {
      mp_direct_image->load_data (filename, true /*update min and max values*/);
      s_filename = filename;
      update ();
    }
  }

END_PROTECTED
}

void
PropertiesPage::reset_pressed ()
{
  m_no_signals = true;

  gamma_sb->setValue (1.0);
  gamma_slider->setValue (50);

  brightness_sb->setValue (0);
  brightness_slider->setValue (0);

  contrast_sb->setValue (0);
  contrast_slider->setValue (0);

  r_sb->setValue (1.0);
  r_slider->setValue (50);

  g_sb->setValue (1.0);
  g_slider->setValue (50);

  b_sb->setValue (1.0);
  b_slider->setValue (50);

  m_no_signals = false;

  preview ();
}

void
PropertiesPage::preview_checked ()
{
  preview ();
}

void
PropertiesPage::preview ()
{
  if (preview_cbx->isChecked ()) {

    BEGIN_PROTECTED_CLEANUP

    apply (); // this is a HACK, because it changes the current object

    END_PROTECTED_CLEANUP
    {
      preview_cbx->setChecked (false);
    }

  }
}

void
PropertiesPage::define_landmarks_pressed ()
{
  if (mp_direct_image) {
    img::LandmarksDialog dialog (this, *mp_direct_image);
    dialog.exec ();
  }
}

}

