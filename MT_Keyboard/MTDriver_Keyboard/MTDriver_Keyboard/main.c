#include <ntddk.h>
#include <ntddkbd.h>

// �豸��չ�ṹ
typedef struct _Dev_exten
{
	ULONG Size;						// �ýṹ��С
	PDEVICE_OBJECT FilterDevice;	// �����豸����
	PDEVICE_OBJECT TargeDevice;		// ��һ�豸����
	PDEVICE_OBJECT LowDevice;		// ��ײ��豸����
	KSPIN_LOCK IoRequestSpinLock;	// ������
	KEVENT IoInProgressEvent;		// �¼�
	PIRP pIrp;						// IRP
} DEV_EXTENSION, *PDEV_EXTENSION;


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

extern POBJECT_TYPE *IoDriverObjectType;


//�����
NTSTATUS DeAttach(PDEVICE_OBJECT pdevice)
{
	PDEV_EXTENSION devExt;
	devExt = (PDEV_EXTENSION)pdevice->DeviceExtension;

	IoDetachDevice(devExt->TargeDevice);
	devExt->TargeDevice = NULL;
	IoDeleteDevice(pdevice);
	devExt->FilterDevice = NULL;

	return STATUS_SUCCESS;
}


//�豸ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT pDriver)
{
	PDEVICE_OBJECT pDevice;
	PDEV_EXTENSION devExt;

	UNREFERENCED_PARAMETER(pDriver);
	DbgPrint("DriverEntry Unloading...\n");

	pDevice = pDriver->DeviceObject;
	while (pDevice)
	{
		DeAttach(pDevice);
		pDevice = pDevice->NextDevice;
	}

	pDriver->DeviceObject = NULL;

	return STATUS_SUCCESS;
}


// �豸����ͨ�÷ַ�����
NTSTATUS GeneralDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	NTSTATUS status;

	DbgPrint("General Diapatch\n");
	PDEV_EXTENSION devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;
	PDEVICE_OBJECT lowDevice = devExt->LowDevice;
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(lowDevice, pIrp);
	
	return status;
}


// IRP����������ɻص�����
NTSTATUS ReadComp(PDEVICE_OBJECT pDevice, PIRP pIrp, PVOID Context)
{
	NTSTATUS status;
	PIO_STACK_LOCATION stack;
	ULONG keyNumber;
	PKEYBOARD_INPUT_DATA myData;
	stack = IoGetCurrentIrpStackLocation(pIrp);
	if (NT_SUCCESS(pIrp->IoStatus.Status))
	{
		// ��ȡ��������
		myData = pIrp->AssociatedIrp.SystemBuffer;
		keyNumber = (ULONG)(pIrp->IoStatus.Information / sizeof(PKEYBOARD_INPUT_DATA));
		for (ULONG i = 0; i < keyNumber; i++)
		{
			DbgPrint("numkey:%u\n", keyNumber);
			DbgPrint("sancode:%x\n", myData->MakeCode);
			DbgPrint("%s\n", myData->Flags ? "Up" : "Down");
			
			if (myData->MakeCode == 0x1f)
			{
				myData->MakeCode = 0x20;
			}
		}
	}
	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}
	return pIrp->IoStatus.Status;
}


// IRP���ַ�����
NTSTATUS ReadDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
		NTSTATUS status = STATUS_SUCCESS;
		PDEV_EXTENSION devExt;
		PDEVICE_OBJECT lowDevice;
		PIO_STACK_LOCATION stack;
		if (pIrp->CurrentLocation == 1)
		{
			DbgPrint("irp send error..\n");
			status = STATUS_INVALID_DEVICE_REQUEST;
			pIrp->IoStatus.Status = status;
			pIrp->IoStatus.Information = 0;
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			return status;
		}
		// �õ��豸��չ��Ŀ����֮��Ϊ�˻����һ���豸��ָ�롣
		devExt = pDevice->DeviceExtension;
		lowDevice = devExt->LowDevice;
		stack = IoGetCurrentIrpStackLocation(pIrp);

		// ����IRPջ
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		// ����IRP��ɻص�����
		IoSetCompletionRoutine(pIrp, ReadComp, pDevice, TRUE, TRUE, TRUE);
		status = IoCallDriver(lowDevice, pIrp);
		return status;
}


// ��ԴIRP�ַ�����
NTSTATUS PowerDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	PDEV_EXTENSION devExt;
	devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;

	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	return PoCallDriver(devExt->TargeDevice, pIrp);
}


