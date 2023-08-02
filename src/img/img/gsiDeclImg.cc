
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



#include "gsiDecl.h"
#include "gsiSignals.h"
#include "imgObject.h"
#include "imgService.h"
#include "imgStream.h"
#include "dbTilingProcessor.h"
#include "layLayoutViewBase.h"

#if defined(HAVE_QT)
#  include <QImage>
#endif

namespace gsi
{

static img::DataMapping *new_data_mapping ()
{
  return new img::DataMapping ();
}

static void clear_colormap (img::DataMapping *dm)
{
  dm->false_color_nodes.clear ();
}

static void add_colormap (img::DataMapping *dm, double value, tl::color_t color)
{
  dm->false_color_nodes.push_back (std::make_pair (value, std::make_pair (tl::Color (color), tl::Color (color))));
}

static void add_colormap2 (img::DataMapping *dm, double value, tl::color_t lcolor, tl::color_t rcolor)
{
  dm->false_color_nodes.push_back (std::make_pair (value, std::make_pair (tl::Color (lcolor), tl::Color (rcolor))));
}

static size_t num_colormap_entries (const img::DataMapping *dm)
{
  return dm->false_color_nodes.size ();
}

static tl::color_t colormap_color (const img::DataMapping *dm, size_t i)
{
  if (i < dm->false_color_nodes.size ()) {
    return dm->false_color_nodes [i].second.first.rgb ();
  } else {
    return 0;
  }
}

static tl::color_t colormap_lcolor (const img::DataMapping *dm, size_t i)
{
  if (i < dm->false_color_nodes.size ()) {
    return dm->false_color_nodes [i].second.first.rgb ();
  } else {
    return 0;
  }
}

static tl::color_t colormap_rcolor (const img::DataMapping *dm, size_t i)
{
  if (i < dm->false_color_nodes.size ()) {
    return dm->false_color_nodes [i].second.second.rgb ();
  } else {
    return 0;
  }
}

static double colormap_value (const img::DataMapping *dm, size_t i)
{
  if (i < dm->false_color_nodes.size ()) {
    return dm->false_color_nodes [i].first;
  } else {
    return 0.0;
  }
}

static void set_brightness (img::DataMapping *dm, double b)
{
  dm->brightness = b;
}

static double brightness (const img::DataMapping *dm)
{
  return dm->brightness;
}

static void set_contrast (img::DataMapping *dm, double c)
{
  dm->contrast = c;
}

static double contrast (const img::DataMapping *dm)
{
  return dm->contrast;
}

static void set_gamma (img::DataMapping *dm, double g)
{
  dm->gamma = g;
}

static double gamma (const img::DataMapping *dm)
{
  return dm->gamma;
}

static void set_red_gain (img::DataMapping *dm, double g)
{
  dm->red_gain = g;
}

static double red_gain (const img::DataMapping *dm)
{
  return dm->red_gain;
}

static void set_green_gain (img::DataMapping *dm, double g)
{
  dm->green_gain = g;
}

static double green_gain (const img::DataMapping *dm)
{
  return dm->green_gain;
}

static void set_blue_gain (img::DataMapping *dm, double g)
{
  dm->blue_gain = g;
}

static double blue_gain (const img::DataMapping *dm)
{
  return dm->blue_gain;
}

gsi::Class<img::DataMapping> decl_ImageDataMapping ("lay", "ImageDataMapping",
  gsi::constructor ("new", &gsi::new_data_mapping,
    "@brief Create a new data mapping object with default settings"
  ) +
  gsi::method_ext ("clear_colormap", &gsi::clear_colormap, 
    "@brief The the color map of this data mapping object."
  ) +
  gsi::method_ext ("add_colormap_entry", &gsi::add_colormap, gsi::arg ("value"), gsi::arg ("color"),
    "@brief Add a colormap entry for this data mapping object.\n"
    "@param value The value at which the given color should be applied.\n"
    "@param color The color to apply (a 32 bit RGB value).\n"
    "\n"
    "This settings establishes a color mapping for a given value in the monochrome channel. "
    "The color must be given as a 32 bit integer, where the lowest order byte describes the "
    "blue component (0 to 255), the second byte the green component and the third byte the "
    "red component, i.e. 0xff0000 is red and 0x0000ff is blue. "
  ) +
  gsi::method_ext ("add_colormap_entry", &gsi::add_colormap2, gsi::arg ("value"), gsi::arg ("lcolor"), gsi::arg ("rcolor"),
    "@brief Add a colormap entry for this data mapping object.\n"
    "@param value The value at which the given color should be applied.\n"
    "@param lcolor The color to apply left of the value (a 32 bit RGB value).\n"
    "@param rcolor The color to apply right of the value (a 32 bit RGB value).\n"
    "\n"
    "This settings establishes a color mapping for a given value in the monochrome channel. "
    "The colors must be given as a 32 bit integer, where the lowest order byte describes the "
    "blue component (0 to 255), the second byte the green component and the third byte the "
    "red component, i.e. 0xff0000 is red and 0x0000ff is blue.\n"
    "\n"
    "In contrast to the version with one color, this version allows specifying a color left and right "
    "of the value - i.e. a discontinuous step.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("num_colormap_entries", &gsi::num_colormap_entries,
    "@brief Returns the current number of color map entries.\n"
    "@return The number of entries.\n"
  ) +
  gsi::method_ext ("colormap_value", &gsi::colormap_value, gsi::arg ("n"),
    "@brief Returns the value for a given color map entry.\n"
    "@param n The index of the entry (0..\\num_colormap_entries-1)\n"
    "@return The value (see \\add_colormap_entry for a description).\n"
  ) +
  gsi::method_ext ("colormap_color", &gsi::colormap_color, gsi::arg ("n"),
    "@brief Returns the color for a given color map entry.\n"
    "@param n The index of the entry (0..\\num_colormap_entries-1)\n"
    "@return The color (see \\add_colormap_entry for a description).\n"
    "\n"
    "NOTE: this version is deprecated and provided for backward compatibility. For discontinuous nodes "
    "this method delivers the left-sided color."
  ) +
  gsi::method_ext ("colormap_lcolor", &gsi::colormap_lcolor, gsi::arg ("n"),
    "@brief Returns the left-side color for a given color map entry.\n"
    "@param n The index of the entry (0..\\num_colormap_entries-1)\n"
    "@return The color (see \\add_colormap_entry for a description).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method_ext ("colormap_rcolor", &gsi::colormap_rcolor, gsi::arg ("n"),
    "@brief Returns the right-side color for a given color map entry.\n"
    "@param n The index of the entry (0..\\num_colormap_entries-1)\n"
    "@return The color (see \\add_colormap_entry for a description).\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method_ext ("brightness=", &gsi::set_brightness, gsi::arg ("brightness"),
    "@brief Set the brightness\n"
    "See \\brightness for a description of this property.\n"
  ) +
  gsi::method_ext ("brightness", &gsi::brightness, 
    "@brief The brightness value\n"
    "\n"
    "The brightness is a double value between roughly -1.0 and 1.0. \n"
    "Neutral (original) brightness is 0.0.\n"
  ) +
  gsi::method_ext ("contrast=", &gsi::set_contrast, gsi::arg ("contrast"),
    "@brief Set the contrast\n"
    "See \\contrast for a description of this property.\n"
  ) +
  gsi::method_ext ("contrast", &gsi::contrast, 
    "@brief The contrast value\n"
    "\n"
    "The contrast is a double value between roughly -1.0 and 1.0. \n"
    "Neutral (original) contrast is 0.0.\n"
  ) +
  gsi::method_ext ("gamma=", &gsi::set_gamma, gsi::arg ("gamma"),
    "@brief Set the gamma\n"
    "See \\gamma for a description of this property.\n"
  ) +
  gsi::method_ext ("gamma", &gsi::gamma, 
    "@brief The gamma value\n"
    "\n"
    "The gamma value allows one to adjust for non-linearities in the display chain and to enhance contrast.\n"
    "A value for linear intensity reproduction on the screen is roughly 0.5. The exact value depends on the \n"
    "monitor calibration. Values below 1.0 give a \"softer\" appearance while values above 1.0 give a \"harder\" appearance.\n"
  ) +
  gsi::method_ext ("red_gain=", &gsi::set_red_gain, gsi::arg ("red_gain"),
    "@brief Set the red_gain\n"
    "See \\red_gain for a description of this property.\n"
  ) +
  gsi::method_ext ("red_gain", &gsi::red_gain, 
    "@brief The red channel gain\n"
    "\n"
    "This value is the multiplier by which the red channel is scaled after applying \n"
    "false color transformation and contrast/brightness/gamma.\n"
    "\n"
    "1.0 is a neutral value. The gain should be >=0.0.\n"
  ) +
  gsi::method_ext ("green_gain=", &gsi::set_green_gain, gsi::arg ("green_gain"),
    "@brief Set the green_gain\n"
    "See \\green_gain for a description of this property.\n"
  ) +
  gsi::method_ext ("green_gain", &gsi::green_gain, 
    "@brief The green channel gain\n"
    "\n"
    "This value is the multiplier by which the green channel is scaled after applying \n"
    "false color transformation and contrast/brightness/gamma.\n"
    "\n"
    "1.0 is a neutral value. The gain should be >=0.0.\n"
  ) +
  gsi::method_ext ("blue_gain=", &gsi::set_blue_gain, gsi::arg ("blue_gain"),
    "@brief Set the blue_gain\n"
    "See \\blue_gain for a description of this property.\n"
  ) +
  gsi::method_ext ("blue_gain", &gsi::blue_gain, 
    "@brief The blue channel gain\n"
    "\n"
    "This value is the multiplier by which the blue channel is scaled after applying \n"
    "false color transformation and contrast/brightness/gamma.\n"
    "\n"
    "1.0 is a neutral value. The gain should be >=0.0.\n"
  ),
  "@brief A structure describing the data mapping of an image object\n"
  "\n"
  "Data mapping is the process of transforming the data into RGB pixel values.\n"
  "This implementation provides four adjustment steps: first, in the case of monochrome\n"
  "data, the data is converted to a RGB triplet using the color map. The default color map\n"
  "will copy the value to all channels rendering a gray scale. After having normalized the data \n"
  "to 0..1 cooresponding to the min_value and max_value settings of the image, a color channel-independent\n"
  "brightness and contrast adjustment is applied. Then, a per-channel multiplier (red_gain, green_gain,\n"
  "blue_gain) is applied. Finally, the gamma function is applied and the result converted into a 0..255 \n"
  "pixel value range and clipped.\n"
);

class ImageRef;

static void replace_image_base (lay::LayoutViewBase *view, size_t id, ImageRef &new_obj);
static void erase_image_base (lay::LayoutViewBase *view, size_t id);

/**
 *  @brief An extension of the img::Object that provides "live" updates of the view
 */
class ImageRef
  : public img::Object
{
public:
  ImageRef ()
    : img::Object (), dm_update_view (this, &ImageRef::do_update_view)
  {
    //  .. nothing yet ..
  }

  ImageRef (const img::Object &img)
    : img::Object (img), dm_update_view (this, &ImageRef::do_update_view)
  {
    //  .. nothing yet ..
  }

  ImageRef (const img::Object &other, lay::LayoutViewBase *view)
    : img::Object (other), mp_view (view), dm_update_view (this, &ImageRef::do_update_view)
  {
    //  .. nothing yet ..
  }

  ImageRef (const ImageRef &other)
    : img::Object (other), mp_view (other.mp_view), dm_update_view (this, &ImageRef::do_update_view)
  {
    //  .. nothing yet ..
  }

  ImageRef &operator= (const ImageRef &other)
  {
    //  NOTE: assignment changes the properties, not the reference
    if (this != &other) {
      img::Object::operator= (other);
    }
    return *this;
  }

  bool operator== (const ImageRef &other) const
  {
    return img::Object::operator== (other);
  }

  bool operator!= (const ImageRef &other) const
  {
    return img::Object::operator!= (other);
  }

  void detach ()
  {
    mp_view.reset (0);
  }

  bool is_valid () const
  {
    return mp_view;
  }

  void erase ()
  {
    if (mp_view) {
      erase_image_base (mp_view.get (), id ());
      detach ();
    }
  }

  template <class T>
  ImageRef transformed (const T &t) const
  {
    return ImageRef (img::Object::transformed<T> (t), const_cast<lay::LayoutViewBase *> (mp_view.get ()));
  }

  void set_view (lay::LayoutViewBase *view)
  {
    mp_view.reset (view);
  }

  void update_view ()
  {
    dm_update_view.cancel ();
    do_update_view ();
  }

protected:
  void property_changed ()
  {
    //  NOTE: property changes are not reflected immediately since they may be
    //  inefficient. Hence we delay their execution.
    dm_update_view ();
  }

  void do_update_view ()
  {
    if (mp_view) {
      replace_image_base (mp_view.get (), id (), *this);
    }
  }

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  tl::DeferredMethod<ImageRef> dm_update_view;
};

static ImageRef *img_from_s (const std::string &s)
{
  std::unique_ptr<ImageRef> img (new ImageRef ());
  img->from_string (s.c_str ());
  return img.release ();
}

static ImageRef *load_image (const std::string &path)
{
  tl::InputFile file (path);
  tl::InputStream stream (file);

  std::unique_ptr<img::Object> read;
  read.reset (img::ImageStreamer::read (stream));
  //  need to create a copy for now ...
  return new ImageRef (*read);
}

static void save_image (const ImageRef *image, const std::string &path)
{
  tl::OutputFile file (path);
  tl::OutputStream stream (file);
  img::ImageStreamer::write (stream, *image);
}

static ImageRef *new_image ()
{
  return new ImageRef ();
}

static ImageRef *new_image_ft (const std::string &filename, const db::DCplxTrans &trans)
{
  return new ImageRef (img::Object (filename, trans));
}

static ImageRef *new_image_pbt (const tl::PixelBuffer &pixel_buffer, const db::DCplxTrans &trans)
{
  return new ImageRef (img::Object (pixel_buffer, trans));
}

#if defined(HAVE_QTBINDINGS)
static ImageRef *new_image_qit (const QImage &image, const db::DCplxTrans &trans)
{
  return new ImageRef (img::Object (image, trans));
}
#endif

static ImageRef *new_image_whd (size_t w, size_t h, const std::vector<double> &data)
{
  return new ImageRef (img::Object (w, h, db::DCplxTrans (), data));
}

static ImageRef *new_image_whtd (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector<double> &data)
{
  return new ImageRef (img::Object (w, h, trans * db::DCplxTrans (db::DVector (0.5 * w, 0.5 * h)), data));
}

static ImageRef *new_image_whrgb (size_t w, size_t h, const std::vector<double> &red, const std::vector<double> &green, const std::vector<double> &blue)
{
  return new ImageRef (img::Object (w, h, db::DCplxTrans (), red, green, blue));
}

static ImageRef *new_image_whtrgb (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector<double> &red, const std::vector<double> &green, const std::vector<double> &blue)
{
  return new ImageRef (img::Object (w, h, trans * db::DCplxTrans (db::DVector (0.5 * w, 0.5 * h)), red, green, blue));
}

static double img_get_pixel_width (const ImageRef *obj)
{
  return obj->matrix ().mag_x ();
}

static void img_set_pixel_width (ImageRef *obj, double w)
{
  db::Matrix3d m = obj->matrix ();
  db::Matrix3d n = db::Matrix3d::perspective (m.perspective_tilt_x (1.0), m.perspective_tilt_y (1.0), 1.0) * db::Matrix3d::disp (m.disp ()) * db::Matrix3d::rotation (m.angle ()) * db::Matrix3d::shear (m.shear_angle ()) * db::Matrix3d::mag (w, m.mag_y ()) * db::Matrix3d::mirror (m.is_mirror ());
  obj->set_matrix (n);
}

static double img_get_pixel_height (const ImageRef *obj)
{
  return obj->matrix ().mag_y ();
}

static void img_set_pixel_height (ImageRef*obj, double h)
{
  db::Matrix3d m = obj->matrix ();
  db::Matrix3d n = db::Matrix3d::perspective (m.perspective_tilt_x (1.0), m.perspective_tilt_y (1.0), 1.0) * db::Matrix3d::disp (m.disp ()) * db::Matrix3d::rotation (m.angle ()) * db::Matrix3d::shear (m.shear_angle ()) * db::Matrix3d::mag (m.mag_x (), h) * db::Matrix3d::mirror (m.is_mirror ());
  obj->set_matrix (n);
}

static db::DCplxTrans img_get_trans (const ImageRef *obj)
{
  const db::Matrix3d &m = obj->matrix ();
  return db::DCplxTrans (1.0, m.angle (), m.is_mirror (), m.disp ()) * db::DCplxTrans (db::DVector (obj->width () * -0.5 * m.mag_x (), obj->height () * -0.5 * m.mag_y ()));
}

static void img_set_trans (ImageRef *obj, const db::DCplxTrans &t)
{
  //  to be consistent with the definition of KLayout 0.21, we keep mag_x and mag_y as pixel dimensions
  //  and refer to the image's pixel 0,0 as the rotation center.
  db::Matrix3d m = obj->matrix ();
  db::Matrix3d n = db::Matrix3d::disp (t.disp ()) * db::Matrix3d::rotation (t.angle ()) * db::Matrix3d::mag (t.mag () * m.mag_x (), t.mag () * m.mag_y ()) * db::Matrix3d::mirror (t.is_mirror ()) * db::Matrix3d::disp (db::DVector (obj->width () * 0.5, obj->height () * 0.5));
  obj->set_matrix (n);
}

static std::vector<double> get_data (ImageRef *obj, int component)
{
  std::vector<double> data;
  data.reserve (obj->width () * obj->height ());
  for (size_t y = 0; y < obj->height (); ++y) {
    for (size_t x = 0; x < obj->width (); ++x) {
      data.push_back (obj->pixel (x, y, (unsigned int) component));
    }
  }
  return data;
}

static void set_mask_data (ImageRef *obj, const std::vector<bool> &mask)
{
  std::vector<bool>::const_iterator m = mask.begin ();
  for (size_t y = 0; y < obj->height (); ++y) {
    for (size_t x = 0; x < obj->width (); ++x) {
      obj->set_mask (x, y, m == mask.end () ? true : *m++);
    }
  }
}

static std::vector<bool> get_mask_data (ImageRef *obj)
{
  std::vector<bool> data;
  data.reserve (obj->width () * obj->height ());
  for (size_t y = 0; y < obj->height (); ++y) {
    for (size_t x = 0; x < obj->width (); ++x) {
      data.push_back (obj->mask (x, y));
    }
  }
  return data;
}

//  NOTE: img::Object is available as "BasicImage" to allow binding for other methods.
gsi::Class<img::Object> decl_BasicImage ("lay", "BasicImage", gsi::Methods (), "@hide\n@alias Image");

gsi::Class<ImageRef> decl_Image (decl_BasicImage, "lay", "Image",
  gsi::constructor ("from_s", &gsi::img_from_s, gsi::arg ("s"),
    "@brief Creates an image from the string returned by \\to_s.\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::constructor ("read", &load_image, gsi::arg ("path"),
    "@brief Loads the image from the given path.\n"
    "\n"
    "This method expects the image file as a KLayout image format file (.lyimg). "
    "This is a XML-based format containing the image data plus placement and transformation "
    "information for the image placement. In addition, image manipulation parameters for "
    "false color display and color channel enhancement are embedded.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::constructor ("new", &gsi::new_image,
    "@brief Create a new image with the default attributes"
    "\n"
    "This will create an empty image without data and no particular pixel width or related.\n"
    "Use the \\read_file or \\set_data methods to set image properties and pixel values.\n"
  ) +
  gsi::constructor ("new", &gsi::new_image_ft, gsi::arg ("filename"), gsi::arg ("trans", db::DCplxTrans (), "unity"),
    "@brief Constructor from a image file\n"
    "\n"
    "This constructor creates an image object from a file (which can have any format supported by Qt) and \n"
    "a transformation. The image will originally be put to position 0,0 (lower left corner) and each pixel\n"
    "will have a size of 1. The transformation describes how to transform this image into micron space.\n"
    "\n"
    "@param filename The path to the image file to load.\n"
    "@param trans The transformation to apply to the image when displaying it.\n"
  ) +
  gsi::constructor ("new", &gsi::new_image_pbt, gsi::arg ("pixels"), gsi::arg ("trans", db::DCplxTrans (), "unity"),
    "@brief Constructor from a image pixel buffer\n"
    "\n"
    "This constructor creates an image object from a pixel buffer object. This object holds RGB or mono image data similar to "
    "QImage, except it is available also when Qt is not available (e.g. inside the Python module).\n"
    "\n"
    "The image will originally be put to position 0,0 (lower left corner) and each pixel\n"
    "will have a size of 1. The transformation describes how to transform this image into micron space.\n"
    "\n"
    "@param filename The path to the image file to load.\n"
    "@param trans The transformation to apply to the image when displaying it.\n"
  ) +
#if defined(HAVE_QTBINDINGS)
  gsi::constructor ("new", &gsi::new_image_qit, gsi::arg ("image"), gsi::arg ("trans", db::DCplxTrans (), "unity"),
    "@brief Constructor from a image pixel buffer\n"
    "\n"
    "This constructor creates an image object from a pixel QImage object and uses RGB or mono image data to generate the image.\n"
    "\n"
    "The image will originally be put to position 0,0 (lower left corner) and each pixel\n"
    "will have a size of 1. The transformation describes how to transform this image into micron space.\n"
    "\n"
    "@param filename The path to the image file to load.\n"
    "@param trans The transformation to apply to the image when displaying it.\n"
  ) +
#endif
  gsi::constructor ("new", &gsi::new_image_whd, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("data"),
    "@brief Constructor for a monochrome image with the given pixel values\n"
    "\n"
    "This constructor creates an image from the given pixel values. The values have to be organized\n"
    "line by line. Each line must consist of \"w\" values where the first value is the leftmost pixel.\n"
    "Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to \n"
    "the common convention for image data.\n"
    "Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). \n"
    "To adjust the data range use the \\min_value and \\max_value properties.\n"
    "\n"
    "@param w The width of the image\n"
    "@param h The height of the image\n"
    "@param d The data (see method description)\n"
  ) +
  gsi::constructor ("new", &gsi::new_image_whtd, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("trans"), gsi::arg ("data"),
    "@brief Constructor for a monochrome image with the given pixel values\n"
    "\n"
    "This constructor creates an image from the given pixel values. The values have to be organized\n"
    "line by line. Each line must consist of \"w\" values where the first value is the leftmost pixel.\n"
    "Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to \n"
    "the common convention for image data.\n"
    "Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). \n"
    "To adjust the data range use the \\min_value and \\max_value properties.\n"
    "\n"
    "@param w The width of the image\n"
    "@param h The height of the image\n"
    "@param trans The transformation from pixel space to micron space\n"
    "@param d The data (see method description)\n"
  ) +
  gsi::constructor ("new", &gsi::new_image_whrgb, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("red"), gsi::arg ("green"), gsi::arg ("blue"),
    "@brief Constructor for a color image with the given pixel values\n"
    "\n"
    "This constructor creates an image from the given pixel values. The values have to be organized\n"
    "line by line and separated by color channel. Each line must consist of \"w\" values where the first value is the leftmost pixel.\n"
    "Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to \n"
    "the common convention for image data.\n"
    "Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). \n"
    "To adjust the data range use the \\min_value and \\max_value properties.\n"
    "\n"
    "@param w The width of the image\n"
    "@param h The height of the image\n"
    "@param red The red channel data set which will become owned by the image\n"
    "@param green The green channel data set which will become owned by the image\n"
    "@param blue The blue channel data set which will become owned by the image\n"
  ) +
  gsi::constructor ("new", &gsi::new_image_whtrgb, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("trans"), gsi::arg ("red"), gsi::arg ("green"), gsi::arg ("blue"),
    "@brief Constructor for a color image with the given pixel values\n"
    "\n"
    "This constructor creates an image from the given pixel values. The values have to be organized\n"
    "line by line and separated by color channel. Each line must consist of \"w\" values where the first value is the leftmost pixel.\n"
    "Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to \n"
    "the common convention for image data.\n"
    "Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). \n"
    "To adjust the data range use the \\min_value and \\max_value properties.\n"
    "\n"
    "@param w The width of the image\n"
    "@param h The height of the image\n"
    "@param trans The transformation from pixel space to micron space\n"
    "@param red The red channel data set which will become owned by the image\n"
    "@param green The green channel data set which will become owned by the image\n"
    "@param blue The blue channel data set which will become owned by the image\n"
  ) +
  gsi::method ("box", &ImageRef::box,
    "@brief Gets the bounding box of the image\n"
    "@return The bounding box\n"
  ) +
  gsi::method ("transformed", &ImageRef::transformed<db::DTrans>, gsi::arg ("t"),
    "@brief Transforms the image with the given simple transformation\n"
    "@param t The transformation to apply\n"
    "@return The transformed object\n"
  ) +
  gsi::method ("transformed|#transformed_matrix", &ImageRef::transformed<db::Matrix3d>, gsi::arg ("t"),
    "@brief Transforms the image with the given matrix transformation\n"
    "@param t The transformation to apply (a matrix)\n"
    "@return The transformed object\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method ("transformed|#transformed_cplx", &ImageRef::transformed<db::DCplxTrans>, gsi::arg ("t"),
    "@brief Transforms the image with the given complex transformation\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed object\n"
  ) +
  gsi::method ("clear", &ImageRef::clear,
    "@brief Clears the image data (sets to 0 or black).\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("width", &ImageRef::width,
    "@brief Gets the width of the image in pixels\n"
    "@return The width in pixels\n"
  ) +
  gsi::method ("height", &ImageRef::height,
    "@brief Gets the height of the image in pixels\n"
    "@return The height in pixels\n"
  ) +
  gsi::method ("filename", &ImageRef::filename,
    "@brief Gets the name of the file loaded of an empty string if not file is loaded\n"
    "@return The file name (path)\n"
  ) +
  gsi::method ("is_empty?", &ImageRef::is_empty,
    "@brief Returns true, if the image does not contain any data (i.e. is default constructed)\n"
    "@return True, if the image is empty\n"
  ) +
  gsi::method ("is_color?", &ImageRef::is_color,
    "@brief Returns true, if the image is a color image\n"
    "@return True, if the image is a color image\n"
  ) +
  gsi::method ("set_mask", &ImageRef::set_mask, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("m"),
    "@brief Sets the mask for a pixel\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "@param m The mask\n"
    "\n"
    "If the mask of a pixel is set to false, the pixel is not drawn. The default is true for all pixels.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("mask", (bool (ImageRef::*) (size_t x, size_t y) const) &ImageRef::mask, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets the mask for one pixel\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "@return false if the pixel is not drawn.\n"
    "\n"
    "See \\set_mask for details about the mask.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("set_pixel", (void (ImageRef::*)(size_t x, size_t y, double v)) &ImageRef::set_pixel, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("v"),
    "@brief Sets one pixel (monochrome)\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "@param v The value\n"
    "\n"
    "If the component index, x or y value exceeds the image bounds of the image is a color image,\n"
    "this method does nothing.\n"
  ) +
  gsi::method ("set_pixel", (void (ImageRef::*)(size_t x, size_t y, double r, double g, double b)) &ImageRef::set_pixel, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("r"), gsi::arg ("g"), gsi::arg ("b"),
    "@brief Sets one pixel (color)\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "@param red The red component\n"
    "@param green The green component\n"
    "@param blue The blue component\n"
    "\n"
    "If the component index, x or y value exceeds the image bounds of the image is not a color image,\n"
    "this method does nothing.\n"
  ) +
  gsi::method ("get_pixel", (double (ImageRef::*)(size_t x, size_t y) const) &ImageRef::pixel, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets one pixel (monochrome only)\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "\n"
    "If x or y value exceeds the image bounds, this method \n"
    "returns 0.0. This method is valid for monochrome images only. For color images it will return 0.0 always.\n"
    "Use \\is_color? to decide whether the image is a color image or monochrome one.\n"
  ) +
  gsi::method ("get_pixel", (double (ImageRef::*)(size_t x, size_t y, unsigned int component) const) &ImageRef::pixel, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("component"),
    "@brief Gets one pixel (monochrome and color)\n"
    "\n"
    "@param x The x coordinate of the pixel (0..width()-1)\n"
    "@param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)\n"
    "@param component 0 for red, 1 for green, 2 for blue.\n"
    "\n"
    "If the component index, x or y value exceeds the image bounds, this method \n"
    "returns 0.0. For monochrome images, the component index is ignored.\n"
  ) +
  gsi::method ("set_data", (void (ImageRef::*)(size_t w, size_t h, const std::vector<double> &d)) &ImageRef::set_data, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("d"),
    "@brief Writes the image data field (monochrome)\n"
    "@param w The width of the new data\n"
    "@param h The height of the new data\n"
    "@param d The (monochrome) data to load into the image\n"
    "\n"
    "See the constructor description for the data organisation in that field.\n"
  ) +
  gsi::method ("set_data", (void (ImageRef::*)(size_t w, size_t h, const std::vector<double> &r, const std::vector<double> &g, const std::vector<double> &b)) &img::Object::set_data, gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("r"), gsi::arg ("g"), gsi::arg ("b"),
    "@brief Writes the image data field (color)\n"
    "@param w The width of the new data\n"
    "@param h The height of the new data\n"
    "@param r The red channel data to load into the image\n"
    "@param g The green channel data to load into the image\n"
    "@param b The blue channel data to load into the image\n"
    "\n"
    "See the constructor description for the data organisation in that field.\n"
  ) +
  gsi::method_ext ("data", &get_data, gsi::arg ("channel", 0),
    "@brief Gets the data array for a specific color channel\n"
    "Returns an array of pixel values for the given channel. For a color image, channel 0 is green, channel 1 is red and channel 2 is blue. "
    "For a monochrome image, the channel is ignored.\n"
    "\n"
    "For the format of the data see the constructor description.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("mask_data=", &set_mask_data, gsi::arg ("mask_data"),
    "@brief Sets the mask from a array of boolean values\n"
    "The order of the boolean values is line first, from bottom to top and left to right and is the same as the order in the data array.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("mask_data", &get_mask_data,
    "@brief Gets the mask from a array of boolean values\n"
    "See \\set_mask_data for a description of the data field.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("pixel_width=", &img_set_pixel_width, gsi::arg ("w"),
    "@brief Sets the pixel width\n"
    "\n"
    "The pixel width determines the width of on pixel in the original space which is transformed to\n"
    "micron space with the transformation.\n"
    "\n"
    "Starting with version 0.22, this property is incorporated into the transformation matrix.\n"
    "This property is provided for convenience only."
  ) +
  gsi::method_ext ("pixel_width", &img_get_pixel_width,
    "@brief Gets the pixel width\n"
    "\n"
    "See \\pixel_width= for a description of that property.\n"
    "\n"
    "Starting with version 0.22, this property is incorporated into the transformation matrix.\n"
    "This property is provided for convenience only."
  ) +
  gsi::method_ext ("pixel_height=", &img_set_pixel_height, gsi::arg ("h"),
    "@brief Sets the pixel height\n"
    "\n"
    "The pixel height determines the height of on pixel in the original space which is transformed to\n"
    "micron space with the transformation.\n"
    "\n"
    "Starting with version 0.22, this property is incorporated into the transformation matrix.\n"
    "This property is provided for convenience only."
  ) +
  gsi::method_ext ("pixel_height", &img_get_pixel_height,
    "@brief Gets the pixel height\n"
    "\n"
    "See \\pixel_height= for a description of that property.\n"
    "\n"
    "Starting with version 0.22, this property is incorporated into the transformation matrix.\n"
    "This property is provided for convenience only."
  ) +
  gsi::method ("z_position", &ImageRef::z_position,
    "@brief Gets the z position of the image\n"
    "Images with a higher z position are painted in front of images with lower z position.\n"
    "The z value is an integer that controls the position relative to other images.\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::method ("z_position=", &ImageRef::set_z_position, gsi::arg ("z"),
    "@brief Sets the z position of the image\n"
    "\n"
    "See \\z_position for details about the z position attribute.\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::method ("matrix=", &ImageRef::set_matrix, gsi::arg ("t"),
    "@brief Sets the transformation matrix\n"
    "\n"
    "This transformation matrix converts pixel coordinates (0,0 being the center and each pixel having the dimension of pixel_width and pixel_height)\n"
    "to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.\n"
    "\n"
    "The matrix is more general than the transformation used before and supports shear and perspective transformation. This property replaces the \\trans property which is "
    "still functional, but deprecated.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method ("matrix", &ImageRef::matrix,
    "@brief Returns the pixel-to-micron transformation matrix\n"
    "\n"
    "This transformation matrix converts pixel coordinates (0,0 being the center and each pixel having the dimension of pixel_width and pixel_height)\n"
    "to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.\n"
    "\n"
    "The matrix is more general than the transformation used before and supports shear and perspective transformation. This property replaces the \\trans property which is "
    "still functional, but deprecated.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("trans", &img_get_trans,
    "@brief Returns the pixel-to-micron transformation\n"
    "\n"
    "This transformation converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)\n"
    "to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.\n"
    "\n"
    "The general property is \\matrix which also allows perspective and shear transformation. This property will only "
    "work, if the transformation does not include perspective or shear components. Therefore this property is deprecated."
    "\n"
    "Please note that for backward compatibility, the rotation center is pixel 0,0 (lowest left one), while it "
    "is the image center for the matrix transformation."
  ) +
  gsi::method_ext ("trans=", &img_set_trans, gsi::arg ("t"),
    "@brief Sets the transformation\n"
    "\n"
    "This transformation converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)\n"
    "to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.\n"
    "\n"
    "The general property is \\matrix which also allows perspective and shear transformation."
    "\n"
    "Please note that for backward compatibility, the rotation center is pixel 0,0 (lowest left one), while it "
    "is the image center for the matrix transformation."
  ) +
  gsi::method ("min_value=", &ImageRef::set_min_value, gsi::arg ("v"),
    "@brief Sets the minimum value\n"
    "\n"
    "See \\min_value for the description of the minimum value property.\n"
  ) +
  gsi::method ("min_value", &ImageRef::min_value,
    "@brief Gets the upper limit of the values in the data set\n"
    "\n"
    "This value determines the upper end of the data mapping (i.e. white value etc.).\n"
    "It does not necessarily correspond to the minimum value of the data set but it must be\n"
    "larger than that.\n"
  ) +
  gsi::method ("max_value=", &ImageRef::set_max_value, gsi::arg ("v"),
    "@brief Gets the upper limit of the values in the data set\n"
    "\n"
    "This value determines the upper end of the data mapping (i.e. white value etc.).\n"
    "It does not necessarily correspond to the maximum value of the data set but it must be\n"
    "larger than that.\n"
  ) +
  gsi::method ("max_value", &ImageRef::max_value,
    "@brief Sets the maximum value\n"
    "\n"
    "See the \\max_value method for the description of the maximum value property.\n"
  ) +
  gsi::method ("visible=", &ImageRef::set_visible, gsi::arg ("v"),
    "@brief Sets the visibility\n"
    "\n"
    "See the \\is_visible? method for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method ("is_visible?", &ImageRef::is_visible,
    "@brief Gets a flag indicating whether the image object is visible\n"
    "\n"
    "An image object can be made invisible by setting the visible property to false.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method ("id", (size_t (ImageRef::*) () const) &ImageRef::id,
    "@brief Gets the Id\n"
    "\n"
    "The Id is an arbitrary integer that can be used to track the evolution of an\n"
    "image object. The Id is not changed when the object is edited.\n"
    "On initialization, a unique Id is given to the object. The Id cannot be changed. "
    "This behaviour has been modified in version 0.20."
  ) +
  gsi::method ("data_mapping=", &ImageRef::set_data_mapping, gsi::arg ("data_mapping"),
    "@brief Sets the data mapping object\n"
    "\n"
    "The data mapping describes the transformation of a pixel value (any double value) into pixel data "
    "which can be sent to the graphics cards for display. See \\ImageDataMapping for a more detailed description.\n"
  ) +
  gsi::method ("data_mapping", &ImageRef::data_mapping,
    "@brief Gets the data mapping\n"
    "@return The data mapping object\n"
    "\n"
    "The data mapping describes the transformation of a pixel value (any double value) into pixel data "
    "which can be sent to the graphics cards for display. See \\ImageDataMapping for a more detailed description.\n"
  ) +
  gsi::method ("detach", &ImageRef::detach,
    "@brief Detaches the image object from the view\n"
    "If the image object was inserted into the view, property changes will be "
    "reflected in the view. To disable this feature, 'detach'' can be called after which "
    "the image object becomes inactive and changes will no longer be reflected in the view.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("update", &ImageRef::update_view,
    "@brief Forces an update of the view\n"
    "Usually it is not required to call this method. The image object is automatically synchronized "
    "with the view's image objects. For performance reasons this update is delayed to collect multiple "
    "update requests. Calling 'update' will ensure immediate updates.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("delete", &ImageRef::erase,
    "@brief Deletes this image from the view\n"
    "If the image is an \"active\" one, this method will remove it from the view. "
    "This object will become detached and can still be manipulated, but without having an "
    "effect on the view."
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("is_valid?", &ImageRef::is_valid,
    "@brief Returns a value indicating whether the object is a valid reference.\n"
    "If this value is true, the object represents an image on the screen. Otherwise, the "
    "object is a 'detached' image which does not have a representation on the screen.\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::method ("to_s", &ImageRef::to_string,
    "@brief Converts the image to a string\n"
    "The string returned can be used to create an image object using \\from_s.\n"
    "@return The string\n"
  ) +
  gsi::method_ext ("write", &save_image, gsi::arg ("path"),
    "@brief Saves the image to KLayout's image format (.lyimg)\n"
    "This method has been introduced in version 0.27."
  ),
  "@brief An image to be stored as a layout annotation\n"
  "\n"
  "Images can be put onto the layout canvas as annotations, along with rulers and markers.\n"
  "Images can be monochrome (represent scalar data) as well as color (represent color images).\n"
  "The display of images can be adjusted in various ways, i.e. color mapping (translation of scalar values to\n"
  "colors), geometrical transformations (including rotation by arbitrary angles) and similar.\n"
  "Images are always based on floating point data. The actual data range is not fixed and can be adjusted to "
  "the data set (i.e. 0..255 or -1..1). This gives a great flexibility when displaying data which is the result of "
  "some measurement or calculation for example.\n"
  "The basic parameters of an image are the width and height of the data set, the width and height of one pixel, "
  "the geometrical transformation to be applied, the data range (min_value to max_value) and the data mapping which "
  "is described by an own class, \\ImageDataMapping.\n"
  "\n"
  "Starting with version 0.22, the basic transformation is a 3x3 matrix rather than the simple "
  "affine transformation. This matrix includes the pixel dimensions as well. One consequence of that is "
  "that the magnification part of the matrix and the pixel dimensions are no longer separated. "
  "That has certain consequences, i.e. setting an affine transformation with a magnification scales "
  "the pixel sizes as before but an affine transformation returned will no longer contain the pixel dimensions "
  "as magnification because it only supports isotropic scaling. For backward compatibility, the rotation "
  "center for the affine transformations while the default center and the center for matrix transformations "
  "is the image center.\n"
  "\n"
  "As with version 0.25, images become 'live' objects. Changes to image properties will be reflected in the "
  "view automatically once the image object has been inserted into a view. "
  "Note that changes are not immediately reflected in the view, but are delayed until the view is refreshed. "
  "Hence, iterating the view's images will not render the same results than the image objects attached to the view. "
  "To ensure synchronization, call \\Image#update."
);

/**
 *  @brief An alternative iterator that returns "live" ImageRef objects
 */
struct ImageRefIterator
  : public img::ImageIterator
{
public:
  typedef ImageRef reference;

  ImageRefIterator ()
    : img::ImageIterator ()
  {
    //  .. nothing yet ..
  }

  ImageRefIterator (const img::ImageIterator &iter, lay::LayoutViewBase *view)
    : img::ImageIterator (iter), mp_view (view)
  {
    //  .. nothing yet ..
  }

  reference operator* () const
  {
    return reference (img::ImageIterator::operator* (), const_cast<lay::LayoutViewBase * >(mp_view.get ()));
  }

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
};

static void clear_images (lay::LayoutViewBase *view)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {
    img_service->clear_images ();
  }
}

static void show_image (lay::LayoutViewBase *view, size_t id, bool visible)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {

    const img::Object *img = img_service->object_by_id (id);
    if (img == 0) {
      throw tl::Exception (tl::to_string (tr ("The image Id is not valid")));
    }

    img::Object new_img (*img);
    new_img.set_visible (visible);

    img_service->change_image_by_id (id, new_img);

  }
}

