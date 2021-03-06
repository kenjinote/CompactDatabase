﻿#pragma comment(lib,"rpcrt4")
#pragma comment(lib,"shlwapi")

#include <windows.h>
#include <shlwapi.h>
#include <odbcinst.h>

#import "C:\Program Files (x86)\Common Files\Microsoft Shared\DAO\dao360.dll" rename_namespace("DAO") rename("EOF", "adoEOF")

TCHAR szClassName[] = TEXT("Window");

BOOL CreateGUID(TCHAR *szGUID)
{
	GUID m_guid = GUID_NULL;
	HRESULT hr = UuidCreate(&m_guid);
	if (HRESULT_CODE(hr) != RPC_S_OK){ return FALSE; }
	if (m_guid == GUID_NULL){ return FALSE; }
	wsprintf(szGUID, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
		m_guid.Data1, m_guid.Data2, m_guid.Data3,
		m_guid.Data4[0], m_guid.Data4[1], m_guid.Data4[2], m_guid.Data4[3],
		m_guid.Data4[4], m_guid.Data4[5], m_guid.Data4[6], m_guid.Data4[7]);
	return TRUE;
}

BOOL CreateTempDirectory(LPTSTR pszDir)
{
	TCHAR szGUID[39];
	if (GetTempPath(MAX_PATH, pszDir) == 0)return FALSE;
	if (CreateGUID(szGUID) == 0)return FALSE;
	if (PathAppend(pszDir, szGUID) == 0)return FALSE;
	if (CreateDirectory(pszDir, 0) == 0)return FALSE;
	return TRUE;
}

BOOL CompactDatabase(HWND hWnd, LPCTSTR lpszMDBFilePath)
{
	CoInitialize(NULL);
	BOOL bRet = FALSE;
	DAO::_DBEngine* pEngine = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(DAO::DBEngine), NULL, CLSCTX_ALL, IID_IDispatch, (LPVOID*)&pEngine);
	if (SUCCEEDED(hr) && pEngine)
	{
		hr = -1;
		TCHAR szTempDirectoryPath[MAX_PATH];
		if (CreateTempDirectory(szTempDirectoryPath))
		{
			PathAppend(szTempDirectoryPath, TEXT("TmpDatabase.mdb"));
			try
			{
				hr = pEngine->CompactDatabase((_bstr_t)lpszMDBFilePath, (_bstr_t)szTempDirectoryPath);
			}
			catch (_com_error& e)
			{
				MessageBox(hWnd, e.Description(), 0, 0);
			}
			if (SUCCEEDED(hr))
			{
				if (MoveFileEx(szTempDirectoryPath, lpszMDBFilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				{
					bRet = TRUE;
				}
			}
			else
			{
				DeleteFile(szTempDirectoryPath);
			}
			if (PathRemoveFileSpec(szTempDirectoryPath) && PathIsDirectory(szTempDirectoryPath))
			{
				RemoveDirectory(szTempDirectoryPath);
			}
		}
		pEngine->Release();
		pEngine = NULL;
	}
	CoUninitialize();
	return bRet;
}

BOOL CompactDatabase2(HWND hWnd, LPCTSTR lpszFilePath)
{
	if (!PathFileExists(lpszFilePath))
	{
		return FALSE;
	}
	CoInitialize(NULL);
	TCHAR szAttributes[1024];
	wsprintf(szAttributes, TEXT("COMPACT_DB=\"%s\" \"%s\" General\0"), lpszFilePath, lpszFilePath);
	if (!SQLConfigDataSource(hWnd, ODBC_ADD_DSN, TEXT("Microsoft Access Driver (*.mdb)"), szAttributes))
	{
		CoUninitialize();
		return FALSE;
	}
	CoUninitialize();
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_DROPFILES:
		{
			const HDROP hDrop = (HDROP)wParam;
			TCHAR szFileName[MAX_PATH];
			DragQueryFile(hDrop, 0, szFileName, sizeof(szFileName));
			LPCTSTR lpExt = PathFindExtension(szFileName);
			if (PathMatchSpec(lpExt, TEXT("*.mdb")))
			{
				CompactDatabase(hWnd, szFileName);
			}
			DragFinish(hDrop);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ドラッグされたデータベースMDBを最適化する"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
		);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
