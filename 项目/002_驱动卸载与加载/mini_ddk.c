//_stdcall
#include <ntddk.h>
#define INITCODE code_seg("INIT")//初始化内存
#pragma INITCODE
#define PAGEDDATA data_seg("PAGE")//分页内存
VOID DDK_Unload(PDRIVER_OBJECT pdriver);



int DriverEntry(PDRIVER_OBJECT pdriver, PUNICODE_STRING path_reg)
{
	DbgPrint("驱动被加载了!++++++");
   pdriver->DriverUnload=DDK_Unload;
   return 1;
}
VOID DDK_Unload(PDRIVER_OBJECT pdriver)
{
	DbgPrint("驱动被卸载了!------");

}
