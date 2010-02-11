/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#include <iostream>
#include <sstream>

#include <unistd.h>

#include "zypp/Pathname.h"
#include "zypp/ByteCount.h" // for download progress reporting
#include "zypp/base/String.h" // for toUpper()
#include "zypp/base/Logger.h" // for toUpper()

#include "main.h"
#include "utils/colors.h"
#include "AliveCursor.h"
#include "CommitData.h"

#include "OutNormal.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;

using namespace zypp::str;

OutNormal::OutNormal(Verbosity verbosity)
  : Out(TYPE_NORMAL, verbosity)
  , _use_colors (false)
  , _isatty     (isatty(STDOUT_FILENO))
  , _newline    (true)
  , _ow_lines   (0)
{}

OutNormal::~OutNormal()
{

}

bool OutNormal::mine(Type type)
{
  // Type::TYPE_NORMAL is mine
  if (type & Out::TYPE_NORMAL)
    return true;
  return false;
}

bool OutNormal::infoWarningFilter(Verbosity verbosity, Type mask)
{
  if (!mine(mask))
    return true;
  if (this->verbosity() < verbosity)
    return true;
  return false;
}

void OutNormal::info(const std::string & msg, Verbosity verbosity, Type mask)
{
  if (infoWarningFilter(verbosity, mask))
    return;

  if (!_newline)
    cout << endl;

  if (verbosity == Out::QUIET)
    print_color(msg, COLOR_CONTEXT_RESULT);
  else
    print_color(msg, COLOR_CONTEXT_MSG_STATUS);

  cout << endl;
  _newline = true;
}

void OutNormal::warning(const std::string & msg, Verbosity verbosity, Type mask)
{
  if (infoWarningFilter(verbosity, mask))
    return;

  if (!_newline)
    cout << endl;

  print_color(_("Warning: "), COLOR_CONTEXT_MSG_WARNING);
  cout << msg << endl;
  _newline = true;
}

void OutNormal::error(const std::string & problem_desc, const std::string & hint)
{
  if (!_newline)
    cout << endl;

  fprint_color(cerr, problem_desc, COLOR_CONTEXT_MSG_ERROR);
  if (!hint.empty() && this->verbosity() > Out::QUIET)
    cerr << endl << hint;
  cerr << endl;
  _newline = true;
}

// ----------------------------------------------------------------------------

void OutNormal::error(const zypp::Exception & e,
                      const string & problem_desc,
                      const string & hint)
{
  if (!_newline)
    cout << endl;

  // problem
  fprint_color(cerr, problem_desc, COLOR_CONTEXT_MSG_ERROR);
  cerr << endl;
  // cause
  fprint_color(cerr, zyppExceptionReport(e), COLOR_CONTEXT_MSG_ERROR);
  cerr << endl;

  // hint
  if (!hint.empty() && this->verbosity() > Out::QUIET)
    cerr << hint << endl;

  _newline = true;
}

// ----------------------------------------------------------------------------

void OutNormal::displayProgress (const string & s, int percent)
{
  static AliveCursor cursor;

  if (_isatty)
  {
    cout << CLEARLN << s << " [";
    // dont display percents if invalid
    if (percent >= 0 && percent <= 100)
      cout << percent << "%";
    else
      cout << ++cursor;
    cout << "]";
  }
  else
    cout << '.';
  cout << std::flush;
}

// ----------------------------------------------------------------------------

void OutNormal::displayTick (const string & s)
{
  static AliveCursor cursor;

  if (_isatty)
  {
    cout << CLEARLN << s << " [" << ++cursor << "]";
    cout << std::flush;
  }
  else
    cout << '.' << std::flush;
}

// ----------------------------------------------------------------------------

void OutNormal::progressStart(const std::string & id,
                              const std::string & label,
                              bool is_tick)
{
  if (progressFilter())
    return;

  if (!_isatty)
    cout << label << " [";

  if (is_tick)
    displayTick(label);
  else
    displayProgress(label, 0);

  _newline = false;
}

void OutNormal::progress(const std::string & id, const string & label, int value)
{
  if (progressFilter())
    return;

  if (value)
    displayProgress(label, value);
  else
    displayTick(label);

  _newline = false;
}

