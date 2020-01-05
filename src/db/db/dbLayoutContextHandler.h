
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


#ifndef HDR_dbLayoutContextHandler
#define HDR_dbLayoutContextHandler

#include "tlExpression.h"
#include "dbLayout.h"
#include "dbCommon.h"

namespace db
{

/**
 * @brief  A layout providing a context for the expression evaluation
 */
class DB_PUBLIC LayoutContextHandler
  : public tl::ContextHandler
{
public:
  /**
   *  @brief Constructor: provides a context from a layout
   *  This object will provide bindings for <..> (layers) and <<..>> (cells) expressions.
   *  Plus it will provide a database unit value.
   */
  LayoutContextHandler (const db::Layout *layout);

  /**
   *  @brief The constructor with a non-const layout context
   *  In the non-const context, layers or cells will be created if they
   *  are not there yet. To enable non-const context, set the "can_modify"
   *  flag to true.
   */
  LayoutContextHandler (db::Layout *layout, bool can_modify);

  /**
   *  @brief Provide <..> bindings.
   */
  virtual tl::Variant eval_bracket (const std::string &content) const;

  /**
   *  @brief Provide <<..>> bindings.
   */
  virtual tl::Variant eval_double_bracket (const std::string &content) const;

  /**
   *  @brief Provide a database unit value
   */
  virtual double dbu () const
  {
    return mp_layout->dbu ();
  }

private:
  const db::Layout *mp_layout;
  db::Layout *mp_layout_nc;
};

}

#endif

