
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

#if defined(HAVE_GIT2)

#include "tlGit.h"
#include "tlFileUtils.h"
#include "tlProgress.h"
#include "tlStaticObjects.h"
#include "tlLog.h"

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
    m_local_path = tl::tmpdir ("git2klayout");
    m_is_temp = true;
  }

  //  ensures the directory is clean
  if (! m_is_temp) {
    if (! tl::rm_dir_recursive (m_local_path)) {
      throw tl::Exception (tl::to_string (tr ("Unable to clean local Git repo path: %s")), m_local_path);
    }
    if (! tl::mkpath (m_local_path)) {
      throw tl::Exception (tl::to_string (tr ("Unable to regenerate local Git repo path: %s")), m_local_path);
    }
  }
}

GitObject::~GitObject ()
{
  if (m_is_temp) {
    tl::rm_dir_recursive (m_local_path);
  }
}

static int
#if LIBGIT2_VER_MAJOR >= 1
fetch_progress (const git_indexer_progress *stats, void *payload)
#else
fetch_progress (const git_transfer_progress *stats, void *payload)
#endif
{
  tl::RelativeProgress *progress = reinterpret_cast<tl::RelativeProgress *> (payload);

  //  first half of progress
  size_t count = size_t (5000.0 * double (stats->received_objects) / double (std::max (1u, stats->total_objects)) + 1e-10);
  try {
    progress->set (count);
  } catch (...) {
    //  TODO: stop
  }

  return 0;
}

static void
checkout_progress(const char * /*path*/, size_t cur, size_t tot, void *payload)
{
  tl::RelativeProgress *progress = reinterpret_cast<tl::RelativeProgress *> (payload);

  //  first half of progress
  size_t count = size_t (5000.0 * double (cur) / double (std::max (size_t (1), tot)) + 1e-10);
  try {
    progress->set (count + 5000u);
  } catch (...) {
    //  ignore cancel requests (TODO: how to stop?)
  }
}

static void check (int error)
{
  if (error != 0) {
#if LIBGIT2_VER_MAJOR > 0 || (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR >= 28)
    const git_error *err = git_error_last ();
#else
    const git_error *err = giterr_last ();
#endif
    throw tl::Exception (tl::to_string (tr ("Error cloning Git repo: %s")), (const char *) err->message);
  }
}

static bool
ref_matches (const char *name, const std::string &ref)
{
  if (!name) {
    return false;
  } else if (name == ref) {
    return true;
  } else if (name == "refs/heads/" + ref) {
    return true;
  } else if (name == "refs/tags/" + ref) {
    return true;
  } else {
    return false;
  }
}

namespace
{

class GitBuffer
{
public:
  GitBuffer ()
  {
    m_buf = GIT_BUF_INIT_CONST (NULL, 0);
  }

  ~GitBuffer ()
  {
#if LIBGIT2_VER_MAJOR > 0 || (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR >= 28)
    git_buf_dispose (&m_buf);
#else
    git_buf_free (&m_buf);
#endif
  }

  const char *c_str () const { return m_buf.ptr; }

  git_buf *get () { return &m_buf; }
  const git_buf *get () const { return &m_buf; }

private:
  git_buf m_buf;
};

}

static void
checkout_branch (git_repository *repo, git_remote *remote, const git_checkout_options *co_opts, const char *branch)
{
  GitBuffer remote_branch;
  git_oid oid;

  //  if no branch is given, use the default branch
  if (! branch) {
    check (git_remote_default_branch (remote_branch.get (), remote));
    branch = remote_branch.c_str ();
    if (tl::verbosity () >= 10) {
      tl::info << tr ("Git checkout: Using default branch for repository ") << git_remote_url (remote) << ": " << branch;
    }
  } else {
    if (tl::verbosity () >= 10) {
      tl::info << tr ("Git checkout: Checking out branch for repository ") << git_remote_url (remote) << ": " << branch;
    }
  }

  //  resolve the branch by using ls-remote:

  size_t n = 0;
  const git_remote_head **ls = NULL;
  check (git_remote_ls (&ls, &n, remote));

  if (tl::verbosity () >= 20) {
    tl::info << "Git checkout: ls-remote on " << git_remote_url (remote) << ":";
  }

  bool found = false;

  for (size_t i = 0; i < n; ++i) {
    const git_remote_head *rh = ls[i];
    if (tl::verbosity () >= 20) {
      char oid_fmt [80];
      git_oid_tostr (oid_fmt, sizeof (oid_fmt), &rh->oid);
      tl::info << "  " << rh->name << ": " << (const char *) oid_fmt;
    }
    if (ref_matches (rh->name, branch)) {
      oid = rh->oid;
      found = true;
    }
  }

  if (! found) {
    throw tl::Exception (tl::to_string (tr ("Git checkout - Unable to resolve reference name: ")) + branch);
  }

  if (tl::verbosity () >= 10) {
    char oid_fmt [80];
    git_oid_tostr (oid_fmt, sizeof (oid_fmt), &oid);
    tl::info << tr ("Git checkout: resolving ") << branch << tr (" to ") << (const char *) oid_fmt;
  }

  check (git_repository_set_head_detached (repo, &oid));
  check (git_checkout_head (repo, co_opts));
}

