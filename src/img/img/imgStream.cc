
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

#include "imgStream.h"
#include "tlXMLParser.h"
#include "tlXMLWriter.h"
#include "tlTimer.h"
#include "layConverters.h"

#include <memory>

namespace img
{

class ImageProxy
{
public:
  ImageProxy (const img::Object *img = 0)
    : mp_img (img),
      m_width (1), m_height (1),
      m_min_value (0.0), m_max_value (1.0),
      m_color (false)
  {
    init ();
  }

  bool is_color () const
  {
    return mp_img->is_color ();
  }

  void set_color (bool f)
  {
    m_color = f;
  }

  size_t width () const
  {
    return mp_img->width ();
  }

  void set_width (size_t w)
  {
    m_width = w;
  }

  size_t height () const
  {
    return mp_img->height ();
  }

  void set_height (size_t h)
  {
    m_height = h;
  }

  std::list<std::string>::const_iterator begin_byte_data () const
  {
    return m_byte_data.begin ();
  }

  std::list<std::string>::const_iterator end_byte_data () const
  {
    return m_byte_data.end ();
  }

  void push_byte_data (const std::string &s)
  {
    m_byte_data.push_back (s);
  }

  std::list<std::string>::const_iterator begin_data () const
  {
    return m_data.begin ();
  }

  std::list<std::string>::const_iterator end_data () const
  {
    return m_data.end ();
  }

  void push_data (const std::string &s)
  {
    m_data.push_back (s);
  }

  const db::Matrix3d &matrix () const
  {
    return mp_img->matrix ();
  }

  void set_matrix (const db::Matrix3d &m)
  {
    m_matrix = m;
  }

  double min_value () const
  {
    return mp_img->min_value ();
  }

  void set_min_value (double h)
  {
    m_min_value = h;
  }

  double max_value () const
  {
    return mp_img->max_value ();
  }

  void set_max_value (double h)
  {
    m_max_value = h;
  }

  const img::DataMapping &data_mapping () const
  {
    return mp_img->data_mapping ();
  }

  void set_data_mapping (const img::DataMapping &dm)
  {
    m_data_mapping = dm;
  }

  const img::Object::landmarks_type &landmarks () const
  {
    return mp_img->landmarks ();
  }

  void set_landmarks (const img::Object::landmarks_type &lm)
  {
    m_landmarks = lm;
  }

  img::Object *get_image () const;

private:
  const img::Object *mp_img;

  //  reader mode
  size_t m_width, m_height;
  img::Object::landmarks_type m_landmarks;
  img::DataMapping m_data_mapping;
  double m_min_value, m_max_value;
  db::Matrix3d m_matrix;
  std::list<std::string> m_byte_data, m_data;
  bool m_color;

