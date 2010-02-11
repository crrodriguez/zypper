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
#include "zypp/Package.h"

struct CommitData
{
  struct PkgProgressData
  {
    PkgProgressData() : seq_number(0), percentage(0) {}
    bool empty() const { return !seq_number; }
    bool done()  const { return percentage == 100; }

    zypp::Package::constPtr pkg;
    //! Number of the package in the commit sequence.
    unsigned seq_number;
    //! Download progress percentage.
    int percentage;
    std::string error_msg;
    //! e.g. for additional rpm output
    std::string info_msg;
  };

  struct PkgDownloadData : PkgProgressData
  {
    PkgDownloadData() :
      speed(0), avg_speed(0), delta_dwnld_pg(-1), delta_apply_pg(-1)
    {}

    bool hasDelta() const
    { return delta_dwnld_pg >= 0 && !delta_filename.empty(); }

    //! Amount of bytes to get.
    zypp::ByteCount size;
    //! Current download speed.
    zypp::ByteCount speed;
    //! Current (and final) average download speed.
    zypp::ByteCount avg_speed;

    std::string delta_filename;
    zypp::ByteCount delta_size;
    int delta_dwnld_pg;
    int delta_apply_pg;
  };


  // --------------------------------------------------------------------------

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

  // --------------------------------------------------------------------------

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

  //! Filenames of packages being currently retrieved (can retrieve multiple
  //! simultaneously) and their respective download data.
  std::map<std::string, PkgDownloadData> _dwnld_data;
  //! Filenames of packages being currently retrieved (can retrieve multiple
  //! simultaneously) and their respective download data.
  std::map<std::string, PkgDownloadData> _dwnld_data_done;
  //! The last package number that started downloading.
  unsigned _last_dwnld_nr;

  //! Data of the package being installed
  PkgProgressData _inst;
  //! Data of the last package which was installed
  PkgProgressData _inst_done;
  //! The last package number that started installing
  unsigned _last_inst_nr;
};


#endif /* ZYPPER_COMMITPROGRESS_H_ */
