#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <shlobj.h>
#include <string.h>
#include <shellapi.h>
#include <commctrl.h>
#include "resource.h"

// g (Global optimization), s (Favor small code), y (No frame pointers).
#pragma optimize("gsy", on)
#pragma comment(linker, "/FILEALIGN:0x200")
#pragma comment(linker, "/MERGE:.rdata=.data")
#pragma comment(linker, "/MERGE:.text=.data")
#pragma comment(linker, "/MERGE:.reloc=.data")
#pragma comment(linker, "/SECTION:.text, EWR /IGNORE:4078")
#pragma comment(linker, "/OPT:NOWIN98")		// Make section alignment really small.
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "COMCTL32.lib")
#pragma comment(lib, "shell32.lib")

#define CSIDL_ADMINTOOLS                0x0030      // <user name>\Start Menu\Programs\Administrative Tools
#define CSIDL_CDBURN_AREA               0x003b		// USERPROFILE\Local Settings\Application Data\Microsoft\CD Burning
#define CSIDL_COMMON_APPDATA            0x0023		// All Users\Application Data
#define CSIDL_WINDOWS                   0x0024		// GetWindowsDirectory()
#define CSIDL_SYSTEM                    0x0025		// GetSystemDirectory()
#define CSIDL_PROGRAM_FILES             0x0026		// C:\Program Files
#define CSIDL_COMMON_DOCUMENTS          0x002e		// All Users\Documents
#define CSIDL_COMMON_ADMINTOOLS         0x002f		// All Users\Start Menu\Programs\Administrative Tools
#define CSIDL_COMMON_MUSIC              0x0035		// All Users\My Music
#define CSIDL_COMMON_PICTURES           0x0036		// All Users\My Pictures
#define CSIDL_COMMON_VIDEO              0x0037		// All Users\My Video
#define CSIDL_COMMON_TEMPLATES          0x002d		// All Users\Templates
#define CSIDL_MYMUSIC                   0x000d		// "My Music" folder
#define CSIDL_MYPICTURES                0x0027		// C:\Program Files\My Pictures
#define CSIDL_LOCAL_APPDATA             0x001c		// <user name>\Local Settings\Applicaiton Data (non roaming)
#define CSIDL_PROFILE                   0x0028		// USERPROFILE
#define CSIDL_PROGRAM_FILES_COMMON      0x002b		// C:\Program Files\Common

HWND		hDlg;
HINSTANCE	hInst;
HWND		hList= 0;		// List View identifier
LVCOLUMN	LvCol;			// Make Column struct for ListView
LVITEM		LvItem;			// ListView Item struct
int			iIndex, iSelect;

BOOL	CALLBACK Main(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int		CALLBACK Properties(HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam);
void	StartUp( void );
BOOL	InitList( HWND hDlg );

int WINAPI WinMain( HINSTANCE hinstCurrent,HINSTANCE hinstPrevious,LPSTR lpszCmdLine,int nCmdShow ){
	InitCommonControls();
	hInst= hinstCurrent;
	DialogBox(hinstCurrent,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)Main);
	return 0;
}

