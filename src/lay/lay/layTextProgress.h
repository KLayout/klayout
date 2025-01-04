
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#ifndef HDR_layTextProgress
#define HDR_layTextProgress

#include "layCommon.h"

#include "layProgress.h"

#include <string>

namespace lay
{

/**
 *  @brief A helper to show the progress in the terminal
 */
class TextProgress
  : public lay::ProgressBar
{
public:
  /**
   *  @brief Constructor
   *  @param verbosity The verbosity threshold above (or equal to) which the progress is printed
   */
  TextProgress (int verbosity);

  virtual void update_progress (tl::Progress *progress);
  virtual void show_progress_bar (bool show);

private:
  int m_verbosity;
  std::string m_progress_text, m_progress_value;
};

}

#endif
