#include "mmio.h"
#include "screen.h"
#include "delay.h"
#include "sd.h"

#define EMMC_ARG2           MMIO_BASE + 0x00300000
#define EMMC_BLKSIZECNT     MMIO_BASE + 0x00300004
#define EMMC_ARG1           MMIO_BASE + 0x00300008
#define EMMC_CMDTM          MMIO_BASE + 0x0030000C
#define EMMC_RESP0          MMIO_BASE + 0x00300010
#define EMMC_RESP1          MMIO_BASE + 0x00300014
#define EMMC_RESP2          MMIO_BASE + 0x00300018
#define EMMC_RESP3          MMIO_BASE + 0x0030001C
#define EMMC_DATA           MMIO_BASE + 0x00300020
#define EMMC_STATUS         MMIO_BASE + 0x00300024
#define EMMC_CONTROL0       MMIO_BASE + 0x00300028
#define EMMC_CONTROL1       MMIO_BASE + 0x0030002C
#define EMMC_INTERRUPT      MMIO_BASE + 0x00300030
#define EMMC_INT_MASK       MMIO_BASE + 0x00300034
#define EMMC_INT_EN         MMIO_BASE + 0x00300038
#define EMMC_CONTROL2       MMIO_BASE + 0x0030003C
#define EMMC_SLOTISR_VER    MMIO_BASE + 0x003000FC

// command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xfff9c004
#define CMD_RCA_MASK        0xffff0000

// COMMANDs
#define CMD_GO_IDLE         0x00000000
#define CMD_ALL_SEND_CID    0x02010000
#define CMD_SEND_REL_ADDR   0x03020000
#define CMD_CARD_SELECT     0x07030000
#define CMD_SEND_IF_COND    0x08020000
#define CMD_STOP_TRANS      0x0C030000
#define CMD_READ_SINGLE     0x11220010
#define CMD_READ_MULTI      0x12220032
#define CMD_SET_BLOCKCNT    0x17020000
#define CMD_APP_CMD         0x37000000
#define CMD_SET_BUS_WIDTH   (0x06020000 | CMD_NEED_APP)
#define CMD_SEND_OP_COND    (0x29020000 | CMD_NEED_APP)
#define CMD_SEND_SCR        (0x33220010 | CMD_NEED_APP)

// STATUS register settings
#define SR_READ_AVAILABLE   0x00000800
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_CMD_DONE        0x00000001

#define INT_ERROR_MASK      0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN      0x00100000
#define C0_HCTL_HS_EN       0x00000004
#define C0_HCTL_DWITDH      0x00000002

#define C1_SRST_DATA        0x04000000
#define C1_SRST_CMD         0x02000000
#define C1_SRST_HC          0x01000000
#define C1_TOUNIT_DIS       0x000f0000
#define C1_TOUNIT_MAX       0x000e0000
#define C1_CLK_GENSEL       0x00000020
#define C1_CLK_EN           0x00000004
#define C1_CLK_STABLE       0x00000002
#define C1_CLK_INTLEN       0x00000001

// SLOTISR_VER values
#define HOST_SPEC_NUM       0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3        2
#define HOST_SPEC_V2        1
#define HOST_SPEC_V1        0

// SCR flags
#define SCR_SD_BUS_WIDTH_4  0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS        0x00000001

#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000

unsigned long sd_scr[2], sd_ocr, sd_rca, sd_err, sd_hv;

/**
 * Wait for data or command ready
 */
int sd_status(unsigned int mask)
{
    int cnt = 500000;
    while( (mmio_read(EMMC_STATUS) & mask) && !(mmio_read(EMMC_INTERRUPT) & INT_ERROR_MASK) && cnt-- ) {
    	wait_msec(1);
    };
    return (cnt <= 0 || (mmio_read(EMMC_INTERRUPT) & INT_ERROR_MASK)) ? SD_ERROR : SD_OK;
}

/**
 * Wait for interrupt
 */
int sd_int(unsigned int mask)
{
    unsigned int r, m = mask | INT_ERROR_MASK;
    int cnt = 1000000;
    while( !(mmio_read(EMMC_INTERRUPT) & m) && cnt-- ) {
    	wait_msec(1);
    }

    r = mmio_read(EMMC_INTERRUPT);
    if(cnt <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT) ) {
    	mmio_write(EMMC_INTERRUPT, r);
    	return SD_TIMEOUT;
    } else if(r & INT_ERROR_MASK) {
    	mmio_write(EMMC_INTERRUPT, r);
    	return SD_ERROR;
    }

    mmio_write(EMMC_INTERRUPT, mask);
    return 0;
}

