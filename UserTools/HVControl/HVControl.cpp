#include "HVControl.h"
#include <typeinfo>

HVControl::HVControl():Tool(){}


bool HVControl::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  HVControl::configfile = configfile;

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  std::string jsonfile;
  m_variables.Get("JSON_file", jsonfile);
  RunDB db;
  db.addFile(jsonfile);
  HVControl::db = db;
  RunTable hv = db.getTable("Supply", "HV0");  //fix to not have to specify "HV0"

  HVControl::sys_index = hv.getIndex();

  CAENHV_SYSTEM_TYPE_t sysType;
  std::string system = hv["system"].cast<std::string>();
  if (system=="SY5527") sysType = SY5527;

  int link = hv["LinkType"].cast<int>();
  std::string IP = hv["IP"].cast<std::string>().data();
  std::string username = hv["Username"].cast<std::string>();
  std::string passwd = hv["Password"].cast<std::string>();

  char arg[30];
  strcpy(arg, IP.data());

  CAENHVRESULT ret;
  int handle;

  ret = CAENHV_InitSystem(sysType, link, arg, username.data(), passwd.data(), &handle);
  if (ret) {
    std::cerr<<"Error connecting to HV system: "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
    return false;
  } else {std::cout<<"Successfully connected to: "<<hv.getIndex()<<std::endl;}

  HVControl::handle = handle;
  bool temp = HVControl::GetCrateMap();
  if (!temp) {
    std::cout<<"Error while getting crate map"<<std::endl;
    return false;
  }

  //Create ZMQ socket
  HVControl::context = new zmq::context_t(1);
  HVControl::socket = new zmq::socket_t(*context, ZMQ_PUB);
  m_variables.Get("address", HVControl::address);
  HVControl::socket->bind(HVControl::address.c_str());

  HVControl::PrevSendTime = HVControl::get_time();

  m_data->SC_vars.InitThreadedReceiver(m_data->context, 8123, 200, true);

  m_data->SC_vars.Add("Read_HVConfig", SlowControlElementType(BUTTON));
  m_data->SC_vars["Read_HVConfig"]->SetValue(false);

  m_data->SC_vars.Add("Config_HV", SlowControlElementType(BUTTON));
  m_data->SC_vars["Config_HV"]->SetValue(false);

  m_data->SC_vars.Add("PowerOn_HV", SlowControlElementType(BUTTON));
  m_data->SC_vars["PowerOn_HV"]->SetValue(false);

  m_data->SC_vars.Add("PowerOff_HV", SlowControlElementType(BUTTON));
  m_data->SC_vars["PowerOff_HV"]->SetValue(false);

  HVControl::config_read==false;
  HVControl::configed=false;

  return true;
}


bool HVControl::Execute(){
  bool temp;

  if (m_data->SC_vars["Read_HVConfig"]->GetValue<bool>()) {
    temp = HVControl::Read_Config();
    if (!temp) {
      std::cout<<"Something went wrong while reading config file"<<std::endl;
      m_data->SC_vars["Read_HVConfig"]->SetValue(false);
      return false;
    }
    HVControl::config_read=true;
    m_data->SC_vars["Read_HVConfig"]->SetValue(false);
  }

  if (m_data->SC_vars["Config_HV"]->GetValue<bool>()) {
    if (!HVControl::config_read) {
      std::cout<<"Must read config before setting HV parameters"<<std::endl;
      m_data->SC_vars["Config_HV"]->SetValue(false);
      return true;
    }
    temp = HVControl::Configure();
    if (!temp) {
      std::cout<<"Something went wrong while configuring HV"<<std::endl;
      m_data->SC_vars["Config_HV"]->SetValue(false);
      return false;
    }
    HVControl::configed=true;
    HVControl::config_read=false;
    std::cout<<"HV configured"<<std::endl;
    m_data->SC_vars["Config_HV"]->SetValue(false);
  }

  if (m_data->SC_vars["PowerOn_HV"]->GetValue<bool>()) {
    if (!HVControl::configed) {
      std::cout<<"Must configure HV before powering on/off"<<std::endl;
      m_data->SC_vars["PowerOn_HV"]->SetValue(false);
      return true;
    }
    temp = HVControl::PowerOn();
    if (temp){std::cout<<"Turned on channels!"<<std::endl;}
    else {
      std::cout<<"Something went wrong turning on channels"<<std::endl;
      m_data->SC_vars["PowerOn_HV"]->SetValue(false);
      return false;
    }
    m_data->SC_vars["PowerOn_HV"]->SetValue(false);
  }

  if (m_data->SC_vars["PowerOff_HV"]->GetValue<bool>()) {
    if (!HVControl::configed) {
      std::cout<<"Must configure HV before powering on/off"<<std::endl;
      m_data->SC_vars["PowerOff_HV"]->SetValue(false);
      return true;
    }
    temp = HVControl::PowerOff();
    if (!temp) {
      std::cout<<"Something went wrong while turning off channels"<<std::endl;
      m_data->SC_vars["PowerOff_HV"]->SetValue(false);
      return false;
    }
    else {std::cout<<"Turned off channels"<<std::endl;}
    m_data->SC_vars["PowerOff_HV"]->SetValue(false);
  }

  int handle = HVControl::handle;
  uint64_t CurrentTime, ElapsedTime;

  CAENHVRESULT ret;
  std::string prop = "SwRelease";
  char result[30];
  ret = CAENHV_GetSysProp(handle, prop.data(), result);
  if (ret) {
    std::cerr<<"Error in Keep Alive Loop: "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
    return false;
  }

  CurrentTime = HVControl::get_time();
  ElapsedTime = CurrentTime - HVControl::PrevSendTime;
  if (ElapsedTime>1000) {
    HVControl::BuildMonitorData();
    HVControl::PrevSendTime = HVControl::get_time();
  }
  return true;
}


