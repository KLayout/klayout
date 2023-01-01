
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


//  This header by intention does not have a include guard
//  It is used multiple times inside gsiCallback.h
//
//  It needs these macros to be defined (example for the one argument case)
//   _TMPLARGPART  ", class A1"
//   _FUNCARGLIST  "A1"
//   _CALLARGLIST  ", A1 a1"
//   _SETVALUE     "args.template write<A1> (a1);"

template <class X _COMMA _TMPLARGPART>
void issue (void (X::*) (_FUNCARGLIST) _COMMA _CALLARGLIST) const
{
  SerialArgs args (argsize), ret (retsize);
  _SETVALUE
  call_int (args, ret);
}

template <class X _COMMA _TMPLARGPART>
void issue (void (X::*) (_FUNCARGLIST) const _COMMA _CALLARGLIST) const
{
  SerialArgs args (argsize), ret (retsize);
  _SETVALUE
  call_int (args, ret);
}

template <class X, class R _COMMA _TMPLARGPART>
R issue (R (X::*) (_FUNCARGLIST) _COMMA _CALLARGLIST) const
{
  tl::Heap heap;
  SerialArgs args (argsize), ret (retsize);
  _SETVALUE
  call_int (args, ret);
  return ret.template read<R> (heap);
}

template <class X, class R _COMMA _TMPLARGPART>
R issue (R (X::*) (_FUNCARGLIST) const _COMMA _CALLARGLIST) const
{
  tl::Heap heap;
  SerialArgs args (argsize), ret (retsize);
  _SETVALUE
  call_int (args, ret);
  return ret.template read<R> (heap);
}

