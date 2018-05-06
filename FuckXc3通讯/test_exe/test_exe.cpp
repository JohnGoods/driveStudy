// test_exe.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include "ctl_code.h"


// sub (HANDLE hDevice, int a,int b);
#define CTL_CODET(x) CTL_CODE(FILE_DEVICE_UNKNOWN,0x800 + x,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SymbolsInfo									CTL_CODET(0)
#define IOCTL_Windbg										CTL_CODET(1)
#define IOCTL_CallBack										CTL_CODET(2)

#define LINK_GLOBAL_NAME	L"\\DosDevices\\Global\\FacKProtects"

int add(HANDLE hDevice, int a,int b)
{
	
	int port[2];
	int bufret;
	ULONG dwWrite;
	port[0]=a;
	port[1]=b;

	//DeviceIoControl(hDevice, IOCTL_SymbolsInfo, &port, 8, &bufret, 4, &dwWrite, NULL);
	return bufret;

}

int main(int argc, char* argv[])
{
	//add
	//CreateFile 打开设备 获取hDevice
	HANDLE hDevice = 
		CreateFile(L"\\\\.\\FacKProtects",  //L"\\DosDevices\\Global\\FacKProtects"
		GENERIC_READ | GENERIC_WRITE,
		0,		// share mode none
		NULL,	// no security
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );		// no template
	printf("start\n");
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("获取驱动句柄失败: %s with Win32 error code: %d\n","MyDriver", GetLastError() );
		getchar();
		return -1;
	}

	int r = DeviceIoControl(hDevice, IOCTL_SymbolsInfo, NULL, NULL, NULL, NULL, NULL, NULL);
	//int a=999;
	//int b=111;
 // int r=add(hDevice,a,b);
 // printf("%d+%d=%d \n",a,b,r);
 // getchar();
	return 0;
}

