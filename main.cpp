#include <Windows.h>
#include <Psapi.h>
#include <cstdio>
#include <vector>
using namespace std;

//#define LOGGING 

#ifndef LOGGING
#define LOG(x)
#else
#include <fstream>
static ofstream logfile("searchlog.txt",ofstream::out); 
#define LOG(x) { logfile << hex << x << endl; }
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
	int s;
	vector<int*> results, resultsTmp;
	
	printf("Relative address: %x\n",&s);
	printf("Type integer to search for (-1 returns, -2 to input number in hex): ");
	scanf("%d",&s);
	if(s == -2)
		scanf("%x",&s);
	LOG("\nSearching for " + s);		
	while(s != -1)
	{
		if(s == -2) //Show results again
		{
			for(int i = 0; i < results.size(); i++)
				printf("%p %d\n",results[i],*results[i]);
			
		}
		else if(!results.size())
			while(VirtualQueryEx(handle, memAddress, &memInfo, sizeof(memInfo)) > 0) //No existing results
			{
				if(memInfo.Protect == PAGE_READWRITE || memInfo.Protect == PAGE_READONLY || memInfo.Protect == PAGE_EXECUTE_READ || memInfo.Protect == PAGE_EXECUTE_READWRITE)
				{
					for(int* i = (int*)memInfo.BaseAddress; i < (int*)memInfo.BaseAddress + memInfo.RegionSize/sizeof(int); i++	)
						if(*i == s)
						{
							if(i == &s) //Ignore variables we store in the DLL itself
								continue;
							LOG(reinterpret_cast<void *>(i));
							printf("%p %d\n",i,*i);
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
					LOG(reinterpret_cast<void *>(results[i]));
					printf("%p %d\n",results[i],*results[i]);
				}
			results = resultsTmp;	
		}
		printf("Type integer to search for within results (-1 returns, -2 to display current values): ");
		scanf("%d",&s);
	}
	return;
}

void Monitor()
{
	int* mon;
	printf("Type in specific address (in hex) to monitor: ");
	scanf("%x",&mon);
	
	char option;
	do
	{
		printf("\n%p:%d",mon,*mon);
		printf("Again (y or n)? ");
		scanf("%c",&option);
		printf("\n");
	} while(option != 'n');
	
	return;
}


void Modify()
{
	int* mod;
	printf("Type in specific address (in hex) to modify: ");
	scanf("%x",&mod);
	
	printf("\n%p:%d",mod,*mod);
	printf("Type in new value: ");
	scanf("%d",mod);
	printf("\n");
	
}

DWORD WINAPI initialize(LPVOID param)
{
	while(!setupFlag);
	
	LOG("Initialized!");
	char command;	
	do 
	{
		printf("What do? ");
		scanf("%c",&command);
		printf("\n");
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