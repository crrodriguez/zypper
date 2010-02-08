/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#ifndef ZMART_MEDIA_CALLBACKS_H
#define ZMART_MEDIA_CALLBACKS_H

#include <stdlib.h>
#include <ctime>
#include <iostream>

#include <boost/format.hpp>

#include "zypp/ZYppCallbacks.h"
#include "zypp/base/Logger.h"
#include "zypp/Pathname.h"
#include "zypp/Url.h"

#include "Zypper.h"
#include "utils/prompt.h"


#define REPEAT_LIMIT 3

using zypp::media::MediaChangeReport;
using zypp::media::DownloadProgressReport;

///////////////////////////////////////////////////////////////////
namespace ZmartRecipients
{
  class repeat_counter_ {
    private:
      zypp::Url url;
      unsigned counter;
    public:
      repeat_counter_():counter(0){}
      bool counter_overrun(const zypp::Url & u){
        if (u==url)
        {
          if (++counter==REPEAT_LIMIT)
            return true;
        }
        else
        {
          url = u;
          counter = 0;
        }
        return false;
      }
  };

  struct MediaChangeReportReceiver : public zypp::callback::ReceiveReport<MediaChangeReport>
  {
    virtual MediaChangeReport::Action
    requestMedia(zypp::Url & url,
                 unsigned                         mediumNr,
                 const std::string &              label,
                 MediaChangeReport::Error         error,
                 const std::string &              description,
                 const std::vector<std::string> & devices,
                 unsigned int &                   index);
    private:
      repeat_counter_ repeat_counter;
  };

  // progress for downloading a file
  struct DownloadProgressReportReceiver
    : public zypp::callback::ReceiveReport<zypp::media::DownloadProgressReport>
  {
    DownloadProgressReportReceiver()
      : _gopts(Zypper::instance()->globalOpts()), _be_quiet(false)
    {}

    virtual void start( const zypp::Url & uri, zypp::Pathname localfile )
    {
      _last_reported = time(NULL);
      _last_drate_avg = -1;

      Zypper & zypper = *Zypper::instance();

      // don't normally report download progress, only if --verbose
      // downloads while committing are reported separately
      if (zypper.out().verbosity() < Out::HIGH)
        _be_quiet = true;
      else
        _be_quiet = false;

      if (!zypper.commitData()._commit_running && !_be_quiet)
        zypper.out().dwnldProgressStart(uri);
    }

    virtual bool progress(int value, const zypp::Url & uri, double drate_avg, double drate_now);

    virtual DownloadProgressReport::Action
    problem( const zypp::Url & uri, DownloadProgressReport::Error error, const std::string & description )
    {
      DBG << "media problem" << std::endl;
      if (!_be_quiet)
        Zypper::instance()->out().dwnldProgressEnd(uri, _last_drate_avg, true);
      Zypper::instance()->out().error(zcb_error2str(error, description));

      Action action = (Action) read_action_ari(
          PROMPT_ARI_MEDIA_PROBLEM, DownloadProgressReport::ABORT);
      if (action == DownloadProgressReport::RETRY)
        Zypper::instance()->requestExit(false);
      return action;
    }

    // used only to finish, errors will be reported in media change callback (libzypp 3.20.0)
    virtual void finish( const zypp::Url & uri, Error error, const std::string & konreason );

  private:
    const GlobalOptions & _gopts;
    bool _be_quiet;
    time_t _last_reported;
    double _last_drate_avg;
  };


  struct AuthenticationReportReceiver : public zypp::callback::ReceiveReport<zypp::media::AuthenticationReport>
  {
    virtual bool prompt(const zypp::Url & url,
                        const std::string & description,
                        zypp::media::AuthData & auth_data)
    {
      if (Zypper::instance()->globalOpts().non_interactive)
      {
        MIL << "Non-interactive mode: aborting" << std::endl;
        return false;
      }

      // curl authentication
      zypp::media::CurlAuthData * curl_auth_data =
        dynamic_cast<zypp::media::CurlAuthData*> (&auth_data);

      if (curl_auth_data)
        curl_auth_data->setAuthType("basic,digest");

      // user name

      std::string username;
      // expect the input from machine on stdin
      if (Zypper::instance()->globalOpts().machine_readable)
      {
        Zypper::instance()->out().prompt(
            PROMPT_AUTH_USERNAME, _("User Name"), PromptOptions(), description);
        std::cin >> username;
      }
      // input from human using readline
      else
      {
        std::cout << description << std::endl;
        username = get_text(_("User Name") + std::string(": "), auth_data.username());
      }
      if (username.empty())
        return false;
      auth_data.setUsername(username);

      // password

      Zypper::instance()->out().prompt(
          PROMPT_AUTH_PASSWORD, _("Password"), PromptOptions());

      std::string password;
      // expect the input from machine on stdin
      if (Zypper::instance()->globalOpts().machine_readable)
        std::cin >> password;
      else
        password = get_password();
      if (password.empty())
        return false;
      auth_data.setPassword(password);

      return true;
    }
  };

    ///////////////////////////////////////////////////////////////////
}; // namespace ZmartRecipients
///////////////////////////////////////////////////////////////////

class MediaCallbacks {

  private:
    ZmartRecipients::MediaChangeReportReceiver _mediaChangeReport;
    ZmartRecipients::DownloadProgressReportReceiver _mediaDownloadReport;
    ZmartRecipients::AuthenticationReportReceiver _mediaAuthenticationReport;
  public:
    MediaCallbacks()
    {
      MIL << "Set media callbacks.." << std::endl;
      _mediaChangeReport.connect();
      _mediaDownloadReport.connect();
      _mediaAuthenticationReport.connect();
    }

    ~MediaCallbacks()
    {
      _mediaChangeReport.disconnect();
      _mediaDownloadReport.disconnect();
      _mediaAuthenticationReport.disconnect();
    }
};

#endif
// Local Variables:
// mode: c++
// c-basic-offset: 2
// End:
