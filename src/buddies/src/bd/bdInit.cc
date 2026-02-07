
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

#include "bdInit.h"
#include "dbInit.h"
#include "tlCommandLineParser.h"
#include "tlProgress.h"
#include "version.h"
#include "gsi.h"
#include "gsiExpression.h"

#include <list>

namespace bd
{

void init ()
{
  std::string version = prg_version;
  version += " r";
  version += prg_rev;
  tl::CommandLineOptions::set_version (version);

  std::string license (prg_author);
  license += "\n";
  license += prg_date;
  license += ", Version ";
  license += prg_version;
  license += " r";
  license += prg_rev;
  license += "\n";
  license += "\n";
  license += prg_about_text;
  tl::CommandLineOptions::set_license (license);

  //  initialize db plugins
  db::init ();

  //  initialize the GSI class system (Variant binding, Expression support)
  //  We have to do this now since plugins may register GSI classes and before the
  //  ruby interpreter, because it depends on a proper class system.
  gsi::initialize ();

  //  initialize the tl::Expression subsystem with GSI-bound classes
  gsi::initialize_expressions ();
}

class ProgressAdaptor
  : public tl::ProgressAdaptor
{
public:
  ProgressAdaptor (int verbosity);
  virtual ~ProgressAdaptor ();

  virtual void trigger (tl::Progress *progress);
  virtual void yield (tl::Progress *progress);

private:
  int m_verbosity;
  std::string m_progress_text, m_progress_value;
};

ProgressAdaptor::ProgressAdaptor (int verbosity)
  : m_verbosity (verbosity)
{
  //  .. nothing yet ..
}

ProgressAdaptor::~ProgressAdaptor ()
{
  //  .. nothing yet ..
}

void
ProgressAdaptor::trigger (tl::Progress *progress)
{
  if (progress && first () == progress && tl::verbosity () >= m_verbosity) {

    std::string text = progress->desc ();

    if (m_progress_text != text) {
      tl::info << text << " ..";
      m_progress_text = text;
    }

    std::string value = progress->formatted_value ();
    if (m_progress_value != value) {
      tl::info << ".. " << value;
      m_progress_value = value;
    }

  }
}

void
ProgressAdaptor::yield (tl::Progress * /*progress*/)
{
  //  .. nothing yet ..
}

int _main_impl (int (*delegate) (int, char *[]), int argc, char *argv[])
{
  try {
    ProgressAdaptor progress_adaptor (10);
    init ();
    return (*delegate) (argc, argv);
  } catch (tl::CancelException & /*ex*/) {
    return 0;
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "unspecific error";
    return 1;
  }
}

}
