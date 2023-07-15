/*
 * FONTS.h
 *
 *  Created on: Nov 15, 2020
 *      Author: tochk
 */

#ifndef FONTS_FONTS_H_
#define FONTS_FONTS_H_


//#include "FONT_SimSun_17_10.h"


//uint16_t *fontConsole(font_SimSun_17_10);


typedef struct{
	uint16_t *font;
	uint16_t width;
	uint16_t height;
} Font_TypeDef;

//Font_TypeDef fontConsole = {font_SimSun_17_10, FONT_SIMSUN_17_10_WIDTH, FONT_SIMSUN_17_10_HEIGHT};


#endif /* FONTS_FONTS_H_ */
