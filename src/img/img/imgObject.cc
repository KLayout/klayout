
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


#include "imgObject.h"
#include "imgStream.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "layPlugin.h"
#include "layConverters.h"
#include "tlPixelBuffer.h"
#include "dbPolygonTools.h"
#include "tlFileUtils.h"
#include "tlUri.h"
#include "tlThreads.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <string>
#include <memory.h>

#if defined(HAVE_QT)
#  include <QImage>
#endif

namespace img
{

// --------------------------------------------------------------------------------------
//  img::DataMapping implementation

DataMapping::DataMapping ()
  : brightness (0.0), contrast (0.0), gamma (1.0), red_gain (1.0), green_gain (1.0), blue_gain (1.0)
{
  false_color_nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  false_color_nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
}

bool 
DataMapping::operator== (const DataMapping &d) const
{
  const double epsilon = 1e-6;

  if (fabs (brightness - d.brightness) > epsilon) {
    return false;
  }

  if (fabs (contrast - d.contrast) > epsilon) {
    return false;
  }

  if (fabs (gamma - d.gamma) > epsilon) {
    return false;
  }

  if (fabs (red_gain - d.red_gain) > epsilon) {
    return false;
  }

  if (fabs (green_gain - d.green_gain) > epsilon) {
    return false;
  }

  if (fabs (blue_gain - d.blue_gain) > epsilon) {
    return false;
  }

  if (false_color_nodes.size () != d.false_color_nodes.size ()) {
    return false;
  }

  for (unsigned int i = 0; i < false_color_nodes.size (); ++i) {
    if (fabs (false_color_nodes[i].first - d.false_color_nodes[i].first) > epsilon) {
      return false;
    }
    if (false_color_nodes[i].second.first != d.false_color_nodes[i].second.first) {
      return false;
    }
    if (false_color_nodes[i].second.second != d.false_color_nodes[i].second.second) {
      return false;
    }
  }

  return true;
}

bool 
DataMapping::operator< (const DataMapping &d) const
{
  const double epsilon = 1e-6;

  if (fabs (brightness - d.brightness) > epsilon) {
    return brightness < d.brightness;
  }

  if (fabs (contrast - d.contrast) > epsilon) {
    return contrast < d.contrast;
  }

  if (fabs (gamma - d.gamma) > epsilon) {
    return gamma < d.gamma;
  }

  if (fabs (red_gain - d.red_gain) > epsilon) {
    return red_gain < d.red_gain;
  }

  if (fabs (green_gain - d.green_gain) > epsilon) {
    return green_gain < d.green_gain;
  }

  if (fabs (blue_gain - d.blue_gain) > epsilon) {
    return blue_gain < d.blue_gain;
  }

  if (false_color_nodes.size () != d.false_color_nodes.size ()) {
    return false_color_nodes.size () < d.false_color_nodes.size ();
  }

  for (unsigned int i = 0; i < false_color_nodes.size (); ++i) {
    if (fabs (false_color_nodes[i].first - d.false_color_nodes[i].first) > epsilon) {
      return false_color_nodes[i].first < d.false_color_nodes[i].first;
    }
    if (false_color_nodes[i].second.first != d.false_color_nodes[i].second.first) {
      return false_color_nodes[i].second.first.rgb () < d.false_color_nodes[i].second.first.rgb ();
    }
    if (false_color_nodes[i].second.second != d.false_color_nodes[i].second.second) {
      return false_color_nodes[i].second.second.rgb () < d.false_color_nodes[i].second.second.rgb ();
    }
  }

  return false;
}

tl::DataMappingBase *
DataMapping::create_data_mapping (bool monochrome, double xmin, double xmax, unsigned int channel) const
{
  double scale = 1.0;
  if (channel == 0) {
    scale = red_gain;
  } else if (channel == 1) {
    scale = green_gain;
  } else if (channel == 2) {
    scale = blue_gain;
  }

  tl::TableDataMapping *linear = new tl::TableDataMapping ();
  double m = contrast < 0.0 ? 1.0 / (1.0 - contrast * 2.0) : 1.0 + contrast * 2.0;
  linear->push_back (0.0, 0.5 + m * (brightness - 1.0) * 0.5);
  linear->push_back (1.0, 0.5 + m * (brightness + 1.0) * 0.5);

  tl::TableDataMapping *x_norm = new tl::TableDataMapping ();
  x_norm->push_back (xmin, 0.0);
  x_norm->push_back (xmax, 1.0);

  tl::TableDataMapping *to_pixel = new tl::TableDataMapping ();

  int nslices = 32;
  for (int i = 0; i <= nslices; ++i) {

    double x = double (i) / double (nslices);
    double y = 255 * pow (x, gamma);

    to_pixel->push_back (x, y);

  }

  tl::DataMappingBase *dm = 0;

  if (monochrome && false_color_nodes.size () > 1) {

    tl::TableDataMapping *gray_to_color = new tl::TableDataMapping ();

    for (unsigned int i = 1; i < false_color_nodes.size (); ++i) {

      unsigned int h1, s1, v1;
      false_color_nodes [i - 1].second.second.get_hsv (h1, s1, v1);

      unsigned int h2, s2, v2;
      false_color_nodes [i].second.first.get_hsv (h2, s2, v2);

      int dh = int (h1) - int (h2);
      int ds = int (s1) - int (s2);
      int dv = int (v1) - int (v2);

      //  The number of steps is chosen such that the full HSV band divides into approximately 200 steps
      double nsteps = 0.5 * sqrt (double (dh * dh) + double (ds * ds) + double (dv * dv));
      int n = int (floor (nsteps + 1.0));
      double dx = (false_color_nodes [i].first - false_color_nodes [i - 1].first) / n;
      double x = false_color_nodes [i - 1].first;

      for (int j = 0; j < n; ++j) {

        tl::Color c = interpolated_color (false_color_nodes, x);

        double y = 0.0;
        if (channel == 0) {
          y = c.red ();
        } else if (channel == 1) {
          y = c.green ();
        } else if (channel == 2) {
          y = c.blue ();
        }

        gray_to_color->push_back (x, y / 255.0);

        x += dx;

      }

    }

    double ylast = 0.0;
    if (channel == 0) {
      ylast = false_color_nodes.back ().second.second.red ();
    } else if (channel == 1) {
      ylast = false_color_nodes.back ().second.second.green ();
    } else if (channel == 2) {
      ylast = false_color_nodes.back ().second.second.blue ();
    }

    gray_to_color->push_back (false_color_nodes.back ().first, ylast / 255.0);

    dm = new tl::CombinedDataMapping (
                to_pixel, 
                new tl::LinearCombinationDataMapping (
                  0.0, new tl::CombinedDataMapping (
                         linear, 
                         new tl::CombinedDataMapping (gray_to_color, x_norm)
                       ), scale
                )
              );

  } else {

    dm = new tl::CombinedDataMapping (
                to_pixel, 
                new tl::LinearCombinationDataMapping (
                  0.0, new tl::CombinedDataMapping (linear, x_norm), scale
                )
              );

  }

  return dm;
}

// --------------------------------------------------------------------------------------

namespace
{

struct compare_first_of_node
{
  bool operator() (const std::pair <double, std::pair<tl::Color, tl::Color> > &a, const std::pair <double, std::pair<tl::Color, tl::Color> > &b) const
  {
    return a.first < b.first;
  }
};

}

tl::Color
interpolated_color (const DataMapping::false_color_nodes_type &nodes, double x)
{
  if (nodes.size () < 1) {
    return tl::Color ();
  } else if (nodes.size () < 2) {
    return x < nodes[0].first ? nodes[0].second.first : nodes[0].second.second;
  } else {

    std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > >::const_iterator p = std::lower_bound (nodes.begin (), nodes.end (), std::make_pair (x, std::make_pair (tl::Color (), tl::Color ())), compare_first_of_node ());
    if (p == nodes.end ()) {
      return nodes.back ().second.second;
    } else if (p == nodes.begin ()) {
      return nodes.front ().second.first;
    } else {

      double x1 = p[-1].first;
      double x2 = p->first;

      unsigned int h1 = 0, s1 = 0, v1 = 0;
      p[-1].second.second.get_hsv (h1, s1, v1);

      unsigned int h2 = 0, s2 = 0, v2 = 0;
      p->second.first.get_hsv (h2, s2, v2);

      int h = int (0.5 + h1 + double(x - x1) * double (int (h2) - int (h1)) / double(x2 - x1));
      int s = int (0.5 + s1 + double(x - x1) * double (int (s2) - int (s1)) / double(x2 - x1));
      int v = int (0.5 + v1 + double(x - x1) * double (int (v2) - int (v1)) / double(x2 - x1));

      return tl::Color::from_hsv ((unsigned int) h, (unsigned int) s, (unsigned int) v);

    }

  }
}

// --------------------------------------------------------------------------------------
//  img::DataHeader definition and implementation

class DataHeader
{
public:
  DataHeader (size_t w, size_t h, bool color, bool bytes)
    : m_width (w), m_height (h), m_ref_count (0)
  {
    size_t n = m_width * m_height;

    mp_mask = 0;

    mp_data = 0;
    mp_byte_data = 0;
    for (unsigned int i = 0; i < 3; ++i) {
      mp_color_data [i] = 0;
      mp_color_byte_data [i] = 0;
    }

    if (color) {

      if (bytes) {

        for (unsigned int i = 0; i < 3; ++i) {
          mp_color_byte_data [i] = new unsigned char [n];
          for (size_t j = 0; j < n; ++j) {
            mp_color_byte_data [i][j] = 0;
          }
        }

      } else {

        for (unsigned int i = 0; i < 3; ++i) {
          mp_color_data [i] = new float [n];
          for (size_t j = 0; j < n; ++j) {
            mp_color_data [i][j] = 0.0;
          }
        }

      }

    } else {

      if (bytes) {

        mp_byte_data = new unsigned char [n];
        for (size_t j = 0; j < n; ++j) {
          mp_byte_data[j] = 0;
        }

      } else {

        mp_data = new float [n];
        for (size_t j = 0; j < n; ++j) {
          mp_data[j] = 0.0;
        }

      }

    }

  }

