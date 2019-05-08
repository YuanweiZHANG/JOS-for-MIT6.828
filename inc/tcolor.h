/* This file is for text color defination 
 * Created by Yuanwei ZHANG on 2019.03.04
*/
#ifndef JOS_INC_TCOLOR_H
#define JOS_INC_TCOLOR_H

/* text color */
#define BLACK              0x0000
#define BLUE               0x0100
#define GREEN              0x0200
#define CYAN               0x0300
#define RED                0x0400
#define MAGENTA            0x0500
#define BROWN              0x0600
#define LIGHT_GRAY         0x0700  // Default
#define GRAY               0X0800
#define LIGHT_BLUE         0X0900
#define LIGHT_GREEN        0X0A00
#define LIGHT_CYAN         0X0B00
#define LIGHT_RED          0X0C00
#define LIGHT_MAGENTA      0X0D00
#define YELLOW             0X0E00
#define WHITE              0x0f00

/* screen color */
#define SCREEN_BLACK       0x0000
#define SCREEN_BLUE        0x1000
#define SCREEN_GREEN       0x2000
#define SCREEN_CYAN        0x3000
#define SCREEN_MAGENTA     0x5000
#define SCREEN_RED         0x4000
#define SCREEN_BROWN       0x6000
#define SCREEN_LIGHT_GRAY  0x7000 

/* shining */
#define SHINING            0x8000
#define NO_SHINING         0x0000

extern unsigned int textcolor;

#endif