void replace_image_base (lay::LayoutViewBase *view, size_t id, ImageRef &new_obj)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {

    const img::Object *img = img_service->object_by_id (id);
    if (img == 0) {
      throw tl::Exception (tl::to_string (tr ("The image Id is not valid")));
    }

    img_service->change_image_by_id (id, new_obj);

  }
}

static void replace_image (lay::LayoutViewBase *view, size_t id, ImageRef &new_obj)
{
  replace_image_base (view, id, new_obj);
}

void erase_image_base (lay::LayoutViewBase *view, size_t id)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {

    const img::Object *img = img_service->object_by_id (id);
    if (img == 0) {
      throw tl::Exception (tl::to_string (tr ("The image Id is not valid")));
    }

    img_service->erase_image_by_id (id);

  }
}

static void erase_image (lay::LayoutViewBase *view, size_t id)
{
  erase_image_base (view, id);
}

static void insert_image (lay::LayoutViewBase *view, ImageRef &obj)
{
  if (obj.is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("The object is already inserted into a view - detach the object first or create a different object.")));
  }

  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {
    img::Object *inew = img_service->insert_image (obj);
    obj.id (int (inew->id ()));
    obj.set_view (view);
  }
}

static ImageRef get_image (lay::LayoutViewBase *view, size_t id)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {
    for (ImageRefIterator iter (img_service->begin_images (), view); !iter.at_end(); ++iter) {
      if ((*iter).id () == id) {
        return *iter;
      }
    }
  }
  return ImageRef ();
}

