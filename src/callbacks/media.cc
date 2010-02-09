/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#include "callbacks/media.h"

#include "utils/misc.h"
#include "utils/messages.h"

#include "zypp/Pathname.h"
#include "zypp/media/MediaManager.h"

using namespace zypp;
using namespace std;


// ---------[ common ]--------------------------------------------------------

static void set_common_option_help(PromptOptions & popts)
{
  // help text for the "Abort, retry, ignore?" prompt for media errors
  popts.setOptionHelp(0, _("Skip retrieval of the file and abort current operation."));
  // help text for the "Abort, retry, ignore?" prompt for media errors
  popts.setOptionHelp(1, _("Try to retrieve the file again."));
  // help text for the "Abort, retry, ignore?" prompt for media errors
  popts.setOptionHelp(2, _("Skip retrieval of the file and try to continue with the operation without the file."));
  // help text for the "Abort, retry, ignore?" prompt for media errors
  popts.setOptionHelp(3, _("Change current base URI and try retrieving the file again."));
  // hide advanced options
  popts.setShownCount(3);
}

static MediaChangeReport::Action
handle_common_options(unsigned reply, Url & url)
{
  MediaChangeReport::Action action = MediaChangeReport::ABORT;
  switch (reply)
  {
    case 0: /* abort */
      break;
    case 3: /* change url */
    {
      // translators: this is a prompt label, will appear as "New URI: "
      Url newurl(get_text(_("New URI") + string(": "), url.asString()));
      url = newurl;
    }
    case 1: /* retry */
      action = MediaChangeReport::RETRY;
      break;
    case 2: /* ignore */
      action = MediaChangeReport::IGNORE;
    default:
      WAR << "invalid prompt reply: " << reply << endl;
  }
  return action;
}


// ---------[ https ]--------------------------------------------------------

static MediaChangeReport::Action
request_medium_https_handler(Zypper & zypper, Url & url)
{
  // https options
  // translators: a/r/i/u are replies to the "Abort, retry, ignore?" prompt.
  // Translate the a/r/i part exactly as you did the a/r/i string.
  // The 'u' reply means 'Change URI'.
  // https protocol-specific options:
  // 's' stands for Disable SSL certificate authority check
  PromptOptions popts(_("a/r/i/u/s"), 0);
  set_common_option_help(popts);
  popts.setOptionHelp(4, _("Disable SSL certificate authority check and continue."));

  // translators: this is a prompt text
  zypper.out().prompt(PROMPT_ARI_MEDIA_PROBLEM, _("Abort, retry, ignore?"), popts);
  int reply = get_prompt_reply(zypper, PROMPT_ARI_MEDIA_PROBLEM, popts);

  MediaChangeReport::Action action;
  switch (reply)
  {
    case 4: /* disable SSL */
      url.setQueryParam("ssl_verify", "no");
      zypper.out().info(_("SSL certificate authority check disabled."));
      action = MediaChangeReport::RETRY;
      break;
    default:
      action = handle_common_options(reply, url);
  }
  return action;
}


// ---------[ cd/dvd ]---------------------------------------------------------

static void eject_drive_dialog(
    Zypper & zypper,
    Url & url,
    const vector<string> & devices,
    unsigned int & index)
{
  bool cancel = false;
  if (devices.empty())
  {
    zypper.out().info(_("No devices detected, cannot eject."));
    zypper.out().info(_("Try to eject the device manually."));
  }
  if (devices.size() == 1)
  {
    MIL << "ejecting " << devices.front() << endl;
    media::MediaManager mm;
    media::MediaAccessId mid = mm.open(url);
    mm.release(mid, devices.front());
  }
  else
  {
    zypper.out().info(_("Detected devices:"));
    int devn = 1;
    ostringstream numbers;
    for_(it, devices.begin(), devices.end())
    {
      // enhancement: we could try to get nicer device names like
      // "DVDRAM GSA-U10N on /dev/sr0"
      cout << devn << "  " << *it << " " << endl;
      numbers << devn << "/";
      ++devn;
    }
    numbers << "c"; // c for cancel

    int devcount = devices.size();
    PromptOptions popts(numbers.str(), 0);
    popts.setOptionHelp(devcount, _("Cancel"));
    zypper.out().prompt(PROMPT_MEDIA_EJECT, _("Select device to eject."), popts);
    int reply = get_prompt_reply(zypper, PROMPT_MEDIA_EJECT, popts);
    if (reply == devcount)
      cancel = true;
    else
    {
      MIL << "ejecting " << devices[reply] << endl;
      media::MediaManager mm;
      media::MediaAccessId mid = mm.open(url);
      mm.release(mid, devices[reply]);
    }
  }

  if (!cancel)
  {
    zypper.out().info(_("Insert the CD/DVD and press ENTER to continue."));
    getchar();
  }
  zypper.out().info(_("Retrying..."));
}


static MediaChangeReport::Action
request_medium_dvd_handler(
    Zypper & zypper,
    zypp::Url & url,
    const vector<string> & devices,
    unsigned int & index)
{
  // cd/dvd options
  // translators: a/r/i/u are replies to the "Abort, retry, ignore?" prompt.
  // Translate the a/r/i part exactly as you did the a/r/i string.
  // The 'u' reply means 'Change URI'.
  // cd/dvd protocol-specific options:
  // 'e' stands for Eject medium
  PromptOptions popts(_("a/r/i/u/e"), 0);
  set_common_option_help(popts);
  popts.setOptionHelp(4, _("Eject medium."));

  // translators: this is a prompt text
  zypper.out().prompt(PROMPT_ARI_MEDIA_PROBLEM, _("Abort, retry, ignore?"), popts);
  int reply = get_prompt_reply(zypper, PROMPT_ARI_MEDIA_PROBLEM, popts);
  MediaChangeReport::Action action;
  switch (reply)
  {
    case 4: /* eject medium */
      eject_drive_dialog(zypper, url, devices, index);
      action = MediaChangeReport::RETRY;
      break;
    default:
      action = handle_common_options(reply, url);
  }
  return action;
}

