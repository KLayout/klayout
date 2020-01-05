
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


#ifndef HDR_edtPCellParametersPage
#define HDR_edtPCellParametersPage

#include "dbPCellDeclaration.h"

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
  : public QFrame
{
Q_OBJECT

public:
  struct State
  {
    State () : valid (false), hScrollPosition (0), vScrollPosition (0) { }

    bool valid;
    int hScrollPosition;
    int vScrollPosition;
  };

  /**
   *  @brief Constructor: creates a page showing the given parameters
   *
   *  @param parent The parent widget
   *  @param layout The layout in which the PCell instance resides
   *  @param view The layout view from which to take layers for example
   *  @param cv_index The index of the cellview in "view"
   *  @param pcell_decl The PCell declaration
   *  @param parameters The parameter values to show (if empty, the default values are used)
   */
  PCellParametersPage (QWidget *parent, const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters);

  /**
   *  @brief Default constructor
   *
   *  Use "setup" to configure the page.
   */
  PCellParametersPage (QWidget *parent);

  /**
   *  @brief Delayed initialization
   *
   *  Use this method to setup when the arguments are not available in the constructor
   */
  void setup (const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &parameters);

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
   */
  std::vector<tl::Variant> get_parameters ();

  /**
   *  @brief Get the PCell declaration pointer
   */
  const db::PCellDeclaration *pcell_decl () const
  {
    return mp_pcell_decl;
  }

  /**
   *  @brief Sets the given parameters as values
   */
  void set_parameters (const  std::vector<tl::Variant> &values);

public slots:
  void activated ();
  void clicked ();
  
private:
  QScrollArea *mp_parameters_area;
  QLabel *mp_error_label;
  QLabel *mp_error_icon;
  const db::PCellDeclaration *mp_pcell_decl;
  std::vector<QWidget *> m_widgets;
  const db::Layout *mp_layout;
  lay::LayoutView *mp_view;
  int m_cv_index;
  db::pcell_parameters_type m_parameters;

  void init ();
};

}

#endif
