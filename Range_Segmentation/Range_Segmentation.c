//
//  Range_Segmentation.c
//
//  Created by Pavan V Hosatti on 04/12/22.
//

#include "Range_Segmentation.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Macro definitions */
#define MAX_QUEUE 10000
#define PI 3.142857

/* Function declarations */
void Calc_Threshold(unsigned char * range, unsigned char * Threshold_img);
void Range_to_3D (unsigned char * range_image);
void check_seed(int * flag, int r, int c, unsigned char * Threshold_img, unsigned char* labels);
void Calc_Surface_Normal (double * Surface_Normal_X, double * Surface_Normal_Y, double * Surface_Normal_Z);
void Region_Grow(unsigned char* image,
                unsigned char* labels,
                int r, int c,
                int paint_over_label,
                int new_label,
                int* indices,int *count, double * Surface_Normal_X, double * Surface_Normal_Y, double * Surface_Normal_Z);

/* Global variables */
int ROWS, COLS, ROWS_reflectance, COLS_reflectance;
double        P[3][128*128];
unsigned char * threshold_img;

int main()
{
    unsigned char * range_img;
    unsigned char * reflectance_img;
    unsigned char * labels;
    unsigned char * output_image;
    int * indices;
    FILE *fpt;
    FILE * fpt1;
    FILE * fpt2;
    int BYTES,i,j, r, c, m,z;
    char header[256];
    char header_reflectance[256];
    int total_regions;
    int valid;
    int count;
    int flag;
    int RegionSize = 0;
    unsigned char * final_colored_image;
    int temp = 0;
    double        * Surface_Normal_X;
    double        * Surface_Normal_Y;
    double        * Surface_Normal_Z;
    
    /* Load the Range image */
    fpt1 = fopen("chair-range.ppm", "rb");
    if (fpt1 == NULL)
    {
        printf("Unable to open chair-range.ppm file for reading\n");
        exit(0);
    }
    /* Read the image data abd check for validity */
    fscanf(fpt1,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
    if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
        printf("Input image chair.ppm is not a greyscale 8-bit PPM image\n");
        exit(0);
    }
    
    /* Now load the Reflectance image */
    fpt2 = fopen("chair-reflectance.ppm", "rb");
    if (fpt2 == NULL)
    {
        printf("Unable to open chair-reflectance.ppm file for reading\n");
        exit(0);
    }
    /* Read the image data abd check for validity */
    fscanf(fpt2,"%s %d %d %d",header_reflectance,&COLS_reflectance,&ROWS_reflectance,&BYTES);
    if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
        printf("Input image chair.ppm is not a greyscale 8-bit PPM image\n");
        exit(0);
    }
    
    /* Initialise range image and copy the extracted data */
    range_img = (unsigned char * )calloc(ROWS * COLS, sizeof(unsigned char));
    header[0]=fgetc(fpt1);
    fread(range_img,sizeof(unsigned char),ROWS*COLS,fpt1);
    
    /* Initialise reflectance image and copy the extracted data  */
    reflectance_img = (unsigned char * )calloc(ROWS_reflectance * COLS_reflectance, sizeof(unsigned char));
    header_reflectance[0]=fgetc(fpt2);
    fread(reflectance_img,sizeof(unsigned char),ROWS_reflectance*COLS_reflectance,fpt2);
    
    /* Calculate the threshold image (select a threshold for range image) */
    threshold_img = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
    Calc_Threshold(range_img, threshold_img);
    
    fpt = fopen("threshold_output.ppm", "wb");
    if (fpt == NULL)
    {
        printf("unable to open file threshold_output.ppm for reading\n");
        exit(0);
    }
    fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
    fwrite(threshold_img, ROWS*COLS, sizeof(unsigned char), fpt);
    fclose(fpt);
    
    /* Convert range image into 3D cartesian coordinate data */
    Range_to_3D (range_img);
    
    /* Assign memory for Surface normal output variables */
    Surface_Normal_X = (double *)calloc(ROWS * COLS, sizeof(double));
    Surface_Normal_Y = (double *)calloc(ROWS * COLS, sizeof(double));
    Surface_Normal_Z = (double *)calloc(ROWS * COLS, sizeof(double));
    
    /* Calculate surface Normal for image */
    Calc_Surface_Normal(Surface_Normal_X, Surface_Normal_Y, Surface_Normal_Z);
    
    /* Region Growing */
    /* initialise the variables */
    labels = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
    indices = (int *)calloc(ROWS*COLS, sizeof(unsigned char));
    
    total_regions = 0;
    
    for (r = 0; r < ROWS; r++)
    {
        for(c = 0; c < COLS; c++)
        {
            /* determine in the window of 5x5, the seed */
            flag = 1;
            check_seed(&flag, r,c, threshold_img, labels);

            if (flag == 1)
            {
                
                /* Invoke Region Grow function */
                Region_Grow(threshold_img, labels, r, c, 0, total_regions, indices, &RegionSize, Surface_Normal_X, Surface_Normal_Y, Surface_Normal_Z);
                        
                /* If region size was less than 70, its not taken into count and set to 0 */
                
                if (RegionSize < 70)
                {
                    for (m = 0; m < RegionSize; m++)
                    {
                        labels[indices[m]] = 0;
                    }
                    total_regions--;
                }
                else
                {
                    total_regions++;
                    printf("Region  %d with total number of pixels %d\n", total_regions, RegionSize);
                }
            }
        }
    }
        
    fpt2 = fopen("final.ppm", "wb");
    if (fpt2 == NULL)
    {
        printf("unable to open file final.ppm for reading\n");
        exit(0);
    }
    fprintf(fpt2, "P5 %d %d 255\n", COLS, ROWS);
    fwrite(labels, 1, ROWS*COLS, fpt2);
    fclose(fpt2);
}

