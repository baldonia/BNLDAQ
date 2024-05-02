#ifndef HVControl_H
#define HVControl_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "json.h"
#include "RunDB.h"
#include "DataModel.h"
#define UNIX
#include "CAENHVWrapper.h"
#include <ctime>

/**
 * \class HVControl
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class HVControl: public Tool {


 public:

  HVControl(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise funciton used to clean up resources.
  ~HVControl();
  bool Configure();
  bool Read_Config();
  bool BuildMonitorData();
  bool PowerOn();
  bool PowerOff();
  bool GetCrateMap();
  long get_time();
//  time_t Get_TimeStamp();

  template <typename T>
  T GetBdParam(int slot, std::string paramName);

  template <typename T>
  T GetChParam(int slot, int ch, std::string paramName);

  int handle;
  std::string sys_index;
  RunDB db;
  unsigned short NrOfSl, *NrOfCh;
  long PrevSendTime;
  bool configed=false;
  bool config_read=false;

  std::string address, configfile;
  zmq::context_t* context;
  zmq::socket_t* socket;

 private:





};

typedef struct HVData {
  typedef struct HVChData {
    int chID;
    float VMon;
    float IMon;
    uint32_t Status;
    bool Pw;
  } HVChData;

  int slot;
  float Temp;
  uint32_t BdStatus;
//  time_t timestamp;

  HVChData channels[48];
} HVData;


#endif
