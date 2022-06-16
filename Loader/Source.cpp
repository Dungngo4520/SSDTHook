#include <stdio.h>
#include <Windows.h>

int main() {
	SC_HANDLE hSCManager, hService;
	SERVICE_STATUS ss;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

	printf("Load Driver\n");

	if (hSCManager) {
		printf("Create Service\n");

		hService = CreateService(hSCManager,
			L"SSDTHook",
			L"SSDTHook Driver",
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_IGNORE,
			L"SSDTHook.sys",
			NULL, NULL, NULL, NULL, NULL);

		if (!hService) {
			hService = OpenService(hSCManager, L"SSDTHook", SERVICE_START);
		}

		if (hService) {
			printf("Start Service\n");

			StartService(hService, 0, NULL);
			printf("Press Enter to close service\r\n");
			getchar();
			ControlService(hService, SERVICE_CONTROL_STOP, &ss);

			CloseServiceHandle(hService);

			DeleteService(hService);
		}

		CloseServiceHandle(hSCManager);
	}

	return 0;
}