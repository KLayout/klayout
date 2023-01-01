
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "imgPropertiesPage.h"
#include "imgLandmarksDialog.h"
#include "imgStream.h"
#include "layLayoutView.h"
#include "layFileDialog.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlFileUtils.h"

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
  m_index = 0;

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
  m_in_color_mapping_signal = false;

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

  colors->set_color (std::make_pair (QColor (), QColor ()));
  colors->setEnabled (false);
  value_le->setEnabled (false);

  connect (browse_pb, SIGNAL (clicked ()), this, SLOT (browse ()));
  connect (colors, SIGNAL (color_changed (std::pair<QColor, QColor>)), false_color_control, SLOT (set_current_color (std::pair<QColor, QColor>)));
  connect (false_color_control, SIGNAL (selection_changed (std::pair<QColor, QColor>)), colors, SLOT (set_color (std::pair<QColor, QColor>)));
  connect (false_color_control, SIGNAL (selection_changed (std::pair<QColor, QColor>)), this, SLOT (color_mapping_changed ()));
  connect (false_color_control, SIGNAL (color_mapping_changed ()), this, SLOT (color_mapping_changed ()));

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

  connect (from_le, SIGNAL (editingFinished ()), this, SLOT (min_max_value_changed ()));
  connect (to_le, SIGNAL (editingFinished ()), this, SLOT (min_max_value_changed ()));
  connect (value_le, SIGNAL (editingFinished ()), this, SLOT (value_changed ()));

  connect (width_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (height_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (x_offset_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (y_offset_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (angle_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (shear_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (persp_tx_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (persp_ty_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));

  connect (mirror_cbx, SIGNAL (clicked ()), this, SIGNAL (edited ()));
  connect (reset_pb, SIGNAL (clicked ()), this, SLOT (reset_pressed ()));
  connect (save_pb, SIGNAL (clicked ()), this, SLOT (save_pressed ()));
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

size_t
PropertiesPage::count () const
{
  return m_selection.size ();
}

void
PropertiesPage::select_entries (const std::vector<size_t> &entries)
{
  tl_assert (entries.size () == 1);
  m_index = entries.front ();
  invalidate ();
}

std::string
PropertiesPage::description (size_t entry) const
{
  const img::Object *obj = dynamic_cast <const img::Object *> (m_selection [entry]->ptr ());
  if (! obj) {
    return std::string ("nil");
  }

  std::string d = tl::to_string (tr ("Image"));
  if (! obj->filename ().empty ()) {
    d += "[" + tl::filename (obj->filename ()) + "]";
  }
  d += tl::sprintf ("(%dx%d)", obj->width (), obj->height ());
  return d;
}

std::string
PropertiesPage::description () const
{
  return tl::to_string (tr ("Images"));
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
PropertiesPage::get_xmin_xmax (double &xmin, double &xmax, bool &has_error_out)
{
  bool has_error = false;

  try {
    tl::from_string_ext (tl::to_string (from_le->text ()), xmin);
    lay::indicate_error (from_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (from_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (to_le->text ()), xmax);
    lay::indicate_error (to_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (to_le, &ex);
    has_error = true;
  }

  if (! has_error && xmin >= xmax) {
    tl::Exception ex (tl::to_string (QObject::tr ("Invalid data value range (min. value must be less than max. value)")));
    lay::indicate_error (from_le, &ex);
    lay::indicate_error (to_le, &ex);
    has_error = true;
  }

  if (has_error) {
    has_error_out = true;
  }
}

void 
PropertiesPage::min_max_value_changed ()
{
  value_le->setText (QString ());
  value_le->setEnabled (false);

  colors->setEnabled (false_color_control->has_selection ());
  colors->set_single_mode (false);

  double xmin, xmax;
  bool has_error = false;
  get_xmin_xmax (xmin, xmax, has_error);

  if (has_error) {
    return;
  }

  if (false_color_control->has_selection () && false_color_control->selected_node () > 0 && false_color_control->selected_node () < int (false_color_control->nodes ().size ()) - 1) {

    double x = false_color_control->nodes () [false_color_control->selected_node ()].first;
    double xx = x * (xmax - xmin) + xmin;

    value_le->setText (tl::to_qstring (tl::sprintf ("%.4g", xx)));
    value_le->setEnabled (true);

  } else if (false_color_control->has_selection ()) {

    colors->set_single_mode (true);

  }

  recompute_histogram ();

  emit edited ();
}

void
PropertiesPage::color_mapping_changed ()
{
  if (! m_no_signals) {

    bool has_error = false;

    value_le->setText (QString ());
    value_le->setEnabled (false);

    colors->setEnabled (false_color_control->has_selection ());
    colors->set_single_mode (false);

    if (false_color_control->has_selection () && false_color_control->selected_node () > 0 && false_color_control->selected_node () < int (false_color_control->nodes ().size ()) - 1) {

      double xmin, xmax;
      get_xmin_xmax (xmin, xmax, has_error);

      if (! has_error) {

        double x = false_color_control->nodes () [false_color_control->selected_node ()].first;
        double xx = x * (xmax - xmin) + xmin;

        value_le->setText (tl::to_qstring (tl::sprintf ("%.4g", xx)));
        value_le->setEnabled (true);

      }

    } else if (false_color_control->has_selection ()) {

      colors->set_single_mode (true);

    }

    if (! has_error) {
      m_in_color_mapping_signal = true;
      emit edited ();
      m_in_color_mapping_signal = false;
    }

  }
}

void
PropertiesPage::value_changed ()
{
  double xx = 0;
  bool has_error = false;

  double xmin, xmax;
  get_xmin_xmax (xmin, xmax, has_error);

  double x = 0.0;
  try {
    tl::from_string_ext (tl::to_string (value_le->text ()), x);
    lay::indicate_error (value_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (value_le, &ex);
    has_error = true;
  }

  xx = (x - xmin) / (xmax - xmin);
  if (! has_error && (xx < 0 || xx > 1.0)) {
    tl::Exception ex (tl::to_string (QObject::tr ("The position entered (%g) must be between the minimum (%g) and maximum (%g) value")), x, xmin, xmax);
    lay::indicate_error (value_le, &ex);
    has_error = true;
  }

  if (! has_error) {

    m_no_signals = true;
    false_color_control->set_current_position (xx);
    m_no_signals = false;

    emit edited ();

  }
}

inline double 
round_to_zero (double x)
{
  return 1e-6 * floor (0.5 + 1e6 * x);
}

void 
PropertiesPage::update ()
{
  if (m_in_color_mapping_signal) {
    return;
  }

  m_no_signals = true;

  if (mp_service) {

    mp_service->highlight (m_index);

    //  create a local copy in which we can apply modifications
    if (! mp_direct_image) {
      const img::Object *image = dynamic_cast <const img::Object *> (m_selection [m_index]->ptr ());
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
    tl::from_string_ext (tl::to_string (from_le->text ()), xmin);
    tl::from_string_ext (tl::to_string (to_le->text ()), xmax);
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
  emit edited ();
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
  emit edited ();
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
  emit edited ();
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
  emit edited ();
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

  emit edited ();

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
  emit edited ();

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
  emit edited ();

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
  emit edited ();

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
  emit edited ();

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
  emit edited ();

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
  emit edited ();

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
  emit edited ();

  m_no_signals = false;  
}

void  
PropertiesPage::black_to_white ()
{
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > nodes;
  nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
  false_color_control->set_nodes (nodes);
  emit edited ();
}

void  
PropertiesPage::white_to_black ()
{
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > nodes;
  nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
  nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  false_color_control->set_nodes (nodes);
  emit edited ();
}

void  
PropertiesPage::red_to_blue ()
{
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > nodes;
  nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (255, 0, 0), tl::Color (255, 0, 0))));
  nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (0, 0, 255), tl::Color (0, 0, 255))));
  false_color_control->set_nodes (nodes);
  emit edited ();
}

void  
PropertiesPage::blue_to_red ()
{
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > nodes;
  nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 255), tl::Color (0, 0, 255))));
  nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 0, 0), tl::Color (255, 0, 0))));
  false_color_control->set_nodes (nodes);
  emit edited ();
}

