#include "ReadBoard.h"
#include <math.h>

ReadBoard::ReadBoard():Tool(){}


bool ReadBoard::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  int handle, bID;
//  uint32_t AllocatedSize;
//  char *buffer = NULL;
  CAEN_DGTZ_ErrorCode ret;
/*
  m_data->SC_vars.InitThreadedReceiver(m_data->context, 88888, 200, true);

  m_data->SC_vars.Add("Config_Digitizers", SlowControlElementType(BUTTON));
  m_data->SC_vars["Start_Acquisition"]->SetValue(false);

  m_data->SC_vars.Add("Start_Acquisition", SlowControlElementType(BUTTON));
  m_data->SC_vars["Start_Acquisition"]->SetValue(false);

  m_data->SC_vars.Add("Stop_Acquisition", SlowControlElementType(BUTTON));
  m_data->SC_vars["Stop_Acquisition"]->SetValue(false);
*/
  handle = ReadBoard::OpenBoard(m_variables);
  ReadBoard::handle = handle;

  m_variables.Get("bID", bID);
  ReadBoard::bID = bID;
/*
  bool test = ReadBoard::ConfigureBoard(handle, m_variables);
  if(!test) {
    std::cout<<"Board "<<bID<<" configuration failed!!!"<<std::endl;
    return true;
  }
  else {
    std::cout<<"Board "<<bID<<" configured!"<<std::endl;
  }

  ret = CAEN_DGTZ_MallocReadoutBuffer(handle,&buffer,&AllocatedSize);
  if (ret) {
    std::cout<<"Error allocating readout buffer"<<std::endl;
    return true;
  }

  ReadBoard::buffer = buffer;

  int ev_per_file;
  m_variables.Get("ev_per_file", ev_per_file);
  ReadBoard::ev_per_file = ev_per_file;

  std::string ofile;
  m_variables.Get("ofile", ofile);

  std::string timestamp = ReadBoard::Get_TimeStamp();
  std::string ofile_part;
  ofile_part = ofile + "_" + timestamp;
  ReadBoard::ofile_part = ofile_part;
  std::string ofile_full = ofile_part + "_0.bin";

  if (!m_data->outfile.is_open()) {
    m_data->outfile.open(ofile_full, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
  }

// Temperature file
  int store_temps;
  m_variables.Get("store_temps", store_temps);
  ReadBoard::store_temps = store_temps;
  if (store_temps){
    std::string bdname = ReadBoard::ModelName;
    if (bdname.find("730") != std::string::npos){
      std::string tempfile = ofile_part + "_temps_b" + std::to_string(bID) + ".txt";
      ReadBoard::tfile.open(tempfile, std::ofstream::out);

      ReadBoard::tfile<<"#Each column corresponds to a channel"<<std::endl;
    }
  }

  // Arm all boards except the first
  if (bID != 1) {
    ret = CAEN_DGTZ_WriteRegister(handle, 0x8100, 0x6);
    if (ret) {
      std::cout<<"Error while arming board: "<<ret<<std::endl;
      return false;
    }
    else {std::cout<<"Board "<<bID<<" armed"<<std::endl;}
  }

  // Set run start delays to compensate for daisy chain
  uint32_t delay = 3*(2-bID);
  ret = CAEN_DGTZ_WriteRegister(handle, 0x8170, delay);
  if (ret) {
    std::cout<<"Error while setting run delay: "<<ret<<std::endl;
    return false;
  }
  else {
    uint32_t reg_data;
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8170, &reg_data);
    if (ret) {
      std::cout<<"Error reading run delay: "<<ret<<std::endl;
      return false;
    }
    else {std::cout<<"Set board "<<bID<<" run delay to: "<<((reg_data & 0xff) * 16)<<" ns"<<std::endl;}
  }

  usleep(1000000);
*/
  return true;
}