static tl::Event &get_images_changed_event (lay::LayoutViewBase *view)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  return img_service->images_changed_event;
}

static tl::Event &get_image_selection_changed_event (lay::LayoutViewBase *view)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  return img_service->image_selection_changed_event;
}

static tl::event<int> &get_image_changed_event (lay::LayoutViewBase *view)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  return img_service->image_changed_event;
}

static ImageRefIterator begin_images (lay::LayoutViewBase *view)
{
  img::Service *img_service = view->get_plugin <img::Service> ();
  if (img_service) {
    return ImageRefIterator (img_service->begin_images (), view);
  } else {
    return ImageRefIterator ();
  }
}

static
gsi::ClassExt<lay::LayoutViewBase> layout_view_decl (
  gsi::method_ext ("clear_images", &gsi::clear_images, 
    "@brief Clear all images on this view"
  ) +
  gsi::method_ext ("replace_image", &gsi::replace_image, gsi::arg("id"), gsi::arg("new_obj"),
    "@brief Replace an image object with the new image\n"
    "\n"
    "@param id The id of the object to replace\n"
    "@param new_obj The new object to replace the old one\n"
    "\n"
    "Replaces  the image with the given Id with the new object. The Id can be obtained with if \"id\" method of the image object.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method_ext ("erase_image", &gsi::erase_image, gsi::arg("id"),
    "@brief Erase the given image\n"
    "@param id The id of the object to erase\n"
    "\n"
    "Erases the image with the given Id. The Id can be obtained with if \"id\" method of the image object.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
    "\n"
    "With version 0.25, \\Image#delete can be used to achieve the same results."
  ) +
  gsi::method_ext ("show_image", &gsi::show_image, gsi::arg("id"), gsi::arg("visible"),
    "@brief Shows or hides the given image\n"
    "@param id The id of the object to show or hide\n"
    "@param visible True, if the image should be shown\n"
    "\n"
    "Sets the visibility of the image with the given Id. The Id can be obtained with if \"id\" method of the image object.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
    "\n"
    "With version 0.25, \\Image#visible= can be used to achieve the same results."
  ) +
  gsi::method_ext ("insert_image", &gsi::insert_image, gsi::arg("obj"),
    "@brief Insert an image object into the given view\n"
    "Insert the image object given by obj into the view.\n"
    "\n"
    "With version 0.25, this method will attach the image object to the view and the image object will become a 'live' "
    "object - i.e. changes to the object will change the appearance of the image on the screen.\n"
  ) +
  gsi::method_ext ("image", &gsi::get_image, gsi::arg ("id"),
    "@brief Gets the image given by an ID\n"
    "Returns a reference to the image given by the respective ID or an invalid image if the ID is not valid.\n"
    "Use \\Image#is_valid? to determine whether the returned image is valid or not.\n"
    "\n"
    "The returned image is a 'live' object and changing it will update the view.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::event_ext ("on_images_changed", &get_images_changed_event,
    "@brief A event indicating that images have been added or removed\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::event_ext ("on_image_selection_changed", &get_image_selection_changed_event,
    "@brief A event indicating that the image selection has changed\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::event_ext ("on_image_changed", &get_image_changed_event, gsi::arg ("id"),
    "@brief A event indicating that an image has been modified\n"
    "The argument of the event is the ID of the image that was changed.\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::iterator_ext ("each_image", &gsi::begin_images,
    "@brief Iterate over all images attached to this view\n"
    "\n"
    "With version 0.25, the objects returned by the iterator are references and can be manipulated to change their "
    "appearance.\n"
  ),
  ""
);

