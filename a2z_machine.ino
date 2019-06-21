/*
 * A2Z Z Machine Emulator
 * Play Zork and other interactive fiction games on Arduino
 * See https://DanTheGeek.com for more info
 * 
 */
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>
#include "Adafruit_QSPI_Flash.h"
#include <mcurses.h>
#include "ztypes.h"

#define A2Z_VERSION "3.1"

#define MAXFILELIST 50 // max. # of game files to display
char **storyfilelist;

Adafruit_QSPI_Flash flash;
Adafruit_M0_Express_CircuitPython spiffs(flash);

extern ztheme_t themes[];
extern int themecount;

int theme = 2; // default theme

void Blink(byte PIN, byte DELAY_MS, byte loops)
{  

  for (byte i=0; i<loops; i++)
  {
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
  }
  digitalWrite(PIN,LOW);
}

// sort an array of filenames
void filesort(char **a, int size) {
    for(int i=0; i<(size-1); i++) {
        for(int j=0; j<(size-(i+1)); j++) {
                int t1, t2;
                t1 = atoi(a[j]);
                t2 = atoi(a[j+1]);
                if((t1 > 0) && (t2 > 0))
                {
                  // number sort
                  if(t1 > t2)
                  {
                    char *t = a[j];
                    a[j] = a[j+1];
                    a[j+1] = t;
                  }
                }
                else
                {
                  String t1 = String(a[j]);
                  String t2 = String(a[j+1]);
                  t1.toLowerCase();
                  t2.toLowerCase();
                  if(t1 > t2) {
                    // string sort
                    char *t = a[j];
                    a[j] = a[j+1];
                    a[j+1] = t;
                  }
                }
        }
    }
}

char **getDirectory(Adafruit_SPIFlash_FAT::File dir)
{
  static char filebuff[MAXFILELIST*32];
  char *filebuffptr = filebuff;
  static char* dirlist[MAXFILELIST]; // max file list size
  int dirlistcount = 0;

  if(!dir.isDirectory())
  {
    fatal(String("getDirectory(): not a valid folder / " + String(dir.name())).c_str());
  }

  //clear previous filelist
  for(int i = 0; i < MAXFILELIST; i++)
  {
    dirlist[i] = NULL;
  }
  while (true)
  {
    Adafruit_SPIFlash_FAT::File entry = dir.openNextFile();
    if (! entry)
    {
      break;
    }
    if (! entry.isDirectory())
    {
        dirlist[dirlistcount++] = filebuffptr;
        strcpy(filebuffptr,entry.name());
        filebuffptr += strlen(entry.name()) + 1;
        if (dirlistcount > MAXFILELIST)
        {
           // no more files
          break;
        }
    }
    entry.close();
  }
   filesort(dirlist,dirlistcount);
   return dirlist;
}

void displayA2ZScreen(char filenames[MAXFILELIST][20], int count, int storynum)
{
  //int themecount = sizeof(themes)/sizeof(themes[0]);
  attrset(themes[theme].text_attr);
  erase(  );
  // top line
  attrset(themes[theme].status_attr);
  int     col;
  move(0,0);
  for (col = 0; col < COLS; col++)
  {
    addch (' ');
  }

  mvaddstr_P (0, 1, String("A2Z Machine - Version " + String(A2Z_VERSION) + " - DanTheGeek.com").c_str());
  attrset(themes[theme].text_attr);
  yield();
  mvaddstr_P ( 2, 1, String("Theme: " + String(themes[theme].tname)).c_str());
  yield();
  mvaddstr_P ( 3, 1, "Select a game to play:");
  yield();
  mvaddstr_P ( DEFAULT_ROWS - 2, 1, "MOVE:<tab>,<shift><tab>,<cursor>,letter|SELECT:<enter>|THEME:/|REFRESH:<f1>");
  yield();
  mvaddstr_P ( DEFAULT_ROWS - 1, 1, "Project details at https://DanTheGeek.com/a2zmachine");
  yield();
  // show stories
  move(5,0);
  for(int i = 0 ; i < count; i++)
  {
    if(i == storynum)
      attrset(themes[theme].status_attr);      
    int x = i % 5;
    int y = i / 5;
    mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[i]) + String(" ")).c_str() );
    yield();
    if(i == storynum)
      attrset(themes[theme].text_attr);
  }

  move(7+(count/5),1);
}

