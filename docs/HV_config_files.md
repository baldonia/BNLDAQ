# HV config files
The parameters in this file are for individual HV channels in a CAEN HV supply.

| Parameter Name | Type | Description                                                      |
|:---------------|------|------------------------------------------------------------------|
| slot           | int  | The HV crate slot in which the HV board is located (board only)  |
| V0Set          | int  | The voltage at which the channel is set (V)                      |
| I0Set          | int  | The current limit (uA) at which the channel will trip            |
| RUp            | int  | The rate of voltage increase (V/s) upon Power On                 |
| RDWn           | int  | The rate of voltage decrease (V/s) upon Power Down               |
| Trip           | int  | The time limit for over current before trip (sec)                |
| SVMax          | int  | The maximum voltage before tripping                              |
| enable         | bool | Power on this channel                                            |
