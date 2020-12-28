#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define PNG_DEBUG 3
#include <png.h>

#define RESET_PIN 0
#define DC_PIN 6
#define BUSY_PIN 5

int fd;
int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep *row_pointers;

void abort_(const char *s, ...);
void read_png_file(char *file_name);
void process_file(void);
void DEPG0213RWS800F13_Wait(void);
void DEPG0213RWS800F13_Command(uint8_t cmd);
void DEPG0213RWS800F13_Data(uint8_t data);
void DEPG0213RWS800F13_RedOn(void);
void DEPG0213RWS800F13_RedOff(void);
void DEPG0213RWS800F13_BlackOn(void);
void DEPG0213RWS800F13_BlackOff(void);
void DEPG0213RWS800F13_UpdateAndSleep(void);
void DEPG0213RWS800F13_Init(void);

void abort_(const char *s, ...)
{
      va_list args;
      va_start(args, s);
      vfprintf(stderr, s, args);
      fprintf(stderr, "\n");
      va_end(args);
      abort();
}

void read_png_file(char *file_name)
{
      char header[8]; // 8 is the maximum size that can be checked

      /* open file and test for it being a png */
      FILE *fp = fopen(file_name, "rb");
      if (!fp)
            abort_("[read_png_file] File %s could not be opened for reading", file_name);
      fread(header, 1, 8, fp);
      if (png_sig_cmp(header, 0, 8))
            abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);

      /* initialize stuff */
      png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

      if (!png_ptr)
            abort_("[read_png_file] png_create_read_struct failed");

      info_ptr = png_create_info_struct(png_ptr);
      if (!info_ptr)
            abort_("[read_png_file] png_create_info_struct failed");

      if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during init_io");

      png_init_io(png_ptr, fp);
      png_set_sig_bytes(png_ptr, 8);

      png_read_info(png_ptr, info_ptr);

      width = png_get_image_width(png_ptr, info_ptr);
      height = png_get_image_height(png_ptr, info_ptr);
      color_type = png_get_color_type(png_ptr, info_ptr);
      bit_depth = png_get_bit_depth(png_ptr, info_ptr);

      number_of_passes = png_set_interlace_handling(png_ptr);
      png_read_update_info(png_ptr, info_ptr);

      /* read file */
      if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during read_image");

      row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
      for (y = 0; y < height; y++)
            row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));

      png_read_image(png_ptr, row_pointers);

      fclose(fp);
}

void process_file(void)
{
      uint16_t i;
      uint8_t tmp = 0;
      uint8_t rBuf[4000] = {0x00};
      uint8_t bBuf[4000] = {0x00};

      if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB)
            abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
                   "(lacks the alpha channel)");
      for (y = 0; y < height; y++)
      {
            png_byte *row = row_pointers[y];
            for (x = 0; x < width; x++)
            {
                  png_byte *ptr = &(row[x * 3]);

                  // 提取画图板的红色
                  if (ptr[0] == 237 && ptr[1] == 28 && ptr[2] == 36)
                  {
                        //     printf("Pixel at position [ %d - %d ] has Red Color.\n",x, y);
                        //先要计算这个像素落在哪个byte
                        tmp = rBuf[16 * y + (15 - x / 8)];
                        rBuf[16 * y + (15 - x / 8)] = tmp | (1 << (x % 8));
                  }
                  // 提取画图板上的黑色
                  else if (ptr[0] == 255 && ptr[1] == 255 && ptr[2] == 255)
                  {
                        //     printf("Pixel at position [ %d - %d ] has Black Color.\n",x, y);
                        //先要计算这个像素落在哪个byte
                        tmp = bBuf[16 * y + (15 - x / 8)];
                        bBuf[16 * y + (15 - x / 8)] = tmp | (1 << (x % 8));
                  }
                  ptr[0] = 0;
                  ptr[1] = ptr[2];
            }
      }

      DEPG0213RWS800F13_Init();

      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x26);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(rBuf[i]);
      }
      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x24);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(bBuf[i]);
      }

      DEPG0213RWS800F13_UpdateAndSleep();
}

void DEPG0213RWS800F13_Wait(void)
{
      while (1)
      {
            if (digitalRead(BUSY_PIN) == 0)
                  break;
            usleep(5);
      }
      usleep(100);
}

void DEPG0213RWS800F13_Command(uint8_t cmd)
{
      digitalWrite(DC_PIN, 0);
      uint8_t buffer[1] = {cmd};
      wiringPiSPIDataRW(fd, buffer, 1);
}

void DEPG0213RWS800F13_Data(uint8_t data)
{
      digitalWrite(DC_PIN, 1);
      uint8_t buffer[1] = {data};
      wiringPiSPIDataRW(fd, buffer, 1);
}

void DEPG0213RWS800F13_RedOn(void)
{
      uint16_t i;
      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x26);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(0xFF);
      }
}

void DEPG0213RWS800F13_RedOff(void)
{
      uint16_t i;
      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x26);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(0x00);
      }
}

void DEPG0213RWS800F13_BlackOn(void)
{
      uint16_t i;

      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x24);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(0x00);
      }
}

void DEPG0213RWS800F13_BlackOff(void)
{
      uint16_t i;

      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x24);
      for (i = 0; i < 4000; i++)
      {
            DEPG0213RWS800F13_Data(0xFF);
      }
}

void DEPG0213RWS800F13_UpdateAndSleep(void)
{
      DEPG0213RWS800F13_Command(0x20);
      DEPG0213RWS800F13_Wait();

      DEPG0213RWS800F13_Command(0x10);
      DEPG0213RWS800F13_Data(0x01);
      usleep(100 * 1000);
}

void DEPG0213RWS800F13_Init(void)
{
      usleep(10 * 1000);
      digitalWrite(RESET_PIN, 0);
      usleep(10 * 1000);
      digitalWrite(RESET_PIN, 1);
      usleep(10 * 1000);
      DEPG0213RWS800F13_Wait();
      DEPG0213RWS800F13_Command(0x12);
      DEPG0213RWS800F13_Wait();
}

int main()
{
      wiringPiSetup();
      // DC
      pinMode(DC_PIN, OUTPUT);
      // RST
      pinMode(RESET_PIN, OUTPUT);
      // BUSY
      pinMode(BUSY_PIN, INPUT);

      fd = wiringPiSPISetup(0, 500000);

      read_png_file("new.png");
      process_file();

      /*
      printf("正在准备刷全红色\n");
      DEPG0213RWS800F13_Init();
      DEPG0213RWS800F13_RedOn();
      DEPG0213RWS800F13_BlackOff();
      DEPG0213RWS800F13_UpdateAndSleep();
       sleep(3);

      printf("正在准备刷全黑色\n");
      DEPG0213RWS800F13_Init();
      DEPG0213RWS800F13_RedOff();
      DEPG0213RWS800F13_BlackOn();
      DEPG0213RWS800F13_UpdateAndSleep();
      sleep(3);

      printf("正在准备刷全白色\n");
      DEPG0213RWS800F13_Init();
      DEPG0213RWS800F13_RedOff();
      DEPG0213RWS800F13_BlackOff();
      DEPG0213RWS800F13_UpdateAndSleep();
      sleep(3);
      */

      return 0;
}
