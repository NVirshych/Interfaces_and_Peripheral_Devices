#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include <ntddscsi.h>

using namespace std;

void getMemInfo(_ULARGE_INTEGER& clusters, _ULARGE_INTEGER& freeClusters, _ULARGE_INTEGER& busyClusters, int driveNumber)
{
	_ULARGE_INTEGER diskSpace = { 0 }, freeSpace = { 0 };
	string mountPath("\\\\.\\ :"), volumePath(" :\\");

	for (char volumeLetter = 'A'; volumeLetter <= 'Z'; volumeLetter++){

		mountPath[4] = volumeLetter;
		HANDLE logicalDiskHandle = CreateFile(mountPath.c_str(), GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (logicalDiskHandle == INVALID_HANDLE_VALUE) continue;

		volumePath[0] = volumeLetter;

		STORAGE_DEVICE_NUMBER devNum = { 0 };
		DeviceIoControl(logicalDiskHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &devNum, sizeof(devNum), NULL, NULL);
		if (devNum.DeviceNumber != driveNumber) continue;

		GetDiskFreeSpaceExA(volumePath.c_str(), 0, &diskSpace, &freeSpace);
		clusters.QuadPart += diskSpace.QuadPart;
		freeClusters.QuadPart += freeSpace.QuadPart;
		CloseHandle(logicalDiskHandle);
	}
	busyClusters.QuadPart = clusters.QuadPart - freeClusters.QuadPart;
}

int main() {

	char data[400];
	STORAGE_DEVICE_DESCRIPTOR* deviceDesc;
	deviceDesc = (STORAGE_DEVICE_DESCRIPTOR*)&data;
	_ULARGE_INTEGER freeClusters = { 0 }, clusters = { 0 }, busyClusters = { 0 };
	const char* interfaceTypes[] = { "Unknown", "SCSI", "ATAPI", "ATA", "1394", "SSA", "Fibre", "USB", "RAID", "ISCSI", "SAS", "SATA", "SD", "MMC", "Virtual", "FileBackedVirtual",
		"Spaces", "Nvme", "SCM", "Ufs", "Max", "MaxReserved" };

	HANDLE hDevice;
	string deviceName = "\\\\.\\PhysicalDrive0";

	DWORD dwIoControlCode = IOCTL_STORAGE_QUERY_PROPERTY;

	STORAGE_PROPERTY_QUERY query;
	ZeroMemory(&query, sizeof(query));
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;
	LPVOID lpInBuffer = &query;
	DWORD nInBufferSize = sizeof(query);

	LPVOID lpOutBuffer = &data;
	DWORD nOutBufferSize = sizeof(data);

	DWORD BytesReturned = NULL;
	LPOVERLAPPED lpOverlapped = NULL;

	for (int i = 0;; i++) {

		deviceName[18] = i;
		hDevice = CreateFile(deviceName.c_str(), NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
			break;

		cout << "\tDevice '" << deviceName << "'" << endl;

		if (DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, &BytesReturned, lpOverlapped)) {

			printf("VendorID = %d\n", static_cast<int>(*data + deviceDesc->VendorIdOffset));
			printf("Model = %s\n", data + deviceDesc->ProductIdOffset);
			printf("Serial = %s\n", data + deviceDesc->SerialNumberOffset);
			printf("Firmware revision = %s\n", data + deviceDesc->ProductRevisionOffset);
			printf("Interface type: %s\n", interfaceTypes[deviceDesc->BusType]);
		}
		else {

			cout << "Error: " << GetLastError() << endl;
		}

		getMemInfo(clusters, freeClusters, busyClusters, i);
		printf("Size = %lld bytes\n", clusters.QuadPart);
		printf("Free space = %lld bytes\n", freeClusters.QuadPart);
		printf("Busy space = %lld bytes\n", busyClusters.QuadPart);

		CloseHandle(hDevice);
	}

	return 0;
}