bool ReadBoard::Execute(){

  int handle=ReadBoard::handle, bID=ReadBoard::bID, Nb=ReadBoard::Nb, Ne=ReadBoard::Ne;
  uint32_t BufferSize, NumEvents;
  uint64_t CurrentTime, ElapsedTime;
  CAEN_DGTZ_ErrorCode ret;
  bool temp;

  if (m_data->config){
    if (ReadBoard::acq_started){
      std::cout<<"Acquisition in progress. Please stop acquisition to reconfigure digitizers."<<std::endl;
      m_data->config=false;
      return true;
    }
    temp = ReadBoard::ConfigureBoard(handle, m_variables);
    if (!temp) {
      std::cout<<"Something went wrong with configuring board "<<bID<<"!"<<std::endl;
      m_data->config=false;
      return false;
    }
    std::cout<<"Board "<<bID<<" configured"<<std::endl;
    ReadBoard::configed=true;
  }

  if (m_data->start_acq){
    if (!ReadBoard::configed){
      std::cout<<"Board must be configured before starting acquisition"<<std::endl;
      m_data->start_acq=false;
      return true;
    }
    if (ReadBoard::acq_started){
      std::cout<<"Acquisition already started. Please stop current run before starting a new one."<<std::endl;
      m_data->start_acq=false;
      return true;
    }
    temp = ReadBoard::Start_Acquisition(handle);
    if (!temp) {
      std::cout<<"Something went wrong with starting acquisition on board "<<bID<<std::endl;
      m_data->start_acq=false;
      return false;
    }
    ReadBoard::acq_started=true;
    std::cout<<"Board "<<bID<<" acquisition started"<<std::endl;
  }

  if (m_data->stop_acq){
    if (!ReadBoard::acq_started){
      std::cout<<"No run to stop. Please start acquisition before stopping acquisition."<<std::endl;
      m_data->stop_acq=false;
      return true;
    }
    temp = ReadBoard::Stop_Acquisition(handle);
    if (!temp) {
      std::cout<<"Something went wrong with stopping acquisition on board "<<bID<<std::endl;
      m_data->stop_acq=false;
      return false;
    }
    ReadBoard::acq_started=false;
    ReadBoard::configed=false;
    std::cout<<"Board "<<bID<<" acquisition stopped"<<std::endl;
  }

  /*
  if (!(ReadBoard::acq_started)) {
    ret = CAEN_DGTZ_SWStartAcquisition(handle);
    if (ret)  {
      std::cout<<"Error starting acquisition"<<std::endl;
      return true;
    }
    std::cout<<"Acquisition started!"<<std::endl;
    ReadBoard::acq_started = 1;
    Nb = 0;
    Ne = 0;
    ReadBoard::PrevRateTime = ReadBoard::get_time();
    ReadBoard::PrevTempTime = ReadBoard::get_time();
  }

  if (!(ReadBoard::acq_started)) {
    if (bID==1) {
      ret = CAEN_DGTZ_WriteRegister(handle, 0x8100, 0x6);
      if (ret)  {
        std::cout<<"Error arming board 1"<<std::endl;
        return false;
      }
      std::cout<<"Armed board 1"<<std::endl;
    }
    ReadBoard::acq_started = 1;
    Nb = 0;
    Ne = 0;
    ReadBoard::PrevRateTime = ReadBoard::get_time();
    ReadBoard::PrevTempTime = ReadBoard::get_time();
  }
*/
  if (ReadBoard::acq_started){
    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, ReadBoard::buffer, &BufferSize);
    if (ret) {
      std::cout<<"ReadData Error: "<<ret<<std::endl;
      return true;
    }

//  if (BufferSize) m_data->outfile.write(ReadBoard::buffer, BufferSize);

    NumEvents = 0;
    if (BufferSize != 0) {
      ret = CAEN_DGTZ_GetNumEvents(handle, ReadBoard::buffer, BufferSize, &NumEvents);
      if (ret) {
        std::cout<<"GetNumEvents Error: "<<ret<<std::endl;
        return true;
      }
    }

    if (ReadBoard::bID==1 && ReadBoard::ev_per_file){
      ReadBoard::event_count += NumEvents;
      if (ReadBoard::event_count > ReadBoard::ev_per_file){
        if (m_data->outfile.is_open()) {
          m_data->outfile.close();
        }
        ReadBoard::file_num += 1;
        std::string new_ofile = ReadBoard::ofile_part + "_" + std::to_string(ReadBoard::file_num) + ".bin";
        if (!m_data->outfile.is_open()) {
          m_data->outfile.open(new_ofile, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
        }
        ReadBoard::event_count = 0;
      }
    }

    if (BufferSize) m_data->outfile.write(ReadBoard::buffer, BufferSize);

    Nb += BufferSize;
    Ne += NumEvents;
    CurrentTime = ReadBoard::get_time();
    ElapsedTime = CurrentTime - ReadBoard::PrevRateTime;

    int show_data_rate;
    m_variables.Get("show_data_rate", show_data_rate);
    if ((ElapsedTime > 2000) && show_data_rate) {
//    ret = CAEN_DGTZ_SendSWtrigger(handle);

      if (Nb==0) std::cout<<"Board "<<bID<<": No data..."<<std::endl;
      else {
        std::cout<<"Data rate Board "<<bID<<": "<<(float)Nb/((float)ElapsedTime*1048.576f)<<" MB/s   Trigger Rate Board "<<bID<<": "<<((float)Ne*1000.0f)/(float)ElapsedTime<<" Hz"<<std::endl;
      }

      Nb = 0;
      Ne = 0;
      ReadBoard::PrevRateTime = CurrentTime;
    }

    if (ReadBoard::store_temps){
      std::string bdname = ReadBoard::ModelName;
      if (bdname.find("730") != std::string::npos){
        ElapsedTime = CurrentTime - ReadBoard::PrevTempTime;
        int temp_time;
        m_variables.Get("temp_time", temp_time);
        if (ElapsedTime > temp_time*1000){
          uint32_t temp;
          for (int i; i<16; i++){
            ret = CAEN_DGTZ_ReadTemperature(handle, i, &temp);
            if (ret) {
              std::cout<<"Error reading temp: "<<ret<<std::endl;
              return false;
            }
            ReadBoard::tfile<<temp<<" ";
          }
          ReadBoard::tfile<<std::endl;
          ReadBoard::PrevTempTime = CurrentTime;
        }
      }
    }

    ReadBoard::Ne = Ne;
    ReadBoard::Nb = Nb;
  }

  return true;
}


