
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


#ifndef HDR_edtDialogs
#define HDR_edtDialogs

#include <limits>
#include <list>
#include <utility>

#include <QDialog>

#include "dbLayout.h"
#include "dbPoint.h"

#include "ui_InstantiationForm.h"
#include "ui_ChangeLayerOptionsDialog.h"
#include "ui_AlignOptionsDialog.h"
#include "ui_DistributeOptionsDialog.h"
#include "ui_CopyModeDialog.h"
#include "ui_MakeCellOptionsDialog.h"
#include "ui_MakeArrayOptionsDialog.h"
#include "ui_RoundCornerOptionsDialog.h"
#include "ui_AreaAndPerimeterDialog.h"

namespace lay
{
  class LayoutViewBase;
  class Marker;
  class ObjectInstPath;
}

namespace edt {

/**
 *  @brief The copy mode dialog
 */
class CopyModeDialog 
  : public QDialog,
    public Ui::CopyModeDialog
{
Q_OBJECT

public:
  CopyModeDialog (QWidget *parent);
  virtual ~CopyModeDialog ();

  bool exec_dialog (unsigned int &mode, bool &dont_ask);
};

/**
 *  @brief The instantiation report form
 */
class InstantiationForm 
  : public QDialog,
    public Ui::InstantiationForm
{
Q_OBJECT

public:
  InstantiationForm (QWidget *parent);
  virtual ~InstantiationForm ();

  void show (lay::LayoutViewBase *view, const lay::ObjectInstPath &path);

public slots:
  void display_mode_changed (bool);
  void double_clicked (QListWidgetItem *item);

private:
  void update ();

  lay::LayoutViewBase *mp_view;
  const lay::ObjectInstPath *mp_path;
  lay::Marker *mp_marker;
  bool m_enable_cb_callbacks;
};

/**
 *  @brief The change layer options dialog
 */
class ChangeLayerOptionsDialog 
  : public QDialog,
    public Ui::ChangeLayerOptionsDialog
{
Q_OBJECT

public:
  ChangeLayerOptionsDialog (QWidget *parent);
  virtual ~ChangeLayerOptionsDialog ();

  bool exec_dialog (lay::LayoutViewBase *view, int cv_index, unsigned int &new_layer);
};

/**
 *  @brief Align function options dialog
 */
class AlignOptionsDialog 
  : public QDialog,
    public Ui::AlignOptionsDialog
{
Q_OBJECT

public:
  AlignOptionsDialog (QWidget *parent);
  virtual ~AlignOptionsDialog ();

  bool exec_dialog (int &hmode, int &vmode, bool &visible_layers);
};

/**
 *  @brief Distribute function options dialog
 */
class DistributeOptionsDialog
  : public QDialog,
    public Ui::DistributeOptionsDialog
{
Q_OBJECT

public:
  DistributeOptionsDialog (QWidget *parent);
  virtual ~DistributeOptionsDialog ();

  bool exec_dialog (bool &hdistribute, int &hmode, double &hpitch, double &hspace, bool &vdistribute, int &vmode, double &vpitch, double &vspace, bool &visible_layers);
};

/**
 *  @brief Options dialog for the "make cell" function
 */
class MakeCellOptionsDialog
  : public QDialog, 
    private Ui::MakeCellOptionsDialog
{
Q_OBJECT

public:
  MakeCellOptionsDialog (QWidget *parent);
  bool exec_dialog (const db::Layout &layout, std::string &name, int &mode_x, int &mode_y);

private slots:
  void button_clicked ();
};

/**
 *  @brief Options dialog for the "make array" function
 */
class MakeArrayOptionsDialog
  : public QDialog, 
    private Ui::MakeArrayOptionsDialog
{
Q_OBJECT

public:
  MakeArrayOptionsDialog (QWidget *parent);
  bool exec_dialog (db::DVector &a, unsigned int &na, db::DVector &b, unsigned int &nb);

  virtual void accept ();
};

/**
 *  @brief Options dialog for the "round corners" function
 */
class RoundCornerOptionsDialog
  : public QDialog, 
    private Ui::RoundCornerOptionsDialog
{
Q_OBJECT

public:
  RoundCornerOptionsDialog (QWidget *parent);
  ~RoundCornerOptionsDialog ();

  bool exec_dialog (const db::Layout &layout, double &router, double &rinner, unsigned int &npoints, bool &undo_before_apply, double router_extracted, double rinner_extracted, unsigned int npoints_extracted, bool has_extracted);

  virtual void accept ();

private slots:
  void amend_changed ();

private:
  const db::Layout *mp_layout;
  double m_router_extracted, m_rinner_extracted;
  unsigned int m_npoints_extracted;
  bool m_has_extracted;
};

/**
 *  @brief Result dialog for "area and perimeter"
 */
class AreaAndPerimeterDialog
  : public QDialog,
    private Ui::AreaAndPerimeterDialog
{
Q_OBJECT

public:
  AreaAndPerimeterDialog (QWidget *parent);
  ~AreaAndPerimeterDialog ();

  bool exec_dialog (double area, double perimeter);
};

} // namespace edt

#endif

#endif

