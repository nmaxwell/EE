/*
 * Demo application for libsub
 * Copyright (C) 2008-2009 Alex Kholodenko <sub20@xdimax.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
*-------------------------------------------------------------------
* Description: SUB-20 command line application
*-------------------------------------------------------------------
* Version: $Id: sub_app.c,v 1.58 2010-08-15 19:48:12 avr32 Exp $
*-------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "libsub.h"

#include "cmd_pars.h"
#include "sub_app.h"


#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

/*
*-------------------------------------------------------------------------------
* Constants
*-------------------------------------------------------------------------------
*/

/*
*-------------------------------------------------------------------------------
* Types
*-------------------------------------------------------------------------------
*/

/*
*-------------------------------------------------------------------------------
* Global Variables
*-------------------------------------------------------------------------------
*/
sub_handle* fd = NULL ;

/*
*-------------------------------------------------------------------------------
* Local Function Definition
*-------------------------------------------------------------------------------
*/
void hex_scan( char* buf_in, char* buf_out, int* sz );
void hex_ascii_dump( char* buf, int sz );
void adc_printf( int adc, int mux );


/*
*-------------------------------------------------------------------------------
* START
*-------------------------------------------------------------------------------
*/
int main( int argc, char* argv[] )
{
    int     rc=0,i,tmp,cmd;
	sub_int32_t tmp1;
	struct usb_device* dev;

    /* Init */
    config.fpwm_duty[0]=config.fpwm_duty[1]=config.fpwm_duty[2]=-1;
    for( i=0; i<8; i++ )
        config.pwm_duty[i] = -1;

	config.serno=-1;	/* by default open first available device */
    config.bb_i2c_ch=-1;
    config.bb_i2c_mode=BB_I2C_STD;

    /* Parse command line */
    rc=parse_argv( argc, argv, OPTIONS_N );
    if( rc )
        return rc;

	dev = NULL;
	if( config.serno != -1 )
	{
		char sn_buf[20];
		int found = 0 ;
		while( ( dev =  sub_find_devices( dev ) ) )
		{
			fd = sub_open( dev );
			if(!fd)
			{
				printf("ERROR: Cannot open handle to device\n" );
				return -1;
			}

            rc = sub_get_serial_number( fd, sn_buf, sizeof(sn_buf) );

			sub_close( fd );

			if(rc<0)
			{
				printf("ERROR: Cannot obtain seral number: %d\n", rc );
				return -1;
			}

			if( config.serno == strtoul(sn_buf, NULL, 16 ) )
			{
				/* device found */
				found = 1;
				break;
			}
		}

		if( !found )
		{
			printf("ERROR: Device with SN=0x%04x not found\n", config.serno );
			return -1;
		}
	}

	/* Open USB device */
	fd = sub_open( dev );

	if( !fd )
	{
		printf("sub_open: %s\n", sub_strerror(sub_errno));
		return -1;
	}    


    /*
    * Command
    */
    if( config.cmd_sz )
    {
        rc = usb_transaction( fd, 
                    config.cmd, config.cmd_sz, 
                    config.cmd, config.resp_sz,
                    config.usb_to_ms );
        if( rc<0 )
        {
            printf( "Transaction failed (%s)\n", sub_strerror( sub_errno ) );
        }
        rc=0;
    }

    /*
    * todo
    */
    for( cmd=0; cmd<config.todo_sz; cmd++ )
    {
        switch( config.todo[cmd] )
        {
        /* FIFO */
        case 'a':
            rc = sub_fifo_read( fd, config.buf, config.sz, 10000 );
            if( rc >= 0 )
            {
                printf("%d byte(s) read from FIFO\n", rc );
                hex_ascii_dump( config.buf, rc );
                rc = 0;
            }
            break;

        case 'A':
            rc = sub_fifo_write( fd, config.buf, config.sz, 10000 );
            if( rc >=0 )
            {
                printf("%d byte(s) written to FIFO\n", rc );
                rc = 0;
            }
            break; 

        case 'B':
            rc = sub_control_request( fd, 0x40, 0x80, 0, 0, 0, 0, 1000 );
            if( rc < 0 )
                printf("Control Request failed\n");
            else
                printf("Device in Boot Mode\n");
            return rc; 
        case 'b':
            rc = sub_i2c_freq( fd, &config.i2c_freq );
            if( !rc )
#if (_MSC_VER==800)
				printf("I2C Frequency: %ld\n", config.i2c_freq);
#else
				printf("I2C Frequency: %d\n", config.i2c_freq);
#endif
            break;

            /* FIFO Config */
        case 'c':
            rc = sub_fifo_config( fd, config.fifo_cfg );
            break;

        case 'C':
            rc = sub_bb_i2c_config( fd, config.bb_i2c_mode, 
                                                config.bb_i2c_stretch );
            break;

            /* I2C Config */
        case 'd':
            rc = sub_i2c_config( fd, config.i2c_sa, 0x00 );
            break;

        case 'D':
            /* GPIO Config */
            rc = sub_gpio_config( fd, config.gpio_val, &tmp1, config.gpio_mask );
            if( !rc )
            {
                printf("GPIO Config: val=%08X,mask=%08X\n",
                                    config.gpio_val, config.gpio_mask);
                printf("GPIO Config: %08X\n", tmp1 ); 
            }
            break;
        case 'E':
            /* GPIOB Config */
            rc = sub_gpiob_config( fd, config.gpiob_val, &tmp1, config.gpiob_mask );
            if( !rc )
            {
                printf("GPIOB Config: val=%02X,mask=%02X\n",
                                    config.gpiob_val, config.gpiob_mask);
                printf("GPIOB Config: %02X\n", tmp1 ); 
            }
            break;

        case 'f':
            rc = sub_spi_config( fd, config.spi_config, 0 );
            break;

        case 'G':
            rc = sub_spi_config( fd, 0, &tmp );
            if( !rc )
                printf("SPI Config: %02xh\n", tmp);
            break;
    
        case 'I':
            /* GPIO Read */
            rc = sub_gpio_read( fd, &tmp1 );
            if( !rc )
                printf("GPIO: %08X\n", tmp1 );
            break;
        case 'e':
            /* GPIOB Read */
            rc = sub_gpiob_read( fd, &tmp1 );
            if( !rc )
                printf("GPIOB: %02X\n", tmp1 );
            break;

        case 'i':
        {
            char id_buf[30];
            rc = sub_get_product_id( fd, id_buf, 30 );
            if( rc )
                printf("Product ID: %s\n", id_buf);
            rc = !rc;
        }    
            break; 

        case 'K':
            /* PWM */
            rc = sub_pwm_config( fd, config.pwm_res, config.pwm_limit );
            break;
        case 'k':
            for( i=0; i<8; i++ )
            {
                if( config.pwm_duty[i] != -1 )
                {
                    printf( "PWM%d: duty=%d\n", i, config.pwm_duty[i] );
                    rc = sub_pwm_set( fd, i, config.pwm_duty[i] );
                    config.pwm_duty[i] = -1;
                }
            }
            break;

        case 'L':
            /* LCD Write */
            rc = sub_lcd_write( fd, config.lcd_str );
            break;
            
        case 'M':
            /* Fast PWM */
            printf("Fast PWM frequency: %lf Hz\n", config.fpwm_freq); 
            rc = sub_fpwm_config( fd, config.fpwm_freq, config.fpwm_flags );
            break;
        case 'm':
            for( i=0; i<3; i++ )
            {    
                if( config.fpwm_duty[i] != -1 )
                {
                    printf( "Fast PWM%d: %lf%%\n", i,config.fpwm_duty[i] );
                    rc = sub_fpwm_set( fd, i, config.fpwm_duty[i] );
                    config.fpwm_duty[i] = -1;
                    break;
                }
            }
            break;
        
        case 'N':
        {
            char    scan_buf[128];
            int     n_slave;   
            if( config.bb_i2c_ch != -1 )
                rc = sub_bb_i2c_scan( fd, config.bb_i2c_ch, 
                                                &n_slave, scan_buf );
            else
                rc = sub_i2c_scan( fd, &n_slave, scan_buf );
            if( !rc )
            {
                if( config.bb_i2c_ch != -1 )
                    printf( "I2Cx%d ",config.bb_i2c_ch );
                printf("I2C Slaves(%d): ",n_slave); 
                for( i=0; i<n_slave; i++ ) 
                    printf("0x%02X ", scan_buf[i] );
                printf("\n");
            }
        }
            break;

        case 'O':
            /* GPIO Write */
            rc = sub_gpio_write( fd, config.gpio_val, &tmp1, config.gpio_mask );
            if( !rc )
            {
                printf("GPIO Write: val=%08X,mask=%08X\n",
                                    config.gpio_val, config.gpio_mask);
                printf("GPIO: %08X\n", tmp1 ); 
            }
            break;
        case 'n':
            /* GPIOB Write */
            rc = sub_gpiob_write( fd, config.gpiob_val, &tmp1, config.gpiob_mask );
            if( !rc )
            {
                printf("GPIOB Write: val=%02X,mask=%02X\n",
                                    config.gpiob_val, config.gpiob_mask);
                printf("GPIOB: %02X\n", tmp1 ); 
            }
            break;

        case 'o':
            /* mdio */
            if( (config.mdio_n == 1) && !config.mdio_channel )
            {
                if( config.mdios[0].clause22.op & SUB_MDIO22 )
                    rc = sub_mdio22( fd, 
                                config.mdios[0].clause22.op, 
                                config.mdios[0].clause22.phyad,
                                config.mdios[0].clause22.regad,
                                config.mdios[0].clause22.data,
                                &config.mdios[0].clause22.data );
                else
                    rc = sub_mdio45( fd, 
                                config.mdios[0].clause45.op, 
                                config.mdios[0].clause45.prtad,
                                config.mdios[0].clause45.devad,
                                config.mdios[0].clause45.data,
                                &config.mdios[0].clause45.data );
            }
            else if( config.mdio_n >= 1 )
            {
                if( config.mdio_channel )
                    rc = sub_mdio_xfer_ex( fd, config.mdio_channel,
                                            config.mdio_n, config.mdios );
                else
                    rc = sub_mdio_xfer( fd, config.mdio_n, config.mdios );
            }
            else
                break;

            if( !rc )
            for( i=0; i<config.mdio_n; i++ )
                printf( "MDIO[%d] data=0x%04X\n", 
                        i, config.mdios[i].clause45.data );

            config.mdio_n=0;
            break;

        case 'P':
            rc = sub_i2c_stop( fd );
            break;
        case 'R':
            if( config.bb_i2c_ch != -1 )
                rc = sub_bb_i2c_read( fd, config.bb_i2c_ch, config.sa, 
                            config.ma, config.ma_sz, config.buf, config.sz );
            else
                rc = sub_i2c_read( fd, config.sa, 
                            config.ma, config.ma_sz, config.buf, config.sz );
            if( !rc )
                hex_ascii_dump( config.buf, config.sz );
            break;
        case 'r':
            if( config.rs_baud )
                rc=sub_rs_set_config( fd, config.rs_config, config.rs_baud );
            else
                rc=sub_rs_get_config( fd,&config.rs_config,&config.rs_baud );
            if( !rc )
                printf( "RS Config:0x%02X Baud=%d\n", 
                                        config.rs_config, config.rs_baud );
            break;
        case 'S':
            rc = sub_i2c_start( fd );
            break;
        case 's':
        {
            char sn_buf[20];
            rc = sub_get_serial_number( fd, sn_buf, 20 );
            if( rc )
                printf("Serial number: %s\n", sn_buf);
            rc = !rc;
        }    
            break; 

        case 't':
            /* SPI Read */
            rc = sub_spi_transfer( fd, 0, config.buf, 
                                                config.spi_in_sz, config.ss );
            if( !rc )
                hex_ascii_dump( config.buf, config.sz );
            break;
        case 'T':
            /* SPI Write */
            rc = sub_spi_transfer( fd, config.buf, config.buf, 
                                                config.spi_in_sz, config.ss );
            if( !rc )
                hex_ascii_dump( config.buf, config.spi_in_sz );
            break;
        case 'U':
            /* SPI SDIO */
            rc = sub_sdio_transfer( fd, config.buf, config.buf, 
                            config.spi_out_sz, config.spi_in_sz, config.ss );
            if( !rc && config.spi_in_sz )
                hex_ascii_dump( config.buf, config.spi_in_sz );
            break;

        case 'u':
            /* Set RS Timing */
            rc = sub_rs_timing( fd, config.rs_flags, config.rs_tx_space, 
                                config.rs_rx_msg_to, config.rs_rx_byte_to );
            break;

        case 'V':
        {
            const struct sub_version* sub_ver;
            sub_ver = sub_get_version( fd );
            printf("DLL/Lib: %d.%d.%d.%d\n", 
                sub_ver->dll.major,sub_ver->dll.minor,
                sub_ver->dll.micro,sub_ver->dll.nano);
            printf("Driver : %d.%d.%d.%d\n",
                sub_ver->driver.major,sub_ver->driver.minor,
                sub_ver->driver.micro,sub_ver->driver.nano);
            printf("SUB-20 : %d.%d.%d\n",
                sub_ver->sub_device.major,
                sub_ver->sub_device.minor,
                sub_ver->sub_device.micro);
            printf("Boot   : %x.%x\n",
                sub_ver->sub_device.boot>>4,
                sub_ver->sub_device.boot&0x0F);
        }
            break;

        case 'v':
        {
            /* Get boot_cfg.vpd */
            const struct sub_cfg_vpd* cfg_vpd;
            cfg_vpd = sub_get_cfg_vpd( fd );
            if( !cfg_vpd )
                rc = -1;
            else
            {
                printf("LCD Width  : %d\n", cfg_vpd->lcd_w );
                printf("LCD Lines  : %d\n", cfg_vpd->lcd_h );
                printf("Serial     : %d\n", cfg_vpd->serial);
                printf("Buttons    : %d\n", cfg_vpd->buttons);
                printf("IR Carrier : %d\n", cfg_vpd->ir_car);
                rc = 0;
            }
        }
            break;
            

        case 'W':
            if( config.bb_i2c_ch != -1 )
                rc = sub_bb_i2c_write( fd, config.bb_i2c_ch, config.sa, 
                            config.ma, config.ma_sz, config.buf, config.sz );
            else
                rc = sub_i2c_write( fd, config.sa, 
                            config.ma, config.ma_sz, config.buf, config.sz );
            break;

        case 'X':
            /* RS Xfer */
            rc = sub_rs_xfer( fd, 
                config.buf, config.rs_tx_sz, config.buf, config.rs_rx_sz );
            if( rc >= 0 )
            {
                printf( "Resecived %d bytes\n", rc );
                hex_ascii_dump( config.buf, rc );
                rc = 0;
            }
            break;
       
        case 'Z':
            /* ADC Config */
            rc = sub_adc_config( fd, config.adc_cfg );
            break;

        case 'z':
            /* ADC Read */
        {
            int ibuf[30];    
            rc = sub_adc_read( fd, ibuf, config.adc_mux, config.adc_reads );
            if( !rc )
            {
                printf( 
"Analog Input   ADC           Vref=5.0V Vref=3.3V Vref=2.56V\n" ); 
                for( i=0; i<config.adc_reads; i++ )
                    adc_printf( ibuf[i], config.adc_mux[i] );
            }
        }
            break;
             
        }/*end case*/

        if( rc )
            break;
    }

	if( rc )
    {
        printf("Failed(rc=%d): %s\n", rc,sub_strerror(sub_errno));
        if( rc == SE_I2C )
            printf("I2C Status: %02x\n", sub_i2c_status );
        
    }
    sub_close( fd );
	return rc;
}


