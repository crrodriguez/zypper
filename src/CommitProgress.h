/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file CommitProgress.h
 * 
 */

#ifndef ZYPPER_COMMITPROGRESS_H_
#define ZYPPER_COMMITPROGRESS_H_

#include <map>
#include <string>

#include "zypp/ByteCount.h"

struct CommitProgress
{
  struct DownloadData
  {
    DownloadData() : speed(0), percentage(0) {}
    zypp::ByteCount speed;
    int percentage;
  };


  CommitProgress() { reset(0,0,0); }
  CommitProgress(
      unsigned pkgs_to_get, unsigned bytes_to_get, unsigned pkgs_to_inst)
  { reset(pkgs_to_get, bytes_to_get, pkgs_to_inst); }
  ~CommitProgress() {}

  void reset(unsigned pkgs_to_get, unsigned bytes_to_get, unsigned pkgs_to_inst);
  void get_eta();

  std::string eta_as_string() const;


  // *** Commit constants

  //! Total number of packages that need to be retrieved.
  unsigned _count_to_get;
  //! Total number of bytes to retrieve.
  zypp::ByteCount _bytes_to_get;

  //! Total number of packages that need to be installed.
  unsigned _count_to_install;


  // *** Overall progress

  //! Overall progress percentage
  short int _percentage;
  //! Estimated time left to completion in seconds.
  int _eta;


  // *** Individual action progress

  //! Numbers of packages being currently retrieved (can retrieve multiple
  //! simultaneously) and their respective download data.
  std::map<unsigned, DownloadData> _get_data;

  //! Number of the package being currently installed.
  unsigned _inst_nr;
  //! Progress of installation of current package.
  int _inst_percent;
};


#endif /* ZYPPER_COMMITPROGRESS_H_ */