bool ReadBoard::Finalise(){

  int handle=ReadBoard::handle, bID=ReadBoard::bID, verbose=ReadBoard::verbose;
  CAEN_DGTZ_ErrorCode ret;
  char* buffer = ReadBoard::buffer;

  ret = CAEN_DGTZ_SWStopAcquisition(handle);
  if (!ret && verbose) {
    std::cout<<"Acquisition Stopped"<<std::endl;
    ReadBoard::acq_started = 0;
  }
  if (buffer!=NULL){
    ret = CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  }

  if (m_data->outfile.is_open()) {
    m_data->outfile.close();
  }

  if (ReadBoard::store_temps){
    std::string bdname = ReadBoard::ModelName;
    if (bdname.find("730") != std::string::npos){
      if (ReadBoard::tfile.is_open()) {
        ReadBoard::tfile.close();
      }
    }
  }

  ret = CAEN_DGTZ_CloseDigitizer(handle);
  if (!ret && verbose) std::cout<<"Closed Board "<<bID<<std::endl;

  return true;
}

//===========================================================================================

// Gets current time in milliseconds
long ReadBoard::get_time(){

  long time_ms;

  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  return time_ms;
}

// Gets timestamp as a string (DDMMYYYYTHHMM)
std::string ReadBoard::Get_TimeStamp(){

  char timestamp[13];

  time_t rawtime;
  struct tm* timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(timestamp, 13, "%y%m%dT%H%M", timeinfo);

  return timestamp;
}

// Opens board and returns board handle
int ReadBoard::OpenBoard(Store m_variables){

  uint32_t address;
  int handle, bID, VME_bridge, LinkNum, ConetNode, verbose;
  CAEN_DGTZ_ErrorCode ret;
  CAEN_DGTZ_BoardInfo_t BoardInfo;

  m_variables.Get("VME_bridge", VME_bridge);
  m_variables.Get("LinkNum", LinkNum);
  m_variables.Get("ConetNode", ConetNode);
  m_variables.Get("verbose", verbose);
  m_variables.Get("bID", bID);

  if (VME_bridge) {

    std::string tmp;

    m_variables.Get("address", tmp);
    address = std::stoi(tmp, 0, 16);
  }
  else address = 0;

  ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, LinkNum, ConetNode, address, &handle);
  if (ret) std::cout<<"Error opening digitizer. CAEN Error Code: "<<ret<<std::endl;

  ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  if (ret) std::cout <<"Error getting board info. CAEN Error Code: "<<ret<<std::endl;
  else if (!ret && verbose) {
    std::cout<<"Connected to Board "<<bID<<" Model: "<<BoardInfo.ModelName<<std::endl;
  }
  ReadBoard::ModelName = BoardInfo.ModelName;

  return handle;
}

