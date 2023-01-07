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

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int nCmdShow)

{
	MSG			msg;
	HWND		hWnd;
	WNDCLASS	wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "ID_PLUS_ICON");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = "ID_MAIN_MENU";
	wc.lpszClassName = "PLUS";

	if (!RegisterClass(&wc))
		return(FALSE);

	hWnd = CreateWindow("PLUS", "Active Contours Semester Project",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT, 0, 800, 800, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return(FALSE);

	ShowScrollBar(hWnd, SB_BOTH, FALSE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MainWnd = hWnd;

	ShowPixelCoords = 0;
	BigDots = 0;
	step_mode = 0;
	play_mode = 0;
	Button_J_Press = 0;
	clear_mode = 0;
	rubber = 0;

	strcpy(filename, "");
	OriginalImage = NULL;
	Gray_scale_image = NULL;
	ROWS = COLS = 0;

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return(msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)

{
	HMENU				hMenu;
	OPENFILENAME		ofn;
	FILE* fpt;
	HDC					hDC;
	char				header[320], text[320];
	int					BYTES, xPos, yPos, x, y;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SHOWPIXELCOORDS:
			ShowPixelCoords = (ShowPixelCoords + 1) % 2;
			PaintImage();
			break;
		case ID_DISPLAY_BIGDOTS:
			BigDots = (BigDots + 1) % 2;
			PaintImage();
			break;
		case ID_ORIGINAL_IMAGE:
			Original_image = (Original_image + 1) % 2;
			if (Original_image == 1)
			{
				sobel_image = 0;
				PaintImage();
			}
			break;
		case ID_SOBEL_FILTERED_IMAGE:
			sobel_image = (sobel_image + 1) % 2;
			if (sobel_image == 1)
			{
				Original_image = 0;
				PaintSobelFilteredImage();
			}
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
				OriginalImage = NULL;
			}
			memset(&(ofn), 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = filename;
			filename[0] = 0;
			ofn.nMaxFile = MAX_FILENAME_CHARS;
			ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
			ofn.lpstrFilter = "PNM files\0*.pnm\0All files\0*.*\0\0";
			if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
				break;		/* user cancelled load */
			if ((fpt = fopen(filename, "rb")) == NULL)
			{
				MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
				break;
			}
			fscanf(fpt, "%s %d %d %d", header, &COLS, &ROWS, &BYTES);
			if (strcmp(header, "P6") != 0 || BYTES != 255)
			{
				MessageBox(NULL, "Not a PPM (P5 greyscale) image", filename, MB_OK | MB_APPLMODAL);
				fclose(fpt);
				break;
			}
			OriginalImage = (unsigned char*)calloc(ROWS * COLS * 3, sizeof(unsigned char));
			sobel_output = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
			Gray_scale_image = (unsigned char*)calloc(ROWS * COLS * 3, sizeof(unsigned char));
			image = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
			header[0] = fgetc(fpt);	/* whitespace character after header */
			fread(OriginalImage, 1, ROWS * COLS * 3, fpt);
			fread(image, 1, ROWS * COLS, fpt);
			fclose(fpt);

			/* Convert RGB image to Grayscale */
			RGB_To_Grayscale();

			SetWindowText(hWnd, filename);
			PaintImage();
			break;

		case ID_FILE_QUIT:
			DestroyWindow(hWnd);
			break;
		case ID_BALLOON_MODEL:
			balloon = (balloon + 1) % 2;
			if (balloon == 1)
			{
				balloon_mode = 1;
				PaintImage();
				_beginthread(Active_contour_Balloon_Model, 0, MainWnd);
			}
			break;
		case ID_MANUAL_NEUTRAL_MODE:
			int g, a, b;
			hDC = GetDC(MainWnd);
			manual = (manual + 1) % 2;
			if (manual == 1)
			{
				if (rubber_mode == 1)
				{
					manual_coord_count = 0;
					/* Copy the final contour of rubbber encountered previously */
					for ( p = 0; p < pixel_count; p++)
					{
						x_coordinates_manual[p] = x_value[p];
						y_coordinates_manual[p] = y_value[p];
						manual_coord_count++;
					}

					for (g = 0; g < pixel_count; g++)
					{
						for (a = y_value[g] - 2; a <= y_value[g] + 2; a++)
						{
							for (b = x_value[g] - 2; b <= x_value[g] + 2; b++)
							{
								/* color the pixels moved along with the old ones */
								SetPixel(hDC, b, a, RGB(0, 0, 255));
								//Sleep(1000);
							}
						}
					}
					ReleaseDC(MainWnd, hDC);

				}
				else if (balloon_mode == 1)
				{
					manual_coord_count = 0;
					/* Copy the final contour of balloon encountered previously */
					for (p = 0; p < iindex_b; p++)
					{
						x_coordinates_manual[p] = x_coord_balloon[p];
						y_coordinates_manual[p] = y_coord_balloon[p];
						manual_coord_count++;
					}

					for (g = 0; g < pixel_count; g++)
					{
						for (a = y_coord_balloon[g] - 2; a <= y_coord_balloon[g] + 2; a++)
						{
							for (b = x_coord_balloon[g] - 2; b <= x_coord_balloon[g] + 2; b++)
							{
								/* color the pixels moved along with the old ones */
								SetPixel(hDC, b, a, RGB(0, 0, 255));
							}
						}
					}
					ReleaseDC(MainWnd, hDC);
				}

				/* Initiate the Neutral mode thread */
				_beginthread(Neutral_mode, 0, MainWnd);
			}
		case ID_CLEAR:
			clear_mode = 1;
			step_mode = 0;
			play_mode = 0;
			PaintImage();
			break;
		case ID_RUBBER_BAND_MODEL:
			rubber = (rubber + 1) % 2;
			pixel_count = 0;
			if (rubber == 1)
			{
				rubber_mode = 1;
				PaintImage();
				for (int d = 0; d < xy_count; d++)
				{
					if (d % 5 == 0)
					{
						x_value[pixel_count] = x_coordinates[d];
						y_value[pixel_count] = y_coordinates[d];
						pixel_count++;
					}
				}

				/* begin the rubber band thread */
				_beginthread(Active_contour_Rubber_band, 0, MainWnd);
			}
			break;
		case ID_COLOR:
			color_box.lStructSize = sizeof(CHOOSECOLORA);
			color_box.hwndOwner = hwnd;
			color_box.rgbResult = current_rgb;
			color_box.lpCustColors = (LPDWORD)custom_clrs;
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
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_PAINT:
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_LBUTTONDOWN:
		/* Handlers for region growing disabled */
		//DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_DIALOG_BOX_1), hWnd, Dialog_Box1_handle);
		//DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_DIALOG_BOX_2), hWnd, Dialog_Box2_handle);
		shift_pressed = 0;
		shift_pressed = GetKeyState(VK_SHIFT);
		if ((shift_pressed & 0x8000) != 0)
		{
			shift_pressed = 1;
			last_Saved_x_coord = LOWORD(lParam);
			last_Saved_y_coord = HIWORD(lParam);
		}
		break;
	case WM_RBUTTONDOWN:
		mouse_x = LOWORD(lParam);
		mouse_y = HIWORD(lParam);
		//clear_mode = 0;
		iindex_b = 0;
		int m = 0;
		y_coord_balloon = (int*)calloc(ROWS * COLS, sizeof(int));
		x_coord_balloon = (int*)calloc(ROWS * COLS, sizeof(int));

		if (mouse_x >= 0 && mouse_x < COLS && mouse_y >= 0 && mouse_y < ROWS)
		{
			hDC = GetDC(MainWnd);
			int m = 0;
			xy_count_balloon = 0;
			for (double i = 0; i < 2 * M_PI; i += 0.001)
			{
				if (m % 3 == 0)
				{
					x_coord_balloon[m/3] = (int)(mouse_x + 10 * cos(i));
					y_coord_balloon[m/3] = (int)(mouse_y + 10 * sin(i));
				}
				m++;
			}
			iindex_b = m / 3;
			for (i = 0; i < iindex_b; i++)
			{
				for (int r = y_coord_balloon[i] -2; r <= y_coord_balloon[i]+2; r++)
				{
					for (int c = x_coord_balloon[i] - 2; c <= x_coord_balloon[i]+2; c++)
					{
						SetPixel(hDC, c, r, RGB(0, 100, 150));
					}
				}
			}
			ReleaseDC(MainWnd, hDC);
		}
		break;
	case WM_MOUSEMOVE:
		downsampl_cnt = 0;
		if (ShowPixelCoords == 1)
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
			{
				sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
				hDC = GetDC(MainWnd);
				TextOut(hDC, 0, 0, text, strlen(text));		/* draw text on the window */
				if (BigDots == 0)
					SetPixel(hDC, xPos, yPos, RGB(255, 0, 0));	/* color the cursor position red */
				else
				{
					for (x = -2; x <= 2; x++)
						for (y = -2; y <= 2; y++)
							SetPixel(hDC, xPos + x, yPos + y, RGB(255, 0, 0));
				}
				ReleaseDC(MainWnd, hDC);
			}
		}

		if (WM_LBUTTONDOWN && wParam)
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);


			if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
			{
				/* Downsampling input to 5th point */
				if (downsampl_cnt % 5 == 0)
				{
					x_coordinates[xy_count] = xPos;
					y_coordinates[xy_count] = yPos;
					xy_count++;
				}
				downsampl_cnt++;
				hDC = GetDC(MainWnd);

				for (i = -1; i <= 1; i++)
					for (j = -1; j <= 1; j++)
					{
						SetPixel(hDC, xPos + i, yPos + j, RGB(255, 0, 0));
					}
				ReleaseDC(MainWnd, hDC);
			}

		}

		if (WM_LBUTTONDOWN && (shift_pressed == 1))
		{
			hDC = GetDC(MainWnd);

			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			
			if (rubber_mode == 1)
			{
				for (p = 0; p < pixel_count; p++)
				{
					if ((x_coordinates_manual[p] == last_Saved_x_coord) && (y_coordinates_manual[p] == last_Saved_y_coord))
					{
						x_coordinates_manual[p] = xPos;
						y_coordinates_manual[p] = yPos;
						SetPixel(hDC, xPos, yPos, RGB(0, 0, 0));
					}
				}
				ReleaseDC(MainWnd, hDC);
			}
		}

		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_KEYDOWN:
		if (wParam == 'j' || wParam == 'J')
		{
			Button_J_Press = 1;
		}
		//PostMessage(MainWnd,WM_COMMAND, ID_STEP_MODE,0);	  /* send message to self */
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
		hDC = GetDC(MainWnd);
		SetPixel(hDC, TimerCol, TimerRow, RGB(0, 0, 255));	/* color the animation pixel blue */
		ReleaseDC(MainWnd, hDC);
		TimerRow++;
		TimerCol += 2;
		break;
	case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	}

	hMenu = GetMenu(MainWnd);
	if (ShowPixelCoords == 1)
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_UNCHECKED);
	if (rubber == 1)
		CheckMenuItem(hMenu, ID_RUBBER_BAND_MODEL, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_RUBBER_BAND_MODEL, MF_UNCHECKED);
	if (balloon == 1)
		CheckMenuItem(hMenu, ID_BALLOON_MODEL, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_BALLOON_MODEL, MF_UNCHECKED);
	if (manual == 1)
		CheckMenuItem(hMenu, ID_MANUAL_NEUTRAL_MODE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_MANUAL_NEUTRAL_MODE, MF_UNCHECKED);
	if (BigDots == 1)
		CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_UNCHECKED);
	if (Original_image == 1)
		CheckMenuItem(hMenu, ID_ORIGINAL_IMAGE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_ORIGINAL_IMAGE, MF_UNCHECKED);
	if (sobel_image == 1)
		CheckMenuItem(hMenu, ID_SOBEL_FILTERED_IMAGE, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_SOBEL_FILTERED_IMAGE, MF_UNCHECKED);
	if (step_mode == 1 && play_mode == 0)
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

