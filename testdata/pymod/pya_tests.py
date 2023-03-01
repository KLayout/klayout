
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

  for suite in [
    unittest.TestLoader().loadTestsFromTestCase(tlTest.TLTest),
    unittest.TestLoader().loadTestsFromTestCase(dbPCells.DBPCellTests),
    unittest.TestLoader().loadTestsFromTestCase(dbLayoutTest.DBLayoutTest),
    unittest.TestLoader().loadTestsFromTestCase(dbPolygonTest.DBPolygonTests),
    unittest.TestLoader().loadTestsFromTestCase(dbReaders.DBReadersTests),
    unittest.TestLoader().loadTestsFromTestCase(dbRegionTest.DBRegionTest),
    unittest.TestLoader().loadTestsFromTestCase(dbTransTest.DBTransTests),
    unittest.TestLoader().loadTestsFromTestCase(dbLayoutToNetlist.DBLayoutToNetlistTests),
    unittest.TestLoader().loadTestsFromTestCase(dbLayoutVsSchematic.DBLayoutVsSchematicTests),
    unittest.TestLoader().loadTestsFromTestCase(dbNetlistCrossReference.DBNetlistCrossReferenceTests),
    unittest.TestLoader().loadTestsFromTestCase(layLayers.LAYLayersTests)
  ]:
    if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
      sys.exit(1)