/**
 * Send a command
 */
int sd_cmd(unsigned int code, unsigned int arg)
{
    unsigned int r = 0;
    sd_err = SD_OK;
    if(code & CMD_NEED_APP) {
        r = sd_cmd(CMD_APP_CMD | (sd_rca ? CMD_RSPNS_48 : 0), sd_rca);
        if(sd_rca && !r) {
        	print("ERROR: failed to send SD APP command\n");
        	sd_err = SD_ERROR;
        	return 0;
        }
        code &= ~CMD_NEED_APP;
    }
    if( sd_status(SR_CMD_INHIBIT) ) {
    	print("ERROR: EMMC busy\n");
    	sd_err = SD_TIMEOUT;
    	return 0;
    }
    print("EMMC: Sending command ");
    print_hex(code);
    print(" arg ");
    print_hex(arg);
    print("\n");

    mmio_write(EMMC_INTERRUPT, mmio_read(EMMC_INTERRUPT));
    mmio_write(EMMC_ARG1, arg);
    mmio_write(EMMC_CMDTM, code);

    if(code == CMD_SEND_OP_COND) {
    	wait_msec(1000);
    } else if(code == CMD_SEND_IF_COND || code == CMD_APP_CMD) {
    	wait_msec(100);
    }

    if( (r = sd_int(INT_CMD_DONE)) ) {
    	print("ERROR: failed to send EMMC command\n");
    	sd_err = r;
    	return 0;
    }
    r = mmio_read(EMMC_RESP0);

    if(code == CMD_GO_IDLE || code == CMD_APP_CMD) {
    	return 0;
    } else if(code == (CMD_APP_CMD | CMD_RSPNS_48)) {
    	return r & SR_APP_CMD;
    } else if(code == CMD_SEND_OP_COND) {
    	return r;
    } else if(code == CMD_SEND_IF_COND) {
    	return r == arg ? SD_OK : SD_ERROR;
    } else if(code == CMD_ALL_SEND_CID) {
    	r |= mmio_read(EMMC_RESP3);
    	r |= mmio_read(EMMC_RESP2);
    	r |= mmio_read(EMMC_RESP1);
    	return r; 
    } else if(code == CMD_SEND_REL_ADDR) {
        sd_err = ( (r & 0x1fff) | (r & 0x2000) << 6 | ((r & 0x4000) << 8) | ((r & 0x8000) << 8) ) & CMD_ERRORS_MASK;
        return r & CMD_RCA_MASK;
    }

    return r & CMD_ERRORS_MASK;
    // make gcc happy
    return 0;
}

/**
 * read a block from sd card and return the number of bytes read
 * returns 0 on error.
 */