BOOL CALLBACK Main(HWND hDlgMain,UINT uMsg,WPARAM wParam,LPARAM lParam) {
	NMHDR* nmNotification = (NMHDR*)lParam;
	PAINTSTRUCT		ps;
	HDC				hdc;
	BOOL			bRes;

	LPNMHDR				lpnmhdr;
	LPNMITEMACTIVATE	lpnmitem;
	//LVITEM				lvItem;
	LVHITTESTINFO		lvHti;
	HMENU				hMenu;
	POINT				pt;
	char	*buffer;
	SHELLEXECUTEINFO	ShExecInfo= {0};

	hDlg= hDlgMain;

	switch(uMsg) {
		case WM_INITDIALOG:
			hList= GetDlgItem(hDlg, IDC_FOLDERS);
			ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES);
			SendMessageA(hDlg,WM_SETICON,ICON_SMALL, (LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			SendMessageA(hDlg,WM_SETICON, ICON_BIG,(LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			StartUp();
			ListView_DeleteAllItems(hList);
			bRes= InitList(hDlg);
			break;
		case WM_COMMAND:
			switch( wParam ){
				case IDC_REFRESH:
					ListView_DeleteAllItems(hList);
					bRes= InitList(hDlg);
					break;
				case IDC_EXIT:
					EndDialog(hDlg,wParam);
					break;
				case IDM_PROPERTIES:
					bRes= DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTIES), hDlg, Properties, 0);
					return TRUE;
				case IDM_FOLDER_PROPERTIES:
					buffer= (char*)calloc(512, sizeof(char));
					ListView_GetItemText(hList, iSelect, 2, buffer, 512);

					ShExecInfo.cbSize= sizeof(SHELLEXECUTEINFO);
					ShExecInfo.fMask= SEE_MASK_INVOKEIDLIST;
					ShExecInfo.hwnd= NULL;
					ShExecInfo.lpVerb= "properties";
					ShExecInfo.lpFile= buffer;
					ShExecInfo.lpParameters= "";
					ShExecInfo.nShow= SW_SHOW;
					ShExecInfo.hInstApp= NULL;
					ShellExecuteEx(&ShExecInfo);
					return TRUE;
			}
			break;
		case WM_NOTIFY:
			lpnmhdr= (LPNMHDR)lParam;
			if( lpnmhdr->hwndFrom==hList ){
				if(lpnmhdr->code == NM_RCLICK) {
					lpnmitem= (LPNMITEMACTIVATE)lParam;
					hMenu= CreatePopupMenu();

					ZeroMemory(&lvHti, sizeof(LVHITTESTINFO));

					lvHti.pt= lpnmitem->ptAction;
					iSelect= ListView_HitTest(hList, &lvHti);

					if( lvHti.flags&LVHT_ONITEM ){
						AppendMenu(hMenu, MF_STRING, IDM_PROPERTIES, " Properties");
						AppendMenu(hMenu, MF_STRING, IDM_FOLDER_PROPERTIES, "Folder Properties");
					}
					GetCursorPos(&pt);
					TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hDlg, 0);
				}
			}
			DestroyMenu(hMenu);
			return TRUE;
		case WM_PAINT:
			hdc= BeginPaint(hDlg, &ps);
			InvalidateRect(hDlg, NULL, TRUE);
			EndPaint (hDlg, &ps);
		break;
		case WM_CLOSE:
			EndDialog(hDlg,wParam);
			DestroyWindow(hDlg);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
	}
	return FALSE;
}

