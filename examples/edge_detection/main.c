#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define E1 1
#define E3 1
#define E4 -4
#define E5 1
#define E7 1

#define G0 1
#define G1 2
#define G2 1
#define G3 2
#define G4 4
#define G5 2
#define G6 1
#define G7 2
#define G8 1

int minH1;
int minV1;
int maxH1;
int maxV1;
int dimh;
int dimv;

void gaussianBlur(unsigned char* original1,unsigned char* result, unsigned int dimh, unsigned int h, unsigned int v){

  int sum=0;

  int A0=0;
  int A1=0;
  int A2=0;
  int A3=0;
  int A4=0;
  int A5=0;
  int A6=0;
  int A7=0;
  int A8=0;

  A0=original1[(v-1)*dimh+h-1];
  A1=original1[(v-1)*dimh+h];
  A2=original1[(v-1)*dimh+h+1];
  A3=original1[(v)*dimh+h-1];     
  A4=original1[(v)*dimh+h];     
  A5=original1[(v)*dimh+h+1];
  A6=original1[(v+1)*dimh+h-1];               
  A7=original1[(v+1)*dimh+h];
  A8=original1[(v+1)*dimh+h+1];           

  sum=A0*G0+A1*G1+A2*G2+A3*G3+A4*G4+A5*G5+A6*G6+A7*G7+A8*G8;
  sum=sum>>4;

  result[v*dimh+h]=sum;

}

void grayScale(unsigned char* original1, unsigned char* result, unsigned int dimh, unsigned int h, unsigned int v){

   unsigned char res;
   res = (original1[3*(v*dimh+h)-1] + original1[3*(v*dimh+h)] + original1[3*(v*dimh+h)+1])  / 3;
   result[v*dimh+h] = res;
}

void edgeLaplace(unsigned char* original1, unsigned char* result, unsigned int dimh, unsigned int h, unsigned int v){

  int sum=0;

  int A1=0;
  int A3=0;
  int A4=0;
  int A5=0;
  int A7=0;

  A1=original1[(v-1)*dimh+h];
  A3=original1[(v)*dimh+h-1];     
  A4=original1[(v)*dimh+h];     
  A5=original1[(v)*dimh+h+1];         
  A7=original1[(v+1)*dimh+h];     

  sum=A1*E1+A3*E3+A4*E4+A5*E5+A7*E7;


  if(sum>0)
  {
    result[v*dimh+h]=sum;
  }
  else
  {
    result[v*dimh+h]=-sum;
  }

}

void threshold(unsigned char* original1, unsigned char* result, unsigned int dimh, unsigned int h, unsigned int v, int thresh){

  int a = 0;
  if(original1[v*dimh+h]<thresh){
    result[v*dimh+h]=0; 
  }
  else{
    result[v*dimh+h]=255; 
  }
}

void loadImage(char *filename, unsigned char **dest, int *width, int *height)
{

  char buffer[200];
  FILE *file = fopen(filename, "r");

  if (file == NULL)
  {
    printf("Error opening file %s.", filename);
    exit(1);
  }
  fgets(buffer, 200, file); // Type

  fscanf(file, "%d %d\n", width, height);
  fgets(buffer, 200, file); // Max intensity

  *dest = (unsigned char*) malloc((*width) * (*height) * sizeof(unsigned char) * 3);

  int pixels_read = 0;
  int pixel = 0;
  int curr_component = 0;

  int i;
  for(i = 0; i < ((*width) * (*height) * 3); i++) {
    int v;
    int got = fscanf(file, "%d", &v);
    if (got == 0)
    {
      printf("Unexpected end of file after reading %d color values", i);
      exit(1);
    }
    (*dest)[i] = (unsigned char)v;
  }
  fclose(file);
}

void writeImage(char *filename, unsigned char  *data, int width, int height)
{
  FILE *file = fopen(filename, "w");

  fprintf(file, "P3\n");
  fprintf(file, "%d %d\n", width, height);
  fprintf(file, "255\n");

  int i; 
  for(i = 0; i < width * height; i++)
  {
    data[i] = data[i] > 255 ? 255 : data[i];
    data[i] = data[i] < 0 ? 0 : data[i];

    int j; for (j = 0; j < 3; j++)
      fprintf(file, "%d\n",(int)data[i]);
    
  }
  fclose(file);
}

int main(int argc, char *argv[]){

  unsigned char* inImage;
  loadImage("lena.ppm", &inImage, &dimv, &dimh);

  unsigned int pixel = dimv * dimh;
  unsigned char  *outImage= (unsigned char *) malloc(pixel * sizeof(unsigned char )*3);
  
  unsigned char  *scaled  = (unsigned char *) malloc(pixel * sizeof(unsigned char ));
  unsigned char  *gaused  = (unsigned char *) malloc(pixel * sizeof(unsigned char ));
  unsigned char  *edged   = (unsigned char *) malloc(pixel * sizeof(unsigned char ));
  unsigned char  *test    = (unsigned char *) malloc(pixel * sizeof(unsigned char ));

  minH1 = 1;
  minV1 = 1;
  maxH1 = dimh - 1;
  maxV1 = dimv - 1;

  unsigned int h,v;
  for(v=minV1;v<maxV1;v++)
    for(h=minH1;h<maxH1;h++)
       grayScale(inImage, scaled, dimh, h, v);

  for(v=minV1;v<maxV1;v++)
    for(h=minH1;h<maxH1;h++)
       gaussianBlur(scaled, gaused, dimh, h, v);

  for(v=minV1;v<maxV1;v++)
    for(h=minH1;h<maxH1;h++)
       edgeLaplace(gaused, edged, dimh, h, v);

  for(v=minV1;v<maxV1;v++)
    for(h=minH1;h<maxH1;h++)
       threshold(edged, outImage, dimh, h, v, 10);
  
  writeImage("edge.ppm", outImage, dimv, dimh);

  return 0;
}