// Opens data and temperature files, arms boards
bool ReadBoard::Start_Acquisition(int handle) {
  CAEN_DGTZ_ErrorCode ret;
  std::string ofile;
  m_variables.Get("ofile", ofile);

  std::string timestamp = ReadBoard::Get_TimeStamp();
  std::string ofile_part;
  ofile_part = ofile + "_" + timestamp;
  ReadBoard::ofile_part = ofile_part;
  std::string ofile_full = ofile_part + "_0.bin";
  ReadBoard::file_num = 0;

  if (!m_data->outfile.is_open()) {
    m_data->outfile.open(ofile_full, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
  }

// Temperature file
  if (ReadBoard::store_temps){
    std::string bdname = ReadBoard::ModelName;
    if (bdname.find("730") != std::string::npos){
      std::string tempfile = ofile_part + "_temps_b" + std::to_string(bID) + ".txt";
      ReadBoard::tfile.open(tempfile, std::ofstream::out);

      ReadBoard::tfile<<"#Each column corresponds to a channel"<<std::endl;
    }
  }
/*
  // Arm all boards except the first
  if (ReadBoard::bID != 1) {
    ret = CAEN_DGTZ_WriteRegister(handle, 0x8100, 0x6);
    if (ret) {
      std::cout<<"Error while arming board: "<<ret<<std::endl;
      return false;
    }
    else {std::cout<<"Board "<<ReadBoard::bID<<" armed"<<std::endl;}
  }
*/

// Set trigger to propagate on first board
  if (bID == 1){
    std::string TrigInMode;
    m_variables.Get("TrigInMode", TrigInMode);
    if (TrigInMode=="DISABLED") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
    }
    else if (TrigInMode=="EXTOUT_ONLY") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY);
    }
    else if (TrigInMode=="ACQ_ONLY") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
    }
    else if (TrigInMode=="ACQ_AND_EXTOUT") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
    }
    else {
      std::cout<<"TrigInMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
      ret = CAEN_DGTZ_GenericError;
    }

// Arm first board
    ret = CAEN_DGTZ_WriteRegister(handle, 0x8100, 0x6);
    if (ret)  {
      std::cout<<"Error arming board 1"<<std::endl;
      return false;
    }
    std::cout<<"Armed board 1"<<std::endl;
  }
//  ReadBoard::acq_started = 1;
  Nb = 0;
  Ne = 0;
  ReadBoard::PrevRateTime = ReadBoard::get_time();
  ReadBoard::PrevTempTime = ReadBoard::get_time();

  return true;
}

