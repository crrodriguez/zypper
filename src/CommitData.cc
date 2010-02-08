/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file CommitData.cc
 * 
 */

#include "CommitData.h"

#include <iostream>
//#include "zypp/base/Easy.h"

#include "zypp/base/String.h"

using namespace zypp;
using namespace std;

void CommitData::reset(
    unsigned pkgs_to_get, unsigned bytes_to_get, unsigned pkgs_to_inst)
{
  _pkgs_to_get     = pkgs_to_get;
  _bytes_to_get     = ByteCount(bytes_to_get);
  _pkgs_to_install = pkgs_to_inst;

  _percentage       = -1;
  _eta              = -1;

  _dwnld_data.clear();
  _dwnld_data_done.clear();
  _last_dwnld_nr    = 0;
  _inst_nr          = 0;
}

void CommitData::markDownloadDone(
    const std::string & id, const zypp::ByteCount & rate)
{
  _dwnld_data_done[id] = _dwnld_data[id];
  _dwnld_data_done[id].percentage = 100;
  _dwnld_data.erase(id);
}

string CommitData::eta_as_string() const
{
  if (_eta < 0)
    return "N/A";
  if (_eta < 60)
    return str::form("0:%2d", _eta);
  return str::form("%d:%2d", _eta/60, _eta%60);
}

void CommitData::dumpTo(ostream & out) const
{
  out << "--- CommitData start ---" << endl;
  out << "total:   " << _pkgs_to_get << "/" << _pkgs_to_install << endl;
  out << "current: " << _last_dwnld_nr << "/" << _inst_nr << endl;
  out << "getting: " << _dwnld_data.size() << endl;
  for_(it, _dwnld_data.begin(), _dwnld_data.end())
    out << "  " << it->first << " [" << it->second.percentage << "%]" << endl;
  out << "--- CommitData end ---" << endl;
}
