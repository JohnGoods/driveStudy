#ifndef   CTL_CODE
#pragma message("\n \n-----------EXEģʽ . Include winioctl.h ")
 #include<winioctl.h> //CTL_CODE ntddk.h wdm.h
  #else 
#pragma message("-\n \n---------SYSģʽ .NO Include winioctl.h ")
#endif

#define add_code CTL_CODE(FILE_DEVICE_UNKNOWN, 	0x800, 	METHOD_NEITHER,FILE_ANY_ACCESS)
#define sub_code CTL_CODE(FILE_DEVICE_UNKNOWN, 	0x801, 	METHOD_NEITHER,FILE_ANY_ACCESS)
