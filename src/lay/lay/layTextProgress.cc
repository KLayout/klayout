
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "layTextProgress.h"
#include "tlLog.h"

namespace lay
{

TextProgress::TextProgress (int verbosity)
  : m_verbosity (verbosity)
{
  //  .. nothing yet ..
}

void TextProgress::update_progress (tl::Progress *progress)
{
  if (! progress || progress->is_abstract ()) {
    return;
  }

  std::string text = progress->desc ();
  if (m_progress_text != text && tl::verbosity () >= m_verbosity) {
    tl::info << text << " ..";
    m_progress_text = text;
  }

  std::string value = progress->formatted_value ();
  for (tl::Progress *p = progress->next (); p != 0; p = p->next ()) {
    value += " " + p->formatted_value ();
  }

  if (m_progress_value != value && tl::verbosity () >= m_verbosity) {
    tl::info << ".. " << value;
    m_progress_value = value;
  }
}

void TextProgress::show_progress_bar (bool /*show*/)
{
  m_progress_text.clear ();
  m_progress_value.clear ();
}

}
