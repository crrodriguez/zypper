/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file CommitData.h
 * 
 */

#ifndef ZYPPER_COMMITPROGRESS_H_
#define ZYPPER_COMMITPROGRESS_H_

#include <map>
#include <string>

#include <iosfwd>

#include "zypp/ByteCount.h"

struct CommitData
{
  struct DownloadData
  {
    DownloadData() : speed(0), percentage(0) {}
    //! Number of the package in the commit sequence.
    unsigned seq_number;
    //! Current (and final) download speed.
    zypp::ByteCount speed;
    //! Download progress percentage.
    int percentage;
  };


  CommitData() : _commit_running(false) { reset(0, 0, 0); }

  CommitData(
      unsigned pkgs_to_get,
      unsigned bytes_to_get,
      unsigned pkgs_to_inst)
    : _commit_running(false)
  { reset(pkgs_to_get, bytes_to_get, pkgs_to_inst); }

  ~CommitData() {}

  void reset(
      unsigned pkgs_to_get,
      unsigned bytes_to_get,
      unsigned pkgs_to_inst);
  void setCommitRunning(bool value = true) { _commit_running = value; }

  void markDownloadDone(const std::string & id, const zypp::ByteCount & rate = 0);

  void get_eta();

  std::string eta_as_string() const;

  void dumpTo(std::ostream & out) const;

  bool _commit_running;

  // *** Commit constants

  //! Total number of packages that need to be retrieved.
  unsigned _pkgs_to_get;
  //! Total number of bytes to retrieve.
  zypp::ByteCount _bytes_to_get;

  //! Total number of packages that need to be installed.
  unsigned _pkgs_to_install;


  // *** Overall progress

  //! Overall progress percentage
  short int _percentage;
  //! Estimated time left to completion in seconds.
  int _eta;


  // *** Individual action progress

  //! Numbers of packages being currently retrieved (can retrieve multiple
  //! simultaneously) and their respective download data.
  std::map<std::string, DownloadData> _dwnld_data;
  //! Numbers of packages being currently retrieved (can retrieve multiple
  //! simultaneously) and their respective download data.
  std::map<std::string, DownloadData> _dwnld_data_done;
  //! The last package number that started downloading.
  unsigned _last_dwnld_nr;

  //! Number of the package being currently installed.
  unsigned _inst_nr;
  //! Progress of installation of current package.
  int _inst_percent;
};


#endif /* ZYPPER_COMMITPROGRESS_H_ */