  DataHeader (size_t w, size_t h, unsigned char *data, unsigned char *mask = 0)
    : m_width (w), m_height (h), m_ref_count (0)
  {
    mp_mask = mask;
    mp_byte_data = data;
    mp_data = 0;
    for (unsigned int i = 0; i < 3; ++i) {
      mp_color_data [i] = 0;
      mp_color_byte_data [i] = 0;
    }
  }

  DataHeader (size_t w, size_t h, unsigned char *red, unsigned char *green, unsigned char *blue, unsigned char *mask = 0)
    : m_width (w), m_height (h), m_ref_count (0)
  {
    mp_mask = mask;
    mp_byte_data = 0;
    mp_data = 0;
    mp_color_byte_data[0] = red;
    mp_color_byte_data[1] = green;
    mp_color_byte_data[2] = blue;
    for (unsigned int i = 0; i < 3; ++i) {
      mp_color_data [i] = 0;
    }
  }

  DataHeader (size_t w, size_t h, float *data, unsigned char *mask = 0)
    : m_width (w), m_height (h), m_ref_count (0)
  {
    mp_mask = mask;
    mp_byte_data = 0;
    mp_data = data;
    for (unsigned int i = 0; i < 3; ++i) {
      mp_color_data [i] = 0;
      mp_color_byte_data [i] = 0;
    }
  }

  DataHeader (size_t w, size_t h, float *red, float *green, float *blue, unsigned char *mask = 0)
    : m_width (w), m_height (h), m_ref_count (0)
  {
    mp_mask = mask;
    mp_byte_data = 0;
    mp_data = 0;
    mp_color_data[0] = red;
    mp_color_data[1] = green;
    mp_color_data[2] = blue;
    for (unsigned int i = 0; i < 3; ++i) {
      mp_color_byte_data [i] = 0;
    }
  }

  void add_ref () 
  {
    ++m_ref_count;
  }

  void release_ref ()
  {
    --m_ref_count;
    if (! m_ref_count) {
      delete this;
    }
  }

  size_t width () const
  {
    return m_width;
  }

  size_t height () const
  {
    return m_height;
  }

  size_t data_length () const
  {
    return m_width * m_height;
  }

  unsigned char *mask ()
  {
    return mp_mask;
  }

  unsigned char *set_mask ()
  {
    if (! mp_mask) {
      size_t n = data_length ();
      mp_mask = new unsigned char [n];
      memset (mp_mask, true, n);
    }
    return mp_mask;
  }

  unsigned char *byte_data (unsigned int i)
  {
    return mp_color_byte_data [i];
  }

  float *float_data (unsigned int i)
  {
    return mp_color_data [i];
  }

  unsigned char *byte_data ()
  {
    return mp_byte_data;
  }

  float *float_data ()
  {
    return mp_data;
  }

  bool has_mask () const
  {
    return mp_mask != 0;
  }

  bool is_byte_data () const
  {
    return mp_byte_data != 0 || mp_color_byte_data [0] != 0;
  }

  bool is_color () const
  {
    return mp_color_data [0] != 0 || mp_color_byte_data [0] != 0;
  }

