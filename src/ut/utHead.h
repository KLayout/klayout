
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

 
#ifndef HDR_utHead
#define HDR_utHead

#include "utCommon.h"
#include "utTestBase.h"
#include "utTestConsole.h"

namespace ut {

extern tl::LogTee ctrl;
extern tl::LogTee noctrl;

/**
 *  @brief The unit test execution function
 */
UT_PUBLIC int main (int argc, char **argv);

} // namespace ut

#define TEST(NAME) \
  namespace {\
struct TestImpl##NAME \
      : public ut::TestBase \
    { \
      TestImpl##NAME () : TestBase (__FILE__, #NAME) { } \
      virtual void execute (ut::TestBase *_this) throw (tl::Exception); \
    }; \
    static TestImpl##NAME TestImpl_Inst##NAME; \
  } \
  void TestImpl##NAME::execute (ut::TestBase *_this) throw (tl::Exception)

#define EXPECT_EQ(WHAT,EQUALS) \
  _this->checkpoint (__FILE__, __LINE__); \
  _this->eq_helper (true, (WHAT), (EQUALS), #WHAT, #EQUALS, __FILE__, __LINE__);

#define EXPECT_NE(WHAT,EQUALS) \
  _this->checkpoint (__FILE__, __LINE__); \
  _this->eq_helper (false, (WHAT), (EQUALS), #WHAT, #EQUALS, __FILE__, __LINE__);

#define EXPECT(WHAT) \
  _this->checkpoint (__FILE__, __LINE__); \
  if (!(WHAT)) { \
    std::ostringstream sstr; \
    sstr << #WHAT << " is not true"; \
    _this->raise (__FILE__, __LINE__, sstr.str ()); \
  } 

#define CHECKPOINT() \
  _this->checkpoint (__FILE__, __LINE__);

#define FAIL_ARG(MSG,WHAT) \
  { \
    std::ostringstream sstr; \
    sstr << MSG << ", value is " << (WHAT); \
    _this->raise (__FILE__, __LINE__, sstr.str ()); \
  } 

#endif

