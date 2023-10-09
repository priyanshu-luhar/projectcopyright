//this is carlos's individual source file.

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
#include "fonts.h"
#include "csandoval.h"

extern Global gl;
extern X11_wrapper x11;


void handleMenu() {
    int menuChoice = 0;
    bool inMenu = true;

    while (inMenu) {
        glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glColor3f(1.0, 1.0, 1.0);
        Rect r;
        r.center = 0;
        r.bot = gl.yres - 20;
        r.left = 10;

        // Display menu options
        ggprint8b(&r, 16, 0x00ffffff, "Main Menu");
        r.bot -= 40;
        if (menuChoice == 0)
            ggprint8b(&r, 16, 0x00ff0000, "> Start Game");
        else
            ggprint8b(&r, 16, 0x00ffffff, "Start Game");

        r.bot -= 20;
        if (menuChoice == 1)
            ggprint8b(&r, 16, 0x00ff0000, "> Options");
        else
            ggprint8b(&r, 16, 0x00ffffff, "Options");

        r.bot -= 20;
        if (menuChoice == 2)
            ggprint8b(&r, 16, 0x00ff0000, "> Quit");
        else
            ggprint8b(&r, 16, 0x00ffffff, "Quit");

        glPopMatrix();
        x11.swapBuffers();

        // Handle menu input
        XEvent e;
        while (XCheckWindowEvent(x11.dpy, x11.win, KeyPressMask, &e)) {
            if (e.type == KeyPress) {
                if (e.xkey.keycode == 39) {
                    // 'S' key to select menu option
                    if (menuChoice == 0) {
                        inMenu = false;  // Start the game
                    } else if (menuChoice == 1) {
                        // Open Options menu (customize this part)
                        // Example: inOptions = true;
                    } else if (menuChoice == 2) {
                        // Quit the game
                        exit(0);
                    }
                } else if (e.xkey.keycode == 111) {
                    // Up arrow key
                    menuChoice = (menuChoice + 2) % 3;
                } else if (e.xkey.keycode == 116) {
                    // Down arrow key
                    menuChoice = (menuChoice + 1) % 3;
                }
            }
        }
    }
}


void nightmodefilter(int xres, int yres)
{
	//applies the "night-time-esque" filter on screen

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0.6f, .6f, 1.0f, 0.3f);
	glPushMatrix();

	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(0, 0);
    glVertex2f(0, yres);
    glVertex2f(xres, 0);
    glVertex2f(xres, yres);
	
	glEnd();
	glPopMatrix();
}

void display_border(int xres, int yres)
	{
	// draw border around game.
	int b =5;
	glColor3f(1.0f, 1.0f, 0.0f);
	glPushMatrix();

	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(0,0);
	glVertex2f(0+b, 0+b);
	glVertex2f(0, 0+yres);
	glVertex2f(0+b, 0+yres-b);
	glVertex2f(xres, 0+yres);
	glVertex2f(xres-b, 0+yres-b);
	glVertex2f(xres, 0);
	glVertex2f(xres-b, b);
	glVertex2f(0, 0);
	glVertex2f(0+b, 0+b);

	glEnd();
	glPopMatrix();
}

void display_name(int x, int y)
{
	// Show's the name listed in  ggprint8b on the side of the screen
	Rect r;
	r.bot = y;
	r.left = x;
	r.center = 0;
    ggprint8b(&r, 0, 0xFFFFFFF, "Carlos");
}	