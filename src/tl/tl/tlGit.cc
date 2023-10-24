
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


#include "tlGit.h"
#include "tlFileUtils.h"
#include "tlProgress.h"
#include "tlStaticObjects.h"

#include <git2.h>
#include <cstdio>

namespace tl
{

// ---------------------------------------------------------------
//  Library initialization helper

namespace {

class InitHelper
{
public:
  InitHelper ()
  {
    git_libgit2_init ();
  }

  ~InitHelper ()
  {
    git_libgit2_shutdown ();
  }

  static void ensure_initialized ()
  {
    if (ms_init) {
      return;
    }

    ms_init = new InitHelper ();
    tl::StaticObjects::reg (&ms_init);
  }

private:
  static InitHelper *ms_init;
};

InitHelper *InitHelper::ms_init = 0;

}

// ---------------------------------------------------------------
//  GitCollection implementation

GitObject::GitObject (const std::string &local_path)
  : m_local_path (local_path), m_is_temp (false)
{
  InitHelper::ensure_initialized ();

  if (local_path.empty ()) {
    //  @@@ generic tempnam on Windows/Posix ...
    char tmp[] = "git2klayoutXXXXXX";
    mkstemp (tmp);
    m_local_path = tmp;
    m_is_temp = true;
  }

  tl::mkpath (m_local_path);

  //  ensures the directory is clean
  tl::rm_dir_recursive (m_local_path); // @@@ TODO: error handling?
  tl::mkpath (m_local_path);
}

GitObject::~GitObject ()
{
  if (m_is_temp) {
    tl::rm_dir_recursive (m_local_path);
  }
}

static int
fetch_progress (const git_indexer_progress *stats, void *payload)
{
  tl::RelativeProgress *progress = reinterpret_cast<tl::RelativeProgress *> (payload);

  //  first half of progress
  size_t count = size_t (5000.0 * double (stats->received_objects) / double (std::max (1u, stats->total_objects)) + 1e-10);
  progress->set (count);

  return 0;
}

static void
checkout_progress(const char * /*path*/, size_t cur, size_t tot, void *payload)
{
  tl::RelativeProgress *progress = reinterpret_cast<tl::RelativeProgress *> (payload);

  //  first half of progress
  size_t count = size_t (5000.0 * double (cur) / double (std::max (size_t (1), tot)) + 1e-10);
  progress->set (count + 5000u);
}


void
GitObject::read (const std::string &url, const std::string &filter, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  //  @@@ use callback, timeout?
  tl::RelativeProgress progress (tl::to_string (tr ("Download progress")), 10000, 1 /*yield always*/);

  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

  const char *paths_cstr[1];
  paths_cstr[0] = filter.c_str ();
  if (! filter.empty ()) {
    checkout_opts.paths.count = 1;
    checkout_opts.paths.strings = (char **) &paths_cstr;
  }

  checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE |
                                    GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
  checkout_opts.progress_cb = &checkout_progress;
  checkout_opts.progress_payload = (void *) &progress;

  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;

  clone_opts.checkout_opts = checkout_opts;

  //  NOTE: really has to be a branch! Tags won't work.
  if (! branch.empty ()) {
    clone_opts.checkout_branch = branch.c_str ();
  }

#if LIBGIT2_VER_MAJOR > 1 || (LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 7)
  clone_opts.fetch_opts.depth = 1;  // shallow (single commit)
#endif
  clone_opts.fetch_opts.callbacks.transfer_progress = &fetch_progress;
  clone_opts.fetch_opts.callbacks.payload = (void *) &progress;

  //  Do the clone
  git_repository *cloned_repo = NULL;
  int error = git_clone (&cloned_repo, url.c_str (), m_local_path.c_str (), &clone_opts);
  if (error != 0) {
    const git_error *err = git_error_last ();
    throw tl::Exception (tl::to_string (tr ("Error cloning Git repo (%d): %s")), err->klass, (const char *) err->message);
  }

  if (cloned_repo) {
    git_repository_free (cloned_repo);
    //  remove the worktree we don't need
    tl::rm_dir_recursive (tl::combine_path (m_local_path, ".git"));
  }
}

bool
GitObject::download (const std::string &url, const std::string &target, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  GitObject obj (target);
  obj.read (url, std::string (), branch, timeout, callback);
  return false;
}

tl::InputStream *
GitObject::download_item (const std::string &url, const std::string &file, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  GitObject obj;
  obj.read (url, file, branch, timeout, callback);

  //  extract the file and return a memory blob, so we can delete the temp folder

  tl::InputStream file_stream (tl::combine_path (obj.local_path (), file));
  std::string data = file_stream.read_all ();

  char *data_copy = new char [data.size ()];
  memcpy (data_copy, data.c_str (), data.size ());
  return new tl::InputStream (new tl::InputMemoryStream (data_copy, data.size (), true));
}

}
