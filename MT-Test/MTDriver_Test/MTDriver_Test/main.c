#include <ntddk.h>


NTSTATUS DriverUnload(PDRIVER_OBJECT Driver)
{
	DbgPrint("This driver is unloading...\n");	//��ӡж����Ϣ

	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING RegPath)
{
	Driver->DriverUnload = DriverUnload;		// ��������ж�غ���
	DbgPrint("%ws\n", RegPath->Buffer);			// ��ӡRegPath
	return STATUS_SUCCESS;
}