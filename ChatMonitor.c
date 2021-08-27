// Mail and Chat Server for BPQ32 Packet Switch
//
//	Monitor Window(s) Module

#include "BPQChat.h"

static char ClassName[]="BPQMONWINDOW";


static WNDPROC wpOrigInputProc; 
static WNDPROC wpOrigOutputProc; 

HWND hMonitor;

static HWND hwndInput;
static HWND hwndOutput;

static HMENU hMenu;		// handle of menu 


#define InputBoxHeight 25
RECT MonitorRect;
RECT OutputRect;

int Height, Width, LastY;

static char kbbuf[160];
static int kbptr=0;

static char * readbuff;
static int readbufflen;

static BOOL StripLF = TRUE;
static BOOL MonBBS = TRUE;
static BOOL MonCHAT = TRUE;
static BOOL MonTCP = TRUE;

BOOL LogBBS = TRUE;
BOOL LogCHAT = TRUE;
BOOL LogTCP = TRUE;

static int PartLinePtr=0;
static int PartLineIndex=0;		// Listbox index of (last) incomplete line


static LRESULT CALLBACK MonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static void MoveWindows();

#define BGCOLOUR RGB(236,233,216)

extern char MonitorSize[32];

BOOL CreateMonitor()
{
    WNDCLASS  wc;
	HBRUSH bgBrush;
	char Text[80];

	if (hMonitor)
	{
		ShowWindow(hMonitor, SW_SHOWNORMAL);
		SetForegroundWindow(hMonitor);
		return FALSE;							// Alreaqy open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = MonWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hMonitor=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hMonitor)
        return (FALSE);

	wsprintf(Text, "Chat %s Monitor", Session);
	SetWindowText(hMonitor, Text);

	readbuff = zalloc(1000);
	readbufflen = 1000;

	hMenu=GetMenu(hMonitor);

	CheckMenuItem(hMenu,MONBBS, MonBBS ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,MONCHAT, MonCHAT ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,MONTCP, MonTCP ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	// Retrieve the handlse to the edit controls. 

	hwndOutput = GetDlgItem(hMonitor, 121); 
 
	// Set our own WndProcs for the controls. 

	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);

	if (cfgMinToTray)
	{
		AddTrayMenuItem(hMonitor, Text);
	}

	ShowWindow(hMonitor, SW_SHOWNORMAL);

	if (MonitorRect.right < 100 || MonitorRect.bottom < 100)
	{
		GetWindowRect(hMonitor,	&MonitorRect);
	}

	MoveWindow(hMonitor,MonitorRect.left,MonitorRect.top, MonitorRect.right-MonitorRect.left, MonitorRect.bottom-MonitorRect.top, TRUE);

	MoveWindows();

	return TRUE;

}


static void MoveWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	GetWindowRect(hMonitor, &rcMain);
	GetClientRect(hMonitor, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

//	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, 2, ClientWidth-4, ClientHeight-4, TRUE);
//	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);
}


static LRESULT CALLBACK MonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	
	switch (message) { 

		case WM_ACTIVATE:

			SetFocus(hwndInput);
			break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

		case MONBBS:

			ToggleParam(hMenu, hWnd, &MonBBS, MONBBS);
			break;

		case MONCHAT:

			ToggleParam(hMenu, hWnd, &MonCHAT, MONCHAT);
			break;

		case MONTCP:

			ToggleParam(hMenu, hWnd, &MonTCP, MONTCP);
			break;


		case BPQCLEAROUT:

			SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(hwndOutput);
			break;



		//case BPQHELP:

		//	HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
		//	break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

			MoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&MonitorRect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			hMonitor = NULL;

			free(readbuff);
			readbufflen = 0;

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}



LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	if (uMsg >= WM_MOUSEFIRST && uMsg <=  WM_LBUTTONDBLCLK) 
       return TRUE; 

	return CallWindowProc(wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

int WritetoMonitorWindow(char * Msg, int len)
{
	char * ptr1, * ptr2;
	int index;

	if (len+PartLinePtr > readbufflen)
	{
		readbufflen += len+PartLinePtr;
		readbuff = realloc(readbuff, readbufflen);
	}

	if (PartLinePtr != 0)
		SendMessage(hwndOutput,LB_DELETESTRING,PartLineIndex,(LPARAM)(LPCTSTR) 0 );		

	memcpy(&readbuff[PartLinePtr], Msg, len);
		
	len=len+PartLinePtr;

	ptr1=&readbuff[0];
	readbuff[len]=0;

	do {
		ptr2=memchr(ptr1,7,len);
			
		if (ptr2)
			*(ptr2)=32;

	} while (ptr2);

lineloop:

//	if (PartLinePtr > 300)
//		PartLinePtr = 0;

	if (len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			PartLinePtr=len;
			memmove(readbuff,ptr1,len);
			PartLineIndex=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) PartLineIndex, MAKELPARAM(FALSE, 0));

			return (0);

		}

		*(ptr2++)=0;

		index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
					
	//		if (LogOutput) WriteMonitorLine(ptr1, ptr2 - ptr1);

		PartLinePtr=0;

		len-=(ptr2-ptr1);

		ptr1=ptr2;

		if ((len > 0) && StripLF)
		{
			if (*ptr1 == 0x0a)					// Line Feed
			{
				ptr1++;
				len--;
			}
		}

		if (index > 1200)
						
		do{

			index=SendMessage(hwndOutput,LB_DELETESTRING, 0, 0);
			
			} while (index > 1000);

		SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

		goto lineloop;
	}


	return (0);
}