int
#if LIBGIT2_VER_MAJOR >= 1
credentials_cb (git_credential ** /*out*/, const char * /*url*/, const char * /*username*/, unsigned int /*allowed_types*/, void *)
#else
credentials_cb (git_cred ** /*out*/, const char * /*url*/, const char * /*username*/, unsigned int /*allowed_types*/, void *)
#endif
{
  //  no credentials aquired
#if LIBGIT2_VER_MAJOR > 0 || (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR >= 28)
  git_error_set_str (GIT_ERROR_NONE, "anonymous access is supported only, but server requests credentials");
#else
  giterr_set_str (GITERR_NONE, "anonymous access is supported only, but server requests credentials");
#endif
  return GIT_EUSER;

}

void
GitObject::read (const std::string &org_url, const std::string &org_filter, const std::string &subfolder, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  std::string url = org_url;

  std::string filter = org_filter;
  if (! subfolder.empty ()) {
    if (filter.empty ()) {
      filter = subfolder + "/**";
    } else {
      filter = subfolder + "/" + filter;
    }
  }

  //  TODO: use callback, timeout?
  tl::RelativeProgress progress (tl::to_string (tr ("Download progress")), 10000, 1 /*yield always*/);

  //  build checkout options

  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
  checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;

  const char *paths_cstr[1];
  paths_cstr[0] = filter.c_str ();
  if (! filter.empty ()) {
    checkout_opts.paths.count = 1;
    checkout_opts.paths.strings = (char **) &paths_cstr;
  }

  checkout_opts.progress_cb = &checkout_progress;
  checkout_opts.progress_payload = (void *) &progress;

  //  build fetch options

  git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

  fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_AUTO;

#if LIBGIT2_VER_MAJOR > 1 || (LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 7)
  fetch_opts.depth = 1;  // shallow (single commit)
#endif
  fetch_opts.callbacks.transfer_progress = &fetch_progress;
  fetch_opts.callbacks.credentials = &credentials_cb;
  fetch_opts.callbacks.payload = (void *) &progress;

  //  build refspecs in case they are needed

  char *refs[] = { (char *) branch.c_str () };
  git_strarray refspecs;
  refspecs.count = 1;
  refspecs.strings = refs;
  git_strarray *refspecs_p = branch.empty () ? NULL : &refspecs;

  //  Make repository

  git_repository *cloned_repo = NULL;
  git_remote *remote = NULL;

  try {

    check (git_repository_init (&cloned_repo, m_local_path.c_str (), 0));

    check (git_remote_create (&remote, cloned_repo, "download", url.c_str ()));

    //  actually fetch
    if (tl::verbosity () >= 10) {
      tl::info << tr ("Fetching Git repo from ") << git_remote_url (remote) << " ...";
    }
    check (git_remote_fetch (remote, refspecs_p, &fetch_opts, NULL));

    //  checkout
    checkout_branch (cloned_repo, remote, &checkout_opts, branch.empty () ? 0 : branch.c_str ());

    //  free the repo and remote
    git_repository_free (cloned_repo);
    git_remote_free (remote);

    //  get rid of ".git" - we do not need it anymore

    tl::rm_dir_recursive (tl::combine_path (m_local_path, ".git"));

  } catch (...) {
    //  free the repo in the error case
    if (cloned_repo != NULL) {
      git_repository_free (cloned_repo);
    }
    if (remote != NULL) {
      git_remote_free (remote);
    }
    throw;
  }

  //  pull subfolder files to target path level
  if (! subfolder.empty ()) {

    std::string pp = tl::combine_path (m_local_path, subfolder);
    if (! tl::is_dir (pp)) {
      throw tl::Exception (tl::to_string (tr ("Error cloning Git repo - failed to fetch subdirectory: ")) + pp);
    }

    //  rename the source to a temporary folder so we don't overwrite the source folder with something from within
    std::string tmp_dir = "__temp__";
    for (unsigned int i = 0; ; ++i) {
      if (! tl::file_exists (tl::combine_path (m_local_path, tmp_dir + tl::to_string (i)))) {
        tmp_dir += tl::to_string (i);
        break;
      }
    }
    auto pc = tl::split (subfolder, "/");
    if (! tl::rename_file (tl::combine_path (m_local_path, pc.front ()), tmp_dir)) {
      throw tl::Exception (tl::to_string (tr ("Error cloning Git repo - failed to rename temp folder")));
    }
    pc.front () = tmp_dir;

    if (! tl::mv_dir_recursive (tl::combine_path (m_local_path, tl::join (pc, "/")), m_local_path)) {
      throw tl::Exception (tl::to_string (tr ("Error cloning Git repo - failed to move subdir components")));
    }

  }
}

bool
GitObject::download (const std::string &url, const std::string &target, const std::string &subfolder, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  try {

    GitObject obj (target);
    obj.read (url, std::string (), subfolder, branch, timeout, callback);

    return true;

  } catch (tl::Exception &ex) {

    tl::error << tl::sprintf (tl::to_string (tr ("Error downloading Git repo from %s (subdir '%s', ref '%s')")), url, subfolder, branch);

    return false;

  }
}

tl::InputStream *
GitObject::download_item (const std::string &url, const std::string &file, const std::string &subfolder, const std::string &branch, double timeout, tl::InputHttpStreamCallback *callback)
{
  GitObject obj;
  obj.read (url, file, subfolder, branch, timeout, callback);

  //  extract the file and return a memory blob, so we can delete the temp folder

  tl::InputStream file_stream (tl::combine_path (obj.local_path (), file));
  std::string data = file_stream.read_all ();

  char *data_copy = new char [data.size ()];
  memcpy (data_copy, data.c_str (), data.size ());
  return new tl::InputStream (new tl::InputMemoryStream (data_copy, data.size (), true));
}

}

#endif
