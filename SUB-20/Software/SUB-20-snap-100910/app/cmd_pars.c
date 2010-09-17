/*
 * Command line parser for libsub application
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
* Version : $Id: cmd_pars.c,v 1.5 2009-09-26 04:51:13 avr32 Exp $
*-------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CMD_PARS_MODULE
 #include "cmd_pars.h"

/*
*-----------------------------------------------------------------------------
* Types Definition
*-----------------------------------------------------------------------------
*/

/*
*-----------------------------------------------------------------------------
* Constants
*-----------------------------------------------------------------------------
*/

/*
*-----------------------------------------------------------------------------
* Global Variables
*-----------------------------------------------------------------------------
*/

/*
*-----------------------------------------------------------------------------
* Global Function Definition
*-----------------------------------------------------------------------------
*/

/*
*-----------------------------------------------------------------------------
* Local Function Definition
*-----------------------------------------------------------------------------
*/

/*
*-----------------------------------------------------------------------------
*								parse_argv
*-----------------------------------------------------------------------------
* Dscr:  parse argv list and call process_arg for each option
*        fill object_name object_argv, object_argc values   
*-----------------------------------------------------------------------------
* Input : argc, argv 
* Output: 0 - continue
*		  <0 - error code to exit 
*         >0 - code to exit
* Notes : call exit(1) in case of cmd line error  
*-----------------------------------------------------------------------------
*/
int parse_argv( int my_argc, char** my_argv, int options_n )
{
	int i,i2,len;
	int rc;
	char* equ_ptr = 0;

	/*
	* Parse command line untill object name found
	*/
	for( i=1; i<my_argc; i++ )
	{
		if( my_argv[i][0] != '-' )
		{
			printf( "Wrong parameter \"%s\"\n", my_argv[i] );
			usage( 0, options_n );
			return -1;
		}

		equ_ptr = 0;

		if( my_argv[i][1] != '-' )	/* option in short form "-x" */
		{
			for( i2=0; i2<options_n; i2++ )
				if( my_argv[i][1] == usage_descriptor[i2].m_option )
				{
                    /* Determine option value */
					if( usage_descriptor[i2].equ_alias[0] != '\0' )
					{
						if( my_argv[i][2] != '\0' )
						{
							equ_ptr = &my_argv[i][2];	/* -xVALUE */
							if( *equ_ptr == '=' )
								equ_ptr++;				/* -x=VALUE */
						}
						else
						{								/* -x VALUE */
							/* Take next argument as value */
							if( (i+1 == my_argc) || (my_argv[i+1][0] == '-') )
							{
								/* This was last argument or next argument starts with '-' */
								printf( "Option %s is not complete\n", my_argv[i] );
								usage( 0, options_n );
								return -1;
							}

							i++;
							equ_ptr = my_argv[i];
						}
					}
					rc = process_arg( i2, equ_ptr );
					if( rc )
						return rc;
					break;
				}
		}
		else	/* option in verbose form "--option" */
		{
			for( i2=0; i2<options_n; i2++ )
			{
				len = (int)strlen( usage_descriptor[i2].mm_option );
				if( !strncmp(usage_descriptor[i2].mm_option, &my_argv[i][2], len) )
				{
					if( (my_argv[i][2+len]=='\0') || (my_argv[i][2+len]=='=') )
					{

						/* Split argument to option=value if needed */

						if( usage_descriptor[i2].equ_alias[0] != '\0' )
						{
							equ_ptr = strchr( my_argv[i], '=' );
							if( !equ_ptr || !strlen( equ_ptr+1 ) )
							{
								/* format option=value not found */
								printf( "Option %s is not complete\n", my_argv[i] );
								usage( 0, options_n );
								return -1;
							}
							equ_ptr++;
						}

						rc = process_arg( i2, equ_ptr );
						if( rc )
							return rc;
						break;
					}
				}
			}
		}

		if( i2 == options_n )
		{
			printf( "Unknown option %s\n", my_argv[i] );
			usage( 0, options_n );
			return -1;
		}
	}/*end for*/

	return 0;
}


/*
*-----------------------------------------------------------------------------
*								usage
*-----------------------------------------------------------------------------
* Dscr:  Output usage message
*-----------------------------------------------------------------------------
* Input : 
* Output:
* Notes :
*-----------------------------------------------------------------------------
*/
void usage( int full_help, int options_n )
{
	int i;
	int cntr;

	if( full_help )
	{
		for( i=0; i<options_n; i++ )
		{
			if( usage_descriptor[i].case_code >= MIN_HIDDEN_CODE )
				continue;			// Skip hidden option
	
			if( usage_descriptor[i].help_desc[0] == '\0' )
				continue;			// Skip options without help context

			if( usage_descriptor[i].m_option != ' ' )
				printf( "-%c, ",usage_descriptor[i].m_option );
			else 
				printf( "    " );

			printf("--");
			cntr = printf( "%.11s", usage_descriptor[i].mm_option );
			if( usage_descriptor[i].equ_alias[0] )
				cntr+=printf( "=%-7s", usage_descriptor[i].equ_alias );

			while( cntr++ < 18 )
				putchar(' ');

			printf( " %s\n" ,usage_descriptor[i].help_desc );  
		}
	}
	else
		printf( "  Use --help|-h to get more information\n" );
}





