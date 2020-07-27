#include <stdint.h>
#include "mmio.h"
#include "uart.h"
#include "delay.h"
#include "power.h"
#include "framebuffer.h"
#include "screen.h"
#include "sd.h"
#include "libc.h"
#include "rand.h"

#define VERSION "v0.0.1"
#define UNUSED(x) (void)x

extern uint8_t __end;

int kern_shell() {
  print("> ");
  char buff[80] = "";
  int bufflen = 0;
  while(buff[bufflen - 1] != '\r') {
    char a = uart_getc();

    if(a == '\b' || a == 0x7F) {
      // CHANGE THIS IN FUTURE IF ARROW KEYS
      // ARE IMPLEMENTED
      buff[bufflen - 1] = '\0';
      bufflen--;
      // OK, THE REST OF THE CODE IS GOOD
      screen_del();
      continue;
    }

    if(bufflen == 80) {
      if(a == '\r') {
        break;
      } else {
        continue;
      }
    } 

    print_c(a);
    buff[bufflen] = a;

    bufflen++;
  }
  print("\r\n");

  // get rid of \n at end of buff
  buff[bufflen - 1] = '\0'; // just overwrite it with null terminator

  char tokens[40][80];
  int numtoks = 0;
  char curr_tok[75];
  int curr_tok_i = 0;
  int i;
  for(i = 0; i < bufflen; i++) {
    if(buff[i] == ' ' || i == bufflen - 1) {
      strcpy(tokens[numtoks], curr_tok); 
      numtoks++;
      int j;
      for(j = 0; j <= curr_tok_i; j++) {
        curr_tok[j] = 0;
      }
      curr_tok_i = 0;
    } else {
      curr_tok[curr_tok_i] = buff[i];
      curr_tok_i++;
    }
  }

  if(!strcmp(tokens[0], "version")) {
    print("osARM ");
    print(VERSION);
    print("\r\n");
  } else if(!strcmp(tokens[0], "cls")) {
    screen_cls();
    set_cursor(0, 0);
  } else if(!strcmp(tokens[0], "rand")) {
    print_hex(rand(0,4294967295));
    print("\r\n");
  } else if(!strcmp(tokens[0], "disk")) {
    if(sd_readblock(0,&__end,1)) {
      // dump it to serial console
      uart_dump(&__end);
    }
  } else if(!strcmp(tokens[0], "hex")) {
    print(itoa(atoi(tokens[1])));
    print("\r\n");
  } else if(!strcmp(tokens[0], "set")) {
    if(!strcmp(tokens[1], "screen")) {
      if(strlen(tokens[2]) > 1) {
        print("Screen codes are 1 digit\r\n");
      } else {
        int code = *tokens[2] - '0';
        set_screen_mode(code);
      }
    } else {
      print("No such kernel option\r\n");
    }
  } else if(!strcmp(tokens[0], "get")) {
    if(!strcmp(tokens[1], "screen")) {
      int code = get_screen_mode();
      print_c(code + '0');
      print("\r\n");
    } else {
      print("No such kernel option\r\n");
    }
  } else if(!strcmp(tokens[0], "wait")) {
    wait_msec(1000);
  } else if(!strcmp(tokens[0], "reboot")) {
    return -1;  // triggers reboot sequence
  } else {
    print("Unsupported Kernel Command\r\n");
  }

  return 0;
}

void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3) {
  UNUSED(dtb_ptr32);
  UNUSED(x1);
  UNUSED(x2);
  UNUSED(x3);

  uart_init();
  rand_init();
  fb_init();
  screen_cls(); // will apply color
  // COMMENT OUT FOLLOWING TO DISABLE SD CARD
  if(sd_init() != SD_OK) {
    print("FATAL ERROR: SD INITALIZATION ERROR");
    while(1) {
    }
  }
  // END COMMENTING OUT

  print("\r\nWelcome to osARM!\r\n\r\n");

  while (1) {
    int r = kern_shell();
    switch(r) {
      case 0: {
        continue;
      }
      case -1: {
        reboot();
      }
    }
  }
}