bool ReadBoard::Stop_Acquisition(int handle) {
  CAEN_DGTZ_ErrorCode ret;
  ret = CAEN_DGTZ_SWStopAcquisition(handle);
  if (!ret) {
    std::cout<<"Acquisition Stopped"<<std::endl;
//    ReadBoard::acq_started = 0;
  }
  else {
    std::cout<<"SWStopAcquisition Error: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_DGTZ_FreeReadoutBuffer(&(ReadBoard::buffer));
  if (ret) {
    std::cout<<"FreeReadoutBuffer Error: "<<ret<<std::endl;
    return false;
  }

  if (m_data->outfile.is_open()) {
    m_data->outfile.close();
  }

  if (ReadBoard::store_temps){
    std::string bdname = ReadBoard::ModelName;
    if (bdname.find("730") != std::string::npos){
      if (ReadBoard::tfile.is_open()) {
        ReadBoard::tfile.close();
      }
    }
  }

  ret = CAEN_DGTZ_Reset(handle);
  if (!ret) std::cout<<"Board "<<ReadBoard::bID<<" reset"<<std::endl;
  else if (ret) {
    std::cout<<"Board reset failed!!!"<<std::endl;
    return false;
  }

  return true;
}

// Sets board settings like trigger threshold, record length, DC offset, etc.
bool ReadBoard::ConfigureBoard(int handle, Store m_variables) {

  std::string bdname = ReadBoard::ModelName;
  std::string tmp, tmp2, ChanSelfTrigMode, GrpSelfTrigMode;
  uint32_t ChanEnableMask, ChanSelfTrigMask, GrpEnableMask, GrpSelfTrigMask;

  if (bdname.find("730") != std::string::npos) {
    m_variables.Get("ChanEnableMask", tmp);
    m_variables.Get("ChanSelfTrigMask", tmp2);
    m_variables.Get("ChanSelfTrigMode", ChanSelfTrigMode);
    ChanEnableMask = std::stoi(tmp, 0, 16);
    ChanSelfTrigMask = std::stoi(tmp2, 0, 16);
  }
  else if (bdname.find("740") != std::string::npos) {
    m_variables.Get("GrpEnableMask", tmp);
    m_variables.Get("GrpSelfTrigMask", tmp2);
    m_variables.Get("GrpSelfTrigMode", GrpSelfTrigMode);
    GrpEnableMask = std::stoi(tmp, 0, 16);
    GrpSelfTrigMask = std::stoi(tmp2, 0, 16);
  }

  uint32_t recLen, postTrig, thresh, DCOff;
  uint32_t length, mask, percent, reg;
  uint16_t dec_factor;
  int bID, verbose, use_ETTT;
  float dynRange;
  std::string polarity, TrigInMode, SWTrigMode, IOLevel;
  CAEN_DGTZ_TriggerPolarity_t pol;
  CAEN_DGTZ_TriggerMode_t selftrigmode;
  CAEN_DGTZ_IOLevel_t iolevel;
  CAEN_DGTZ_ErrorCode ret;

  m_variables.Get("RecLen", recLen);
  m_variables.Get("PostTrig", postTrig);
  m_variables.Get("DynRange", dynRange);
  m_variables.Get("thresh", thresh);
  m_variables.Get("polarity", polarity);
  m_variables.Get("TrigInMode", TrigInMode);
  m_variables.Get("SWTrigMode", SWTrigMode);
  m_variables.Get("DCOff", DCOff);
  m_variables.Get("bID", bID);
  m_variables.Get("verbose", verbose);
  m_variables.Get("use_ETTT", use_ETTT);
  m_variables.Get("IOLevel", IOLevel);

  int ev_per_file;
  m_variables.Get("ev_per_file", ev_per_file);
  ReadBoard::ev_per_file = ev_per_file;

  int store_temps;
  m_variables.Get("store_temps", store_temps);
  ReadBoard::store_temps = store_temps;

  ret = CAEN_DGTZ_Reset(handle);
  if (!ret && verbose) std::cout<<"Board reset"<<std::endl;
  else if (ret) {
    std::cout<<"Board reset failed!!!"<<std::endl;
    return false;
  }

  ret = CAEN_DGTZ_WriteRegister(handle,0xEF08,bID);
  if (!ret && verbose) std::cout<<"Set Board ID!"<<std::endl;
  else if (ret) {
    std::cout<<"Error setting board ID"<<std::endl;
    return false;
  }
  if (bdname.find("740")!=std::string::npos) {
    // Enforce record length is multiple of 24 samples for ease of data decoding for V1740
    int rem = recLen%24;
    if (rem < 12) {recLen -= rem;}
    else {recLen += (24-rem);}
  }
  else {
    // Enforce record length is multiple of 20 samples to avoid filler words in data
    int rem = recLen%20;
    if (rem < 10) {recLen -= rem;}
    else {recLen += (20-rem);}
  }

  ret = CAEN_DGTZ_SetRecordLength (handle,recLen);
  ret = CAEN_DGTZ_GetRecordLength(handle,&length);
  if (!ret && verbose) std::cout<<"Record Length set to: "<<length<<std::endl;
  else if (ret) {
    std::cout<<"Error setting record length"<<std::endl;
    return false;
  }
//  usleep(1000000);
  if (bdname.find("740")!=std::string::npos) {
    ret = CAEN_DGTZ_SetDecimationFactor(handle, 1);
    if (ret) {
      std::cout<<"Error setting decimation factor: "<<ret<<std::endl;
      return false;
    }
   // usleep(1000000);
    ret = CAEN_DGTZ_GetDecimationFactor(handle, &dec_factor);
    if (!ret) {std::cout<<"Decimation Factor: "<<dec_factor<<std::endl;}
    else {
      std::cout<<"Error getting decimation factor: "<<ret<<std::endl;
      return false;
    }
  }
 // usleep(1000000);

  if (bdname.find("730")!=std::string::npos) {
    ret = CAEN_DGTZ_SetPostTriggerSize(handle, postTrig);
    ret = CAEN_DGTZ_GetPostTriggerSize(handle, &percent);
    if (!ret && verbose) std::cout<<"Post Trigger set to: "<<percent<<"%"<<std::endl;
    else if (ret) {
      std::cout<<"Error setting post trigger size"<<std::endl;
      return false;
    }
  }
  else if (bdname.find("740")!=std::string::npos) {
    uint32_t posttrig = int(length*postTrig/100);
    ret = CAEN_DGTZ_WriteRegister(handle, 0x8114, posttrig);
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8114, &percent);

    if (!ret && verbose) std::cout<<"Post Trigger set to: "<<float(percent)/float(length)*100<<"%"<<std::endl;
    else if (ret) {
      std::cout<<"Error setting post trigger size"<<std::endl;
      return false;
    }
  }

  if (polarity!="positive" && polarity!="negative") {
    std::cout<<"Invalid trigger polarity. Must be 'positive' or 'negative'"<<std::endl;
    ret = CAEN_DGTZ_GenericError;
  }
  else if (polarity=="positive") {
    ret = CAEN_DGTZ_SetTriggerPolarity(handle,0,CAEN_DGTZ_TriggerOnRisingEdge);
    ret = CAEN_DGTZ_GetTriggerPolarity(handle,0,&pol);
  }
  else if (polarity=="negative") {
    ret = CAEN_DGTZ_SetTriggerPolarity(handle,0,CAEN_DGTZ_TriggerOnFallingEdge);
    ret = CAEN_DGTZ_GetTriggerPolarity(handle,0,&pol);
  }
  if (!ret && verbose && pol==0) std::cout<<"Trigger polarity set to: positive"<<std::endl;
  else if (!ret && verbose && pol==1) std::cout<<"Trigger polarity set to: negative"<<std::endl;
  else if (ret) {
    std::cout<<"Error setting trigger polarity"<<std::endl;
    return false;
  }

  if (bdname.find("730") != std::string::npos) {
    ret = CAEN_DGTZ_SetChannelEnableMask(handle, ChanEnableMask);
    ret = CAEN_DGTZ_GetChannelEnableMask(handle, &mask);
    if (!ret && verbose) std::cout<<"Channels Enabled: "<<std::hex<<mask<<std::dec<<std::endl;
    else if (ret) {
      std::cout<<"Error enabling channels"<<std::endl;
      return false;
    }
  }
  else if (bdname.find("740") != std::string::npos) {
    ret = CAEN_DGTZ_SetGroupEnableMask(handle, GrpEnableMask);
    ret = CAEN_DGTZ_GetGroupEnableMask(handle, &mask);
    if (!ret && verbose) std::cout<<"Groups Enabled: "<<std::hex<<mask<<std::dec<<std::endl;
    else if (ret) {
      std::cout<<"Error enabling groups"<<std::endl;
      return false;
    }
  }
  if (bID != 1){
    if (TrigInMode=="DISABLED") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
    }
    else if (TrigInMode=="EXTOUT_ONLY") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY);
    }
    else if (TrigInMode=="ACQ_ONLY") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
    }
    else if (TrigInMode=="ACQ_AND_EXTOUT") {
      ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
    }
    else {
      std::cout<<"TrigInMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
      ret = CAEN_DGTZ_GenericError;
    }
  }

  if (SWTrigMode=="DISABLED") {
    ret = CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
  }
  else if (SWTrigMode=="EXTOUT_ONLY") {
    ret = CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY);
  }
  else if (SWTrigMode=="ACQ_ONLY") {
    ret = CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  }
  else if (SWTrigMode=="ACQ_AND_EXTOUT") {
    ret = CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  }
  else {
    std::cout<<"SWTrigMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
    ret = CAEN_DGTZ_GenericError;
  }


  ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  if (ret) {
    std::cout<<"Error setting acquisition mode"<<std::endl;
    return false;
  }

  if (use_ETTT) {
    ret = CAEN_DGTZ_ReadRegister(handle, 0x811C, &reg);
    reg = (reg & 0xFF0FFFFF) | 0x00400000;
    ret = CAEN_DGTZ_WriteRegister(handle, 0x811C, reg);
    if (ret) {
      std::cout<<"Error setting ETTT"<<std::endl;
      return false;
    }
  }

  ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 10);
  if (ret) {
    std::cout<<"Error setting Max Num Events BLT"<<std::endl;
    return false;
  }

  if (IOLevel=="TTL") {
    ret = CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_TTL);
  }
  else if (IOLevel=="NIM") {
    ret = CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  }
  else {
    std::cout<<"IO Level must be TTL or NIM"<<std::endl;
    ret = CAEN_DGTZ_GenericError;
  }
  if (ret) {
    std::cout<<"Error setting IO Level"<<std::endl;
    return false;
  }
  else {
    ret = CAEN_DGTZ_GetIOLevel(handle, &iolevel);
    if (iolevel==0) {
      std::cout<<"IO Level: NIM"<<std::endl;
    }
    else if (iolevel==1) {
      std::cout<<"IO Level: TTL"<<std::endl;
    }
    else {
      std::cout<<"Error reading IO Level"<<std::endl;
      return false;
    }
  }

  int use_global;
  m_variables.Get("use_global", use_global);