void Calc_Threshold(unsigned char * range, unsigned char * Threshold_img)
{
    int i;
    int Threshold = 137;
    
    for (i = 0; i < ROWS * COLS; i++)
    {
        if (range[i] <= Threshold)
        {
            Threshold_img[i] = 0;
        }
        else
        {
            Threshold_img[i] = 255;
        }
    }
}

void check_seed(int * flag, int r, int c, unsigned char * Threshold_img, unsigned char* labels)
{
    int i, j;
    for (i = -2; i < 3; i++)
    {
        for (j = -2; j < 3; j++)
        {
            if (threshold_img[(r+i)*COLS+(c+j)] == 255 || labels[(r+i)*COLS+(c+j)] !=0)
            {
                *flag = 0;
            }
        }
    }
}

/* Range image to 3D conversion (provided in the course website) */

void Range_to_3D (unsigned char * range_image)
{
    int    r,c;
    double    cp[7];
    double    xangle,yangle,dist;
    double    ScanDirectionFlag,SlantCorrection;
    unsigned char    RangeImage[128*128];
    int ImageTypeFlag = 0; // Downward
    
    cp[0]=1220.7;        /* horizontal mirror angular velocity in rpm */
    cp[1]=32.0;        /* scan time per single pixel in microseconds */
    cp[2]=(COLS/2)-0.5;        /* middle value of columns */
    cp[3]=1220.7/192.0;    /* vertical mirror angular velocity in rpm */
    cp[4]=6.14;        /* scan time (with retrace) per line in milliseconds */
    cp[5]=(ROWS/2)-0.5;        /* middle value of rows */
    cp[6]=10.0;        /* standoff distance in range units (3.66cm per r.u.) */

    cp[0]=cp[0]*3.1415927/30.0;    /* convert rpm to rad/sec */
    cp[3]=cp[3]*3.1415927/30.0;    /* convert rpm to rad/sec */
    cp[0]=2.0*cp[0];        /* beam ang. vel. is twice mirror ang. vel. */
    cp[3]=2.0*cp[3];        /* beam ang. vel. is twice mirror ang. vel. */
    cp[1]/=1000000.0;        /* units are microseconds : 10^-6 */
    cp[4]/=1000.0;            /* units are milliseconds : 10^-3 */
    
    switch(ImageTypeFlag)
      {
      case 1:        /* Odetics image -- scan direction upward */
        ScanDirectionFlag=-1;
        break;
      case 0:        /* Odetics image -- scan direction downward */
        ScanDirectionFlag=1;
        break;
      default:        /* in case we want to do this on synthetic model */
        ScanDirectionFlag=0;
        break;
      }

    /* start with semi-spherical coordinates from laser-range-finder: */
        /*            (r,c,RangeImage[r*COLS+c])          */
        /* convert those to axis-independant spherical coordinates:      */
        /*            (xangle,yangle,dist)              */
        /* then convert the spherical coordinates to cartesian:           */
        /*            (P => X[] Y[] Z[])              */

    if (ImageTypeFlag != 3)
      {
      for (r=0; r<ROWS; r++)
        {
        for (c=0; c<COLS; c++)
          {
          SlantCorrection=cp[3]*cp[1]*((double)c-cp[2]);
          xangle=cp[0]*cp[1]*((double)c-cp[2]);
          yangle=(cp[3]*cp[4]*(cp[5]-(double)r))+    /* Standard Transform Part */
        SlantCorrection*ScanDirectionFlag;    /*  + slant correction */
          dist=(double)range_image[r*COLS+c]+cp[6];
          P[2][r*COLS+c]=sqrt((dist*dist)/(1.0+(tan(xangle)*tan(xangle))
        +(tan(yangle)*tan(yangle))));
          P[0][r*COLS+c]=tan(xangle)*P[2][r*COLS+c];
          P[1][r*COLS+c]=tan(yangle)*P[2][r*COLS+c];
          }
        }
      }
}

