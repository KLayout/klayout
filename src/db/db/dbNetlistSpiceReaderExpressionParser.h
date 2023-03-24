
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

#ifndef HDR_dbNetlistSpiceReaderExpressionParser
#define HDR_dbNetlistSpiceReaderExpressionParser

#include "dbCommon.h"
#include "tlStream.h"
#include "tlVariant.h"
#include "tlString.h"

#include <string>
#include <map>
#include <vector>

namespace db
{

/**
 *  @brief A class implementing the expression parser
 *
 *  This class is exposed mainly for testing purposes.
 */
class DB_PUBLIC NetlistSpiceReaderExpressionParser
{
public:
  typedef std::map<std::string, tl::Variant> variables_type;

  NetlistSpiceReaderExpressionParser (const variables_type *vars, double def_scale = 1.0);
  NetlistSpiceReaderExpressionParser (const variables_type *vars1, const variables_type *vars2, double def_scale = 1.0);

  tl::Variant read (tl::Extractor &ex) const;
  tl::Variant read (const std::string &s) const;
  bool try_read (tl::Extractor &ex, tl::Variant &v) const;
  bool try_read (const std::string &s, tl::Variant &v) const;

private:
  const variables_type *mp_variables1, *mp_variables2;
  double m_def_scale;

  tl::Variant read_atomic_value (tl::Extractor &ex, bool *status) const;
  tl::Variant read_dot_expr (tl::Extractor &ex, bool *status) const;
  tl::Variant read_bar_expr (tl::Extractor &ex, bool *status) const;
  tl::Variant read_pwr_expr (tl::Extractor &ex, bool *status) const;
  tl::Variant read_compare_expr (tl::Extractor &ex, bool *status) const;
  tl::Variant read_logical_op (tl::Extractor &ex, bool *status) const;
  tl::Variant read_ternary_op (tl::Extractor &ex, bool *status) const;
  tl::Variant read_tl_expr (tl::Extractor &ex, bool *status) const;
  tl::Variant eval_func (const std::string &name, const std::vector<tl::Variant> &params, bool *status) const;
};
}

#endif