int CALLBACK Properties(HWND hDlgAbout, UINT message, WPARAM wParam, LPARAM lParam){
	HDC			wHdc;
	PAINTSTRUCT	wPs;
	char		*szBuff;
	DWORD		dwAttributes;

	switch( message ){
		case WM_INITDIALOG:
			szBuff= (char*)calloc(512, sizeof(char));
			ListView_GetItemText(hList, iSelect, 0, szBuff, 512);
			SetDlgItemText(hDlgAbout, IDC_FOLDERNAME, szBuff);
			ListView_GetItemText(hList, iSelect, 1, szBuff, 512);
			SetDlgItemText(hDlgAbout, IDC_HIDDEN, szBuff);
			ListView_GetItemText(hList, iSelect, 2, szBuff, 512);
			SetDlgItemText(hDlgAbout, IDC_FOLDERPATH, szBuff);
			dwAttributes= GetFileAttributes(szBuff);
			if( dwAttributes&FILE_ATTRIBUTE_READONLY ){
				SetDlgItemText(hDlgAbout, IDC_READONLY, "Yes");
			}else{
				SetDlgItemText(hDlgAbout, IDC_READONLY, "No");
			}
			if( dwAttributes&FILE_ATTRIBUTE_SYSTEM ){
				SetDlgItemText(hDlgAbout, IDC_SYSTEM, "Yes");
			}else{
				SetDlgItemText(hDlgAbout, IDC_SYSTEM, "No");
			}
			ListView_GetItemText(hList, iSelect, 3, szBuff, 512);
			SetDlgItemText(hDlgAbout, IDC_CSIDL, szBuff);
			ListView_GetItemText(hList, iSelect, 4, szBuff, 512);
			SetDlgItemText(hDlgAbout, IDC_CSIDLNAME, szBuff);
			break;
		case WM_COMMAND:
			if( LOWORD(wParam)==IDC_DONE ){
				if( HIWORD(wParam)==BN_CLICKED ){
					EndDialog(hDlgAbout,wParam);
					DestroyWindow(hDlgAbout);
				}
			}
			break;
		case WM_PAINT:
			wHdc= BeginPaint(hDlgAbout, &wPs);
			InvalidateRect(hDlgAbout, NULL, TRUE);
			EndPaint(hDlgAbout, &wPs);
			break;
		case WM_CLOSE:
			EndDialog(hDlgAbout,wParam);
			DestroyWindow(hDlgAbout);
			break;
		default:
			return 0;
	}
	return 0;
}

