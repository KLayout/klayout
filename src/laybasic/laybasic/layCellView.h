
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


#ifndef HDR_layCellView
#define HDR_layCellView

#include "laybasicCommon.h"
#include "layLayoutHandle.h"

#include <string>
#include <vector>

#include "tlObject.h"
#include "dbLayout.h"
#include "dbMetaInfo.h"
#include "dbReader.h"
#include "dbInstElement.h"
#include "dbTechnology.h"
#include "gsi.h"

namespace lay 
{

/**
 *  @brief A "cell view" reference
 *
 *  A cell view reference points to a certain cell within a certain layout.
 *  The layout pointer can be 0, indicating that it is invalid.
 *  Also, the cell view describes a cell within that layout. The cell
 *  is addressed by an cell_index or a cell pointer. 
 *  The cell is not only identified by it's index or pointer but as well 
 *  by the path leading to that cell. This path describes how to find the
 *  cell in the context of parent cells. 
 *  The path is in fact composed in twofold: once in an unspecific fashion,
 *  just describing which parent cells are used. The target of this path
 *  is called the context cell. It is accessible by the ctx_cell_index
 *  or ctx_cell method.
 *  Additionally the path may further identify a certain instance of a certain
 *  subcell in the context cell. This is done through a set of db::InstElement
 *  objects. The target of this context path is the actual cell addressed by the
 *  cellview.
 *  In the viewer, the target cell is shown in the context of the context cell.
 *  The hierarchy levels are counted from the context cell, which is on level 0.
 *  If the context path is empty, the context cell is identical with the target cell.
 */

class LAYBASIC_PUBLIC CellView
  : public tl::Object
{
public:
  typedef db::cell_index_type cell_index_type;
  typedef std::vector <cell_index_type> unspecific_cell_path_type;
  typedef std::vector <db::InstElement> specific_cell_path_type;

  /**
   *  @brief Constructor: create an invalid cellview
   */
  CellView ();

  /**
   *  @brief Equality: compares the cell the cv points to, not the path
   */
  bool operator== (const CellView &cv) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellView &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Test if the cv points to a valid cell
   */
  bool is_valid () const;

  /**
   *  @brief Return the layout handle
   */
  lay::LayoutHandle *operator-> () const
  {
    return m_layout_href.get ();
  }

  /**
   *  @brief Return the layout handle (not via operator->)
   */
  lay::LayoutHandle *handle () const
  {
    return m_layout_href.get ();
  }

  /**
   *  @brief Set the unspecific part of the path explicitly
   *
   *  Setting the unspecific part will clear the context part and
   *  update the context and target cell.
   */
  void set_unspecific_path (const unspecific_cell_path_type &p);

  /**
   *  @brief Set the context part of the path explicitly
   *
   *  This method assumes that the unspecific part of the path 
   *  is established already and that the context part starts
   *  from the context cell.
   */
  void set_specific_path (const specific_cell_path_type &p);

  /**
   *  @brief Set the path to the given cell
   *
   *  This method will construct any path to this cell, not a 
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (cell_index_type index);

  /**
   *  @brief Set the cell by name
   *
   *  If the name is not a valid one, the cellview will become
   *  invalid.
   *  This method will construct any path to this cell, not a 
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (const std::string &name);

  /**
   *  @brief Reset the cell 
   *
   *  The cellview will become invalid. The layout object will
   *  still be attached to the cellview.
   */
  void reset_cell ();

  /**
   *  @brief Set the layout handle
   *
   *  Connect the cellview with a certain layout.
   *  This will reset the target and context cell.
   */
  void set (lay::LayoutHandle *handle);

  /**
   *  @brief Get the context cell pointer
   */
  db::Cell *ctx_cell () const
  {
    return mp_ctx_cell;
  }

  /**
   *  @brief Get the context cell index
   */
  cell_index_type ctx_cell_index () const
  {
    return m_ctx_cell_index;
  }

  /**
   *  @brief Get the target cell pointer
   */
  db::Cell *cell () const
  {
    return mp_cell;
  }

  /**
   *  @brief Get the target cell index
   */
  cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Get the cell's combined path in an unspecific form
   */
  unspecific_cell_path_type combined_unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const unspecific_cell_path_type &unspecific_path () const
  {
    return m_unspecific_path;
  }

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const specific_cell_path_type &specific_path () const
  {
    return m_specific_path;
  }

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path
   */
  db::ICplxTrans context_trans () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path as a micron-unit transformation
   */
  db::DCplxTrans context_dtrans () const;

  /**
   *  @brief Deep copy of the cellview
   *
   *  This method performs a deep copy on the cellview.
   *  A layout must be set already. Rather the creating another reference to the layout
   *  (which is done on operator= for example), this method copies the content of the 
   *  source layout to the current one and transfers cell path and other parameters.
   *
   *  @param manager The database object manager that the new layout is put under
   */
  CellView deep_copy (db::Manager *manager) const;

private:
  lay::LayoutHandleRef m_layout_href;
  db::Cell *mp_ctx_cell;
  cell_index_type m_ctx_cell_index;
  db::Cell *mp_cell;
  cell_index_type m_cell_index;
  unspecific_cell_path_type m_unspecific_path;
  specific_cell_path_type m_specific_path;
};

/**
 *  @brief A cellview reference
 *
 *  This object acts like a proxy to a lay::CellView object. It is connected to a cellview
 *  and a LayoutView and upon changes, the LayoutView will be configured accordingly.
 */
class LAYBASIC_PUBLIC CellViewRef
  : public gsi::ObjectBase
{
public:
  typedef CellView::unspecific_cell_path_type unspecific_cell_path_type;
  typedef CellView::specific_cell_path_type specific_cell_path_type;
  typedef CellView::cell_index_type cell_index_type;

  /**
   *  @brief Default constructor
   *  This constructor creates an invalid cellview reference
   */
  CellViewRef ();

  /**
   *  @brief Constructor
   *  @param cv The reference to the target cellview
   *  @param view The reference to the layout view
   */
  CellViewRef (lay::CellView *cv, lay::LayoutViewBase *view);

  /**
   *  @brief Gets the cellview index of this reference
   *  If the cellview is not valid, -1 will be returned.
   */
  int index () const;

  /**
   *  @brief Gets the LayoutViewBase the reference is pointing to
   */
  lay::LayoutViewBase *view ();

  /**
   *  @brief Equality: Gives true, if the cellviews are identical
   */
  bool operator== (const CellView &cv) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellView &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Equality: Gives true, if the references point to the same cellview
   */
  bool operator== (const CellViewRef &cv) const
  {
    return mp_cv.get () == cv.mp_cv.get ();
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const CellViewRef &cv) const
  {
    return !operator== (cv);
  }

  /**
   *  @brief Test if the cv points to a valid cell and is valid otherwise
   */
  bool is_valid () const;

  /**
   *  @brief Returns the layout handle
   */
  lay::LayoutHandle *operator-> () const;

  /**
   *  @brief Return the layout handle (not via operator->)
   */
  lay::LayoutHandle *handle () const
  {
    return operator-> ();
  }

  /**
   *  @brief Sets the name of the cellview
   *
   *  This equivalent to calling Layout#rename_cellview.
   *  The name is made unique, hence the final name may
   *  differ from the one given.
   */
  void set_name (const std::string &name);

  /**
   *  @brief Set the unspecific part of the path explicitly
   *
   *  Setting the unspecific part will clear the context part and
   *  update the context and target cell.
   */
  void set_unspecific_path (const unspecific_cell_path_type &p);

  /**
   *  @brief Set the context part of the path explicitly
   *
   *  This method assumes that the unspecific part of the path
   *  is established already and that the context part starts
   *  from the context cell.
   */
  void set_specific_path (const specific_cell_path_type &p);

  /**
   *  @brief Set the path to the given cell
   *
   *  This method will construct any path to this cell, not a
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (cell_index_type index);

  /**
   *  @brief Set the cell by name
   *
   *  If the name is not a valid one, the cellview will become
   *  invalid.
   *  This method will construct any path to this cell, not a
   *  particular one. It will clear the context part of the path
   *  and update the context and target cell.
   */
  void set_cell (const std::string &name);

  /**
   *  @brief Resets the cell
   */
  void reset_cell ();

  /**
   *  @brief Get the context cell pointer
   *  An invalid cellview reference will return a null pointer.
   */
  db::Cell *ctx_cell () const;

  /**
   *  @brief Get the context cell index
   */
  cell_index_type ctx_cell_index () const
  {
    db::Cell *c = ctx_cell ();
    return c ? c->cell_index () : 0;
  }

  /**
   *  @brief Get the target cell pointer
   *  An invalid cellview reference will return a null pointer.
   */
  db::Cell *cell () const;

  /**
   *  @brief Get the target cell index
   */
  cell_index_type cell_index () const
  {
    db::Cell *c = cell ();
    return c ? c->cell_index () : 0;
  }

  /**
   *  @brief Get the cell's combined path in an unspecific form
   */
  unspecific_cell_path_type combined_unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const unspecific_cell_path_type &unspecific_path () const;

  /**
   *  @brief Get the cell's unspecific part of the path
   */
  const specific_cell_path_type &specific_path () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path
   */
  db::ICplxTrans context_trans () const;

  /**
   *  @brief Retrive the accumulated transformation induced by the context part of the path in micron units
   */
  db::DCplxTrans context_dtrans() const;

private:
  tl::weak_ptr<lay::CellView> mp_cv;
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
};

}

#endif