class SelectionIterator 
{
public:
  typedef ImageRef value_type;
  typedef std::map<img::Service::obj_iterator, unsigned int>::const_iterator iterator_type;
  typedef void pointer; 
  typedef value_type reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  SelectionIterator (const std::vector<img::Service *> &services) 
    : m_services (services), m_service (0)
  {
    if (! m_services.empty ()) {
      m_iter = m_services [m_service]->selection ().begin ();
      next ();
    }
  }

  bool at_end () const
  {
    return (m_service >= m_services.size ());
  }

  SelectionIterator &operator++ ()
  {
    ++m_iter;
    next ();
    return *this;
  }

  value_type operator* () const
  {
    return value_type (*(dynamic_cast<const img::Object *> (m_iter->first->ptr ())), m_services[m_service]->view ());
  }

private:
  std::vector<img::Service *> m_services;
  unsigned int m_service;
  iterator_type m_iter;

  void next ()
  {
    while (m_iter == m_services [m_service]->selection ().end ()) {
      ++m_service;
      if (m_service < m_services.size ()) {
        m_iter = m_services [m_service]->selection ().begin ();
      } else {
        break;
      }
    }
  }
};

//  extend the layout view by "edtService" specific methods 

static bool has_image_selection (const lay::LayoutViewBase *view)
{
  std::vector<img::Service *> img = view->get_plugins <img::Service> ();
  for (std::vector<img::Service *>::const_iterator s = img.begin (); s != img.end (); ++s) {
    if ((*s)->has_selection ()) {
      return true;
    }
  }
  return false;
}

