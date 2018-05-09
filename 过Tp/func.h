#include <ntddk.h>
#include "LDE64x64.h"
//typedef KTRAP_FRAME *PKEXCEPTION_FRAME;
#define kmalloc(_s) ExAllocatePoolWithTag(NonPagedPool, _s, 'SYSQ')
#define kfree(_p) ExFreePool(_p)
#define PAGEDCODE code_seg("PAGE")   
#define LOCKEDCODE code_seg()   
#define INITCODE code_seg("INIT")   
#define PAGEDDATA data_seg("PAGE")   
#define LOCKEDDATA data_seg()   
#define INITDATA data_seg("INIT")

ULONG GetPatchSize(PUCHAR Address)
{
	ULONG LenCount=0,Len=0;
	while(LenCount<=14)	//至少需要14字节
	{
		Len=LDE(Address,64);
		Address=Address+Len;
		LenCount=LenCount+Len;
	}
	return LenCount;
}


//KdpStub 函数地址变量存储
ULONGLONG KdpStubAddr;

//KdpTrap 函数地址变量存储
ULONGLONG KdpTrapAddr;
ULONGLONG KiDebugRoutineAddr;


//KdDebuggerEnabled变量存储
BOOLEAN gKdDebuggerEnabled = TRUE;

//KdPitchDebugger变量存储
BOOLEAN gKdPitchDebugger = FALSE;

//KiDebugRoutine变量存储
ULONGLONG gKiDebugRoutine = 0;
extern PVOID  KdEnteredDebugger;
//ULONGLONG pKdEnteredDebugger=(ULONGLONG)KdEnteredDebugger;

//找这个个替死鬼 KeBugCheckEx
//UCHAR* KeBugCheckAddr=(UCHAR*)KeBugCheckEx;
UCHAR oldvalue[2];
UCHAR KdpStubHead5[5];
ULONG OldTpVal;
ULONGLONG jmpCode = (ULONGLONG)((char*)KeBugCheckEx+10);//往后15字节不能再用

ULONG pslp_patch_size=0;		//IoAllocateMdl被修改了N字节
PUCHAR pslp_head_n_byte=NULL;	//IoAllocateMdl的前N字节数组
PVOID ori_pslp=NULL;			//IoAllocateMdl的原函数



typedef PMDL (__fastcall *_MyIoAllocateMdl)(
	__in_opt PVOID  VirtualAddress,
	__in ULONG  Length,
	__in BOOLEAN  SecondaryBuffer,
	__in BOOLEAN  ChargeQuota,
	__inout_opt PIRP  Irp  OPTIONAL);
_MyIoAllocateMdl IoAllocateMdlAddr=NULL;
KIRQL WPOFFx64()
{
	KIRQL irql=KeRaiseIrqlToDpcLevel();
	UINT64 cr0=__readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0=__readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}
ULONGLONG calcJmpAddr(ULONGLONG curAddr, ULONGLONG codeLength, ULONG codenum)
{
	ULONGLONG high8bit;
	ULONGLONG low8bit;
	high8bit = curAddr&0xffffffff00000000;
	low8bit =  curAddr&0x00000000ffffffff;
	low8bit = (low8bit+codeLength+codenum)&0x00000000ffffffff;
	return low8bit+high8bit;

}
ULONG calcJmpCodeNum(ULONGLONG curAddr, ULONGLONG codeLength, ULONGLONG targetAddr)
{	
	ULONGLONG high8bit;
	ULONGLONG low8bit;
	ULONG result;
	result = (targetAddr-curAddr-codeLength)&0x00000000ffffffff;
	return result;
}
ULONGLONG ScanFeatureCode(unsigned char* szFeatureCode,ULONG featLength, ULONGLONG startAddr)
{
	ULONGLONG result=0;
	ULONGLONG i;
	for (i =startAddr;i<startAddr+2014;i++)
	{
		if(RtlEqualMemory(szFeatureCode,(void*)i,featLength))
		{
			//找到了
			result = i+featLength;
			break;
		}

	}
	return result;
}


