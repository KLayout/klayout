
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

#include "tlBase64.h"
#include "tlException.h"
#include "tlInternational.h"

namespace tl
{

namespace {

class EncoderTable
{
public:
  EncoderTable ()
  {
    char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (unsigned int i = 0; i < 256; ++i) {
      m_char2bin[i] = 0xff;
    }
    for (unsigned int i = 0; i < 64; ++i) {
      m_bin2char[i] = charset[i];
      m_char2bin[(unsigned int) charset[i]] = i;
    }
  }

  inline char c (unsigned char b) const { return m_bin2char[b]; }
  inline unsigned char b (char c) const { return m_char2bin[(unsigned char) c]; }

private:
  char m_bin2char[64];
  unsigned char m_char2bin[256];
};

}

static EncoderTable s_enc;

std::vector<unsigned char> from_base64 (const char *s)
{
  size_t sz = 0;
  for (const char *t = s; *t; ++t) {
    ++sz;
  }

  unsigned int sh = 0;

  std::vector<unsigned char> data;
  data.reserve ((sz * 6 + 7) / 8);

  for (const char *t = s; *t; ++t) {

    if ((unsigned char) *t <= ' ') {

      //  ignore white space characters

    } else if (*t == '=') {

      //  padding/termination
      if (data.empty () || data.back () != 0) {
        throw tl::Exception (tl::to_string (tr ("Error decoding base64 data: padding character does not match zero byte")));
      }
      data.pop_back ();
      break;

    } else {

      unsigned char b = s_enc.b (*t);
      if (b >= 64) {
        throw tl::Exception (tl::to_string (tr ("Error decoding base64 data: invalid character '%c'")), *t);
      }

      sh += 2;

      if (sh == 8) {
        data.back () |= b;
        sh = 0;
      } else if (sh == 2) {
        data.push_back (b << 2);
      } else {
        data.back () |= (b >> (8 - sh));
        data.push_back (b << sh);
      }

    }

  }

  return data;
}

std::string to_base64 (const unsigned char *data, size_t size)
{
  std::string s;
  s.reserve (((size + 2) / 3) * 4);

  size_t bits = size * 8;

  for (size_t b = 0; b < bits; b += 6) {
    size_t bit = b % 8;
    size_t byte = b / 8;
    if (bit <= 2) {
      s += s_enc.c ((data [byte] >> (2 - bit)) & 0x3f);
    } else if (b + 8 < bits) {
      s += s_enc.c (((data [byte] << (bit - 2)) | (data [byte + 1] >> (10 - bit))) & 0x3f);
    } else {
      s += s_enc.c ((data [byte] << (bit - 2)) & 0x3f);
      s += '=';
      if (bit == 6) {
        s += '=';
      }
    }
  }

  return s;
}

}
