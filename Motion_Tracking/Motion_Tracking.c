//
//  Motion_Tracking.c
//
//
//  Created by Pavan V Hosatti on 11/11/22.
//

#include "Motion_Tracking.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define WINDOW_SIZE 0.3
#define ACCX_THRESHOLD 0.002
#define PITCH_THRESHOLD 0.0025
#define ROLL_THRESHOLD 0.02
#define YAW_THRESHOLD 0.0025

void calc_variance (int data_count, double *var_accX, double *var_accY, double *var_accZ, double *var_pitch, double *var_roll, double *var_yaw );
double gyro_sampling(int data_count, double * data, int start, int end);
double accelerometer_sampling(int data_count, double *data, int start, int end);

double time_data[5000], accX[5000], accY[5000], accZ[5000], pitch[5000], roll[5000], yaw[5000];
double time_stamps;

int main()
{
    double t, ax, ay, az, p, r, y;
    FILE *fpt;
    FILE *fpt2;
    int data_count = 0;
    char str1[10], str2[10], str3[10],str4[10],str5[10],str6[10], str7[10];
    double var_time[1000] = {0}, var_accX[1000] = {0}, var_accY[1000] = {0}, var_accZ[1000] = {0}, var_pitch[1000] = {0}, var_roll[1000] = {0}, var_yaw[1000] = {0};
    int i;
    double dist[7];
    
    /* Read the data from acc_gyro.txt */
    if ((fpt=fopen("acc_gyro.txt","rb")) == NULL)
      {
      printf("Unable to open acc_gyro.txt for reading\n");
      exit(0);
      }
    /* Reading out the first row of titles */
    fscanf(fpt,"%s %s %s %s %s %s %s\n", str1, str2, str3, str4, str5, str6, str7);
    
    /* Read all the data from file acc_gyro.txt */
    while ((fscanf(fpt, "%lf %lf %lf %lf %lf %lf %lf\n", &t, &ax, &ay, &az, &p, &r, &y)) != EOF)
        {
            time_data[data_count] = t;
            accX[data_count] = ax;
            accY[data_count] = ay;
            accZ[data_count] = az;
            pitch[data_count] = p;
            roll[data_count] = r;
            yaw[data_count] = y;
            data_count++;

        }
    fclose(fpt);
    
    /* Calculate variance using the data provided */
    calc_variance (data_count, var_accX, var_accY, var_accZ, var_pitch, var_roll, var_yaw);
    
    /* store the calculated variance data */
    fpt2 = fopen("variance.csv", "w");
    fprintf(fpt2, "ACCX, ACCY, ACCZ, PITCH, ROLL, YAW\n");
    for (i = 0; i < (data_count/time_stamps); i++)
    {
        fprintf(fpt2, "%lf, %lf, %lf, %lf, %lf, %lf\n", var_accX[i], var_accY[i], var_accZ[i], var_pitch[i], var_roll[i], var_yaw[i]);
    }
    
    /* Adding 1 to z data to stabilize */
    for (i = 0; i < data_count; i++ )
    {
        accZ[i] = accZ[i]+ 1;
    }
    
    int count = 0;
    int start = 0;
    int end = time_stamps;
    
    /*Store the variations in csv*/
    fpt2 = fopen("distance.csv", "w");
    fprintf(fpt2, "start(time), end(time), dist1(accX), dist2(accY), dist3(accZ), dist4(pitch), dist5(roll), dist6(yaw)\n");

    /* compare variances with the threshold */
    for (i = 0; i < (data_count/time_stamps); i++)
    {
        if (var_accX[i] > ACCX_THRESHOLD || var_accY[i] >  ACCX_THRESHOLD || var_accZ[i] >  ACCX_THRESHOLD || var_pitch[i] > PITCH_THRESHOLD || var_roll[i] > ROLL_THRESHOLD || var_yaw[i] > YAW_THRESHOLD)
            {
                /* Multiply with sampling time with gyroscope data */
                dist[0] = accelerometer_sampling(data_count, accX, start, end);
                dist[1] = accelerometer_sampling(data_count, accY, start, end);
                dist[2] = accelerometer_sampling(data_count, accZ, start, end);
                dist[3] = gyro_sampling(data_count, pitch, start, end);
                dist[4] = gyro_sampling(data_count, roll, start, end);
                dist[5] = gyro_sampling(data_count, yaw, start, end);
                fprintf(fpt2, "%lf, %lf, %lf, %lf, %lf, %lf, %lf , %lf\n", time_data[start], time_data[end], dist[0], dist[1], dist[2], dist[3], dist[4], dist[5]);
            }
            else
            {
                fprintf(fpt2, "%lf, %lf, 0,0,0,0,0,0\n", time_data[start], time_data[end] );
            }
            start = end+1;
            end = end + time_stamps;
    }
    fclose(fpt2);
    
    return 0;
}