void OutNormal::progressEnd(const std::string & id, const string & label, bool error)
{
  if (progressFilter())
    return;

  if (!error && _use_colors)
    cout << get_color(COLOR_CONTEXT_MSG_STATUS);

  if (_isatty)
    cout << CLEARLN << label << " [";

  if (error)
    print_color(_("error"), COLOR_CONTEXT_NEGATIVE);
  else
    cout << _("done");

  cout << "]";

  if (!error && _use_colors)
    cout << COLOR_RESET;

  cout << endl << std::flush;
  _newline = true;
}

// progress with download rate
void OutNormal::dwnldProgressStart(const zypp::Url & uri)
{
  if (verbosity() < NORMAL)
    return;

  if (_isatty)
    cout << CLEARLN;
  cout << _("Retrieving:") << " ";
  if (verbosity() == DEBUG)
    cout << uri; //! \todo shorten to fit the width of the terminal
  else
    cout << zypp::Pathname(uri.getPathName()).basename();
  if (_isatty)
    cout << " [" << _("starting") << "]"; //! \todo align to the right
  else
    cout << " [" ;

  cout << std::flush;
  _newline = false;
}

void OutNormal::dwnldProgress(const zypp::Url & uri,
                              int value,
                              long rate)
{
  if (verbosity() < NORMAL)
    return;

  if (!isatty(STDOUT_FILENO))
  {
    cout << '.' << std::flush;
    return;
  }

  cout << CLEARLN << _("Retrieving:") << " ";
  if (verbosity() == DEBUG)
    cout << uri; //! \todo shorten to fit the width of the terminal
  else
    cout << zypp::Pathname(uri.getPathName()).basename();
  // dont display percents if invalid
  if ((value >= 0 && value <= 100) || rate >= 0)
  {
    cout << " [";
    if (value >= 0 && value <= 100)
      cout << value << "%";
    if (rate >= 0)
      cout << " (" << zypp::ByteCount(rate) << "/s)";
    cout << "]";
  }

  cout << std::flush;
  _newline = false;
}

void OutNormal::dwnldProgressEnd(const zypp::Url & uri, long rate, bool error)
{
  if (verbosity() < NORMAL)
    return;

  if (!error && _use_colors)
    cout << get_color(COLOR_CONTEXT_MSG_STATUS);

  if (_isatty)
  {
    cout << CLEARLN << _("Retrieving:") << " ";
    if (verbosity() == DEBUG)
      cout << uri; //! \todo shorten to fit the width of the terminal
    else
      cout << zypp::Pathname(uri.getPathName()).basename();
    cout << " [";
    if (error)
      print_color(_("error"), COLOR_CONTEXT_NEGATIVE);
    else
      cout << _("done");
  }
  else
    cout << (error ? _("error") : _("done"));

  if (rate >= 0)
    cout << " (" << zypp::ByteCount(rate) << "/s)";
  cout << "]";

  if (!error && _use_colors)
    cout << COLOR_RESET;

  cout << endl << std::flush;
  _newline = true;
}

static string cursor_up(unsigned lines)
{ return zypp::str::form("\x1B[%dA\r\x1B[J", lines); }

