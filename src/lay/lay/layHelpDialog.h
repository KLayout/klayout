
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


#ifndef HDR_layHelpDialog
#define HDR_layHelpDialog

#include "layCommon.h"

#include <QDialog>

#include <memory>
#include <string>

namespace Ui
{
  class HelpDialog;
}

namespace lay
{

class HelpSource;
class BrowserPanel;

/**
 *  @brief The help dialog (aka assistant)
 */
class HelpDialog 
  : public QDialog 
{
Q_OBJECT 

public:
  HelpDialog (QWidget *parent, bool modal = false);
  ~HelpDialog ();

  void search (const std::string &topic);
  void load (const std::string &url);
  void showEvent (QShowEvent *);
  void hideEvent (QHideEvent *);

protected slots:
  void title_changed (const QString &t);

private:
  Ui::HelpDialog *mp_ui;
  QRect m_geometry;
  static lay::HelpSource *mp_help_source;
  QString m_def_title;
  bool m_initialized;

  void initialize ();
};

}

#endif