bool HVControl::Finalise(){

  int handle = HVControl::handle;
  std::string sys_index = HVControl::sys_index;

  CAENHVRESULT ret;

  bool temp = HVControl::PowerOff();
  if (!temp) {
    std::cout<<"Something went wrong while turning off channels"<<std::endl;
    return false;
  }
  else {std::cout<<"Turned off channels"<<std::endl;}
  usleep(2000000);

  ret = CAENHV_DeinitSystem(handle);
  if (ret) {
    std::cerr<<"Error disconnecting from HV system: "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
    return false;
  }else {std::cout<<"Disconnected from "<<sys_index<<std::endl;}

  //Close ZMQ socket
  std::string stop = "Stop";
  char* zmqBuffer = (char*)malloc(sizeof(stop));
  memcpy(zmqBuffer, &stop, sizeof(stop));
  zmq::message_t message((void*)zmqBuffer, sizeof(stop), 0, 0);
  size_t sent = HVControl::socket->send(message);
  usleep(100000);
  HVControl::socket->close();

  return true;
}

bool HVControl::Read_Config(){

  if(HVControl::configfile!="")  m_variables.Initialise(HVControl::configfile);

  std::string jsonfile;
  m_variables.Get("JSON_file", jsonfile);
  std::cout<<"Reading "<<jsonfile<<"voltage and enable"<<std::endl;
  RunDB db;
  db.addFile(jsonfile);
  HVControl::db = db;

  RunTable hv = db.getTable("Supply", "HV0");
  HVControl::sys_index = hv.getIndex();
  std::string sys_index = HVControl::sys_index;
  unsigned short NrOfSl=HVControl::NrOfSl, *NrOfCh=HVControl::NrOfCh;
  std::string index, ParamName;
  std::vector<std::string> members;
  float fParamVal;

  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();

    for (int j=0; j<NrOfCh[slot]; j++) {
      std::string chname = "CH" + std::to_string(j);
      std::string chindx = sys_index + "-" + bname;
      if (!db.tableExists(chname, chindx)){continue;}
      RunTable ch = db.getTable(chname, chindx);
      index = ch.getIndex();

      members = ch.getMembers();
      for (auto param = members.begin(); param!=members.end(); ++param) {
        ParamName = *param;
        if (ParamName=="enable") {fParamVal = ch[ParamName].cast<bool>();}
        else if (ParamName=="V0Set") {fParamVal = ch[ParamName].cast<int>();}
        else {continue;}
        std::cout<<index<<" "<<chname<<" "<<ParamName<<": "<<fParamVal<<std::endl;

      }
    }
  }
  std::cout<<"Finished reading "<<jsonfile<<std::endl;
  return true;
}