void OutNormal::commitProgress(const CommitData & cd)
{
  cd.dumpTo(INT);

  // move the cursor up to update variable status lines
  // clear the rest of the screen
  if (_ow_lines)
    cout << cursor_up(_ow_lines) << CLEARLN;
  // cout << endl << "       < - - - - - - - - " << endl;

  // *** recently finished ***

  // downloads

  for_(dd, cd._dwnld_data_done.begin(), cd._dwnld_data_done.end())
  {
    cout << "Retrieved " << dd->first;
    if (dd->second.speed > 0)
      cout << " [" << dd->second.speed << "/s]";
    cout << endl;
    // todo special info in case of delta rpm
  }

  // installs

  if (!cd._inst_done.empty())
  {
    cout << zypp::str::form(_("Installed %s-%s"),
        cd._inst_done.pkg->name().c_str(), cd._inst_done.pkg->edition().c_str()) << endl;
    if (!cd._inst_done.info_msg.empty())
      cout << cd._inst_done.info_msg << endl;
  }

  // *** currently processed ***

  // downloads

  _ow_lines = 0;
  for_(dd, cd._dwnld_data.begin(), cd._dwnld_data.end())
  {
    if (dd->second.hasDelta() && dd->second.delta_dwnld_pg != 100)
    {
      cout << "Retrieving " << dd->second.delta_filename
          << " [" << dd->second.delta_dwnld_pg << "% (" << dd->second.speed << "/s)]" << endl;
    }
    else if (dd->second.delta_dwnld_pg != 100 && dd->second.delta_dwnld_pg != -1)
    {
      cout << "Applying " << dd->second.delta_filename
          << " [" << dd->second.delta_apply_pg << "%]" << endl;
    }
    else
    {
      cout << "Retrieving " << dd->first
          << " [" << dd->second.percentage << "% (" << dd->second.speed << "/s)]" << endl;
    }
    ++_ow_lines;
  }

  // install

  if (!cd._inst.empty())
  {
    // translators: this is a progress display label e.g. "Installing foo-1.1.2 [42%]"
    cout << zypp::str::form(_("Installing: %s-%s"),
        cd._inst.pkg->name().c_str(), cd._inst.pkg->edition().c_str()) << endl;
    ++_ow_lines;
  }

  // write overall progress
  //
  // retrieving 1 of 10                          1% ETA -:--
  // retrieving 2,3 of 5; installing 1 of 10    10% ETA 1:02
  // installing 10 of 10                        99% ETA 0:01

  string overall;
  if (!cd._dwnld_data.empty())
  {
    ostringstream sstr;
    std::map<string, CommitData::PkgDownloadData>::const_iterator it =
        cd._dwnld_data.begin();
    sstr << it->second.seq_number;
    for (++it; it != cd._dwnld_data.end(); ++it)
      sstr << "," << it->second.seq_number;

    // retrieving 1 of 10                          1% ETA -:--
    if (cd._inst.empty())
    {
      overall = form(_("Retrieving %s of %d"),
          sstr.str().c_str(), cd._pkgs_to_get);
    }
    // retrieving 2,3 of 5; installing 1 of 10    10% ETA 1:02
    else
    {
      overall = form(_("Retrieving %s of %d; installing %d of %d"),
          sstr.str().c_str(), cd._pkgs_to_get, cd._inst.seq_number, cd._pkgs_to_install);
    }
  }
  // installing 10 of 10                        99% ETA 0:01
  else if (!cd._inst.empty())
  {
    overall = form(_("Installing %d of %d"),
        cd._inst.seq_number, cd._pkgs_to_install);
  }

  if (!overall.empty())
    cout << overall << " 0% ETA -:--" << std::flush;
  _newline = false;
}


void OutNormal::prompt(PromptId id,
                       const string & prompt,
                       const PromptOptions & poptions,
                       const std::string & startdesc)
{
  if (!_newline)
    cout << endl;

  if (startdesc.empty())
  {
    if (_isatty)
      cout << CLEARLN;
  }
  else
    cout << startdesc << endl;
  cout << prompt;
  if (!poptions.empty())
    cout << " " << poptions.optionString();
  cout << ": " << std::flush;
  // prompt ends with newline (user hits <enter>) unless exited abnormaly
  _newline = true;
}

void OutNormal::promptHelp(const PromptOptions & poptions)
{
  cout << endl;
  if (poptions.helpEmpty())
    cout << _("No help available for this prompt.") << endl;
  else
  {
    unsigned int pos = 0;
    for(PromptOptions::StrVector::const_iterator it = poptions.options().begin();
        it != poptions.options().end(); ++it, ++pos)
    {
      if (poptions.isDisabled(pos))
        continue;
      cout << *it << " - ";
      const string & hs_r = poptions.optionHelp(pos);
      if (hs_r.empty())
        cout << "(" << _("no help available for this option") << ")";
      else
        cout << hs_r;
      cout << endl;
    }
  }

  cout << endl << poptions.optionString() << ": " << std::flush;
  // prompt ends with newline (user hits <enter>) unless exited abnormaly
  _newline = true;
}
