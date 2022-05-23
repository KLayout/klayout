
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
  
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(tlTest.TLTest)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbPCells.DBPCellTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbLayoutTest.DBLayoutTest)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbPolygonTest.DBPolygonTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbReaders.DBReadersTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbRegionTest.DBRegionTest)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbTransTest.DBTransTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbLayoutToNetlist.DBLayoutToNetlistTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbLayoutVsSchematic.DBLayoutVsSchematicTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(dbNetlistCrossReference.DBNetlistCrossReferenceTests)
  suite = unittest.TestLoader().loadTestsFromTestCase(layLayers.LAYLayersTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

