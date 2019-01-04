#include <ntddk.h>
#include <windef.h>

PVOID updatetimeAddr = NULL;

PVOID querycounterAddr = NULL;

const DWORD g_dwSpeedBase = 100;		// ���ٻ���
DWORD g_dwSpeed_X = 1000;				// ������ֵ


LARGE_INTEGER g_originCounter;

LARGE_INTEGER g_returnCounter;

// ����ж�غ���
NTSTATUS DriverUnload(PDRIVER_OBJECT dDriver)
{
	return STATUS_SUCCESS;
}


// ����ڴ�д���Ժ���
void __declspec(naked) WPOFF()
{
	__asm
	{
		cli
		mov eax, cr0
		and eax, not 0x10000
		mov cr0, eax
		ret
	}
}


// ȥ���ڴ�д���Ժ���
void __declspec(naked) WPON()
{
	__asm
	{
		mov eax, cr0
		or eax, 0x10000
		mov cr0, eax
		sti
		ret
	}
}


// KeUpdateSystemTime�ı��ݺ���
void __declspec(naked) __cdecl updatetimeOriginCode()
{
	__asm
	{
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		mov esi, updatetimeAddr
		add esi, 7
		jmp esi
	}
}


// KeQueryPerformanceCounter�ı��ݺ���
LARGE_INTEGER __declspec(naked) __stdcall querycounterOriginCode(OUT PLARGE_INTEGER PerformanceFrequency)
{
	__asm
	{
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		mov eax, querycounterAddr
		add eax, 5
		jmp eax
	}
}


// KeUpdateSystemTime��Hook����
void __declspec(naked) __cdecl fakeupdatetimeAddr()
{
	__asm
	{

		mul g_dwSpeed_X							// �ڵ���KeUpdateSystemTime֮ǰ�Բ���EAX�����޸�
		div g_dwSpeedBase						// ʵ�ֱ��٣�EAX * ��ǰ�ٶ�ֵ / �ٶȻ�����
		jmp updatetimeOriginCode
	}
}


// KeQueryPerformanceCounter��Hook����
LARGE_INTEGER __stdcall fakequerycounterAddr(OUT PLARGE_INTEGER PerformanceFrequency)
{
	LARGE_INTEGER realTime;
	LARGE_INTEGER fakeTime;

	realTime = querycounterOriginCode(PerformanceFrequency);		// ��ȡ��ǰʱ��

	fakeTime.QuadPart = g_returnCounter.QuadPart + (realTime.QuadPart - g_originCounter.QuadPart) * g_dwSpeed_X / g_dwSpeedBase;	// ����α��ʱ��

	g_originCounter.QuadPart = realTime.QuadPart;	// ����ԭʼʱ��
	g_returnCounter.QuadPart = fakeTime.QuadPart;		// ����α��ʱ��

	return fakeTime;
}


// ������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
	pDriver->DriverUnload = DriverUnload;		// ע������ж�غ���

	UNICODE_STRING updatetimeName = RTL_CONSTANT_STRING(L"KeUpdateSystemTime");
	updatetimeAddr = MmGetSystemRoutineAddress(&updatetimeName);				// ��ȡKeUpdateSystemTime�ĵ�ַ
	UNICODE_STRING querycounterName = RTL_CONSTANT_STRING(L"KeQueryPerformanceCounter");
	querycounterAddr = MmGetSystemRoutineAddress(&querycounterName);			// ��ȡKeQueryPerformanceCounter�ĵ�ַ

	g_originCounter.QuadPart = 0;
	g_returnCounter.QuadPart = 0;
	g_originCounter = KeQueryPerformanceCounter(NULL);
	g_returnCounter.QuadPart = g_originCounter.QuadPart;	// �ڱ���ǰ�Ȼ�ȡ�µ�ǰϵͳʱ��

	BYTE updatetimeJmpCode[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };		// KeUpdateSystemTime��JmpCode
	BYTE querycounterJmpCode[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };		// KeQueryPerformanceCounter��JmpCode
	*(DWORD*)(updatetimeJmpCode + 1) = (DWORD)fakeupdatetimeAddr - ((DWORD)updatetimeAddr + 5);				// ������תƫ��
	*(DWORD*)(querycounterJmpCode + 1) = (DWORD)fakequerycounterAddr - ((DWORD)querycounterAddr + 5);		// ������תƫ��

	WPOFF();		// �޸ĵ�ǰ���̣�system�����ڴ�����Ϊ��д
	KIRQL Irql = KeRaiseIrqlToDpcLevel();		// ����жϼ���������������
	RtlCopyMemory(updatetimeOriginCode, updatetimeAddr, 7);				// ��KeUpdateSystemTimeԭʼ�����ǰ5�ֽڱ��ݵ�updatetimeOriginCode
	RtlCopyMemory((BYTE*)updatetimeAddr, updatetimeJmpCode, 5);			// ��JmpCode���ǵ�KeUpdateSystemTime��������ʼ��ַ��

	RtlCopyMemory(querycounterOriginCode, querycounterAddr, 5);			// ��KeQueryPerformanceCounterԭʼ�����ǰ5�ֽڱ��ݵ�querycounterOriginCode
	RtlCopyMemory((BYTE*)querycounterAddr, querycounterJmpCode, 5);		// ��JmpCode���ǵ�KeQueryPerformanceCounter��������ʼ��ַ��

	KeLowerIrql(Irql);		// ��ԭ�жϼ�
	WPON();					// ��ԭ�ڴ�����

	return STATUS_SUCCESS;
}