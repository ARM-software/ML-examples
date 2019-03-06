#include "mbed.h"
#include "stm32746g_discovery_camera.h"
#include "LCD_DISCO_F746NG.h"
#include "stm32f7xx_hal_dcmi.h"

extern "C" {
  //defined in stm32746g_discovery_camera.c
  extern DCMI_HandleTypeDef hDcmiHandler;
  void DCMI_IRQHandler(void) {
      HAL_DCMI_IRQHandler(&hDcmiHandler);
  }
  void DMA2_Stream1_IRQHandler(void) {
      HAL_DMA_IRQHandler(hDcmiHandler.DMA_Handle);
  }
}

// 2 input channels, as input is in RGB565 format
#define NUM_IN_CH 2
#define NUM_OUT_CH 3
#define IMG_WIDTH 160
#define IMG_HEIGHT 120
#define CNN_IMG_SIZE 32
#define resolution RESOLUTION_R160x120

uint8_t camera_buffer[NUM_IN_CH*IMG_WIDTH*IMG_HEIGHT];
uint8_t resized_buffer[NUM_OUT_CH*CNN_IMG_SIZE*CNN_IMG_SIZE];
char lcd_output_string[50];
LCD_DISCO_F746NG lcd;
Serial pc(USBTX, USBRX);

void resize_rgb565in_rgb888out(uint8_t* camera_image, uint8_t* resize_image)
{
  // offset so that only the center part of rectangular image is selected for resizing
  int width_offset = ((IMG_WIDTH-IMG_HEIGHT)/2)*NUM_IN_CH;

  int yresize_ratio = (IMG_HEIGHT/CNN_IMG_SIZE)*NUM_IN_CH;
  int xresize_ratio = (IMG_WIDTH/CNN_IMG_SIZE)*NUM_IN_CH;
  int resize_ratio = (xresize_ratio<yresize_ratio)?xresize_ratio:yresize_ratio;

  for(int y=0; y<CNN_IMG_SIZE; y++) {
    for(int x=0; x<CNN_IMG_SIZE; x++) {
      int orig_img_loc = (y*IMG_WIDTH*resize_ratio + x*resize_ratio + width_offset);
      // correcting the image inversion here
      int out_img_loc = ((CNN_IMG_SIZE-1-y)*CNN_IMG_SIZE + (CNN_IMG_SIZE-1-x))*NUM_OUT_CH;
      uint8_t pix_lo = camera_image[orig_img_loc];
      uint8_t pix_hi = camera_image[orig_img_loc+1];
      // convert RGB565 to RGB888
      resize_image[out_img_loc] = (0xF8 & pix_hi); 
      resize_image[out_img_loc+1] = ((0x07 & pix_hi)<<5) | ((0xE0 & pix_lo)>>3);
      resize_image[out_img_loc+2] = (0x1F & pix_lo) << 3;
    }
  }
}

void display_image_rgb888(int x_dim, int y_dim, uint8_t* image_data)
{
  for(int y=0; y<y_dim; y++) {
    for(int x=0; x<x_dim; x++) {
      int pix_loc = (y*x_dim + x)*3;
      uint8_t a = 0xFF;
      uint8_t r = image_data[pix_loc];
      uint8_t g = image_data[pix_loc+1];
      uint8_t b = image_data[pix_loc+2];
      int pixel = a<<24 | r<<16 | g<<8 | b;
      lcd.DrawPixel(300+x, 100+y, pixel);
    }
  }
}
 
void display_image_rgb565(int x_dim, int y_dim, uint8_t* image_data)
{
  for(int y=0; y<y_dim; y++) {
    for(int x=0; x<x_dim; x++) {
      int pix_loc = (y*x_dim + x)*2;
      uint8_t a = 0xFF;
      uint8_t pix_lo = image_data[pix_loc];
      uint8_t pix_hi = image_data[pix_loc+1];
      uint8_t r = (0xF8 & pix_hi);
      uint8_t g = ((0x07 & pix_hi)<<5) | ((0xE0 & pix_lo)>>3);
      uint8_t b = (0x1F & pix_lo) << 3;
      int pixel = a<<24 | r<<16 | g<<8 | b;
      // inverted image, so draw from bottom-right to top-left
      lcd.DrawPixel(200-x, 160-y, pixel);
    }
  }
}
 
int main()
{
  pc.baud(115200);
  lcd.Clear(LCD_COLOR_WHITE);
  HAL_Init();
  wait_ms(100);
  if( BSP_CAMERA_Init(resolution) == CAMERA_OK ) {
      pc.printf("Camera init - SUCCESS\r\n");
  } else {
      pc.printf("Camera init - FAILED\r\n");
      lcd.Clear(LCD_COLOR_RED);
  }
  wait_ms(100);


  while(1) {
    BSP_CAMERA_SnapshotStart(camera_buffer);
    resize_rgb565in_rgb888out(camera_buffer, resized_buffer);
    display_image_rgb888(CNN_IMG_SIZE, CNN_IMG_SIZE, resized_buffer);
    display_image_rgb565(IMG_WIDTH, IMG_HEIGHT, camera_buffer);
    // run neural network 
    sprintf(lcd_output_string,"Original vs. scaled images");
    lcd.DisplayStringAt(0, LINE(8), (uint8_t *)lcd_output_string, CENTER_MODE);
  }
}


