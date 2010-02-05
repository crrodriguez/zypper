/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file CommitProgress.cc
 * 
 */

#include "CommitProgress.h"

#include "zypp/base/String.h"

using namespace zypp;
using namespace std;

void CommitProgress::reset(
    unsigned pkgs_to_get, unsigned bytes_to_get, unsigned pkgs_to_inst)
{
  _count_to_get     = pkgs_to_get;
  _bytes_to_get     = ByteCount(bytes_to_get);
  _count_to_install = pkgs_to_inst;

  _percentage       = -1;
  _eta              = -1;

  _get_data.clear();
  _inst_nr          = 0;
}

string CommitProgress::eta_as_string() const
{
  if (_eta < 0)
    return "N/A";
  if (_eta < 60)
    return str::form("0:%2d", _eta);
  return str::form("%d:%2d", _eta/60, _eta%60);
}
