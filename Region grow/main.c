
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"
#include <windowsx.h>

void RegGrowThread();
BOOL CALLBACK Dialog_Box1_handle(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
BOOL CALLBACK Dialog_Box2_handle(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPTSTR lpCmdLine, int nCmdShow)

{
MSG			msg;
HWND		hWnd;
WNDCLASS	wc;

wc.style=CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc=(WNDPROC)WndProc;
wc.cbClsExtra=0;
wc.cbWndExtra=0;
wc.hInstance=hInstance;
wc.hIcon=LoadIcon(hInstance,"ID_PLUS_ICON");
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
wc.lpszMenuName="ID_MAIN_MENU";
wc.lpszClassName="PLUS";

if (!RegisterClass(&wc))
  return(FALSE);

hWnd=CreateWindow("PLUS","plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT,0,400,400,NULL,NULL,hInstance,NULL);
if (!hWnd)
  return(FALSE);

ShowScrollBar(hWnd,SB_BOTH,FALSE);
ShowWindow(hWnd,nCmdShow);
UpdateWindow(hWnd);
MainWnd=hWnd;

ShowPixelCoords=0;
BigDots=0;
step_mode = 0;
play_mode = 0;
Button_J_Press = 0;
clear_mode = 0;

strcpy(filename,"");
OriginalImage=NULL;
ROWS=COLS=0;

InvalidateRect(hWnd,NULL,TRUE);
UpdateWindow(hWnd);

while (GetMessage(&msg,NULL,0,0))
  {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  }
return(msg.wParam);
}


LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam)

{
HMENU				hMenu;
OPENFILENAME		ofn;
FILE				*fpt;
HDC					hDC;
char				header[320],text[320];
int					BYTES,xPos,yPos,x,y;

switch (uMsg)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
      {
	  case ID_SHOWPIXELCOORDS:
		ShowPixelCoords=(ShowPixelCoords+1)%2;
		PaintImage();
		break;
	  case ID_DISPLAY_BIGDOTS:
		  BigDots = (BigDots + 1) % 2;
		  PaintImage();
		  break;
	  case ID_STEP_MODE:  
		  step_mode = 1;
		  if (play_mode == 1)
		  {
			  play_mode = 0;
			  //clear_mode = 1;
		  }
		  clear_mode = 0;
		  PostMessage(MainWnd, uMsg, WM_RBUTTONDOWN, 0);
		  break;
	  case ID_PLAY_MODE:
		  play_mode = 1;
		  if (step_mode == 1)
		  {
			  step_mode = 0;
			  //clear_mode = 1;
		  }
		  clear_mode = 0;
		  PostMessage(MainWnd, uMsg, WM_RBUTTONDOWN, 0);
		  break;
	  case ID_FILE_LOAD:
		if (OriginalImage != NULL)
		  {
		  free(OriginalImage);
		  OriginalImage=NULL;
		  }
		memset(&(ofn),0,sizeof(ofn));
		ofn.lStructSize=sizeof(ofn);
		ofn.lpstrFile=filename;
		filename[0]=0;
		ofn.nMaxFile=MAX_FILENAME_CHARS;
		ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
		ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
		if (!( GetOpenFileName(&ofn))  ||  filename[0] == '\0')
		  break;		/* user cancelled load */
		if ((fpt=fopen(filename,"rb")) == NULL)
		  {
		  MessageBox(NULL,"Unable to open file",filename,MB_OK | MB_APPLMODAL);
		  break;
		  }
		fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
		if (strcmp(header,"P5") != 0  ||  BYTES != 255)
		  {
		  MessageBox(NULL,"Not a PPM (P5 greyscale) image",filename,MB_OK | MB_APPLMODAL);
		  fclose(fpt);
		  break;
		  }
		OriginalImage=(unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
		image = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
		header[0]=fgetc(fpt);	/* whitespace character after header */
		fread(OriginalImage,1,ROWS*COLS,fpt);
		fread(image,1,ROWS*COLS,fpt);
		fclose(fpt);
		SetWindowText(hWnd,filename);
		PaintImage();
		break;

      case ID_FILE_QUIT:
        DestroyWindow(hWnd);
        break;
	  case ID_CLEAR:
		clear_mode = 1;
		step_mode = 0;
		play_mode = 0;
		PaintImage();
		break;
	  case ID_COLOR:
		  color_box.lStructSize = sizeof(CHOOSECOLORA);
		  color_box.hwndOwner = hwnd;
		  color_box.rgbResult = current_rgb;
		  color_box.lpCustColors = (LPDWORD) custom_clrs;
		  color_box.Flags = CC_FULLOPEN; 

		  /* Calling a choose color function to display color dialog box */
		  if (ChooseColor(&color_box))
		  {
			  hbrush = CreateSolidBrush(color_box.rgbResult);
			  current_rgb = color_box.rgbResult;
		  }
      }
    break;
  case WM_SIZE:		  /* could be used to detect when window size changes */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_PAINT:
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_LBUTTONDOWN:
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_DIALOG_BOX_1), hWnd, Dialog_Box1_handle);
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_DIALOG_BOX_2), hWnd, Dialog_Box2_handle);
	break;
  case WM_RBUTTONDOWN:
     mouse_y = GET_X_LPARAM(lParam);
     mouse_x = GET_Y_LPARAM(lParam);
	 //clear_mode = 0;
	 /* starts a region growing thread */
    _beginthread(RegGrowThread, 0, MainWnd);	
	break;
  case WM_MOUSEMOVE:
	if (ShowPixelCoords == 1)
	  {
	  xPos=LOWORD(lParam);
	  yPos=HIWORD(lParam);
	  if (xPos >= 0  &&  xPos < COLS  &&  yPos >= 0  &&  yPos < ROWS)
		{
		sprintf(text,"%d,%d=>%d     ",xPos,yPos,OriginalImage[yPos*COLS+xPos]);
		hDC=GetDC(MainWnd);
		TextOut(hDC,0,0,text,strlen(text));		/* draw text on the window */
		if (BigDots == 0)
		  SetPixel(hDC,xPos,yPos,RGB(255,0,0));	/* color the cursor position red */
		else
		  {
		  for (x=-2; x<=2; x++)
			for (y=-2; y<=2; y++)
			  SetPixel(hDC, xPos+x, yPos+y, RGB(255, 0, 0));
		  }
		ReleaseDC(MainWnd,hDC);
		}
	  }
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_KEYDOWN:
	  if (wParam == 'j' || wParam == 'J')
	  {
		  Button_J_Press = 1;
	  }
	  //PostMessage(MainWnd,WM_COMMAND, ID_STEP_MODE,0);	  /* send message to self */
	  return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
	hDC=GetDC(MainWnd);
	SetPixel(hDC,TimerCol,TimerRow,RGB(0,0,255));	/* color the animation pixel blue */
	ReleaseDC(MainWnd,hDC);
	TimerRow++;
	TimerCol+=2;
	break;
  case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
    break;
  }