//传入：待HOOK函数地址，代理函数地址，接收原始函数地址的指针，接收补丁长度的指针；返回：原来头N字节的数据
PVOID HookKernelApi(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize)
{
	KIRQL irql;
	UINT64 tmpv;
	PVOID head_n_byte,ori_func;
	UCHAR jmp_code[]="\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	UCHAR jmp_code_orifunc[]="\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	//How many bytes shoule be patch
	*PatchSize=GetPatchSize((PUCHAR)ApiAddress);
	//step 1: Read current data
	head_n_byte=kmalloc(*PatchSize);
	irql=WPOFFx64();
	memcpy(head_n_byte,ApiAddress,*PatchSize);
	WPONx64(irql);
	
	//step 2: Create ori function
	ori_func=kmalloc(*PatchSize+14);	//原始机器码+跳转机器码
	RtlFillMemory(ori_func,*PatchSize+14,0x90);
	tmpv=(ULONG64)ApiAddress+*PatchSize;	//跳转到没被打补丁的那个字节
	memcpy(jmp_code_orifunc+6,&tmpv,8);
	memcpy((PUCHAR)ori_func,head_n_byte,*PatchSize);
	memcpy((PUCHAR)ori_func+*PatchSize,jmp_code_orifunc,14);
	*Original_ApiAddress=ori_func;
	//step 3: fill jmp code
	tmpv=(UINT64)Proxy_ApiAddress;
	memcpy(jmp_code+6,&tmpv,8);
	//step 4: Fill NOP and hook
	irql=WPOFFx64();
	RtlFillMemory(ApiAddress,*PatchSize,0x90);
	memcpy(ApiAddress,jmp_code,14);
	WPONx64(irql);
	//return ori code
	return head_n_byte;
}
//传入：被HOOK函数地址，原始数据，补丁长度
VOID UnhookKernelApi(IN PVOID ApiAddress, IN PVOID OriCode, IN ULONG PatchSize)
{
	KIRQL irql;
	irql=WPOFFx64();
	memcpy(ApiAddress,OriCode,PatchSize);
	WPONx64(irql);
}
ULONGLONG process;
PMDL MyIoAllocateMdl(
	__in_opt PVOID  VirtualAddress,
	__in ULONG  Length,
	__in BOOLEAN  SecondaryBuffer,
	__in BOOLEAN  ChargeQuota,
	__inout_opt PIRP  Irp  OPTIONAL)
{

	if (KdEnteredDebugger == VirtualAddress)
	{
		process=(ULONGLONG)PsGetCurrentProcess();
		KdPrint(("KdEnteredDebugger addr is %p\nprocess name :%s\n",KdEnteredDebugger,(PUCHAR)(process+0x2e0)));
		VirtualAddress = (PVOID)((ULONGLONG)KdEnteredDebugger + 0x40);  //+0x20  是让他读到其他的位置
		//DbgBreakPoint();
	}
	return ((_MyIoAllocateMdl)ori_pslp)(VirtualAddress,Length,SecondaryBuffer,ChargeQuota,Irp);



}
VOID HookIoAllocateMdl()
{
	pslp_head_n_byte = HookKernelApi(IoAllocateMdl,
									(PVOID)MyIoAllocateMdl,
									&ori_pslp,
									&pslp_patch_size);
}

VOID UnhookIoAllocateMdl()
{
	UnhookKernelApi(IoAllocateMdl,
					pslp_head_n_byte,
					pslp_patch_size);
}


typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
	ULONG Unknow1;
	ULONG Unknow2;
	ULONG Unknow3;
	ULONG Unknow4;
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT NameLength;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	char ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG Count;//内核中以加载的模块的个数
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _KLDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY64 InLoadOrderLinks;
	ULONG64 __Undefined1;
	ULONG64 __Undefined2;
	ULONG64 __Undefined3;
	ULONG64 NonPagedDebugInfo;
	ULONG64 DllBase;
	ULONG64 EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG   Flags;
	USHORT  LoadCount;
	USHORT  __Undefined5;
	ULONG64 __Undefined6;
	ULONG   CheckSum;
	ULONG   __padding1;
	ULONG   TimeDateStamp;
	ULONG   __padding2;
}KLDR_DATA_TABLE_ENTRY, *PKLDR_DATA_TABLE_ENTRY;

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation
(
	IN ULONG	SystemInformationClass,
	OUT PVOID	SystemInformation,
	IN ULONG	Length,
	OUT PULONG	ReturnLength
);



