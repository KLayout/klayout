
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


#ifndef HDR_imgObject
#define HDR_imgObject

#include "imgCommon.h"

#include "dbUserObject.h"
#include "dbBox.h"
#include "dbTrans.h"
#include "dbMatrix.h"
#include "dbPolygon.h"
#include "tlDataMapping.h"
#include "tlColor.h"
#include "tlPixelBuffer.h"

#include <string>
#include <vector>

#if defined(HAVE_QT)
class QImage;
#endif

namespace img {
  
class DataHeader;

/**
 *  @brief A structure describing the data mapping of an image object
 *
 *  Data mapping is the process of transforming the data into RGB pixel values.
 */
struct IMG_PUBLIC DataMapping
{
public:
  typedef std::vector< std::pair<double, std::pair<tl::Color, tl::Color> > > false_color_nodes_type;

  /**
   *  @brief The constructor
   */
  DataMapping ();

  /**
   *  @brief Equality
   */
  bool operator== (const DataMapping &d) const;

  /**
   *  @brief Less operator
   */
  bool operator< (const DataMapping &d) const;

  /**
   *  @brief The false color mapping nodes
   *
   *  Each node is a pair or x-value (normalized to a range of 0..1) and a corresponding color.
   *  The list should have an element with x value of 0.0 and one with an x value of 1.0.
   */
  false_color_nodes_type false_color_nodes;

  /**
   *  @brief The brightness value
   *
   *  The brightness is a double value between -1.0 and 1.0. 
   *  Neutral brightness is 0.0.
   */
  double brightness;

  /**
   *  @brief The contrast value
   *
   *  The contrast is a double value between -1.0 and 1.0. 
   *  Neutral contrast is 0.0.
   */
  double contrast;

  /**
   *  @brief The gamma value
   */
  double gamma;

  /**
   *  @brief The red channel gain
   *
   *  This value is the multiplier by which the red channel is scaled after applying 
   *  false color transformation and contrast/brightness/gamma.
   *
   *  1.0 is a neutral value. The gain should be >=0.0.
   */
  double red_gain;

  /**
   *  @brief The green channel gain
   *
   *  This value is the multiplier by which the green channel is scaled after applying 
   *  false color transformation and contrast/brightness/gamma.
   *
   *  1.0 is a neutral value. The gain should be >=0.0.
   */
  double green_gain;

  /**
   *  @brief The blue channel gain
   *
   *  This value is the multiplier by which the blue channel is scaled after applying 
   *  false color transformation and contrast/brightness/gamma.
   *
   *  1.0 is a neutral value. The gain should be >=0.0.
   */
  double blue_gain;

