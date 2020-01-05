
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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



#ifndef HDR_layNetTracerIO
#define HDR_layNetTracerIO

#include "ui_NetTracerTechComponentEditor.h"

#include "dbNetTracer.h"
#include "dbNetTracerIO.h"
#include "dbTechnology.h"

#include "layNetTracerConfig.h"
#include "layBrowser.h"
#include "layPlugin.h"
#include "layViewObject.h"
#include "layMarker.h"
#include "layTechnology.h"

#include "tlObject.h"

namespace db
{
  class NetTracerTechnologyComponent;
}

namespace lay
{

class FileDialog;

class NetTracerTechComponentEditor
  : public lay::TechnologyComponentEditor,
    public Ui::NetTracerTechComponentEditor
{
Q_OBJECT

public:
  NetTracerTechComponentEditor (QWidget *parent);

  void commit ();
  void setup ();

public slots:
  void add_clicked ();
  void del_clicked ();
  void move_up_clicked ();
  void move_down_clicked ();
  void symbol_add_clicked ();
  void symbol_del_clicked ();
  void symbol_move_up_clicked ();
  void symbol_move_down_clicked ();

private:
  db::NetTracerTechnologyComponent m_data;

  void update ();
};

}

#endif

