//_stdcall
#include "mini_ddk.h"
#pragma  INITCODE
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING B) //TYPEDEF LONG NTSTATUS
{  
	ULONG cur,old;
	JMPCODE JmpCode;
	cur=GetNt_CurAddr();//A
	old=GetNt_OldAddr();//C
	if (cur!=old)
	{   JmpCode.E9=0xE9; 
		JmpCode.JMPADDR=old-cur-5;
	    KdPrint(("要写入的地址%X",JmpCode.JMPADDR));
        //写入JMP   C-A-5=B //实际要写入地址

		KdPrint(("NtOpenProcess被HOOK了"));
	}
 /* ULONG SSDT_NtOpenProcess_Cur_Addr;
KdPrint(("驱动成功被加载...OK++++++++\n\n"));
 //读取SSDT表中 NtOpenProcess当前地址 KeServiceDescriptorTable
 // [[KeServiceDescriptorTable]+0x7A*4] 

__asm
{    int 3
	push ebx
	push eax
		mov ebx,KeServiceDescriptorTable
		mov ebx,[ebx] //表的基地址
		mov eax,0x7a
		shl eax,2//0x7A*4 //imul eax,eax,4//shl eax,2
		add ebx,eax//[KeServiceDescriptorTable]+0x7A*4
		mov ebx,[ebx]
        mov SSDT_NtOpenProcess_Cur_Addr,ebx
	pop  eax	
	pop  ebx
}
KdPrint(("SSDT_NtOpenProcess_Cur_Addr=%x\n\n",SSDT_NtOpenProcess_Cur_Addr));*/
 //注册派遣函数
pDriverObject->MajorFunction[IRP_MJ_CREATE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_READ]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
 CreateMyDevice(pDriverObject);//创建相应的设备
 pDriverObject->DriverUnload=DDK_Unload;
return (1);
}
//#pragma code_seg("PAGE")
#pragma PAGECODE
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
#pragma PAGECODE
NTSTATUS ddk_DispatchRoutine_CONTROL(IN PDEVICE_OBJECT pDevobj,IN PIRP pIrp	)
{
	//对相应的IPR进行处理
	pIrp->IoStatus.Information=0;//设置操作的字节数为0，这里无实际意义
	pIrp->IoStatus.Status=STATUS_SUCCESS;//返回成功
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);//指示完成此IRP
	KdPrint(("离开派遣函数\n"));//调试信息
	return STATUS_SUCCESS; //返回成功
}