static bool showA2ZScreen(int &storynum)
{
  char filenames[MAXFILELIST][20];
  //int themecount = sizeof(themes)/sizeof(themes[0]);

  if ( !initscr(  ) )
  {
    fatal( "initialize_screen(): Couldn't init curses." );
  }
  setFunction_putchar(Arduino_putchar); // tell the library which output channel shall be used
  setFunction_getchar(Arduino_getchar); // tell the library which input channel shall be used
  curs_set(0);

  int x = 0, y = 0, count = 0;
  storyfilelist = getDirectory(spiffs.open(GAMEPATH));
  while(storyfilelist[count] != NULL && count < MAXFILELIST)
  {
    yield();
    char shortname[15];
    char paddedname[15];
    int i;
    for(i = 0; i < 14; i++)
    {
      yield();
      if(storyfilelist[count][i] == '.')
      {
        shortname[i] = '\0';
        break;
      }
      else
      {
        shortname[i] = storyfilelist[count][i];
      }
    }
    shortname[i] = '\0';
    for(i = 0; i < 14; i++)
    {
      yield();
      if(strlen(shortname) <= i)
        paddedname[i] = ' ';
      else
        //paddedname[i] = 'a';
        paddedname[i] = shortname[i];
   }
    paddedname[14] = '\0';
    strcpy(filenames[count], paddedname);
    //Serial.println(filenames[count]);
    count++;
  }
  displayA2ZScreen(filenames, count, storynum);

  if(count == 0)
  {
    Serial.print("No stories found [press any key to try again]");
    while (!Serial.available()){yield();};
    Serial.read();
    Serial.println();
    return false;
  }
  while(1)
  {
    #define SHIFTTAB 514
    #define CURSORUP 515
    #define CURSORDOWN 516
    #define CURSORRIGHT 517
    #define CURSORLEFT 518
    #define FUNCTION1  519
    int keystroke, escape1, escape2;
    keystroke = Arduino_getchar();
    if(keystroke == 0x1b)
    {
      escape1 = Arduino_getchar();
      escape2 = Arduino_getchar();
      //Serial.print("got ");Serial.println(escape1,HEX);
      //Serial.print("got ");Serial.println(escape2,HEX);
      if(escape1 == '[')
      {
        switch(escape2)
        {
          case 'A': //
            keystroke = CURSORUP;
            break;
          case 'B':
            keystroke = CURSORDOWN;
            break;
          case 'C':
            keystroke = CURSORRIGHT;
            break;
          case 'D':
            keystroke = CURSORLEFT;
            break;
          case 'Z':
            keystroke = SHIFTTAB;
            break;
          case '1':
            keystroke = FUNCTION1;
        }
      }
    }
    move(7+(count/5),1);
    //Serial.print(" got ");Serial.println(keystroke,HEX);
    yield();
    switch(keystroke)
    {
      case '\t': // tab
      case CURSORRIGHT:
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].text_attr);
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       yield();
       storynum = (storynum+1)%count;
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].status_attr);      
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       attrset(themes[theme].text_attr);
       move(7+(count/5),1);
       break;
      case SHIFTTAB:
      case CURSORLEFT:
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].text_attr);
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       storynum = (storynum-1)%count;
       if(storynum < 0)
        storynum = count - 1;
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].status_attr);      
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       attrset(themes[theme].text_attr);
       move(7+(count/5),1);
       break;
      case CURSORUP:
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].text_attr);
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       if(storynum - 5 >= 0)
       {
        storynum = (storynum-5)%count;
       }
       else
       {
        storynum = count - (count)%5 + (storynum-1)%count;
        if(storynum >= count)
          storynum -= 5;
       }
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].status_attr);      
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       attrset(themes[theme].text_attr);
       move(7+(count/5),1);
       break;
      case CURSORDOWN:
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].text_attr);
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       if(storynum + 5 < count)
        storynum = (storynum+5)%count;
       else
       x = storynum % 5;
       y = storynum / 5;
       attrset(themes[theme].status_attr);      
       mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
       attrset(themes[theme].text_attr);
       move(7+(count/5),1);
       break;
      case '/': // change theme
        theme = (theme + 1 ) %themecount;
       displayA2ZScreen(filenames, count, storynum);
       break;
      case '\r': // return
        curs_set(1);
        return true;
      case FUNCTION1: // refresh
        return false;
        break;
      default: // first letter of filename?
        if(isalnum(keystroke))
        {
          for(int i = 1; i <= count; i++)
          {
            if(tolower(*filenames[(storynum+i)%count]) == tolower(keystroke))
            {
             x = storynum % 5;
             y = storynum / 5;
             attrset(themes[theme].text_attr);
             mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
             storynum = (storynum+i)%count;
             x = storynum % 5;
             y = storynum / 5;
             attrset(themes[theme].status_attr);      
             mvaddstr_P (5+y,x*16,String(String(" ") + String(filenames[storynum]) + String(" ")).c_str());
             attrset(themes[theme].text_attr);
             move(7+(count/5),1);
             break;
            }
          }
        }
        break;
    }
  }
  curs_set(1);
  return true;
}

