#include <ntddk.h>

// �����豸���ͷ�����
#define DEVICE_NAME L"\\Device\\MTReadDevice"
#define SYM_LINK_NAME L"\\??\\MTRead"

PDEVICE_OBJECT pDevice;
UNICODE_STRING DeviceName;
UNICODE_STRING SymLinkName;

// �豸��������
NTSTATUS DeviceCreate(PDEVICE_OBJECT Device, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	// I/O���������
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	DbgPrint("Create Device Success\n");
	return STATUS_SUCCESS;
}

// �豸����������
NTSTATUS DeviceRead(PDEVICE_OBJECT Device, PIRP pIrp)
{
	// ��ȡָ��IRP�Ķ�ջ��ָ��
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	// ��ȡ��ջ����
	ULONG length = stack->Parameters.Read.Length;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = length;
	// ����ջ�ϵ�����ȫ����Ϊ0xAA
	memset(pIrp->AssociatedIrp.SystemBuffer, 0xAA, length);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	DbgPrint("Read Device Success\n");
	return STATUS_SUCCESS;
}

// �豸�رպ���
NTSTATUS DeviceClose(PDEVICE_OBJECT Device, PIRP pIrp)
{
	// ���豸����������ͬ
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	DbgPrint("Close Device Success\n");
	return STATUS_SUCCESS;
}

// ����ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT Driver)
{
	NTSTATUS status;

	// ɾ�����ź��豸
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(pDevice);
	DbgPrint("This Driver Is Unloading...\n");
	return STATUS_SUCCESS;
}

// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING RegPath)
{
	NTSTATUS status;
	
	// ע���豸�����������豸���������豸�رպ���������ж�غ���
	Driver->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
	Driver->MajorFunction[IRP_MJ_READ] = DeviceRead;
	Driver->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
	Driver->DriverUnload = DriverUnload;

	// ���豸��ת��ΪUnicode�ַ���
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	// �����豸����
	status = IoCreateDevice(Driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, NULL, &pDevice);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create Device Faild!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// ��������ת��ΪUnicode�ַ���
	RtlInitUnicodeString(&SymLinkName, SYM_LINK_NAME);
	// ���������豸����
	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Create SymLink Faild!\n");
		IoDeleteDevice(pDevice);
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("Initialize Success\n");

	// ����pDevice�Ի�������ʽ��ȡ
	pDevice->Flags = DO_BUFFERED_IO;

	return STATUS_SUCCESS;
}