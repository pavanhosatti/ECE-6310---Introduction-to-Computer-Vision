//
//  Assignment 5 - Active_contours.c
//
//  Created by Pavan V Hosatti on 01/11/22.
//

#include "Active_contours.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define SQR(x) ((x)*(x))

void draw_initial_contour_points(int ROWS, int COLS, unsigned char *output_img_1);
unsigned char * sobel_operator(unsigned char * image, int ROWS, int COLS, float *
sobel_output_img, int * sobel_before_normalisation);
void Active_contour(int rows, int cols, int * cols_contours, int * rows_contours,
unsigned char * sobel_output);
float * normalize_energies(float * energy);
void Internal_Energy1(int * cols_contours, int * rows_contours, float *
int_energy1, float * int_energy1_normalized, int k);
void Internal_Energy2(int * cols_contours, int * rows_contours, float *
int_energy2, float * int_energy1, int average_dist, float * int_energy2_normalized,
int k);
void External_Energy(float * ext_energy, unsigned char * sobel_output, float *
ext_energy_normalized, int k,int * cols_contours, int * rows_contours);
void draw_final_contour_points(int * cols_contours, int * rows_contours, int ROWS,
int COLS, unsigned char *output_img_2);
int contour_row_pts[42], contour_col_pts[42];
float * sobel_output_img;
int * sobel_before_normalisation;
int total_contours = 42;
int COLS, ROWS;
int main()
{
    unsigned char * input_img;
    unsigned char * output_img_1;
    unsigned char * output_img_2;
    unsigned char *sobel_after_normalisation;
    FILE * fpt;
    int BYTES,i;
    char header[256];
    
    /* Load the input image */
    fpt = fopen("hawk.ppm", "rb");
    if (fpt == NULL)
    {
        printf("Unable to open hawk.ppm file for reading\n");
        exit(0);
    }
    /* Read the image data abd check for validity */
    fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
    if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
      printf("Input image hawk.ppm is not a greyscale 8-bit PPM image\n");
      exit(0);
    }
    
    /* copy the image data into other variable */
    input_img = calloc(COLS*ROWS,sizeof(unsigned char));
    output_img_1 = calloc(COLS*ROWS,sizeof(unsigned char));
    output_img_2 = calloc(COLS*ROWS,sizeof(unsigned char));
    header[0]=fgetc(fpt);
    fread(input_img,sizeof(unsigned char),ROWS*COLS,fpt);
    fseek(fpt, 0, SEEK_SET);
    
    for (i = 0; i< ROWS*COLS; i++)
    {
        output_img_1[i] = input_img[i];
        output_img_2[i] = input_img[i];
    }
    fclose(fpt);
    
    /* Read the initial contour text file and draw the initial contour points on
the input image */
    draw_initial_contour_points(ROWS, COLS, output_img_1);
    
    /* Apply sobel filter to input image */
    sobel_output_img = calloc(ROWS*COLS, sizeof(float));
    sobel_before_normalisation = calloc(ROWS*COLS, sizeof(float));
    sobel_after_normalisation = sobel_operator(input_img, ROWS, COLS,
sobel_output_img, sobel_before_normalisation);
    
    /* Active Contour Algorithm */
    Active_contour(ROWS, COLS, contour_col_pts, contour_row_pts,
sobel_after_normalisation);
    
    /* Draw final contour points calculated in function Active_contour*/
    draw_final_contour_points(contour_col_pts, contour_row_pts, ROWS, COLS,
output_img_2);
    
    /* Display the contour points changed */
    for (i = 0; i < total_contours; i++)
    {
        printf("%d %d\n", contour_row_pts[i], contour_col_pts[i]);
    }
}
void draw_initial_contour_points(int ROWS, int COLS, unsigned char *output_img_1)
{
    FILE *contour_text;
    FILE *fpt;
    int cols_contour, rows_contour, i, j,k=0;
    
    fpt = fopen("hawk_init.txt", "rb");
    if (fpt == NULL)
    {
        printf("Unable to open hawk_init.txt file for reading\n");
        exit(0);
    }
    
    /* Read the file if successfully opened and draw plus on it (vertical line '|'
and horizontal line '-') */
    while(fscanf(fpt, "%d %d\n", &cols_contour, &rows_contour) != EOF)
    {
        contour_row_pts[k] = rows_contour;
        contour_col_pts[k] = cols_contour;
        k++;
        
        /* 7x7 area */
        for (i = -3; i<=3; i++ )
        {
            output_img_1[(rows_contour+i)*COLS+cols_contour] = 0;
            output_img_1[(rows_contour)*COLS+(cols_contour+i)] = 0;
        }
    }
    
    /* storing the first output after drawing initial contour points */
    fpt = fopen("Output_1.ppm", "wb");
    if (fpt == NULL)
    {
        printf("failed to create the OP_img file\n");
        exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(output_img_1,COLS*ROWS,1,fpt);
    fclose(fpt);
}
void draw_final_contour_points(int * cols_contours, int * rows_contours, int ROWS,
int COLS, unsigned char *output_img_2)
{
    FILE *contour_text;
    FILE *fpt;
    int cols_contour, rows_contour, i, j,k=0;
    
    /* Read the file if successfully opened and draw plus on it (vertical line '|'
and horizontal line '-') */
    for (i = 0; i < 42; i++)
    {
        
        /* 7x7 area */
        for (j = -3; j<=3; j++ )
        {
            output_img_2[(rows_contours[i]+j)*COLS+cols_contours[i]] = 0;
            output_img_2[(rows_contours[i])*COLS+(cols_contours[i]+j)] = 0;
        }
    }
    
    /* storing the first output after drawing initial contour points */
    fpt = fopen("Final_output.ppm", "wb");
    if (fpt == NULL)
    {
        printf("failed to create the Final_output file\n");
        exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(output_img_2,COLS*ROWS,1,fpt);
    fclose(fpt);
}
unsigned char * sobel_operator(unsigned char * image, int ROWS, int COLS, float *
sobel_output_img, int * sobel_before_normalisation)
{
    
    int i,j,r,c,k;
    FILE *fpt;
    int Gx = 0;
    int Gy = 0;
    int min = 0;
    int max = 500;
    unsigned char * sobel_after_normalisation;
    sobel_after_normalisation = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned
char));
    
    /* Kernels for sobel operation */
    int x_kernel[9] = {-1,0,1,-2,0,2,-1,0,1};
    int y_kernel[9] = {-1,-2,-1,0,0,0,1,2,1};
    
    /* make duplicate of input image */
    for (i = 0; i < ROWS * COLS; i++)
    {
        sobel_output_img[i] = image[i];
        sobel_before_normalisation[i] = image[i];
    }
    
    /* loop through each pixel of image and convolve with the kernels */
    for (r = 1; r < ROWS-1; r++)
    {
        for (c = 1; c < COLS-1; c++)
        {
            Gx=Gy=0;
            for (i = -1; i<= 1; i++)
            {
                for (j = -1; j<=1; j++)
                {
                    Gx += image[(r+i)*COLS+(c+j)] * x_kernel[(i+1)*3+(j+1)];
                    Gy += image[(r+i)*COLS+(c+j)] * y_kernel[(i+1)*3+(j+1)];
                }
            }
            sobel_output_img[r*COLS+c] = sqrt(pow(Gx,2) + pow(Gy,2));
            sobel_before_normalisation[r*COLS+c] = sqrt(pow(Gx,2) + pow(Gy,2));
            if(sobel_before_normalisation[r*COLS+c] < min )
                min = sobel_before_normalisation[r*COLS+c];
            if(sobel_before_normalisation[r*COLS+c] > max )
                max = sobel_before_normalisation[r*COLS+c];
        }
    }
    
    if ((int)(max - min) == 0)
    {
        printf("unable to calculate further\n");
        exit(0);
    }
    
    /* Normalise the output of convolution to range 0 - 255 */
    for (r = 0; r < ROWS; r++)
    {
        for (c = 0; c < COLS; c++)
        {
                sobel_after_normalisation[r*COLS+c] =
((sobel_before_normalisation[r*COLS+c] - min)*(255 - 0)/(max-min)) + 0;
        }
    }
    /* storing the first output after drawing initial contour points */
    fpt = fopen("sobel_output.ppm", "wb");
    if (fpt == NULL)
    {
        printf("failed to create the OP_img file\n");
        exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(sobel_after_normalisation,COLS*ROWS,1,fpt);
    fclose(fpt);
    
    return sobel_after_normalisation;
}
void Active_contour(int rows, int cols, int * cols_contours, int * rows_contours,
unsigned char * sobel_output)
{
    int i, j, k, row, col, m,n;
    float avg_dist;
    int dist1, dist2;
    float * int_energy1;
    float * int_energy1_normalized;
    float * int_energy2;
    float * int_energy2_normalized;
    float * ext_energy;
    float * ext_energy_normalized;
    float * Total_Energy;
    int min, min_row = 0, min_col = 0;
    int *new_row_contour;
    int *new_col_contour;
    
    new_row_contour = (int *)calloc(total_contours, sizeof(int));
    new_col_contour = (int *)calloc(total_contours, sizeof(int));

    /* Apply for 30 iterations */
    for (i = 0; i < 31; i++)
    {
        avg_dist = 0;
        
        /* sqrt(sqr(x2-x1) + sqr(y2-y1)) */
        for (j = 0; j < total_contours-1; j++ )
        {
            dist1 = (rows_contours[j] - rows_contours[j+1]);
            dist2 = (cols_contours[j] - cols_contours[j+1]);
            avg_dist += sqrt((dist1*dist1) + (dist2*dist2));
        }
        /* calculating the distance between first and last contour point */
        dist1 = (rows_contours[j] - rows_contours[0]);
        dist2 = (cols_contours[j] - cols_contours[0]);
        avg_dist += sqrt((dist1*dist1) + (dist2*dist2));
        avg_dist = avg_dist/total_contours;
        //printf("avg -> %f\n", avg_dist);
        
        int_energy1 = (float *)calloc(7*7, sizeof(float));
        int_energy1_normalized = (float *)calloc(7*7, sizeof(float));
        
        int_energy2 = (float *)calloc(7*7, sizeof(float));
        int_energy2_normalized = (float *)calloc(7*7, sizeof(float));
        
        ext_energy = (float *)calloc(7*7, sizeof(float));
        ext_energy_normalized = (float *)calloc(7*7, sizeof(float));
        
        Total_Energy = (float *)calloc(7*7, sizeof(float));
        
        for (k = 0; k < total_contours; k++)
        {
            
            /* Calculate Internal Energy 1 */
            Internal_Energy1(cols_contours, rows_contours, int_energy1,int_energy1_normalized, k);
            
            /* Calculate Internal Energy 2 */
            Internal_Energy2(cols_contours, rows_contours, int_energy2,int_energy1, avg_dist, int_energy2_normalized, k);
            
            /* Calculate external energy */
            External_Energy(ext_energy, sobel_output, ext_energy_normalized, k,cols_contours, rows_contours);
            
            /* Calculate total energy as sum of all energies */
            
            for(m = 0; m < 7*7; m++)
            {
                Total_Energy[m] = 2*int_energy2_normalized[m] + int_energy2_normalized[m] + ext_energy[m];
            }
            
            for(m = 0; m < 7*7; m++)
            {
                if (Total_Energy[m] < 20000)
                {
                    min = Total_Energy[m];
                    min_row = (m / 7) - 3;
                    min_col = (m % 7) - 3;
                }
            }
            
            new_row_contour[k] = rows_contours[k] + min_row;
            new_col_contour[k] = cols_contours[k] + min_col;
        }
        int z = 0;
        /* find min row and col in the total energies */
        for (n = 0; n < total_contours; n++)
        {
 
            rows_contours[n] = new_row_contour[n];
            cols_contours[n] = new_col_contour[n];
            /*printf("%d iteration\n", z);
            z++;
            printf("new_row -> %d\n", rows_contours[n]);
            printf("new_col -> %d\n", cols_contours[n]);*/
        }
    }
    
}
void Internal_Energy1(int * cols_contours, int * rows_contours, float *
int_energy1, float * int_energy1_normalized, int k)
{
    int i, row, col,j;
    int dist1, dist2;
    
        j  = 0;
        /* calculate internal energy 1 */
        for (row = -7/2; row < 7/2; row++)
        {
            for (col = -7/2; col < 7/2; col++)
            {
                if (k != total_contours-1)
                {
                    dist1 = ((row + rows_contours[k]) - rows_contours[0]);
                    dist2 = ((col + cols_contours[k]) - cols_contours[0]);
                    int_energy1[(row+3)*7+(col+3)] = sqrt( SQR(dist1) + SQR(dist2) );
                }
                else
                {
                    dist1 = ((row + rows_contours[k]) - rows_contours[k+1]);
                    dist2 = ((col + cols_contours[k]) - cols_contours[k+1]);
                    int_energy1[(row+3)*7+(col+3)] = sqrt( SQR(dist1) + SQR(dist2) );
                }
            }
        }

        /* Normalise the output */
    int_energy1_normalized = normalize_energies(int_energy1);
}
void Internal_Energy2(int * cols_contours, int * rows_contours, float *
int_energy2, float * int_energy1, int average_dist, float * int_energy2_normalized,
int k)
{
    int i, row, col,j;
    int dist1, dist2;
    
        /* calculate internal energy 1 */
        for (row = -7/2; row < 7/2; row++)
        {
            for (col = -7/2; col < 7/2; col++)
            {
                if (k != total_contours-1)
                {
                    dist1 = ((row + rows_contours[k]) - rows_contours[k+1]);
                    dist2 = ((col + cols_contours[k]) - cols_contours[k+1]);
                    int_energy2[(row+3)*7+(col+3)] = pow((average_dist - sqrt(int_energy1[(row+3)*7+(col+3)])), 2);
                    
                }
                else
                {
                    dist1 = ((row + rows_contours[k]) - rows_contours[0]);
                    dist2 = ((col + cols_contours[k]) - cols_contours[0]);
                    int_energy2[(row+3)*7+(col+3)] = pow((average_dist - sqrt(int_energy1[(row+3)*7+(col+3)])),2);
                }
            }
        }

        /* Normalise the output */
        int_energy2_normalized = normalize_energies(int_energy2);
    
}
void External_Energy(float * ext_energy, unsigned char * sobel_output, float *
ext_energy_normalized, int k,int * cols_contours, int * rows_contours)
{
    int i, row, col,j;
    int dist1, dist2;
    
    /* Invert the sobel output */
    for (i = 0; i < ROWS * COLS; i++)
    {
        sobel_output[i] = 1 - sobel_output[i];
    }
    
        /* calculate external energy */
    for (row = -7/2; row < 7/2; row++)
    {
        for (col = -7/2; col < 7/2; col++)
        {
            ext_energy[(row+3)*7+(col+3)] = pow(sobel_output[(rows_contours[k]+row)*7+(cols_contours[k]+col)], 2);
        }
    }
    
        /* Normalise the output */
        ext_energy_normalized = normalize_energies(ext_energy);
}

float *  normalize_energies(float * energy)
{
    int r, c;
    int min = energy[0];
    int max = energy[0];
    float * energy_normalized;
    
    energy_normalized = (float *)calloc(ROWS*COLS, sizeof(float));
    for (r = 0; r < ROWS; r++)
    {
        for (c = 0; c < COLS; c++)
        {
            /* calculating min and max in the image so that it can be normalised to 0 - 255 later */
            if(energy[r*COLS+c] < min )
                min = energy[r*COLS+c];
            if(energy[r*COLS+c] > max )
                max = energy[r*COLS+c];
        }
    }
    
    /* If max and min are same, then return */
    if ((int)(max - min) == 0)
    {
    printf("unable to calculate further\n");
    exit(0);
    }
    /* Normalise the energies to change the range to new range (0-1) */
    for (r = 0; r < ROWS; r++)
    {
        for (c = 0; c < COLS; c++)
        {
            energy_normalized[r*COLS+c] = (energy[r*COLS+c] - min)*(1)/(max-min) +
0;
        }
    }
    return energy_normalized;
}