// ���弴��IRP�ַ�����
NTSTATUS PnPDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	PDEV_EXTENSION devExt;
	PIO_STACK_LOCATION stack;
	NTSTATUS status = STATUS_SUCCESS;

	devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(pIrp);

	switch (stack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE :
		// ���Ȱ�������ȥ
		IoSkipCurrentIrpStackLocation(pIrp);
		IoCallDriver(devExt->LowDevice, pIrp);
		// Ȼ�����󶨡�
		IoDetachDevice(devExt->LowDevice);
		// ɾ�������Լ����ɵ������豸��
		IoDeleteDevice(pDevice);
		status = STATUS_SUCCESS;
		break;

	default :
		// �����������͵�IRP��ȫ����ֱ���·����ɡ� 
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->LowDevice, pIrp);
	}
	return status;
}


// ��ʼ����չ�豸
NTSTATUS DevExtInit(PDEV_EXTENSION devExt, PDEVICE_OBJECT filterDevice, PDEVICE_OBJECT targetDevice, PDEVICE_OBJECT lowDevice)
{
	memset(devExt, 0, sizeof(DEV_EXTENSION));
	devExt->FilterDevice = filterDevice;
	devExt->TargeDevice = targetDevice;
	devExt->LowDevice = lowDevice;
	devExt->Size = sizeof(DEV_EXTENSION);
	KeInitializeSpinLock(&devExt->IoRequestSpinLock);
	KeInitializeEvent(&devExt->IoInProgressEvent, NotificationEvent, FALSE);
	return STATUS_SUCCESS;
}

// �������豸�󶨵�Ŀ���豸��
NTSTATUS AttachDevice(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPatch)
{
	UNICODE_STRING kbdName = RTL_CONSTANT_STRING(L"\\Driver\\Kbdclass");
	NTSTATUS status = 0;
	PDEV_EXTENSION devExt;			// �����豸����չ�豸
	PDEVICE_OBJECT filterDevice;	// �����豸 
	PDEVICE_OBJECT targetDevice;		// Ŀ���豸�������豸��
	PDEVICE_OBJECT lowDevice;		// �ײ��豸����ĳһ���豸�ϼ�һ���豸ʱ��һ���Ǽӵ����豸�ϣ��������豸ջ��ջ����
	PDRIVER_OBJECT kbdDriver;		// ���ڽ��մ򿪵����������豸

	// ��ȡ���������Ķ��󣬱�����kbdDriver
	status = ObReferenceObjectByName(&kbdName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &kbdDriver);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Open KeyBoard Driver Failed\n");
		return status;
	}
	else
	{
		// ������
		ObDereferenceObject(kbdDriver);
	}

	// ��ȡ���������豸���еĵ�һ���豸
	targetDevice = kbdDriver->DeviceObject;
	// ����������һ�����������̼����豸���е������豸
	while (targetDevice)
	{
		// ����һ�������豸
		status = IoCreateDevice(pDriver, sizeof(DEV_EXTENSION), NULL, targetDevice->DeviceType, targetDevice->Characteristics, FALSE, &filterDevice);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("Create New FilterDevice Failed\n");
			filterDevice = targetDevice = NULL;
			return status;
		}
		// �󶨣�lowDevice�ǰ�֮��õ�����һ���豸��
		lowDevice = IoAttachDeviceToDeviceStack(filterDevice, targetDevice);
		if (!lowDevice)
		{
			DbgPrint("Attach Faided!\n");
			IoDeleteDevice(filterDevice);
			filterDevice = NULL;
			return status;
		}
		// ��ʼ���豸��չ
		devExt = (PDEV_EXTENSION)filterDevice->DeviceExtension;
		DevExtInit(devExt, filterDevice, targetDevice, lowDevice);

		filterDevice->DeviceType = lowDevice->DeviceType;
		filterDevice->Characteristics = lowDevice->Characteristics;
		filterDevice->StackSize = lowDevice->StackSize + 1;
		filterDevice->Flags |= lowDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
		// ������һ���豸
		targetDevice = targetDevice->NextDevice;
	}
	DbgPrint("Create And Attach Finshed...\n");
	return status;
}


// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPatch)
{
	ULONG i;
	NTSTATUS status = STATUS_SUCCESS;

	pDriver->DriverUnload = DriverUnload;					// ע������ж�غ���

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriver->MajorFunction[i] = GeneralDispatch;		// ע��ͨ�õ�IRP�ַ�����
	}

	pDriver->MajorFunction[IRP_MJ_READ] = ReadDispatch;		// ע���IRP�ַ�����
	pDriver->MajorFunction[IRP_MJ_POWER] = PowerDispatch;	// ע���ԴIRP�ַ�����
	pDriver->MajorFunction[IRP_MJ_PNP] = PnPDispatch;		// ע�ἴ�弴��IRP�ַ�����

	AttachDevice(pDriver, RegPatch);						// ���豸

	return status;
}