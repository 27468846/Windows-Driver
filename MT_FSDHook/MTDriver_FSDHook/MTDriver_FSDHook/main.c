#include <ntddk.h>
#include <ntddkbd.h>

extern POBJECT_TYPE *IoDriverObjectType;
PDRIVER_OBJECT kbdDriver = NULL;

typedef NTSTATUS(*POldReadDispatch)(PDEVICE_OBJECT pDevice, PIRP pIrp);

POldReadDispatch OldReadDispatch = NULL;

// ����΢��δ������ObReferenceObjectByName()����
NTSTATUS ObReferenceObjectByName(
	PUNICODE_STRING ObjectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContest,
	PVOID *Object
);


//�豸ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT pDriver)
{
	DbgPrint("The Driver is Unloading...\n");
	// ж������ʱ�������˻�ԭ
	if (kbdDriver != NULL)
	{
		kbdDriver->MajorFunction[IRP_MJ_READ] = OldReadDispatch;
	}
	return STATUS_SUCCESS;
}

// Hook����
NTSTATUS HookDispatch(PDEVICE_OBJECT pDevice,PIRP pIrp)
{
	// �������л���ʱ���ú����ͻᱻ���ã������Զ���ú����Ĺ��ܣ����������ʾ��ӡһ�仰
	DbgPrint("----Hook KeyBoard Read----\n");
	// ����ٵ��û�ԭ������ǲ���������ü�����������
	return OldReadDispatch(pDevice, pIrp);
}

// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
	pDriver->DriverUnload = DriverUnload;		// ע������ж�غ���

	UNICODE_STRING kbdName = RTL_CONSTANT_STRING(L"\\Driver\\Kbdclass");
	NTSTATUS status = ObReferenceObjectByName(&kbdName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &kbdDriver);	// ��ȡ���������Ķ��󣬱�����kbdDriver
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Open Keyboard Driver Failed\n");
		return status;
	}
	else
	{
		// ������
		ObDereferenceObject(kbdDriver);
	}

	OldReadDispatch = (POldReadDispatch)kbdDriver->MajorFunction[IRP_MJ_READ];		// ���滻֮ǰ�ȱ������������READ��ǲ������ַ���Ա��������
	kbdDriver->MajorFunction[IRP_MJ_READ] = HookDispatch;							// ������������READ��ǲ�����滻Ϊ���ǵ�Hook����

	return status;
}