#include <ntddk.h>
#include <windef.h>

#define SECOND_OF_DAY 86400

UINT8 DayOfMon[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
ULONG BanedTime = 1568431212;	// 2019.9.14 11:20:12

extern POBJECT_TYPE* PsThreadType;
PETHREAD pThreadObj = NULL;
BOOLEAN TimeSwitch = FALSE;

// ����ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT dDriver)
{
	TimeSwitch = TRUE;
	// �ȴ��߳��˳�
	KeWaitForSingleObject(pThreadObj, Executive, KernelMode, FALSE, NULL);
	ObReferenceObject(pThreadObj);
	return STATUS_SUCCESS;
}

// У��ʱ�亯��
BOOLEAN CheckTimeLocal()
{
	LARGE_INTEGER snow, now, tickcount;
	TIME_FIELDS now_fields;

	// ��ȡ��׼ʱ��
	KeQuerySystemTime(&snow);

	// ת��Ϊ����ʱ��
	ExSystemTimeToLocalTime(&snow, &now);

	// ������ꡢ�¡��ա�ʱ���֡���
	RtlTimeToTimeFields(&now, &now_fields);

	// ��ӡ������
	DbgPrint("��ǰʱ�䣺%d-%d-%d\n", now_fields.Year, now_fields.Month, now_fields.Day);
	
	SHORT i, Cyear = 0;
	ULONG CountDay = 0;

	// ����ʱ����㷨
	for ( i = 1970; i < now_fields.Year; i++)
	{
		if ((i % 4 == 0) && (i % 100 != 0) || (i % 400 == 0))
		{
			Cyear++;
		}
	}
	CountDay = Cyear * 366 + (now_fields.Year - 1970 - Cyear) * 365;
	for ( i = 1; i < now_fields.Month; i++)
	{
		if ((i == 2) && (((now_fields.Year % 4 == 0) && (now_fields.Year % 100 != 0)) || (now_fields.Year % 400 == 0)))
		{
			CountDay += 29;
		}
		else
		{
			CountDay += DayOfMon[i - 1];
		}
		CountDay += (now_fields.Day - 1);

		CountDay = CountDay * SECOND_OF_DAY + (unsigned long)now_fields.Hour * 3600 + (unsigned long)now_fields.Minute * 60 + now_fields.Second;

		// �Ա�ʱ���
		DbgPrint("ʱ��� ��%d", CountDay);
		if (CountDay < BanedTime)
		{
			return TRUE;
		}
		return FALSE;
	}
}

// ʱ��У���߳�
VOID CheckTimeThread()
{
	LARGE_INTEGER SleepTime;
	SleepTime.QuadPart = -20000000;

	DbgPrint("Enter The Thread\n");
	while (1)
	{
		if (TimeSwitch)
		{
			break;
		}
		if (!CheckTimeLocal())
		{
			DbgPrint("������Ч\n");
		}
		KeDelayExecutionThread(KernelMode, FALSE, &SleepTime);
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
	pDriver->DriverUnload = DriverUnload;		// ע������ж�غ���

	OBJECT_ATTRIBUTES ObjAddr = { 0 };
	HANDLE ThreadHandle = 0;
	// ��ʼ������
	InitializeObjectAttributes(&ObjAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	// �����߳�
	NTSTATUS status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, &ObjAddr, NULL, NULL, CheckTimeThread, NULL);
	if (!NT_SUCCESS(status))
	{
		return STATUS_NOT_SUPPORTED;
	}
	// ��ȡ�̶߳���
	status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj, NULL);
	if (!NT_SUCCESS(status))
	{
		ZwClose(ThreadHandle);
		return STATUS_NOT_SUPPORTED;
	}
	ZwClose(ThreadHandle);

	DbgPrint("������ʼ����\n");

	return STATUS_SUCCESS;
}