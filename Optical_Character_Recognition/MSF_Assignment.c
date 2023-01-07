#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

unsigned char* copy_img_and_threshold(int row, int col);
int find_end_branch_pts(unsigned char *thinned);


int        ROWS,COLS,rows,cols;
int        i,j,rows1,cols1,x,y;
unsigned char  *orig_img, *copy_image,*copy_img, *img_threshold;

int main()
{
    
    FILE        *fpt, *fp_csv;
    unsigned char *msf_image_normalized,*img_end_branch;
    char        header[320],letter[10],gt_letter[10];
    int BYTES,m;
    int edge_nonedge_pixels,neighbor_pixels;
    bool flag;
    int tp,tn,fp,fn;
    int letter_found = 0;
    
  /* Open and read parenthood template image */
  if ((fpt=fopen("parenthood_e_template.ppm","r")) == NULL)
    {
    printf("Unable to open parenthood_e_template.ppm for reading\n");
    exit(0);
    }
  fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
  if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf("Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

  /* Now Open and Read Parenthood Image */
  if ((fpt=fopen("parenthood.ppm","r")) == NULL)
    {
    printf("Unable to open parenthood.ppm for reading\n");
    exit(0);
    }
  fscanf(fpt,"%s %d %d %d",header,&cols,&rows,&BYTES);
  if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf("Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

  /* Allocate memory where original image is copied */
  orig_img=(unsigned char *)calloc(rows*cols,sizeof(unsigned char));
  header[0]=fgetc(fpt);    /* read white-space character that separates header */
  fread(orig_img,1,cols*rows,fpt);
  fclose(fpt);

  /* Open and read MSF Normalised image */
  if ((fpt=fopen("MSF_Normalised_OP.ppm","r")) == NULL)
    {
    printf("Unable to open MSF_Normalised_OP.ppm for reading\n");
    exit(0);
    }
  fscanf(fpt,"%s %d %d %d",header,&cols,&rows,&BYTES);
  if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf("Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

  msf_image_normalized = (unsigned char *)calloc(rows*cols,sizeof(unsigned char));
  header[0]=fgetc(fpt);
  fread(msf_image_normalized,1,cols*rows,fpt);
  fclose(fpt);
   
  /* Allocate Memory for following parameters */
    
  copy_image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  img_threshold=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  img_end_branch = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));

  /* open and create a csv file to store data */

  fp_csv = fopen("ROC_data.csv", "wb");
  fprintf(fp_csv, "Threshold,TP,FP,TN,FN,TPR,FPR,PPV\n");
 

  for (m = 0; m < 256; m++)
  {
    /* Reading parenthood_gt file */
    fpt = fopen("parenthood_gt.txt","rb");
    tp = fp = fn = tn = 0;
    strcpy(letter, "e");

    /* Going through each letter in text file */
    while((fscanf(fpt, "%s %d %d\n", gt_letter, &cols1, &rows1)) != EOF)
    {
      for (i = rows1-7; i <= (rows1 + 7); i++)
      {
        for (j = cols1-4; j <= (cols1 + 4); j++)
        {
          if (msf_image_normalized[i*cols+j] > m)
          {
            letter_found = 1;
          }
        }
      }
        bool flag2;
        int row, col;
      if (letter_found == 1)
      {
          /* Copy the original image and threshold at 128 */
          copy_image = copy_img_and_threshold(rows1,cols1);
          
          
          
          /* Thin the image by going around the pixel in a letter */
          flag2 = true;
          while (flag2)
          {
            flag2 = false;
            for (row = 0; row < 15; row++)
            {
              for (col = 0; col < 9; col++)
              {

                if ((copy_image[row*9+col] > 0))
                {
                  edge_nonedge_pixels = 0;
                  neighbor_pixels = 0;
                    
                  /* North */
                  if (copy_image[(row*9+col)-9] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    flag = false;
                  }
                    
                  /* North East */
                  if (copy_image[(row*9+col)-9+1] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }
                    
                  /* East */
                  if (copy_image[(row*9+col)+1] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                      flag = false;
                  }
                    
                  /* South East */
                  if (copy_image[(row*9+col)+1+9] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }
                    
                  /* South */
                  if (copy_image[(row*9+col)+9] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }
                    
                  /* South West */
                  if (copy_image[(row*9+col)+9-1] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }
                    
                  /* West */
                  if (copy_image[(row*9+col)-1] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }
                    
                  /* North West */
                  if (copy_image[(row*9+col)-1-9] == 255)
                  {
                    neighbor_pixels++;
                    flag = true;
                  }
                  else
                  {
                    if (flag)
                    {
                      edge_nonedge_pixels++;
                    }
                    flag = false;
                  }

                  if ((copy_image[(row*9+col)-9] == 0) && flag)
                  {
                    edge_nonedge_pixels++;
                  }

                  /* Applyng 3 conditions to mark and remove the pixel */
                    
                  if ((edge_nonedge_pixels == 1) && (2 <= neighbor_pixels && neighbor_pixels <= 6))
                  {
                      if ((copy_image[(row*9+col)-9] == 0) || (copy_image[(row*9+col)+1] == 0) || ((copy_image[(row*9+col)-1] == 0) && (copy_image[(row*9+col)+9] == 0)))
                      {
                        copy_image[row*9+col] = 0;
                        flag2 = true;
                      }
                  }
                }
              }
            }
          }
          
        /* Copy the thresholded data and send to detect branch and end points */
        for (i=0;i<9*15;i++){
          img_end_branch[i] = img_threshold[i];
        }

        letter_found = find_end_branch_pts(img_end_branch);
      }

      if ((letter_found == 1) && (strcmp(gt_letter, letter) == 0))
      {
        tp++;
      }
      if ((letter_found == 1) && (strcmp(gt_letter, letter) != 0))
      {
        fp++;
      }
      if ((letter_found == 0) && (strcmp(gt_letter, letter) == 0))
      {
        fn++;
      }
      if ((letter_found == 0) && (strcmp(gt_letter, letter) != 0))
      {
        tn++;
      }
      letter_found = 0;
    }

    fprintf(fp_csv, "%d,%d,%d,%d,%d,%lf,%lf,%lf\n",m,tp,fp,tn,fn,(double)tp/(double)(tp+fn),(double)fp/(double)(fp+tn),fp/(double)(tp+fp));
  }
  fclose(fpt);
  fclose(fp_csv);

  /* Copied Image */
  fpt=fopen("copied_image.ppm","wb");
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(copy_img,COLS*ROWS,1,fpt);
  fclose(fpt);

  /* After thinning */
  fpt=fopen("after_thinning.ppm","wb");
  fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
  fwrite(copy_image,COLS*ROWS,1,fpt);
  fclose(fpt);
}


unsigned char* copy_img_and_threshold(int row, int col)
{
  copy_img=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
  x = 0;
  for (i = row-7; i <= (row + 7); i++)
  {
    y = 0;
    for (j = col-4; j <= (col + 4); j++)
    {
      copy_img[x*COLS+y] = orig_img[i*cols+j];
      y++;
    }
    
    x++;
  }

  for (i = 0; i < ROWS*COLS; i++)
  {
    if (copy_img[i] < 128)
    {
      img_threshold[i] = 255;
    }
    else
    {
      img_threshold[i] = 0;
    }
  }
    
    for (i = 0; i< ROWS*COLS; i++)
    {
        threshold_image[i] = img_threshold[i];
    }
    
  return img_threshold;
}


int find_end_branch_pts(unsigned char *img_thinned)
{
  int edge_nonedge_pixels,row,col,branch_points=0,end_points=0,letter_found;
  bool flag;
  int i,j;
    
  for (i = 0; i < 15; i++)
    {
      for (j = 0; j < 9; j++)
      {
        /* Check that particular pixel whether it has a value */
        if ((img_thinned[i*9+j] > 0))
        {
            edge_nonedge_pixels = 0;
            
          /* North */
          if (img_thinned[(i*9+j)-9] == 255)
          {
            flag = true;
          }
          else
          {
            flag = false;
          }
            
          /* North East */
          if (img_thinned[(i*9+j)-9+1] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* East */
          if (img_thinned[(i*9+j)+1] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* South East */
          if (img_thinned[(i*9+j)+1+9] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* South */
          if (img_thinned[(i*9+j)+9] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* South West */
          if (img_thinned[(i*9+j)+9-1] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* West */
          if (img_thinned[(i*9+j)-1] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          /* North West */
          if (img_thinned[(i*9+j)-1-9] == 255)
          {
            flag = true;
          }
          else
          {
            if (flag)
            {
                edge_nonedge_pixels++;
            }
            flag = false;
          }
            
          if ((img_thinned[(i*9+j)-9] == 0) && flag)
          {
              edge_nonedge_pixels++;
          }

          if (edge_nonedge_pixels == 1)
          {
            end_points++;
          }
          if (edge_nonedge_pixels > 2)
          {
              branch_points++;
          }
        }
      }
    }
  if (branch_points == 1 && end_points == 1)
  {
    letter_found = 1;
  }
  else
  {
    letter_found = 0;
  }

  return letter_found;
}
