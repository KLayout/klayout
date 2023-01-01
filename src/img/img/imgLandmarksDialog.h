
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

#if defined(HAVE_QT)

#ifndef HDR_imgLandmarksDialog
#define HDR_imgLandmarksDialog

#include <QDialog>

#include "ui_ImageLandmarksDialog.h"
#include "tlObject.h"

namespace img
{

class Object;
class LandmarkEditorService;

/**
 *  @brief The landmark editor for the image dialog
 */
class LandmarksDialog :
  public QDialog,
  public Ui::ImageLandmarksDialog,
  public tl::Object
{
Q_OBJECT

public:
  LandmarksDialog (QWidget *parent, img::Object &img);
  ~LandmarksDialog ();

  enum mode_t { Move, Add, Delete, None } m_mode;

private slots:
  void update_mode ();
  void accept ();

private:
  LandmarkEditorService *mp_service;
  img::Object *mp_image, *mp_original_image;

  void landmarks_updated ();
};

}

#endif

#endif
