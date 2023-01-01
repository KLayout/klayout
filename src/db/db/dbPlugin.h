
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


#ifndef HDR_dbPlugin
#define HDR_dbPlugin

/**
 *  @brief A struct to hold the data of the plugin
 *
 *  Use it like this:
 *
 *  @code
 *  static DbPlugin plugin_desc = {
 *    "1.0", //  (const char *) version information - should be given at least
 *    0      //  (const char *) description or 0/empty if no description is given
 *  };
 *  DECLARE_DB_PLUGIN (plugin_desc);
 *  @endcode
 */
struct DbPlugin
{
  const char *version;
  const char *description;
};

/**
 *  @brief A typedef for the initialization function a plugin is supposed to expose.
 */
typedef void (*dbp_init_func_t) (const char **version, const char **description);

# if defined _WIN32 || defined __CYGWIN__
#   define DBP_PUBLIC __declspec(dllexport)
# else
#   if __GNUC__ >= 4 || defined(__clang__)
#     define DBP_PUBLIC __attribute__ ((visibility ("default")))
#   else
#     define DBP_PUBLIC
#   endif

# endif

#define DECLARE_DB_PLUGIN(desc) \
  extern "C" { \
    DBP_PUBLIC void dbp_init (const char **version, const char **description) { \
      *version = desc.version; \
      *description = desc.description; \
    } \
  }

#endif
