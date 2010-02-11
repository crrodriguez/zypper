/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#ifndef ZMART_SOURCE_CALLBACKS_H
#define ZMART_SOURCE_CALLBACKS_H

#include <sstream>
#include <boost/format.hpp>

#include "zypp/base/Logger.h"
#include "zypp/ZYppCallbacks.h"
#include "zypp/Pathname.h"
#include "zypp/Url.h"

#include "Zypper.h"
#include "CommitData.h"
#include "utils/prompt.h"
#include "utils/misc.h"

///////////////////////////////////////////////////////////////////
namespace ZmartRecipients
{
///////////////////////////////////////////////////////////////////

// progress for downloading a resolvable
struct DownloadResolvableReportReceiver : public zypp::callback::ReceiveReport<zypp::repo::DownloadResolvableReport>
{
  zypp::Package::constPtr _pkg_ptr;
  zypp::Url _url;
  zypp::Pathname _patch;
  zypp::ByteCount _patch_size;

  // Dowmload delta rpm:
  // - path below url reported on start()
  // - expected download size (0 if unknown)
  // - download is interruptable
  // - problems are just informal
  virtual void startDeltaDownload(
      const zypp::Pathname & filename, const zypp::ByteCount & downloadsize)
  {
    Zypper & zypper = *Zypper::instance();

    CommitData::PkgDownloadData & dd = zypper.commitData()
        ._dwnld_data[_pkg_ptr->location().filename().basename()];

    dd.delta_dwnld_pg = 0;
    dd.delta_filename = filename.basename();
    dd.delta_size = downloadsize;

    zypper.out().commitProgress(zypper.commitData());

    /*
    _delta = filename;
    _delta_size = downloadsize;
    std::ostringstream s;
    s << _("Retrieving delta") << ": "
        << _delta << ", " << _delta_size;
    Zypper::instance()->out().info(s.str());
    */
  }

  // implementation not needed, the progress is reported by the media backend
  // virtual bool progressDeltaDownload( int value )

  virtual void problemDeltaDownload( const std::string & description )
  {
    Zypper::instance()->out().error(description);
  }

  // implementation not needed, the media backend reports the download progress
  // virtual void finishDeltaDownload()

  // Apply delta rpm:
  // - local path of downloaded delta
  // - aplpy is not interruptable
  // - problems are just informal
  virtual void startDeltaApply( const zypp::Pathname & filename )
  {
    Zypper & zypper = *Zypper::instance();
    zypper.commitData()._dwnld_data[_pkg_ptr->location().filename().basename()].delta_apply_pg = 0;
    zypper.out().commitProgress(zypper.commitData());

    // std::ostringstream s;
    // translators: this text is a progress display label e.g. "Applying delta foo [42%]"
    // s << _("Applying delta") << ": " << _delta;
    // _label_apply_delta = s.str();
    // Zypper::instance()->out().progressStart("apply-delta", _label_apply_delta, false);
  }

  virtual void progressDeltaApply( int value )
  {
    Zypper & zypper = *Zypper::instance();
    zypper.commitData()._dwnld_data[_pkg_ptr->location().filename().basename()].delta_apply_pg = value;
    zypper.out().commitProgress(zypper.commitData());
    // Zypper::instance()->out().progress("apply-delta", _label_apply_delta, value);
  }

  virtual void problemDeltaApply( const std::string & description )
  {
    // Zypper::instance()->out().progressEnd("apply-delta", _label_apply_delta, true);
    // Zypper::instance()->out().error(description);
  }

  virtual void finishDeltaApply()
  {
    Zypper & zypper = *Zypper::instance();
    zypper.commitData()._dwnld_data[_pkg_ptr->location().filename().basename()].delta_apply_pg = 100;
    zypper.out().commitProgress(zypper.commitData());
    // Zypper::instance()->out().progressEnd("apply-delta", _label_apply_delta);
  }

  // Download patch rpm:
  // - path below url reported on start()
  // - expected download size (0 if unknown)
  // - download is interruptable
  virtual void startPatchDownload( const zypp::Pathname & filename, const zypp::ByteCount & downloadsize )
  {
    _patch = filename.basename();
    _patch_size = downloadsize;
    std::ostringstream s;
    s << _("Retrieving patch rpm") << ": " << _patch << ", " << _patch_size;
    Zypper::instance()->out().info(s.str());
  }

  virtual bool progressPatchDownload( int value )
  {
    // seems this is never called, the progress is reported by the media backend anyway
    INT << "not impelmented" << std::endl;
    // TranslatorExplanation This text is a progress display label e.g. "Applying patch rpm [42%]"
    //display_step( "apply-delta", _("Applying patch rpm") /* + _patch.asString() */, value );
    return true;
  }

  virtual void problemPatchDownload( const std::string & description )
  {
    Zypper::instance()->out().error(description);
  }

  // implementation not needed prehaps - the media backend reports the download progress
  /*
  virtual void finishPatchDownload()
  {
    display_done ("apply-delta", cout_v);
  }
  */