void Calc_Surface_Normal (double * Surface_Normal_X, double * Surface_Normal_Y, double * Surface_Normal_Z)
{
    int r,c;
    double ax, ay, az, bx, by, bz;
    double x0, x1, x2, y0, y1, y2, z0, z1, z2;
    
    for (r = 0; r < ROWS-3; r++)
    {
        for (c = 0; c < COLS-3; c++)
        {
                x0 = P[0][r*COLS+c];
                y0 = P[1][r*COLS+c];
                z0 = P[2][r*COLS+c];
                
                x1 = P[0][r*COLS+(c+3)];
                y1 = P[1][r*COLS+(c+3)];
                z1 = P[2][r*COLS+(c+3)];
                
                x2 = P[0][(r+3)*COLS+(c)];
                y2 = P[1][(r+3)*COLS+(c)];
                z2 = P[2][(r+3)*COLS+(c)];
                
                ax = x1 - x0;
                ay = y1 - y0;
                az = z1 - z0;
                
                bx = x2 - x0;
                by = y2 - y0;
                bz = z2 - z0;
                
                /* X, Y, Z co-ordinates of 3rd vector */
                Surface_Normal_X[r*COLS+c] = (ay * bz) - (az * by);
                Surface_Normal_Y[r*COLS+c] = ((az * bx) - (ax * bz));
                Surface_Normal_Z[r*COLS+c] = (ax - by) - (ay * bx);
        }
    }
}

void Region_Grow(unsigned char* image,
                unsigned char* labels,
                int r, int c,
                int paint_over_label,
                int new_label,
                int* indices,int *count, double * Surface_Normal_X, double * Surface_Normal_Y, double * Surface_Normal_Z)
{
    int r2, c2;
    int queue[MAX_QUEUE], qh, qt;
    int average, total; /* average and total intensity in growing region */
    int distance;
    int temp;
    *count = 0;
    double distance_1, distance_2, angle_in_radians, dot_product;
    double avg_X, avg_Y, avg_Z, sum_X, sum_Y, sum_Z;
    avg_X = sum_X =  Surface_Normal_X[r*COLS+c];
    avg_Y = sum_Y =  Surface_Normal_Y[r*COLS+c];
    avg_Z = sum_Z =  Surface_Normal_Z[r*COLS+c];
    if (labels[r * COLS + c] != paint_over_label)
        return;
    
    labels[r * COLS + c] = new_label;
    
    if (indices != NULL)
        indices[0] = r * COLS + c;
    
    queue[0] = r * COLS + c;
    qh = 1; /* queue head */
    qt = 0; /* queue tail */
    (*count) = 1;
    
    while (qt != qh)
    {
        for (r2 = -1; r2 <= 1; r2++)
        {
            for (c2 = -1; c2 <= 1; c2++)
            {
                if (r2 == 0 && c2 == 0)
                    continue;
                
                if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS || (queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)
                    continue;
                
                if (labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != paint_over_label)
                    continue;
                
                /* test criteria to join */
                /* calculate the angle between 2 different poles using dot product
                 a . b = |a||b| cos (angle_in_radians)  - rearranging - >>
                 angle_in_radians = a . b / (|a||b|) */
                
                /* calculate the first distance */
                distance_1 = sqrt(pow(avg_X, 2) + pow(avg_Y, 2) + pow(avg_Z, 2));
                
                temp = ((queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2);
                
                /* calculate the second distance */
                distance_2 = sqrt(pow(Surface_Normal_X[temp], 2)  + pow(Surface_Normal_Y[temp], 2) + pow(Surface_Normal_Z[temp], 2));
                
                /* calculate the dot product */
                dot_product = (avg_X * Surface_Normal_X[temp]) + (avg_Y * Surface_Normal_Y[temp]) + (avg_Z * Surface_Normal_Z[temp]);
                
                /* calculate the angle (in radians) */
                angle_in_radians = acos(dot_product / (distance_1 * distance_2));
                
                /* Set the threshold angle to decide the region to include (here angle is set to 40 degrees */
                if ( angle_in_radians > (40 * PI/180)) /* Radians to degrees */
                {
                    continue;   /* skip those, whose angle exceeds above threshold value */
                }
                
                labels[temp] = new_label;
                if (indices != NULL)
                {
                    indices[*count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
                }
                
                (*count)++;
                
                /* calcualate average values of X, Y, Z */
                sum_X += Surface_Normal_X[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
                sum_Y += Surface_Normal_Y[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
                sum_Z += Surface_Normal_Z[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
                
                avg_X = sum_X / (*count);
                avg_Y = sum_Y / (*count);
                avg_Z = sum_Z / (*count);
                
                queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
                qh = (qh + 1) % MAX_QUEUE;
                if (qh == qt)
                {
                    printf("Max queue size exceeded\n");
                    exit(0);
                }
            }
        }
        qt = (qt + 1) % MAX_QUEUE;
    }
    printf(" avg_X = %lf, avg_Y = %lf, avg_Z = %lf\n", avg_X, avg_Y, avg_Z);
}