  /**
   *  @brief Create a tl::DataMapping object that represents this data mapping
   *
   *  @param xmin The x value corresponding to the lower limit
   *  @param xmax The x value corresponding to the upper limit
   *  @param channel 0=red, 1=green, 2=blue
   *  @return A new'd DataMapping object
   */
  tl::DataMappingBase *create_data_mapping (bool monochrome, double xmin, double xmax, unsigned int channel) const;
};

/**
 *  @brief A helper function to interpolate a color in the color bar at a given x
 */
tl::Color interpolated_color (const DataMapping::false_color_nodes_type &nodes, double x);

/**
 *  @brief A image object
 * 
 *  This class implements the actual image.
 *  Since this class derives from db::UserObjectBase, these objects
 *  can be stored within the database.
 */  
class IMG_PUBLIC Object
  : public db::DUserObjectBase
{
public:
  typedef db::coord_traits<coord_type> coord_traits;
  typedef std::vector <db::DPoint> landmarks_type;

  /**
   *  @brief Default constructor
   *
   *  This will create an empty image with no defined bounding box and a unit transformation.
   */
  Object ();

  /**
   *  @brief Destructor
   */
  ~Object ();

  /**
   *  @brief Constructor for monochrome or color images with zero pixel values
   *
   *  This constructor creates an image object from a data set describing one monochrome channel
   *  or three color channels.
   *  Each channel consists of an array of x*y values where the first "x" values describe the first (lowest!) row
   *  and so on. Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The data fields can be accessed with the "data", "set_data", "pixel" or "set_pixel" methods.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param color True to create a color image.
   *  @param byte_data True to make the image store the data in bytes
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, bool color, bool byte_data);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The pixel values are given as unsigned char values with a data range of 0 to 255.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param d The data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, unsigned char *d);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param d The data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, float *d);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param d The data set
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector <double> &d);

  /**
   *  @brief Constructor for a color image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The pixel values are given as unsigned char values with a data range of 0 to 255.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param red The red channel data set which will become owned by the image
   *  @param green The green channel data set which will become owned by the image
   *  @param blue The blue channel data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, unsigned char *red, unsigned char *green, unsigned char *blue);

  /**
   *  @brief Constructor for a color image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param red The red channel data set which will become owned by the image
   *  @param green The green channel data set which will become owned by the image
   *  @param blue The blue channel data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, float *red, float *green, float *blue);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param trans The transformation from pixel space to micron space
   *  @param red The red channel data set 
   *  @param green The green channel data set
   *  @param blue The blue channel data set
   */
  Object (size_t w, size_t h, const db::DCplxTrans &trans, const std::vector <double> &red, const std::vector <double> &green, const std::vector <double> &blue);

  /**
   *  @brief Constructor from a image file 
   *
   *  This constructor creates an image object from a file (which can have any format supported by Qt) and 
   *  a transformation. The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const std::string &filename, const db::DCplxTrans &trans);

  /**
   *  @brief Constructor from a PixelBuffer object
   *
   *  This constructor creates an image object from a PixelBuffer object.
   *  The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const tl::PixelBuffer &pixel_buffer, const db::DCplxTrans &trans);

#if defined(HAVE_QT)
  /**
   *  @brief Constructor from a QImage object
   *
   *  This constructor creates an image object from a QImage object.
   *  The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const QImage &image, const db::DCplxTrans &trans);
#endif

  /**
   *  @brief Constructor for monochrome or color images with zero pixel values
   *
   *  This constructor creates an image object from a data set describing one monochrome channel
   *  or three color channels.
   *  Each channel consists of an array of x*y values where the first "x" values describe the first (lowest!) row
   *  and so on. Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The data fields can be accessed with the "data", "set_data", "pixel" or "set_pixel" methods.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param color True to create a color image.
   *  @param byte_data True to create n image using bytes rather than floats
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, bool color, bool byte_data);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The pixel values are given as unsigned char values with a data range of 0 to 255.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param d The data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, unsigned char *d);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param d The data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, float *d);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param d The data set
   */
  Object (size_t w, size_t h, const db::Matrix3d &trans, const std::vector <double> &d);

  /**
   *  @brief Constructor for a color image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *  The pixel values are given as unsigned char values with a data range of 0 to 255.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param red The red channel data set which will become owned by the image
   *  @param green The green channel data set which will become owned by the image
   *  @param blue The blue channel data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, unsigned char *red, unsigned char *green, unsigned char *blue);

  /**
   *  @brief Constructor for a color image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param red The red channel data set which will become owned by the image
   *  @param green The green channel data set which will become owned by the image
   *  @param blue The blue channel data set which will become owned by the image
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, float *red, float *green, float *blue);

  /**
   *  @brief Constructor for a monochrome image with the given pixel values
   *
   *  This constructor creates an image from the given pixel values. The values have to be organized
   *  line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
   *  Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
   *  the common convention for image data.
   *
   *  @param w The width of the image
   *  @param h The height of the image
   *  @param matrix The 3d transformation matrix from pixel space to micron space
   *  @param red The red channel data set 
   *  @param green The green channel data set
   *  @param blue The blue channel data set
   */
  Object (size_t w, size_t h, const db::Matrix3d &matrix, const std::vector <double> &red, const std::vector <double> &green, const std::vector <double> &blue);

  /**
   *  @brief Constructor from a image file 
   *
   *  This constructor creates an image object from a file (which can have any format supported by Qt) and 
   *  a transformation. The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const std::string &filename, const db::Matrix3d &trans);

  /**
   *  @brief Constructor from a PixelBuffer object
   *
   *  This constructor creates an image object from a PixelBuffer object.
   *  The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const tl::PixelBuffer &pixel_buffer, const db::Matrix3d &trans);

#if defined(HAVE_QT)
  /**
   *  @brief Constructor from a QImage object
   *
   *  This constructor creates an image object from a QImage object.
   *  The image will originally be put to position 0, 0 (lower left corner) and each pixel
   *  will have a size of 1. The transformation describes how to transform this image into micron space.
   */
  Object (const QImage &image, const db::Matrix3d &trans);
#endif

  /**
   *  @brief Copy constructor
   */
  Object (const img::Object &d);

  /**
   *  @brief Assignment
   */
  Object &operator= (const img::Object &d);

  /**
   *  @brief The generic "equal" operator
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual bool equals (const db::DUserObjectBase *d) const;

  /**
   *  @brief The generic "less" operator
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual bool less (const db::DUserObjectBase *d) const;

  /**
   *  @brief Get the class Id which internally distinguishes the img::Object's class from other classes
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual unsigned int class_id () const;

  /**
   *  @brief Clone the object
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual db::DUserObjectBase *clone () const;

  /**
   *  @brief Get the bounding box
   *
   *  This method reimplements the db::DUserObjectBase interface.
   *
   *  The bounding box can be a world bbox since under certain perspective transformations the 
   *  image may extend into the forbidden z space. For a more exact representation use 
   *  the image_box_polygon method which delivers the box clipped to a certain viewport.
   */
  virtual db::DBox box () const;

  /**
   *  @brief Get the transformed image box clipped by the given viewport in post-transformation space
   *
   *  Under certain perspective transformations, the image may not be representable by a finite bounding
   *  box. This happens if corner points of the image are located in the invalid z space (z <= 0). In this
   *  case this method can be used which delivers a polygon which is the transformed image box clipped
   *  by the given viewport box which and viewport transformation. More precisely, the viewport box vp
   *  is transformed with vpt.inverted() and the resulting polygon is used as the clip polygon to determine the 
   *  outer contour of the image in viewport space (micron space plus vpt). 
   *  This method may deliver an empty polygon if the image is outside the viewport area.
   */
  db::DPolygon image_box_poly (const db::DBox vp, const db::DCplxTrans &vpt) const;

  /**
   *  @brief Transform with a given matrix transformation
   */
  virtual void transform (const db::Matrix3d &t);

  /**
   *  @brief Transform with a given complex transformation
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual void transform (const db::DCplxTrans &t);

  /**
   *  @brief Transform with a given standard transformation
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual void transform (const db::DTrans &t);

  /**
   *  @brief Transform with a given fixpoint transformation
   *
   *  This method reimplements the db::DUserObjectBase interface.
   */
  virtual void transform (const db::DFTrans &t);

  /**
   *  @brief Return the transformed object
   */
  template <class Trans>
  img::Object transformed (const Trans &t) const
  {
    img::Object obj (*this);
    obj.transform (t);
    return obj;
  }

  /**
   *  @brief In-place move
   */
  Object &move (const db::DVector &p)
  {
    transform (db::DTrans (p));
    return *this;
  }

  /**
   *  @brief Return the moved object 
   */
  Object moved (const db::DVector &p) const
  {
    img::Object d (*this);
    d.move (p);
    return d;
  }

  /**
   *  @brief Accessor to the width property
   */
  size_t width () const;

  /**
   *  @brief Accessor to the height property
   */
  size_t height () const;

  /**
   *  @brief Get the number of entries in the data set
   */
  size_t data_length () const;

  /**
   *  @brief Get the name of the file currently loaded
   */
  const std::string &filename () const
  {
    return m_filename;
  }

  /**
   *  @brief Accessor to the is_empty property
   *
   *  @return True, if the image is empty
   */
  bool is_empty () const;

  /**
   *  @brief Accessor to the is_byte_data property
   *
   *  @return True, if the image stores data as byte data (value range 0..255)
   */
  bool is_byte_data () const;

  /**
   *  @brief Accessor to the is_color property
   *
   *  @return True, if the image is a color image
   */
  bool is_color () const;

  /**
   *  @brief Set the mask value
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *  @param m The mask value
   *
   *  If the mask value is false, the pixel is not drawn.
   *  By default all pixels are drawn.
   */
  void set_mask (size_t x, size_t y, bool m);

  /**
   *  @brief Gets the mask value for the given pixel
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *  @return m The mask value
   */
  bool mask (size_t x, size_t y) const;

  /**
   *  @brief Set one pixel (monochrome)
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *  @param v The value
   *
   *  If the component index, x or y value exceeds the image bounds of the image is a color image,
   *  this method does nothing.
   */
  void set_pixel (size_t x, size_t y, double v);

  /**
   *  @brief Set one pixel (color)
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *  @param red The red component
   *  @param green The green component
   *  @param blue The blue component
   *
   *  If the component index, x or y value exceeds the image bounds of the image is not a color image,
   *  this method does nothing.
   */
  void set_pixel (size_t x, size_t y, double red, double green, double blue);

  /**
   *  @brief Accessor to one pixel (monchrome)
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *
   *  If the x or y value exceeds the image bounds or the image is not a monochrome image, this method 
   *  returns 0.0.
   */
  double pixel (size_t x, size_t y) const;

  /**
   *  @brief Accessor to one pixel (monochrome and color)
   *
   *  @param x The x coordinate of the pixel (0..width()-1)
   *  @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
   *  @param component 0 for red, 1 for green, 2 for blue.
   *
   *  If the component index, x or y value exceeds the image bounds, this method 
   *  returns 0.0. For monochrome images, the component index is ignored.
   */
  double pixel (size_t x, size_t y, unsigned int component) const;

  /**
   *  @brief Accessor to the mask data field
   *
   *  @return The pointer to an unsigned char field with w*h entries or 0 if no mask is set.
   */
  const unsigned char *mask () const;

  /**
   *  @brief Accessor to the image data field
   *
   *  @return The pointer to an unsigned char field with w*h entries or 0 if the image is a color image.
   *  This method returns 0 if the data is not stored as bytes (is_byte_data() is false).
   */
  const unsigned char *byte_data () const;

  /**
   *  @brief Accessor to the image data field (color component)
   *
   *  @return The pointer to an unsigned char field with w*h entries or 0 if the image is not a color image.
   *  This method returns 0 if the data is not stored as bytes (is_byte_data() is false).
   *
   *  @param component 0 for red, 1 for green and 2 for blue component.
   *  @return The pointer to an float field with w*h entries or 0 if the image is not a color image.
   */
  const unsigned char *byte_data (unsigned int component) const;

  /**
   *  @brief Accessor to the image data field
   *
   *  @return The pointer to an float field with w*h entries or 0 if the image is a color image.
   *  This method returns 0 if the data is stored as bytes (is_byte_data() is true).
   */
  const float *float_data () const;

  /**
   *  @brief Accessor to the image data field (color component)
   *
   *  @return The pointer to an float field with w*h entries or 0 if the image is not a color image.
   *  This method returns 0 if the data is stored as bytes (is_byte_data() is true).
   *
   *  @param component 0 for red, 1 for green and 2 for blue component.
   *  @return The pointer to an float field with w*h entries or 0 if the image is not a color image.
   */
  const float *float_data (unsigned int component) const;

  /**
   *  @brief Write accessor to the image data field (monochrome)
   *
   *  This method must be passed a new'd pointer to a unsigned char array with a length of width * height.
   *  The pointer to the data set becomes owned by the image object.
   */
  void set_data (size_t width, size_t height, unsigned char *d);

  /**
   *  @brief Write accessor to the image data field (monochrome)
   *
   *  This method must be passed a new'd pointer to a float array with a length of width * height.
   *  The pointer to the data set becomes owned by the image object.
   */
  void set_data (size_t width, size_t height, float *d);

  /**
   *  @brief Write accessor to the image data field (monochrome)
   *
   *  This variant takes the data from a field of double values.
   */
  void set_data (size_t width, size_t height, const std::vector<double> &d);

  /**
   *  @brief Write accessor to the image data field (color)
   *
   *  This method must be passed new'd pointers to unsigned char arrays with a length of "data_length()".
   *  The pointers to the data set become owned by the image object.
   */
  void set_data (size_t width, size_t height, unsigned char *red, unsigned char *green, unsigned char *blue);

  /**
   *  @brief Write accessor to the image data field (color)
   *
   *  This method must be passed new'd pointers to float arrays with a length of "data_length()".
   *  The pointers to the data set become owned by the image object.
   */
  void set_data (size_t width, size_t height, float *red, float *green, float *blue);

  /**
   *  @brief Write accessor to the image data field (monochrome)
   *
   *  This variant takes the data from three fields of double values.
   */
  void set_data (size_t width, size_t height, const std::vector<double> &red, const std::vector<double> &green, const std::vector<double> &blue);

  /**
   *  @brief Clears the pixel data (sets the values to 0)
   */
  void clear ();

  /**
   *  @brief Set the transformation matrix
   *
   *  This transformation matrix converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)
   *  to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.
   */
  void set_matrix (const db::Matrix3d &trans);

  /**
   *  @brief Return the pixel-to-micron transformation
   *
   *  This transformation converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)
   *  to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.
   */
  const db::Matrix3d &matrix () const
  {
    return m_trans;
  }

  /**
   *  @brief Get the upper limit of the values in the data set
   *
   *  This value determines the upper end of the data mapping (i.e. white value etc.).
   *  It does not necessarily correspond to the minimum value of the data set but it must be
   *  larger than that.
   */
  double min_value () const
  {
    return m_min_value;
  }

  /** 
   *  @brief Set the minimum value
   *
   *  See the \min_value method for the description of the minimum value property.
   */
  void set_min_value (double h);

  /**
   *  @brief Get the upper limit of the values in the data set
   *
   *  This value determines the upper end of the data mapping (i.e. white value etc.).
   *  It does not necessarily correspond to the maximum value of the data set but it must be
   *  larger than that.
   */
  double max_value () const
  {
    return m_max_value;
  }

  /** 
   *  @brief Set the maximum value
   *
   *  See the \max_value method for the description of the maximum value property.
   */
  void set_max_value (double h);

  /**  
   *  @brief Get the Id 
   *  
   *  Upon initialization, an unique Id is given to the object. The Id is used to 
   *  identify the object in the context of the service.
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Sets the ID
   *
   *  NOTE: this method is only to be used for internal purposes. Don't change the ID of an object
   *  intentionally.
   */
  void id (int _id)
  {
    m_id = _id;
  }

  /**
   *  @brief Get the data mapping
   */
  const DataMapping &data_mapping () const
  {
    return m_data_mapping;
  }

  /**
   *  @brief Set the data mapping
   */
  void set_data_mapping (const DataMapping &dm);

  /**
   *  @brief Get the visibility flag
   */
  bool is_visible () const
  {
    return m_visible;
  }

  /**
   *  @brief Set the visibility
   */
  void set_visible (bool v) 
  {
    if (m_visible != v) {
      m_visible = v;
      if (m_updates_enabled) {
        property_changed ();
      }
    }
  }

  /**
   *  @brief Sets the z position
   */
  void set_z_position (int z)
  {
    if (m_z_position != z) {
      m_z_position = z;
      if (m_updates_enabled) {
        property_changed ();
      }
    }
  }

  /**
   *  @brief Gets the z position
   */
  int z_position () const
  {
    return m_z_position;
  }

  /**
   *  @brief Get the RGB pixel data sets obtained by applying the LUT's
   */
  const tl::color_t *pixel_data () const
  {
    validate_pixel_data ();
    return mp_pixel_data;
  }

  /**
   *  @brief Load the data from the given file
   *
   *  @param adjust_min_max True, if min and max values shall be adjusted
   */
  void load_data (const std::string &filename, bool adjust_min_max = true);

  /**
   *  @brief Get the landmarks 
   *
   *  The landmark coordinates are given relative to the center of the image.
   */
  const landmarks_type &landmarks () const;

  /**
   *  @brief Sets the landmarks
   */
  void set_landmarks (const landmarks_type &lm);

  /**
   *  @brief Check the given matrix for validity
   *
   *  A matrix is valid if the z coordinate does not become zero or negative.
   */
  bool is_valid_matrix (const db::Matrix3d &matrix);

  /**
   *  @brief Equality
   */
  bool operator== (const img::Object &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const img::Object &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief The class name for the generic user object factory 
   */
  virtual const char *class_name () const;

  /**
   *  @brief Fill from a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual void from_string (const char *str, const char *base_dir = 0);

  /**
   *  @brief Convert to a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual std::string to_string () const;

  /**
   *  @brief Swap with another image object
   */
  void swap (img::Object &other);

  /**
   *  @brief Return the memory used in bytes
   */
  virtual void mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

protected:
  /**
   *  @brief A notification method that is called when a property of the annotation has changed
   */
  virtual void property_changed ();

private:
  std::string m_filename;
  db::Matrix3d m_trans;
  DataHeader *mp_data;
  size_t m_id;
  double m_min_value, m_max_value;
  bool m_min_value_set, m_max_value_set;
  DataMapping m_data_mapping;
  bool m_visible;
  mutable const tl::color_t *mp_pixel_data;
  std::vector <db::DPoint> m_landmarks;
  int m_z_position;
  bool m_updates_enabled;

  void release ();
  void invalidate_pixel_data ();
  void validate_pixel_data () const;
  void allocate (bool color);
  void read_file ();
#if defined(HAVE_QT)
  void create_from_qimage (const QImage &qimage);
#endif
  void create_from_pixel_buffer (const tl::PixelBuffer &img);
};

}

#endif