  void init ();
};

template <class T1, class T2>
static void
string_to_pixels (img::Object *img, const std::string &s, size_t row, size_t w, bool color)
{
  tl::Extractor ex (s.c_str ());

  size_t column = 0;
  while (! ex.at_end () && column < w) {

    T1 r = 0;
    T1 g = 0;
    T1 b = 0;
    T2 m = 0;

    unsigned int i = 0;
    bool has_mask = false;

    while (! ex.at_end () && ! ex.test (";")) {

      if (i == 0) {
        ex.read (r);
      } else if (color && i == 1) {
        ex.read (g);
      } else if (color && i == 2) {
        ex.read (b);
      } else {
        ex.read (m);
        has_mask = true;
      }
      ++i;

      ex.test (",");

    }

    if (color) {
      img->set_pixel (column, row, double (r), double (g), double (b));
    } else {
      img->set_pixel (column, row, double (r));
    }

    if (has_mask) {
      img->set_mask (column, row, m);
    }

    ++column;

  }

}

img::Object *
ImageProxy::get_image () const
{
  std::unique_ptr<img::Object> img (new Object (std::max (size_t (1), m_width), std::max (size_t (1), m_height), m_matrix, m_color, ! m_byte_data.empty ()));
  img->set_min_value (m_min_value);
  img->set_max_value (m_max_value);
  img->set_data_mapping (m_data_mapping);
  img->set_landmarks (m_landmarks);

  if (! m_byte_data.empty ()) {

    std::list<std::string>::const_iterator s = m_byte_data.begin ();
    for (size_t i = 0; i < m_height; ++i) {
      string_to_pixels<unsigned char, unsigned char> (img.get (), *s++, i, m_width, m_color);
    }

  } else {

    std::list<std::string>::const_iterator s = m_data.begin ();
    for (size_t i = 0; i < m_height; ++i) {
      string_to_pixels<float, unsigned char> (img.get (), *s++, i, m_width, m_color);
    }

  }

  return img.release ();
}

static void add_entry (std::string &heap, const float *&b, bool &first)
{
  if (b) {
    if (! first) {
      heap += ",";
    }
    heap += tl::to_string (*b++);
    first = false;
  }
}

static void add_entry (std::string &heap, const unsigned char *&b, bool &first)
{
  if (b) {
    if (! first) {
      heap += ",";
    }
    heap += tl::to_string ((unsigned int) *b++);
    first = false;
  }
}

template <class T1, class T2, class T3, class T4>
static const std::string &data_to_string (std::string &heap, size_t l, const T1 *r, const T2 *g, const T3 *b, const T4 *m)
{
  heap.clear ();

  while (l-- > 0) {
    bool first = true;
    add_entry (heap, r, first);
    add_entry (heap, g, first);
    add_entry (heap, b, first);
    add_entry (heap, m, first);
    if (l > 0) {
      heap += ";";
    }
  }

  return heap;
}

void
ImageProxy::init ()
{
  if (!mp_img) {
    return;
  }

  size_t w = mp_img->width ();
  size_t h = mp_img->height ();

  static std::string s;

  if (mp_img->is_color ()) {

    if (mp_img->is_byte_data ()) {

      const unsigned char *r = mp_img->byte_data (0);
      const unsigned char *g = mp_img->byte_data (1);
      const unsigned char *b = mp_img->byte_data (2);
      const unsigned char *m = mp_img->mask ();

      for (size_t i = 0; i < h; ++i) {
        m_byte_data.push_back (data_to_string (s, w, r + i * w, g + i * w, b + i * w, m ? (m + i * w) : 0));
      }

    } else {

      const float *r = mp_img->float_data (0);
      const float *g = mp_img->float_data (1);
      const float *b = mp_img->float_data (2);
      const unsigned char *m = mp_img->mask ();

      for (size_t i = 0; i < h; ++i) {
        m_data.push_back (data_to_string (s, w, r + i * w, g + i * w, b + i * w, m ? (m + i * w) : 0));
      }

    }

  } else {

    if (mp_img->is_byte_data ()) {

      const unsigned char *g = mp_img->byte_data ();
      const unsigned char *m = mp_img->mask ();

      for (size_t i = 0; i < h; ++i) {
        m_byte_data.push_back (data_to_string (s, w, g + i * w, (const unsigned char *) 0, (const unsigned char *) 0, m ? (m + i * w) : 0));
      }

    } else {

      const float *g = mp_img->float_data ();
      const unsigned char *m = mp_img->mask ();

      for (size_t i = 0; i < h; ++i) {
        m_data.push_back (data_to_string (s, w, g + i * w, (const float *) 0, (const float *) 0, m ? (m + i * w) : 0));
      }

    }

  }
}

// --------------------------------------------------------------------------------------------------------------------------

namespace {

  struct PointConverter
  {
    std::string to_string (const db::DPoint &p) const
    {
      return p.to_string ();
    }

    void from_string (const std::string &s, db::DPoint &p) const
    {
      tl::Extractor ex (s.c_str ());
      ex.read (p);
    }
  };