  /** this is interesting because we have full resolvable data at hand here
   * The media backend has only the file URI
   * \todo combine this and the media data progress callbacks in a reasonable manner
   */
  virtual void start( zypp::Resolvable::constPtr resolvable_ptr, const zypp::Url & url )
  {
    zypp::Package::constPtr pkg = zypp::asKind<zypp::Package> (resolvable_ptr);
    _pkg_ptr =  pkg;
    _url = url;
    Zypper & zypper = *Zypper::instance();
    CommitData & cd = zypper.commitData();

    ++cd._last_dwnld_nr;

    std::ostringstream s;
    s << boost::format(_("Retrieving %s %s-%s.%s"))
        % kind_to_string_localized(_pkg_ptr->kind(), 1)
        % _pkg_ptr->name()
        % _pkg_ptr->edition() % _pkg_ptr->arch();

    s << " (" << cd._last_dwnld_nr << "/" << cd._pkgs_to_get << ")";

    s << ", " << _pkg_ptr->downloadSize () << " "
      // TranslatorExplanation %s is package size like "5.6 M"
      << boost::format(_("(%s unpacked)")) % _pkg_ptr->installSize();

    if (cd._commit_running)
    {
      CommitData::PkgDownloadData dd;
      dd.seq_number = cd._last_dwnld_nr;
      cd._dwnld_data[_pkg_ptr->location().filename().basename()] = dd;
      zypper.out().commitProgress(cd);
    }
    else
      zypper.out().info(s.str());
  }

  // return false if the download should be aborted right now
  virtual bool progress(int value, zypp::Resolvable::constPtr /*resolvable_ptr*/)
  {
    // seems this is never called, the progress is reported by the media backend anyway
    INT << "not impelmented" << std::endl;
    return true;
  }

  virtual Action problem( zypp::Resolvable::constPtr resolvable_ptr, Error /*error*/, const std::string & description )
  {
    Zypper::instance()->out().error(description);
    DBG << "error report" << std::endl;

    Action action = (Action) read_action_ari(PROMPT_ARI_RPM_DOWNLOAD_PROBLEM, ABORT);
    if (action == DownloadResolvableReport::RETRY)
      --Zypper::instance()->runtimeData().commit_pkg_current;
    else
    {
      zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>(resolvable_ptr);
      // TODO better mark as/move to failed
      Zypper::instance()->commitData()._dwnld_data.erase(
          pkg->location().filename().basename());
    }
    return action;
  }

  // implementation not needed prehaps - the media backend reports the download progress
  virtual void finish( zypp::Resolvable::constPtr resolvable_ptr, Error error, const std::string & reason )
  {
    Zypper & zypper = *Zypper::instance();
    if (zypper.commitData()._commit_running)
    {
      zypp::Package::constPtr pkg = zypp::asKind<zypp::Package> (resolvable_ptr);
      zypper.commitData().markDownloadDone(pkg->location().filename().basename());
      zypper.out().commitProgress(zypper.commitData());
      zypper.commitData()._dwnld_data_done.clear();
    }
  }
};

struct ProgressReportReceiver  : public zypp::callback::ReceiveReport<zypp::ProgressReport>
{
  virtual void start( const zypp::ProgressData &data )
  {
    Zypper::instance()->out().progressStart(
        zypp::str::numstring(data.numericId()),
        data.name(),
        data.reportAlive());
  }

  virtual bool progress( const zypp::ProgressData &data )
  {
    if (data.reportAlive())
      Zypper::instance()->out().progress(
          zypp::str::numstring(data.numericId()),
          data.name());
    else
      Zypper::instance()->out().progress(
          zypp::str::numstring(data.numericId()),
          data.name(), data.val());
    return true;
  }

//   virtual Action problem( zypp::Repository /*repo*/, Error error, const std::string & description )
//   {
//     display_done ();
//     display_error (error, description);
//     return (Action) read_action_ari ();
//   }

  virtual void finish( const zypp::ProgressData &data )
  {
    Zypper::instance()->out().progressEnd(
        zypp::str::numstring(data.numericId()),
        data.name());
  }
};


struct RepoReportReceiver  : public zypp::callback::ReceiveReport<zypp::repo::RepoReport>
{
  virtual void start(const zypp::ProgressData & pd, const zypp::RepoInfo repo)
  {
    _repo = repo;
    Zypper::instance()->out()
      .progressStart("repo", "(" + _repo.name() + ") " + pd.name());
  }

  virtual bool progress(const zypp::ProgressData & pd)
  {
    Zypper::instance()->out()
      .progress("repo", "(" + _repo.name() + ") " + pd.name(), pd.val());
    return true;
  }

  virtual Action problem( zypp::Repository /*repo*/, Error error, const std::string & description )
  {
    Zypper::instance()->out()
      .progressEnd("repo", "(" + _repo.name() + ") ");
    Zypper::instance()->out().error(zcb_error2str(error, description));
    return (Action) read_action_ari (PROMPT_ARI_REPO_PROBLEM, ABORT);
  }

  virtual void finish( zypp::Repository /*repo*/, const std::string & task, Error error, const std::string & reason )
  {
    Zypper::instance()->out()
      .progressEnd("repo", "(" + _repo.name() + ") ");
    if (error != NO_ERROR)
      Zypper::instance()->out().error(zcb_error2str(error, reason));
//    display_step(100);
    // many of these, avoid newline -- probably obsolete??
    //if (task.find("Reading patch") == 0)
      //cout_n << '\r' << flush;
//    else
//      display_done ("repo", cout_n);
  }

  zypp::RepoInfo _repo;
};
    ///////////////////////////////////////////////////////////////////
}; // namespace ZmartRecipients
///////////////////////////////////////////////////////////////////

class SourceCallbacks {

  private:
    ZmartRecipients::RepoReportReceiver _repoReport;
    ZmartRecipients::DownloadResolvableReportReceiver _downloadReport;
    ZmartRecipients::ProgressReportReceiver _progressReport;
  public:
    SourceCallbacks()
    {
      _repoReport.connect();
      _downloadReport.connect();
      _progressReport.connect();
    }

    ~SourceCallbacks()
    {
      _repoReport.disconnect();
      _downloadReport.disconnect();
      _progressReport.disconnect();
    }

};

#endif
// Local Variables:
// mode: c++
// c-basic-offset: 2
// End:
