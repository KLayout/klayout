
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_edtPCellParametersPage
#define HDR_edtPCellParametersPage

#include "dbPCellDeclaration.h"
#include "tlDeferredExecution.h"

#include <QFrame>
#include <QScrollArea>
#include <QLabel>

namespace lay
{
  class LayoutView;
}

namespace edt
{

/**
 *  @brief A QScrollArea that displays and allows editing PCell parameters
 */
class PCellParametersPage
  : public QFrame, public tl::Object
{
Q_OBJECT

public:
  struct State
  {
    State () : valid (false), hScrollPosition (0), vScrollPosition (0) { }

    bool valid;
    int hScrollPosition;
    int vScrollPosition;
    QString focusWidget;
  };

  /**
   *  @brief Default constructor
   *
   *  Use "setup" to configure the page.
   *
   *  @param dense Use a dense layout if true
   */
  PCellParametersPage (QWidget *parent, bool dense = false);

  /**
   *  @brief initialization
   *
   *  Use this method to setup when the arguments are not available in the constructor
   *
   *  @param layout The layout in which the PCell instance resides
   *  @param view The layout view from which to take layers for example
   *  @param cv_index The index of the cellview in "view"
   *  @param pcell_decl The PCell declaration
   *  @param parameters The parameter values to show (if empty, the default values are used)
   */
  void setup (lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters);

  /**
   *  @brief Gets the pages current state
   */
  State get_state ();

  /**
   *  @brief Restores the state
   */
  void set_state (const State &s);

  /**
   *  @brief Get the current parameters
   *
   *  *ok is set to true, if there is no error. In case of an error it's set to false.
   *  The error is indicated in the error label in the editor page.
   *  If ok is null, an exception is thrown.
   */
  std::vector<tl::Variant> get_parameters (bool *ok = 0);

  /**
   *  @brief Get the PCell declaration pointer
   */
  const db::PCellDeclaration *pcell_decl () const
  {
    return mp_pcell_decl.get ();
  }

  /**
   *  @brief Sets the given parameters as values
   */
  void set_parameters (const  std::vector<tl::Variant> &values);

signals:
  void edited ();

private slots:
  void parameter_changed ();

private:
  QScrollArea *mp_parameters_area;
  QLabel *mp_error_label;
  QLabel *mp_error_icon;
  tl::weak_ptr<db::PCellDeclaration> mp_pcell_decl;
  std::vector<QWidget *> m_widgets;
  lay::LayoutView *mp_view;
  int m_cv_index;
  db::pcell_parameters_type m_parameters;
  bool m_dense;
  tl::DeferredMethod<PCellParametersPage> dm_parameter_changed;

  void init ();
  void do_parameter_changed ();
};

}

#endif
