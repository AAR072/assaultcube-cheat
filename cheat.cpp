#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cwchar>
#include <vector>
#include <windows.h>
#include <TlHelp32.h>
// nop an area in memeory given start and end address and the handle of process with 0's in the area
void nop(DWORD start, DWORD end, HANDLE hProcess)
{
    DWORD oldProtect;
    VirtualProtectEx(hProcess, (LPVOID)start, end - start, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset((LPVOID)start, 0x90, end - start);
    VirtualProtectEx(hProcess, (LPVOID)start, end - start, oldProtect, &oldProtect);
}
// write over an area in memory given the start and end address
int toDec(int convertME)
{
    //std::cout << "Converting " << std::hex<<convertME << " to decimal..." << std::endl;
    std::string decMe = std::to_string(convertME);
    //std::cout << "Decimal: " << std::hex <<decMe << std::endl;
    int returnME = std::stoi(decMe);
    //std::cout << "Decimal: " << std::dec << returnME << std::endl;
    return convertME;
}
// Get process id from name
DWORD GetProcId(const wchar_t *procName)
{
    PROCESSENTRY32W procEntry;
    procEntry.dwSize = sizeof(procEntry);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    if (Process32FirstW(hSnap, &procEntry))
    {
        do
        {
            if (std::wcscmp(procEntry.szExeFile, procName) == 0)
            {
                CloseHandle(hSnap);
                return procEntry.th32ProcessID;
            }
        } while (Process32NextW(hSnap, &procEntry));
    }

    CloseHandle(hSnap);
    return 0;
}
// Get base address of module from process id and module name
uintptr_t GetModuleBaseAddress(DWORD procId, wchar_t *modName)
{
    uintptr_t baseAddress = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {

        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);

        if (Module32FirstW(hSnap, &modEntry))
        {
            do
            {
                if (std::wcscmp(modEntry.szModule, modName) == 0)
                {
                    baseAddress = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return baseAddress;
}
// get dma address
uintptr_t FindDMAAddress(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets)
{
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); i++)
    {
        ReadProcessMemory(hProc, (BYTE *)addr, &addr, sizeof(addr), 0);
        addr += offsets[i];
    }
    return addr;
}






void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess)
{
	DWORD oldprotect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	VirtualProtectEx(hProcess, dst, size, oldprotect, &oldprotect);
}



void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess)
{
	BYTE* nopArray = new BYTE[size];
	memset(nopArray, 0x90, size);

	PatchEx(dst, nopArray, size, hProcess);
	delete[] nopArray;
}









