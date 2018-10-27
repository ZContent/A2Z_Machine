#include <SPI.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>
#include <Adafruit_QSPI_GD25Q.h>

#include "ztypes.h"

#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
#define MAXFILELIST 50 // max. # of game files to display
char **storyfilelist;

Adafruit_QSPI_GD25Q flash;

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
  char buf[buflen];
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
  do
  {
    char buffer[100];
    int buflen = 100;
    int timeout = 0;
    int read_size = 0;
    count = 0;
    while(storyfilelist[count] != NULL && count < MAXFILELIST)
    {
      Serial.print(count + 1);Serial.print(": ");Serial.println(storyfilelist[count]);
      count++;
    }
    if(count == 0)
    {
      fatal("No stories found");
    }
    Serial.print("Choose a story by number[");Serial.print(storynum);Serial.print("]: "); 
    input_line( buflen, buffer, timeout, &read_size );
    if(read_size > 0)
    {
      buffer[read_size] = '\0';
      storynum = atoi(buffer);      
    }
    Serial.println();
  }
  while(storynum <= 0 || storynum > count);

  return storynum - 1;
}

char **getDirectory(File dir)
{
  static char filebuff[MAXFILELIST*32];
  char *filebuffptr = filebuff;
  static char* dirlist[MAXFILELIST]; // max file list size
  int dirlistcount = 0;

  if(!dir.isDirectory())
  {
    fatal("getDirectory(): not a valid folder");
  }

  //clear previous filelist
  for(int i = 0; i < MAXFILELIST; i++)
  {
    dirlist[i] = NULL;
  }
  while (true)
  {
    File entry = dir.openNextFile();
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

void setup()
{
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    delay(100);
  }  

  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    fatal("Error, failed to initialize flash chip!");
  }
  flash.setFlashType(FLASH_TYPE);
  
  if (!spiffs.begin()) {
    fatal("Failed to mount filesystem, was CircuitPython loaded onto the board?");
  }
  storyfilelist = getDirectory(spiffs.open(GAMEPATH));
}

void loop()
{
  // This loops once per game
  Serial.println("A2Z Machine - Play Zork and other interactive fiction games on Arduino");
  Serial.println("Version 1.0");
  Serial.println("Visit DanTheGeek.com for project details.");
  Serial.println();

  // prompt for theme
  theme = selectTheme();
  initialize_screen(  );
  // prompt for story file
  int storynum = selectStory();

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

