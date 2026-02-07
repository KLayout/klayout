
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "bdConverterMain.h"
#include "bdWriterOptions.h"
#include "dbStream.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "tlUnitTest.h"

//  Testing the converter main implementation (CIF)
TEST(1)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::cif_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "CIF");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (DXF)
TEST(2)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::dxf_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "DXF");
  }

  //  Fix top cell name (which is TOP in DXF, not RINGO as in reference)
  std::pair<bool, db::cell_index_type> top = layout.cell_by_name ("TOP");
  EXPECT_EQ (top.first, true);
  layout.rename_cell (top.second, "RINGO");

  //  Use GDS2 normalization to solve the box vs. polygon issue
  db::compare_layouts (this, layout, input, db::WriteGDS2);
}

//  Testing the converter main implementation (GDS2)
TEST(3)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::gds2_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (GDS2Text)
TEST(4)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::gds2text_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2Text");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (OASIS)
TEST(5)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "OASIS");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (MAG)
TEST(6)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string input_au = tl::testdata ();
  input_au += "/magic/strm2mag_au.gds";

  std::string output = this->tmp_file ("RINGO.mag");

  const char *argv[] = { "x", input.c_str (), output.c_str (), "--magic-lambda-out=0.005" };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::mag_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_option_by_name ("mag_lambda", 0.005);
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "MAG");
  }

  db::compare_layouts (this, layout, input_au, db::WriteGDS2);
}

