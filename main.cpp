#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"
#include <stdio.h>

#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "rtc.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#include "util.h"

#include "button_debounce.h"

#include "Adafruit_NeoPixel.hpp"

//////////MENU
int sd_skylander_count = 0;
int selected_skylander = -1; // If this is -1, do not read a skylander
int selected_slot = 0;
//////////ENDOFMENU
//////////PORTAL
#define MSG_SIZE 32
#define BLOCK_SIZE 16
#define MAX_SKYLANDER_COUNT 4
FIL *loaded_skylanders[MAX_SKYLANDER_COUNT] = {0};
//bool shutthefuckup = false;
//bool send_data_now = false;
char sense_counter = 0;
//////////END OF PORTAL

//////////BUTTON STUFF
enum { butt_select, butt_right, butt_left, butt_slot_right, butt_slot_left, butt_start, num_buttons };
const short butt_gpios[num_buttons] = {21, 20, 19, 15, 14, 18};
struct debounce_state_t db_state[num_buttons];
//////////END BUTTON

//////////LED STUFF
Adafruit_NeoPixel neo(2, 6, NEO_GBR|NEO_KHZ800);
//////////END OF LED
//////////LCD STUFF
// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER 1
#define LCD_COMMAND 0

#define MAX_LINES 2
#define MAX_CHARS 16

int SkylanderEndsWith(const char *, const char *);

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val)
{
#ifdef i2c_default
  i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val)
{
  // Toggle enable pin on LCD display
  // We cannot do this too quickly or things don't work
#define DELAY_US 600
  sleep_us(DELAY_US);
  i2c_write_byte(val | LCD_ENABLE_BIT);
  sleep_us(DELAY_US);
  i2c_write_byte(val & ~LCD_ENABLE_BIT);
  sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode)
{
  uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
  uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

  i2c_write_byte(high);
  lcd_toggle_enable(high);
  i2c_write_byte(low);
  lcd_toggle_enable(low);
}

void lcd_clear(void)
{
  lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position)
{
  int val = (line == 0) ? 0x80 + position : 0xC0 + position;
  lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val)
{
  lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s)
{
  while (*s)
  {
    lcd_char(*s++);
  }
}

void lcd_init()
{
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x02, LCD_COMMAND);

  lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
  lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
  lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
  lcd_clear();
}
/////////END OF LCD STUFF

void hid_task(void);

size_t f_listfiles(char *names[], size_t max)
{
  FRESULT res;
  DIR dir;
  FILINFO fno;
  size_t count = 0;
  f_opendir(&dir, "/"); // Open Root
  do
  {
    f_readdir(&dir, &fno);
    if (fno.fname[0] != 0)
    {

      if (SkylanderEndsWith(fno.fname, "bin") == 1 || SkylanderEndsWith(fno.fname, "sky") == 1 || SkylanderEndsWith(fno.fname, "dmp") == 1 || SkylanderEndsWith(fno.fname, "dump") == 1)
      {
        printf("String to alloc: %s | String length = %d\n", fno.fname, strlen(fno.fname) + 1);
        printf("Writing to FileNames[%d]\n", count);
        names[count] = (char *)malloc(strlen(fno.fname) + 1);
        strcpy(names[count], fno.fname);
        count++;
      }
    }
  } while (fno.fname[0] != 0);

  f_closedir(&dir);
  return count;
}

void lcd_draw_2line(const char *line1, const char *line2)
{
  lcd_clear();
  lcd_set_cursor(0, (MAX_CHARS / 2) - strlen(line1) / 2);
  lcd_string(line1);
  lcd_set_cursor(1, (MAX_CHARS / 2) - strlen(line2) / 2);
  lcd_string(line2);
}

void lcd_draw_status()
{
    lcd_clear();
    lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("Skylander Portal") / 2);
    lcd_string("Skylander Portal");
    lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("    Emulator    ") / 2);
    lcd_string("    Emulator    ");
    sleep_ms(500);
}

