
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


#ifndef HDR_layTextInfo
#define HDR_layTextInfo

#include "laybasicCommon.h"

#include "dbText.h"
#include "dbBox.h"

namespace lay
{

class LayoutViewBase;

/**
 *  @brief A class providing information about a text's visual bounding box
 *
 *  The class can act as a BoxConverter.
 */
class LAYBASIC_PUBLIC TextInfo
{
public:
  /**
   *  @brief Constructor
   *
   *  @param view The LayoutView from which to take the text display parameters
   */
  TextInfo (const LayoutViewBase *view);

  /**
   *  @brief Constructor
   *
   *  @param default_text_size The default text size in micron
   *  @param default_font The default font
   *  @param apply_text_trans True if text transformations are to be applied
   *  @param resolution The resolution value (logical pixel size per physical unit pixel)
   *  @param vp_trans The effective micron-to-pixel transformation
   */
  TextInfo (double default_text_size, const db::Font &default_font, bool apply_text_trans, double resolution);

  /**
   *  @brief Gets the visual bounding box of the given DText object
   *
   *  The visual bounding box is returned in micrometer units.
   *  It encloses the glyphs of the text, taking into account the
   *  text view settings by the view.
   *
   *  @param text The text object
   *  @param vp_trans The effective micron-to-pixel transformation
   */
  db::DBox bbox (const db::DText &text, const db::DCplxTrans &vp_trans) const;
  
private:
  double m_default_text_size;
  db::Font m_default_font;
  bool m_apply_text_trans;
  double m_resolution;
};

}

#endif

