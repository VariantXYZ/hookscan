#include <Windows.h>
#include <Psapi.h>
#include <cstdio>
#include <vector>

using namespace std;

//#define LOGGING 

#ifndef LOGGING

#define LOG(x,...) do { if(setupFlag) printf(x, ##__VA_ARGS__); printf("\n"); } while(false)
#define LOGn(x,...) do { if(setupFlag) printf(x, ##__VA_ARGS__); } while(false) //Log without newline

#elif LOGGING == 1

FILE *logfile = fopen("log.txt","w");
#define LOG(x,...) do { if(setupFlag) { printf(x, ##__VA_ARGS__); printf("\n"); fprintf(logfile,x, ##__VA_ARGS__); } fprintf(logfile,"\n"); fflush(logfile); } while(false)
#define LOGn(x,...) do { if(setupFlag) { printf(x, ##__VA_ARGS__); fprintf(logfile,x, ##__VA_ARGS__); } fflush(logfile); } while(false)

#else

#define LOG(x,...)
#define LOGn(x,...)

#endif

static volatile bool setupFlag = false;

static HANDLE handle = NULL;
static DWORD threadIDConsole = 0;
static DWORD threadID = 0;

void Search()
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	LPVOID memAddress = NULL;
	MEMORY_BASIC_INFORMATION memInfo;
	unsigned int s;
	vector<unsigned int*> results, resultsTmp;
	
	LOG("Relative address of s: 0x%X",&s);
	LOGn("Type integer to search for (-1 returns, -2 to input number in hex): ");
	scanf("%d",&s);
	if(s == -2)
		scanf("0x%X",&s);
	LOG("\nSearching for %u == 0x%X",s,s);
	while(s != -1)
	{
		if(s == -2) //Show results again
		{
			for(int i = 0; i < results.size(); i++)
				LOG("%p %u == 0x%X",results[i],*results[i],*results[i]);
			
		}
		else if(!results.size())
			while(VirtualQueryEx(handle, memAddress, &memInfo, sizeof(memInfo)) > 0) //No existing results
			{
				if(memInfo.Protect == PAGE_READWRITE || memInfo.Protect == PAGE_READONLY || memInfo.Protect == PAGE_EXECUTE_READ || memInfo.Protect == PAGE_EXECUTE_READWRITE)
				{
					for(unsigned int* i = (unsigned int*)memInfo.BaseAddress; i < (unsigned int*)memInfo.BaseAddress + memInfo.RegionSize/sizeof(int); i++	)
						if(*i == s)
						{
							if(i == &s) //Ignore variables we store in the DLL itself
								continue;
							if(memInfo.Protect == PAGE_EXECUTE_READ || memInfo.Protect == PAGE_EXECUTE_READWRITE || memInfo.Protect == PAGE_EXECUTE || memInfo.Protect == PAGE_EXECUTE_WRITECOPY)
								LOGn("(EXECUTABLE)");
							LOG("%p %u == 0x%X",i,*i,*i);
							results.push_back(i);
						}
				}
				memAddress = (char*)memInfo.BaseAddress + memInfo.RegionSize;
				if (memAddress >= sysInfo.lpMaximumApplicationAddress) //Don't go beyond this
					break;
			}
		else
		{
			resultsTmp.clear();
			for(int i = 0; i < results.size(); i++)
				if(*results[i] == s)
				{
					resultsTmp.push_back(results[i]);
					LOG("%p %u == 0x%X",results[i],*results[i],*results[i]);
				}
			results = resultsTmp;	
		}
		LOGn("Type integer to search for within results (-1 returns, -2 to display current values): ");
		scanf("%u",&s);
		LOG("");
	}
	return;
}

void Monitor()
{
	int* mon;
	LOGn("Type in specific address (in hex) to monitor: ");
	scanf("0x%X",&mon);
	LOG("0x%X",mon);
	char option;
	do
	{
		LOG("%p: %d == 0x%X",mon,*mon,*mon);
		LOGn("Again (y or n)? ");
		scanf("%c",&option);
		LOG("");
	} while(option != 'n');
	
	return;
}


void Modify()
{
	int* mod;
	LOGn("Type in specific address (in hex) to modify: ");
	scanf("0x%X",&mod);
	LOG("");
	
	LOG("%p: %u == 0x%X",mod,*mod,*mod);
	LOGn("Type in new value: ");
	scanf("%d",mod);
	LOG("");
	
}


DWORD WINAPI initialize(LPVOID param)
{
	while(!setupFlag);
	
	LOG("Initialized!");
	char command;	
	do 
	{
		LOGn("What do? ");
		scanf("%c",&command);
		LOG("");
		switch(command)
		{
			case 's': Search(); break;
			case 'm': Monitor(); break;
			case 'c': Modify(); break;
			default: break;
		}
		
	} while(command != 'q');
	
	return 0;
}

DWORD WINAPI setupConsole(LPVOID param)
{
	AllocConsole();
	freopen("CONOUT$","w",stdout);
	freopen("CONIN$","r",stdin);
	setupFlag = true;
	while(true)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			switch (msg.message) {
				case WM_QUIT:
					FreeConsole();
					return msg.wParam;
			}
		}
	}
	
}

extern "C" BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason) 
	{
		case DLL_PROCESS_ATTACH: 
		{
			LOG("Attached!");
			char moduleName[MAX_PATH];
			handle = GetCurrentProcess();
			GetModuleBaseName(handle, NULL, moduleName, MAX_PATH);
			LOG(moduleName);
			CreateThread(NULL,0,setupConsole,NULL,0,&threadIDConsole);
			CreateThread(NULL,0,initialize,NULL,0,&threadID);
			break; 
		}
		case DLL_PROCESS_DETACH:
			if (threadIDConsole)
				PostThreadMessage(threadID, WM_QUIT, 0, 0);
#ifdef LOGGING
			logfile.close();
#endif
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}