bool HVControl::Configure(){

/*  if(HVControl::configfile!="")  m_variables.Initialise(HVControl::configfile);

  std::string jsonfile;
  m_variables.Get("JSON_file", jsonfile);
  RunDB db;
  db.addFile(jsonfile);
  HVControl::db = db;

  RunTable hv = db.getTable("Supply", "HV0");
  HVControl::sys_index = hv.getIndex();
*/
  int handle = HVControl::handle;
  RunDB db = HVControl::db;
  std::string sys_index = HVControl::sys_index;
  CAENHVRESULT ret;
  unsigned short NrOfSl=HVControl::NrOfSl, *NrOfCh=HVControl::NrOfCh;
  std::string index, ParamName;
  std::vector<std::string> members;
  unsigned short *ChList;
  ChList = (unsigned short*)malloc(1*sizeof(unsigned short));
  float fParamVal;
  float *fParamValList = NULL;
  fParamValList = (float*)malloc(1*sizeof(float));
  unsigned long *lParamValList = NULL;
  lParamValList = (unsigned long*)malloc(1*sizeof(unsigned long));

  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();

    for (int j=0; j<NrOfCh[slot]; j++) {
      std::string chname = "CH" + std::to_string(j);
      std::string chindx = sys_index + "-" + bname;
      if (!db.tableExists(chname, chindx)){continue;}
      RunTable ch = db.getTable(chname, chindx);
      index = ch.getIndex();
      ChList[0] = j;

      members = ch.getMembers();
      for (auto param = members.begin(); param!=members.end(); ++param) {
        ParamName = *param;
        if (ParamName == "name" || ParamName=="index" || ParamName=="enable") {continue;}
        fParamVal = ch[ParamName].cast<int>();
        ret = CAENHV_SetChParam(handle, slot, ParamName.data(), 1, ChList, &fParamVal);
        if (ret) {
          std::cerr<<"Error setting "<<index<<" "<<chname<<" "<<ParamName<<": "<<CAENHV_GetError(handle)<<"Error no. "<<ret<<std::endl;
          return false;
        }/*
        else {
          //usleep(10000);
          usleep(30000);
          ret = CAENHV_GetChParam(handle, slot, ParamName.data(), 1, ChList, fParamValList);
          if (ret) {
            std::cerr<<"Error getting "<<index<<" "<<chname<<" "<<ParamName<<": "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
            return false;
          }
          std::cout<<"Set "<<index<<" "<<chname<<" "<<ParamName<<" to: "<<fParamValList[0]<<std::endl;
        }
      }
//      usleep(100000);
      for (auto param = members.begin(); param!=members.end(); ++param) {
        ParamName = *param;
        if (ParamName == "name" || ParamName=="index" || ParamName=="enable") {continue;}
        ret = CAENHV_GetChParam(handle, slot, ParamName.data(), 1, ChList, fParamValList);
        if (ret) {
          std::cerr<<"Error getting "<<index<<" "<<chname<<" "<<ParamName<<": "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
          return false;
        }
        std::cout<<"Set "<<index<<" "<<chname<<" "<<ParamName<<" to: "<<fParamValList[0]<<std::endl;
*/
      }
    }
  }

  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();

    for (int j=0; j<NrOfCh[slot]; j++) {
      std::string chname = "CH" + std::to_string(j);
      std::string chindx = sys_index + "-" + bname;
      if (!db.tableExists(chname, chindx)){continue;}
      RunTable ch = db.getTable(chname, chindx);
      index = ch.getIndex();
      ChList[0] = j;

      members = ch.getMembers();
      for (auto param = members.begin(); param!=members.end(); ++param) {
        ParamName = *param;
        if (ParamName == "name" || ParamName=="index" || ParamName=="enable") {continue;}
        ret = CAENHV_GetChParam(handle, slot, ParamName.data(), 1, ChList, fParamValList);
        if (ret) {
          std::cerr<<"Error getting "<<index<<" "<<chname<<" "<<ParamName<<": "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
          return false;
        }
        std::cout<<"Set "<<index<<" "<<chname<<" "<<ParamName<<" to: "<<fParamValList[0]<<std::endl;
      }
    }
  }

  return true;
}

