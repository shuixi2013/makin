//
// author: Lasha Khasaia
// contact: @_qaz_qaz
// license: MIT License
//

#include "stdafx.h"


#ifndef _WIN64
#define DWORD64 DWORD32 // maybe there is a better way...
#endif

std::vector<std::wstring> loadDll{};

std::vector<std::string> hookFunctions{};

VOID process_output_string(const PROCESS_INFORMATION pi, const OUTPUT_DEBUG_STRING_INFO out_info) {

    std::unique_ptr<TCHAR[]> pMsg{ new TCHAR[out_info.nDebugStringLength * sizeof(TCHAR)] };

    ReadProcessMemory(pi.hProcess, out_info.lpDebugStringData, pMsg.get(), out_info.nDebugStringLength, nullptr);

    if (pMsg.get()[0] != L'[')
        wprintf_s(L"[OutputDebugString] msg: %s\n\n", pMsg.get()); // raw message from the sample
    else if (_tcslen(pMsg.get()) > 3 && (pMsg.get()[0] == L'[' && pMsg.get()[1] == L'_' && pMsg.get()[2] == L']')) // [_]
    {
        for (auto i = 0; i < loadDll.size(); ++i)
        {
            TCHAR tmp[MAX_PATH + 2]{};
            _tcscpy_s(tmp, MAX_PATH + 2, pMsg.get() + 3);
            const std::wstring wtmp(tmp);
            if (!wtmp.compare(loadDll[i])) // #SOURCE - The "Ultimate" Anti-Debugging Reference: 7.B.iv
            {
				hookFunctions.push_back("LdrLoadDll");
                wprintf(L"[LdrLoadDll] The debuggee attempts to use LdrLoadDll/NtCreateFile trick: %s\n\tref: The \"Ultimate\" Anti-Debugging Reference: 7.B.iv\n\n", wtmp.data());
            }
        }
    }
    else{
        wprintf(L"%s\n", pMsg.get()); // from us, starts with [ symbol

		// save functions for IDA script
		std::wstring tmpStr(pMsg.get());

		// ntdll
		if (tmpStr.find(L"NtClose") != std::string::npos)
			hookFunctions.push_back("NtClose");
		else if (tmpStr.find(L"NtOpenProcess") != std::string::npos)
			hookFunctions.push_back("NtOpenProcess");
		else if (tmpStr.find(L"NtCreateFile") != std::string::npos)
			hookFunctions.push_back("NtCreateFile");
		else if (tmpStr.find(L"NtSetDebugFilterState") != std::string::npos)
			hookFunctions.push_back("NtSetDebugFilterState");
		else if (tmpStr.find(L"NtQueryInformationProcess") != std::string::npos)
			hookFunctions.push_back("NtQueryInformationProcess");
		else if (tmpStr.find(L"NtQuerySystemInformation") != std::string::npos)
			hookFunctions.push_back("NtQuerySystemInformation");
		else if (tmpStr.find(L"NtSetInformationThread") != std::string::npos)
			hookFunctions.push_back("NtSetInformationThread");
		else if (tmpStr.find(L"NtCreateUserProcess") != std::string::npos)
			hookFunctions.push_back("NtCreateUserProcess");
		else if (tmpStr.find(L"NtCreateThreadEx") != std::string::npos)
			hookFunctions.push_back("NtCreateThreadEx");
		else if (tmpStr.find(L"NtSystemDebugControl") != std::string::npos)
			hookFunctions.push_back("NtSystemDebugControl");
		else if (tmpStr.find(L"NtYieldExecution") != std::string::npos)
			hookFunctions.push_back("NtYieldExecution");
		else if (tmpStr.find(L"NtSetLdtEntries") != std::string::npos)
			hookFunctions.push_back("NtSetLdtEntries");
		else if (tmpStr.find(L"NtQueryInformationThread") != std::string::npos)
			hookFunctions.push_back("NtQueryInformationThread");
		else if (tmpStr.find(L"NtCreateDebugObject") != std::string::npos)
			hookFunctions.push_back("NtCreateDebugObject");
		else if (tmpStr.find(L"NtQueryObject") != std::string::npos)
			hookFunctions.push_back("NtQueryObject");
		else if (tmpStr.find(L"RtlAdjustPrivilege") != std::string::npos)
			hookFunctions.push_back("RtlAdjustPrivilege");
		else if (tmpStr.find(L"NtShutdownSystem") != std::string::npos)
			hookFunctions.push_back("NtShutdownSystem");

		// kernelbase
		else if (tmpStr.find(L"IsDebuggerPresent") != std::string::npos)
			hookFunctions.push_back("IsDebuggerPresent");
		else if (tmpStr.find(L"CheckRemoteDebuggerPresent") != std::string::npos)
			hookFunctions.push_back("CheckRemoteDebuggerPresent");
		else if (tmpStr.find(L"SetUnhandledExceptionFilter") != std::string::npos)
			hookFunctions.push_back("SetUnhandledExceptionFilter");

    }
}