BOOL InitList( HWND hDlg ){
	char	*szPath, *szHidden;
	BOOL	bReturn, bRes;
	DWORD	dwBufSize= 512;
	DWORD	dwAttrs;
	LVITEM	lvItem;
	int		iCount= 0;

	szPath= (char*)calloc(1024, sizeof(char));
	szHidden= (char*)calloc(8, sizeof(char));
	
	lvItem.mask= LVIF_TEXT;
	lvItem.cchTextMax= MAX_PATH;

	// C:\Documents and Settings\Administrator\Start Menu\Programs\Administrative Tools	0x30
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	bRes= SHGetSpecialFolderPath(hDlg, szPath, CSIDL_ADMINTOOLS, 0);
	if( bRes ){
		lvItem.pszText= "Administrative Tools";
		SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
		lvItem.iSubItem= 1;
		dwAttrs= GetFileAttributes(szPath);
		if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
			szHidden= "Yes";
		}else{
			szHidden= "No";
		}
		lvItem.pszText= szHidden;
		SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
		lvItem.iSubItem= 2;
		lvItem.pszText= szPath;
		SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
		lvItem.iSubItem= 3;
		lvItem.pszText= "0x30";
		SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
		lvItem.iSubItem= 4;
		lvItem.pszText= "CSIDL_ADMINTOOLS";
		SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
		iCount++;
	}

	// Application Data	C:\Documents and Settings\Administrator\Application Data	0x1a
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_APPDATA, 0);
	lvItem.pszText= "Application Data";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x1a";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_APPDATA";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// CD Burning	C:\Documents and Settings\Administrator\Local Settings\Application Data\Microsoft\CD Burning	0x3b
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_CDBURN_AREA, 0);
	lvItem.pszText= "CD Burning";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x3b";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_CDBURN_AREA";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Administrative Tools	C:\Documents and Settings\All Users\Start Menu\Programs\Administrative Tools	0x2f
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_ADMINTOOLS, 0);
	lvItem.pszText= "Common Administrative Tools";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x2f";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_ADMINTOOLS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Application Data	C:\Documents and Settings\All Users\Application Data	0x23
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_APPDATA, 0);
	lvItem.pszText= "Common Application Data";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x23";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_APPDATA";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Desktop	C:\Documents and Settings\All Users\Desktop	0x19
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_DESKTOPDIRECTORY, 0);
	lvItem.pszText= "Common Desktop";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x19";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_DESKTOPDIRECTORY";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Documents	C:\Documents and Settings\All Users\Documents	0x2e
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_DOCUMENTS, 0);
	lvItem.pszText= "Common Documents";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x2e";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_DOCUMENTS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Favorites	C:\Documents and Settings\All Users\Favorites	0x1f
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_FAVORITES, 0);
	lvItem.pszText= "Common Favorites";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x1f";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_FAVORITES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Music	C:\Documents and Settings\All Users\Documents\My Music	0x35
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_MUSIC, 0);
	lvItem.pszText= "Common Music";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x35";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_MUSIC";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Pictures	C:\Documents and Settings\All Users\Documents\My Pictures	0x36
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_PICTURES, 0);
	lvItem.pszText= "Common Pictures";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x36";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_PICTURES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Start Menu	C:\Documents and Settings\All Users\Start Menu	0x16
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_STARTMENU, 0);
	lvItem.pszText= "Common Start Menu";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x16";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_STARTMENU";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Start Menu Programs	C:\Documents and Settings\All Users\Start Menu\Programs	0x17
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_PROGRAMS, 0);
	lvItem.pszText= "Common Start Menu Programs";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x17";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_PROGRAMS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Startup	C:\Documents and Settings\All Users\Start Menu\Programs\Startup	0x18
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_STARTUP, 0);
	lvItem.pszText= "Common Startup";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x18";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_STARTUP";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Templates	C:\Documents and Settings\All Users\Templates	0x2d
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_TEMPLATES, 0);
	lvItem.pszText= "Common Templates";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x2d";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_TEMPLATES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Common Video	C:\Documents and Settings\All Users\Documents\My Videos	0x37
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COMMON_VIDEO, 0);
	lvItem.pszText= "Common Video";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x37";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COMMON_VIDEO";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Cookies	C:\Documents and Settings\Administrator\Cookies	0x21
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_COOKIES, 0);
	lvItem.pszText= "Cookies";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x21";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_COOKIES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Desktop	C:\Documents and Settings\Administrator\Desktop	0x10
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_DESKTOPDIRECTORY, 0);
	lvItem.pszText= "Desktop";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x10";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_DESKTOPDIRECTORY";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Favorites	C:\Documents and Settings\Administrator\Favorites	0x06
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_FAVORITES, 0);
	lvItem.pszText= "Favorites";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x06";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_FAVORITES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Fonts	C:\WINDOWS\Fonts	0x14
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_FONTS, 0);
	lvItem.pszText= "Fonts";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x06";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_FONTS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// History	C:\Documents and Settings\Administrator\Local Settings\History	0x22
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_HISTORY, 0);
	lvItem.pszText= "History";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x22";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_HISTORY";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Local Application Data	C:\Documents and Settings\Administrator\Local Settings\Application Data	0x1c
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_LOCAL_APPDATA, 0);
	lvItem.pszText= "Local Application Data";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x1c";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_LOCAL_APPDATA";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// My Documents	C:\Documents and Settings\Administrator\My Documents	0x05
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PERSONAL, 0);
	lvItem.pszText= "My Documents";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x05";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PERSONAL";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// My Music	C:\Documents and Settings\Administrator\My Documents\My Music	0x0d
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_MYMUSIC, 0);
	lvItem.pszText= "My Music";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x0d";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_MYMUSIC";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// My Pictures	C:\Documents and Settings\Administrator\My Documents\My Pictures	0x27
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_MYPICTURES, 0);
	lvItem.pszText= "My Pictures";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x27";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_MYPICTURES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// NetHood	C:\Documents and Settings\Administrator\NetHood	0x13
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_NETHOOD, 0);
	lvItem.pszText= "NetHood";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x13";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_NETHOOD";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// PrintHood	C:\Documents and Settings\Administrator\PrintHood	0x1b
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PRINTHOOD, 0);
	lvItem.pszText= "PrintHood";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x1b";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PRINTHOOD";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Profile Folder	C:\Documents and Settings\Administrator	0x28
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PROFILE, 0);
	lvItem.pszText= "Profile Folder";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x28";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PROFILE";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Program Files	C:\Program Files	0x26
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PROGRAM_FILES, 0);
	lvItem.pszText= "Program Files";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x26";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PROGRAM_FILES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Program Files - Common	C:\Program Files\Common Files	0x2b
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PROGRAM_FILES_COMMON, 0);
	lvItem.pszText= "Program Files - Common";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x2b";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PROGRAM_FILES_COMMON";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Recent	C:\Documents and Settings\Administrator\Recent	0x08
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_RECENT, 0);
	lvItem.pszText= "Recent";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x08";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_RECENT";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Send To	C:\Documents and Settings\Administrator\SendTo	0x09
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_SENDTO, 0);
	lvItem.pszText= "Send To";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x09";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_SENDTO";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Start Menu	C:\Documents and Settings\Administrator\Start Menu	0x0b
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_STARTMENU, 0);
	lvItem.pszText= "Start Menu";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x0b";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_STARTMENU";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Start Menu Programs	C:\Documents and Settings\Administrator\Start Menu\Programs	0x02
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_PROGRAMS, 0);
	lvItem.pszText= "Start Menu Programs";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x02";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_PROGRAMS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;
	
	// Startup	C:\Documents and Settings\Administrator\Start Menu\Programs\Startup	0x07
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_STARTUP, 0);
	lvItem.pszText= "Startup";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x07";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_STARTUP";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// System Directory	C:\WINDOWS\system32	0x25
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_SYSTEM, 0);
	lvItem.pszText= "System Directory";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x25";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_SYSTEM";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Templates	C:\Documents and Settings\Administrator\Templates	0x15
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_TEMPLATES, 0);
	lvItem.pszText= "Templates";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x15";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_TEMPLATES";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Temporary Folder	C:\Documents and Settings\Administrator\Local Settings\Temp
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	bReturn= GetTempPath(dwBufSize, szPath);
	lvItem.pszText= "Temporary Folder";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x20";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Temporary Internet Files	C:\Documents and Settings\Administrator\Local Settings\Temporary Internet Files	0x20
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_INTERNET_CACHE, 0);
	lvItem.pszText= "Temporary Internet Files";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x20";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_INTERNET_CACHE";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	iCount++;

	// Windows Directory	C:\WINDOWS	0x24
	lvItem.iItem= iCount;
	lvItem.iSubItem= 0;
	SHGetSpecialFolderPath(hDlg, szPath, CSIDL_WINDOWS, 0);
	lvItem.pszText= "Windows Directory";
	SendMessage(hList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 1;
	dwAttrs= GetFileAttributes(szPath);
	if( dwAttrs&FILE_ATTRIBUTE_HIDDEN ){
		szHidden= "Yes";
	}else{
		szHidden= "No";
	}
	lvItem.pszText= szHidden;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 2;
	lvItem.pszText= szPath;
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 3;
	lvItem.pszText= "0x24";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);
	lvItem.iSubItem= 4;
	lvItem.pszText= "CSIDL_WINDOWS";
	SendMessage(hList, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvItem);

	free(szPath);
	szPath= NULL;
	return TRUE;
}

void StartUp( void ){
	LVCOLUMN	lvCol;
	int			width= 380;

	ZeroMemory(&lvCol, sizeof(LVCOLUMN));

	LvCol.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;	// Type of mask
	lvCol.fmt= LVCFMT_LEFT;	
	LvCol.cx=0x70;													// width between each column
	LvCol.pszText="Folder Name";									// First Header
	LvCol.cx=0x70;
	
	// Inserting Columns as much as we want
	SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);			// Insert/Show the column
	LvCol.pszText="Hidden";											// Next column
	SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
	LvCol.pszText="Folder Path";
	SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
	LvCol.pszText="CSIDL";
	SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);
	LvCol.pszText="CSIDL Name";
	SendMessage(hList,LVM_INSERTCOLUMN,4,(LPARAM)&LvCol);			// ...same as above
	//SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
}