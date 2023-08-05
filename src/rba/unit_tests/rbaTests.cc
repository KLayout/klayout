
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


#ifdef HAVE_RUBY

#include "rba.h"
#include "gsiDecl.h"

// On Windows, ruby.h is not compatible with windows.h which is included by utHead - at least not if 
// windows.h is included before ruby.h ...
#include "tlUnitTest.h"

TEST (basic)
{
  EXPECT_EQ (gsi::has_class ("Value"), true);
  EXPECT_EQ (gsi::has_class ("DoesNotExist"), false);
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  EXPECT_EQ (gsi::has_class ("QDialog"), true);
  EXPECT_EQ (gsi::has_class ("QApplication"), true);
#endif

  tl_assert (rba::RubyInterpreter::instance ());

  bool err = false;
  try {
    rba::RubyInterpreter::instance ()->eval_string ("raise \"an error\"");
  } catch (tl::Exception &ex) {
    EXPECT_EQ (std::string (ex.msg (), 0, 22), std::string ("RuntimeError: an error"));
    err = true;
  }

  EXPECT_EQ (err, true);

  rba::RubyInterpreter::instance ()->eval_string ("puts 'Special chars: <&>'");
  err = false;
  try {
    rba::RubyInterpreter::instance ()->eval_string ("Quatsch");
  } catch (tl::Exception &ex) {
    EXPECT_EQ (std::string (ex.msg (), 0, 41) == std::string ("NameError: uninitialized constant Quatsch") ||
               std::string (ex.msg (), 0, 49) == std::string ("NameError: uninitialized constant Object::Quatsch"),
               true);
    err = true;
  }

  EXPECT_EQ (err, true);

  std::string fn (tl::testsrc ());
  fn += "/testdata/ruby/basic.rb";
  try {
    rba::RubyInterpreter::instance ()->load_file (fn.c_str ());
    rba::RubyInterpreter::instance ()->eval_string ("RBA::E.reset_inst");
  } catch (tl::ExitException &ex) {
    rba::RubyInterpreter::instance ()->eval_string ("RBA::E.reset_inst");
    EXPECT_EQ (ex.status (), 0);
  } catch (...) {
    rba::RubyInterpreter::instance ()->eval_string ("RBA::E.reset_inst");
    throw;
  }
}

void run_rubytest (tl::TestBase * /*_this*/, const std::string &fn)
{
  tl_assert (rba::RubyInterpreter::instance ());

  std::string fp (tl::testsrc ());
  fp += "/testdata/ruby/";
  fp += fn;
  rba::RubyInterpreter::instance ()->load_file (fp.c_str ());
}

#define RUBYTEST(n, file) \
  TEST(n) { run_rubytest(_this, file); }

RUBYTEST (antTest, "antTest.rb")
RUBYTEST (dbBooleanTest, "dbBooleanTest.rb")
RUBYTEST (dbBoxTest, "dbBoxTest.rb")
RUBYTEST (dbCellInstArrayTest, "dbCellInstArrayTest.rb")
RUBYTEST (dbCellMapping, "dbCellMapping.rb")
RUBYTEST (dbEdgePairsTest, "dbEdgePairsTest.rb")
RUBYTEST (dbEdgePairTest, "dbEdgePairTest.rb")
RUBYTEST (dbEdgesTest, "dbEdgesTest.rb")
RUBYTEST (dbEdgeTest, "dbEdgeTest.rb")
RUBYTEST (dbGlyphs, "dbGlyphs.rb")
RUBYTEST (dbInstanceTest, "dbInstanceTest.rb")
RUBYTEST (dbInstElementTest, "dbInstElementTest.rb")
RUBYTEST (dbLayerMapping, "dbLayerMapping.rb")
RUBYTEST (dbLibrary, "dbLibrary.rb")
RUBYTEST (dbLayoutTests1, "dbLayoutTests1.rb")
RUBYTEST (dbLayoutTests2, "dbLayoutTests2.rb")
RUBYTEST (dbCellTests, "dbCellTests.rb")
RUBYTEST (dbRecursiveShapeIterator, "dbRecursiveShapeIterator.rb")
RUBYTEST (dbRecursiveInstanceIterator, "dbRecursiveInstanceIterator.rb")
RUBYTEST (dbLayoutDiff, "dbLayoutDiff.rb")
RUBYTEST (dbLayoutQuery, "dbLayoutQuery.rb")
RUBYTEST (dbLayoutToNetlist, "dbLayoutToNetlist.rb")
RUBYTEST (dbLayoutVsSchematic, "dbLayoutVsSchematic.rb")
RUBYTEST (dbNetlistCrossReference, "dbNetlistCrossReference.rb")
RUBYTEST (dbMatrix, "dbMatrix.rb")
RUBYTEST (dbNetlist, "dbNetlist.rb")
RUBYTEST (dbNetlistDeviceClasses, "dbNetlistDeviceClasses.rb")
RUBYTEST (dbNetlistDeviceExtractors, "dbNetlistDeviceExtractors.rb")
RUBYTEST (dbNetlistWriterTests, "dbNetlistWriterTests.rb")
RUBYTEST (dbNetlistReaderTests, "dbNetlistReaderTests.rb")
RUBYTEST (dbNetlistCompare, "dbNetlistCompare.rb")
RUBYTEST (dbPathTest, "dbPathTest.rb")
RUBYTEST (dbPCells, "dbPCells.rb")
RUBYTEST (dbPointTest, "dbPointTest.rb")
RUBYTEST (dbPolygonTest, "dbPolygonTest.rb")
RUBYTEST (dbRegionTest, "dbRegionTest.rb")
RUBYTEST (dbReaders, "dbReaders.rb")
RUBYTEST (dbShapesTest, "dbShapesTest.rb")
RUBYTEST (dbSimplePolygonTest, "dbSimplePolygonTest.rb")
RUBYTEST (dbTextTest, "dbTextTest.rb")
RUBYTEST (dbTextsTest, "dbTextsTest.rb")
RUBYTEST (dbTilingProcessorTest, "dbTilingProcessorTest.rb")
RUBYTEST (dbTransTest, "dbTransTest.rb")
RUBYTEST (dbVectorTest, "dbVectorTest.rb")
RUBYTEST (dbUtilsTests, "dbUtilsTests.rb")
RUBYTEST (edtTest, "edtTest.rb")
RUBYTEST (extNetTracer, "extNetTracer.rb")
RUBYTEST (imgObject, "imgObject.rb")
RUBYTEST (layLayers, "layLayers.rb")
RUBYTEST (layLayoutView, "layLayoutView.rb")
RUBYTEST (layMarkers, "layMarkers.rb")
RUBYTEST (layMacro, "layMacro.rb")
RUBYTEST (layMenuTest, "layMenuTest.rb")
RUBYTEST (layPixelBuffer, "layPixelBuffer.rb")
RUBYTEST (laySession, "laySession.rb")
RUBYTEST (layTechnologies, "layTechnologies.rb")
RUBYTEST (laySaveLayoutOptions, "laySaveLayoutOptions.rb")
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
RUBYTEST (qtbinding, "qtbinding.rb")
#endif
RUBYTEST (rdbTest, "rdbTest.rb")
RUBYTEST (tlTest, "tlTest.rb")

#endif

