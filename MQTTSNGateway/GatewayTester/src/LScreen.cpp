/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

LScreen::LScreen()
{
	_hight = 0;
	_width = 0;
	fcntl(0, F_SETFL, O_NONBLOCK );
}

LScreen::~LScreen()
{

}

void LScreen::getSize(void)
{
	struct winsize wsize ;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);
	_width = wsize.ws_col ? wsize.ws_col : 132;
	_hight = wsize.ws_row ? wsize.ws_row : 10;
}

void LScreen::clear(void)
{
	getSize();
	printf("\033[2J");
	reprompt();

}

void  LScreen::display(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vdisplay(format, args);
	va_end(args);
	reprompt();
}

void  LScreen::vdisplay(const char* format, va_list args)
{
	vsprintf(_buffer, format, args);
	fprintf(stdout, "\033[%d;%dH\033[2K", _hight, 1);
	fprintf(stdout,"%s", _buffer);
	reprompt();
}

void  LScreen::prompt(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vprompt(format, args);
	va_end(args);
}

void  LScreen::vprompt(const char* format, va_list args)
{
	getSize();
	int pos = 0;
	string fmt = format;

	if ( ( pos = fmt.find("\n")) > 0 )
	{
		fmt.replace(pos, 1, " ");
	}

	vsprintf(_buffer, format, args);
	_prompt = _buffer;
	reprompt();
}

void  LScreen::reprompt(void)
{
	int len = 0;
	if ( (len =_prompt.size()) >= _width )
	{
		len = _width - 1;
	}
	fprintf(stdout,"\033[%d;%dH", _hight, 1);
	fprintf(stdout,"\033[0;33m%s\033[0K\033[0;37m", _prompt.substr(0, len).c_str());
	fflush(stdout);
}


bool LScreen::checkKeyIn(char* val)
{
	int c = 0;
	int cprev = 0;

	while  ( read(0, &c, 1) == 1 )
	{
		if ( c == '\n' )
		{
			*val = cprev;
			fprintf(stdout, "\033[1T");
			fflush(stdout);
			reprompt();
			return true;
		}
		cprev = c;
	}
	return false;
}

