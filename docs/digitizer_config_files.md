# Digitizer config files
The parameters in this file are for global board settings. Individual channel settings can be set in the [individual channel settings configuration file](channel_settings_config_files.md).

| Parameter Name   | Type      | Description                                                                                      | 
|:-----------------|-----------|--------------------------------------------------------------------------------------------------|
| verbose          | int       | Print confirmation messages (0 or 1)                                                             |
| show_data_rate   | int       | Print data rate every 2 seconds (0 or 1)                                                         |
| VME_bridge       | int       | Digitizer is connected via VME bridge (0 or 1)                                                   |
| address          | uint32_t  | Digitizer VME base address (hex) (only used if VME_bridge=1)                                     |
| LinkNum          | int       | Optical link port on A3818                                                                       |
| ConetNode        | int       | Digitzer optical link node in daisy chain                                                        |
| bID              | int       | Board ID                                                                                         |
| use_global       | int       | Use global settings (0 or 1, 0 will use individual channel settings)                             |
| DynRange         | float     | Input dynamic range (must be 0.5 or 2.0)                                                         |
| RecLen           | uint32_t  | Acquisition window length in samples                                                             |
| DCOff            | uint32_t  | DC offset in LSB (32767 means offset at 0V)                                                      |
| ChanEnableMask   | uint32_t  | 16-bit mask of active channels (hex)                                                             |
| PostTrig         | uint32_t  | Percent of acquisition window after trigger                                                      |
| thresh           | uint32_t  | Trigger threshold in LSB (not relative to baseline)                                              |
| polarity         | string    | Trigger on rising edge ("positive") or falling edge ("negative")                                 |
| ChanSelfTrigMask | uint32_t  | 16-bit mask of channels to activate self trigger (hex)                                           |
| ChanSelfTrigMode | string    | Channel self trigger propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT"   |
| TrigInMode       | string    | External trigger input propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT" |
| SWTrigMode       | string    | SW trigger propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT" (currently not implemented, keep at DISABLED)|
| chan_set_file    | string    | Path to individual channel settings file                                                         |
| ofile            | string    | Output file name or path (must be the same for all digitizer config files, working on updating) |