void  
PropertiesPage::reverse_color_order ()
{
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > nodes (false_color_control->nodes ());
  for (size_t i = 0; i < nodes.size () / 2; ++i) {
    std::swap (nodes [i].second.second, nodes [nodes.size () - 1 - i].second.first);
    std::swap (nodes [i].second.first, nodes [nodes.size () - 1 - i].second.second);
  }
  false_color_control->set_nodes (nodes);
  emit edited ();
}

void 
PropertiesPage::apply ()
{
  bool has_error = false;

  db::Matrix3d matrix = mp_direct_image->matrix ();

  //  The observer distance for perspective distortion is the average of the images width and height.
  //  The coordinate space origin in the center of the image.
  double pw2 = std::max (0.5, 0.5 * matrix.mag_x () * mp_direct_image->width ());
  double ph2 = std::max (0.5, 0.5 * matrix.mag_y () * mp_direct_image->height ());
  double z = pw2 + ph2;

  double w = matrix.mag_x (), h = matrix.mag_y (), x = matrix.disp ().x (), y = matrix.disp ().y (), 
         a = matrix.angle (), sa = matrix.shear_angle (), tx = matrix.perspective_tilt_x (z), ty = matrix.perspective_tilt_y (z);

  try {
    tl::from_string_ext (tl::to_string (width_le->text ()), w);
    if (w <= 0.0 || h <= 0.0) {
      throw tl::Exception (tl::to_string (QObject::tr ("Pixel width or height must be positive, non-null values")));
    }
    lay::indicate_error (width_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (width_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (height_le->text ()), h);
    lay::indicate_error (height_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (height_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (x_offset_le->text ()), x);
    lay::indicate_error (x_offset_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (x_offset_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (y_offset_le->text ()), y);
    lay::indicate_error (y_offset_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (y_offset_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (angle_le->text ()), a);
    lay::indicate_error (angle_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (angle_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (shear_le->text ()), sa);
    if (sa <= -45 || sa >= 45) {
      throw tl::Exception (tl::to_string (QObject::tr ("The shear angle must be larger than -45 and less than 45 degree")));
    }
    lay::indicate_error (shear_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (shear_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (persp_tx_le->text ()), tx);
    if (tx <= -90 || tx >= 90) {
      throw tl::Exception (tl::to_string (QObject::tr ("The perspective tilt angles must be larger than -90 and less than 90 degree")));
    }
    lay::indicate_error (persp_tx_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (persp_tx_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (persp_ty_le->text ()), ty);
    if (ty <= -90 || ty >= 90) {
      throw tl::Exception (tl::to_string (QObject::tr ("The perspective tilt angles must be larger than -90 and less than 90 degree")));
    }
    lay::indicate_error (persp_ty_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (persp_ty_le, &ex);
    has_error = true;
  }

  bool mirror = mirror_cbx->isChecked ();

  double xmin, xmax;
  get_xmin_xmax (xmin, xmax, has_error);

  if (has_error) {
    throw tl::Exception (tl::to_string (tr ("At least one value is invalid - see highlighted entry fields")));
  }

  //  Compute the new observer distance
  pw2 = std::max (0.5, 0.5 * w * mp_direct_image->width ());
  ph2 = std::max (0.5, 0.5 * h * mp_direct_image->height ());
  z = pw2 + ph2;

  matrix = db::Matrix3d::disp (db::DVector (x, y)) * db::Matrix3d::perspective (tx, ty, z) * db::Matrix3d::rotation (a) * db::Matrix3d::shear (sa) * db::Matrix3d::mag (w, h) * db::Matrix3d::mirror (mirror);
  mp_direct_image->set_matrix (matrix);

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
    mp_service->change_image (m_selection [m_index], *mp_direct_image);
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
PropertiesPage::save_pressed ()
{
BEGIN_PROTECTED

  apply ();

  lay::FileDialog file_dialog (this, tl::to_string (QObject::tr ("Save As KLayout Image File")), tl::to_string (QObject::tr ("KLayout image files (*.lyimg);;All files (*)")));

  std::string filename = mp_direct_image->filename ();
  if (! filename.empty () && tl::extension (filename) != "lyimg") {
    filename = tl::basename (filename) + ".lyimg";
  }

  if (file_dialog.get_save (filename)) {

    tl::OutputFile file (filename);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, *mp_direct_image);

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

  emit edited ();
}

void
PropertiesPage::define_landmarks_pressed ()
{
  if (mp_direct_image) {
    img::LandmarksDialog dialog (this, *mp_direct_image);
    if (dialog.exec ()) {
      emit edited ();
    }
  }
}

}

#endif
