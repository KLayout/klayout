
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


#ifndef HDR_dbColdProxy
#define HDR_dbColdProxy

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbCell.h"

#include "tlObject.h"
#include "tlTypeTraits.h"

namespace db
{

struct LayoutOrCellContextInfo;

/**
 *  @brief A cell specialization: a cold proxy representing a library or PCell which has gone out of scope
 *
 *  If a PCell or library cell gets disconnected - for example, because the technology has changed or during
 *  development of PCell code - this proxy replaces the original one. It stores the connection information, so
 *  it can be regenerated when it becomes valid again.
 */
class DB_PUBLIC ColdProxy 
  : public Cell, public tl::Object
{
public:
  /** 
   *  @brief The constructor
   *
   *  Creates a cold proxy represented by the ProxyContextInfo data.
   */
  ColdProxy (db::cell_index_type ci, db::Layout &layout, const LayoutOrCellContextInfo &info);

  /**
   *  @brief The destructor
   */
  ~ColdProxy ();

  /**
   *  @brief Cloning 
   */
  virtual Cell *clone (Layout &layout) const;

  /**
   *  @brief Get the library id 
   */
  const LayoutOrCellContextInfo &context_info () const
  {
    return *mp_context_info;
  }

  /**
   *  @brief Indicates that this cell is a proxy cell
   */
  virtual bool is_proxy () const 
  { 
    return true; 
  }

  /**
   *  @brief Gets a list of cold proxies for a given library name
   */
  static const tl::weak_collection<ColdProxy> &cold_proxies_per_lib_name (const std::string &libname);

  /**
   *  @brief Gets the basic name
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Gets the display name
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Gets the qualified name
   */
  virtual std::string get_qualified_name () const;

private:
  LayoutOrCellContextInfo *mp_context_info;

  ColdProxy (const ColdProxy &d);
  ColdProxy &operator= (const ColdProxy &d);
};

}

#endif


