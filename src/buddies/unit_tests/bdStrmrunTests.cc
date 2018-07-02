
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "tlUnitTest.h"

#if defined(HAVE_QT)
# include <QProcess>
# include <QStringList>
#endif

//  Testing the converter main implementation (CIF)
TEST(1)
{
//  TODO: provide a Qt-less way of running these tests
#if defined(HAVE_QT)
  QProcess process;
  process.setProcessChannelMode (QProcess::MergedChannels);

  QStringList args;

  std::string fp (tl::testsrc ());
  fp += "/testdata/bd/strmrun.py";
  args << tl::to_qstring (fp);

  process.start (tl::to_qstring ("./strmrun"), args);
  bool success = process.waitForFinished (-1);

  QByteArray ba = process.readAll ();
  EXPECT_EQ (ba.constData (), "Hello, world (0,-42;42,0)!\n");
  EXPECT_EQ (success, true);
#else
  EXPECT_EQ (true, false);
#endif
}