/*
*-----------------------------------------------------------------------------
*                               process_arg
*-----------------------------------------------------------------------------
* Dscr: process argument  
*-----------------------------------------------------------------------------
* Input : option index, equ_ptr - pointer to option value
* Output: 0 - continue
*         <0 - error code to exit 
*         >0 - code to exit
* Notes : call exit(1) in case of error in argument
*-----------------------------------------------------------------------------
*/
int process_arg( int opt_index, char* equ_ptr )
{
    int     rc=0;
    int     i,sz;

    /*
    * process argument
    */
    switch ( usage_descriptor[opt_index].case_code )
    {
    case HELP_CMD_CASE:
        usage( 1, OPTIONS_N );
        return 1;

    case VER_CMD_CASE:
        XPRINT_VERSION;
        config.todo[config.todo_sz++]='V';
        break;

	case SERNO_CMD_CASE:
		config.serno = strtoul(equ_ptr, NULL, 0);
		break;

	case STOP_CMD_CASE:
        config.todo[config.todo_sz++]='P';
        break;

    case START_CMD_CASE:
        config.todo[config.todo_sz++]='S';
        break;

    case CMD_CMD_CASE:
        hex_scan( equ_ptr, config.cmd, &config.cmd_sz );
        equ_ptr+=(config.cmd_sz*3-1);
        if( *equ_ptr != ',' )
        {
            printf("RESP_SIZE missing\n");
            return -1;
        }        
        sscanf( equ_ptr+1,"%d", &config.resp_sz );
        break;

    case I2C_CMD_CASE:
    {
        char hex_buf[MAX_CMD*3];
        i = sscanf( equ_ptr, "%x,%x,%x,%d,%768c",
                &config.sa, &config.ma_sz, &config.ma, &config.sz, hex_buf);
        if( i == 5 )
        {
            config.todo[config.todo_sz++]='W';
            hex_scan( hex_buf, config.buf, &sz );
            if( sz != config.sz )
            {
                printf("HEX size=%d mismatch in i2c option\n", sz);
                return -1;
            }
        }
        else if( i == 4 )
        {
            config.todo[config.todo_sz++]='R';
        }
        else
        {
            printf("I2C Command incomplete\n");
            return -1;
        }
    }
        break;

    case SPI_CMD_CASE:
    {
        char hex_buf[MAX_CMD*3];
        char cmd=0;
        if( (equ_ptr[0]=='i') || (equ_ptr[0]=='I') )
        {
            equ_ptr++;
            cmd = 'U';
        }     
        i = sscanf( equ_ptr, "%d,%768c", &config.spi_in_sz, hex_buf);
        if( i == 2 )
        {
            if( !cmd )
                cmd = 'T';
            if( !strcmp(hex_buf,"...") )
                config.spi_out_sz = config.spi_in_sz;
            else
            {
                hex_scan( hex_buf, config.buf, &sz );
                if( (sz != config.spi_in_sz) && (cmd=='T') )
                {
                    printf("HEX size=%d mismatch in spi option\n", sz);
                    return -1;
                }
                config.spi_out_sz = sz;
            }
        }
        else 
        {
            if( !cmd )
                cmd = 't';
            config.spi_out_sz=0;
        }
        config.todo[config.todo_sz++]=cmd;
    }
        break;

    case SPI_SS_CMD_CASE:
        i = equ_ptr[0]-'0';
        if( i>15 )
        {
            printf("SS number %d does not exist\n",i);
            return -1;
        }
        if( !strcasecmp("h",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_H);
        else if( !strcasecmp("hl",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HL);
        else if( !strcasecmp("hhl",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HHL);
        else if( !strcasecmp("hhhl",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HHHL);
        else if( !strcasecmp("hhhhl",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HHHHL);
        else if( !strcasecmp("hi",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HI);
        else if( !strcasecmp("l",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_L);
        else if( !strcasecmp("lh",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_LH);
        else if( !strcasecmp("llh",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_LLH);
        else if( !strcasecmp("lllh",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_LLLH);
        else if( !strcasecmp("llllh",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_LLLLH);
        else if( !strcasecmp("lo",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_LO);
        else if( !strcasecmp("hiz",equ_ptr+2) )
            config.ss = SS_CONF(i,SS_HiZ);
        else if( !strcasecmp("0",equ_ptr+2) )
            config.ss = SS_CONF(i,0);
        else
        {
            printf("Unknown SS configuration\n");
            return -1;
        }
        break; 

    case I2C_CFG_CMD_CASE:
        switch( equ_ptr[0] )
        {
        case 'f':
        case 'F':
            config.i2c_freq = strtoul(equ_ptr+1,0,0);
            config.todo[config.todo_sz++]='b';
            break;
        case 's':
        case 'S':
            config.i2c_sa = strtoul(equ_ptr+1,0,0);
            config.todo[config.todo_sz++]='d';
            break;
        case 'm':
        case 'M':
            config.bb_i2c_mode = strtoul(equ_ptr+1,0,0);
            config.todo[config.todo_sz++]='C';
            break; 
        case 'c':
        case 'C':
            config.bb_i2c_stretch = strtoul(equ_ptr+1,0,0);
            config.todo[config.todo_sz++]='C';
            break;
        default:
            printf( "Unknown parameter `%c' in i2c config\n", equ_ptr[0] );
            return -1;
        }
        break;

    case I2C_CH_CMD_CASE:
        config.bb_i2c_ch = strtoul(equ_ptr,0,0);
        break;

    case SCAN_CMD_CASE:
        config.todo[config.todo_sz++]='N';
        break;

    case SPI_CFG_CMD_CASE:
        config.spi_config = 0;
        while( *equ_ptr )
        {
            switch( *equ_ptr )
            {
            case 'g':
            case 'G':
                config.todo[config.todo_sz++]='G';
                goto xx1;
            case 'e':
            case 'E':
                config.spi_config|=SPI_ENABLE;
                break;
            case 's':
            case 'S':
                config.spi_config|=SPI_SLAVE;
                break;
            case 'P':
            case 'p':
                config.spi_config|=SPI_SMPL_SETUP;
                break;
            case 't':
            case 'T':
                config.spi_config|=SPI_SETUP_SMPL;
                break;
            case 'r':
            case 'R':
                config.spi_config|=SPI_CPOL_RISE;
                break;
            case 'f':
            case 'F':
                config.spi_config|=SPI_CPOL_FALL;
                break;
            case 'm':
            case 'M':
                config.spi_config|=SPI_MSB_FIRST;
                break;
            case 'l':
            case 'L':
                config.spi_config|=SPI_LSB_FIRST;
                break;
            case '8':
                config.spi_config|=SPI_CLK_8MHZ;
                break;
            case '4':
                config.spi_config|=SPI_CLK_4MHZ;
                break;
            case '2':
                config.spi_config|=SPI_CLK_2MHZ;
                break;
            case '1':
                config.spi_config|=SPI_CLK_1MHZ;
                break;
            case '5':
                config.spi_config|=SPI_CLK_500KHZ;
                break;
            case '6':
                config.spi_config|=SPI_CLK_250KHZ;
                break;
            case '7':
                config.spi_config|=SPI_CLK_125KHZ;
                break;
            }
            equ_ptr++;
        }         
        config.todo[config.todo_sz++]='f'; 
xx1:
        break;

    case GPIO_CMD_CASE:
        switch( equ_ptr[0] )
        {
        case 'B':
        case 'b':
            switch( equ_ptr[1] )
            {
            case 'R':
            case 'r':
                config.todo[config.todo_sz++]='e'; 
                break;
            case 'W':
            case 'w':
                config.todo[config.todo_sz++]='n'; 
                break;
            case 'C':
            case 'c':
                config.todo[config.todo_sz++]='E'; 
                break;
            default:
                printf("Wrong GPIOB function\n");
                return -1;
            }
            sscanf( equ_ptr+2, ",%x,%x", &config.gpiob_val, &config.gpiob_mask );
            break;
        case 'R':
        case 'r':
            config.todo[config.todo_sz++]='I'; 
            break;
        case 'W':
        case 'w':
            config.todo[config.todo_sz++]='O'; 
            break;
        case 'C':
        case 'c':
            config.todo[config.todo_sz++]='D'; 
            break;
        default:
            printf("Wrong GPIO function\n");
            return -1;
        }
        sscanf( equ_ptr+1, ",%x,%x", &config.gpio_val, &config.gpio_mask );
        break;
            
    case RPT_CMD_CASE:
        sz = strtoul(equ_ptr,0,0);
        for( i=0; i<sz; i++ )
        {
            config.todo[config.todo_sz]=config.todo[config.todo_sz-1];
            config.todo_sz++;
        }
        break;

    case GETSN_CMD_CASE:
        config.todo[config.todo_sz++]='s';
        break;
    case GETID_CMD_CASE:
        config.todo[config.todo_sz++]='i';
        break;
    case BOOT_CMD_CASE:
        config.todo[config.todo_sz++]='B';
        break;
    case GETVPD_CMD_CASE:
        config.todo[config.todo_sz++]='v';
        break;

    case FIFO_CMD_CASE:
    {
        char *p;
        sscanf( equ_ptr, "%d", &config.sz );
        p = strchr( equ_ptr,',' );
        if( p )
        {
            hex_scan( p+1, config.buf, 0 );
            config.todo[config.todo_sz++]='A';
        }
        else
            config.todo[config.todo_sz++]='a';
     }   
        break;
    case FIFO_CFG_CMD_CASE:
        while( *equ_ptr )
        {
            switch( *equ_ptr )
            {
            case 's':
            case 'S':
                config.fifo_cfg |= FIFO_SELECT_SPI;
                break;
            case 'u':
            case 'U':
                config.fifo_cfg |= FIFO_SELECT_UART;
                break;
            case 'c':
            case 'C':
                config.fifo_cfg |= FIFO_CLEAR;
                break;
            case 'i':
            case 'I':
                config.fifo_cfg |= FIFO_SELECT_I2C;
                break;
            default:
                printf("Unknown FIFO Config parameter `%c'\n", *equ_ptr);
                return -1;
            }
            equ_ptr++;
        }    
        config.todo[config.todo_sz++]='c';
        break;

    case LCDWR_CMD_CASE:
    {
        int i=0,j=0;
        config.lcd_str = equ_ptr;

        /* Parse LCD String */
        while( config.lcd_str[i] )
        {
            if( config.lcd_str[i] == '\\' )
            {
                switch( config.lcd_str[i+1] )
                {
                case 'n':
                    config.lcd_str[j] = '\n';
                    i++;
                    break;
                case 'f':
                    config.lcd_str[j] = '\f';
                    i++;
                    break;
                case 'r':
                    config.lcd_str[j] = '\r';
                    i++;
                    break;
                case 'e':
                    config.lcd_str[j++]=0x1b;
                    config.lcd_str[j++]=config.lcd_str[i+2];
                    config.lcd_str[j]  =config.lcd_str[i+3];
                    i+=3;
                    break;
                default:
                    config.lcd_str[j] = '\\';
                }
            }
            else
                config.lcd_str[j] = config.lcd_str[i];
            j++;
            i++;
        }
        config.lcd_str[j] = 0x00;
        config.todo[config.todo_sz++]='L';
    }
        break;

    case RS_CFG_CMD_CASE:
    {
        char* p = equ_ptr;

        config.todo[config.todo_sz++]='r'; 
        if( *p == '0' )
        {
            config.rs_baud = 0;
            break;
        }
        while( *p )
        {
            if( *p == '_' )
                break;
            p++;
        }
        if( !*p )
        {
            printf( "Wrong RS Config parameter\n" ); 
            return -1;
        }
        *p=0;
        
        config.rs_baud = atoi( equ_ptr );
        p++;
        switch( *p )
        {
        case '5':
            config.rs_config |= RS_CHAR_5;
            break;
        case '6':
            config.rs_config |= RS_CHAR_6;
            break;
        case '7':
            config.rs_config |= RS_CHAR_7;
            break;
        case '8':
            config.rs_config |= RS_CHAR_8;
            break;
        case '9':
            config.rs_config |= RS_CHAR_9;
            break;
        default:
            printf( "Wrong character size in RS Config\n" );
            return -1;
        }

        p++;
        switch( *p )
        {
        case 'N':
            config.rs_config |= RS_PARITY_NONE;
            break; 
        case 'E':
            config.rs_config |= RS_PARITY_EVEN;
            break; 
        case 'O':
            config.rs_config |= RS_PARITY_ODD;
            break; 
        default:
            printf( "Wrong parity code in RS Config\n" );
            return -1;
        }
        p++;
        switch( *p )
        {
        case '1':
            config.rs_config |= RS_STOP_1;
            break;
        case '2':
            config.rs_config |= RS_STOP_2;
            break;
        default:
            printf( "Wrong stop bits in RS Config\n" );
            return -1;
        }
        config.rs_config |= RS_RX_ENABLE|RS_TX_ENABLE;
    }         
        break;

    case RS_TMG_CMD_CASE:
        if( toupper(equ_ptr[0]) == 'B' )
        {
            config.rs_flags |= RS_RX_BEFORE_TX;
            equ_ptr+=2;
        }
        else if( toupper(equ_ptr[0]) == 'A' )
        {
            config.rs_flags |= RS_RX_AFTER_TX;
            equ_ptr+=2;
        }
        if( sscanf(equ_ptr, "%u,%u,%u", &config.rs_tx_space, 
                    &config.rs_rx_msg_to, &config.rs_rx_byte_to) != 3 )
        {
            printf("Bad RS Timing configuration\n" );
            return -1;
        }
        config.todo[config.todo_sz++]='u';
        break;
        
    case RS_XFER_CMD_CASE:
    {
        char hex_buf[MAX_CMD*3];
        i = sscanf( equ_ptr, "%d,%768c", &config.rs_rx_sz, hex_buf);
        if( i == 2 )
        {
            hex_scan( hex_buf, config.buf, &config.rs_tx_sz );
        }
    }
        config.todo[config.todo_sz++]='X';
        break;

    case FPWM_CMD_CASE:
        config.fpwm_flags |= FPWM_ENABLE;
        if( (equ_ptr[0]=='f') || (equ_ptr[0]=='F') )
        {
            if( sscanf( equ_ptr+1, "%lf", &config.fpwm_freq ) != 1 )
            {
                printf("Specify fast PWM frequency as --fpwm=F<FREQ_HZ>\n");
                return -1;
            }
            if( config.fpwm_freq == 0 )
                config.fpwm_flags &= ~FPWM_ENABLE;
            config.todo[config.todo_sz++]='M';
        } 
        else
        {
            int pwm_n = equ_ptr[0]-'0';
            equ_ptr++;
            if( (pwm_n<0) || (pwm_n>2) )
            {
                printf("Fast PWM number must be 0,1,2\n");
                return -1;
            }
            config.fpwm_flags |= FPWM_EN0>>(pwm_n*2);
            if( sscanf(equ_ptr, ",%lf", &config.fpwm_duty[pwm_n]) != 1 )
            {
                printf("Fast PWM set format is --fpwm=<N>,<DUTY>\n");
                return -1;
            }
            config.todo[config.todo_sz++]='m';
        }
        break;

    case PWM_CMD_CASE:
        if( (equ_ptr[0]=='c') || (equ_ptr[0]=='C') )
        {
            if( sscanf(equ_ptr+1,"%u,%u",&config.pwm_res,&config.pwm_limit) 
                    != 2 )
            {
                printf("Specify PWM configuration as --pwm=C<RES>,<LIMIT>\n");
                return -1;
            }
            config.todo[config.todo_sz++]='K';
        } 
        else
        {
            int pwm_n = equ_ptr[0]-'0';
            equ_ptr++;
            if( (pwm_n<0) || (pwm_n>7) )
            {
                printf("PWM number must be 0..7\n");
                return -1;
            }
            if( sscanf(equ_ptr, ",%u", &config.pwm_duty[pwm_n]) != 1 )
            {
                printf("PWM set format is --pwm=<N>,<DUTY>\n");
                return -1;
            }
            config.todo[config.todo_sz++]='k';
        }
        break;

    case ADC_CFG_CMD_CASE:
        while( *equ_ptr )
        {
            switch(*equ_ptr)
            {
            case 'e':
            case 'E':
                config.adc_cfg|=ADC_ENABLE;
                break;
            case 'v':
            case 'V':
                config.adc_cfg|=ADC_REF_VCC;
                break;
            case '2':
                config.adc_cfg|=ADC_REF_2_56;
                break;
            default:
                printf("Unknown option `%c' in ADC config\n", *equ_ptr);
                return -1;
            }
            equ_ptr++;
        }
        config.todo[config.todo_sz++]='Z';
        break;

    case ADC_CMD_CASE:
        config.adc_mux[ config.adc_reads++ ] = strtoul( equ_ptr,0,0 );
        if( config.adc_reads == 1 )
            config.todo[config.todo_sz++]='z';
        break;    

    case MDIO_CMD_CASE:
        if( *equ_ptr == '1' )
        {
            config.mdio_channel = 1;
            equ_ptr++;
        }
        switch( *equ_ptr )
        {
        case 'a':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO45_ADDR;
            break;
        case 'w':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO45_WRITE;
            break;
        case 'r':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO45_READ;
            break;
        case 'p':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO45_PRIA;
            break;
        case 'R':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO22_READ;
            break;
        case 'W':
            config.mdios[config.mdio_n].clause45.op = SUB_MDIO22_WRITE;
            break;
        default:
            printf("Unknown mdio operation\n");
            return -1;
        }
        if( sscanf(equ_ptr+1, ",%x,%x,%x", 
                    &config.mdios[config.mdio_n].clause45.prtad,
                    &config.mdios[config.mdio_n].clause45.devad,
                    &config.mdios[config.mdio_n].clause45.data) != 3 )
        {
            printf("Wrong mdio option format\n");
            return -1;
        }
        config.mdio_n++;
        config.todo[config.todo_sz++]='o';
        break;

    }/*switch*/

    return rc;
}

/*
* Convert HEX string to byte array 
*/
void hex_scan( char* buf_in, char* buf_out, int* sz )
{
    int     out_sz=0;

    while( *buf_in )
    {
        sscanf( buf_in, "%02hhx", &buf_out[out_sz++] );
        buf_in+=2;
        if( *buf_in != ' ' )
            break;
        buf_in++;
    }        

    if( sz )
        *sz = out_sz;
}

/* Dump buffer in Hex and ASCII */
void hex_ascii_dump( char* buf, int sz )
{
#define HEX_DUMP_WIDTH  16
    int             i;
    unsigned char   ascii[HEX_DUMP_WIDTH+1];

    for( i=0; i<sz; i++ )
    {
        printf("%02X ", ((unsigned int)buf[i])&0x0FF );
        if( (buf[i] >= 0x20) && ((unsigned char)buf[i] < 0x80) )
            ascii[i%HEX_DUMP_WIDTH] = buf[i];
        else
            ascii[i%HEX_DUMP_WIDTH] = '.';

        if( i%HEX_DUMP_WIDTH == HEX_DUMP_WIDTH-1 )
        {
            ascii[HEX_DUMP_WIDTH] = 0x00;
            printf( "| %s\n", ascii );
        }
    }

    ascii[i%HEX_DUMP_WIDTH] = 0x00;
    if( ascii[0] )
    {
        for( ; i%HEX_DUMP_WIDTH ; i++ )
            printf("   ");
        printf( "| %s\n", ascii );
    }
}

/*
* Print A/D Result
*/
void adc_printf( int adc, int mux )
{
    int gain=1,scale=1023;

    switch( mux )
    {
    case ADC_S0:
        printf("ADC0           ");
        break;
    case ADC_S1:
        printf("ADC1           ");
        break;
    case ADC_S2:
        printf("ADC2           ");
        break;
    case ADC_S3:
        printf("ADC3           ");
        break;
    case ADC_S4:
        printf("ADC4           ");
        break;
    case ADC_S5:
        printf("ADC5           ");
        break;
    case ADC_S6:
        printf("ADC6           ");
        break;
    case ADC_S7:
        printf("ADC7           ");
        break;
    case ADC_D10_10X:
        printf("+ADC1-ADC0x10  ");
        gain = 10;
        scale= 512;
        break;
    case ADC_D10_200X:
        printf("+ADC1-ADC0x200 ");
        gain = 200;
        scale= 512;
        break;
    case ADC_D32_10X: 
        printf("+ADC3-ADC2x10  ");
        gain = 10;
        scale= 512;
        break;
    case ADC_D32_200X:
        printf("+ADC3-ADC2x200 ");
        gain = 200;
        scale= 512;
        break;
    case ADC_D01:     
        printf("+ADC0-ADC1     ");
        scale= 512;
        break;
    case ADC_D21:     
        printf("+ADC2-ADC1     ");
        scale= 512;
        break;
    case ADC_D31:     
        printf("+ADC3-ADC1     ");
        scale= 512;
        break;
    case ADC_D41:     
        printf("+ADC4-ADC1     ");
        scale= 512;
        break;
    case ADC_D51:     
        printf("+ADC5-ADC1     ");
        scale= 512;
        break;
    case ADC_D61:     
        printf("+ADC6-ADC1     ");
        scale= 512;
        break;
    case ADC_D71:     
        printf("+ADC7-ADC1     ");
        scale= 512;
        break;
    case ADC_D02:     
        printf("+ADC0-ADC2     ");
        scale= 512;
        break;
    case ADC_D12:     
        printf("+ADC1-ADC2     ");
        scale= 512;
        break;
    case ADC_D32:     
        printf("+ADC3-ADC2     ");
        scale= 512;
        break;
    case ADC_D42:     
        printf("+ADC4-ADC2     ");
        scale= 512;
        break;
    case ADC_D52:     
        printf("+ADC5-ADC2     ");
        scale= 512;
        break;
    default:
        printf("MUX=%02d         ", mux);
    }    

    printf( "%04d(0x%04x)  %1.4fV   %1.4fV   %1.4fV\n", 
                adc, adc&0xFFFF,
                5.00*adc/scale/gain,
                3.30*adc/scale/gain,
                2.56*adc/scale/gain
    );
}