//取得模块基址和大小函数
#pragma LOCKEDCODE
ULONG64 FindKrlModule( __out ULONG64 *ulSysModuleBase, __out  ULONG64 *ulSize, __in  PCHAR modulename_0)
{
	ULONG NeedSize, i, ModuleCount, BufferSize = 0x5000;
	PVOID pBuffer = NULL;
	PCHAR pDrvName = NULL;
	NTSTATUS Result;
	PSYSTEM_MODULE_INFORMATION pSystemModuleInformation;
	do
	{
		//分配内存
		pBuffer = kmalloc(BufferSize);
		if (pBuffer == NULL)
			return 0;
		//查询模块信息
		Result = ZwQuerySystemInformation(11, pBuffer, BufferSize, &NeedSize);
		if (Result == STATUS_INFO_LENGTH_MISMATCH)
		{
			kfree(pBuffer);
			BufferSize *= 2;
		}
		else if (!NT_SUCCESS(Result))
		{
			//查询失败则退出
			kfree(pBuffer);
			return 0;
		}
	} while (Result == STATUS_INFO_LENGTH_MISMATCH);
	pSystemModuleInformation = (PSYSTEM_MODULE_INFORMATION)pBuffer;
	//获得模块的总数量
	ModuleCount = pSystemModuleInformation->Count;
	//遍历所有的模块
	for (i = 0; i < ModuleCount; i++)
	{
		if ((ULONG64)(pSystemModuleInformation->Module[i].Base) >(ULONG64)0x8000000000000000)
		{
			pDrvName = pSystemModuleInformation->Module[i].ImageName;
			//L"" 代表宽字符，UNICODESTRING 就是宽字符，wchar_t宽字符  ""字符
			if (strstr(pDrvName, modulename_0))
			{

				*ulSysModuleBase = (ULONG64)pSystemModuleInformation->Module[i].Base;
				*ulSize = (ULONG64)pSystemModuleInformation->Module[i].Size;

				break;
			}

		}
	}
	kfree(pBuffer);
	return 0;
}

PDRIVER_OBJECT pDriverObject;


ULONG64 moduleAddr_0;//kdcom 基址
ULONG64 modulesize_0;//kdcom 大小
					 /*
					 PKLDR_DATA_TABLE_ENTRY链表结构 Blink=后一个 Flink前一个
					 DllBase  基址
					 BaseDllName   内核名字  ntoskrnl.exe
					 FullDllName  完整路径名字 c:\windows\system32\ntkrnlmp.exe
					 */


VOID HideDriver()
{
	PKLDR_DATA_TABLE_ENTRY entry = (PKLDR_DATA_TABLE_ENTRY)pDriverObject->DriverSection;
	PKLDR_DATA_TABLE_ENTRY firstentry;

	KIRQL OldIrql;
	firstentry = entry;
	FindKrlModule(&moduleAddr_0, &modulesize_0, "kdcom.dll");
	while ((PKLDR_DATA_TABLE_ENTRY)entry->InLoadOrderLinks.Flink != firstentry)
	{
		if (entry->DllBase == moduleAddr_0)
		{

			OldIrql = KeRaiseIrqlToDpcLevel();
			((LIST_ENTRY64*)(entry->InLoadOrderLinks.Flink))->Blink = entry->InLoadOrderLinks.Blink;
			((LIST_ENTRY64*)(entry->InLoadOrderLinks.Blink))->Flink = entry->InLoadOrderLinks.Flink;
			entry->InLoadOrderLinks.Flink = 0;
			entry->InLoadOrderLinks.Blink = 0;
			KeLowerIrql(OldIrql);
			DbgPrint("Remove LIST_ENTRY64 OK!");
			break;
		}
		//kprintf("%llx\t%wZ\t%wZ",entry->DllBase,entry->BaseDllName,entry->FullDllName);
		entry = (PKLDR_DATA_TABLE_ENTRY)entry->InLoadOrderLinks.Flink;
	}
}

