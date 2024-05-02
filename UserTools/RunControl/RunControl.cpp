#include "RunControl.h"

RunControl::RunControl():Tool(){}


bool RunControl::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  m_data->SC_vars.InitThreadedReceiver(m_data->context, 8123, 200, true);

  m_data->SC_vars.Add("Config_Digitizers", SlowControlElementType(BUTTON));
  m_data->SC_vars["Config_Digitizers"]->SetValue(false);

  m_data->SC_vars.Add("Start_Acquisition", SlowControlElementType(BUTTON));
  m_data->SC_vars["Start_Acquisition"]->SetValue(false);

  m_data->SC_vars.Add("Stop_Acquisition", SlowControlElementType(BUTTON));
  m_data->SC_vars["Stop_Acquisition"]->SetValue(false);

  return true;
}


bool RunControl::Execute(){

  m_data->config=false;
  m_data->start_acq=false;
  m_data->stop_acq=false;

  if (m_data->SC_vars["Config_Digitizers"]->GetValue<bool>()){
    m_data->config = true;
    m_data->SC_vars["Config_Digitizers"]->SetValue(false);
  }

  if (m_data->SC_vars["Start_Acquisition"]->GetValue<bool>()){
    m_data->start_acq = true;
    m_data->SC_vars["Start_Acquisition"]->SetValue(false);
  }

  if (m_data->SC_vars["Stop_Acquisition"]->GetValue<bool>()){
    m_data->stop_acq = true;
    m_data->SC_vars["Stop_Acquisition"]->SetValue(false);
  }

  return true;
}


bool RunControl::Finalise(){

  return true;
}
