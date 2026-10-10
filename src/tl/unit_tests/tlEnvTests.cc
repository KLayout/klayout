
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

#include "tlEnv.h"
#include "tlString.h"
#include "tlThreads.h"
#include "tlUnitTest.h"

#include <string>
#include <vector>

const char *dne_name = "__DOES_NOT_EXIST__";

TEST(1)
{
  EXPECT_EQ (tl::has_env (dne_name), false);

  tl::set_env (dne_name, "123");
  EXPECT_EQ (tl::has_env (dne_name), true);

  EXPECT_EQ (tl::get_env (dne_name), "123");

  tl::set_env (dne_name, "42");
  EXPECT_EQ (tl::has_env (dne_name), true);

  EXPECT_EQ (tl::get_env (dne_name), "42");

  tl::unset_env (dne_name);
  EXPECT_EQ (tl::get_env (dne_name, "bla"), "bla");
  EXPECT_EQ (tl::has_env (dne_name), false);
}

TEST(2)
{
  const char *name = "__KLAYOUT_ENV_TEST_2__";

  tl::unset_env (name);
  EXPECT_EQ (tl::has_env (name), false);

  //  different value lengths including empty
  const char *values[] = { "", "x", "1234567890", "a somewhat longer value with spaces" };
  for (size_t i = 0; i < sizeof (values) / sizeof (values[0]); ++i) {
    tl::set_env (name, values [i]);
    EXPECT_EQ (tl::has_env (name), true);
    EXPECT_EQ (tl::get_env (name), std::string (values [i]));
  }

  //  repeated set of the same name, growing then shrinking
  tl::set_env (name, "short");
  EXPECT_EQ (tl::get_env (name), "short");
  tl::set_env (name, "a much longer value that forces the buffer to grow");
  EXPECT_EQ (tl::get_env (name), "a much longer value that forces the buffer to grow");
  tl::set_env (name, "s");
  EXPECT_EQ (tl::get_env (name), "s");

  tl::unset_env (name);
  EXPECT_EQ (tl::has_env (name), false);
  EXPECT_EQ (tl::get_env (name, "def"), "def");
}

class EnvThread
  : public tl::Thread
{
public:
  EnvThread (int n)
    : m_n (n), m_failed (false)
  { }

  bool failed () const { return m_failed; }

  void run ()
  {
    for (int i = 0; i < m_n; ++i) {
      std::string name = "__KLAYOUT_ENV_MT_" + tl::to_string (i % 16);
      std::string value = "value_" + tl::to_string (i % 16);
      if (! tl::has_env (name) || tl::get_env (name) != value) {
        m_failed = true;
      }
    }
  }

private:
  int m_n;
  bool m_failed;
};

TEST(3)
{
  //  Concurrent readers through the tl lock. The variables are set before the threads start and
  //  removed afterwards: setenv/unsetenv are not safe against foreign readers (Qt helper threads).
  for (int i = 0; i < 16; ++i) {
    tl::set_env ("__KLAYOUT_ENV_MT_" + tl::to_string (i), "value_" + tl::to_string (i));
  }

  const int n_threads = 8;
  const int n_iter = 20000;

  std::vector<EnvThread *> threads;
  for (int i = 0; i < n_threads; ++i) {
    threads.push_back (new EnvThread (n_iter));
  }

  for (size_t i = 0; i < threads.size (); ++i) {
    threads [i]->start ();
  }
  for (size_t i = 0; i < threads.size (); ++i) {
    threads [i]->wait ();
  }
  for (size_t i = 0; i < threads.size (); ++i) {
    EXPECT_EQ (threads [i]->failed (), false);
    delete threads [i];
  }

  for (int i = 0; i < 16; ++i) {
    tl::unset_env ("__KLAYOUT_ENV_MT_" + tl::to_string (i));
  }
}