//  Testing the converter main implementation (LStream)
TEST(7)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::lstream_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "LStream");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Large LEF/DEF to OAS converter test
TEST(10)
{
  test_is_long_runner ();

  std::string input_dir = tl::testdata_private ();
  input_dir += "/lefdef/strm2oas/";

  std::string lef_dir = input_dir + "/lef";
  std::string def_dir = input_dir + "/def";
  std::string gds_dir = input_dir + "/gds";

  std::string input_au = input_dir + "/strm2oas_au_2.oas";

  std::string output = this->tmp_file ("strm2oas.oas");
  std::string map_arg = "--lefdef-map=" + input_dir + "/sky130.map";

  const char *lef_files[] = {
    "sky130_fd_sc_hd.tlef",
    "sky130_fd_sc_hd_merged.lef",
    "sky130_fd_sc_hs_merged.lef",
    "sky130_ef_sc_hd__decap_20_12.lef",
    "sky130_ef_sc_hd__decap_80_12.lef",
    "sky130_ef_sc_hd__fill_4.lef",
    "sky130_ef_sc_hd__decap_40_12.lef",
    "sky130_ef_sc_hd__decap_60_12.lef",
    "sky130_ef_io__analog_esd_pad.lef",
    "sky130_ef_io__analog_noesd_pad.lef",
    "sky130_ef_io__analog_pad.lef",
    "sky130_ef_io__bare_pad.lef",
    "sky130_ef_io__com_bus_slice_10um.lef",
    "sky130_ef_io__com_bus_slice_1um.lef",
    "sky130_ef_io__com_bus_slice_20um.lef",
    "sky130_ef_io__com_bus_slice_5um.lef",
    "sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um.lef",
    "sky130_ef_io__corner_pad.lef",
    "sky130_ef_io__disconnect_vccd_slice_5um.lef",
    "sky130_ef_io__disconnect_vdda_slice_5um.lef",
    "sky130_ef_io__gpiov2_pad.lef",
    "sky130_ef_io__gpiov2_pad_wrapped.lef",
    "sky130_ef_io__top_power_hvc.lef",
    "sky130_ef_io__vccd_hvc_pad.lef",
    "sky130_ef_io__vccd_lvc_clamped2_pad.lef",
    "sky130_ef_io__vccd_lvc_clamped3_pad.lef",
    "sky130_ef_io__vccd_lvc_clamped_pad.lef",
    "sky130_ef_io__vccd_lvc_pad.lef",
    "sky130_ef_io__vdda_hvc_clamped_pad.lef",
    "sky130_ef_io__vdda_hvc_pad.lef",
    "sky130_ef_io__vdda_lvc_pad.lef",
    "sky130_ef_io__vddio_hvc_clamped_pad.lef",
    "sky130_ef_io__vddio_hvc_pad.lef",
    "sky130_ef_io__vddio_lvc_pad.lef",
    "sky130_ef_io__vssa_hvc_clamped_pad.lef",
    "sky130_ef_io__vssa_hvc_pad.lef",
    "sky130_ef_io__vssa_lvc_pad.lef",
    "sky130_ef_io__vssd_hvc_pad.lef",
    "sky130_ef_io__vssd_lvc_clamped2_pad.lef",
    "sky130_ef_io__vssd_lvc_clamped3_pad.lef",
    "sky130_ef_io__vssd_lvc_clamped_pad.lef",
    "sky130_ef_io__vssd_lvc_pad.lef",
    "sky130_ef_io__vssio_hvc_clamped_pad.lef",
    "sky130_ef_io__vssio_hvc_pad.lef",
    "sky130_ef_io__vssio_lvc_pad.lef",
    "sky130_fd_io__signal_5_sym_hv_local_5term.lef",
    "sky130_fd_io__top_gpiov2.lef",
    "sky130_fd_io__top_power_hvc_wpadv2.lef",
    "sky130_fd_sc_hvl__a21o_1.lef",
    "sky130_fd_sc_hvl__a21oi_1.lef",
    "sky130_fd_sc_hvl__a22o_1.lef",
    "sky130_fd_sc_hvl__a22oi_1.lef",
    "sky130_fd_sc_hvl__and2_1.lef",
    "sky130_fd_sc_hvl__and3_1.lef",
    "sky130_fd_sc_hvl__buf_1.lef",
    "sky130_fd_sc_hvl__buf_16.lef",
    "sky130_fd_sc_hvl__buf_2.lef",
    "sky130_fd_sc_hvl__buf_32.lef",
    "sky130_fd_sc_hvl__buf_4.lef",
    "sky130_fd_sc_hvl__buf_8.lef",
    "sky130_fd_sc_hvl__conb_1.lef",
    "sky130_fd_sc_hvl__decap_4.lef",
    "sky130_fd_sc_hvl__decap_8.lef",
    "sky130_fd_sc_hvl__dfrbp_1.lef",
    "sky130_fd_sc_hvl__dfrtp_1.lef",
    "sky130_fd_sc_hvl__dfsbp_1.lef",
    "sky130_fd_sc_hvl__dfstp_1.lef",
    "sky130_fd_sc_hvl__dfxbp_1.lef",
    "sky130_fd_sc_hvl__dfxtp_1.lef",
    "sky130_fd_sc_hvl__diode_2.lef",
    "sky130_fd_sc_hvl__dlclkp_1.lef",
    "sky130_fd_sc_hvl__dlrtp_1.lef",
    "sky130_fd_sc_hvl__dlxtp_1.lef",
    "sky130_fd_sc_hvl__einvn_1.lef",
    "sky130_fd_sc_hvl__einvp_1.lef",
    "sky130_fd_sc_hvl__fill_1.lef",
    "sky130_fd_sc_hvl__fill_2.lef",
    "sky130_fd_sc_hvl__fill_4.lef",
    "sky130_fd_sc_hvl__fill_8.lef",
    "sky130_fd_sc_hvl__inv_1.lef",
    "sky130_fd_sc_hvl__inv_16.lef",
    "sky130_fd_sc_hvl__inv_2.lef",
    "sky130_fd_sc_hvl__inv_4.lef",
    "sky130_fd_sc_hvl__inv_8.lef",
    "sky130_fd_sc_hvl__lsbufhv2hv_hl_1.lef",
    "sky130_fd_sc_hvl__lsbufhv2hv_lh_1.lef",
    "sky130_fd_sc_hvl__lsbufhv2lv_1.lef",
    "sky130_fd_sc_hvl__lsbufhv2lv_simple_1.lef",
    "sky130_fd_sc_hvl__lsbuflv2hv_1.lef",
    "sky130_fd_sc_hvl__lsbuflv2hv_clkiso_hlkg_3.lef",
    "sky130_fd_sc_hvl__lsbuflv2hv_isosrchvaon_1.lef",
    "sky130_fd_sc_hvl__lsbuflv2hv_symmetric_1.lef",
    "sky130_fd_sc_hvl__mux2_1.lef",
    "sky130_fd_sc_hvl__mux4_1.lef",
    "sky130_fd_sc_hvl__nand2_1.lef",
    "sky130_fd_sc_hvl__nand3_1.lef",
    "sky130_fd_sc_hvl__nor2_1.lef",
    "sky130_fd_sc_hvl__nor3_1.lef",
    "sky130_fd_sc_hvl__o21a_1.lef",
    "sky130_fd_sc_hvl__o21ai_1.lef",
    "sky130_fd_sc_hvl__o22a_1.lef",
    "sky130_fd_sc_hvl__o22ai_1.lef",
    "sky130_fd_sc_hvl__or2_1.lef",
    "sky130_fd_sc_hvl__or3_1.lef",
    "sky130_fd_sc_hvl__probe_p_8.lef",
    "sky130_fd_sc_hvl__probec_p_8.lef",
    "sky130_fd_sc_hvl__schmittbuf_1.lef",
    "sky130_fd_sc_hvl__sdfrbp_1.lef",
    "sky130_fd_sc_hvl__sdfrtp_1.lef",
    "sky130_fd_sc_hvl__sdfsbp_1.lef",
    "sky130_fd_sc_hvl__sdfstp_1.lef",
    "sky130_fd_sc_hvl__sdfxbp_1.lef",
    "sky130_fd_sc_hvl__sdfxtp_1.lef",
    "sky130_fd_sc_hvl__sdlclkp_1.lef",
    "sky130_fd_sc_hvl__sdlxtp_1.lef",
    "sky130_fd_sc_hvl__xnor2_1.lef",
    "sky130_fd_sc_hvl__xor2_1.lef",
    "caravel.lef",
    "caravel_clocking.lef",
    "caravel_core.lef",
    "gpio_defaults_block.lef",
    "gpio_logic_high.lef",
    "housekeeping.lef",
    "mgmt_protect_hv.lef",
    "mprj2_logic_high.lef",
    "mprj_io_buffer.lef",
    "mprj_logic_high.lef",
    "spare_logic_block.lef",
    "user_project_wrapper.lef",
    "xres_buf.lef",
    "caravel_logo-stub.lef",
    "caravel_motto-stub.lef",
    "chip_io.lef",
    "copyright_block-stub.lef",
    "empty_macro.lef",
    "manual_power_connections.lef",
    "open_source-stub.lef",
    "simple_por.lef",
    "user_id_programming.lef",
    "user_id_textblock-stub.lef",
    "RAM128.lef"
  };

  std::string lefs_arg = "--lefdef-lefs=";
  for (size_t i = 0; i < sizeof (lef_files) / sizeof (lef_files[0]); ++i) {
    if (i > 0) {
      lefs_arg += ",";
    }
    lefs_arg += lef_dir + "/" + lef_files[i];
  }

  const char *lefdef_layout_files[] = {
    "sky130_fd_sc_hd.gds",
    "sky130_fd_sc_hvl__sdlxtp_1.gds",
    "sky130_fd_sc_hvl__decap_8.gds",
    "sky130_fd_sc_hvl__decap_4.gds",
    "sky130_fd_sc_hvl__nand3_1.gds",
    "sky130_fd_sc_hvl__sdfxbp_1.gds",
    "sky130_fd_sc_hvl__lsbufhv2hv_hl_1.gds",
    "sky130_fd_sc_hvl__sdfrbp_1.gds",
    "sky130_fd_sc_hvl__a21o_1.gds",
    "sky130_fd_sc_hvl__inv_2.gds",
    "sky130_fd_sc_hvl__inv_16.gds",
    "sky130_fd_sc_hvl__inv_1.gds",
    "sky130_fd_sc_hvl__inv_4.gds",
    "sky130_fd_sc_hvl__inv_8.gds",
    "sky130_fd_sc_hvl__nand2_1.gds",
    "sky130_fd_sc_hvl__dfstp_1.gds",
    "sky130_fd_sc_hvl__a22o_1.gds",
    "sky130_fd_sc_hvl__schmittbuf_1.gds",
    "sky130_fd_sc_hvl__a22oi_1.gds",
    "sky130_fd_sc_hvl__lsbuflv2hv_1.gds",
    "sky130_fd_sc_hvl__fill_4.gds",
    "sky130_fd_sc_hvl__fill_1.gds",
    "sky130_fd_sc_hvl__fill_2.gds",
    "sky130_fd_sc_hvl__fill_8.gds",
    "sky130_fd_sc_hvl__sdfrtp_1.gds",
    "sky130_fd_sc_hvl__sdfxtp_1.gds",
    "sky130_fd_sc_hvl__o22a_1.gds",
    "sky130_fd_sc_hvl__dfsbp_1.gds",
    "sky130_fd_sc_hvl__o21a_1.gds",
    "sky130_fd_sc_hvl__a21oi_1.gds",
    "sky130_fd_sc_hvl__buf_1.gds",
    "sky130_fd_sc_hvl__buf_2.gds",
    "sky130_fd_sc_hvl__buf_4.gds",
    "sky130_fd_sc_hvl__buf_32.gds",
    "sky130_fd_sc_hvl__buf_16.gds",
    "sky130_fd_sc_hvl__buf_8.gds",
    "sky130_fd_sc_hvl__einvp_1.gds",
    "sky130_fd_sc_hvl__conb_1.gds",
    "sky130_fd_sc_hvl__and3_1.gds",
    "sky130_fd_sc_hvl__lsbufhv2lv_1.gds",
    "sky130_fd_sc_hvl__and2_1.gds",
    "sky130_fd_sc_hvl__nor3_1.gds",
    "sky130_fd_sc_hvl__dlclkp_1.gds",
    "sky130_fd_sc_hvl__lsbuflv2hv_symmetric_1.gds",
    "sky130_fd_sc_hvl__sdfstp_1.gds",
    "sky130_fd_sc_hvl__dfrbp_1.gds",
    "sky130_fd_sc_hvl__dfxbp_1.gds",
    "sky130_fd_sc_hvl__nor2_1.gds",
    "sky130_fd_sc_hvl__diode_2.gds",
    "sky130_fd_sc_hvl__dlrtp_1.gds",
    "sky130_fd_sc_hvl__dlxtp_1.gds",
    "sky130_fd_sc_hvl__lsbufhv2lv_simple_1.gds",
    "sky130_fd_sc_hvl__lsbuflv2hv_clkiso_hlkg_3.gds",
    "sky130_fd_sc_hvl__sdlclkp_1.gds",
    "sky130_fd_sc_hvl__o22ai_1.gds",
    "sky130_fd_sc_hvl__or3_1.gds",
    "sky130_fd_sc_hvl__sdfsbp_1.gds",
    "sky130_fd_sc_hvl__xor2_1.gds",
    "sky130_fd_sc_hvl__mux4_1.gds",
    "sky130_fd_sc_hvl__or2_1.gds",
    "sky130_fd_sc_hvl__probe_p_8.gds",
    "sky130_fd_sc_hvl__dfxtp_1.gds",
    "sky130_fd_sc_hvl__mux2_1.gds",
    "sky130_fd_sc_hvl__dfrtp_1.gds",
    "sky130_fd_sc_hvl__lsbuflv2hv_isosrchvaon_1.gds",
    "sky130_fd_sc_hvl__probec_p_8.gds",
    "sky130_fd_sc_hvl__xnor2_1.gds",
    "sky130_fd_sc_hvl__einvn_1.gds",
    "sky130_fd_sc_hvl__o21ai_1.gds",
    "sky130_fd_sc_hvl__lsbufhv2hv_lh_1.gds",
    "sky130_ef_io__analog.gds",
    "sky130_ef_io__bare_pad.gds",
    "sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um.gds",
    "sky130_ef_io__disconnect_vccd_slice_5um.gds",
    "sky130_ef_io__disconnect_vdda_slice_5um.gds",
    "sky130_ef_io__gpiov2_pad_wrapped.gds",
    "sky130_ef_sc_hd__decap_12.gds",
    "sky130_ef_sc_hd__decap_20_12.gds",
    "sky130_ef_sc_hd__decap_40_12.gds",
    "sky130_ef_sc_hd__decap_60_12.gds",
    "sky130_ef_sc_hd__decap_80_12.gds",
    "sky130_ef_sc_hd__fill_12.gds",
    "sky130_ef_sc_hd__fill_2.gds",
    "sky130_ef_sc_hd__fill_4.gds",
    "sky130_ef_sc_hd__fill_8.gds",
    "sky130_ef_sc_hvl__fill_8.gds",
    "caravel_logo.gds.gz",
    "caravel_motto.gds.gz",
    "chip_io.gds.gz",
    "copyright_block.gds.gz",
    "empty_macro.gds.gz",
    "manual_power_connections.gds.gz",
    "open_source.gds.gz",
    "simple_por.gds.gz",
    "user_id_programming.gds.gz",
    "user_id_textblock.gds.gz",
    "RAM128.gds.gz"
  };

  std::string lefdef_layouts_arg = "--lefdef-lef-layouts=";
  for (size_t i = 0; i < sizeof (lefdef_layout_files) / sizeof (lefdef_layout_files[0]); ++i) {
    if (i > 0) {
      lefdef_layouts_arg += ",";
    }
    lefdef_layouts_arg += gds_dir + "/" + lefdef_layout_files[i];
  }

  const char *def_files[] = {
    "caravel.def",
    "caravel_clocking.def",
    "caravel_core.def.gz",
    "gpio_defaults_block.def",
    "gpio_logic_high.def",
    "housekeeping.def",
    "mgmt_protect_hv.def",
    "mprj2_logic_high.def",
    "mprj_io_buffer.def",
    "mprj_logic_high.def",
    "spare_logic_block.def",
    "user_project_wrapper.def",
    "xres_buf.def"
  };

  std::string input;
  for (size_t i = 0; i < sizeof (def_files) / sizeof (def_files[0]); ++i) {
    if (i > 0) {
      input += "+";
    }
    input += def_dir + "/" + def_files[i];
  }

  const char *argv[] = { "x",
                         "--lefdef-no-implicit-lef",
                         map_arg.c_str (),
                         lefs_arg.c_str (),
                         lefdef_layouts_arg.c_str (),
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with +
TEST(11_1)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_1.oas";
  std::string input = input_dir + "/strm2oas_1.oas+" + input_dir + "/strm2oas_2.oas";

  std::string output = this->tmp_file ("strm2oas_1.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with + not allowed on different DBUs
TEST(11_2)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_1.oas";
  std::string input = input_dir + "/strm2oas_1.oas+" + input_dir + "/strm2oas_2_10nm.oas";

  std::string output = this->tmp_file ("strm2oas_1.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  try {
    bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name);
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Former and present database units are not compatible: 0.001 (former) vs. 0.01 (present)");
  }
}

//  Merging with + not allowed on different DBUs
TEST(11_3)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_3.oas";
  std::string input = input_dir + "/strm2oas_1.oas," + input_dir + "/strm2oas_2_10nm.oas";

  std::string output = this->tmp_file ("strm2oas_3.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with + and , under the presence of ghost cells: test+test,top->(test)
TEST(12_1)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_12_1.oas";
  std::string input = input_dir + "/strm2oas_a.oas+" + input_dir + "/strm2oas_b.oas," + input_dir + "/strm2oas_c.oas";

  std::string output = this->tmp_file ("strm2oas_12_1.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with + and , under the presence of ghost cells: top->(test),test+test
TEST(12_2)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_12_2.oas";
  std::string input = input_dir + "/strm2oas_c.oas," + input_dir + "/strm2oas_a.oas+" + input_dir + "/strm2oas_b.oas";

  std::string output = this->tmp_file ("strm2oas_12_2.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with + and , under the presence of ghost cells: test+test,toptop->top->(test)
TEST(12_3)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_12_3.oas";
  std::string input = input_dir + "/strm2oas_a.oas+" + input_dir + "/strm2oas_b.oas," + input_dir + "/strm2oas_cc.oas";

  std::string output = this->tmp_file ("strm2oas_12_3.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}

//  Merging with + and , under the presence of ghost cells: toptop->top->(test),test+test
TEST(12_4)
{
  std::string input_dir = tl::testdata ();
  input_dir += "/bd";

  std::string input_au = input_dir + "/strm2oas_au_12_4.oas";
  std::string input = input_dir + "/strm2oas_cc.oas," + input_dir + "/strm2oas_a.oas+" + input_dir + "/strm2oas_b.oas";

  std::string output = this->tmp_file ("strm2oas_12_4.oas");
  const char *argv[] = { "x",
                         "--blend-mode=0",
                         input.c_str (),
                         output.c_str ()
                       };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;
  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (this, layout, input_au, db::WriteOAS);
}
