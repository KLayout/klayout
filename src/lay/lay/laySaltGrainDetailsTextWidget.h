
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

#ifndef HDR_laySaltGrainDetailsTextWidget
#define HDR_laySaltGrainDetailsTextWidget

#include "laySaltGrain.h"

#include <QTextBrowser>
#include <memory>

namespace lay
{

class SaltGrain;

/**
 *  @brief A specialization of QTextBrowser that displays the details of the salt grain
 */
class SaltGrainDetailsTextWidget
  : public QTextBrowser
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  SaltGrainDetailsTextWidget (QWidget *w);

  /**
   *  @brief Sets the grain whose details are to be shown
   */
  void set_grain (const SaltGrain *g);

protected:
  virtual QVariant loadResource (int type, const QUrl &url);

public slots:
  void show_detailed_view (bool f);

private slots:
  void open_link (const QUrl &url);

private:
  std::unique_ptr<lay::SaltGrain> mp_grain;

  QString details_text ();
  bool m_detailed_view;
};

}

#endif
