//
//  Active_contours.h
//  CV_Class_1
//
//  Created by Pavan V Hosatti on 08/11/22.
//

void ActiveContour(float *sobel_image, int *contour_row, int *contour_col, int ROWS, int COLS);
void InvertSobel(float *sobel_image, int ROWS, int COLS);
void Min(int *min_row, int *min_col, float *temp);
int Distance(int x1, int x2, int y1, int y2);
void InternalEnergy1(float *energy_array, int *contour_row, int *contour_col, int i);
void InternalEnergy2(float *energy_array, int *contour_row, int *contour_col, int avg, int i);
void ExternalEnergy(float *energy_array, float *sobel_image, int *contour_row, int *contour_col, int COLS, int i);
void SobelFilter(unsigned char *image, float *sobel_image, int ROWS, int COLS);
void Normalize(float *temp, int ROWS, int COLS, int range);
void MakeFinal(unsigned char *image, int *contour_row, int *contour_col, int ROWS, int COLS);
void Float2Unsigned(float *input, unsigned char *output, int ROWS, int COLS);

const int F1[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
const int F2[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