// Use the global settings
  if (use_global) {

    if (bdname.find("730") != std::string::npos) {
      if(dynRange==2.0) {
        ret = CAEN_DGTZ_WriteRegister(handle, 0x8028, 0);
        if (!ret && verbose) std::cout<<"Dynamic Range set to 2 Vpp"<<std::endl;
      }
      else if (dynRange==0.5) {
        ret = CAEN_DGTZ_WriteRegister(handle, 0x8028, 1);
        if (!ret && verbose) std::cout<<"Dynamic Range set to 0.5 Vpp"<<std::endl;
      }
      else {
        std::cout<<"Invalid Dynamic Range Input"<<std::endl;
        ret = CAEN_DGTZ_GenericError;
      }
      if (ret) {
        std::cout<<"Error setting dynamic range"<<std::endl;
        return false;
      }
    }

    ret = CAEN_DGTZ_WriteRegister(handle, 0x8080, thresh);
    if (!ret && verbose) std::cout<<"Trigger threshold set"<<std::endl;
    else if (ret) {
      std::cout<<"Error setting trigger threshold"<<std::endl;
      return false;
    }

    ret = CAEN_DGTZ_WriteRegister(handle, 0x8098, DCOff);
    if (!ret && verbose) std::cout<<"DC Offset set"<<std::endl;
    else if (ret) {
      std::cout<<"Error setting DC offset"<<std::endl;
      return false;
    }

    if (bdname.find("730") != std::string::npos) {
      if (ChanSelfTrigMode=="DISABLED") {
        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, ChanSelfTrigMask);
      }
      else if (ChanSelfTrigMode=="EXTOUT_ONLY") {
        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, ChanSelfTrigMask);
      }
      else if (ChanSelfTrigMode=="ACQ_ONLY") {
        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, ChanSelfTrigMask);
      }
      else if (ChanSelfTrigMode=="ACQ_AND_EXTOUT") {
        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, ChanSelfTrigMask);
      }
      else {
        std::cout<<"ChanSelfTrigMode must be DISABLED/EXT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
        ret = CAEN_DGTZ_GenericError;
      }
      if (ret) {
        std::cout<<"Error setting channel self trigger"<<std::endl;
        return false;
      }
    }
    else if (bdname.find("740") != std::string::npos) {
      if (GrpSelfTrigMode=="DISABLED") {
        ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, GrpSelfTrigMask);
      }
      else if (GrpSelfTrigMode=="EXTOUT_ONLY") {
        ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, GrpSelfTrigMask);
      }
      else if (GrpSelfTrigMode=="ACQ_ONLY") {
        ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, GrpSelfTrigMask);
      }
      else if (GrpSelfTrigMode=="ACQ_AND_EXTOUT") {
        ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, GrpSelfTrigMask);
      }
      else {
        std::cout<<"GrpSelfTrigMode must be DISABLED/EXT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
        ret = CAEN_DGTZ_GenericError;
      }
      if (ret) {
        std::cout<<"Error setting group self trigger"<<std::endl;
        return false;
      }
    }
  }

