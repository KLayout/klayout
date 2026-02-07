
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


#include "layConverters.h"
#include "tlInternational.h"
#include "tlString.h"

namespace lay
{

// ----------------------------------------------------------------
//  ColorConverter implementation

#if defined(HAVE_QT)
std::string 
ColorConverter::to_string (const QColor &c) const
{
  if (! c.isValid ()) {
    return "auto";
  } else {
    return tl::to_string (c.name ());
  }
}
#endif

std::string
ColorConverter::to_string (const tl::Color &c) const
{
  if (! c.is_valid ()) {
    return "auto";
  } else {
    return c.to_string ();
  }
}

#if defined(HAVE_QT)
void
ColorConverter::from_string (const std::string &s, QColor &c) const
{
  std::string t (tl::trim (s));
  if (t == "auto") {
    c = QColor ();
  } else {
    c = QColor (t.c_str ());
  } 
}
#endif

void
ColorConverter::from_string (const std::string &s, tl::Color &c) const
{
  std::string t (tl::trim (s));
  if (t == "auto") {
    c = tl::Color ();
  } else {
    c = tl::Color (t);
  }
}

// -----------------------------------------------------------------------------
//  ACConverter implementation

std::string
ACConverter::to_string (const lay::angle_constraint_type &m)
{
  if (m == lay::AC_Any) {
    return "any";
  } else if (m == lay::AC_Diagonal) {
    return "diagonal";
  } else if (m == lay::AC_DiagonalOnly) {
    return "diagonal_only";
  } else if (m == lay::AC_Ortho) {
    return "ortho";
  } else if (m == lay::AC_Horizontal) {
    return "horizontal";
  } else if (m == lay::AC_Vertical) {
    return "vertical";
  } else if (m == lay::AC_Global) {
    return "global";
  } else {
    return "";
  }
}

void
ACConverter::from_string (const std::string &tt, lay::angle_constraint_type &m)
{
  std::string t (tl::trim (tt));
  if (t == "any") {
    m = lay::AC_Any;
  } else if (t == "diagonal") {
    m = lay::AC_Diagonal;
  } else if (t == "diagonal_only") {
    m = lay::AC_DiagonalOnly;
  } else if (t == "ortho") {
    m = lay::AC_Ortho;
  } else if (t == "horizontal") {
    m = lay::AC_Horizontal;
  } else if (t == "vertical") {
    m = lay::AC_Vertical;
  } else if (t == "global") {
    m = lay::AC_Global;
  } else {
    m = lay::AC_Any;
  }
}

// -----------------------------------------------------------------------------
//  HAlignConverter implementation

std::string
HAlignConverter::to_string (db::HAlign a)
{
  if (a == db::HAlignCenter) {
    return "center";
  } else if (a == db::HAlignLeft) {
    return "left";
  } else if (a == db::HAlignRight) {
    return "right";
  } else {
    return "";
  }
}

void
HAlignConverter::from_string (const std::string &tt, db::HAlign &a)
{
  std::string t (tl::trim (tt));
  if (t == "center") {
    a = db::HAlignCenter;
  } else if (t == "left") {
    a = db::HAlignLeft;
  } else if (t == "right") {
    a = db::HAlignRight;
  } else {
    a = db::NoHAlign;
  }
}

// -----------------------------------------------------------------------------
//  VAlignConverter implementation

std::string
VAlignConverter::to_string (db::VAlign a)
{
  if (a == db::VAlignCenter) {
    return "center";
  } else if (a == db::VAlignBottom) {
    return "bottom";
  } else if (a == db::VAlignTop) {
    return "top";
  } else {
    return "";
  }
}

void
VAlignConverter::from_string (const std::string &tt, db::VAlign &a)
{
  std::string t (tl::trim (tt));
  if (t == "center") {
    a = db::VAlignCenter;
  } else if (t == "bottom") {
    a = db::VAlignBottom;
  } else if (t == "top") {
    a = db::VAlignTop;
  } else {
    a = db::NoVAlign;
  }
}

// -----------------------------------------------------------------------------
//  EditGridConverter implementation

std::string
EditGridConverter::to_string (const db::DVector &eg)
{
  if (eg == db::DVector ()) {
    return "global";
  } else if (eg.x () < 1e-6) {
    return "none";
  } else if (fabs (eg.x () - eg.y ()) < 1e-6) {
    return tl::to_string (eg.x ());
  } else {
    return tl::to_string (eg.x ()) + "," + tl::to_string (eg.y ());
  }
}

void
EditGridConverter::from_string (const std::string &s, db::DVector &eg)
{
  tl::Extractor ex (s.c_str ());

  double x = 0, y = 0;
  if (ex.test ("global")) {
    eg = db::DVector ();
  } else if (ex.test ("none")) {
    eg = db::DVector (-1.0, -1.0);
  } else if (ex.try_read (x)) {
    y = x;
    if (ex.test (",")) {
      ex.try_read (y);
    }
    eg = db::DVector (x, y);
  }
}

void
EditGridConverter::from_string_picky (const std::string &s, db::DVector &eg)
{
  tl::Extractor ex (s.c_str ());

  if (ex.test ("global")) {
    eg = db::DVector ();
  } else if (ex.test ("none")) {
    eg = db::DVector (-1.0, -1.0);
  } else {
    double x = 0.0, y = 0.0;
    ex.read (x);
    if (ex.test (",")) {
      ex.read (y);
    } else {
      y = x;
    }
    if (x < 1e-6 || y < 1e-6) {
      throw tl::Exception (tl::to_string (tr ("The grid must be larger than zero")));
    }
    eg = db::DVector (x, y);
  }
  ex.expect_end ();
}

}