hMenu=GetMenu(MainWnd);
if (ShowPixelCoords == 1)
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_UNCHECKED);
if (BigDots == 1)
	CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
	CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_UNCHECKED);
if (step_mode == 1  && play_mode == 0)
	CheckMenuItem(hMenu, ID_STEP_MODE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
	CheckMenuItem(hMenu, ID_STEP_MODE, MF_UNCHECKED);
if (play_mode == 1 && step_mode == 0)
	CheckMenuItem(hMenu, ID_PLAY_MODE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
	CheckMenuItem(hMenu, ID_PLAY_MODE, MF_UNCHECKED);

DrawMenuBar(hWnd);

return(0L);
}




void PaintImage()

{
PAINTSTRUCT			Painter;
HDC					hDC;
BITMAPINFOHEADER	bm_info_header;
BITMAPINFO			*bm_info;
int					i,r,c,DISPLAY_ROWS,DISPLAY_COLS;
unsigned char		*DisplayImage;

if (OriginalImage == NULL)
  return;		/* no image to draw */

		/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
DISPLAY_ROWS=ROWS;
DISPLAY_COLS=COLS;
if (DISPLAY_ROWS % 4 != 0)
  DISPLAY_ROWS=(DISPLAY_ROWS/4+1)*4;
if (DISPLAY_COLS % 4 != 0)
  DISPLAY_COLS=(DISPLAY_COLS/4+1)*4;
DisplayImage=(unsigned char *)calloc(DISPLAY_ROWS*DISPLAY_COLS,1);
for (r=0; r<ROWS; r++)
  for (c=0; c<COLS; c++)
	DisplayImage[r*DISPLAY_COLS+c]=OriginalImage[r*COLS+c];

BeginPaint(MainWnd,&Painter);
hDC=GetDC(MainWnd);
bm_info_header.biSize=sizeof(BITMAPINFOHEADER); 
bm_info_header.biWidth=DISPLAY_COLS;
bm_info_header.biHeight=-DISPLAY_ROWS; 
bm_info_header.biPlanes=1;
bm_info_header.biBitCount=8; 
bm_info_header.biCompression=BI_RGB; 
bm_info_header.biSizeImage=0; 
bm_info_header.biXPelsPerMeter=0; 
bm_info_header.biYPelsPerMeter=0;
bm_info_header.biClrUsed=256;
bm_info_header.biClrImportant=256;
bm_info=(BITMAPINFO *)calloc(1,sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD));
bm_info->bmiHeader=bm_info_header;
for (i=0; i<256; i++)
  {
  bm_info->bmiColors[i].rgbBlue=bm_info->bmiColors[i].rgbGreen=bm_info->bmiColors[i].rgbRed=i;
  bm_info->bmiColors[i].rgbReserved=0;
  } 

SetDIBitsToDevice(hDC,0,0,DISPLAY_COLS,DISPLAY_ROWS,0,0,
			  0, /* first scan line */
			  DISPLAY_ROWS, /* number of scan lines */
			  DisplayImage,bm_info,DIB_RGB_COLORS);
ReleaseDC(MainWnd,hDC);
EndPaint(MainWnd,&Painter);

free(DisplayImage);
free(bm_info);
}
BOOL Status;
BOOL CALLBACK Dialog_Box1_handle(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			/* Fetching the intensity predicate value from user */
			Pixel_intensity = GetDlgItemInt(hwnd, ID_INTENSITY, &Status, 1);
			if (Status == FALSE)
			{
				MessageBox(hwnd, "Unable to fetch Intensity value", NULL, 0);
				SendDlgItemMessage(hwnd, ID_INTENSITY, EM_SETSEL, 0, -1L);
			}
			EndDialog(hwnd, IDOK);
			return TRUE;

			// Fall through. 

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CALLBACK Dialog_Box2_handle(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			/* Fetching the centroid_dist value from user */
			Centroid_dist = GetDlgItemInt(hwnd, ID_CENTROID, &Status, 1);
			if (Status == FALSE)
			{
				MessageBox(hwnd, "Unable to fetch Centroid value", NULL, 0);
				SendDlgItemMessage(hwnd, ID_CENTROID, EM_SETSEL, 0, -1L);
			}
			EndDialog(hwnd, wParam);
			return TRUE;

			// Fall through. 

		case IDCANCEL:
			EndDialog(hwnd, wParam);
			return TRUE;
		}
	}
	return FALSE;
}