// Function will take the socket address as an input?
bool HVControl::BuildMonitorData(){

  int handle = HVControl::handle;
  std::string sys_index = HVControl::sys_index;
  RunDB db = HVControl::db;

  unsigned short NrOfSl, *NrOfCh;
  NrOfSl = HVControl::NrOfSl;
  NrOfCh = HVControl::NrOfCh;
  CAENHVRESULT ret;

  std::string index, ParamName;

  float bd_temp, ch_vmon, ch_imon;
  uint32_t bd_status, ch_status;
  bool ch_pw;

  //time_t timestamp = HVControl::Get_TimeStamp();

// Connect to each board and get board params
  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();

    HVData data;

    //data.timestamp = timestamp;
    data.slot = slot;

    //Get board temp
    bd_temp = HVControl::GetBdParam<float>(slot, "Temp");
    if (bd_temp==-999){
      std::cout<<"Something wrong with board "<<slot<<" temp!"<<std::endl;
      return false;
    }
    data.Temp = bd_temp;

    //Get board status
    bd_status = HVControl::GetBdParam<uint32_t>(slot, "BdStatus");
    if (bd_status==48) {
      std::cout<<"Something wrong with board "<<slot<<" status!"<<std::endl;
      return false;
    }
    data.BdStatus = bd_status%64;

    for (int j=0; j<NrOfCh[slot]; j++) {
      HVData::HVChData& ch = data.channels[j];

      ch.chID = j;

      //Get ch power
      ch_pw = HVControl::GetChParam<float>(slot, j, "Pw");
      if (ch_pw==-999) {
        std::cout<<"Something wrong with board:channel "<<slot<<":"<<j<<" power!"<<std::endl;
        return false;
      }
      ch.Pw = ch_pw;

      //Get ch status
      ch_status = HVControl::GetChParam<uint32_t>(slot, j, "Status");
      if (ch_status==4096) {
        std::cout<<"Something wrong with board:channel "<<slot<<":"<<j<<" status!"<<std::endl;
        return false;
      }
      ch.Status = ch_status;

      //Get ch voltage
      /* if (ch.Pw==0) {
        ch.VMon = 0;
      }
      else {*/
        ch_vmon = HVControl::GetChParam<float>(slot, j, "VMon");
        if (ch_vmon==-999) {
          std::cout<<"Something wrong with board:channel "<<slot<<":"<<j<<" voltage!"<<std::endl;
          return false;
        }
        ch.VMon = ch_vmon;
      //}

      //Get ch current
      /*if (ch.Pw==0) {
        ch.IMon = 0;
      }
      else {*/
        ch_imon = HVControl::GetChParam<float>(slot, j, "IMon");
        if (ch_imon==-999) {
          std::cout<<"Something wrong with board:channel "<<slot<<":"<<j<<" current!"<<std::endl;
          return false;
        }
        ch.IMon = ch_imon;
      //}
    }
    size_t dataSize = sizeof(data);
    char* zmqBuffer = (char*)malloc(dataSize);
    memcpy(zmqBuffer, &data, dataSize);
    zmq::message_t message((void*)zmqBuffer, dataSize, 0, 0);
    size_t sent = HVControl::socket->send(message);
    //if (sent){std::cout<<"Sending HV monitor info"<<std::endl;}

  // Get supply fan status

  //Get HV fan status

  //Get HV fan speed

  //Get power supply status
  }

  return true;
}

// Function to grab bd parameter
template <typename T>
T HVControl::GetBdParam(int slot, std::string paramName) {

  int handle = HVControl::handle;
  CAENHVRESULT ret;
  unsigned short *slotList;
  slotList = (unsigned short*)malloc(1*sizeof(unsigned short));
  slotList[0] = slot;
  float *fParamValList = NULL;
  fParamValList = (float*)malloc(sizeof(float));
  uint32_t *lParamValList = NULL;
  lParamValList = (uint32_t*)malloc(sizeof(uint32_t));

  if (paramName=="BdStatus") {
    ret = CAENHV_GetBdParam(handle, 1, slotList, paramName.data(), lParamValList);
    if (ret) {
      std::cout<<"Error reading board status: "<<ret<<std::endl;
      return 48;
    }
    return lParamValList[0];
  }
  else {
    ret = CAENHV_GetBdParam(handle, 1, slotList, paramName.data(), fParamValList);
    if (ret) {
      std::cout<<"Error reading board temp: "<<ret<<std::endl;
      return -999;
    }
    return fParamValList[0];
  }
}

// Function to grab ch parameter
template <typename T>
T HVControl::GetChParam(int slot, int ch, std::string paramName) {

  int handle = HVControl::handle;
  CAENHVRESULT ret;
  unsigned short *ChList;
  ChList = (unsigned short*)malloc(1*sizeof(unsigned short));
  ChList[0] = ch;
  float *fParamValList = NULL;
  fParamValList = (float*)malloc(sizeof(float));
  uint32_t *lParamValList = NULL;
  lParamValList = (uint32_t*)malloc(sizeof(uint32_t));

  if (paramName == "Status") {
    ret = CAENHV_GetChParam(handle, slot, paramName.data(), 1, ChList, lParamValList);
    if (ret) {
      std::cout<<"Error reading ch status: "<<ret<<std::endl;
      return 4096;
    }
    return lParamValList[0];
  }
  else {
    ret = CAENHV_GetChParam(handle, slot, paramName.data(), 1, ChList, fParamValList);
    if (ret) {
      std::cout<<"Error reading ch "<<paramName<<": "<<ret<<std::endl;
      return -999;
    }
    return fParamValList[0];
  }
}