void PaintSobelFilteredImage()
{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO* bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char* sobel_display;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

	/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	sobel_display = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			sobel_display[r * DISPLAY_COLS + c] = sobel_output[r * COLS + c];

	BeginPaint(MainWnd, &Painter);
	hDC = GetDC(MainWnd);
	bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
	bm_info_header.biWidth = DISPLAY_COLS;
	bm_info_header.biHeight = -DISPLAY_ROWS;
	bm_info_header.biPlanes = 1;
	bm_info_header.biBitCount = 8;
	bm_info_header.biCompression = BI_RGB;
	bm_info_header.biSizeImage = 0;
	bm_info_header.biXPelsPerMeter = 0;
	bm_info_header.biYPelsPerMeter = 0;
	bm_info_header.biClrUsed = 256;
	bm_info_header.biClrImportant = 256;
	bm_info = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bm_info->bmiHeader = bm_info_header;
	for (i = 0; i < 256; i++)
	{
		bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
		bm_info->bmiColors[i].rgbReserved = 0;
	}

	SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
		0, /* first scan line */
		DISPLAY_ROWS, /* number of scan lines */
		sobel_display, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

	free(sobel_display);
	free(bm_info);
}


void PaintImage()
{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO* bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char* DisplayImage;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

	/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	DisplayImage = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			DisplayImage[r * DISPLAY_COLS + c] = Gray_scale_image[r * COLS + c];

	BeginPaint(MainWnd, &Painter);
	hDC = GetDC(MainWnd);
	bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
	bm_info_header.biWidth = DISPLAY_COLS;
	bm_info_header.biHeight = -DISPLAY_ROWS;
	bm_info_header.biPlanes = 1;
	bm_info_header.biBitCount = 8;
	bm_info_header.biCompression = BI_RGB;
	bm_info_header.biSizeImage = 0;
	bm_info_header.biXPelsPerMeter = 0;
	bm_info_header.biYPelsPerMeter = 0;
	bm_info_header.biClrUsed = 256;
	bm_info_header.biClrImportant = 256;
	bm_info = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bm_info->bmiHeader = bm_info_header;
	for (i = 0; i < 256; i++)
	{
		bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
		bm_info->bmiColors[i].rgbReserved = 0;
	}

	SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
		0, /* first scan line */
		DISPLAY_ROWS, /* number of scan lines */
		DisplayImage, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

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

void RGB_To_Grayscale()
{
	int r, c;
	for (r = 0; r < ROWS; r++)
	{
		for (c = 0; c < COLS; c++)
		{
			Gray_scale_image[r * COLS + c] = (int)((OriginalImage[(r * COLS + c) * 3] + OriginalImage[(r * COLS + c) * 3 + 1] + OriginalImage[(r * COLS + c) * 3 + 2]) / 3);
		}
	}
}

void sobel_operator()
{
	int Gx = 0;
	int Gy = 0;
	int min = 0;
	int max = 500;
	int r, c, i, j;
	unsigned char* sobel_before_normalisation;
	sobel_before_normalisation = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));

	int x_kernel[9] = { -1,0,1,-2,0,2,-1,0,1 };
	int y_kernel[9] = { -1,-2,-1,0,0,0,1,2,1 };

	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			Gx = Gy = 0;
			for (i = -1; i <= 1; i++)
			{
				for (j = -1; j <= 1; j++)
				{
					Gx += Gray_scale_image[(r + i) * COLS + (c + j)] * x_kernel[(i + 1) * 3 + (j + 1)];
					Gy += Gray_scale_image[(r + i) * COLS + (c + j)] * y_kernel[(i + 1) * 3 + (j + 1)];
				}
			}
			sobel_before_normalisation[r * COLS + c] = sqrt(pow(Gx, 2) + pow(Gy, 2));
			if (sobel_before_normalisation < min)
				min = sobel_before_normalisation[r * COLS + c];
			if (sobel_before_normalisation > max)
				max = sobel_before_normalisation[r * COLS + c];
		}
	}

	/* Normalise the output of convolution to range 0 - 255 */
	for (r = 0; r < ROWS; r++)
	{
		for (c = 0; c < COLS; c++)
		{
			sobel_output[r * COLS + c] = ((sobel_before_normalisation[r * COLS + c] - min) * (255 - 0) / (max - min)) + 0;
		}
	}
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

	ThreadRow = ThreadCol = 0;
	while (ThreadRunning == 1)
	{
		hDC = GetDC(MainWnd);
		SetPixel(hDC, ThreadCol, ThreadRow, RGB(0, 255, 0));	/* color the animation pixel green */
		sprintf(text, "%d,%d     ", ThreadRow, ThreadCol);
		TextOut(hDC, 300, 0, text, strlen(text));		/* draw text on the window */
		ReleaseDC(MainWnd, hDC);
		ThreadRow += 3;
		ThreadCol++;
		Sleep(100);		/* pause 100 ms */
	}
}

