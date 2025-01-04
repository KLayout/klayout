
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


#ifndef HDR_layBooleanOperationsDialogs
#define HDR_layBooleanOperationsDialogs

#include "ui_BooleanOptionsDialog.h"
#include "ui_SizingOptionsDialog.h"
#include "ui_MergeOptionsDialog.h"

namespace db
{
  class Layout;
}

namespace lay
{

class CellView;
class LayoutViewBase;

/**
 *  @brief The boolean operation options
 */
class BooleanOptionsDialog 
  : public QDialog,
    public Ui::BooleanOptionsDialog
{
Q_OBJECT

public:
  BooleanOptionsDialog (QWidget *parent);
  virtual ~BooleanOptionsDialog ();

  bool exec_dialog (lay::LayoutViewBase *view, int &cv_a, int &layer_a, int &cv_b, int &layer_b, int &cv_res, int &layer_res, int &mode, int &hier_mode, bool &min_coherence);

public slots:
  void cv_changed (int);

private:
  virtual void accept ();

  lay::LayoutViewBase *mp_view;
};

/**
 *  @brief The sizing operation options
 */
class SizingOptionsDialog 
  : public QDialog,
    public Ui::SizingOptionsDialog
{
Q_OBJECT

public:
  SizingOptionsDialog (QWidget *parent);
  virtual ~SizingOptionsDialog ();

  bool exec_dialog (lay::LayoutViewBase *view, int &cv, int &layer, int &cv_res, int &layer_res, double &dx, double &dy, unsigned int &size_mode, int &hier_mode, bool &min_coherence);

public slots:
  void cv_changed (int);

private:
  virtual void accept ();

  lay::LayoutViewBase *mp_view;
};

/**
 *  @brief The merge operation options
 */
class MergeOptionsDialog 
  : public QDialog,
    public Ui::MergeOptionsDialog
{
Q_OBJECT

public:
  MergeOptionsDialog (QWidget *parent);
  virtual ~MergeOptionsDialog ();

  bool exec_dialog (lay::LayoutViewBase *view, int &cv, int &layer, int &cv_res, int &layer_res, unsigned int &min_wc, int &hier_mode, bool &min_coherence);

public slots:
  void cv_changed (int);

private:
  virtual void accept ();

  lay::LayoutViewBase *mp_view;
};

}

#endif