static void configure( zbyte_t min_version, zbyte_t max_version )
{
   zbyte_t header[PAGE_SIZE], second;

   read_page( 0, header );
   datap = header;

   h_type = get_byte( H_TYPE );

   GLOBALVER = h_type;
   if ( h_type < min_version || h_type > max_version ||
        ( get_byte( H_CONFIG ) & CONFIG_BYTE_SWAPPED ) )
      fatal( "Wrong game or version" );
   /*
    * if (h_type == V6 || h_type == V7)
    * fatal ("Unsupported zcode version.");
    */

   if ( h_type < V4 )
   {
      story_scaler = 2;
      story_shift = 1;
      property_mask = P3_MAX_PROPERTIES - 1;
      property_size_mask = 0xe0;
   }
   else if ( h_type < V8 )
   {
      story_scaler = 4;
      story_shift = 2;
      property_mask = P4_MAX_PROPERTIES - 1;
      property_size_mask = 0x3f;
   }
   else
   {
      story_scaler = 8;
      story_shift = 3;
      property_mask = P4_MAX_PROPERTIES - 1;
      property_size_mask = 0x3f;
   }

   h_config = get_byte( H_CONFIG );
   h_version = get_word( H_VERSION );
   h_data_size = get_word( H_DATA_SIZE );
   h_start_pc = get_word( H_START_PC );
   h_words_offset = get_word( H_WORDS_OFFSET );
   h_objects_offset = get_word( H_OBJECTS_OFFSET );
   h_globals_offset = get_word( H_GLOBALS_OFFSET );
   h_restart_size = get_word( H_RESTART_SIZE );
   h_flags = get_word( H_FLAGS );
   h_synonyms_offset = get_word( H_SYNONYMS_OFFSET );
   h_file_size = get_word( H_FILE_SIZE );
   if ( h_file_size == 0 )
      h_file_size = get_story_size(  );
   h_checksum = get_word( H_CHECKSUM );
   h_alternate_alphabet_offset = get_word( H_ALTERNATE_ALPHABET_OFFSET );

   if ( h_type >= V5 )
   {
      h_unicode_table = get_word( H_UNICODE_TABLE );
   }
   datap = NULL;

}