// Use the individual channel settings
  else {
    std::ifstream ifile;

    if (bdname.find("730") != std::string::npos) {
      std::string path;
      m_variables.Get("chan_set_file", path);

      ifile.open(path);

      if (!(ifile.is_open())) {
        std::cout<<"Error opening channel settings file"<<std::endl;
        return false;
      }
    }
    else if (bdname.find("740") != std::string::npos) {
      std::string path;
      m_variables.Get("group_set_file", path);

      ifile.open(path);

      if (!(ifile.is_open())) {
        std::cout<<"Error opening group settings file"<<std::endl;
        return false;
      }
    }

    int value, chan, grp, reg;
    uint32_t mask;
    std::string line;
    std::getline(ifile, line);

    while(std::getline(ifile, line, ',')) {

      if (bdname.find("730") != std::string::npos) {
        chan = std::stoi(line);

        std::getline(ifile, line, ',');
        value = std::stoi(line);
        ret = CAEN_DGTZ_SetChannelDCOffset(handle, chan, value);
        if (ret) {
          std::cout<<"Error setting channel "<<chan<<" DC offset"<<std::endl;
          return false;
        }

        std::getline(ifile, line, ',');
        value = std::stoi(line);
        ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, chan, value);
        if (ret) {
          std::cout<<"Error setting channel "<<chan<<" trigger threshold"<<std::endl;
          return false;
        }

        if (chan%2) std::getline(ifile, line);
        else std::getline(ifile, line, ',');
        reg = 0x1028 + (0x100 * chan);
        if (line=="2.0") {
          ret = CAEN_DGTZ_WriteRegister(handle, reg, 0);
        }
        else if (line=="0.5") {
          ret = CAEN_DGTZ_WriteRegister(handle, reg, 1);
        }
        else {
          ret = CAEN_DGTZ_GenericError;
          std::cout<<"Invalid dynamic range, must be 2.0 or 0.5"<<std::endl;
        }
        if (ret) {
          std::cout<<"Error setting channel "<<chan<<" dynamic range"<<std::endl;
          return false;
        }

        if (!(chan%2)) {

          int ch_to_set = int(pow(2, chan));

          std::getline(ifile, line, ',');
          if (line=="EXTOUT_ONLY") {
            ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, ch_to_set);
          }
          else if (line=="ACQ_ONLY") {
            ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, ch_to_set);
          }
          else if (line=="ACQ_AND_EXTOUT") {
            ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, ch_to_set);
          }
          else if (line=="DISABLED") {
            ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, ch_to_set);
          }
          else {
            std::cout<<"Channel self trigger mode must be DISABLED/ACQ_ONLY/EXTOUT_ONLY/ACQ_AND_EXTOUT"<<std::endl;
            ret = CAEN_DGTZ_GenericError;
          }
          if (ret) {
            std::cout<<"Error setting channel "<<chan<<" self trigger mode"<<std::endl;
            return false;
          }

          reg = 0x1084 + (chan * 0x100);

          std::getline(ifile, line);
          if (line=="OR") {
            ret = CAEN_DGTZ_WriteRegister(handle, reg, 3);
          }
          else if (line=="AND") {
            ret = CAEN_DGTZ_WriteRegister(handle, reg, 0);
          }
          else if (line=="Xn") {
            ret = CAEN_DGTZ_WriteRegister(handle, reg, 1);
          }
          else if (line=="Xn+1") {
            ret = CAEN_DGTZ_WriteRegister(handle, reg, 2);
          }
          else {
            std::cout<<"Channel self trigger logic must be OR/AND/Xn/Xn+1"<<std::endl;
            ret = CAEN_DGTZ_GenericError;
          }
          if (ret) {
            std::cout<<"Error setting channel "<<chan<<" self trigger logic"<<std::endl;
            return false;
          }
        }
      }
      else if (bdname.find("740") != std::string::npos) {
        grp = std::stoi(line);

        std::getline(ifile, line, ',');
        value = std::stoi(line);
        ret = CAEN_DGTZ_SetGroupDCOffset(handle, grp, value);
        if (ret) {
          std::cout<<"Error setting group "<<grp<<" DC offset"<<std::endl;
          return false;
        }

        std::getline(ifile, line, ',');
        value = std::stoi(line);
        ret = CAEN_DGTZ_SetGroupTriggerThreshold(handle, grp, value);
        if (ret) {
          std::cout<<"Error setting grp "<<grp<<" trigger threshold"<<std::endl;
          return false;
        }

        int grp_to_set = int(pow(2, grp));

        std::getline(ifile, line, ',');
        if (line=="EXTOUT_ONLY") {
          ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, grp_to_set);
        }
        else if (line=="ACQ_ONLY") {
          ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, grp_to_set);
        }
        else if (line=="ACQ_AND_EXTOUT") {
          ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, grp_to_set);
        }
        else if (line=="DISABLED") {
          ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, grp_to_set);
        }
        else {
          std::cout<<"Group self trigger mode must be DISABLED/ACQ_ONLY/EXTOUT_ONLY/ACQ_AND_EXTOUT"<<std::endl;
          ret = CAEN_DGTZ_GenericError;
        }
        if (ret) {
          std::cout<<"Error setting group "<<grp<<" self trigger mode"<<std::endl;
          return false;
        }

        std::getline(ifile, line, ',');
        mask = std::stoi(line, 0, 16);
        ret = CAEN_DGTZ_SetChannelGroupMask(handle, grp, mask);
        if (ret) {
          std::cout<<"Error setting group "<<grp<<" channel mask"<<std::endl;
          return false;
        }
      }
    }
    ifile.close();
  }

  uint32_t AllocatedSize;
  char *buffer = NULL;

  ret = CAEN_DGTZ_MallocReadoutBuffer(handle,&buffer,&AllocatedSize);
  if (ret) {
    std::cout<<"Error allocating readout buffer"<<std::endl;
    return false;
  }

  ReadBoard::buffer = buffer;

  // Set run start delays to compensate for daisy chain
  uint32_t delay = 3*(5-bID);
  ret = CAEN_DGTZ_WriteRegister(handle, 0x8170, delay);
  if (ret) {
    std::cout<<"Error while setting run delay: "<<ret<<std::endl;
    return false;
  }
  else {
    uint32_t reg_data;
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8170, &reg_data);
    if (ret) {
      std::cout<<"Error reading run delay: "<<ret<<std::endl;
      return false;
    }
    else {std::cout<<"Set board "<<bID<<" run delay to: "<<((reg_data & 0xff) * 16)<<" ns"<<std::endl;}
  }

  // Arm all boards except the first
  if (ReadBoard::bID != 1) {
    ret = CAEN_DGTZ_WriteRegister(handle, 0x8100, 0x6);
    if (ret) {
      std::cout<<"Error while arming board: "<<ret<<std::endl;
      return false;
    }
    else {std::cout<<"Board "<<ReadBoard::bID<<" armed"<<std::endl;}
  }

  usleep(1000000);

  return true;
}
