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

#include "ztypes.h"

#define A2Z_VERSION "2.0"

#define MAXFILELIST 50 // max. # of game files to display
char **storyfilelist;

Adafruit_QSPI_Flash flash;
Adafruit_M0_Express_CircuitPython spiffs(flash);

extern ztheme_t themes[];
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

int selectTheme()
{
  int buflen = 100;
  static char buf[100];
  static int theme = 1;
  int timeout = 0;
  int read_size = 0;
  buf[0] = '\0';
  extern int themecount;
  
  do
  {
    for(int i = 1; i <= themecount; i++)
    {
      Serial.print(i);Serial.print(": ");Serial.println(themes[i-1].tname);
    }
    Serial.print("Choose a color theme by number [");Serial.print(theme);Serial.print("]: "); 
    input_line( buflen, buf, timeout, &read_size );
    Serial.println();
    if(read_size > 0)
    {
      // use entered value, otherwise, use default
      buf[read_size] = '\0';
      theme = atoi(buf);
    }    
  }
  while(theme < 1 || theme > themecount);
  return theme - 1;
}

int selectStory()
{
  static int storynum = 1;
  int count = 0;
  storyfilelist = getDirectory(spiffs.open(GAMEPATH));
  do
  {
    char buffer[100];
    int buflen = 100;
    int timeout = 0;
    int read_size = 0;
    count = 0;
    Serial.println("0: Refresh list");
    while(storyfilelist[count] != NULL && count < MAXFILELIST)
    {
      Serial.print(count + 1);Serial.print(": ");Serial.println(storyfilelist[count]);
      count++;
    }
    if(count == 0)
    {
      Serial.print("No stories found [press any key to try again]");
      while (!Serial.available()){yield();};
      Serial.read();
      Serial.println();
      return -1;
    }
    Serial.print("Choose a story by number[");Serial.print(storynum);Serial.print("]: ");
    input_line( buflen, buffer, timeout, &read_size );
    Serial.println();
    if(read_size > 0)
    {
      buffer[read_size] = '\0';
      int tnum = atoi(buffer);
      if(tnum > 0)
        storynum = atoi(buffer);
      else
        return -1;    
    }
  }
  while(storynum <= 0 || storynum > count);

  return storynum - 1;
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

// sort an array of filenames
void filesort(char **a, int size) {
    for(int i=0; i<(size-1); i++) {
        for(int j=0; j<(size-(i+1)); j++) {
                int t1, t2;
                t1 = atoi(a[j]);
                t2 = atoi(a[j+1]);
                if((t1 > 0) && (t2 > 0))
                {
                  if(t1 > t2)
                  {
                    char *t = a[j];
                    a[j] = a[j+1];
                    a[j+1] = t;
                  }
                }
                else if(String(a[j]) > String(a[j+1])) {
                    char *t = a[j];
                    a[j] = a[j+1];
                    a[j+1] = t;
                }
        }
    }
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
  pinMode(13, OUTPUT);
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

  //storyfilelist = getDirectory(spiffs.open(GAMEPATH));

}

void loop()
{
  // This loops once per game
  Serial.println("A2Z Machine - Play Zork and other interactive fiction games on Arduino");
  Serial.println("Version " A2Z_VERSION);
  Serial.println("Visit DanTheGeek.com for project details.");
  Serial.println();

  // prompt for theme
  theme = selectTheme();
  initialize_screen(  );
  // prompt for story file
  int storynum = -1;
  while((storynum = selectStory()) < 0){yield();};

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
  Serial.println("Thanks for playing!");
  Serial.println();
}

