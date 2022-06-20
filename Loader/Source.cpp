
#include <stdio.h>
#include <Windows.h>

using namespace std;

int main() {
	SC_HANDLE hSCManager, hService;
	SERVICE_STATUS ss;
	PWCHAR currentDir = NULL;

	printf("Loading Driver\n");



	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	currentDir = (PWCHAR)calloc(BUFSIZ, sizeof(WCHAR));

	if (hSCManager && currentDir) {
		printf("Creating Service\n");

		GetCurrentDirectoryW(BUFSIZ, currentDir);

		printf("Current Dir: %ls\n", currentDir);

		wcscat_s(currentDir,BUFSIZ, L"\\SSDTHook.sys");

		hService = CreateServiceW(
			hSCManager,
			L"SSDTHook",
			L"SSDTHook Driver",
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_IGNORE,
			currentDir,
			NULL, NULL, NULL, NULL, NULL);



		if (hService == NULL && GetLastError() == ERROR_SERVICE_EXISTS) {
			printf("Service Existed. Attempting to open...\n");

			hService = OpenServiceW(hSCManager, L"SSDTHook", SERVICE_ALL_ACCESS);
		}

		if (hService) {
			printf("Start Service\n");
			if (!StartService(hService, 0, NULL)) {
				printf("Failed to start service. %d\n", GetLastError());
			}

			printf("ZwQueryDirectoryFile Hooked.\n");
			system("pause");

			if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
				printf("Failed to control service. %d\n", GetLastError());
			}
			if (!DeleteService(hService)) {
				printf("Failed to delete service. %d\n", GetLastError());
			}

			CloseServiceHandle(hService);
		}
		else {
			printf("Failed to get service handle. %d\n", GetLastError());
		}

		CloseServiceHandle(hSCManager);
	}
	else {
		printf("Failed to get service manager handle. %d\n", GetLastError());
	}

	return 0;
}