int main()
{
    float oldY = 0;
    bool toggleAmmo = false;
    bool toggleHealth = false;
    bool isRunning = true;
    bool toggleFlight = false;
    bool toggleArmor = false;
    // Get process id from name
    DWORD procId = GetProcId(L"ac_client.exe");

    if (!procId)
    {
        std::cout << "Could not find process" << std::endl;
    }
    else
    {
        std::cout << "Process found" << std::endl;
        // print process id
        std::cout << "Process id: " << procId << std::endl;
        // get module base address
        uintptr_t baseAddress = GetModuleBaseAddress(procId, L"ac_client.exe");
        // get handle to process
        HANDLE hProc = 0;
        hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

        // resolve base address
        uintptr_t dynamicPtrBase = baseAddress + 0x195404;
        std::cout << "Dynamic base address: 0x" << std::hex << dynamicPtrBase << std::endl;

        // resolve offsets
        std::vector<unsigned int> ammoffsets = {0x140};
        uintptr_t ammPtr = FindDMAAddress(hProc, dynamicPtrBase, ammoffsets);
        std::cout << "Ammo address: 0x" << std::hex << ammPtr << std::endl;

        std::vector<unsigned int> healthoffsets = {0xEC};
        uintptr_t healthPtr = FindDMAAddress(hProc, dynamicPtrBase, healthoffsets);
        std::cout << "Health address: 0x" << std::hex << healthPtr << std::endl;

        std::vector<unsigned int> xoffsets = {0x28};
        uintptr_t xPtr = FindDMAAddress(hProc, dynamicPtrBase, xoffsets);
        std::cout << "X address: 0x" << std::hex << xPtr << std::endl;

        std::vector<unsigned int> yoffsets = {0x30};
        uintptr_t yPtr = FindDMAAddress(hProc, dynamicPtrBase, yoffsets);
        std::cout << "Y address: 0x" << std::hex << yPtr << std::endl;

        std::vector<unsigned int> zoffsets = {0x2C};
        uintptr_t zPtr = FindDMAAddress(hProc, dynamicPtrBase, zoffsets);
        std::cout << "Z address: 0x" << std::hex << zPtr << std::endl;

        // read ammo value
        int ammoValue = 0;
        ReadProcessMemory(hProc, (BYTE *)ammPtr, &ammoValue, sizeof(ammoValue), 0);
        std::cout << "Ammo value hex: 0x" << std::hex << ammoValue << std::endl;
        std::cout << "Ammo value dec: " << std::dec << ammoValue << std::endl;

        // read health value
        int healthValue = 0;
        ReadProcessMemory(hProc, (BYTE *)healthPtr, &healthValue, sizeof(healthValue), 0);
        std::cout << "Health value hex: 0x" << std::hex << healthValue << std::endl;
        std::cout << "Health value dec: " << std::dec << healthValue << std::endl;

        // read armor value
        int armorValue = 0;
        ReadProcessMemory(hProc, (BYTE *)healthPtr + 0x4, &armorValue, sizeof(armorValue), 0);
        std::cout << "Armor value hex: 0x" << std::hex << armorValue << std::endl;
        std::cout << "Armor value dec: " << std::dec << armorValue << std::endl;

        // read x value
        float xValue = 0;
        ReadProcessMemory(hProc, (BYTE *)xPtr, &xValue, sizeof(xValue), 0);
        std::cout << "X value hex: 0x" << std::hex << xValue << std::endl;
        std::cout << "X value dec: " << std::dec << xValue << std::endl;

        // read y value
        float yValue = 0;
        ReadProcessMemory(hProc, (BYTE *)yPtr, &yValue, sizeof(yValue), 0);
        std::cout << "Y value hex: 0x" << std::hex << yValue << std::endl;
        std::cout << "Y value dec: " << std::dec << yValue << std::endl;
    
        // read z value
        float zValue = 0;
        ReadProcessMemory(hProc, (BYTE *)zPtr, &zValue, sizeof(zValue), 0);
        std::cout << "Z value hex: 0x" << std::hex << zValue << std::endl;
        std::cout << "Z value dec: " << std::dec << zValue << std::endl;

        oldY = ReadProcessMemory(hProc, (BYTE *)yPtr, &yValue, sizeof(yValue), 0);
        // write value
        while (isRunning)
        {   
            // Remove recoil
            NopEx((BYTE*)(baseAddress + 0xC2EC3), 5, hProc);
            
            // if f1 is pressed
            if(GetKeyState(VK_F1) & 0x8000)
            {
                // toggle ammo
                toggleAmmo = !toggleAmmo;
                // wait 0.25 seconds
                Sleep(250);
            }
            if (toggleAmmo)
                {
                    int newAmmoValue = 69420;
                    WriteProcessMemory(hProc, (BYTE *)ammPtr, &newAmmoValue, sizeof(newAmmoValue), 0); 
                }
            // if f2 is pressed
            if(GetKeyState(VK_F2) & 0x8000)
            {
                // toggle health
                toggleHealth = !toggleHealth;
                // wait 0.25 seconds
                Sleep(250);
            }
            if (toggleHealth)
                {
                    int newHealthValue = 69420;
                    WriteProcessMemory(hProc, (BYTE *)healthPtr, &newHealthValue, sizeof(newHealthValue), 0); 
                }
            // if f3 is pressed
            if(GetKeyState(VK_F3) & 0x8000)
            {
                // toggle armor
                toggleArmor = !toggleArmor;
                // wait 0.25 seconds
                Sleep(250);
            }
            if (toggleArmor)
                {
                    int newArmorValue = 69420;
                    WriteProcessMemory(hProc, (BYTE *)healthPtr + 0x4, &newArmorValue, sizeof(newArmorValue), 0); 
                }
            // if f11 is pressed
            if(GetKeyState(VK_F11) & 0x8000)
            {
            std::cout << "Enter x value to write: ";
            float newXValue = 0;
            std::cin >> newXValue;
            std::cout << "Enter z value to write: ";
            float newZValue = 0;
            std::cin >> newZValue;
            std::cout << "Enter y value to write: ";
            float newYValue = 0;
            std::cin >> newYValue;
            WriteProcessMemory(hProc, (BYTE *)xPtr, &newXValue, sizeof(newXValue), 0);
            WriteProcessMemory(hProc, (BYTE *)yPtr, &newYValue, sizeof(newYValue), 0);
            WriteProcessMemory(hProc, (BYTE *)zPtr, &newZValue, sizeof(newZValue), 0);
            }
            // if f4 is pressed
            if(GetKeyState(VK_F4) & 0x8000)
            {
                // toggle flight
                toggleFlight = !toggleFlight;
                Sleep(250);
            }
            if (toggleFlight)
                {
                    WriteProcessMemory(hProc, (BYTE *)yPtr, &yValue, sizeof(yValue), 0);
                    // if space is pressed
                    if(GetKeyState(VK_SPACE) & 0x8000)
                    {
                        yValue += 0.001;
                        WriteProcessMemory(hProc, (BYTE *)yPtr, &yValue, sizeof(yValue), 0);

                    }
                    // if left shift is pressed
                    if(GetKeyState(VK_LSHIFT) & 0x8000)
                    {
                        // decrease y value by 0.2
                        yValue -= 0.001;
                        WriteProcessMemory(hProc, (BYTE *)yPtr, &yValue, sizeof(yValue), 0);
                    }
                }
            
            
            // if f12 is pressed exit
            if(GetKeyState(VK_F12) & 0x8000)
            {
                isRunning = false;
            }
    }
    }


    // wait for key press
    std::cout << "Press enter key to continue" << std::endl;
    std::getchar();
}