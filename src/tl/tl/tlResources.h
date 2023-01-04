
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


#ifndef HDR_tlResources
#define HDR_tlResources

#include "tlAssert.h"
#include "tlStream.h"

#include <cstddef>

namespace tl
{

typedef size_t resource_id_type;

/**
 *  @brief A facility for retrieving resource data similar to Qt resources
 *
 *  This feature is intended to substitute Qt resources when Qt is not available.
 */

/**
 *  @brief Registers the resource data under the given name
 *
 *  @param name The name of the resource
 *  @param compressed True, if the data is gzip-compressed
 *  @param data The data pointer - needs to stay valid during the lifetime of the application
 *  @param data_size The size of the data block
 *
 *  The return value is an Id by which the resource can be unregistered.
 */
TL_PUBLIC resource_id_type register_resource (const char *name, bool compressed, const unsigned char *data, size_t data_size);

/**
 *  @brief Registers the resource data under the given name
 *
 *  @param id The id of the resource to unregister (see "register_resource")
 */
TL_PUBLIC void unregister_resource (size_t id);

/**
 *  @brief Gets the resource data as a stream
 *
 *  @param name The resource name
 *  @return A tl::InputStream object delivering the data or 0 if there is no such resource
 *  It is the responsibility of the called to delete the input stream.
 */
TL_PUBLIC tl::InputStream *get_resource (const char *name);

/**
 *  @brief Gets the resource data as a stream reader delegate plus compressed flag
 *
 *  @param name The resource name
 *  @return A pair of reader delegate and a flag indicating whether the stream is compressed.
 *  If the resource is not found, the reade delegate is 0.
 *  It is the responsibility of the called to delete the reader delegate.
 */
TL_PUBLIC std::pair<tl::InputStreamBase *, bool> get_resource_reader (const char *name);

/**
 *  @brief Get resource names matching a glob pattern
 *
 *  For example, find_resources("/group*") will find resources whose name start with "group".
 *  "*" also matches "/"!
 */
TL_PUBLIC std::vector<std::string> find_resources (const std::string &pattern);

}

#endif