void AnimationThread(HWND AnimationWindowHandle)

{
HDC		hDC;
char	text[300];

ThreadRow=ThreadCol=0;
while (ThreadRunning == 1)
  {
  hDC=GetDC(MainWnd);
  SetPixel(hDC,ThreadCol,ThreadRow,RGB(0,255,0));	/* color the animation pixel green */
  sprintf(text,"%d,%d     ",ThreadRow,ThreadCol);
  TextOut(hDC,300,0,text,strlen(text));		/* draw text on the window */
  ReleaseDC(MainWnd,hDC);
  ThreadRow+=3;
  ThreadCol++;
  Sleep(100);		/* pause 100 ms */
  }
}

void ExplosionThread(HWND AnimationWindowHandle)

{
	HDC		hDC;
	int		width;
	int		x,y,cx,cy;

	cx=mouse_x;
	cy=mouse_y;
	width=3;
	while (width < 62)
	{
		hDC = GetDC(MainWnd);
		y=cy;
		for (x=cx-width; x<=cx+width; x++)
		  {
		  SetPixel(hDC, x, y-width, current_rgb);	/* color the box red */
		  SetPixel(hDC, x, y+width, current_rgb);	/* color the box red */
		  }
		x=cx;
		for (y = cy - width; y <= cy + width; y++)
		  {
		  SetPixel(hDC, x-width, y, current_rgb);	/* color the box red */
		  SetPixel(hDC, x+width, y, current_rgb);	/* color the box red */
		  }
		ReleaseDC(MainWnd, hDC);
		width += 2;
		Sleep(10);		/* pause 100 ms */
	}
}

