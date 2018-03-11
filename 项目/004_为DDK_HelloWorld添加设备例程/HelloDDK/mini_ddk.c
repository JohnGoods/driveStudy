//_stdcall
#include <ntddk.h>
#define INITCODE code_seg("INIT")//初始化内存
#pragma INITCODE
#define PAGEDDATA data_seg("PAGE")//分页内存
VOID DDK_Unload(PDRIVER_OBJECT pdriver);
NTSTATUS CreateMyDevice(IN PDRIVER_OBJECT pDriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING path_reg)
{
	DbgPrint("驱动被加载了!++++++");
	CreateMyDevice(pDriverObject);
   pDriverObject->DriverUnload=DDK_Unload;
   return 1;
}


NTSTATUS CreateMyDevice(IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
	PDEVICE_OBJECT pDevObj;
	NTSTATUS status;
	RtlInitUnicodeString(&devName,L"\\Device\\HelloDDK");
	status=IoCreateDevice(pDriverObject,\
		           0,\
				   &devName,\
				   FILE_DEVICE_UNKNOWN,\
				   0,\
				   TRUE,\
				   &pDevObj);
	if(!NT_SUCCESS(status))
	{
		if(status==STATUS_INSUFFICIENT_RESOURCES)
		{
			DbgPrint("资源不足");
		}
		if(status==STATUS_OBJECT_NAME_EXISTS)
		{
            DbgPrint("指定对象名存在");
		}
		if(status==STATUS_OBJECT_NAME_COLLISION)
		{
			DbgPrint("对象名有冲突");
		}
		return status;
	}
   DbgPrint("创建成功！");
    pDevObj->Flags |= DO_BUFFERED_IO;
   //创建符号链接
   	RtlInitUnicodeString(&symLinkName,L"\\??\\zqsyr888");
	status = IoCreateSymbolicLink( &symLinkName,&devName );
	if (!NT_SUCCESS(status)) 
	{
		IoDeleteDevice( pDevObj );
		return status;
	}
	return STATUS_SUCCESS;

}



VOID DDK_Unload(PDRIVER_OBJECT pdriver)
{
	DbgPrint("驱动被卸载了!------");

}