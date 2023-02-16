
import sys
import os
import unittest
import testprep

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python"))

# Include all tests from testdata/python
# Missing:
#   - basic.py   (Test classes not available yet)
#   - qtbinding  (No applicable because QApplication is missing)

import tlTest
import dbPCells
import dbLayoutTest
import dbPolygonTest
import dbReaders
import dbRegionTest
import dbTransTest
import dbLayoutToNetlist
import dbLayoutVsSchematic
import dbNetlistCrossReference
import layLayers
import layPixelBuffer

if __name__ == '__main__':
  loader = unittest.TestLoader()
  suite = unittest.TestSuite()
  suite.addTests(loader.loadTestsFromTestCase(tlTest.TLTest))
  suite.addTests(loader.loadTestsFromTestCase(dbPCells.DBPCellTests))
  suite.addTests(loader.loadTestsFromTestCase(dbLayoutTest.DBLayoutTest))
  suite.addTests(loader.loadTestsFromTestCase(dbPolygonTest.DBPolygonTests))
  suite.addTests(loader.loadTestsFromTestCase(dbReaders.DBReadersTests))
  suite.addTests(loader.loadTestsFromTestCase(dbRegionTest.DBRegionTest))
  suite.addTests(loader.loadTestsFromTestCase(dbTransTest.DBTransTests))
  suite.addTests(loader.loadTestsFromTestCase(dbLayoutToNetlist.DBLayoutToNetlistTests))
  suite.addTests(loader.loadTestsFromTestCase(dbLayoutVsSchematic.DBLayoutVsSchematicTests))
  suite.addTests(loader.loadTestsFromTestCase(dbNetlistCrossReference.DBNetlistCrossReferenceTests))
  suite.addTests(loader.loadTestsFromTestCase(layLayers.LAYLayersTests))
  suite.addTests(loader.loadTestsFromTestCase(layPixelBuffer.LAYPixelBufferTests))

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)
