//_stdcall
#include "mini_ddk.h"
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING B) //TYPEDEF LONG NTSTATUS
{
KdPrint(("驱动成功被加载...OK++++++++"));
//jmp指令
 CreateMyDevice(pDriverObject);//创建相应的设备
 pDriverObject->DriverUnload=DDK_Unload;
return (1);
}
VOID DDK_Unload (IN PDRIVER_OBJECT pDriverObject)
{
  PDEVICE_OBJECT pDev;//用来取得要删除设备对象
  UNICODE_STRING symLinkName; // 
 
  pDev=pDriverObject->DeviceObject;
  IoDeleteDevice(pDev); //删除设备
  
  //取符号链接名字
   RtlInitUnicodeString(&symLinkName,L"\\??\\yjx888");
  //删除符号链接
   IoDeleteSymbolicLink(&symLinkName);
 KdPrint(("驱动成功被卸载...OK-----------")); //sprintf,printf
 //取得要删除设备对象
//删掉所有设备
 DbgPrint("卸载成功");
}