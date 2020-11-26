#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

// Remember that in the RPI there is BUS memory, physical memory
// and Virtual memory addressing spaces.
// Location of peripheral registers in physical memory.
//#define GPIO_BASE  (0x20000000 + 0x200000)  /* Pi Zero or 1 */
#define GPIO_BASE    (0x3F000000 + 0x200000)  /* Pi 2 or 3    */
//#define GPIO_BASE  (0xFE000000 + 0x200000)  /* Pi 4         */

#define GPLVL0       0x34    // GPIO low set pins Input register.

static volatile uint32_t *gpio;   // Pointer to the gpio (*int)

void printBinary(uint32_t value);
void printBinaryLine(char * prevStr, uint32_t value);

int main() {
   int fd;
   printf("Start of GPIO input max sample rate test program.\n");
   if(getuid()!=0) {
      printf("You must run this program as root (sudo). Exiting.\n");
      return 1;
   }
   if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
      printf("Unable to open /dev/mem: %s\n", strerror(errno));
      return 1;
   }
   // get a pointer that points to the peripheral base for the GPIOs
   gpio = (uint32_t *) mmap(0, getpagesize(), PROT_READ|PROT_WRITE,
          MAP_SHARED, fd, GPIO_BASE);
   if ((int32_t) gpio < 0) {
      printf("Memory mapping failed: %s\n", strerror(errno));
      return 1;
   }

   // jnc - We are going to tell that the pin 21  is an input pin and
   //       by default he already is, but we are going to set it the same.
   //       The pin 21 type is GPFSEL2 and that is the 3º register in memory
   //       after base GPIO address that is GPFSEL2 in bits 5, 4 and 3. 
   //       Remember that adding one 32-bit value moves the addr by 4 bytes
   //       writing NOT 7 (i.e., ~111 that is 000 to put the pin 21 as input),
   //       it clears bits 5, 4 e 3.
   *(gpio + 2) = (*(gpio + 2) & ~(7 << 3));

   const uint32_t BUF_SIZE = 1000000;

   uint32_t buf[BUF_SIZE];
   memset(&buf, 0, sizeof(uint32_t)*BUF_SIZE);
   uint32_t *bufPtr = buf;
   clock_t start, end;
   double cpu_time_used;
   start = clock();

   // Get's the value of the input low set of pins register.
   // They must be volatile so that a fresh version is always
   // directly read from memory.
   volatile uint32_t * reg = (gpio + (GPLVL0/4));

   while(bufPtr < buf + BUF_SIZE)
   {
      //*bufPtr = (*(gpio + (GPLVL0/4)));
      *bufPtr = *reg;
      bufPtr++;
   }
    
   end = clock();
   cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

   printBinaryLine("__buf[0]            : ", buf[0]);
   printBinaryLine("__buf[1000]         : ", buf[1000]);
   printBinaryLine("__buf[500_000]      : ", buf[500000]);
   printBinaryLine("__buf[1_000_000 - 1]: ", buf[999999]);

   printf("To record 1_000_000 samples, the function took %f sec, sample_freq %f MHz\n", cpu_time_used, 1/cpu_time_used);

   close(fd);
   return 0;
}

void printBinary(uint32_t value)
{
    printf(BYTE_TO_BINARY_PATTERN " "BYTE_TO_BINARY_PATTERN " "BYTE_TO_BINARY_PATTERN " "BYTE_TO_BINARY_PATTERN,
           BYTE_TO_BINARY(value>>24), BYTE_TO_BINARY(value>>16), BYTE_TO_BINARY(value>>8), BYTE_TO_BINARY(value));
}

void printBinaryLine(char * prevStr, uint32_t value){
    printf("%s", prevStr);
    printBinary(value);
    printf("\n");    
}

