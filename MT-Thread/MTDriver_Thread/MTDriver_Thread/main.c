#include <ntddk.h>
#include <ntddkbd.h>

PETHREAD pThreadObj = NULL;
BOOLEAN bTerminated = FALSE;


//�豸ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT pDriver)
{
	DbgPrint("The Driver is Unloading...\n");
	// ���ñ�־bTerminatedΪTRUE����������ѭ��
	bTerminated = TRUE;			
	// �ȴ��߳̽���
	KeWaitForSingleObject(pThreadObj, Executive, KernelMode, FALSE, NULL);			
	// ������
	ObDereferenceObject(pThreadObj);				
	return STATUS_SUCCESS;
}


// �߳�������
NTSTATUS TestThread(PVOID pContext)
{
	LARGE_INTEGER inteval;
	// ���ü��ʱ��Ϊ2s
	inteval.QuadPart = -20000000;	
	// inteval.QuadPart = 0;
	while (1)
	{
		// ÿ��2s��ӡһ����Ϣ
		DbgPrint("----TestThread----\n");	
		if (bTerminated)
		{
			// ����־bTerminatedΪTRUEʱ������ѭ��
			break;					
		}
		// ���ߣ��൱��R3������Sleep
		KeDelayExecutionThread(KernelMode, FALSE, &inteval);	
	}
	// ��ֹ�߳�
	PsTerminateSystemThread(STATUS_SUCCESS);			
}


// �̴߳�������
NTSTATUS CreateThread(PVOID TargetEP)
{
	OBJECT_ATTRIBUTES objAddr = { 0 };
	HANDLE threadHandle = 0;
	NTSTATUS status = STATUS_SUCCESS;
	// ��ʼ��һ��OBJECT_ATTRIBUTES ����
	InitializeObjectAttributes(&objAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);			
	// �����߳�
	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAddr, NULL, NULL, TestThread, NULL);		
	if (NT_SUCCESS(status))
	{
		KdPrint(("Thread Created\n"));
		// ͨ���������̵߳Ķ���
		status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj, NULL);		

		// �ͷž��
		ZwClose(threadHandle);			

		if (!NT_SUCCESS(status))
		{
			// ����ȡ����ʧ�ܣ�Ҳ���ñ�־ΪTRUE
			bTerminated = TRUE;			
		}
	}
	return status;
}


// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
	pDriver->DriverUnload = DriverUnload;				// ע������ж�غ���
	NTSTATUS status = status = CreateThread(NULL);		// ����CreateThread�����߳�
	return status;
}