static int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
}

static  void CopyToClipboard(HWND hWnd)
{
	int i,n, len=0;
	HGLOBAL	hMem;
	char * ptr;
	//
	//	Copy List Box to clipboard
	//
	
	n = SendMessage(hWnd, LB_GETCOUNT, 0, 0);		
	
	for (i=0; i<n; i++)
	{
		len+=SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
	}

	hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len+n+n+1);
	

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);
	
		if (OpenClipboard(MainWnd))
		{
			//			CopyScreentoBuffer(GlobalLock(hMem));
			
			for (i=0; i<n; i++)
			{
				ptr+=SendMessage(hWnd, LB_GETTEXT, i, (LPARAM) ptr);
				*(ptr++)=13;
				*(ptr++)=10;
			}

			*(ptr)=0;					// end of data

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
			else
				GlobalFree(hMem);		
	}
}


HANDLE LogHandle[4] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

char * Logs[4] = {"BBS", "CHAT", "TCP", "DEBUG"};

BOOL OpenLogfile(int Flags)
{
	UCHAR FN[MAX_PATH];
	time_t T;
	struct tm * tm;

	T = time(NULL);
	tm = gmtime(&T);	

	sprintf(FN,"%s/logs/log_%02d%02d%02d_%s.txt", GetLogDirectory(), tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, Logs[Flags]);

	LogHandle[Flags] = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	SetFilePointer(LogHandle[Flags], 0, 0, FILE_END);

	return (LogHandle[Flags] != INVALID_HANDLE_VALUE);
}

void WriteLogLine(ChatCIRCUIT * conn, int Flag, char * Msg, int MsgLen, int Flags)
{
	int cnt;
	char CRLF[2] = {0x0d,0x0a};
	struct tm * tm;
	char Stamp[20];
	time_t T;
	struct _EXCEPTION_POINTERS exinfo;

	__try{

	if (hMonitor )
	{
		if (Flags == LOG_CHAT && MonCHAT)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);

			if (conn && conn->Callsign[0])
			{
				char call[20];
				wsprintf(call, "%s          ", conn->Callsign);
				WritetoMonitorWindow(call, 10);
			}
			else
				WritetoMonitorWindow("          ", 10);

			WritetoMonitorWindow(Msg, MsgLen);
			if (Msg[MsgLen-1] != '\r')
				WritetoMonitorWindow(CRLF , 1);
		}
		else if (Flags == LOG_DEBUGx)
		{	
			WritetoMonitorWindow((char *)&Flag, 1);
			WritetoMonitorWindow(Msg, MsgLen);
			WritetoMonitorWindow(CRLF , 1);
		}

	}

	if (Flags == LOG_CHAT && !LogCHAT)
		return;

	if (LogHandle[Flags] == INVALID_HANDLE_VALUE) OpenLogfile(Flags);

	if (LogHandle[Flags] == INVALID_HANDLE_VALUE) return;
	
	T = time(NULL);
	tm = gmtime(&T);	
	
	wsprintf(Stamp,"%02d%02d%02d %02d:%02d:%02d %c",
				tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, Flag);

	WriteFile(LogHandle[Flags] ,Stamp , strlen(Stamp), &cnt, NULL);

	if (conn && conn->Callsign[0])
	{
		char call[20];
		wsprintf(call, "%s          ", conn->Callsign);
		WriteFile(LogHandle[Flags],call, 10, &cnt, NULL);
	}
	else
		WriteFile(LogHandle[Flags], "          ", 10, &cnt, NULL);

	WriteFile(LogHandle[Flags] ,Msg , MsgLen, &cnt, NULL);


	if (Flags == LOG_CHAT && Msg[MsgLen-1] == '\r')
		WriteFile(LogHandle[Flags] ,&CRLF[1] , 1, &cnt, NULL);
	else
		WriteFile(LogHandle[Flags] ,CRLF , 2, &cnt, NULL);

	CloseHandle(LogHandle[Flags]);

	}
	My__except_Routine("WriteLogLine");

	LogHandle[Flags] = INVALID_HANDLE_VALUE;

	

}
