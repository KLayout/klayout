
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_layStreamImportDialog
#define HDR_layStreamImportDialog

#include <QDialog>
#include <QAction>
#include <QFrame>
#include <QComboBox>

#include "dbLayout.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "dbLayerProperties.h"
#include "dbLoadLayoutOptions.h"

#include <string>
#include <vector>
#include <algorithm>

class QTreeWidgetItem;
class QTreeWidget;
class QToolButton;
class QLineEdit;

namespace Ui
{
  class StreamImportDialog;
}

namespace tl
{
  class InputStream;
}

namespace lay
{
  class Dispatcher;
}

namespace lay
{

class StreamImporter;

struct StreamImportData
{
public:
  StreamImportData ();

  enum mode_type { Simple = 0, Instantiate = 1, Extra = 2, Merge = 3 };
  enum layer_mode_type { Original = 0, Offset = 1 };

  mode_type mode;
  std::vector<std::string> files;
  std::string topcell;
  std::vector <std::pair <db::DPoint, db::DPoint> > reference_points;
  db::DCplxTrans explicit_trans;
  layer_mode_type layer_mode;
  db::LayerOffset layer_offset;
  db::LoadLayoutOptions options;

  void setup_importer (StreamImporter *importer);
  void from_string (const std::string &s);
  std::string to_string () const;
};

class StreamImportDialog
  : public QDialog
{
Q_OBJECT 

public:
  StreamImportDialog (QWidget *parent, StreamImportData *data);
  ~StreamImportDialog ();

  int exec ();
  void accept ();
  void reject ();

public slots:
  void next_page ();
  void last_page ();
  void browse_filename ();
  void edit_options ();
  void reset_options ();
  void reset ();
  void mapping_changed ();

private:
  StreamImportData *mp_data;
  Ui::StreamImportDialog *mp_ui;

  void update ();
  void commit_page ();
  void enter_page ();
};

}

#endif