bool HVControl::PowerOn(){

  int handle = HVControl::handle;
  std::string sys_index = HVControl::sys_index;
  RunDB db = HVControl::db;

  unsigned short NrOfSl, *NrOfCh;
  NrOfSl = HVControl::NrOfSl;
  NrOfCh = HVControl::NrOfCh;
  unsigned short *ChList;
  ChList = (unsigned short*)malloc(1*sizeof(unsigned short));
  uint32_t bParamVal = 1;
  std::string ParamName = "Pw";
  std::vector<int> slots;
  int timeout = 90;
  bool on = false;
  CAENHVRESULT ret;

  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();
    slots.push_back(slot);

    for (int j=0; j<NrOfCh[slot]; j++) {
      std::string chname = "CH" + std::to_string(j);
      std::string chindx = sys_index + "-" + bname;
      if (!db.tableExists(chname, chindx)){continue;}
      RunTable ch = db.getTable(chname, chindx);
      bool enable = ch["enable"].cast<bool>();

      if (enable==true){
        ChList[0] = j;
        ret = CAENHV_SetChParam(handle, slot, ParamName.data(), 1, ChList, &bParamVal);
        if (ret){
          std::cout<<"Error turning on slot:ch "<<slot<<":"<<j<<" error no "<<ret<<std::endl;
          return false;
        }
      }
    }
  }

  //Check for stability
  uint64_t CurrentTime = HVControl::get_time();
  uint64_t ElapsedTimeMon, ElapsedTime = 0;
  std::cout<<"Waiting for HV to stabilize..."<<std::endl;
  while(ElapsedTime < (timeout*1000)){
    bool busy = false;
    for (size_t i=0; i<slots.size(); i++){
      for (int ch=0; ch<NrOfCh[slots[i]]; ch++){
        uint32_t Status = HVControl::GetChParam<uint32_t>(slots[i], ch, "Status");
        busy |= (Status & (1<<1 | 1<<2));
        if (Status>7) {
          std::cout<<"Something wrong with HV slot:ch "<<slots[i]<<":"<<ch<<"! Status: "<<Status<<std::endl;
          return false;
        }
      }
    }

    ElapsedTime += (HVControl::get_time() - CurrentTime);
    CurrentTime = HVControl::get_time();
    ElapsedTimeMon = CurrentTime - HVControl::PrevSendTime;
    if (ElapsedTimeMon>1000) {
      HVControl::BuildMonitorData();
      HVControl::PrevSendTime = HVControl::get_time();
    }
    if (!busy) {
      on = true;
      break;
    }
  }
  if (!on) {std::cout<<"WARNING! PowerOn took longer than "<<timeout<<" seconds. HV may not be stable on all channels."<<std::endl;}
  else{std::cout<<"HV stabilized"<<std::endl;}
  return true;
}

