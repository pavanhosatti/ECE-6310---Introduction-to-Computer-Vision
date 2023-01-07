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
unsigned char* OriginalImage;
unsigned char* Gray_scale_image;
unsigned char* sobel_output;
unsigned char* image;
int				ROWS, COLS;
int* x_coord_balloon;
int* y_coord_balloon;
int xy_count_balloon;
int manual;
int rubber_mode;
int balloon_mode;
int x_coordinates_manual[1000];
int y_coordinates_manual[1000];
int shift_pressed;
int last_Saved_x_coord;
int last_Saved_y_coord;
int p;

#define TIMER_SECOND	1			/* ID of timer used for animation */

// Drawing flags
int		TimerRow, TimerCol;
int		ThreadRow, ThreadCol;
int		ThreadRunning;
int		BigDots;
int		mouse_x, mouse_y;
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
int x_coordinates[1000] = { 0 };
int y_coordinates[1000] = { 0 };
int x_value[1000] = { 0 };
int y_value[1000] = { 0 };
int x_value_balloon[1000] = { 0 };
int y_value_balloon[1000] = { 0 };
int xy_count = 0;
int i, j;
int rubber;
int balloon;
int pixel_count;
char str[50];
int downsampl_cnt;
int iindex_b;
int Original_image;
int sobel_image;
int manual_coord_count;

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PaintImage();
void AnimationThread(void*);		/* passes address of window */
void ExplosionThread(void*);		/* passes address of window */
void RegGrowThread();
void sobel_operator();
void RGB_To_Grayscale();
void normalize_energies_rubber(double* energy, double* normalised_energy, int window_size);
void Active_contour_Rubber_band();
BOOL CALLBACK Dialog_Box1_handle(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
BOOL CALLBACK Dialog_Box2_handle(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void Active_contour_Balloon_Model();
void normalize_energies_balloon(double* energy, double* normalised_energy);
void PaintSobelFilteredImage();
void Neutral_mode();