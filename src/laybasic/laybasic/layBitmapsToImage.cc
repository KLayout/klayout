
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


#include "layBitmapsToImage.h"
#include "layBitmap.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "tlPixelBuffer.h"
#include "tlTimer.h"
#include "tlAssert.h"
#include "tlThreads.h"

namespace lay
{

static void
render_scanline_std (const uint32_t *dp, unsigned int ds, const lay::Bitmap *pbitmap, unsigned int y, unsigned int w, unsigned int /*h*/, uint32_t *data)
{
  const uint32_t *ps = pbitmap->scanline (y);
  const uint32_t *dm = dp;

  unsigned int x = w;
  while (x >= lay::wordlen) {
    *data++ = *ps++ & *dm++;
    if (dm == dp + ds) {
      dm = dp;
    }
    x -= lay::wordlen;
  }

  if (x > 0) {
    *data = *ps & *dm;
  }
}

static void
render_scanline_std_edge (const uint32_t *dp, unsigned int ds, const lay::Bitmap *pbitmap, unsigned int y, unsigned int w, unsigned int h, uint32_t *data)
{
  const uint32_t *psp = (y > 0 ? pbitmap->scanline (y - 1) : pbitmap->empty_scanline ());
  const uint32_t *psn = (y < h - 1 ? pbitmap->scanline (y + 1) : pbitmap->empty_scanline ());
  const uint32_t *ps = pbitmap->scanline (y);

  const uint32_t *dm = dp;

  unsigned int b = (y % (32 * ds));
  bool vflag = (dp[b / 32] & (1 << (b % 32))) != 0;

  uint32_t ddp = 0;

  int x = int (w);
  while (x > 0) {

    uint32_t d = 0;
    uint32_t dsn = 0, dsp = 0;
    uint32_t ddn = 0;

    if (x > int (lay::wordlen)) {
      d = *ps++;
      dsn = *psn++;
      dsp = *psp++;
      ddn = *ps;
    } else {
      d = *ps;
      dsn = *psn;
      dsp = *psp;
      if (x < int (lay::wordlen)) {
        d &= ((1 << x) - 1);
      }
    }

    //  di selects the inner bits - such that have a left, right neighbor
    uint32_t dhn1 = (d & ((d >> 1) | ((ddn & 1) << 31)));
    uint32_t dhn2 = (d & ((d << 1) | ((ddp >> 31) & 1)));
    uint32_t dhi = dhn1 & dhn2;
    uint32_t dhn = dhn1 | dhn2;

    //  dvi selects the vertically inner bits - such that have a top and botton neighbor
    uint32_t dvn1 = dsn & d;
    uint32_t dvn2 = dsp & d;
    uint32_t dvi = dvn1 & dvn2;
    uint32_t dvn = dvn1 | dvn2;

#if 1
    /*
      NOTE: this solution is ugly for lines with angles a little away from 45 degree
      like 30..40 and 50..60 degree.
      This is the truth table of the various combinations of bits that are
      encountered. Each combination gets horizontal or vertical bit masks.
      This is basically an edge detection algorithm.
      The diagonal pixels are not considered currently.

      configuration   use mask   dhi   dvi   dhn   dvn
      --------------------------------------------------
       .              H          0     0     0     0
      .x.             [sol] (could be 1 too but covers diagonal edges too,
       .                     so we make it H masked)

       .    .    .    H          x     0     1     x
      xx.  .xx  xxx   [with_hm]
       .    .    .

       x    x    x
      xx.  .xx  xxx
       .    .    .

       .    .    .
      xx.  .xx  xxx
       x    x    x

       x    .    x    V          0     x     x     1
      .x.  .x.  .x.   [with_vm] (the four corner cases will be H and V masks
       .    x    x               and both results get merged)

       x    .    x
      xx.  xx.  xx.
       .    x    x

       x    .    x
      .xx  .xx  .xx
       .    x    x

       x              H*V        1     1     1     1
      xxx             [with_hvm]
       x

     */

    uint32_t sol      = d - (d & (dhi | dvi | dhn | dvn));
    uint32_t with_hm  = (d - (d & dvi)) & dhn;
    uint32_t with_vm  = (d - (d & dhi)) & dvn;
    uint32_t with_hvm = d & dhi & dvi & dhn & dvn;

    uint32_t hm = *dm++;
    uint32_t dd = (sol | with_hm) & hm;
    if (vflag) {
      dd |= with_vm | (with_hvm & hm);
    }

#else
    /*
      NOTE: this alternative solution is ugly for very steep angles
      such as 60 to 85 degree.

      It is based on the following configuration table:

      configuration   use mask   dhi   dvi   dhn   dvn
      --------------------------------------------------
       .              H          0     0     0     0
      .x.             [sol]
       .

       .    .         H          0     0     1     0
      xx.  .xx        [twin]
       .    .

       .    x         H          0     0     0     1
      .x.  .x.        [twin]
       x    .

       .    x         H|V        0     0     1     1
      .xx  .xx        [corner]
       x    .

       .    x
      xx.  xx.
       x    .

       x    x    x    V          0     1     x     (1)
      .x.  .xx  xx.   [vbar]
       x    x    x

       .    .    x    H          1     0     (1)   x
      xxx  xxx  xxx   [hbar]
       .    x    .

       x              H*V        1     1     (1)   (1)
      xxx
       x

     */

    uint32_t sol    = d - (d & (dhi | dvi | dhn | dvn));
    uint32_t twin   = (d - (d & (dhi | dvi))) & (dhn ^ dvn);
    uint32_t corner = (d - (d & (dhi | dvi))) & dhn & dvn;
    uint32_t hbar   = (d - (d & dvi)) & dhi;
    uint32_t vbar   = (d - (d & dhi)) & dvi;
    uint32_t inner  = d & dhi & dvi;

    uint32_t hm = *dm++;
    uint32_t dd = 0;
    //  H (and H|V)
    dd = (sol | twin | hbar | corner /*H|V*/) & hm;
    if (vflag) {
      //  V (and H|V)
      dd |= vbar | corner /*H|V*/;
      //  H*V
      dd |= inner & hm;
    }

#endif

    *data++ = dd;

    if (dm == dp + ds) {
      dm = dp;
    }

    x -= lay::wordlen;
    ddp = d;

  }
}

static void
render_scanline_px (const uint32_t *dp, unsigned int ds, const lay::Bitmap *pbitmap,
                    unsigned int y, unsigned int w, unsigned int h, uint32_t *data,
                    unsigned int pixels)
{
  if (pixels < 1) {
    return;
  }

  if (pixels > 15) {
    pixels = 15;
  }

  const uint32_t *dm = dp;

  unsigned int px1 = (pixels - 1) / 2;
  unsigned int px2 = (pixels - 1) - px1;

  const uint32_t *ps[16];
  for (unsigned int p = 0; p < pixels; ++p) {
    if (y + p < px1) {
      ps[p] = pbitmap->scanline (0);
    } else if ((y + p - px1) >= h) {
      ps[p] = pbitmap->scanline (h - 1);
    } else {
      ps[p] = pbitmap->scanline (y + p - px1);
    }
  }

  uint32_t d, dd = 0, dn = 0;
  for (unsigned int p = 0; p < pixels; ++p) {
    dn |= *(ps[p]++);
  }

  unsigned int x = w;
  while (true) {

    d = dn;

    dn = 0;
    if (x > lay::wordlen) {
      for (unsigned int p = 0; p < pixels; ++p) {
        dn |= *(ps[p]++);
      }
    }

    uint32_t d0 = d;
    for (unsigned int p = 1; p <= px1; ++p) {
      d |= (d0 >> p) | (dn << (32 - p));
    }
    for (unsigned int p = 1; p <= px2; ++p) {
      d |= (d0 << p) | (dd >> (32 - p));
    }

    dd = d0;

    *data++ = d & *dm++;
    if (dm == dp + ds) {
      dm = dp;
    }

    if (x > lay::wordlen) {
      x -= lay::wordlen;
    } else {
      break;
    }

  }

}

static void
render_scanline_cross (const uint32_t *dp, unsigned int ds, const lay::Bitmap *pbitmap,
                       unsigned int y, unsigned int w, unsigned int h, uint32_t *data,
                       unsigned int pixels)
{
  if (pixels < 1) {
    return;
  }

  //  NOTE: hardcoded bar/width ratio for crosses.
  unsigned int lw = std::max (std::min ((unsigned int) 6, pixels / 9), (unsigned int) 1);

  const int max_pixels = 31;
  if (pixels > max_pixels) {
    pixels = max_pixels;
  }

  const uint32_t *dm = dp;
  unsigned int px1 = (pixels - 1) / 2;
  unsigned int px2 = (pixels - 1) - px1;

  unsigned int spx1 = (lw - 1) / 2;
  unsigned int spx2 = (lw - 1) - spx1;

  const uint32_t *ps[max_pixels + 1];
  for (unsigned int p = 0; p < pixels; ++p) {
    if (y + p < px1) {
      ps[p] = pbitmap->scanline (0);
    } else if ((y + p - px1) >= h) {
      ps[p] = pbitmap->scanline (h - 1);
    } else {
      ps[p] = pbitmap->scanline (y + p - px1);
    }
  }

  uint32_t *dpp = data;
  for (unsigned int i = (w + lay::wordlen - 1) / lay::wordlen; i > 0; --i) {
    *dpp++ = 0;
  }

  for (unsigned int o = 0; o < pixels; ++o) {

    dpp = data;

    unsigned int bpx1 = 0, bpx2 = 0;
    if (o >= px1 - spx1 && o <= px1 + spx2) {
      bpx1 = px1;
      bpx2 = px2;
    } else {
      bpx1 = spx1;
      bpx2 = spx2;
    }

    if (bpx1 > 0 || bpx2 > 0) {

      uint32_t d, dd = 0, dn;
      dn = *(ps[o]++);

      unsigned int x = w;
      while (true) {

        d = dn;

        dn = 0;
        if (x > lay::wordlen) {
          dn = *(ps[o]++);
        }

        uint32_t d0 = d;
        if (d0 != 0) {
          for (unsigned int p = 1; p <= bpx1; ++p) {
            d |= (d0 >> p);
          }
          for (unsigned int p = 1; p <= bpx2; ++p) {
            d |= (d0 << p);
          }
        }
        if (dn != 0) {
          for (unsigned int p = 1; p <= bpx1; ++p) {
            d |= (dn << (32 - p));
          }
        }
        if (dd != 0) {
          for (unsigned int p = 1; p <= bpx2; ++p) {
            d |= (dd >> (32 - p));
          }
        }

        dd = d0;

        *dpp++ |= d & *dm++;
        if (dm == dp + ds) {
          dm = dp;
        }

        if (x > lay::wordlen) {
          x -= lay::wordlen;
        } else {
          break;
        }

      }

    } else {

      unsigned int x = w;
      while (true) {

        uint32_t d = *(ps[o]++);

        *dpp++ |= d & *dm++;
        if (dm == dp + ds) {
          dm = dp;
        }

        if (x > lay::wordlen) {
          x -= lay::wordlen;
        } else {
          break;
        }

      }

    }

  }
}

static void create_precursor_bitmaps (const std::vector<lay::ViewOp> &view_ops_in, const std::vector <unsigned int> &vo_map, const std::vector<lay::Bitmap *> &pbitmaps_in, const std::vector<unsigned int> &bm_map, const lay::LineStyles &ls, unsigned int width, unsigned int height, std::map<unsigned int, lay::Bitmap> &precursors, tl::Mutex *mutex)
{
  tl_assert (bm_map.size () == vo_map.size ());

  //  Styled lines with width > 1 are not rendered directly, but through an intermediate step.
  //  We prepare the necessary precursor bitmaps now
  for (unsigned int i = 0; i < vo_map.size (); ++i) {

    unsigned int vo_index = vo_map [i];
    unsigned int bm_index = bm_map [i];

    const ViewOp &op = view_ops_in [vo_index];
    if (op.width () > 1 && ls.style (op.line_style_index ()).width () > 0) {

      //  lock bitmaps against change by the redraw thread
      if (mutex) {
        mutex->lock ();
      }

      lay::Bitmap &bp = precursors.insert (std::make_pair (bm_index, lay::Bitmap (width, height, 1.0))).first->second;
      const LineStyleInfo &ls_info = ls.style (op.line_style_index ()).scaled (op.width ());

      for (unsigned int y = 0; y < height; y++) {
        render_scanline_std_edge (ls_info.pattern (), ls_info.pattern_stride (), pbitmaps_in [bm_index], y, width, height, bp.scanline (y));
      }

      if (mutex) {
        mutex->unlock ();
      }

    }

  }
}

void
bitmaps_to_image (const std::vector<lay::ViewOp> &view_ops_in,
                  const std::vector<lay::Bitmap *> &pbitmaps_in,
                  const lay::DitherPattern &dp,
                  const lay::LineStyles &ls,
                  double dpr,
                  tl::PixelBuffer *pimage, unsigned int width, unsigned int height,
                  bool use_bitmap_index,
                  tl::Mutex *mutex)
{
  bool transparent = pimage->transparent ();

  std::vector<unsigned int> bm_map;
  std::vector<unsigned int> vo_map;

  vo_map.reserve (view_ops_in.size ());
  bm_map.reserve (view_ops_in.size ());
  unsigned int n_in = 0;

  //  drop invisible and empty bitmaps, build bitmap mask
  for (unsigned int i = 0; i < view_ops_in.size (); ++i) {

    const lay::ViewOp &vop = view_ops_in [i];

    unsigned int bi = (use_bitmap_index && vop.bitmap_index () >= 0) ? (unsigned int) vop.bitmap_index () : i;
    const lay::Bitmap *pb = bi < pbitmaps_in.size () ? pbitmaps_in [bi] : 0;

    if ((vop.ormask () | ~vop.andmask ()) != 0 && pb && ! pb->empty ()) {
      vo_map.push_back (i);
      bm_map.push_back (bi);
      ++n_in;
    }

  }

  //  Styled lines with width > 1 are not rendered directly, but through an intermediate step.
  //  We prepare the necessary precursor bitmaps now
  std::map<unsigned int, lay::Bitmap> precursors;
  create_precursor_bitmaps (view_ops_in, vo_map, pbitmaps_in, bm_map, ls, width, height, precursors, mutex);

  std::vector<lay::ViewOp> view_ops;
  std::vector<const lay::Bitmap *> pbitmaps;
  std::vector<std::pair <tl::color_t, tl::color_t> > masks;
  std::vector<uint32_t> non_empty_sls;

  view_ops.reserve (n_in);
  pbitmaps.reserve (n_in);
  masks.reserve (n_in);
  non_empty_sls.reserve (n_in);

  //  to optimize the bitmap generation, the bitmaps are checked
  //  for emptyness in slices of "slice" scanlines
  unsigned int slice = 32;

  //  allocate a pixel buffer large enough to hold a scanline for all 
  //  planes.
  unsigned int nwords = (width + 31) / 32;
  uint32_t *buffer = new uint32_t [n_in * nwords];

  for (unsigned int y = 0; y < height; y++) {

    //  lock bitmaps against change by the redraw thread
    if (mutex) {
      mutex->lock ();
    }

    //  every "slice" scan lines test what bitmaps are empty 
    if (y % slice == 0) { 

      view_ops.erase (view_ops.begin (), view_ops.end ());
      pbitmaps.erase (pbitmaps.begin (), pbitmaps.end ());
      non_empty_sls.erase (non_empty_sls.begin (), non_empty_sls.end ());
      for (unsigned int i = 0; i < n_in; ++i) {

        const lay::ViewOp &vop = view_ops_in [vo_map[i]];
        unsigned int w = vop.width ();

        const lay::Bitmap *pb = 0;
        unsigned int bm_index = bm_map[i];
        if (bm_map [i] < pbitmaps_in.size ()) {
          if (w > 1 && ls.style (vop.line_style_index ()).width () > 0) {
            tl_assert (precursors.find (bm_index) != precursors.end ());
            pb = &precursors [bm_index];
          } else {
            pb = pbitmaps_in [bm_index];
          }
        }

        if (pb != 0 
            && w > 0
            && ((pb->first_scanline () < y + slice && pb->last_scanline () > y) || w > 1)
            && (vop.ormask () | ~vop.andmask ()) != 0) {

          uint32_t non_empty_sl = 0;
          uint32_t m = 1;

          for (unsigned int yy = 0; yy < slice && yy + y < height; ++yy, m <<= 1) {
            if (! pb->is_scanline_empty (yy + y)) {
              non_empty_sl |= m;
            }
          }

          if (non_empty_sl || w > 1) {
            view_ops.push_back (vop);
            pbitmaps.push_back (pb);
            non_empty_sls.push_back (non_empty_sl);
          }

        }

      }

    } 

    //  Collect all necessary information to transfer a single scanline ..
    
    masks.erase (masks.begin (), masks.end ());

    const uint32_t needed_bits = 0x00ffffff; // alpha channel not needed
    const uint32_t fill_bits   = 0xff000000; // fill alpha value with ones
    uint32_t *dptr = buffer;
    uint32_t ne_mask = (1 << (y % slice));
    for (unsigned int i = 0; i < view_ops.size (); ++i) {

      const ViewOp &op = view_ops [i];
      if (op.width () > 1 || (op.width () == 1 && (non_empty_sls [i] & ne_mask) != 0)) {

        const LineStyleInfo &ls_info = ls.style (op.line_style_index ()).scaled (op.width ());
        const DitherPatternInfo &dp_info = dp.pattern (op.dither_index ()).scaled (dpr);
        const uint32_t *dither = dp_info.pattern () [(y + op.dither_offset ()) % dp_info.height ()];
        if (dither != 0) {

          unsigned int dither_stride = dp_info.pattern_stride ();

          masks.push_back (std::make_pair (op.ormask () & needed_bits,
                                           ~op.ormask () & op.andmask () & needed_bits));

          if (op.width () == 1) {
            if (ls_info.width () > 0) {
              render_scanline_std_edge (ls_info.pattern (), ls_info.pattern_stride (), pbitmaps [i], y, width, height, dptr);
            } else {
              render_scanline_std (dither, dither_stride, pbitmaps [i], y, width, height, dptr);
            }
          } else if (op.width () > 1) {
            if (op.shape () == lay::ViewOp::Rect) {
              render_scanline_px (dither, dither_stride, pbitmaps [i], y, width, height, dptr, (unsigned int) op.width ());
            } else if (op.shape () == lay::ViewOp::Cross) {
              render_scanline_cross (dither, dither_stride, pbitmaps [i], y, width, height, dptr, (unsigned int) op.width ());
            }
          }

          dptr += nwords;

        }

      }

    }

    //  unlock bitmaps against change by the redraw thread
    if (mutex) {
      mutex->unlock ();
    }

    //  .. and do the actual transfer.

    if (masks.size () > 0) {

      tl::color_t *pt = (tl::color_t *) pimage->scan_line (height - 1 - y);
      uint32_t *dptr_end = dptr; 

      unsigned int i = 0;
      for (unsigned int x = 0; x < width; x += 32, ++i) {

        tl::color_t y[32];
        if (transparent) {
          for (int i = 0; i < 32; ++i) {
            y[i] = 0;
          }
        } else {
          for (int i = 0; i < 32; ++i) {
            y[i] = fill_bits;
          }
        };

        tl::color_t z[32] = { 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
          lay::wordones, lay::wordones, lay::wordones, lay::wordones, 
        };

        dptr = dptr_end - nwords + i;
        for (int j = int (masks.size () - 1); j >= 0; --j) {

          uint32_t d = *dptr;
          if (d != 0) {

            if (transparent) {
              uint32_t m = 1;
              for (unsigned int k = 0; k < 32 && x + k < width; ++k, m <<= 1) {
                if ((d & m) != 0) {
                  y [k] |= (masks [j].first & z [k]) | fill_bits;
                  z [k] &= masks [j].second;
                }
              }
            } else {
              uint32_t m = 1;
              for (unsigned int k = 0; k < 32 && x + k < width; ++k, m <<= 1) {
                if ((d & m) != 0) {
                  y [k] |= masks [j].first & z [k];
                  z [k] &= masks [j].second;
                }
              }
            }

          }

          dptr -= nwords;

        }

        for (unsigned int k = 0; k < 32 && x + k < width; ++k) {
          *pt = (*pt & z[k]) | y[k];
          ++pt;
        }

      }

    }

  }

  //  free the pixel buffer
  delete [] buffer;
}

void
bitmaps_to_image (const std::vector<lay::ViewOp> &view_ops_in,
                  const std::vector<lay::Bitmap *> &pbitmaps_in,
                  const lay::DitherPattern &dp,
                  const lay::LineStyles &ls,
                  double dpr,
                  tl::BitmapBuffer *pimage, unsigned int width, unsigned int height,
                  bool use_bitmap_index,
                  tl::Mutex *mutex)
{
  std::vector<unsigned int> bm_map;
  std::vector<unsigned int> vo_map;

  vo_map.reserve (view_ops_in.size ());
  bm_map.reserve (view_ops_in.size ());
  unsigned int n_in = 0;

  //  drop invisible and empty bitmaps, build bitmap mask
  for (unsigned int i = 0; i < view_ops_in.size (); ++i) {

    const lay::ViewOp &vop = view_ops_in [i];

    unsigned int bi = (use_bitmap_index && vop.bitmap_index () >= 0) ? (unsigned int) vop.bitmap_index () : i;
    const lay::Bitmap *pb = bi < pbitmaps_in.size () ? pbitmaps_in [bi] : 0;

    if ((vop.ormask () | ~vop.andmask ()) != 0 && pb && ! pb->empty ()) {
      vo_map.push_back (i);
      bm_map.push_back (bi);
      ++n_in;
    }

  }

  //  Styled lines with width > 1 are not rendered directly, but through an intermediate step.
  //  We prepare the necessary precursor bitmaps now
  std::map<unsigned int, lay::Bitmap> precursors;
  create_precursor_bitmaps (view_ops_in, vo_map, pbitmaps_in, bm_map, ls, width, height, precursors, mutex);

  std::vector<lay::ViewOp> view_ops;
  std::vector<const lay::Bitmap *> pbitmaps;
  std::vector<std::pair <tl::color_t, tl::color_t> > masks;
  std::vector<uint32_t> non_empty_sls;

  view_ops.reserve (n_in);
  pbitmaps.reserve (n_in);
  masks.reserve (n_in);
  non_empty_sls.reserve (n_in);

  //  to optimize the bitmap generation, the bitmaps are checked
  //  for emptyness in slices of "slice" scanlines
  unsigned int slice = 32;

  //  allocate a pixel buffer large enough to hold a scanline for all 
  //  planes.
  unsigned int nwords = (width + 31) / 32;
  uint32_t *buffer = new uint32_t [n_in * nwords];

  for (unsigned int y = 0; y < height; y++) {

    //  lock bitmaps against change by the redraw thread
    if (mutex) {
      mutex->lock ();
    }

    //  every "slice" scan lines test what bitmaps are empty 
    if (y % slice == 0) { 

      view_ops.erase (view_ops.begin (), view_ops.end ());
      pbitmaps.erase (pbitmaps.begin (), pbitmaps.end ());
      non_empty_sls.erase (non_empty_sls.begin (), non_empty_sls.end ());
      for (unsigned int i = 0; i < n_in; ++i) {

        const lay::ViewOp &vop = view_ops_in [vo_map[i]];
        unsigned int w = vop.width ();

        const lay::Bitmap *pb = 0;
        unsigned int bm_index = bm_map[i];
        if (bm_map [i] < pbitmaps_in.size ()) {
          if (w > 1 && ls.style (vop.line_style_index ()).width () > 0) {
            tl_assert (precursors.find (bm_index) != precursors.end ());
            pb = &precursors [bm_index];
          } else {
            pb = pbitmaps_in [bm_index];
          }
        }

        if (pb != 0
            && w > 0
            && ((pb->first_scanline () < y + slice && pb->last_scanline () > y) || w > 1)
            && (vop.ormask () | ~vop.andmask ()) != 0) {

          uint32_t non_empty_sl = 0;
          uint32_t m = 1;

          for (unsigned int yy = 0; yy < slice && yy + y < height; ++yy, m <<= 1) {
            if (! pb->is_scanline_empty (yy + y)) {
              non_empty_sl |= m;
            }
          }

          if (non_empty_sl || w > 1) {
            view_ops.push_back (vop);
            pbitmaps.push_back (pb);
            non_empty_sls.push_back (non_empty_sl);
          }

        }

      }

    } 

    //  Collect all necessary information to transfer a single scanline ..
    
    masks.erase (masks.begin (), masks.end ());

    uint32_t needed_bits = 0x008000; // only green bit 7 required
    uint32_t *dptr = buffer;
    uint32_t ne_mask = (1 << (y % slice));
    for (unsigned int i = 0; i < view_ops.size (); ++i) {

      const ViewOp &op = view_ops [i];
      if (op.width () > 1 || (op.width () == 1 && (non_empty_sls [i] & ne_mask) != 0)) {

        const LineStyleInfo &ls_info = ls.style (op.line_style_index ()).scaled (op.width ());
        const DitherPatternInfo &dp_info = dp.pattern (op.dither_index ()).scaled (dpr);
        const uint32_t *dither = dp_info.pattern () [(y + op.dither_offset ()) % dp_info.height ()];
        if (dither != 0) {

          unsigned int dither_stride = dp_info.pattern_stride ();

          masks.push_back (std::make_pair (op.ormask () & needed_bits,
                                           ~op.ormask () & op.andmask () & needed_bits));

          if (op.width () == 1) {
            if (ls_info.width () > 0) {
              render_scanline_std_edge (ls_info.pattern (), ls_info.pattern_stride (), pbitmaps [i], y, width, height, dptr);
            } else {
              render_scanline_std (dither, dither_stride, pbitmaps [i], y, width, height, dptr);
            }
          } else if (op.width () > 1) {
            if (op.shape () == lay::ViewOp::Rect) {
              render_scanline_px (dither, dither_stride, pbitmaps [i], y, width, height, dptr, (unsigned int) op.width ());
            } else if (op.shape () == lay::ViewOp::Cross) {
              render_scanline_cross (dither, dither_stride, pbitmaps [i], y, width, height, dptr, (unsigned int) op.width ());
            }
          }

          dptr += nwords;

        }

      }

    }

    //  unlock bitmaps against change by the redraw thread
    if (mutex) {
      mutex->unlock ();
    }

    //  .. and do the actual transfer.

    if (masks.size () > 0) {

      tl::color_t *pt = (tl::color_t *) pimage->scan_line (height - 1 - y);
      uint32_t *dptr_end = dptr; 

      unsigned int i = 0;
      for (unsigned int x = 0; x < width; x += 32, ++i) {

        uint32_t y = 0;
        uint32_t z = lay::wordones;

        dptr = dptr_end - nwords + i;
        for (int j = int (masks.size () - 1); j >= 0; --j) {
          uint32_t d = *dptr;
          if (d != 0) {
            uint32_t m = 1;
            for (unsigned int k = 0; k < 32 && x + k < width; ++k, m <<= 1) {
              if ((d & m) != 0) { 
                if (masks [j].first & needed_bits) {
                  y |= (z & m);
                }
                if (! (masks [j].second & needed_bits)) {
                  z &= ~m;
                }
              }
            }
          }
          dptr -= nwords;
        }

        *pt = (*pt & z) | y;
        ++pt;

      }

    }

  }

  //  free the pixel buffer
  delete [] buffer;
}

void
bitmap_to_bitmap (const lay::ViewOp &view_op, const lay::Bitmap &bitmap,
                  unsigned char *data,
                  unsigned int width, unsigned int height,
                  const lay::DitherPattern &dp,
                  const lay::LineStyles &ls,
                  double dpr)
{
  //  quick exit, if line width is zero
  if (view_op.width () == 0) {
    return;
  }

  unsigned int nwords = (width + 31) / 32;
  uint32_t *buffer = new uint32_t [nwords];

  //  determine endianess ..
  unsigned int x = 0xc0000001;
  unsigned char x0 = ((unsigned char *) &x) [0];

  const DitherPatternInfo &dp_info = dp.pattern (view_op.dither_index ()).scaled (dpr);
  const LineStyleInfo &ls_info = ls.style (view_op.line_style_index ()).scaled (view_op.width ());

  for (unsigned int y = 0; y < height; ++y) {

    unsigned int nbytes = ((width + 7) / 8);

    if (view_op.width () > 1 || ! bitmap.is_scanline_empty (height - 1 - y)) {

      const uint32_t *dither = dp_info.pattern () [(height - 1 - y + view_op.dither_offset ()) % dp_info.height ()];
      unsigned int dither_stride = dp_info.pattern_stride ();

      if (view_op.width () == 1) {

        if (ls_info.width () > 0) {
          render_scanline_std_edge (ls_info.pattern (), ls_info.pattern_stride (), &bitmap, height - 1 - y, width, height, buffer);
        } else {
          render_scanline_std (dither, dither_stride, &bitmap, height - 1 - y, width, height, buffer);
        }

      } else if (view_op.width () > 1) {

        const lay::Bitmap *bp = &bitmap;

        //  Styled lines with width > 1 are not rendered directly, but through an intermediate step.
        //  We prepare the necessary precursor bitmaps now
        lay::Bitmap precursor;
        if (ls_info.width () > 0) {

          precursor = lay::Bitmap (width, height, 1.0);

          LineStyleInfo lsi = ls_info;

          for (unsigned int y = 0; y < height; y++) {
            render_scanline_std_edge (lsi.pattern (), lsi.pattern_stride (), bp, y, width, height, precursor.scanline (y));
          }

          bp = &precursor;

        }

        if (view_op.shape () == lay::ViewOp::Rect) {
          render_scanline_px (dither, dither_stride, bp, height - 1 - y, width, height, buffer, (unsigned int) view_op.width ());
        } else if (view_op.shape () == lay::ViewOp::Cross) {
          render_scanline_cross (dither, dither_stride, bp, height - 1 - y, width, height, buffer, (unsigned int) view_op.width ());
        }
      }

      const uint32_t *p = buffer;
      uint32_t d = 0;
      const char *dp = (const char *)&d;

      if (x0 == 0xc0) {

        //  MSB first ..
        while (nbytes >= 4) {
          d = *p++;
          if (d) {
            *data++ |= dp[3];
            *data++ |= dp[2];
            *data++ |= dp[1];
            *data++ |= dp[0];
          } else {
            data += 4;
          }
          nbytes -= 4;
        }
        if (nbytes > 0) {
          d = *p++;
          if (d) {
            dp += 4;
            while (nbytes > 0) {
              *data++ |= *--dp;
              --nbytes;
            }
          } else {
            data += nbytes;
          }
        }

      } else if (x0 == 0x01) {

        //  LSB first ..
        while (nbytes >= 4) {
          d = *p++;
          if (d) {
            *data++ |= dp[0];
            *data++ |= dp[1];
            *data++ |= dp[2];
            *data++ |= dp[3];
          } else {
            data += 4;
          }
          nbytes -= 4;
        }
        if (nbytes > 0) {
          d = *p++;
          if (d) {
            while (nbytes > 0) {
              *data++ |= *dp++;
              --nbytes;
            }
          } else {
            data += nbytes;
          }
        }

      } else {
        //  unable to determine endianess
        tl_assert (false);
      }

    } else {
      data += nbytes;
    }
    
  }

  delete [] buffer;
}

}

