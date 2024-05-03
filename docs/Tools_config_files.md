# Tools Config Files

Tools config files are the list of tools to be added to the Toolchain that will be run (in order) by ToolDAQ. 
The format is `<Unique Tool Instance Name> <Tool Name> <path to config file to pass to tool>`. 
For example, to add 2 ReadBoard tools with different config files to the Toolchain, the ToolsConfig file would look like:
```
ReadBoard1 ReadBoard configfiles/config_b1
ReadBoard2 ReadBoard configfiles/config_b2
```