int sd_readblock(unsigned int lba, uint8_t *buffer, unsigned int num)
{
    unsigned int r, c = 0, d;
    if(num < 1) {
    	num = 1;
    }

    print("sd_readblock lba ");
    print_hex(lba);
    print(" num ");
    print_hex(num);
    print("\n");

    if(sd_status(SR_DAT_INHIBIT)) {
    	sd_err = SD_TIMEOUT;
    	return 0;
    }

    unsigned int *buf = (unsigned int *)buffer;

    if(sd_scr[0] & SCR_SUPP_CCS) {
        if(num > 1 && (sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
            sd_cmd(CMD_SET_BLOCKCNT, num);
            if(sd_err) {
            	return 0;
            }
        }
        mmio_write(EMMC_BLKSIZECNT, (num << 16) | 512);
        sd_cmd(num == 1 ? CMD_READ_SINGLE : CMD_READ_MULTI, lba);
        if(sd_err) {
        	return 0;
        }
    } else {
    	mmio_write(EMMC_BLKSIZECNT, (1 << 16) | 512);
    }
    while( c < num ) {
        if( !(sd_scr[0] & SCR_SUPP_CCS) ) {
            sd_cmd(CMD_READ_SINGLE, (lba + c) * 512);
            if(sd_err) {
            	return 0;
            }
        }

        if( (r = sd_int(INT_READ_RDY)) ) {
        	print("\rERROR: Timeout waiting for ready to read\n");
        	sd_err = r;
        	return 0;
        }

        for(d = 0; d < 128; d++) {
        	buf[d] = mmio_read(EMMC_DATA);
        }

        c++;
        buf += 128;
    }

    if( num > 1 && !(sd_scr[0] & SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & SCR_SUPP_CCS) ) {
    	sd_cmd(CMD_STOP_TRANS, 0);
    }

    return sd_err != SD_OK || c != num ? 0 : num * 512;
}

/**
 * set SD clock to frequency in Hz
 */
int sd_clk(unsigned int f)
{
    unsigned int d, c = 41666666 / f, x, s = 32, h = 0;
    int cnt = 100000;
    while( (mmio_read(EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT) ) && cnt--) {
    	wait_msec(1);
    }

    if(cnt <= 0) {
        print("ERROR: timeout waiting for inhibit flag\n");
        return SD_ERROR;
    }

    mmio_write(EMMC_CONTROL1, mmio_read(EMMC_CONTROL1) & ~C1_CLK_EN);
    wait_msec(10);
    
    x = c - 1;
    if(!x) {
    	s = 0;
    } else {
        if( !(x & 0xffff0000u) ) {
        	x <<= 16;
        	s -= 16;
        }
        if( !(x & 0xff000000u) ) {
        	x <<= 8;
        	s -= 8;
        }
        if( !(x & 0xf0000000u) ) {
        	x <<= 4;
        	s -= 4;
        }
        if( !(x & 0xc0000000u) ) {
        	x <<= 2;
        	s -= 2;
        }
        if( !(x & 0x80000000u) ) {
        	x <<= 1;
        	s -= 1;
        }

        if(s>0) {
        	s--;
        }
        if(s>7) {
        	s=7;
        }
    }

    if(sd_hv > HOST_SPEC_V2) {
    	d = c;
    } else {
    	d = (1 << s);
    }

    if(d <= 2) {
    	d = 2;
    	s = 0;
    }

    print("sd_clk divisor ");
    print_hex(d);
    print(", shift ");
    print_hex(s);
    print("\n");

    if(sd_hv>HOST_SPEC_V2) {
    	h = (d & 0x300) >> 2;
    }

    d = ( ( (d & 0x0ff) << 8 ) | h );
    mmio_write(EMMC_CONTROL1, (mmio_read(EMMC_CONTROL1) & 0xffff003f) | d);
    wait_msec(10);
    mmio_write(EMMC_CONTROL1, mmio_read(EMMC_CONTROL1) | C1_CLK_EN);
    wait_msec(10);

    cnt = 10000;
    while( !(mmio_read(EMMC_CONTROL1) & C1_CLK_STABLE) && cnt-- ) {
    	wait_msec(10);
    }

    if(cnt <= 0) {
        print("ERROR: failed to get stable clock\n");
        return SD_ERROR;
    }

    return SD_OK;
}

/**
 * initialize EMMC to read SDHC card
 */
int sd_init() {
    long r, cnt, ccs = 0;
    // GPIO_CD
    r = mmio_read(GPFSEL4);
    r &= ~( 7 << (7 * 3) );

    mmio_write(GPFSEL4, r);
    mmio_write(GPPUD, 2);
    wait_cycles(150);

    mmio_write(GPPUDCLK1, 1<<15);
    wait_cycles(150);

    mmio_write(GPPUD, 0);
    mmio_write(GPPUDCLK1, 0);

    r = mmio_read(GPHEN1);
    r |= (1 << 15);
    mmio_write(GPHEN1, r);

    // GPIO_CLK, GPIO_CMD
    r = mmio_read(GPFSEL4);
    r |= (7 << (8 * 3)) | (7 << (9 * 3));

    mmio_write(GPFSEL4, r);
    mmio_write(GPPUD, 2);
    wait_cycles(150);

    mmio_write(GPPUDCLK1, (1 << 16) | (1 << 17));
    wait_cycles(150);

    mmio_write(GPPUD, 0);
    mmio_write(GPPUDCLK1, 0);

    // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
    r = mmio_read(GPFSEL5);
    r |= (7 << (0*3)) | (7 << (1*3)) | (7 << (2*3)) | (7 << (3*3));

    mmio_write(GPFSEL5, r);
    mmio_write(GPPUD, 2);
    wait_cycles(150);

    mmio_write(GPPUDCLK1, (1<<18) | (1<<19) | (1<<20) | (1<<21));
    wait_cycles(150);

    mmio_write(GPPUD, 0);
    mmio_write(GPPUDCLK1, 0);

    sd_hv = ( mmio_read(EMMC_SLOTISR_VER) & HOST_SPEC_NUM ) >> HOST_SPEC_NUM_SHIFT;
    print("EMMC: GPIO set up\n");

    // Reset the card.
    mmio_write(EMMC_CONTROL0, 0);
    mmio_write(EMMC_CONTROL1, mmio_read(EMMC_CONTROL1) | C1_SRST_HC);

    cnt = 10000;
    do {
    	wait_msec(10);
    } while( (mmio_read(EMMC_CONTROL1) & C1_SRST_HC) && cnt-- );

    if(cnt <= 0) {
        print("ERROR: failed to reset EMMC\n");
        return SD_ERROR;
    }

    print("EMMC: reset OK\n");
    mmio_write( EMMC_CONTROL1, mmio_read(EMMC_CONTROL1) | (C1_CLK_INTLEN | C1_TOUNIT_MAX) );
    wait_msec(10);

    // Set clock to setup frequency.
    if( (r = sd_clk(400000)) ) {
    	return r;
    }
    mmio_write(EMMC_INT_EN, 0xffffffff);
    mmio_write(EMMC_INT_MASK, 0xffffffff);
    sd_scr[0] = sd_scr[1] = sd_rca = sd_err = 0;
    sd_cmd(CMD_GO_IDLE, 0);
    if(sd_err) {
    	return sd_err;
    }

    sd_cmd(CMD_SEND_IF_COND, 0x000001AA);
    if(sd_err) {
    	return sd_err;
    }

    cnt = 6;
    r = 0;
    while( !(r & ACMD41_CMD_COMPLETE) && cnt-- ) {
        wait_cycles(400);
        r = sd_cmd(CMD_SEND_OP_COND, ACMD41_ARG_HC);

        print("EMMC: CMD_SEND_OP_COND returned ");
        if(r & ACMD41_CMD_COMPLETE) {
            print("COMPLETE ");
        }
        if(r&ACMD41_VOLTAGE) {
            print("VOLTAGE ");
        }
        if(r&ACMD41_CMD_CCS) {
            print("CCS ");
        }
        print_hex(r >> 32);
        print_hex(r);
        print("\n");
        if(sd_err != (unsigned long)SD_TIMEOUT && sd_err != SD_OK ) {
            print("ERROR: EMMC ACMD41 returned error\n");
            return sd_err;
        }
    }
    if( !(r & ACMD41_CMD_COMPLETE) || !cnt ) {
    	return SD_TIMEOUT;
    }
    if( !(r & ACMD41_VOLTAGE) ) {
    	return SD_ERROR;
    }
    if(r & ACMD41_CMD_CCS) {
    	ccs = SCR_SUPP_CCS;
    }

    sd_cmd(CMD_ALL_SEND_CID, 0);

    sd_rca = sd_cmd(CMD_SEND_REL_ADDR,0);
    print("EMMC: CMD_SEND_REL_ADDR returned ");
    print_hex(sd_rca >> 32);
    print_hex(sd_rca);
    print("\n");
    if(sd_err) {
    	return sd_err;
    }

    if( (r = sd_clk(25000000)) ) {
    	return r;
    }

    sd_cmd(CMD_CARD_SELECT, sd_rca);
    if(sd_err) {
    	return sd_err;
    }

    if( sd_status(SR_DAT_INHIBIT) ) {
    	return SD_TIMEOUT;
    }

    mmio_write(EMMC_BLKSIZECNT, (1<<16) | 8);
    sd_cmd(CMD_SEND_SCR, 0);
    if(sd_err) {
    	return sd_err;
    }
    if( sd_int(INT_READ_RDY) ) {
    	return SD_TIMEOUT;
    }

    r = 0;
    cnt = 100000;
    while(r < 2 && cnt) {
        if( mmio_read(EMMC_STATUS) & SR_READ_AVAILABLE ) {
            sd_scr[r++] = mmio_read(EMMC_DATA);
        } else {
            wait_msec(1);
        }
    }
    if(r != 2) {
    	return SD_TIMEOUT;
    }
    if(sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
        sd_cmd(CMD_SET_BUS_WIDTH, sd_rca | 2);
        if(sd_err) {
        	return sd_err;
        }
        mmio_write(EMMC_CONTROL0, mmio_read(EMMC_CONTROL0) | C0_HCTL_DWITDH);
    }

    // add software flag
    print("EMMC: supports ");
    if(sd_scr[0] & SCR_SUPP_SET_BLKCNT) {
        print("SET_BLKCNT ");
    }
    if(ccs) {
        print("CCS ");
    }
    print("\n");
    sd_scr[0] &= ~SCR_SUPP_CCS;
    sd_scr[0] |= ccs;
    return SD_OK;
}