  bool less (const DataHeader &d) const
  {
    if (m_width != d.m_width) {
      return (m_width < d.m_width);
    }
    if (m_height != d.m_height) {
      return (m_height < d.m_height);
    }

    if (has_mask () != d.has_mask ()) {
      return has_mask () < d.has_mask ();
    }
    if (has_mask ()) {
      size_t n = data_length ();
      for (size_t j = 0; j < n; ++j) {
        if (mp_mask [j] != d.mp_mask [j]) {
          return (mp_mask [j] < d.mp_mask [j]);
        }
      }
    }

    if (is_color () != d.is_color ()) {
      return is_color () < ! d.is_color ();
    }
    if (is_byte_data () != d.is_byte_data ()) {
      return is_byte_data () < ! d.is_byte_data ();
    }

    if (is_byte_data ()) {

      if (is_color ()) {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          for (unsigned int i = 0; i < 3; ++i) {
            if (mp_color_byte_data [i][j] != d.mp_color_byte_data [i][j]) {
              return (mp_color_byte_data [i][j] < d.mp_color_byte_data [i][j]);
            }
          }
        }

      } else {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          if (mp_byte_data [j] != d.mp_byte_data [j]) {
            return (mp_byte_data [j] < d.mp_byte_data [j]);
          }
        }

      } 

    } else {

      if (is_color ()) {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          for (unsigned int i = 0; i < 3; ++i) {
            if (mp_color_data [i][j] != d.mp_color_data [i][j]) {
              return (mp_color_data [i][j] < d.mp_color_data [i][j]);
            }
          }
        }

      } else {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          if (mp_data [j] != d.mp_data [j]) {
            return (mp_data [j] < d.mp_data [j]);
          }
        }

      } 

    }

    return false;
  }

  bool equals (const DataHeader &d) const
  {
    if (m_width != d.m_width) {
      return false;
    }
    if (m_height != d.m_height) {
      return false;
    }

    if (has_mask () != d.has_mask ()) {
      return false;
    }
    if (has_mask ()) {
      size_t n = data_length ();
      for (size_t j = 0; j < n; ++j) {
        if (mp_mask [j] != d.mp_mask [j]) {
          return false;
        }
      }
    }

    if (is_color () != d.is_color ()) {
      return false;
    }
    if (is_byte_data () != d.is_byte_data ()) {
      return false;
    }

    if (is_byte_data ()) {

      if (is_color ()) {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          for (unsigned int i = 0; i < 3; ++i) {
            if (mp_color_byte_data [i][j] != d.mp_color_byte_data [i][j]) {
              return false;
            }
          }
        }

      } else {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          if (mp_byte_data [j] != d.mp_byte_data [j]) {
            return false;
          }
        }

      } 

    } else {

      if (is_color ()) {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          for (unsigned int i = 0; i < 3; ++i) {
            if (mp_color_data [i][j] != d.mp_color_data [i][j]) {
              return false;
            }
          }
        }

      } else {

        size_t n = data_length ();
        for (size_t j = 0; j < n; ++j) {
          if (mp_data [j] != d.mp_data [j]) {
            return false;
          }
        }

      } 

    }

    return true;
  }

  void mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    size_t n = data_length ();
    for (unsigned int i = 0; i < 3; ++i) {
      if (mp_color_data[i]) {
        stat->add (typeid (float []), (void *) mp_color_data[i], n * sizeof (float), n * sizeof (float), (void *) this, purpose, cat);
      }
      if (mp_color_byte_data[i]) {
        stat->add (typeid (unsigned char []), (void *) mp_color_byte_data[i], n * sizeof (unsigned char), n * sizeof (unsigned char), (void *) this, purpose, cat);
      }
    }

    if (mp_mask) {
      stat->add (typeid (unsigned char []), (void *) mp_mask, n * sizeof (unsigned char), n * sizeof (unsigned char), (void *) this, purpose, cat);
    }
    if (mp_data) {
      stat->add (typeid (float []), (void *) mp_data, n * sizeof (float), n * sizeof (float), (void *) this, purpose, cat);
    }
    if (mp_byte_data) {
      stat->add (typeid (unsigned char []), (void *) mp_byte_data, n * sizeof (unsigned char), n * sizeof (unsigned char), (void *) this, purpose, cat);
    }
  }

private:
  DataHeader (const DataHeader &);
  DataHeader &operator= (const DataHeader &);

  ~DataHeader () 
  { 
    if (mp_mask) {
      delete [] mp_mask;
      mp_mask = 0;
    }

    if (mp_data) {
      delete [] mp_data;
      mp_data = 0;
    }
    if (mp_byte_data) {
      delete [] mp_byte_data;
      mp_byte_data = 0;
    }
    
    for (unsigned int i = 0; i < 3; ++i) {
      if (mp_color_data [i]) {
        delete [] mp_color_data [i];
        mp_color_data [i] = 0;
      }
      if (mp_color_byte_data [i]) {
        delete [] mp_color_byte_data [i];
        mp_color_byte_data [i] = 0;
      }
    } 
  }

  size_t m_width, m_height;
  float *mp_color_data[3];
  float *mp_data;
  unsigned char *mp_mask;
  unsigned char *mp_color_byte_data[3];
  unsigned char *mp_byte_data;
  int m_ref_count;
};


// --------------------------------------------------------------------------------------
//  img::Object implementation

static size_t make_id ()
{
  static tl::Mutex id_lock;
  static size_t s_id_counter = 1;

  //  Get a new Id for the object. Id == 0 is reserved.
  id_lock.lock ();
  size_t id = s_id_counter;
  do {
    ++s_id_counter;
  } while (s_id_counter == 0);
  id_lock.unlock ();

  return id;
}

Object::Object ()
  : m_trans (1.0), mp_data (0), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, bool color, bool byte_data)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = new DataHeader (w, h, color, byte_data);
  mp_data->add_ref ();
  clear ();
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, unsigned char *d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (255.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, float *d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector <double> &d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, unsigned char *red, unsigned char *green, unsigned char *blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, float *red, float *green, float *blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector <double> &red, const std::vector <double> &green, const std::vector <double> &blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (const std::string &filename, const db::DCplxTrans &trans)
  : m_filename (filename), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  read_file ();
  m_updates_enabled = true;
}

Object::Object (const tl::PixelBuffer &pixel_buffer, const db::DCplxTrans &trans)
  : m_filename ("<object>"), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  create_from_pixel_buffer (pixel_buffer);
  m_updates_enabled = true;
}

#if defined(HAVE_QT)
Object::Object (const QImage &qimage, const db::DCplxTrans &trans)
  : m_filename ("<object>"), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  create_from_qimage (qimage);
  m_updates_enabled = true;
}
#endif

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, bool color, bool byte_data)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = new DataHeader (w, h, color, byte_data);
  mp_data->add_ref ();
  clear ();
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, unsigned char *d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, float *d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, const std::vector <double> &d)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, d);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, unsigned char *red, unsigned char *green, unsigned char *blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, float *red, float *green, float *blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (size_t w, size_t h, const db::Matrix3d &trans, const std::vector <double> &red, const std::vector <double> &green, const std::vector <double> &blue)
  : m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;
  mp_data = 0;
  set_data (w, h, red, green, blue);
  m_updates_enabled = true;
}

Object::Object (const std::string &filename, const db::Matrix3d &trans)
  : m_filename (filename), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  read_file ();
  m_updates_enabled = true;
}

Object::Object (const tl::PixelBuffer &pixel_buffer, const db::Matrix3d &trans)
  : m_filename ("<object>"), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  create_from_pixel_buffer (pixel_buffer);
  read_file ();
  m_updates_enabled = true;
}

#if defined(HAVE_QT)
Object::Object (const QImage &qimage, const db::Matrix3d &trans)
  : m_filename ("<object>"), m_trans (trans), m_id (make_id ()), m_min_value (0.0), m_max_value (1.0), m_min_value_set (false), m_max_value_set (false), m_visible (true), m_z_position (0)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  create_from_qimage (qimage);
  m_updates_enabled = true;
}
#endif