  struct ColorMapConverter
  {
    std::string to_string (const std::pair<double, std::pair<tl::Color, tl::Color> > &cm) const
    {
      std::string s;
      s = tl::to_string (cm.first);
      s += ":";

      lay::ColorConverter cc;
      s += tl::to_word_or_quoted_string (cc.to_string (cm.second.first));
      if (cm.second.first != cm.second.second) {
        s += ",";
        s += tl::to_word_or_quoted_string (cc.to_string (cm.second.second));
      }

      return s;
    }

    void from_string (const std::string &s, std::pair<double, std::pair<tl::Color, tl::Color> > &cm) const
    {
      tl::Extractor ex (s.c_str ());

      ex.read (cm.first);
      ex.test (":");

      lay::ColorConverter cc;

      std::string w;
      ex.read_word_or_quoted (w);

      cc.from_string (w, cm.second.first);

      if (ex.test (",")) {

        w.clear ();
        ex.read_word_or_quoted (w);

        cc.from_string (w, cm.second.second);

      } else {
        cm.second.second = cm.second.first;
      }
    }
  };

}

tl::XMLStruct<ImageProxy> s_img_structure ("image-data",
  tl::make_member (&ImageProxy::is_color, &ImageProxy::set_color, "color") +
  tl::make_member (&ImageProxy::width, &ImageProxy::set_width, "width") +
  tl::make_member (&ImageProxy::height, &ImageProxy::set_height, "height") +
  tl::make_member (&ImageProxy::matrix, &ImageProxy::set_matrix, "matrix") +
  tl::make_member (&ImageProxy::min_value, &ImageProxy::set_min_value, "min-value") +
  tl::make_member (&ImageProxy::max_value, &ImageProxy::set_max_value, "max-value") +
  tl::make_element (&ImageProxy::data_mapping, &ImageProxy::set_data_mapping, "data-mapping",
    tl::make_element (&img::DataMapping::false_color_nodes, "color-map",
      tl::make_member<std::pair<double, std::pair<tl::Color, tl::Color> >, img::DataMapping::false_color_nodes_type::const_iterator, img::DataMapping::false_color_nodes_type, ColorMapConverter> (&img::DataMapping::false_color_nodes_type::begin, &img::DataMapping::false_color_nodes_type::end, &img::DataMapping::false_color_nodes_type::push_back, "color-map-entry", ColorMapConverter ())
    ) +
    tl::make_member (&img::DataMapping::brightness, "brightness") +
    tl::make_member (&img::DataMapping::contrast, "contrast") +
    tl::make_member (&img::DataMapping::gamma, "gamma") +
    tl::make_member (&img::DataMapping::red_gain, "red-gain") +
    tl::make_member (&img::DataMapping::green_gain, "green-gain") +
    tl::make_member (&img::DataMapping::blue_gain, "blue-gain")
  ) +
  tl::make_element (&ImageProxy::landmarks, &ImageProxy::set_landmarks, "landmarks",
    tl::make_member<db::DPoint, img::Object::landmarks_type::const_iterator, img::Object::landmarks_type, PointConverter> (&img::Object::landmarks_type::begin, &img::Object::landmarks_type::end, &img::Object::landmarks_type::push_back, "landmark", PointConverter ())
  ) +
  tl::make_member (&ImageProxy::begin_byte_data, &ImageProxy::end_byte_data, &ImageProxy::push_byte_data, "byte-data") +
  tl::make_member (&ImageProxy::begin_data, &ImageProxy::end_data, &ImageProxy::push_data, "data")
);

// --------------------------------------------------------------------------------------------------------------------------

img::Object *
ImageStreamer::read (tl::InputStream &stream)
{
  ImageProxy proxy;

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading image file: ")) + stream.source ());
  tl::XMLStreamSource in (stream, tl::to_string (tr ("Image file")));
  s_img_structure.parse (in, proxy);

  return proxy.get_image ();
}

void
ImageStreamer::write (tl::OutputStream &stream, const img::Object &img)
{
  ImageProxy proxy (&img);

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Writing image file: ")) + stream.path ());
  s_img_structure.write (stream, proxy);
}

} // namespace img