double gyro_sampling(int data_count, double * data, int start, int end)
{
    int i, j = 0;
    int count = 0;
    double distance = 0;
    for(i = start; i < end; i++)
    {
            distance += data[i] * 0.05;
    }
    
    return distance;
}

double accelerometer_sampling(int data_count, double *data, int start, int end)
{
    int i;
    double distance = 0;
    double prev_velocity = 0;
    double velocity = 0;

    for (i = start; i < end; i++)
    {
        prev_velocity = velocity;
        velocity += data[i] * 0.05 * 9.8;
        distance += ((velocity + prev_velocity) * 0.05);
    }
    
    return distance;
}

void calc_variance (int data_count, double *var_accX, double *var_accY, double *var_accZ, double *var_pitch, double *var_roll, double *var_yaw )
{
    int i, j;
    double mean_accX[1000] = {0};
    double mean_accY[1000] = {0};
    double mean_accZ[1000] = {0};
    double mean_pitch[1000] = {0};
    double mean_roll[1000] = {0};
    double mean_yaw[1000] = {0};

    int window_stamps_count = 0;
    int total_windows = 0;
    
    time_stamps = WINDOW_SIZE/0.05;
    
    /* Calculate mean of each parameters to calculate variance */
    for (i = 0; i < data_count; i++)
    {
        if (window_stamps_count < time_stamps)
        {
            mean_accX[total_windows] += accX[i];
            mean_accY[total_windows] += accY[i];
            mean_accZ[total_windows] += accZ[i];
            mean_pitch[total_windows] += pitch[i];
            mean_roll[total_windows] += roll[i];
            mean_yaw[total_windows] += yaw[i];
            window_stamps_count++;
        }
        else
        {
            window_stamps_count = 0;
            total_windows++;
            i--;
        }
    }
    
    
    /* Averaging the calculated sum */
    for (i = 0; i < total_windows; i++)
    {
        mean_accX[i] = mean_accX[i]/time_stamps;
        mean_accY[i] = mean_accY[i]/time_stamps;
        mean_accZ[i] = mean_accZ[i]/time_stamps;
        mean_pitch[i] = mean_pitch[i]/time_stamps;
        mean_roll[i] = mean_roll[i]/time_stamps;
        mean_yaw[i] = mean_yaw[i]/time_stamps;
    }
    
    /* calculate variance using mean calculated above */
    j = 0;
    int count = 0;
    for (i = 0; i < data_count; i++)
    {
        if (count < time_stamps)
        {
            var_accX[j] += pow((accX[i] - mean_accX[j]), 2);
            var_accY[j] += pow((accY[i] - mean_accY[j]), 2);
            var_accZ[j] += pow((accZ[i] - mean_accZ[j]), 2);
            var_pitch[j] += pow((pitch[i] - mean_pitch[j]), 2);
            var_roll[j] += pow((roll[i] - mean_roll[j]), 2);
            var_yaw[j] += pow((yaw[i] - mean_yaw[j]), 2);
            count++;
        }
        else
        {
            count = 0;
            j++;
            i--;
        }
    }

    /* Final calculation (divide total by N-1 */
    for (i = 0; i < total_windows; i++)
    {
        var_accX[i] = (var_accX[i])/(time_stamps-1);
        var_accY[i] = (var_accY[i])/(time_stamps-1);
        var_accZ[i] = (var_accZ[i])/(time_stamps-1);
        var_pitch[i] = (var_pitch[i])/(time_stamps-1);
        var_roll[i] = (var_roll[i])/(time_stamps-1);
        var_yaw[i] = (var_yaw[i])/(time_stamps-1);
    }

}