static SelectionIterator begin_images_selected (const lay::LayoutViewBase *view)
{
  return SelectionIterator (view->get_plugins <img::Service> ());
}


static
gsi::ClassExt<lay::LayoutViewBase> layout_view_decl2 (
  gsi::method_ext ("has_image_selection?", &has_image_selection, 
    "@brief Returns true, if images are selected in this view"
    "\n"
    "This method was introduced in version 0.19."
  ) +
  gsi::iterator_ext ("each_image_selected", &begin_images_selected,
    "@brief Iterate over each selected image object, yielding a \\Image object for each of them"
    "\n"
    "This method was introduced in version 0.19."
  ),
  ""
);

/**
 *  @brief Extension for tiling processor
 */
class ImageCollectingTileOutputReceiver
  : public db::TileOutputReceiver
{
public:
  ImageCollectingTileOutputReceiver (img::Object *image)
    : mp_image (image)
  {
    //  .. nothing yet ..
  }

  virtual void begin (size_t nx, size_t ny, const db::DPoint &p0, double dx, double dy, const db::DBox & /*frame*/)
  {
    if (mp_image) {
      db::Matrix3d m = db::Matrix3d::disp ((p0 - db::DPoint ()) + db::DVector (nx * dx * 0.5, ny * dy * 0.5)) * db::Matrix3d::mag (dx, dy);
      *mp_image = img::Object (nx, ny, m, false, false);
    }
  }

  virtual void put (size_t ix, size_t iy, const db::Box & /*tile*/, size_t  /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool /*clip*/) 
  {
    if (mp_image) {
      mp_image->set_pixel (ix, iy, obj.to_double ());
    }
  }

private:
  img::Object *mp_image;
};

static void tp_output_image (db::TilingProcessor *proc, const std::string &name, img::Object *i)
{
  proc->output (name, 0, new ImageCollectingTileOutputReceiver (i), db::ICplxTrans ());
}

//  extend the db::TilingProcessor with the ability to feed images
static
gsi::ClassExt<db::TilingProcessor> tiling_processor_ext (
  method_ext ("output", &tp_output_image, gsi::arg ("name"), gsi::arg ("image"),
    "@brief Specifies output to an image\n"
    "This method will establish an output channel which delivers float data to image data. "
    "The image is a monochrome image where each pixel corresponds to a single tile. This "
    "method for example is useful to collect density information into an image. The "
    "image is configured such that each pixel covers one tile.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
  ),
  ""
);

}
