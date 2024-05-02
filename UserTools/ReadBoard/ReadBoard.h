#ifndef ReadBoard_H
#define ReadBoard_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "CAENDigitizer.h"
#include "CAENDigitizerType.h"
#include <sys/time.h>
#include <ctime>
#include "DataModel.h"
/**
 * \class ReadBoard
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class ReadBoard: public Tool {


 public:

  ReadBoard(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  static long get_time();

  std::string Get_TimeStamp();

  int OpenBoard(Store m_variables);
  bool ConfigureBoard(int handle, Store m_variables);
  bool Start_Acquisition(int handle);
  bool Stop_Acquisition(int handle);

  uint32_t bID;
  uint64_t PrevRateTime, PrevTempTime;
  int handle, verbose, Nb=0, Ne=0, store_temps, event_count=0;
  int file_num=0, ev_per_file=0;
  char* buffer=NULL;
  std::string ModelName, ofile_part;
  bool configed=false, acq_started=false;

  std::ofstream tfile;

 private:





};


#endif