Adafruit_USBD_MSC usb_msc;

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and 
// return number of copied bytes (must be multiple of block size) 
int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize)
{
  const uint32_t addr = lba*512;
  flash_cache_read((uint8_t*) buffer, addr, bufsize);
  return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  // need to erase & caching write back
  const uint32_t addr = lba*512;
  flash_cache_write(addr, buffer, bufsize);
  return bufsize;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb (void)
{
  flash_cache_flush();
}

//--------------------------------------------------------------------+
// Flash Caching
//--------------------------------------------------------------------+
#define FLASH_CACHE_SIZE          4096        // must be a erasable page size
#define FLASH_CACHE_INVALID_ADDR  0xffffffff

uint32_t cache_addr = FLASH_CACHE_INVALID_ADDR;
uint8_t  cache_buf[FLASH_CACHE_SIZE];

static inline uint32_t page_addr_of (uint32_t addr)
{
  return addr & ~(FLASH_CACHE_SIZE - 1);
}

static inline uint32_t page_offset_of (uint32_t addr)
{
  return addr & (FLASH_CACHE_SIZE - 1);
}

void flash_cache_flush (void)
{
  if ( cache_addr == FLASH_CACHE_INVALID_ADDR ) return;

  // indicator
  digitalWrite(LED_BUILTIN, HIGH);

  flash.eraseSector(cache_addr/FLASH_CACHE_SIZE);
  flash.writeBuffer(cache_addr, cache_buf, FLASH_CACHE_SIZE);

  digitalWrite(LED_BUILTIN, LOW);

  cache_addr = FLASH_CACHE_INVALID_ADDR;
}

uint32_t flash_cache_write (uint32_t dst, void const * src, uint32_t len)
{
  uint8_t const * src8 = (uint8_t const *) src;
  uint32_t remain = len;

  // Program up to page boundary each loop
  while ( remain )
  {
    uint32_t const page_addr = page_addr_of(dst);
    uint32_t const offset = page_offset_of(dst);

    uint32_t wr_bytes = FLASH_CACHE_SIZE - offset;
    wr_bytes = min(remain, wr_bytes);

    // Page changes, flush old and update new cache
    if ( page_addr != cache_addr )
    {
      flash_cache_flush();
      cache_addr = page_addr;

      // read a whole page from flash
      flash.readBuffer(page_addr, cache_buf, FLASH_CACHE_SIZE);
    }

    memcpy(cache_buf + offset, src8, wr_bytes);

    // adjust for next run
    src8 += wr_bytes;
    remain -= wr_bytes;
    dst += wr_bytes;
  }

  return len - remain;
}

void flash_cache_read (uint8_t* dst, uint32_t addr, uint32_t count)
{
  // overwrite with cache value if available
  if ( (cache_addr != FLASH_CACHE_INVALID_ADDR) &&
       !(addr < cache_addr && addr + count <= cache_addr) &&
       !(addr >= cache_addr + FLASH_CACHE_SIZE) )
  {
    int dst_off = cache_addr - addr;
    int src_off = 0;

    if ( dst_off < 0 )
    {
      src_off = -dst_off;
      dst_off = 0;
    }

    int cache_bytes = min(FLASH_CACHE_SIZE-src_off, count - dst_off);

    // start to cached
    if ( dst_off ) flash.readBuffer(addr, dst, dst_off);

    // cached
    memcpy(dst + dst_off, cache_buf + src_off, cache_bytes);

    // cached to end
    int copied = dst_off + cache_bytes;
    if ( copied < count ) flash.readBuffer(addr + copied, dst + copied, count - copied);
  }
  else
  {
    flash.readBuffer(addr, dst, count);
  }
}

void setup()
{
  flash.begin();
  pinMode(LEDPIN, OUTPUT);
  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Adafruit", "A2Z Machine", "1.0");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.pageSize()*flash.numPages()/512, 512);

  // MSC is ready for read/write
  usb_msc.setUnitReady(true);
  
  usb_msc.begin();

  Serial.begin(9600);
  while (!Serial) {
  }  
 
  if (!spiffs.begin()) {
    fatal("Failed to mount filesystem, was CircuitPython loaded onto the board?");
  }

}

void loop()
{
  static int storynum = 0;
  // This loops once per game
  while(!showA2ZScreen(storynum));
  initialize_screen(  );

  char storyfile[200];

  sprintf(storyfile,"%s/%s",GAMEPATH, storyfilelist[storynum]);

  Serial.println("Opening story...");
  delay(500);
  open_story(storyfile);
  configure((zbyte_t) V1, (zbyte_t) V8 );
  initialize_screen(  );
  load_cache();
  z_restart(  );
  ( void ) interpret(  );
  unload_cache(  );
  close_story(  );
  close_script(  );
  reset_screen(  );
}