void ExplosionThread(HWND AnimationWindowHandle)

{
	HDC		hDC;
	int		width;
	int		x, y, cx, cy;

	cx = mouse_x;
	cy = mouse_y;
	width = 3;
	while (width < 62)
	{
		hDC = GetDC(MainWnd);
		y = cy;
		for (x = cx - width; x <= cx + width; x++)
		{
			SetPixel(hDC, x, y - width, current_rgb);	/* color the box red */
			SetPixel(hDC, x, y + width, current_rgb);	/* color the box red */
		}
		x = cx;
		for (y = cy - width; y <= cy + width; y++)
		{
			SetPixel(hDC, x - width, y, current_rgb);	/* color the box red */
			SetPixel(hDC, x + width, y, current_rgb);	/* color the box red */
		}
		ReleaseDC(MainWnd, hDC);
		width += 2;
		Sleep(10);		/* pause 100 ms */
	}
}

void RegGrowThread(HWND AnimationWindowHandle)
{
	int r, c, r1, c1, cx, cy, r2, c2;
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
	int* indices, int* count)
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

void Active_contour_Rubber_band()
{
	double Internal_Energy_1[100];
	double Internal_Energy_1_normalised[100];
	double Internal_Energy_2[100];
	double Internal_Energy_2_normalised[100];
	double External_Energy[100];
	double External_Energy_normalised[10000];
	double total_energy[100];
	int n = 0, i, j, r, c, k, m;
	int iterations = 80;
	int c1, c2, c3;
	int dist1 = 0, dist2 = 0;
	int avg_dist = 0;
	HDC hDC;
	int foll_x_value, foll_y_value, sum = 0;
	int min_row;
	int min_col;
	int window_size = 5;

	/* Apply sobel filter on Gray scale image*/
	sobel_operator();

	/* Iterating for 80 times to get the best result*/
	for (i = 0; i < iterations; i++)
	{
		/* iterating for the number of pixels */
		for (k = 0; k < pixel_count; k++)
		{

			for (m = 0; m < pixel_count; m++)
			{
				if (m >= pixel_count - 1)
				{
					foll_x_value = x_value[0];
					foll_y_value = y_value[0];
				}
				else
				{
					foll_x_value = x_value[m + 1];
					foll_y_value = y_value[m + 1];
				}
				sum += sqrt(pow((x_value[m] - foll_x_value), 2) + pow((y_value[m] - foll_y_value), 2));
			}
			avg_dist = sum / pixel_count;

			/* calculate the first internal energy */
			c1 = 0;
			/* checking in the 5x5 window */
			for (r = -2; r < 3; r++)
			{
				for (c = -2; c < 3; c++)
				{
					if (k >= pixel_count - 1)
					{
						dist1 = ((r + y_value[k]) - y_value[0]);
						dist2 = ((c + x_value[k]) - x_value[0]);
						Internal_Energy_1[c1] = pow(dist1, 2) + pow(dist2, 2);
					}
					else
					{
						dist1 = ((r + y_value[k]) - y_value[k + 1]);
						dist2 = ((c + x_value[k]) - x_value[k + 1]);
						Internal_Energy_1[c1] = pow(dist1, 2) + pow(dist2, 2);
					}
					c1++;
				}
			}

			/* Normalise the first energy */
			normalize_energies_rubber(Internal_Energy_1, Internal_Energy_1_normalised, window_size);

			/* Calculate the second internal energy */
			c2 = 0;
			for (r = -2; r < 3; r++)
			{
				for (c = -2; c < 3; c++)
				{
					if (k >= pixel_count - 1)
					{
						dist1 = ((r + y_value[k]) - y_value[0]);
						dist2 = ((c + x_value[k]) - x_value[0]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					else
					{
						dist1 = ((r + y_value[k]) - y_value[k + 1]);
						dist2 = ((c + x_value[k]) - x_value[k + 1]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					c2++;
				}
			}

			/* Normalise the second energy */

			float max_ie2 = Internal_Energy_2[0]; float min_ie2 = Internal_Energy_2[0];
			for (r = 0; r < window_size * window_size; r++)
			{
				if (Internal_Energy_2[r] > max_ie2) { max_ie2 = Internal_Energy_2[r]; }
				if (Internal_Energy_2[r] < min_ie2) { min_ie2 = Internal_Energy_2[r]; }
			}
			for (r = 0; r < window_size; r++)
			{
				for (c = 0; c < window_size; c++)
				{
					Internal_Energy_2_normalised[r * window_size + c] = (Internal_Energy_2[r * window_size + c] - min_ie2) / (max_ie2 - min_ie2);
				}
			}

			/* Calulate external energy */
			c3 = 0;
			for (r = -2; r < 3; r++)
			{
				for (c = -2; c < 3; c++)
				{
					External_Energy[c3] = pow(sobel_output[(y_value[k] + r) * COLS + (x_value[k] + c)], 2);
					c3++;
				}
			}

			/* Normalise the external energy */
			normalize_energies_rubber(External_Energy, External_Energy_normalised, window_size);

			for (r = 0; r < window_size; r++)
			{
				for (c = 0; c < window_size; c++)
				{
					total_energy[r * window_size + c] = (18*Internal_Energy_1_normalised[r * window_size + c]) + (8*Internal_Energy_2_normalised[r * window_size + c]) - (12 * External_Energy_normalised[r * window_size + c]);
				}
			}

			/* Normalise the total energy */
			int minimum_val = total_energy[0];
			int minimum_index = 0;
			min_row = 0;
			min_col = 0;
			for (r = 0; r < window_size * window_size; r++)
			{
				if (total_energy[r] < total_energy[0])
				{
					minimum_val = total_energy[r];
					minimum_index = r;
					min_row = (minimum_index % window_size) - 2;
					min_col = (minimum_index / window_size) - 2;
				}
			}
			x_value[k] = x_value[k] + min_row;
			y_value[k] = y_value[k] + min_col;
		}
		hDC = GetDC(MainWnd);
		PaintImage();
		for (int g = 0; g < pixel_count; g++)
		{
			for (int a = y_value[g] - 2; a <= y_value[g] + 2; a++)
			{
				for (int b = x_value[g] - 2; b <= x_value[g] + 2; b++)
				{
					SetPixel(hDC, b, a, RGB(0, 0, 255));
					sprintf(str, "Value of Pi = %f", M_PI);
					puts(str);
				}
			}
		}
		ReleaseDC(MainWnd, hDC);
	}
	_endthread();

}

void normalize_energies_rubber(double* energy, double* normalised_energy, int window_size)
{
	int r, c;
	int min = energy[0];
	int max = energy[0];

	for (r = 0; r < window_size; r++)
	{
		for (c = 0; c < window_size; c++)
		{
			/* calculating min and max in the image so that it can be normalised to 0-1 later */
			if (energy[r * window_size + c] < min)
				min = energy[r * 5 + c];
			if (energy[r * window_size + c] > max)
				max = energy[r * window_size + c];
		}
	}

	/* if max and min are same, then return */
	if ((int)(max - min) == 0)
	{
		printf("unable to calculate further\n");
		exit(0);
	}

	/* Normalise the energies to change the range to 0-1 */
	for (r = 0; r < window_size; r++)
	{
		for (c = 0; c < window_size; c++)
		{
			normalised_energy[r * window_size + c] = (energy[r * window_size + c] - min) * (1) / (max - min) + 0;
		}
	}
}

void normalize_energies_balloon(double* energy, double* normalised_energy)
{
	int r, c;
	float min = energy[0];
	float max = energy[0];

	for (r = 0; r < 7; r++)
	{
		for (c = 0; c < 7; c++)
		{
			/* calculating min and max in the image so that it can be normalised to 0-1 later */
			if (energy[r * 7 + c] < min)
				min = energy[r * 7 + c];
			if (energy[r * 7 + c] > max)
				max = energy[r * 7 + c];
		}
	}

	/* if max and min are same, then return */
	if ((int)(max - min) == 0)
	{
		printf("unable to calculate further\n");
		exit(0);
	}

	/* Normalise the energies to change the range to 0-1 */
	for (r = 0; r < 7; r++)
	{
		for (c = 0; c < 7; c++)
		{
			normalised_energy[r * 7 + c] = (energy[r * 7 + c] - min) * (1) / (max - min) + 0;
		}
	}
}

void Active_contour_Balloon_Model()
{
	int i, row, col, m, k, d, c1, c2, c3, min_row, min_col, r, c;
	int dist1, dist2;
	int iterations = 80;
	double Internal_Energy_1[100];
	double Internal_Energy_1_normalised[100];
	double Internal_Energy_2[100];
	double Internal_Energy_2_normalised[100];
	double External_Energy[100];
	double External_Energy_normalised[100];
	double total_energy[100];
	int foll_x_value, foll_y_value, sum=0, avg_dist=0, avg_x=0, avg_y=0;
	HDC hDC;
	int z;
	int count_balloon_coord;
	int x_balloon_coord[1000];
	int y_balloon_coord[1000];

	/* Apply sobel filter to image */
	sobel_operator();

	/* Iterate for 80 times for best fit */
	for (i = 0; i < iterations; i++)
	{

		/* Iterate over all mouse moment pixels which are captured in array */
		for (k = 0; k < iindex_b; k++)
		{
			c2 = 0, c3 = 0, c1 = 0;
			/* Calculate the average distance between each pixels */

			for (m = 0; m < iindex_b; m++)
			{
				if (m >= iindex_b - 1)
				{
					foll_x_value = x_coord_balloon[0];
					foll_y_value = y_coord_balloon[0];
				}
				else
				{
					foll_x_value = x_coord_balloon[m + 1];
					foll_y_value = y_coord_balloon[m + 1];
				}
				sum += sqrt(pow((x_coord_balloon[m] - foll_x_value), 2) + pow((y_coord_balloon[m] - foll_y_value), 2));
			}
			avg_dist = sum / iindex_b;

			/* calculate the average value of x and y pixels */

			for (d = 0; d < iindex_b; d++)
			{
				avg_x = avg_x + x_coord_balloon[d];
				avg_y = avg_y + y_coord_balloon[d];
			}
			avg_x = avg_x / iindex_b;
			avg_y = avg_y / iindex_b;

			/* Calculate the first internal energy */

			for (row = -3; row < 4; row++)
			{
				for (col = -3; col < 4; col++)
				{
					dist1 = ((row + y_coord_balloon[k]) - avg_y);
					dist2 = ((col + x_coord_balloon[k]) - avg_x);
					Internal_Energy_1[c1] = pow(dist1, 2) + pow(dist2, 2);
					c1++;
				}
			}

			/* Normalise the first internal energy */

			normalize_energies_balloon(Internal_Energy_1, Internal_Energy_1_normalised);

			/* Calculate the second internal energy */

			for (row = -3; row < 4; row++)
			{
				for (col = -3; col < 4; col++)
				{
					if (k >= (iindex_b - 1))
					{
						dist1 = ((row + y_coord_balloon[k]) - y_coord_balloon[0]);
						dist2 = ((col + x_coord_balloon[k]) - x_coord_balloon[0]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					else
					{
						dist1 = ((row + y_coord_balloon[k]) - y_coord_balloon[k + 1]);
						dist2 = ((col + x_coord_balloon[k]) - x_coord_balloon[k + 1]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					c2++;
				}
			}

			/* Normalise the second internal energy */

			normalize_energies_balloon(Internal_Energy_2, Internal_Energy_2_normalised);

			/* Caclulate the external energy */

			for (row = -3; row < 4; row++)
			{
				for (col = -3; col < 4; col++)
				{
					External_Energy[c3] = pow(sobel_output[(y_coord_balloon[k] + row) * COLS + (x_coord_balloon[k] + col)], 2);
					c3++;
				}
			}

			/* Normalise the external energy */

			float max_ee = External_Energy[0]; float min_ee = External_Energy[0];
			for (r = 0; r < 7 * 7; r++)
			{
				if (External_Energy[r] > max_ee) { max_ee = External_Energy[r]; }
				if (External_Energy[r] < min_ee) { min_ee = External_Energy[r]; }
			}
			for (r = 0; r < 7; r++)
			{
				for (c = 0; c < 7; c++)
				{
					External_Energy_normalised[r * 7 + c] = (External_Energy[r * 7 + c] - min_ee) / (max_ee - min_ee);
				}
			}

			for (row = 0; row < 7; row++)
			{
				for (col = 0; col < 7; col++)
				{
					if (i <= 10)
					{
						total_energy[row * 7 + col] = -(20*Internal_Energy_1_normalised[row * 7 + col]) + (6*Internal_Energy_2_normalised[row * 7 + col]) - (6 * External_Energy_normalised[row * 7 + col]);
					}
					else if ((i > 10) && (i <= 40))
					{
						total_energy[row * 7 + col] = -(5 * Internal_Energy_1_normalised[row * 7 + col]) + (40 * Internal_Energy_2_normalised[row * 7 + col]) - (15 * External_Energy_normalised[row * 7 + col]);
					}
					else
					{
						total_energy[row * 7 + col] = -(10 * Internal_Energy_1_normalised[row * 7 + col]) + (0 * Internal_Energy_2_normalised[row * 7 + col]) - (40 * External_Energy_normalised[row * 7 + col]);
					}
				}
			}

			/* Normalise the total energy */
			int minimum_val = total_energy[0];
			int minimum_index = 0;
			min_row = 0;
			min_col = 0;
			for (row = 0; row < 7 * 7; row++)
			{
				if (total_energy[row] < total_energy[0])
				{
					minimum_val = total_energy[row];
					minimum_index = row;
					min_row = (minimum_index % 7) - 3;
					min_col = (minimum_index / 7) - 3;
				}
			}
			y_coord_balloon[k] = y_coord_balloon[k] + min_col;
			x_coord_balloon[k] = x_coord_balloon[k] + min_row;
		}
		hDC = GetDC(MainWnd);
		PaintImage();
		for (int g = 0; g < iindex_b; g++)
		{
			for (int a = y_coord_balloon[g] - 2; a <= y_coord_balloon[g] + 2; a++)
			{
				for (int b = x_coord_balloon[g] - 2; b <= x_coord_balloon[g] + 2; b++)
				{
					SetPixel(hDC, b, a, RGB(255, 0, 0));
				}
			}
		}
		ReleaseDC(MainWnd, hDC);
	}
	_endthread();
}
 
void Neutral_mode()
{
	int i, row, col, m, k, d, c1, c2, c3, min_row, min_col, r, c;
	int dist1, dist2;
	int iterations = 80;
	double Internal_Energy_1[100];
	double Internal_Energy_1_normalised[100];
	double Internal_Energy_2[100];
	double Internal_Energy_2_normalised[100];
	double External_Energy[100];
	double External_Energy_normalised[100];
	double total_energy[100];
	int foll_x_value, foll_y_value, sum = 0, avg_dist = 0, avg_x = 0, avg_y = 0;
	HDC hDC;
	int window_size = 7;

	for (i = 0; i < iterations; i++)
	{
		for (k = 0; k < manual_coord_count; k++)
		{
			/* calculate first internal energy */
			c1 = 0, c2 = 0, c3 = 0;
			for (row = -window_size / 2; row < window_size / 2; row++)
			{
				for (col = -window_size / 2; col < window_size / 2; col++)
				{
					if (k >= manual_coord_count - 1)
					{
						dist1 = (row + y_coordinates_manual[k]) - y_coordinates_manual[0];
						dist2 = (col + x_coordinates_manual[k]) - x_coordinates_manual[0];
						Internal_Energy_1[c1] = pow(dist1, 2) + pow(dist2, 2);
					}
					else
					{
						dist1 = (row + y_coordinates_manual[k]) - y_coordinates_manual[k + 1];
						dist2 = (col + x_coordinates_manual[k]) - x_coordinates_manual[k + 1];
						Internal_Energy_1[c1] = pow(dist1, 2) + pow(dist2, 2);
					}
				}
			}

			/* Normalise the first internal energy */

			normalize_energies_balloon(Internal_Energy_1, Internal_Energy_1_normalised);

			/* Calculate second internal energy */
			for (m = 0; m < manual_coord_count; m++)
			{
				if (m >= manual_coord_count - 1)
				{
					foll_x_value = x_coordinates_manual[0];
					foll_y_value = y_coordinates_manual[0];
				}
				else
				{
					foll_x_value = x_coordinates_manual[m + 1];
					foll_y_value = y_coordinates_manual[m + 1];
				}
				sum += sqrt(pow((x_coord_balloon[m] - foll_x_value), 2) + pow((y_coord_balloon[m] - foll_y_value), 2));
			}
			avg_dist = sum / manual_coord_count;


			for (row = -window_size / 2; row < window_size / 2; row++)
			{
				for (col = -window_size / 2; col < window_size / 2; col++)
				{
					if (k >= (iindex_b - 1))
					{
						dist1 = ((row + y_coordinates_manual[k]) - y_coordinates_manual[0]);
						dist2 = ((col + x_coordinates_manual[k]) - x_coordinates_manual[0]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					else
					{
						dist1 = ((row + y_coordinates_manual[k]) - y_coordinates_manual[k + 1]);
						dist2 = ((col + x_coordinates_manual[k]) - x_coordinates_manual[k + 1]);
						Internal_Energy_2[c2] = pow(avg_dist - sqrt(pow(dist1, 2) + pow(dist2, 2)), 2);
					}
					c2++;

				}
			}

			/* Normalise the second internal energy */

			normalize_energies_balloon(Internal_Energy_1, Internal_Energy_1_normalised);

			/* Caclulate the external energy */

			for (row = -window_size / 2; row < window_size / 2; row++)
			{
				for (col = -window_size / 2; col < window_size / 2; col++)
				{
					External_Energy[c3] = pow(sobel_output[(y_coordinates_manual[k] + row) * COLS + (x_coordinates_manual[k] + col)], 2);
					c3++;
				}
			}

			/* Normalise the external energy */

			normalize_energies_balloon(External_Energy, External_Energy_normalised);


			/* calculate the total energy */
			for (r = 0; r < window_size; r++)
			{
				for (c = 0; c < window_size; c++)
				{
					total_energy[r * window_size + c] = (2 * Internal_Energy_1_normalised[r * window_size + c]) + (7 * Internal_Energy_2_normalised[r * window_size + c]) + (4 * External_Energy_normalised[r * window_size + c]);
				}
			}

			/* Normalise the total energy */
			int minimum_val = total_energy[0];
			int minimum_index = 0;
			min_row = 0;
			min_col = 0;
			for (r = 0; r < window_size * window_size; r++)
			{
				if (total_energy[r] < total_energy[0])
				{
					minimum_val = total_energy[r];
					minimum_index = r;
					min_row = (minimum_index % window_size) - window_size / 2;
					min_col = (minimum_index / window_size) - window_size / 2;
				}
			}
			x_coordinates_manual[k] = x_coordinates_manual[k] + min_row;
			y_coordinates_manual[k] = y_coordinates_manual[k] + min_col;
		}
			hDC = GetDC(MainWnd);
			PaintImage();
			for (int g = 0; g < pixel_count; g++)
			{
				for (int a = y_value[g] - 2; a <= y_value[g] + 2; a++)
				{
					for (int b = x_value[g] - 2; b <= x_value[g] + 2; b++)
					{
						SetPixel(hDC, b, a, RGB(0, 0, 255));
					}
				}
			}
		ReleaseDC(MainWnd, hDC);
	}
}