
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

#ifndef HDR_imgPropertiesPage
#define HDR_imgPropertiesPage

#include "layPlugin.h"
#include "layProperties.h"
#include "imgService.h"
#include "ui_ImagePropertiesPage.h"

namespace img
{

/**
 *  @brief The properties page for image objects.
 *
 *  This class is a reimplementation of lay::PropertiesPage but it can serve as
 *  a standlone properties page for embedding into a standalone dialog as well.
 */
class PropertiesPage
  : public lay::PropertiesPage,
    public Ui::ImagePropertiesPage
{
Q_OBJECT

public:
  PropertiesPage (img::Service *service, db::Manager *manager, QWidget *parent);
  PropertiesPage (QWidget *parent);
  ~PropertiesPage ();

  virtual size_t count () const;
  virtual void select_entries (const std::vector<size_t> &entries);
  virtual std::string description (size_t entry) const;
  virtual std::string description () const;
  virtual void update ();
  virtual void leave ();
  virtual bool readonly ();
  virtual void apply (); 

  void set_direct_image (img::Object *image);

private slots:
  void browse ();
  void value_changed ();
  void color_mapping_changed ();
  void brightness_slider_changed (int value);
  void brightness_spinbox_changed (int value);
  void contrast_slider_changed (int value);
  void contrast_spinbox_changed (int value);
  void gamma_slider_changed (int value);
  void gamma_spinbox_changed (double value);
  void red_slider_changed (int value);
  void red_spinbox_changed (double value);
  void green_slider_changed (int value);
  void green_spinbox_changed (double value);
  void blue_slider_changed (int value);
  void blue_spinbox_changed (double value);
  void black_to_white ();
  void white_to_black ();
  void red_to_blue ();
  void blue_to_red ();
  void reverse_color_order ();
  void min_max_value_changed ();
  void reset_pressed ();
  void save_pressed ();
  void define_landmarks_pressed ();

private:
  std::vector <img::Service::obj_iterator> m_selection;
  size_t m_index;
  img::Service *mp_service;
  img::Object *mp_direct_image;
  bool m_no_signals;
  bool m_in_color_mapping_signal;

  void recompute_histogram ();
  void invalidate ();
  void init ();
  void get_xmin_xmax (double &xmin, double &xmax, bool &has_error_out);
};

}

#endif

#endif
