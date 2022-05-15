

#ifndef HDR_layFixedFont
#define HDR_layFixedFont

#include "laybasicCommon.h"

#include <stdint.h>

namespace lay
{

/**
 *  @brief A descriptor class for a fixed font
 */

class LAYBASIC_PUBLIC FixedFont
{
public:
  /**
   *  @brief ctor
   */
  FixedFont (unsigned int h, unsigned int lh, unsigned int w, unsigned char c0, unsigned char nc, uint32_t *d, unsigned int stride);

  /**
   *  @brief Factory
   * 
   *  This renders a FixedFont object for the given resolution.
   */
  static const FixedFont &get_font (double resolution);

  /**
   *  @brief Gets the number of font sizes available
   */
  static int font_sizes ();

  /**
   *  @brief Gets the size description ("small", "large", ...)
   */
  static const char *font_size_name (int sz);

  /**
   *  @brief Set the default font size 
   *
   *  Allowed values are 0 (small), 1 (medium) or 2 (large) etc.
   */
  static void set_default_font_size (int fs);

  /**
   *  @brief Gets the default font size
   */
  static int default_font_size () 
  {
    return ms_default_font_size;
  }

  /**
   *  @brief Character height
   */
  unsigned int height () const
  {
    return m_height;
  }

  /**
   *  @brief Character line height
   */
  unsigned int line_height () const
  {
    return m_line_height;
  }

  /**
   *  @brief Character width
   */
  unsigned int width () const
  {
    return m_width;
  }

  /**
   *  @brief First character
   */
  unsigned char first_char () const
  {
    return m_first_char;
  }

  /**
   *  @brief Number of characters
   */
  unsigned char n_chars () const
  {
    return m_n_chars;
  }

  /**
   *  @brief Character data
   */
  const uint32_t *data () const
  {
    return mp_data;
  }

  /**
   *  @brief Gets the stride (number of words per line)
   */
  unsigned int stride () const
  {
    return m_stride;
  }

private:
  unsigned int m_height, m_line_height, m_width;
  unsigned char m_first_char;
  unsigned char m_n_chars;
  uint32_t *mp_data;
  unsigned int m_stride;
  static int ms_default_font_size;
};

}

#endif

