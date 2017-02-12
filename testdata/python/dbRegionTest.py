
import pya
import unittest
import sys

class DBRegionTest(unittest.TestCase):

  def test_1_Region(self):

    r = pya.Region()
    self.assertEqual(str(r), "")

    r.insert(pya.Box(0, 100, 200, 300))
    self.assertEqual(str(r), "(0,100;0,300;200,300;200,100)")

    r2 = pya.Region(pya.Box(50, 150, 250, 350))
    self.assertEqual(str(r2), "(50,150;50,350;250,350;250,150)")

    r += r2
    self.assertEqual(str(r), "(0,100;0,300;200,300;200,100);(50,150;50,350;250,350;250,150)")

    r.merge()
    self.assertEqual(str(r), "(0,100;0,300;50,300;50,350;250,350;250,150;200,150;200,100)")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBRegionTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

