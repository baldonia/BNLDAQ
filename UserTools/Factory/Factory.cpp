#include "Factory.h"
#include "Unity.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="HVControl") ret=new HVControl;
if (tool=="ReadBoard") ret=new ReadBoard;
if (tool=="RunControl") ret=new RunControl;
return ret;
}