Object::Object (const img::Object &d)
{
  m_updates_enabled = false;
  mp_pixel_data = 0;

  mp_data = 0;
  *this = d;
  m_updates_enabled = true;
}

Object::~Object ()
{
  release ();
}

Object &
Object::operator= (const img::Object &d)
{
  if (this != &d) {

    release ();

    m_landmarks = d.m_landmarks;

    m_trans = d.m_trans;
    m_filename = d.m_filename;

    mp_data = d.mp_data;
    if (mp_data) {
      mp_data->add_ref ();
    }

    m_id = d.m_id;
    m_data_mapping = d.m_data_mapping;

    m_visible = d.m_visible;
    m_z_position = d.m_z_position;

    m_min_value = d.m_min_value;
    m_min_value_set = d.m_min_value_set;
    m_max_value = d.m_max_value;
    m_max_value_set = d.m_max_value_set;

    if (m_updates_enabled) {
      property_changed ();
    }

  }

  return *this;
}

bool 
Object::equals (const db::DUserObjectBase *d) const
{
  const img::Object *img_object = dynamic_cast <const img::Object *> (d);
  return img_object != 0 && *this == *img_object;
}

bool 
Object::less (const db::DUserObjectBase *d) const
{
  const img::Object *img_object = dynamic_cast <const img::Object *> (d);
  tl_assert (img_object != 0);

  if (m_z_position != img_object->m_z_position) {
    return m_z_position < img_object->m_z_position;
  }

  double epsilon = (std::abs (m_min_value) + std::abs (m_max_value)) * 1e-6;
  if (std::abs (m_min_value - img_object->m_min_value) > epsilon) {
    return m_min_value < img_object->m_min_value;
  }
  if (std::abs (m_max_value - img_object->m_max_value) > epsilon) {
    return m_max_value < img_object->m_max_value;
  }

  if (! (m_data_mapping == img_object->m_data_mapping)) {
    return m_data_mapping < img_object->m_data_mapping;
  }

  if (m_visible != img_object->m_visible) {
    return m_visible < img_object->m_visible;
  }

  if (! m_trans.equal (img_object->m_trans)) {
    return m_trans.less (img_object->m_trans);
  }

  if (m_landmarks.size () != img_object->m_landmarks.size ()) {
    return m_landmarks.size () < img_object->m_landmarks.size ();
  }
  for (size_t i = 0; i < m_landmarks.size (); ++i) {
    if (! m_landmarks [i].equal (img_object->m_landmarks [i])) {
      return m_landmarks [i].less (img_object->m_landmarks [i]);
    }
  }

  if (mp_data != img_object->mp_data) {
    if ((mp_data == 0) != (img_object->mp_data == 0)) {
      return ((mp_data == 0) < (img_object->mp_data == 0));
    }
    if (mp_data != 0) {
      return mp_data->less (*img_object->mp_data);
    } else {
      return false;
    }
  }

  return false;
}

bool 
Object::operator== (const img::Object &d) const
{
  if (m_z_position != d.m_z_position) {
    return false;
  }

  //  operator== is all fuzzy compare - 
  double epsilon = (std::abs (m_min_value) + std::abs (m_max_value)) * 1e-6;
  if (std::abs (m_min_value - d.m_min_value) > epsilon) {
    return false;
  }
  if (std::abs (m_max_value - d.m_max_value) > epsilon) {
    return false;
  }

  if (! (m_data_mapping == d.m_data_mapping)) {
    return false;
  }

  if (m_visible != d.m_visible) {
    return false;
  }

  if (! m_trans.equal (d.m_trans)) {
    return false;
  }

  if (m_landmarks.size () != d.m_landmarks.size ()) {
    return false;
  }
  for (size_t i = 0; i < m_landmarks.size (); ++i) {
    if (! m_landmarks [i].equal (d.m_landmarks [i])) {
      return false;
    }
  }

  if (mp_data != d.mp_data) {
    if ((mp_data == 0) != (d.mp_data == 0)) {
      return false;
    }
    if (mp_data != 0) {
      return mp_data->equals (*d.mp_data);
    } else {
      return true;
    }
  }

  return true;
}

unsigned int 
Object::class_id () const
{
  static unsigned int cid = db::get_unique_user_object_class_id ();
  return cid;
}

db::DUserObjectBase *
Object::clone () const
{
  return new img::Object (*this);
}

void
Object::clear ()
{
  if (is_byte_data ()) {

    if (is_color ()) {

      for (unsigned int c = 0; c < 3; ++c) {
        unsigned char *d = mp_data->byte_data (c);
        for (size_t i = data_length (); i > 0; --i) {
          *d++ = 0.0;
        }
      }

    } else {

      unsigned char *d = mp_data->byte_data ();
      for (size_t i = data_length (); i > 0; --i) {
        *d++ = 0.0;
      }

    }

  } else if (is_color ()) {

    for (unsigned int c = 0; c < 3; ++c) {
      float *d = mp_data->float_data (c);
      for (size_t i = data_length (); i > 0; --i) {
        *d++ = 0.0;
      }
    }

  } else {

    float *d = mp_data->float_data ();
    for (size_t i = data_length (); i > 0; --i) {
      *d++ = 0.0;
    }

  }
}

db::DPolygon
Object::image_box_poly (const db::DBox vp, const db::DCplxTrans &vpt) const
{
  db::Matrix3d t = db::Matrix3d (vpt) * matrix ();
  db::Matrix3d ti = t.inverted ();

  std::vector<db::DPoint> pb;
  pb.reserve (4);
  pb.push_back (db::DPoint (vp.left (), vp.bottom ()));
  pb.push_back (db::DPoint (vp.left (), vp.top ()));
  pb.push_back (db::DPoint (vp.right (), vp.top ()));
  pb.push_back (db::DPoint (vp.right (), vp.bottom ()));

  int iinside = -1;
  for (unsigned int i = 0; i < 4; ++i) {
    if (ti.can_transform (pb[i])) {
      iinside = int(i);
      break;
    }
  }

  if (iinside < 0) {
    return db::DPolygon ();
  }

  db::DPolygon image_box_poly (db::DBox (-0.5 * width (), -0.5 * height (), 0.5 * width (), 0.5 * height ()));

  //  clip the image box at the transformed viewport edges

  //  determine the orientation of the viewport edges
  db::DVector v1 (ti.trans (pb[iinside], pb[(iinside + 3) % 4] - pb[iinside]));
  db::DVector v2 (ti.trans (pb[iinside], pb[(iinside + 1) % 4] - pb[iinside]));
  bool mirrored = (db::vprod_sign (v1, v2) < 0);

  for (unsigned int i = 0; i < 4; ++i) {

    unsigned int ii = (i + 1) % 4;

    if (ti.can_transform (pb[i]) || ti.can_transform (pb[ii])) {

      db::DPoint p1;
      db::DVector pv;
      if (ti.can_transform (pb[i])) {
        p1 = ti.trans (pb [i]);
        pv = ti.trans (pb[i], pb[ii] - pb[i]);
      } else {
        p1 = ti.trans (pb [ii]);
        pv = ti.trans (pb[ii], pb[ii] - pb[i]);
      }

      db::DEdge e (p1, p1 + pv);
      if (mirrored) {
        e.swap_points ();
      }

      std::list<db::DPolygon> cp;
      db::cut_polygon (image_box_poly, e, std::front_inserter (cp));
      if (cp.empty ()) {
        //  indicates that the polygon vanished
        return db::DPolygon ();
      } else {
        image_box_poly = cp.front ();
      }

    }

  }

  return image_box_poly.transformed (t);
}

