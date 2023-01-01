
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

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s

const char *prg_exe_name        = "klayout";
const char *prg_name            = "KLayout";

#if defined(KLAYOUT_VERSION)
const char *prg_version         = STRINGIFY(KLAYOUT_VERSION);
#else
const char *prg_version         = "x.xx";
#endif

#if defined(KLAYOUT_VERSION_DATE)
const char *prg_date            = STRINGIFY(KLAYOUT_VERSION_DATE);
#else
const char *prg_date            = "xxxx-xx-xx";
#endif

#if defined(KLAYOUT_VERSION_REV)
const char *prg_rev             = STRINGIFY(KLAYOUT_VERSION_REV);
#else
const char *prg_rev             = "xxxxxxxx";
#endif

const char *prg_author =  
  "By Matthias K\303\266fferlein, Munich"
  ;

const char *prg_about_text = 
  "For feedback and bug reports mail to: contact@klayout.de\n"
  "\n"
  "\n"
  "Copyright (C) 2006-2023 Matthias K\303\266fferlein\n"
  "\n"
  "This program is free software; you can redistribute it and/or modify\n"
  "it under the terms of the GNU General Public License as published by\n"
  "the Free Software Foundation; either version 2 of the License, or\n"
  "(at your option) any later version.\n"
  "\n"
  "This program is distributed in the hope that it will be useful,\n"
  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
  "GNU General Public License for more details.\n"
  "\n"
  "You should have received a copy of the GNU General Public License\n"
  "along with this program; if not, write to the Free Software\n"
  "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA\n"
;