bool HVControl::PowerOff(){

  int handle = HVControl::handle;
  std::string sys_index = HVControl::sys_index;
  RunDB db = HVControl::db;

  unsigned short NrOfSl, *NrOfCh;
  NrOfSl = HVControl::NrOfSl;
  NrOfCh = HVControl::NrOfCh;
  unsigned short *ChList;
  ChList = (unsigned short*)malloc(1*sizeof(unsigned short));
  uint32_t bParamVal = 0;
  std::string ParamName = "Pw";
  std::vector<int> slots;
  bool off = false;
  int timeout = 90;
  CAENHVRESULT ret;

// Turn off all channels
  for (int i=0; i<NrOfSl; i++) {
    std::string bname = "BD" + std::to_string(i);
    if (!db.tableExists(bname, sys_index)){continue;}
    RunTable board = db.getTable(bname, sys_index);
    int slot = board["slot"].cast<int>();
    slots.push_back(slot);

    for (int j=0; j<NrOfCh[slot]; j++) {
      ChList[0] = j;
      ret = CAENHV_SetChParam(handle, slot, ParamName.data(), 1, ChList, &bParamVal);
      if (ret){
        std::cout<<"Error turning off slot:ch "<<slot<<":"<<j<<" error no "<<ret<<std::endl;
        return false;
      }
    }
  }

// Wait for channels to ramp down
  uint64_t CurrentTime = HVControl::get_time();
  uint64_t ElapsedTimeMon, ElapsedTime = 0;
  std::cout<<"Waiting for HV to stabilize at zero..."<<std::endl;
  while(ElapsedTime < (timeout*1000)){
    bool busy = false;
    for (size_t i=0; i<slots.size(); i++){
      for (int ch=0; ch<NrOfCh[slots[i]]; ch++){
        uint32_t Status = HVControl::GetChParam<uint32_t>(slots[i], ch, "Status");
        busy |= (Status & (1 | 1<<2));
        if (Status>7) {
          std::cout<<"Something wrong with HV slot:ch "<<slots[i]<<":"<<ch<<"! Status: "<<Status<<std::endl;
          return false;
        }
      }
    }

    ElapsedTime += (HVControl::get_time() - CurrentTime);
    CurrentTime = HVControl::get_time();
    ElapsedTimeMon = CurrentTime - HVControl::PrevSendTime;
    if (ElapsedTimeMon>1000) {
      HVControl::BuildMonitorData();
      HVControl::PrevSendTime = HVControl::get_time();
    }
    if (!busy) {
      off = true;
      break;
    }
  }

  if (!off) {std::cout<<"WARNING! PowerOff took longer than "<<timeout<<" seconds. HV power may not be off on all channels."<<std::endl;}
// Check voltages and warn if over 10V
  for (size_t i=0; i<slots.size(); i++){
    for (int ch=0; ch<NrOfCh[slots[i]]; ch++){
      float voltage = HVControl::GetChParam<float>(slots[i], ch, "VMon");
      if (voltage > 0.5) std::cout<<"WARNING! slot:ch "<<slots[i]<<":"<<ch<<" voltage is over 0.5V: "<<voltage<<std::endl;
    }
  }
  if (off) {std::cout<<"All HV off"<<std::endl;}
  return true;
}


// Gets current time in milliseconds
long HVControl::get_time(){
  long time_ms;
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  return time_ms;
}

bool HVControl::GetCrateMap(){
  int handle = HVControl::handle;
  std::string sys_index = HVControl::sys_index;
  RunDB db = HVControl::db;

// Crate Map
  unsigned short NrOfSl, *NrOfCh, *SerNumList;
  char *ModelList, *DescriptionList;
  unsigned char *FmwRelMinList, *FmwRelMaxList;
  CAENHVRESULT ret;

  ret = CAENHV_GetCrateMap(handle, &NrOfSl, &NrOfCh, &ModelList, &DescriptionList, &SerNumList,
                                         &FmwRelMinList, &FmwRelMaxList);
  if (ret) {
    std::cerr<<"Error getting crate map: "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
    return false;
  }
  char *m = ModelList;

  for (int i=0; i<NrOfSl; i++, m+=strlen(m)+1){
    if (*m == '\0'){std::cout<<"Board "<<i<<" not present"<<std::endl;}
    else {
      std::cout<<"Board "<<i<<" "<<m<<" Num chan: "<<NrOfCh[i]<<std::endl;
    }
  }

  CAENHV_Free(SerNumList);
  CAENHV_Free(ModelList);
  CAENHV_Free(DescriptionList);
  CAENHV_Free(FmwRelMinList);
  CAENHV_Free(FmwRelMaxList);

  HVControl::NrOfSl = NrOfSl;
  HVControl::NrOfCh = NrOfCh;
//  CAENHV_Free(NrOfCh);

  return true;
}
HVControl::~HVControl(){

  int handle = HVControl::handle;
  CAENHVRESULT ret;
  std::string prop = "SwRelease";
  char result[30];

  ret = CAENHV_GetSysProp(handle, prop.data(), result);

  if (!ret) {
    bool temp = HVControl::PowerOff();
    if (!temp) {
      std::cout<<"Something went wrong while turning off channels"<<std::endl;
    }
    else {std::cout<<"Turned off channels"<<std::endl;}
    usleep(2000000);

    ret = CAENHV_DeinitSystem(handle);
    if (ret) {
      std::cerr<<"Error disconnecting from HV system: "<<CAENHV_GetError(handle)<<" Error no. "<<ret<<std::endl;
    }else {std::cout<<"Disconnected from "<<sys_index<<std::endl;}
  }
}