VOID genRandStr(TCHAR *str, const size_t size) // just enough randomness
{	
	srand(time(nullptr));
	static const TCHAR syms[] =
		L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		L"abcdefghijklmnopqrstuvwxyz";
	for (size_t i = 0; i < size - 1; ++i)
	{
		str[i] = syms[rand() / (RAND_MAX / (_tcslen(syms) - 1) + 1)];
	}
	str[size - 1] = 0;
}

int _tmain() {

	// welcome 
	const TCHAR welcome[] = L"makin --- Copyright (c) 2018 Lasha Khasaia\n"
		                    L"https://www.secrary.com - @_qaz_qaz\n"
		                    L"----------------------------------------------------\n\n";
	wprintf(L"%s", welcome);

   STARTUPINFO si{};
   si.cb = sizeof(si);
   PROCESS_INFORMATION pi{};
   DWORD err;
   DEBUG_EVENT d_event{};
   auto done = FALSE;
   TCHAR dll_path[MAX_PATH + 2]{};
   TCHAR proc_path[MAX_PATH + 2]{};
   auto first_its_me = FALSE;
   TCHAR filePath[MAX_PATH + 2]{};
   /*CONTEXT cxt{};
   cxt.ContextFlags = CONTEXT_ALL;*/

	int nArgs{};
	const auto pArgv = CommandLineToArgvW(GetCommandLine(), &nArgs);

   if (nArgs < 2) {
      printf("Usage: \n./makin.exe \"/path/to/sample\"\n");
      return 1;
   }

   _tcsncpy_s(proc_path, pArgv[1], MAX_PATH + 2);
   if (!PathFileExists(proc_path))
   {
      err = GetLastError();
      wprintf(L"[!] %s is not a valid file\n", proc_path);

      return err;
   }


   const auto hFile = CreateFile(proc_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
   
	if (hFile == INVALID_HANDLE_VALUE)
	{
		err = GetLastError();
		printf("CreateFile error: %ul\n", err);
		return err;
	}

	LARGE_INTEGER size{};
   GetFileSizeEx(hFile, &size);

	SYSTEM_INFO sysInfo{};
	GetSystemInfo(&sysInfo);

	const auto hMapFile = CreateFileMapping(hFile,
		nullptr,
		PAGE_READONLY,
		size.HighPart,
		size.LowPart,
		nullptr);

	if (!hMapFile)
	{
		err = GetLastError();
		printf("CreateFileMapping is NULL: %ul", err);
		return err;
	}

	// Map just one page
	auto lpMapAddr = MapViewOfFile(hMapFile,  
		FILE_MAP_READ, 
		0,
		0,
		sysInfo.dwPageSize);  // one page size is more than we need for now

	if (!lpMapAddr)
	{
		err = GetLastError();
		printf("MapViewOfFIle is NULL: %ul\n", err);
		return err;
	}
;
	// IMAGE_DOS_HEADER->e_lfanew
	const auto e_lfanew = *reinterpret_cast<DWORD*>(static_cast<byte*>(lpMapAddr) + sizeof(IMAGE_DOS_HEADER) - sizeof(DWORD));
	UnmapViewOfFile(lpMapAddr);


	const DWORD NtMapAddrLow = (e_lfanew / sysInfo.dwAllocationGranularity) * sysInfo.dwAllocationGranularity;
	lpMapAddr = MapViewOfFile(hMapFile,
		FILE_MAP_READ,
		0,
		NtMapAddrLow,
		sysInfo.dwPageSize);

	if (!lpMapAddr)
	{
		err = GetLastError();
		printf("MapViewOfFIle is NULL: %ul\n", err);
		return err;
	}

	auto ntHeaderAddr = lpMapAddr;
	if (NtMapAddrLow != e_lfanew)
	{
		ntHeaderAddr = static_cast<byte*>(ntHeaderAddr) + e_lfanew;
	}

   if (PIMAGE_NT_HEADERS(ntHeaderAddr)->OptionalHeader.DataDirectory[9].VirtualAddress)
   {
      printf("[TLS] The executable contains TLS callback(s)\nI can not hook code executed by TLS callbacks\nPlease, abort execution and check it manually\n[c]ontinue / [A]bort: \n\n");
      const char ic = getchar();
      if (ic != 'c')
         ExitProcess(0);
   }

	UnmapViewOfFile(lpMapAddr);
	CloseHandle(hFile);


   if (!CreateProcess(proc_path, nullptr, nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED | DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
      err = GetLastError();
      printf_s("[!] CreateProces failed: %lu\n", err);
      return err;
   }

   // Create Job object
   JOBOBJECT_EXTENDED_LIMIT_INFORMATION jbli{ 0 };
   JOBOBJECT_BASIC_UI_RESTRICTIONS jbur;
   const auto hJob = CreateJobObject(nullptr, L"makinAKAasho");
   if (hJob)
   {
      jbli.BasicLimitInformation.ActiveProcessLimit = 1; // Blocked new process creation
      jbli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

      jbur.UIRestrictionsClass = JOB_OBJECT_UILIMIT_DESKTOP | JOB_OBJECT_UILIMIT_EXITWINDOWS; /*| JOB_OBJECT_UILIMIT_HANDLES*/ // uncomment if you want

      SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jbli, sizeof(jbli));

      SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &jbur, sizeof(jbur));

      if (!AssignProcessToJobObject(hJob, pi.hProcess))
      {
         printf("[!] AssignProcessToJobObject failed: %ul\n", GetLastError());
      }

   }

#ifdef _DEBUG
   SetCurrentDirectory(L"../Debug");
#ifdef _WIN64
   SetCurrentDirectory(L"../x64/Debug");
#endif
#endif

    GetFullPathName(L"./asho.dll", MAX_PATH + 2, dll_path, nullptr);
    if (!PathFileExists(dll_path))
    {
        err = GetLastError();
        wprintf(L"[!] %s is not a valid file\n", dll_path);

        return err;
    }

	// generate random name for asho.dll ;)
	TCHAR randAsho[0x5]{};
	genRandStr(randAsho, 0x5);
	TCHAR ashoTmpDir[MAX_PATH + 2]{};
	GetTempPath(MAX_PATH + 2, ashoTmpDir);
	_tcscat_s(ashoTmpDir, randAsho);
	_tcscat_s(ashoTmpDir, L".dll");
	const auto cStatus = CopyFile(dll_path, ashoTmpDir, FALSE);
	if (!cStatus)
	{
		err = GetLastError();
		wprintf(L"[!] CopyFile failed: %ul\n", err);

		return err;
	}
	if (!PathFileExists(ashoTmpDir))
	{
		err = GetLastError();
		wprintf(L"[!] %s is not a valid file\n", ashoTmpDir);

		return err;
	}

   const LPVOID p_alloc = VirtualAllocEx(pi.hProcess, nullptr, MAX_PATH + 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
   if (!p_alloc) {
      err = GetLastError();
      printf("[!] Allocation failed: %lu\n", err);
      return err;
   }
   if (!WriteProcessMemory(pi.hProcess, p_alloc, ashoTmpDir, MAX_PATH + 2, nullptr)) {
      err = GetLastError();
      printf("WriteProcessMemory failed: %lu\n", err);
      return err;
   }
   const HMODULE h_module = GetModuleHandle(L"kernel32");
   if (!h_module) {
      err = GetLastError();
      printf("GetmModuleHandle failed: %lu\n", err);
      return err;
   }
   const FARPROC load_library_addr = GetProcAddress(h_module, "LoadLibraryW");

   if (!load_library_addr) {
      err = GetLastError();
      printf("GetProcAddress failed: %lu\n", err);
      return err;
   }

   const auto qStatus = QueueUserAPC(PAPCFUNC(load_library_addr), pi.hThread, ULONG_PTR(p_alloc));
   if (!qStatus)
   {
      err = GetLastError();
      printf("QueueUserAPC failed: %lu\n", err);
      return err;
   }

   ResumeThread(pi.hThread);

   while (!done) {
      auto cont_status = DBG_CONTINUE;
      if (WaitForDebugEventEx(&d_event, INFINITE)) {
         switch (d_event.dwDebugEventCode)
         {
         case OUTPUT_DEBUG_STRING_EVENT:
               process_output_string(pi, d_event.u.DebugString);

               cont_status = DBG_CONTINUE;
               break;
         case LOAD_DLL_DEBUG_EVENT:
               // we get load dll as file handle 
               GetFinalPathNameByHandle(d_event.u.LoadDll.hFile, filePath, MAX_PATH + 2, 0);
               if (filePath)
               {
                  const std::wstring tmpStr(filePath + 4);
                  loadDll.push_back(tmpStr);
               }
               // to avoid LdrloadDll / NtCreateFile trick ;)
               CloseHandle(d_event.u.LoadDll.hFile);
               break;

         case EXCEPTION_DEBUG_EVENT:
               cont_status = DBG_EXCEPTION_NOT_HANDLED;
               switch (d_event.u.Exception.ExceptionRecord.ExceptionCode)
               {
               case EXCEPTION_ACCESS_VIOLATION:
                  printf("[EXCEPTION] EXCEPTION_ACCESS_VIOLATION\n\n");
                  // cont_status = DBG_EXCEPTION_HANDLED;
                  break;

               case EXCEPTION_BREAKPOINT:

                  if (!first_its_me) {
                     first_its_me = TRUE;
                     break;
                  }
                  printf("[EXCEPTION] EXCEPTION_BREAKPOINT\n\n");
                  // cont_status = DBG_EXCEPTION_HANDLED;

                  break;

                  /*case EXCEPTION_DATATYPE_MISALIGNMENT:
                  printf("[EXCEPTION] EXCEPTION_DATATYPE_MISALIGNMENT\n");

                  break;*/

               case EXCEPTION_SINGLE_STEP:
                  printf("[EXCEPTION] EXCEPTION_SINGLE_STEP\n\n");

                  break;

               case DBG_CONTROL_C:
                  printf("[EXCEPTION] DBG_CONTROL_C\n\n");

                  break;

               case EXCEPTION_GUARD_PAGE:
                  printf("[EXCEPTION] EXCEPTION_GUARD_PAGE\n\n");
                  // cont_status = DBG_EXCEPTION_HANDLED;
                  break;

               default:
                  // Handle other exceptions. 
                  break;
               }
               break;
         case EXIT_PROCESS_DEBUG_EVENT:
               done = TRUE;
               printf("[EOF] ========================================================================\n");
               break;

         default: 
               break;
         }

         ContinueDebugEvent(d_event.dwProcessId, d_event.dwThreadId, cont_status);

      }
   }

   CloseHandle(pi.hThread);
   CloseHandle(pi.hProcess);
   if (hJob)
      CloseHandle(hJob);

	// IDA script
	char header[] =
		"import idc\n"
		"import idaapi\n"
		"import idautils\n"
		"\n"
		"hookFunctions = [\n";
	char tail[] =
		"\n]\n"
		"\n"
		"def makinbp():\n"
		"	if not idaapi.is_debugger_on():\n"
		"		print \"Please run the process... and call makinbp() again\"\n"
		"		return\n"
		"	print \"\\n\\n---------- makin ----------- \\n\\n\"\n"
		"	for mods in idautils.Modules():\n"
		"		if \"ntdll.dll\" in mods.name.lower() or \"kernelbase.dll\" in mods.name.lower():\n"
		"			# idaapi.analyze_area(mods.base, mods.base + mods.size)\n"
		"			name_addr = idaapi.get_debug_names(mods.base, mods.base + mods.size)\n"
		"			for addr in name_addr:\n"
		"				func_name = Name(addr)\n"
		"				func_name = func_name.split(\"_\")[1]\n"
		"				for funcs in hookFunctions:\n"
		"					if funcs.lower() == func_name.lower():\n"
		"						print \"Adding bp on \", hex(addr), func_name\n"
		"						add_bpt(addr)\n"
		"						hookFunctions.remove(funcs)\n"
		"	print \"\\n\\n----------EOF makin EOF----------- \\n\""
		"\n\n"
		"def main():\n"
		"	if idaapi.is_debugger_on():\n"
		"		makinbp()\n"
		"	else:\n"
		"		print \"Please run the process... and call makinbp()\"\n"
		"\n"
		"main()\n";
		

	const auto hFileIda = CreateFile(L"makin_ida_bp.py", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (hFileIda == INVALID_HANDLE_VALUE)
	{
		err = GetLastError();
		wprintf(L"CreateFile failed: %ul", err);
	}

	WriteFile(hFileIda, header, strlen(header), nullptr, nullptr);
	
	// http://en.cppreference.com/w/cpp/algorithm/unique
	std::sort(hookFunctions.begin(), hookFunctions.end());
	const auto last = std::unique(hookFunctions.begin(), hookFunctions.end());
	hookFunctions.erase(last, hookFunctions.end());

	for (auto func : hookFunctions)
	{
		WriteFile(hFileIda, "\"", strlen("\""), nullptr, nullptr);
		WriteFile(hFileIda, func.data(), strlen(func.data()), nullptr, nullptr);
		WriteFile(hFileIda, "\",\n", strlen("\",\n"), nullptr, nullptr);

	}

	WriteFile(hFileIda, tail, strlen(tail), nullptr, nullptr);

	CloseHandle(hFileIda);

   return 0;
}