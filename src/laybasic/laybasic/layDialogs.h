
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef HDR_layDialogs
#define HDR_layDialogs

#include "ui_LayerSourceDialog.h"
#include "ui_NewLayoutPropertiesDialog.h"
#include "ui_NewLayerPropertiesDialog.h"
#include "ui_NewCellPropertiesDialog.h"
#include "ui_MoveOptionsDialog.h"
#include "ui_MoveToOptionsDialog.h"
#include "ui_DeleteCellModeDialog.h"
#include "ui_CopyCellModeDialog.h"
#include "ui_ReplaceCellOptionsDialog.h"
#include "ui_ClearLayerModeDialog.h"
#include "ui_OpenLayoutModeDialog.h"
#include "ui_RenameCellDialog.h"
#include "ui_DuplicateLayerDialog.h"
#include "ui_AlignCellOptionsDialog.h"
#include "ui_FlattenInstOptionsDialog.h"
#include "ui_UserPropertiesForm.h"
#include "ui_UserPropertiesEditForm.h"

#include "dbPoint.h"
#include "dbVector.h"
#include "dbTypes.h"
#include "laybasicCommon.h"

namespace db
{
  class Layout;
}

namespace lay
{

class CellView;
class LayoutView;

/**
 *  @brief The layer source dialog
 */
class LAYBASIC_PUBLIC LayerSourceDialog
  : public QDialog,
    public Ui::LayerSourceDialog
{
Q_OBJECT

public:
  LayerSourceDialog (QWidget *parent);

  bool exec_dialog (std::string &s);
};

/**
 *  @brief The new cell properties dialog
 */
class LAYBASIC_PUBLIC NewCellPropertiesDialog 
  : public QDialog,
    public Ui::NewCellPropertiesDialog
{
Q_OBJECT

public:
  NewCellPropertiesDialog (QWidget *parent);
  virtual ~NewCellPropertiesDialog ();

  bool exec_dialog (const db::Layout *layout, std::string &cell_name, double &size);

private:
  virtual void accept ();

  const db::Layout *mp_layout;
};

/**
 *  @brief The new layer properties dialog
 */
class LAYBASIC_PUBLIC NewLayerPropertiesDialog 
  : public QDialog,
    public Ui::NewLayerPropertiesDialog
{
Q_OBJECT

public:
  NewLayerPropertiesDialog (QWidget *parent);
  virtual ~NewLayerPropertiesDialog ();

  bool exec_dialog (const lay::CellView &cv, db::LayerProperties &src);
  bool exec_dialog (db::LayerProperties &src);

private:
  virtual void accept ();
  void get (db::LayerProperties &src);
};

/**
 *  @brief The move options dialog
 */
class LAYBASIC_PUBLIC MoveOptionsDialog 
  : public QDialog,
    public Ui::MoveOptionsDialog
{
Q_OBJECT

public:
  MoveOptionsDialog (QWidget *parent);
  virtual ~MoveOptionsDialog ();

  bool exec_dialog (db::DVector &disp);

private:
  virtual void accept ();
};

/**
 *  @brief The move "to" options dialog
 */
class LAYBASIC_PUBLIC MoveToOptionsDialog 
  : public QDialog,
    public Ui::MoveToOptionsDialog
{
Q_OBJECT

public:
  MoveToOptionsDialog (QWidget *parent);
  virtual ~MoveToOptionsDialog ();

  bool exec_dialog (int &mode_x, int &mode_y, db::DPoint &target);

private:
  virtual void accept ();

private slots:
  void button_clicked ();
};

/**
 *  @brief The rename cell options dialog
 */
class LAYBASIC_PUBLIC RenameCellDialog 
  : public QDialog,
    public Ui::RenameCellDialog
{
Q_OBJECT

public:
  RenameCellDialog (QWidget *parent);
  virtual ~RenameCellDialog ();

  bool exec_dialog (const db::Layout &layout, std::string &name);

private:
  virtual void accept ();

  const db::Layout *mp_layout;
};

/**
 *  @brief The replace cell options dialog
 */
class LAYBASIC_PUBLIC ReplaceCellOptionsDialog
  : public QDialog,
    public Ui::ReplaceCellOptionsDialog
{
Q_OBJECT

public:
  ReplaceCellOptionsDialog (QWidget *parent);
  virtual ~ReplaceCellOptionsDialog ();

  /** 
   *  @brief Execute the dialog
   *
   *  The mode is either 0 (for shallow), 1 (for deep) and 2 (for complete)
   */
  bool exec_dialog (const lay::CellView &cv, int &replace_mode, db::cell_index_type &cell);

protected:
  virtual void accept ();
};

/**
 *  @brief The copy cell options dialog
 */
class LAYBASIC_PUBLIC CopyCellModeDialog 
  : public QDialog,
    public Ui::CopyCellModeDialog
{
Q_OBJECT

public:
  CopyCellModeDialog (QWidget *parent);
  virtual ~CopyCellModeDialog ();

  /** 
   *  @brief Execute the dialog
   *
   *  The mode is either 0 (for shallow), 1 (for deep)
   */
  bool exec_dialog (int &copy_mode);
};

/**
 *  @brief The delete cell options dialog
 */
class LAYBASIC_PUBLIC DeleteCellModeDialog 
  : public QDialog,
    public Ui::DeleteCellModeDialog
{
Q_OBJECT

public:
  DeleteCellModeDialog (QWidget *parent);
  virtual ~DeleteCellModeDialog ();

  /** 
   *  @brief Execute the dialog
   *
   *  The mode is either 0 (for shallow), 1 (for deep) and 2 (for complete)
   */
  bool exec_dialog (int &delete_mode);
};

/**
 *  @brief The delete cell options dialog
 */
class LAYBASIC_PUBLIC ClearLayerModeDialog 
  : public QDialog,
    public Ui::ClearLayerModeDialog
{
Q_OBJECT

public:
  ClearLayerModeDialog (QWidget *parent);
  virtual ~ClearLayerModeDialog ();

  /** 
   *  @brief Execute the dialog
   *
   *  The mode is either 0 (for locally), 1 (for hierarchically) and 2 (for all)
   */
  bool exec_dialog (int &clear_mode);
};

/**
 *  @brief The open layout mode dialog
 */
class LAYBASIC_PUBLIC OpenLayoutModeDialog 
  : public QDialog,
    public Ui::OpenLayoutModeDialog
{
Q_OBJECT

public:
  OpenLayoutModeDialog (QWidget *parent);
  virtual ~OpenLayoutModeDialog ();

  /** 
   *  @brief Execute the dialog
   *
   *  The mode is either 0 (to replace current view), 1 (to create new view) and 2 (add to current view)
   */
  bool exec_dialog (int &open_mode);
};

/**
 *  @brief The new layout properties dialog
 */
class LAYBASIC_PUBLIC NewLayoutPropertiesDialog 
  : public QDialog,
    public Ui::NewLayoutPropertiesDialog
{
Q_OBJECT

public:
  NewLayoutPropertiesDialog (QWidget *parent);
  virtual ~NewLayoutPropertiesDialog ();

  bool exec_dialog (std::string &tech_name, std::string &cell_name, double &dbu, double &window_size, bool &current_panel);

private slots:
  void tech_changed ();

private:
  virtual void accept ();
};

/**
 *  @brief The duplicate layer operation options
 */
class LAYBASIC_PUBLIC DuplicateLayerDialog 
  : public QDialog,
    public Ui::DuplicateLayerDialog
{
Q_OBJECT

public:
  DuplicateLayerDialog (QWidget *parent);
  virtual ~DuplicateLayerDialog ();

  bool exec_dialog (lay::LayoutView *view, int &cv, int &layer, int &cv_res, int &layer_res, int &hier_mode, bool &clear_before);

public slots:
  void cv_changed (int);

private:
  virtual void accept ();

  lay::LayoutView *mp_view;
};

/**
 *  @brief The merge operation options
 */
class LAYBASIC_PUBLIC AlignCellOptionsDialog 
  : public QDialog,
    public Ui::AlignCellOptionsDialog
{
Q_OBJECT

public:
  AlignCellOptionsDialog (QWidget *parent);
  virtual ~AlignCellOptionsDialog ();

  bool exec_dialog (int &mode_x, int &mode_y, bool &visible_only, bool &adjust_calls);

public slots:
  void button_clicked ();
};

/**
 *  @brief Options dialog for the "flatten instances" function
 */
class LAYBASIC_PUBLIC FlattenInstOptionsDialog
  : public QDialog, 
    private Ui::FlattenInstOptionsDialog
{
public:
  FlattenInstOptionsDialog (QWidget *parent, bool enable_pruning = true);
  bool exec_dialog (int &levels, bool &prune); 
};

/**
 *  @brief The user properties report form
 */
class LAYBASIC_PUBLIC UserPropertiesForm 
  : public QDialog,
    public Ui::UserPropertiesForm
{
Q_OBJECT

public:
  UserPropertiesForm (QWidget *parent);

  bool show (lay::LayoutView *view, unsigned int cv_index, db::properties_id_type &prop_id);

public slots:
  void add ();
  void remove ();
  void edit ();
  void dbl_clicked (QTreeWidgetItem *, int);

private:
  bool m_editable;
};

/**
 *  @brief The user properties report form
 */
class LAYBASIC_PUBLIC UserPropertiesEditForm 
  : public QDialog,
    public Ui::UserPropertiesEditForm
{
public:
  UserPropertiesEditForm (QWidget *parent);

  bool show (QString &key, QString &value);
  virtual void accept ();
};

}

#endif


