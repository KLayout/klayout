
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

#include "dbNetlistSpiceReaderExpressionParser.h"
#include "dbNetlistSpiceReader.h"

#include <cmath>

namespace db
{

// ------------------------------------------------------------------------------------------------------

static bool to_bool (const tl::Variant &v)
{
  if (v.is_bool ()) {
    return v.to_bool ();
  } else if (v.is_nil ()) {
    return false;
  } else if (v.can_convert_to_double ()) {
    return v.to_double () != 0.0;
  } else {
    return true;
  }
}
// ------------------------------------------------------------------------------------------------------

NetlistSpiceReaderExpressionParser::NetlistSpiceReaderExpressionParser (const variables_type *vars, double def_scale)
  : m_def_scale (def_scale)
{
  mp_variables1 = vars;
  mp_variables2 = 0;
}

NetlistSpiceReaderExpressionParser::NetlistSpiceReaderExpressionParser (const variables_type *vars1, const variables_type *vars2, double def_scale)
  : m_def_scale (def_scale)
{
  mp_variables1 = vars1;
  mp_variables2 = vars2;
}

//  expression syntax taken from ngspice:
//  https://nmg.gitlab.io/ngspice-manual/circuitdescription/paramparametricnetlists/syntaxofexpressions.html

static double sqrt_f (double v)   { return sqrt (v); }
static double sin_f (double v)    { return sin (v); }
static double cos_f (double v)    { return cos (v); }
static double tan_f (double v)    { return tan (v); }
static double sinh_f (double v)   { return sinh (v); }
static double cosh_f (double v)   { return cosh (v); }
static double tanh_f (double v)   { return tanh (v); }
static double asin_f (double v)   { return asin (v); }
static double acos_f (double v)   { return acos (v); }
static double atan_f (double v)   { return atan (v); }
static double asinh_f (double v)  { return asinh (v); }
static double acosh_f (double v)  { return acosh (v); }
static double atanh_f (double v)  { return atanh (v); }
static double exp_f (double v)    { return exp (v); }
static double ln_f (double v)     { return log (v); }
static double log_f (double v)    { return log10 (v); }
static double abs_f (double v)    { return abs (v); }
static double nint_f (double v)   { return nearbyint (v); } //  we basically should we the rounding mode before ...
static double floor_f (double v)  { return floor (v); }
static double ceil_f (double v)   { return ceil (v); }
static double sgn_f (double v)    { return v == 0.0 ? 0.0 : (v < 0.0 ? -1.0 : 1.0); }
static double int_f (double v)    { return sgn_f (v) * floor (sgn_f (v) * v); }

tl::Variant
NetlistSpiceReaderExpressionParser::eval_func (const std::string &name, const std::vector<tl::Variant> &params, bool * /*status*/) const
{
  double (*f) (double) = 0;

  if (name == "SQRT") { f = sqrt_f; } else
  if (name == "SIN") { f = sin_f; } else
  if (name == "COS") { f = cos_f; } else
  if (name == "TAN") { f = tan_f; } else
  if (name == "SINH") { f = sinh_f; } else
  if (name == "COSH") { f = cosh_f; } else
  if (name == "TANH") { f = tanh_f; } else
  if (name == "ASIN") { f = asin_f; } else
  if (name == "ACOS") { f = acos_f; } else
  if (name == "ATAN" || name == "arctan") { f = atan_f; } else
  if (name == "ASINH") { f = asinh_f; } else
  if (name == "ACOSH") { f = acosh_f; } else
  if (name == "ATANH") { f = atanh_f; } else
  if (name == "EXP") { f = exp_f; } else
  if (name == "LN") { f = ln_f; } else
  if (name == "LOG") { f = log_f; } else
  if (name == "ABS") { f = abs_f; } else
  if (name == "NINT") { f = nint_f; } else
  if (name == "FLOOR") { f = floor_f; } else
  if (name == "CEIL") { f = ceil_f; } else
  if (name == "SGN") { f = sgn_f; } else
  if (name == "INT") { f = int_f; }

  if (f != 0) {

    if (params.size () < 1 || ! params.front ().can_convert_to_double ()) {
      return tl::Variant ();
    } else {
      return tl::Variant ((*f) (params.front ().to_double ()));
    }

  } else if (name == "PWR" || name == "POW") {

    if (params.size () < 2 || ! params [0].can_convert_to_double () || ! params [1].can_convert_to_double ()) {
      return tl::Variant ();
    } else {
      return tl::Variant (pow (params [0].to_double (), params [1].to_double ()));
    }

  } else if (name == "TERNERY_FCN") {

    if (params.size () < 3) {
      return tl::Variant ();
    } else {
      return to_bool (params [0]) ? params [1] : params [2];
    }

  } else if (name == "MIN") {

    if (params.size () < 1) {
      return tl::Variant ();
    }

    tl::Variant v = params [0];
    for (size_t i = 1; i < params.size (); ++i) {
      if (params [i] < v) {
         v = params [i];
      }
    }
    return v;

  } else if (name == "MAX") {

    if (params.size () < 1) {
      return tl::Variant ();
    }

    tl::Variant v = params [0];
    for (size_t i = 1; i < params.size (); ++i) {
      if (v < params [i]) {
         v = params [i];
      }
    }
    return v;

  } else {

    return tl::Variant ();

  }
}

tl::Variant
NetlistSpiceReaderExpressionParser::read_atomic_value (tl::Extractor &ex, bool *status) const
{
  double vd = 0.0;
  std::string var;

  if (ex.test ("-")) {

    tl::Variant v = read_atomic_value (ex, status);
    if (v.can_convert_to_double ()) {
      return tl::Variant (-v.to_double ());
    } else {
      return tl::Variant ();
    }

  } else if (ex.test ("!")) {

    tl::Variant v = read_atomic_value (ex, status);
    return tl::Variant (! to_bool (v));

  } else if (ex.test ("(")) {

    tl::Variant v = read_tl_expr (ex, status);
    if (status && !*status) {
      return tl::Variant ();
    }
    if (status) {
      *status = ex.test (")");
    } else {
      ex.expect (")");
    }
    return v;

  } else if (ex.try_read (vd)) {

    if (status) {
      *status = true;
    }

    double f = m_def_scale;
    if (*ex == 't' || *ex == 'T') {
      f = 1e12;
    } else if (*ex == 'g' || *ex == 'G') {
      f = 1e9;
    } else if (*ex == 'k' || *ex == 'K') {
      f = 1e3;
    } else if (*ex == 'm' || *ex == 'M') {
      f = 1e-3;
      if (ex.test_without_case ("meg")) {
        f = 1e6;
      }
    } else if (*ex == 'u' || *ex == 'U') {
      f = 1e-6;
    } else if (*ex == 'n' || *ex == 'N') {
      f = 1e-9;
    } else if (*ex == 'p' || *ex == 'P') {
      f = 1e-12;
    } else if (*ex == 'f' || *ex == 'F') {
      f = 1e-15;
    } else if (*ex == 'a' || *ex == 'A') {
      f = 1e-18;
    }
    while (*ex && isalpha (*ex)) {
      ++ex;
    }

    vd *= f;
    return tl::Variant (vd);

  } else if (ex.try_read_word (var)) {

    var = tl::to_upper_case (var);

    if (ex.test ("(")) {

      //  a function

      std::vector<tl::Variant> params;
      if (! ex.test (")")) {
        while (! ex.at_end ()) {
          params.push_back (read_tl_expr (ex, status));
          if (status && !*status) {
            return tl::Variant ();
          }
          if (! ex.test (",")) {
            break;
          }
        }
        if (status && ! ex.test (")")) {
          *status = false;
          return tl::Variant ();
        } else {
          ex.expect (")");
        }
      }

      return eval_func (var, params, status);

    } else {

      if (mp_variables1) {
        auto vi = mp_variables1->find (var);
        if (vi != mp_variables1->end ()) {
          return vi->second;
        }
      }
      if (mp_variables2) {
        auto vi = mp_variables2->find (var);
        if (vi != mp_variables2->end ()) {
          return vi->second;
        }
      }
      //  keep word as string value
      return tl::Variant (var);

    }

  } else {

    if (status) {
      *status = false;
    } else {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Expected number of variable name here: '...%s'")), ex.get ()));
    }

    return tl::Variant ();

  }
}

tl::Variant NetlistSpiceReaderExpressionParser::read_pwr_expr (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_atomic_value (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  while (true) {
    if (ex.test ("**") || ex.test ("^")) {
      tl::Variant vv = read_atomic_value (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = tl::Variant (pow (v.to_double (), vv.to_double ()));
      }
    } else {
      break;
    }
  }
  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_dot_expr (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_pwr_expr (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  while (true) {
    if (ex.test ("*")) {
      tl::Variant vv = read_pwr_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = v.to_double () * vv.to_double ();
      }
    } else if (ex.test ("/")) {
      tl::Variant vv = read_pwr_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = v.to_double () / vv.to_double ();
      }
    } else if (ex.test ("%")) {
      tl::Variant vv = read_pwr_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = tl::Variant ((long int) v.to_double () % (long int) vv.to_double ());
      }
    } else {
      break;
    }
  }
  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_bar_expr (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_dot_expr (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  while (true) {
    if (ex.test ("+")) {
      tl::Variant vv = read_dot_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = v.to_double () + vv.to_double ();
      }
    } else if (ex.test ("-")) {
      tl::Variant vv = read_dot_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      if (! v.can_convert_to_double () || ! vv.can_convert_to_double ()) {
        v = tl::Variant ();
      } else {
        v = v.to_double () - vv.to_double ();
      }
    } else {
      break;
    }
  }
  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_compare_expr (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_bar_expr (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  while (true) {
    if (ex.test ("==")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (v == vv);
    } else if (ex.test ("!=")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (!(v == vv));
    } else if (ex.test ("<=")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (v < vv || v == vv);
    } else if (ex.test ("<")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (v < vv);
    } else if (ex.test (">=")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (vv < v || v == vv);
    } else if (ex.test (">")) {
      tl::Variant vv = read_bar_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (vv < v);
    } else {
      break;
    }
  }
  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_logical_op (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_compare_expr (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  while (true) {
    if (ex.test ("&&")) {
      tl::Variant vv = read_compare_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (to_bool (v) && to_bool (vv));
    } else if (ex.test ("||")) {
      tl::Variant vv = read_compare_expr (ex, status);
      if (status && !*status) {
        return tl::Variant ();
      }
      v = tl::Variant (to_bool (v) || to_bool (vv));
    } else {
      break;
    }
  }
  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_ternary_op (tl::Extractor &ex, bool *status) const
{
  tl::Variant v = read_logical_op (ex, status);
  if (status && !*status) {
    return tl::Variant ();
  }
  if (ex.test ("?")) {
    tl::Variant vv1 = read_logical_op (ex, status);
    if (status && !*status) {
      return tl::Variant ();
    }
    if (! ex.test (":")) {
      if (status) {
        *status = false;
      } else {
        ex.expect (":");
      }
    }
    tl::Variant vv2 = read_logical_op (ex, status);
    if (status && !*status) {
      return tl::Variant ();
    }
    v = to_bool (v) ? vv1 : vv2;
  }

  return v;
}

tl::Variant NetlistSpiceReaderExpressionParser::read_tl_expr (tl::Extractor &ex, bool *status) const
{
  return read_ternary_op (ex, status);
}

static const char *start_quote (tl::Extractor &ex)
{
  if (ex.test ("'")) {
    return "'";
  } else if (ex.test ("\"")) {
    return "\"";
  } else if (ex.test ("{")) {
    return "}";
  } else {
    return 0;
  }
}

tl::Variant NetlistSpiceReaderExpressionParser::read (const std::string &s) const
{
  tl::Extractor ex (s.c_str ());
  return read (ex);
}

tl::Variant NetlistSpiceReaderExpressionParser::read (tl::Extractor &ex) const
{
  tl::Variant res;

  const char *endquote = start_quote (ex);
  res = read_tl_expr (ex, 0);
  if (endquote) {
    ex.test (endquote);
  }

  return res;
}

bool NetlistSpiceReaderExpressionParser::try_read (const std::string &s, tl::Variant &value) const
{
  tl::Extractor ex (s.c_str ());
  return try_read (ex, value);
}

bool NetlistSpiceReaderExpressionParser::try_read (tl::Extractor &ex, tl::Variant &value) const
{
  tl::Extractor ex_saved = ex;

  bool status = false;
  const char *endquote = start_quote (ex);
  value = read_tl_expr (ex, &status);
  if (endquote && ! ex.test (endquote)) {
    status = false;
  }
  if (! status) {
    value = tl::Variant ();
    ex = ex_saved;
  }

  return status;
}

}