int main()
{
  // INIT SERIAL DEBUG
  stdio_init_all();
  printf("-------------------------------------\n");
  printf(" KAOS - a Raspbery Pi Pico Skylander \n");
  printf("      Portal of Power Emulator       \n");
  printf("-------------------------------------\n");
  printf("    Made by NicoAICP and redcubie    \n");
  printf("-------------------------------------\n");

  // LED init
  neo.begin();
  neo.clear();
  neo.show();

  // INIT GPIO for buttons
  for (int b = 0; b < sizeof(butt_gpios)/sizeof(butt_gpios[0]); b ++) {
    gpio_init(butt_gpios[b]);
    gpio_set_dir(butt_gpios[b], GPIO_IN);
    gpio_pull_up(butt_gpios[b]);
    debounce_init(&db_state[b], butt_gpios[b]);
  }

  // INIT i2c FOR LCD
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  gpio_pull_up(4);
  gpio_pull_up(5);
  // Make the I2C pins available to picotool
  bi_decl(bi_2pins_with_func(4, 5, GPIO_FUNC_I2C));

  lcd_init();
  printf("LCD INIT done\n");
  //sleep_ms(200);

  // SEND MESSAGE TO LCD
  lcd_draw_2line("Starting", "Initialization");
  sleep_ms(500);

 
  printf("SD Init\n");
  // SDCARD INIT
  sd_card_t *pSD = sd_get_by_num(0);
  FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
  if (FR_OK != fr)
  {
    printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);

    lcd_draw_2line("Cannot Mount", "SDCar");

    while (true)
      ;
  }
  printf("SD mounted\n");

  char *skyFiles[255];
  sd_skylander_count = f_listfiles(skyFiles, sizeof(skyFiles)/sizeof(skyFiles[0]));

  // INIT TINYUSB
  board_init();
  // tusb_init();
  tud_init(0);
  printf("INIT TinyUSB done\n");

  lcd_draw_2line("Finished", "Initialization");
  sleep_ms(500);

  lcd_draw_2line("Skylander Portal", "Emulator");

  char nbuffer[MSG_SIZE];

  bool send_data = false;
  while (true)
  {
    if (!debounce_read(&db_state[butt_select]))
    {
      if (selected_skylander == -1)
      {
          lcd_draw_2line("Please select", "a Skylander");
          sleep_ms(500);
          printf("Please select a Skylander\n");
      }
      else
      {
        // TODO: Check if slot already has a file, if yes remove it
        if(loaded_skylanders[selected_slot] != 0)
        {
          FIL *toClose = loaded_skylanders[selected_slot];
          f_close(toClose);
          free(toClose);
          loaded_skylanders[selected_slot] = 0;
          memset(nbuffer, 0, MSG_SIZE);
          nbuffer[0] = 0x53;
          nbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
          nbuffer[5] = sense_counter++;
          nbuffer[6] = 0x01;
          tud_hid_report(0, nbuffer, MSG_SIZE);
          sleep_ms(500);
            
        }

        FIL *newfile = (FIL *)calloc(1, sizeof(FIL));
        FRESULT fr = f_open(newfile, skyFiles[selected_skylander], FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (fr != FR_OK && fr != FR_EXIST)
        {
          printf("f_open(%s) error (Probably because file is already loaded): %s (%d)\n", skyFiles[selected_skylander], FRESULT_str(fr), fr);
          //remove_fd_from_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT);
          lcd_draw_2line("File already", "loaded");
          sleep_ms(500);
          printf("f_open(%s) error probably already loaded\n", skyFiles[selected_skylander]);
        }
        else
        {
          if (fd_in_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT) == 0)
          {
            //add_fd_to_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT);
            loaded_skylanders[selected_slot] = newfile;

            printf("File %s loaded\n", skyFiles[selected_skylander]);
            lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("  File  loaded  ") / 2);
            lcd_string("  File  loaded  ");
            sleep_ms(100);
            // Needs to be called for skylander to be read
            memset(nbuffer, 0, MSG_SIZE);
            nbuffer[0] = 0x53;
            nbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
            nbuffer[5] = sense_counter++;
            nbuffer[6] = 0x01;
            tud_hid_report(0, nbuffer, MSG_SIZE);
            sleep_ms(500);
          }
          else
          {
            //remove_fd_from_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT);
            lcd_draw_2line("File already", "loaded");
            sleep_ms(500);
            printf("File already loaded\n");
          }
        }
      }
      lcd_draw_status();
    }

    if (!debounce_read(&db_state[butt_left]))
    {
      if (selected_skylander == -1)
      {
        selected_skylander = 0;
      }
      else if (selected_skylander > 0)
      {
        selected_skylander--;
      }
      else
       {
        selected_skylander = sd_skylander_count - 1;
      }

      printf("Selected Skylander #%d\n", selected_skylander);
      lcd_draw_2line("Selected File", skyFiles[selected_skylander]);
      sleep_ms(500);
      lcd_draw_status();
    }

    if (!debounce_read(&db_state[butt_right]))
    {
      if (selected_skylander < sd_skylander_count - 1)
      {
        selected_skylander++;
      }
      else
      {
        selected_skylander = 0;
      }
      printf("Selected Skylander #%d\n", selected_skylander);

      lcd_draw_2line("Selected File", skyFiles[selected_skylander]);
      sleep_ms(500);
      lcd_draw_status();
    }

    if(!debounce_read(&db_state[butt_slot_right])){
      if (selected_slot < MAX_SKYLANDER_COUNT - 1)
      {
        selected_slot++;
      }
      else{
        selected_slot = 0;
      }
      char str[2];
      sprintf(str, "%d", selected_slot);
      lcd_draw_2line("Selected Slot", str);
      printf("Selected slot %d\n", selected_slot);
      sleep_ms(500);
      lcd_draw_status();
    }

    if(!debounce_read(&db_state[butt_slot_left])){
      if (selected_slot > 0)
      {
        selected_slot--;
      }
      else
      {
        selected_slot = MAX_SKYLANDER_COUNT - 1;
      }
      char str[2];
      sprintf(str, "%d", selected_slot);
      lcd_draw_2line("Selected Slot", str);
      printf("Selected slot %d\n", selected_slot);
      sleep_ms(500);
      lcd_draw_status();
    }

    if (!debounce_read(&db_state[butt_start]))
    {
      memset(nbuffer, 0, MSG_SIZE);
      nbuffer[0] = 0x53;
      nbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
      nbuffer[5] = sense_counter++;
      nbuffer[6] = 0x01;
      tud_hid_report(0, nbuffer, MSG_SIZE);
      printf("Start\n");
    }

    tud_task();

    // tinyusb device task
    // sleep_ms(1000);
  }
  return 0;
}

