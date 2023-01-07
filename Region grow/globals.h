
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320
#define MAX_QUEUE 10000

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;

		// Image data
unsigned char	*OriginalImage;
unsigned char* image;
int				ROWS,COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;
int		BigDots;
int		mouse_x,mouse_y;
CHOOSECOLORA color_box;
HBRUSH hbrush;
unsigned long current_rgb;
static COLORREF custom_clrs[16];
HWND hwnd;
int Pixel_intensity;
int Centroid_dist;
int step_mode;
int play_mode;
int Button_J_Press;
int clear_mode;
int thread_count;


		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage();
void AnimationThread(void *);		/* passes address of window */
void ExplosionThread(void*);		/* passes address of window */