// ---------------------------------------------------------------------------
// MediaChangeReportReceiver
// ---------------------------------------------------------------------------

MediaChangeReport::Action
ZmartRecipients::MediaChangeReportReceiver::requestMedia(
    zypp::Url &                      url,
    unsigned                         mediumNr,
    const std::string &              label,
    MediaChangeReport::Error         error,
    const std::string &              description,
    const std::vector<std::string> & devices,
    unsigned int &                   index)
{
  Zypper & zypper = *Zypper::instance();

  DBG << "medium problem, url: " << url.asString()
      << ", error " << error
      << ", label '" << label << "', #" << mediumNr << endl;

  zypper.out().error(description);
  if (is_changeable_media(url))
  {
    std::string request = boost::str(boost::format(
        // TranslatorExplanation translate letters 'y' and 'n' to whathever is appropriate for your language.
        // Try to check what answers does zypper accept (it always accepts y/n at least)
        // You can also have a look at the regular expressions used to check the answer here:
        // /usr/lib/locale/<your_locale>/LC_MESSAGES/SYS_LC_MESSAGES
        _("Please insert medium [%s] #%d and type 'y' to continue or 'n' to cancel the operation."))
        % label % mediumNr);
    if (read_bool_answer(PROMPT_YN_MEDIA_CHANGE, request, false))
    {
      zypper.requestExit(false);
      return MediaChangeReport::RETRY;
    }
    else
      return MediaChangeReport::ABORT;
  }

  if (error == MediaChangeReport::IO_SOFT)
  {
    MediaChangeReport::Action default_action = MediaChangeReport::RETRY;
    if (repeat_counter.counter_overrun(url))
      default_action = MediaChangeReport::ABORT;

    MediaChangeReport::Action action = (Action) read_action_ari_with_timeout(
        PROMPT_ARI_MEDIA_PROBLEM, 30, default_action);

    if (action == MediaChangeReport::RETRY)
      zypper.requestExit(false);

    return action;
  }

  Action action = MediaChangeReport::ABORT;
  if (url.getScheme() == "https")
    action = request_medium_https_handler(zypper, url);
  if (url.getScheme() == "cd" || url.getScheme() == "dvd")
    action = request_medium_dvd_handler(zypper, url, devices, index);
  else
  {
    // translators: a/r/i/u are replies to the "Abort, retry, ignore?" prompt
    // Translate the a/r/i part exactly as you did the a/r/i string.
    // the 'u' reply means 'Change URI'.
    PromptOptions popts(_("a/r/i/u"), 0);
    set_common_option_help(popts);

    // translators: this is a prompt text
    zypper.out().prompt(PROMPT_ARI_MEDIA_PROBLEM, _("Abort, retry, ignore?"), popts);
    int reply = get_prompt_reply(zypper, PROMPT_ARI_MEDIA_PROBLEM, popts);

    action = handle_common_options(reply, url);
  }

  // if an rpm download failed and user chose to ignore that, advice to
  // run zypper verify afterwards
  if (action == MediaChangeReport::IGNORE
      && zypper.runtimeData().action_rpm_download
      && !zypper.runtimeData().seen_verify_hint)
    print_verify_hint(Zypper::instance()->out());

  if (action == MediaChangeReport::RETRY)
    zypper.requestExit(false);

  return action;
}

// ---------------------------------------------------------------------------
// DownloadProgressReportReceiver
// ---------------------------------------------------------------------------

//! \todo return false on SIGINT
bool ZmartRecipients::DownloadProgressReportReceiver::progress(
    int value, const Url & uri, double drate_avg, double drate_now)
{
  // don't report more often than 1 second
  time_t now = time(NULL);
  if (now > _last_reported)
    _last_reported = now;
  else
    return true;

  Zypper & zypper = *(Zypper::instance());

  if (zypper.exitRequested())
  {
    DBG << "received exit request" << std::endl;
    return false;
  }

  if (!zypper.runtimeData().raw_refresh_progress_label.empty())
    zypper.out().progress(
      "raw-refresh", zypper.runtimeData().raw_refresh_progress_label);

  if (_be_quiet)
    return true;

  if (zypper.commitData()._commit_running)
  {
    string filename = Pathname(uri.getPathName()).basename();
    CommitData::PkgDownloadData & dd =
        zypper.commitData()._dwnld_data[filename];
    dd.percentage = value;
    dd.speed = drate_now;
    zypper.out().commitProgress(zypper.commitData());
  }
  else
    zypper.out().dwnldProgress(uri, value, (long) drate_now);
  _last_drate_avg = drate_avg; // TODO: get rid of this
  return true;
}

// ---------------------------------------------------------------------------

// used only to finish, errors will be reported in media change callback (libzypp 3.20.0)
void ZmartRecipients::DownloadProgressReportReceiver::finish(
    const Url & uri, Error error, const string & konreason )
{
  if (_be_quiet)
    return;

  Zypper & zypper = *Zypper::instance();

  string filename = Pathname(uri.getPathName()).basename();
  CommitData::PkgDownloadData & dd =
      zypper.commitData()._dwnld_data[filename];
  dd.percentage = 100;
  dd.speed = _last_drate_avg;
  // TODO: out.commitData(cp)? probably not, will be reported by repo.h

  if (!zypper.commitData()._commit_running)
    zypper.out().dwnldProgressEnd(uri, _last_drate_avg, error != NO_ERROR);
}
