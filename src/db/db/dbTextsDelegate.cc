
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


#include "dbTextsDelegate.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------

TextsDelegate::TextsDelegate ()
{
  m_report_progress = false;
}

TextsDelegate::TextsDelegate (const TextsDelegate &other)
  : db::ShapeCollectionDelegateBase ()
{
  operator= (other);
}

TextsDelegate &
TextsDelegate::operator= (const TextsDelegate &other)
{
  if (this != &other) {
    m_report_progress = other.m_report_progress;
  }
  return *this;
}

TextsDelegate::~TextsDelegate ()
{
  //  .. nothing yet ..
}

void TextsDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void TextsDelegate::disable_progress ()
{
  m_report_progress = false;
}

}