void RegGrowThread(HWND AnimationWindowHandle)
{
	int r, c, r1, c1,cx,cy,r2,c2;
	unsigned char* labels;
	int TotalRegions = 0;
	int* indices;
	int RegionSize;
	double avg, var;
	HDC hDc;

	cx = mouse_x;
	cy = mouse_y;

	labels = calloc(ROWS * COLS, sizeof(unsigned char));
	indices = (int*)calloc(ROWS * COLS, sizeof(int));

	TotalRegions++;
	Region_Grow(OriginalImage, labels, ROWS, COLS, cx, cy, 0, TotalRegions, indices, &RegionSize);

	/* Make sure thread has deallocated all its resources */
	_endthread();
}

int Region_Grow(unsigned char* image,
	unsigned char* labels,
	int ROWS, int COLS,
	int r, int c,
	int paint_over_label,
	int new_label,
	int* indices,int *count)
{
	int	r2, c2;
	int	queue[MAX_QUEUE], qh, qt;
	int	average, total;	/* average and total intensity in growing region */
	int distance;
	unsigned long color = current_rgb;
	unsigned int step = step_mode;
	unsigned int play = play_mode;

	*count = 0;
	HDC hDc;
	if (labels[r * COLS + c] != paint_over_label)
		return;
	labels[r * COLS + c] = new_label;
	average = total = (int)image[r * COLS + c];
	if (indices != NULL)
		indices[0] = r * COLS + c;
	queue[0] = r * COLS + c;
	qh = 1;	/* queue head */
	qt = 0;	/* queue tail */
	(*count) = 1;
	while ((qt != qh))
	{
		/* Coloring thw first pixel out on the screen */
		hDc = GetDC(MainWnd);
		SetPixel(hDc, c, r, color);
		ReleaseDC(MainWnd, hDc);

		if ((*count) % 50 == 0)	/* recalculate average after each 50 pixels join */
		{
			average = total / (*count);
		}
		if (play == 1 || step == 1)
		{
			if (play == 1)
			{
				Sleep(1);
			}

			if (step == 1)
			{
				/* Wait until button 'j' or 'J' is pressed */
				while (Button_J_Press == 0)
				{
					/* make sure to destroy the threads which are 'cleared' */
					if (clear_mode == 1)
					{
						_endthread();
					}
				}
				Button_J_Press = 0;
			}

			for (r2 = -1; r2 <= 1; r2++)
				for (c2 = -1; c2 <= 1; c2++)
				{
					if (r2 == 0 && c2 == 0)
						continue;
						
					if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS ||
						(queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)
						continue;

					if (labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != paint_over_label)
						continue;

					/* test criteria to join region */
					if (abs((int)(image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2])
						- average) > Pixel_intensity)
					{
						continue;
					}

					/* Distance betweenn 2 points in a xy plane - sqrt((x2 -x1)^2 + (y2 - y1) ^2)) */
					distance = sqrt(pow((r - (queue[qt] / COLS + r2)), 2) + pow((c - (queue[qt] % COLS + c2)), 2));
					if (distance > Centroid_dist)
						continue;

					labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] = new_label;
					if (indices != NULL)
						indices[*count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					
					total += image[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
					
					/* Setting the pixels with desired color on GUI */
					hDc = GetDC(MainWnd);
					SetPixel(hDc, (queue[qt] % COLS + c2), (queue[qt] / COLS + r2), color);
					ReleaseDC(MainWnd, hDc);
					
					(*count)++;
					queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					qh = (qh + 1) % MAX_QUEUE;
					if (qh == qt)
					{
						printf("Max queue size exceeded\n");
						exit(0);
					}
					/* Make sure to destroy and deallocate all resources of thread when its 'cleared' */
					if (clear_mode == 1)
					{
						_endthread();
					}

				}
			qt = (qt + 1) % MAX_QUEUE;
				
		}
	}
	/* Terminate a thread in case if its not 'cleared' and has reached end */
	_endthread();
	return 0;
}