int SkylanderEndsWith(const char *str, const char *suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (0 == strcmp(str + (str_len - suffix_len), suffix));
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  printf("GET %X", report_type);
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) // Console/ Computer sends data (Query, Shutdown etc.)
{
  char outbuffer[MSG_SIZE];
  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // char paddedbuffer[MSG_SIZE] = {0};
    FIL *curfile = 0;
    uint actual_len = 0;

    switch (buffer[0]) // Commands we can recieve from the host
    {
    case 'R': // 0x52 Reboot/Shutdown Portal
      printf("Recieved reboot\n");
      //shutthefuckup = true;
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = 0x52;
      outbuffer[1] = 0x02;
      outbuffer[2] = 0x1b;
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'J':
      printf("I do not know what it does but something related to sound\n");
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = buffer[0];
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'M':
      printf("Activate / Deactivate speaker\n");
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = buffer[0]; // We only send the command back without any return value, since we do not have a speaker
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'A': // 0x41 Activate Portal
      printf("Recieved activate\n");
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = 0x41;
      outbuffer[1] = buffer[1];
      outbuffer[2] = 0xff;
      outbuffer[3] = 0x77;
      tud_hid_report(0, outbuffer, MSG_SIZE);
      //if (buffer[1] == 0x01)
      //  shutthefuckup = false;
      break;

    case 'S': // 0x53 Sense how many Skylanders are on the Portal
      printf("Recieved sense\n");
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = 0x53;
      outbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
      // pseudo: outbuffer[2:5] = {0x00} (len = 3)
      outbuffer[5] = sense_counter++;
      outbuffer[6] = 0x01;
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'Q': // 0x51 Read Blocks (16 Bytes) From Skylander
      printf("Recieved query\n");
      memset(outbuffer, 0, MSG_SIZE);
      outbuffer[0] = 0x51;
      if ((buffer[1] >> 4) >= 0x1) {
          unsigned char s = buffer[1] & 0x0f;
          if (s < MAX_SKYLANDER_COUNT) {
              outbuffer[1] = 0x10 | s;
              curfile = loaded_skylanders[s];
          }
      }
      
      outbuffer[2] = buffer[2];

      if (curfile != 0) {
          f_lseek(curfile, buffer[2] * BLOCK_SIZE);
          f_read(curfile, outbuffer + 3, BLOCK_SIZE, &actual_len);
      }
      else {
          printf("Curfile invalide\n");
          return;
      }

      if (actual_len != BLOCK_SIZE)
      {
        printf("Read data length is %i not %i", actual_len, BLOCK_SIZE);
        return;
      }

      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'W': // 0x57 Write Blocks (16 Bytes) To Skylander
      printf("Recieved write\n");
      // Add writing to file

      if ((buffer[1] >> 4) >= 0x1) {
          unsigned char s = buffer[1] & 0x0f;
          if (s < MAX_SKYLANDER_COUNT) {
              curfile = loaded_skylanders[s];
          }
      }
      if (curfile != 0) {
          f_lseek(curfile, buffer[2] * BLOCK_SIZE);

          f_write(curfile, buffer+3, BLOCK_SIZE, &actual_len);

          if (actual_len != BLOCK_SIZE)
          {
              // lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("    Starting    ") / 2);
              // lcd_string(" Error  writing ");
              printf("Read data length is %i not %i", actual_len, BLOCK_SIZE);
              return;
          }
      }
      else {
          printf("Invalid Curfile\n");
          return;
      }

      // If skylander #1 -> buffer[1] = 0x10, else buffer[1] = 0x11
      memset(outbuffer, 0, MSG_SIZE);

      memcpy(outbuffer, buffer, 19);

      if ((buffer[1] >> 4) >= 0x1) {
          unsigned char s = buffer[1] & 0x0f;
          outbuffer[1] = 0x10 | s;
      }
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;

    case 'C': // 0x42 Skylander Portal Color
      // byte 1: RED
      // byte 2: GREEN
      // byte 3: BLUE
      printf("Color %d, %d, %d\n", buffer[1], buffer[2], buffer[3]));
      for (int i = 0; i < 2; i ++) {
        neo.setPixelColor(i, buffer[1], buffer[2], buffer[3]);
      }
      neo.show();
      tud_hid_report(0, buffer, MSG_SIZE);
      break;
    }
  }
}
