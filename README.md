
# BNLDAQ

BNLDAQ is the data acquisition software for the 1T and 30T WbLS testbeds at Brookhaven National Lab (BNL). It is based on [ToolDAQ](https://github.com/ToolFramework/ToolApplication) and contains code for controlling CAEN digitizers and CAEN high voltage (HV) supply boards.


****************************
# Installation
****************************

To install BNLDAQ, follow these steps:

   - Install Prerequisites: 
     - RHEL/Centos... ``` yum install git make gcc-c++ zlib-devel dialog ```
     - Debian/Ubuntu.. ``` apt-get install git make g++ libz-dev dialog ```

   - Then clone the repo with ```git clone https://github.com/baldonia/BNLDAQ.git``` or clone your own fork of this 
     repo.

   - Then run ```./GetToolDAQ.sh``` to install dependances and files for creating a ToolDAQ app.

   - Finally, run ```make clean``` and ```make``` to build the application.


****************************
# Usage
****************************

EosDAQ currently has three *Tools*:

  - **ReadBoard** which controls the CAEN digitizers

  - **HVControl** which controls the CAEN HV supply

  - **RunControl** which has commands to configure the digitizers and start/stop the acquisition

The HV supply needs a JSON configuration file passed to it. Ensure the path to this file is reflected in `configfiles/HV/hvconfig`. You will also need to change paths in `configfiles/test/ToolChainConfig` and `configfiles/test/ToolsConfig` to reflect the paths to relevant configuration files. 

When ready to run BNLDAQ, (make sure to do `source Setup.sh` for the first time) run ```./main configfiles/test/ToolChainConfig``` from the main directory. This will start BNLDAQ in the background. In another window, (again source the setup script) run ```./RemoteControl```. Within this program, you can input commands to BNLDAQ.

To receive monitor info from the HV supply, you will need to run the `HV_monitor.py` script. Be sure the address reflected in th HV tool's config file is the same as that in the monitor script.

In the RemoteControl program, first enter `List` until you see your BNLDAQ toolchain appear. The service ID number should be 0. Next, enter `Command 0 Start`. This will start the toolchain. Again, enter `List` until the SlowControlReceiver appears. *Please note that the ID number could be 0 or 1.* To see the commands you can send to this receiver, enter `Command <Receiver ID number> ?`. Currently the DAQ commands are:

  - **Config_Digitizers**: Configures the digitizer settings and arms all but the first board. *You must do this first before 
      the next two commands.*
  - **Start_Acquisition**: Starts the configuration and acquisition in WbLSdaq
  - **Stop_Acquisition**: Stops the acquisition in WbLSdaq *only do if the acquisition is running*.
  - **Read_HVConfig**: Read a JSON HV config. *You must do this each time before configuring the HV supply.*
  - **Config_HV**: Configure the HV supply using the JSON config file read in with the previous command. *You must do this before the next two commands.*
  - **PowerOn_HV**: Ramp the enabled channels up to the set voltage.
  - **PowerOff_HV**: Ramp all channels down to ~0 V.

To send a command, enter `Command <Receiver ID number> <Command to send>`. 

When finished, enter `Command <main BNLDAQ toolchain ID number> Stop` to stop the toolchain, and then `Command <main BNLDAQ toolchain ID number> Quit` to quit BNLDAQ. To exit the Remote Control program, enter `Quit`. 

Changes to config files do not require BNLDAQ to be recompiled.
