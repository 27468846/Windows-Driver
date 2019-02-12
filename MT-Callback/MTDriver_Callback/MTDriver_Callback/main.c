#include <ntifs.h>

// �ص�����CreateProcCallback
VOID CreateProcCallback(HANDLE ParentID, HANDLE ProcessID, BOOLEAN Create)
{
	if (Create)
	{
		PEPROCESS Process = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(ProcessID, &Process);					// ����PID��ȡ���̽ṹ��ĵ�ַ
		int i;
		if (NT_SUCCESS(status))
		{
			for (i = 0; i < 3 * PAGE_SIZE; i++)
			{
				if (!strncmp("notepad.exe", (PCHAR)Process + i, strlen("notepad.exe")))		// �жϽ������Ƿ�Ϊ��notepad.exe��
				{
					DbgPrint("Proces %s is created!\n", (PCHAR)((ULONG)Process + i));
					break;
				}
			}
		}
	}
}

//�豸ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT pDriver)
{
	NTSTATUS stats = PsSetCreateProcessNotifyRoutine(CreateProcCallback, TRUE);
	return STATUS_SUCCESS;
}


// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
	DbgPrint("Enter the driver\n");

	pDriver->DriverUnload = DriverUnload;

	NTSTATUS stats = PsSetCreateProcessNotifyRoutine(CreateProcCallback, FALSE);		// ע����̴����¼��Ļص�����CreateProcCallback

	return stats;
}