// hyxdtester.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <Psapi.h>
#include <vector>
//typedef struct _MOBILE_LOGIN_INFO 
//{
//	char szTag[0x30];
//	char szMobile[0x20];
//	char szPlat[0x10];
//}MOBILE_LOGIN_INFO, *PMOBILE_LOGIN_INFO;


std::vector<DWORD> findPattern(DWORD startAddress, DWORD fileSize, unsigned char * pattern, char mask[]);


DWORD GetPidByName(std::wstring _name)
{
	if (_name.empty()) {
		return 0;
	}

	DWORD maxPid = 256 * 256;
	DWORD _pid = { 0 };
	for (DWORD i = 4; i != maxPid; i += 4)
	{
		auto _handle = OpenProcess(PROCESS_ALL_ACCESS, false, i);
		if (NULL ==_handle){
			continue;
		}

		WCHAR szPath[MAX_PATH];
		auto _ret = GetProcessImageFileName(_handle, szPath, MAX_PATH);
		if (_ret > 0) {
			std::wstring _temp(szPath);
			if (_temp.find(_name) != std::string::npos) {
				_pid = i;
				CloseHandle(_handle);
				break;
			}
		}
		CloseHandle(_handle);
	}
	return _pid;
}

bool GetGameInfoByPid(DWORD _pid, std::string& _result);

int _tmain(int argc, _TCHAR* argv[])
{

	while (true)
	{
		auto _pid1 = GetPidByName(L"hyxd.exe");
		if (_pid1)
		{
			std::string _data;
			if (GetGameInfoByPid(_pid1, _data)) {
				std::cout << _data << std::endl;
				break;
			}
			else {
				//std::cout << "查找失败" << std::endl;
			}
			

		}
		Sleep(100);
	}

	std::cout << "数据查找完成" << std::endl;
	getchar();
	getchar();
	return 0;
}

bool GetGameInfoByPid(DWORD _pid, std::string& _result)
{
	DWORD lastAddr = { 0 };
	bool bRet = false;
	auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _pid);

	//auto hModule =  GetModuleHandleA("hxyd.exe");

	//MODULEINFO info = { 0 };
	//bool b =  GetModuleInformation(hProcess, hModule, &info, sizeof(MODULEINFO));

	if (hProcess)
	{
		PBYTE pAddress = (PBYTE)0x05ffffff;
		BYTE   *lpBuf = new BYTE[1];
		DWORD  dwBufSize = 1;

		//查询页面属性
		while (true)
		{

			if ((DWORD)pAddress > 0x0fffffff)
			{
				break;
			}

			MEMORY_BASIC_INFORMATION mbi;
			if (0 == VirtualQueryEx(hProcess, pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
			{
				std::cout << GetLastError() << std::endl;
				break;
			}


			//查询页面属性成功
			if (MEM_COMMIT != mbi.State || 0 == mbi.Protect
				|| (PAGE_GUARD & mbi.Protect) != 0
				|| (PAGE_NOACCESS & mbi.Protect) != 0)
			{
				pAddress = ((PBYTE)mbi.BaseAddress + mbi.RegionSize);
				continue;
			}

			//std::cout << "BaseAddress: " << mbi.BaseAddress << " RegionSize:" << mbi.RegionSize << std::endl;

			//重新分配内存
			if (mbi.RegionSize > dwBufSize)
			{
				delete[]lpBuf;
				dwBufSize = mbi.RegionSize;
				lpBuf = new BYTE[dwBufSize];
			}


			//开始读取内存

			//std::cout << "mbi.BaseAddress = " << mbi.BaseAddress << "  mbi.RegionSize=" << mbi.RegionSize << std::endl;
			if (!ReadProcessMemory(hProcess, mbi.BaseAddress, lpBuf, (DWORD)mbi.RegionSize, NULL))
			{
				pAddress = ((PBYTE)mbi.BaseAddress + mbi.RegionSize);
				continue;
			}

			//匹配手机号
			auto pkey_mobile = findPattern((DWORD)lpBuf, mbi.RegionSize, (PUCHAR)"&mobile", (PCHAR)"xxxxxxx");
			if (!pkey_mobile.empty())
			{
				for (auto & temp : pkey_mobile)
				{
					std::string _temp((char*)(temp + 6));
					auto pos1 = _temp.find('=');
					auto pos2 = _temp.find('&');
					if (pos1 != std::string::npos && pos2 != std::string::npos)
					{
						_result = _temp.substr(pos1 + 1, pos2 - pos1 - 1);
						bRet = true;
					}
				}
				if (bRet){
					break;
				}
			}

			auto ret_pkey_mobile = findPattern((DWORD)lpBuf, mbi.RegionSize, (PUCHAR)"\"username\"", (PCHAR)"xxxxxxxxxx");
			if (!ret_pkey_mobile.empty())
			{
				for (auto & temp : ret_pkey_mobile)
				{
					std::string _temp((char*)(temp + 11));
					auto pos1 = _temp.find('\"');
					if (pos1 != std::string::npos){
						std::string _temp2 = _temp.substr(pos1 + 1);
						auto pos2 = _temp2.find("\"");
						if (pos2 != std::string::npos) {
							_result = _temp2.substr(0, pos2);
							bRet = true;
							break;
						}
					}
				}

				if (bRet){
					break;
				}
				
			}

			auto ret_pkey = findPattern((DWORD)lpBuf, mbi.RegionSize, (PUCHAR)"client_username", (PCHAR)"xxxxxxxxxxxxxxx");
			if (!ret_pkey.empty())
			{
				for (auto & temp : ret_pkey)
				{
					if (lastAddr < 2){
						lastAddr++;
					}
					else{
						std::string _temp((char*)(temp + 16));
						auto pos1 = _temp.find("\"");
						if (pos1 != std::string::npos){
							std::string _temp2 = _temp.substr(pos1 + 1);
							auto pos2 = _temp2.find("\"");
							if (pos2 != std::string::npos) {
								_result = _temp2.substr(0, pos2);
								bRet = true;
								break;
							}
						}
						
					}
				}

				if (bRet){
					break;
				}
			}

			pAddress = ((PBYTE)mbi.BaseAddress + mbi.RegionSize);
		}

		delete[]lpBuf;

	}

	CloseHandle(hProcess);
	return bRet;
}

std::vector<DWORD> findPattern(DWORD startAddress, DWORD fileSize, unsigned char * pattern, char mask[])
{
	std::vector<DWORD> _list;
	DWORD pos = 0;
	int searchLen = strlen(mask) - 1;
	//从内存内逐个字节进行比较
	for (DWORD retAddress = startAddress; retAddress < startAddress + fileSize; retAddress++)
	{
		//判断当前地址是否有效
		if (IsBadReadPtr((const void *)retAddress, 1))
		{
			pos = 0;
			continue;
		}

		if (*(PBYTE)retAddress == pattern[pos] || mask[pos] == '?')
		{
			if (mask[pos + 1] == '\0')
			{
				_list.push_back(retAddress - searchLen);
				pos = 0;
			}
			else{
				pos++;
			}
		}
		else
		{
			pos = 0;
		}
	}
	return _list;
}

