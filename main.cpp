#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include "resource1.h"

#pragma warning(disable : 4996)

DWORD g_dwProcessId = 0;
HANDLE g_hProcess = NULL;

// Message handler prototype
BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

// Function to terminate the process if it's running
void TerminateRunningProcess() {
    if (g_dwProcessId != 0 && g_hProcess != NULL) {
        TerminateProcess(g_hProcess, 0);
        CloseHandle(g_hProcess);
        g_hProcess = NULL;
    }
}

bool BrowseForFileName(HWND hWnd, char* fileName) {
    OPENFILENAME ofn = { 0 };
    char szFileName[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = "exe";

    if (GetOpenFileNameA(&ofn)) {
        // Copy the selected file name to the output parameter 'fileName'
        strcpy(fileName, szFileName);
        return true;
    }

    return false;
}


// Function to update button states based on process status
void UpdateButtonStates(HWND hWnd) {
    // Check if a process is running or not
    if (g_dwProcessId != 0) {
        DWORD dwExitCode;
        if (GetExitCodeProcess(g_hProcess, &dwExitCode)) {
            if (dwExitCode == STILL_ACTIVE) {
                // Process is running, disable IDC_START and enable IDC_TERMINATE
                EnableWindow(GetDlgItem(hWnd, IDC_START), FALSE);
                EnableWindow(GetDlgItem(hWnd, IDC_TERMINATE), TRUE);
            }
            else {
                // Process has finished, or it was not started, enable IDC_START and disable IDC_TERMINATE
                EnableWindow(GetDlgItem(hWnd, IDC_START), TRUE);
                EnableWindow(GetDlgItem(hWnd, IDC_TERMINATE), FALSE);
            }
        }
    }
    else {
        // No process is running, enable IDC_START and disable IDC_TERMINATE
        EnableWindow(GetDlgItem(hWnd, IDC_START), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_TERMINATE), FALSE);
    }
}

void UpdateProcessInformation(HWND hWnd) {
    if (g_dwProcessId != 0) {
        DWORD dwExitCode;
        if (GetExitCodeProcess(g_hProcess, &dwExitCode)) {
            if (dwExitCode == STILL_ACTIVE) {
                // Process is running
                SetDlgItemTextA(hWnd, IDC_STATE, "Running");

                // Display the process ID
                SetDlgItemInt(hWnd, IDC_PROCESSID, g_dwProcessId, FALSE);

                // Get the process start time
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(g_hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    SYSTEMTIME startTime;
                    FileTimeToSystemTime(&createTime, &startTime);

                    char startTimeStr[64];
                    sprintf(startTimeStr, "%02d:%02d:%02d",
                        startTime.wHour, startTime.wMinute, startTime.wSecond);

                    SetDlgItemTextA(hWnd, IDC_STARTTIME, startTimeStr);
                }
                else {
                    SetDlgItemTextA(hWnd, IDC_STARTTIME, "");
                }

                SetDlgItemTextA(hWnd, IDC_FINISHTIME, ""); // Clear finish time
            }
            else {
                // Process has finished
                SetDlgItemTextA(hWnd, IDC_STATE, "Finished");
                SetDlgItemTextA(hWnd, IDC_PROCESSID, "");
                SetDlgItemTextA(hWnd, IDC_STARTTIME, "");

                // Get the process exit time
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(g_hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    SYSTEMTIME finishTime;
                    FileTimeToSystemTime(&exitTime, &finishTime);

                    char finishTimeStr[64];
                    sprintf(finishTimeStr, "%02d:%02d:%02d",
                        finishTime.wHour, finishTime.wMinute, finishTime.wSecond);

                    SetDlgItemTextA(hWnd, IDC_FINISHTIME, finishTimeStr);
                }
                else {
                    SetDlgItemTextA(hWnd, IDC_FINISHTIME, "");
                }
            }
        }
    }
    else {
        // No process is running
        SetDlgItemTextA(hWnd, IDC_STATE, "");
        SetDlgItemTextA(hWnd, IDC_PROCESSID, "");
        SetDlgItemTextA(hWnd, IDC_STARTTIME, "");
        SetDlgItemTextA(hWnd, IDC_FINISHTIME, "");
    }
}

// Main function
// If the main window template has the resource identifier IDD_MAINDIALOG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        DLGPROC(MainWndProc)
    );
    return 0;
}

// Message handler
BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {

    case WM_INITDIALOG: // Inicializācijas paziņojums (Initialization message)
        SetTimer(hWnd, 1, 250, NULL); // Noskaņot taimeru (Set up the timer)
        return TRUE;

    case WM_COMMAND: // Paziņojums no vadības elementa (Message from a control)
        switch (LOWORD(wParam)) { // Pārbaudīt vadības elementa ID (Check control ID)
        case IDC_START: // Ja bija uzspiesta Start poga (If the Start button was pressed)
            g_dwProcessId = 0; // Iestatām sākumvērtību 0 (Set the initial value to 0)

            // Iegūstam ievades lauka tekstu un saglabājam to mainīgajā szCommandLine (Get the text from the input field and store it in the szCommandLine variable)
            char szCommandLine[MAX_PATH];
            GetDlgItemTextA(hWnd, IDC_COMMANDLINE, szCommandLine, MAX_PATH);

            STARTUPINFOA si; 
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            // Izveidojam jaunu procesu, izmantojot iegūto komandrindu (Create a new process using the provided command line)
            if (CreateProcessA(NULL, szCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                // Aizvērt iepriekšējā procesa turu, ja tas nav vienāds nullei (Close the previous process handle if it exists)
                TerminateRunningProcess();

                // Saglabāt jauna procesa turu globālajā mainīgajā (Save the new process handle in the global variable)
                g_dwProcessId = pi.dwProcessId;
                g_hProcess = pi.hProcess; // Store the process handle
                CloseHandle(pi.hThread);
            }

            UpdateButtonStates(hWnd); // Update button states based on process status
            return TRUE;

        case IDC_TERMINATE:
            TerminateRunningProcess();
            UpdateButtonStates(hWnd);
            return TRUE;

        case IDC_BROWSE:
            char fileName[MAX_PATH];
            if (BrowseForFileName(hWnd, fileName)) {
                // Update the command line field with the selected file name
                SetDlgItemTextA(hWnd, IDC_COMMANDLINE, fileName);
            }
            return TRUE;


        case IDOK: // Ja bija uzspiesta OK poga (If the OK button was pressed)
            DestroyWindow(hWnd); // Aizvērt logu (Close the window)
            return TRUE;
        }
        return FALSE;

    case WM_TIMER:
        // Atjauno procesa stāvokli (Update process status)
        UpdateButtonStates(hWnd); // Update button states based on process status
        UpdateProcessInformation(hWnd);
        return TRUE;

    case WM_DESTROY: // Pabeigšanas pieprasījums (Close request)
        PostQuitMessage(0); // Pabeigt programmu (Terminate the program)
        return TRUE;
    }

    return FALSE;
}
// lalala