db::DBox 
Object::box () const
{
  double w = double (width ());
  double h = double (height ());

  db::DBox b;
  b += m_trans * db::DPoint (-w * 0.5, -h * 0.5);
  b += m_trans * db::DPoint (w * 0.5, -h * 0.5);
  b += m_trans * db::DPoint (-w * 0.5, h * 0.5);
  b += m_trans * db::DPoint (w * 0.5, h * 0.5);

  // include landmarks
  for (std::vector <db::DPoint>::const_iterator l = m_landmarks.begin (); l != m_landmarks.end (); ++l) {
    
    b += m_trans * *l;
  }
  
  return b;
}

void 
Object::transform (const db::Matrix3d &t)
{
  m_trans = t * m_trans;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::transform (const db::DCplxTrans &t)
{
  m_trans = db::Matrix3d (t) * m_trans;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::transform (const db::DTrans &t)
{
  m_trans = db::Matrix3d (t) * m_trans;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::transform (const db::DFTrans &t)
{
  m_trans = db::Matrix3d (t) * m_trans;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::from_string (const char *str, const char *base_dir)
{
  bool en = m_updates_enabled;
  m_updates_enabled = false;

  try {

    tl::Extractor ex (str);

    *this = img::Object ();

    bool color = true;

    if (ex.test ("empty:")) {
      return;
    } else if (ex.test ("color:")) {
      color = true;
    } else if (ex.test ("mono:")) {
      color = false;
    }

    size_t w = 0;
    size_t h = 0;

    db::DCplxTrans tr;
    double pw = 1.0, ph = 1.0;
    bool compatibility_mode = false;

    while (! ex.at_end ()) {

      if (ex.test ("trans=")) {
        ex.read (tr);
        compatibility_mode = true;
      } else if (ex.test ("matrix=")) {
        ex.read (m_trans);
      } else if (ex.test ("pixel_width=")) {
        ex.read (pw);
        compatibility_mode = true;
      } else if (ex.test ("pixel_height=")) {
        ex.read (ph);
        compatibility_mode = true;
      } else if (ex.test ("brightness=")) {
        ex.read (m_data_mapping.brightness);
      } else if (ex.test ("contrast=")) {
        ex.read (m_data_mapping.contrast);
      } else if (ex.test ("gamma=")) {
        ex.read (m_data_mapping.gamma);
      } else if (ex.test ("red_gain=")) {
        ex.read (m_data_mapping.red_gain);
      } else if (ex.test ("green_gain=")) {
        ex.read (m_data_mapping.green_gain);
      } else if (ex.test ("blue_gain=")) {
        ex.read (m_data_mapping.blue_gain);
      } else if (ex.test ("color_mapping=")) {

        ex.test("[");

        double x = 0.0;
        lay::ColorConverter cc;
        tl::Color cl, cr;
        std::string s;

        m_data_mapping.false_color_nodes.clear ();

        while (! ex.at_end () && ! ex.test ("]")) {

          ex.read (x);

          ex.test (",");

          s.clear ();
          ex.read_word_or_quoted (s);
          cc.from_string (s, cl);

          if (ex.test (",")) {
            s.clear ();
            ex.read_word_or_quoted (s);
            cc.from_string (s, cr);
          } else {
            cr = cl;
          }

          m_data_mapping.false_color_nodes.push_back (std::make_pair (x, std::make_pair (cl, cr)));

          ex.test (";");

        }

      } else if (ex.test ("width=")) {
        ex.read (w);
      } else if (ex.test ("height=")) {
        ex.read (h);
      } else if (ex.test ("is_visible=")) {
        ex.read (m_visible);
      } else if (ex.test ("z_position=")) {
        ex.read (m_z_position);
      } else if (ex.test ("min_value=")) {
        ex.read (m_min_value);
        m_min_value_set = true;
      } else if (ex.test ("max_value=")) {
        ex.read (m_max_value);
        m_max_value_set = true;
      } else if (ex.test ("landmarks=")) {

        ex.test ("[");
        m_landmarks.clear ();
        while (! ex.at_end () && ! ex.test ("]")) {
          db::DPoint l;
          ex.read (l);
          m_landmarks.push_back (l);
          ex.test (",");
        }

      } else if (ex.test ("file=")) {

        ex.read_word_or_quoted (m_filename);

        tl::URI fp_uri (m_filename);
        if (base_dir && ! tl::is_absolute (fp_uri.path ())) {
          m_filename = tl::URI (base_dir).resolved (fp_uri).to_abstract_path ();
        }

        read_file ();

      } else if (ex.test ("byte_data=")) {

        release ();
        mp_data = new DataHeader (w, h, color, true);
        mp_data->add_ref ();

        size_t n = data_length ();

        ex.test ("[");

        unsigned int d;
        size_t i = 0;
        while (ex.try_read (d)) {

          if (color) {
            if (i < n) {
              mp_data->byte_data (0)[i] = d;
            }
            ex.test (",");
            ex.read (d);
            if (i < n) {
              mp_data->byte_data (1)[i] = d;
            }
            ex.test (",");
            ex.read (d);
            if (i < n) {
              mp_data->byte_data (2)[i] = d;
            }
          } else if (i < n) {
            mp_data->byte_data ()[i] = d;
          }

          if (ex.test (",")) {
            unsigned int m = 0;
            ex.read (m);
            mp_data->set_mask ()[i] = m;
          }

          ++i;

          ex.test (";");

        }

        ex.test ("]");

      } else if (ex.test ("data=")) {

        release ();
        mp_data = new DataHeader (w, h, color, false);
        mp_data->add_ref ();

        size_t n = data_length ();

        ex.test ("[");

        double d;
        size_t i = 0;
        while (ex.try_read (d)) {

          if (color) {
            if (i < n) {
              mp_data->float_data (0)[i] = d;
            }
            ex.test (",");
            ex.read (d);
            if (i < n) {
              mp_data->float_data (1)[i] = d;
            }
            ex.test (",");
            ex.read (d);
            if (i < n) {
              mp_data->float_data (2)[i] = d;
            }
          } else if (i < n) {
            mp_data->float_data ()[i] = d;
          }

          if (ex.test (",")) {
            unsigned int m = 0;
            ex.read (m);
            mp_data->set_mask ()[i] = m;
          }

          ++i;

          ex.test (";");

        }

        ex.test ("]");

      }

      ex.test (";");

    }

    if (compatibility_mode) {
      m_trans = db::Matrix3d (tr) * db::Matrix3d::mag (pw, ph) * db::Matrix3d::disp (db::DVector (0.5 * width (), 0.5 * height ()));
    }

    if (en) {
      m_updates_enabled = en;
      property_changed ();
    }

  } catch (...) {
    m_updates_enabled = en;
    throw;
  }
}

void 
Object::load_data (const std::string &filename, bool adjust_min_max)
{
  m_min_value_set = ! adjust_min_max;
  m_max_value_set = ! adjust_min_max;

  m_filename = tl::absolute_file_path (filename);

  read_file ();

  m_min_value_set = true;
  m_max_value_set = true;

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::read_file () 
{
  release ();

  if (tl::verbosity () >= 30) {
    tl::info << "Reading image file " << m_filename;
  }

  try {

    tl::InputFile file (m_filename);
    tl::InputStream stream (file);
    std::unique_ptr<img::Object> read;
    read.reset (img::ImageStreamer::read (stream));
    read->m_filename = m_filename;

    //  for now we need to copy here ...
    *this = *read;

    //  exit on success
    return;

  } catch (...) {
    //  continue with other formats ...
  }

#if defined(HAVE_QT)

  QImage qimage (tl::to_qstring (m_filename));
  create_from_qimage (qimage);

#elif defined(HAVE_PNG)

  tl::PixelBuffer img;

  {
    tl::InputStream stream (m_filename);
    img = tl::PixelBuffer::read_png (stream);
  }

  create_from_pixel_buffer (img);

#else
  throw tl::Exception ("No PNG support compiled in - cannot load PNG files");
#endif
}

#if defined(HAVE_QT)
void
Object::create_from_qimage (const QImage &qimage)
{
  if (qimage.isNull ()) {
    return;
  }

  if (! m_min_value_set) {
    m_min_value = 0.0;
  }

  if (! m_max_value_set) {
    m_max_value = 255.0;
  }

  m_min_value_set = true;
  m_max_value_set = true;

  size_t w = qimage.width (), h = qimage.height ();

  mp_data = new DataHeader (w, h, ! qimage.isGrayscale (), true);
  mp_data->add_ref ();

  size_t i = 0;

  if (is_color ()) {

    unsigned char *red   = mp_data->byte_data (0);
    unsigned char *green = mp_data->byte_data (1);
    unsigned char *blue  = mp_data->byte_data (2);
    unsigned char *msk   = qimage.hasAlphaChannel () ? mp_data->set_mask () : 0;

    for (size_t y = 0; y < h; ++y) {
      for (size_t x = 0; x < w; ++x) {
        QRgb rgb = qimage.pixel (QPoint (int (x), int (h - y - 1)));
        red[i] = qRed (rgb);
        green[i] = qGreen (rgb);
        blue[i] = qBlue (rgb);
        if (msk) {
          msk[i] = qAlpha (rgb) > 128;
        }
        ++i;
      }
    }

  } else {

    unsigned char *d = mp_data->byte_data ();
    unsigned char *msk = qimage.hasAlphaChannel () ? mp_data->set_mask () : 0;

    for (size_t y = 0; y < h; ++y) {
      for (size_t x = 0; x < w; ++x) {
        QRgb rgb = qimage.pixel (QPoint (int (x), int (h - y - 1)));
        *d++ = qGreen (rgb);
        if (msk) {
          msk[i] = qAlpha (rgb) > 128;
        }
      }
    }

  }

}
#endif

void
Object::create_from_pixel_buffer (const tl::PixelBuffer &img)
{
  bool is_color = false;
  for (unsigned int i = 0; i < img.height () && ! is_color; ++i) {
    const tl::color_t *d = img.scan_line (i);
    const tl::color_t *dd = d + img.width ();
    while (! is_color && d != dd) {
      tl::color_t c = *d++;
      is_color = (((c >> 8) ^ c) & 0xffff) != 0;
    }
  }

  if (! m_min_value_set) {
    m_min_value = 0.0;
  }

  if (! m_max_value_set) {
    m_max_value = 255.0;
  }

  m_min_value_set = true;
  m_max_value_set = true;

  unsigned int w = img.width (), h = img.height ();

  mp_data = new DataHeader (w, h, is_color, true);
  mp_data->add_ref ();

  if (is_color) {

    unsigned char *red   = mp_data->byte_data (0);
    unsigned char *green = mp_data->byte_data (1);
    unsigned char *blue  = mp_data->byte_data (2);
    unsigned char *msk   = img.transparent () ? mp_data->set_mask () : 0;

    for (unsigned int y = 0; y < h; ++y) {
      const tl::color_t *d = img.scan_line (h - y - 1);
      const tl::color_t *dd = d + img.width ();
      while (d != dd) {
        tl::color_t rgb = *d++;
        *red++ = tl::red (rgb);
        *green++ = tl::green (rgb);
        *blue++ = tl::blue (rgb);
        if (msk) {
          *msk++ = tl::alpha (rgb) > 128;
        }
      }
    }

  } else {

    unsigned char *mono = mp_data->byte_data ();
    unsigned char *msk = img.transparent () ? mp_data->set_mask () : 0;

    for (unsigned int y = 0; y < h; ++y) {
      const tl::color_t *d = img.scan_line (h - y - 1);
      const tl::color_t *dd = d + img.width ();
      while (d != dd) {
        tl::color_t rgb = *d++;
        *mono++ = tl::green (rgb);
        if (msk) {
          *msk++ = tl::alpha (rgb) > 128;
        }
      }
    }

  }
}

void 
Object::release () 
{
  if (mp_data) {
    mp_data->release_ref ();
    mp_data = 0;
  }

  invalidate_pixel_data ();
}

std::string 
Object::to_string () const
{
  std::stringstream os (std::stringstream::out);
  if (is_empty ()) {
    os << "empty:";
  } else {

    if (is_color ()) {
      os << "color:";
    } else {
      os << "mono:";
    }

    os << "matrix=";
    os << m_trans.to_string ();
    os << ";";
  
    os << "min_value=";
    os << tl::to_string (m_min_value);
    os << ";";
  
    os << "max_value=";
    os << tl::to_string (m_max_value);
    os << ";";
  
    os << "is_visible=";
    os << tl::to_string (m_visible);
    os << ";";

    os << "z_position=";
    os << tl::to_string (m_z_position);
    os << ";";

    os << "brightness=";
    os << tl::to_string (data_mapping ().brightness);
    os << ";";

    os << "contrast=";
    os << tl::to_string (data_mapping ().contrast);
    os << ";";

    os << "gamma=";
    os << tl::to_string (data_mapping ().gamma);
    os << ";";

    os << "red_gain=";
    os << tl::to_string (data_mapping ().red_gain);
    os << ";";

    os << "green_gain=";
    os << tl::to_string (data_mapping ().green_gain);
    os << ";";

    os << "blue_gain=";
    os << tl::to_string (data_mapping ().blue_gain);
    os << ";";

    if (! m_landmarks.empty ()) {
      os << "landmarks=[";
      for (std::vector <db::DPoint>::const_iterator l = m_landmarks.begin (); l != m_landmarks.end (); ++l) {
        if (l != m_landmarks.begin ()) {
          os << ",";
        }
        os << l->to_string ();
      }
      os << "];";
    }

    os << "color_mapping=[";

    lay::ColorConverter cc;
    for (unsigned int i = 0; i < data_mapping ().false_color_nodes.size (); ++i) {
      os << data_mapping ().false_color_nodes[i].first;
      os << ",";
      const std::pair<tl::Color, tl::Color> &clr = data_mapping ().false_color_nodes[i].second;
      os << tl::to_word_or_quoted_string (cc.to_string (clr.first));
      if (clr.first != clr.second) {
        os << ",";
        os << tl::to_word_or_quoted_string (cc.to_string (clr.second));
      }
      os << ";";
    }

    os << "];";

    if (m_filename.empty ()) {

      os << "width=";
      os << tl::to_string (width ());
      os << ";";
  
      os << "height=";
      os << tl::to_string (height ());
      os << ";";
  
      if (is_byte_data ()) {

        os << "byte_data=[";
        size_t n = data_length ();
        if (is_color ()) {
          for (size_t i = 0; i < n; ++i) {
            os << ((unsigned int) byte_data (0)[i]) << ","
               << ((unsigned int) byte_data (1)[i]) << ","
               << ((unsigned int) byte_data (2)[i]);
            if (mask ()) {
              os << "," << (unsigned int) mask ()[i];
            }
            os << ";";
          }
        } else {
          for (size_t i = 0; i < n; ++i) {
            os << ((unsigned int) byte_data ()[i]);
            if (mask ()) {
              os << "," << (unsigned int) mask ()[i];
            }
            os << ";";
          }
        }
        os << "]";

      } else {

        os << "data=[";
        size_t n = data_length ();
        if (is_color ()) {
          for (size_t i = 0; i < n; ++i) {
            os << tl::to_string (float_data (0)[i]) << ","
               << tl::to_string (float_data (1)[i]) << ","
               << tl::to_string (float_data (2)[i]);
            if (mask ()) {
              os << "," << (unsigned int) mask ()[i];
            }
            os << ";";
          }
        } else {
          for (size_t i = 0; i < n; ++i) {
            os << tl::to_string (float_data ()[i]);
            if (mask ()) {
              os << "," << (unsigned int) mask ()[i];
            }
            os << ";";
          }
        }
        os << "]";

      }

    } else {
      os << "file=" + tl::to_word_or_quoted_string (m_filename);
    }

  }
  
  return os.str ();
}

void
Object::swap (Object &other)
{
  m_filename.swap (other.m_filename);
  std::swap (m_trans, other.m_trans);
  std::swap (mp_data, other.mp_data);
  std::swap (m_id, other.m_id);
  std::swap (m_min_value, other.m_min_value);
  std::swap (m_max_value, other.m_max_value);
  std::swap (m_min_value_set, other.m_min_value_set);
  std::swap (m_max_value_set, other.m_max_value_set);
  std::swap (m_data_mapping, other.m_data_mapping);
  std::swap (m_visible, other.m_visible);
  std::swap (mp_pixel_data, other.mp_pixel_data);
  m_landmarks.swap (other.m_landmarks);
  std::swap (m_z_position, other.m_z_position);
  std::swap (m_updates_enabled, other.m_updates_enabled);
}

size_t 
Object::width () const
{
  return mp_data ? mp_data->width () : 0;
}

size_t 
Object::height () const
{
  return mp_data ? mp_data->height () : 0;
}

size_t 
Object::data_length () const
{
  return mp_data ? mp_data->data_length () : 0;
}

bool 
Object::is_empty () const
{
  return mp_data == 0;
}

bool 
Object::is_byte_data () const
{
  return mp_data ? mp_data->is_byte_data () : false;
}

bool 
Object::is_color () const
{
  return mp_data ? mp_data->is_color () : false;
}

const unsigned char *
Object::mask () const
{
  return mp_data ? mp_data->mask () : 0;
}

const unsigned char *
Object::byte_data () const
{
  return mp_data ? mp_data->byte_data () : 0;
}

const unsigned char *
Object::byte_data (unsigned int component) const
{
  tl_assert (component < 3);
  return mp_data ? mp_data->byte_data (component) : 0;
}

const float *
Object::float_data () const
{
  return mp_data ? mp_data->float_data () : 0;
}

const float *
Object::float_data (unsigned int component) const
{
  tl_assert (component < 3);
  return mp_data ? mp_data->float_data (component) : 0;
}

bool
Object::mask (size_t x, size_t y) const
{
  if (mp_data && mp_data->mask () && x < width () && y < height ()) {
    return mp_data->mask ()[x + y * width ()] != 0;
  } else {
    return true;
  }
}

void
Object::set_mask (size_t x, size_t y, bool m) 
{
  if (mp_data && x < width () && y < height ()) {
    mp_data->set_mask ()[x + y * width ()] = m;
    if (m_updates_enabled) {
      property_changed ();
    }
  }
}

double 
Object::pixel (size_t x, size_t y) const
{
  if (mp_data && x < width () && y < height () && ! is_color ()) {
    if (is_byte_data ()) {
      return mp_data->byte_data ()[x + y * width ()];
    } else {
      return mp_data->float_data ()[x + y * width ()];
    }
  } else {
    return 0.0;
  }
}

double 
Object::pixel (size_t x, size_t y, unsigned int component) const
{
  if (mp_data && x < width () && y < height ()) {
    if (! is_color ()) {
      if (is_byte_data ()) {
        return mp_data->byte_data ()[x + y * width ()];
      } else {
        return mp_data->float_data ()[x + y * width ()];
      }
    } else if (component < 3) {
      if (is_byte_data ()) {
        return mp_data->byte_data (component)[x + y * width ()];
      } else {
        return mp_data->float_data (component)[x + y * width ()];
      }
    }
  } 
  return 0.0;
}

void 
Object::set_pixel (size_t x, size_t y, double v)
{
  if (mp_data && x < width () && y < height () && ! is_color ()) {
    invalidate_pixel_data ();
    if (is_byte_data ()) {
      mp_data->byte_data ()[x + y * width ()] = (unsigned char) v;
    } else {
      mp_data->float_data ()[x + y * width ()] = v;
    }
    if (m_updates_enabled) {
      property_changed ();
    }
  }
}

void 
Object::set_pixel (size_t x, size_t y, double red, double green, double blue)
{
  if (mp_data && x < width () && y < height () && is_color ()) {
    invalidate_pixel_data ();
    size_t i = x + y * width ();
    if (is_byte_data ()) {
      mp_data->byte_data (0)[i] = (unsigned char) red;
      mp_data->byte_data (1)[i] = (unsigned char) green;
      mp_data->byte_data (2)[i] = (unsigned char) blue;
    } else {
      mp_data->float_data (0)[i] = red;
      mp_data->float_data (1)[i] = green;
      mp_data->float_data (2)[i] = blue;
    }
    if (m_updates_enabled) {
      property_changed ();
    }
  }
}

void 
Object::set_data (size_t w, size_t h, unsigned char *d)
{
  release ();

  mp_data = new DataHeader (w, h, d);
  mp_data->add_ref ();

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::set_data (size_t w, size_t h, float *d)
{
  release ();

  mp_data = new DataHeader (w, h, d);
  mp_data->add_ref ();

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::set_data (size_t w, size_t h, const std::vector<double> &d)
{
  release ();

  mp_data = new DataHeader (w, h, false /*not color*/, false /*float data*/);
  mp_data->add_ref ();

  float *t = mp_data->float_data ();
  std::vector<double>::const_iterator s = d.begin ();

  for (size_t i = std::min (d.size (), data_length ()); i > 0; --i) {
    *t++ = *s++;
  }

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::set_data (size_t w, size_t h, unsigned char *red, unsigned char *green, unsigned char *blue)
{
  release ();

  mp_data = new DataHeader (w, h, red, green, blue);
  mp_data->add_ref ();

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::set_data (size_t w, size_t h, float *red, float *green, float *blue)
{
  release ();

  mp_data = new DataHeader (w, h, red, green, blue);
  mp_data->add_ref ();

  if (m_updates_enabled) {
    property_changed ();
  }
}

void 
Object::set_data (size_t w, size_t h, const std::vector<double> &red, const std::vector<double> &green, const std::vector<double> &blue)
{
  release ();

  mp_data = new DataHeader (w, h, true /*color*/, false /*float data*/);
  mp_data->add_ref ();

  float *t;
  std::vector<double>::const_iterator s;

  t = mp_data->float_data (0);
  s = red.begin ();
  for (size_t i = std::min (red.size (), data_length ()); i > 0; --i) {
    *t++ = *s++;
  }

  t = mp_data->float_data (1);
  s = green.begin ();
  for (size_t i = std::min (green.size (), data_length ()); i > 0; --i) {
    *t++ = *s++;
  }

  t = mp_data->float_data (2);
  s = blue.begin ();
  for (size_t i = std::min (blue.size (), data_length ()); i > 0; --i) {
    *t++ = *s++;
  }

  if (m_updates_enabled) {
    property_changed ();
  }
}

void
img::Object::set_data_mapping (const DataMapping &dm)
{
  invalidate_pixel_data ();
  m_data_mapping = dm;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void
img::Object::set_matrix (const db::Matrix3d &trans)
{
  m_trans = db::Matrix3d (trans);
  if (m_updates_enabled) {
    property_changed ();
  }
}

void
img::Object::set_min_value (double h)
{
  invalidate_pixel_data ();
  m_min_value = h;
  if (m_updates_enabled) {
    property_changed ();
  }
}

void
img::Object::set_max_value (double h)
{
  invalidate_pixel_data ();
  m_max_value = h;
  if (m_updates_enabled) {
    property_changed ();
  }
}

static
void get_min_max (const float *data, size_t n, double &min, double &max)
{
  bool first = true;
  min = max = 0.0;

  for (size_t i = 0; i < n; ++i) {
    if (first || data [i] < min) {
      min = data [i];
    }
    if (first || data [i] > max) {
      max = data [i];
    }
    first = false;
  }
}

void 
Object::validate_pixel_data () const
{
  if (mp_data != 0 && mp_pixel_data == 0 && ! is_empty ()) {

    size_t n = data_length ();

    tl::color_t *nc_pixel_data = new tl::color_t [n];
    mp_pixel_data = nc_pixel_data;

    double min = 0.0, max = 255.0;
    if (! mp_data->is_byte_data () && ! mp_data->is_color ()) {
      get_min_max (mp_data->float_data (), n, min, max);
    }

    tl::DataMappingLookupTable lut[3];

    for (unsigned int i = 0; i < 3; ++i) {

      lut[i].set_data_mapping (m_data_mapping.create_data_mapping (! mp_data->is_color (), m_min_value, m_max_value, i));

      if (! mp_data->is_byte_data () && mp_data->is_color ()) {
        get_min_max (mp_data->float_data (i), n, min, max);
      }
      lut[i].update_table (min, max, 1.0, 1 << ((2 - i) * 8));

    }

    if (mp_data->is_byte_data ()) {

      if (mp_data->is_color ()) {

        tl::color_t *pixel_data = nc_pixel_data;
        const unsigned char *f = mp_data->byte_data (0);
        const tl::DataMappingLookupTable *l = &lut[0];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ = (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->byte_data (1);
        l = &lut[1];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->byte_data (2);
        l = &lut[2];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++); 
        }

      } else {

        tl::color_t *pixel_data = nc_pixel_data;
        const unsigned char *f = mp_data->byte_data ();
        const tl::DataMappingLookupTable *l = &lut[0];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ = (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->byte_data ();
        l = &lut[1];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->byte_data ();
        l = &lut[2];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++); 
        }

      }

    } else {

      if (mp_data->is_color ()) {

        tl::color_t *pixel_data = nc_pixel_data;
        const float *f = mp_data->float_data (0);
        const tl::DataMappingLookupTable *l = &lut[0];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ = (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->float_data (1);
        l = &lut[1];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->float_data (2);
        l = &lut[2];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++); 
        }

      } else {

        tl::color_t *pixel_data = nc_pixel_data;
        const float *f = mp_data->float_data ();
        const tl::DataMappingLookupTable *l = &lut[0];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ = (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->float_data ();
        l = &lut[1];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++);
        }

        pixel_data = nc_pixel_data;
        f = mp_data->float_data ();
        l = &lut[2];
        for (size_t j = 0; j < n; ++j) {
          *pixel_data++ |= (*l) (*f++); 
        }

      }

    }

  }
}

void
Object::invalidate_pixel_data ()
{
  if (mp_pixel_data != 0) {
    delete [] mp_pixel_data;
    mp_pixel_data = 0;
  }
}

void
Object::property_changed ()
{
  //  .. nothing yet ..
}

const std::vector <db::DPoint> &
Object::landmarks () const
{
  return m_landmarks;
}

void
Object::set_landmarks (const std::vector <db::DPoint> &lm)
{
  if (m_landmarks != lm) {
    m_landmarks = lm;
    if (m_updates_enabled) {
      property_changed ();
    }
  }
}

bool 
Object::is_valid_matrix (const db::Matrix3d &matrix)
{
  db::DPoint p[] = {
    db::DPoint (-0.5 * width (), -0.5 * height ()),
    db::DPoint (-0.5 * width (), 0.5 * height ()),
    db::DPoint (0.5 * width (), -0.5 * height ()),
    db::DPoint (0.5 * width (), 0.5 * height ())
  };

  for (unsigned int i = 0; i < sizeof (p) / sizeof (p[0]); ++i) {
    double z = matrix.m ()[2][0] * p[i].x () + matrix.m ()[2][1] * p[i].y () + matrix.m ()[2][2];
    if (z < 1e-10) {
      return false;
    }
  }

  return true;
}

void
Object::mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }
  if (mp_data) {
    mp_data->mem_stat (stat, purpose, cat, false, (void *) this);
  }
}

const char *
Object::class_name () const
{
  return "img::Object";
}

/**
 *  @brief Registration of the img::Object class in the DUserObject space
 */
static db::DUserObjectDeclaration class_registrar (new db::user_object_factory_impl<img::Object, db::DCoord> ("img::Object"));

} // namespace img

