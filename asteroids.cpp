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
#include "jhernandez2.h"
#include "pluhar.h"

//defined types
typedef float Flt;
typedef float Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((Flt)rand())/(Flt)RAND_MAX)
#define random(a) (rand()%a)
#define VecZero(v) (v)[0]=0.0,(v)[1]=0.0,(v)[2]=0.0
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
						(c)[1]=(a)[1]-(b)[1]; \
						(c)[2]=(a)[2]-(b)[2]
//constants
const float timeslice = 1.0f;
const float gravity = 2.2f;
#define PI 3.141592653589793
#define ALPHA 1
const int MAX_BULLETS = 4;
const Flt MINIMUM_ASTEROID_SIZE = 40.0;
const float MINIMUM_ASTEROID_DISTANCE = 200.0; // Adjust this value as needed 
//-----------------------------------------------------------------------------
//Setup Timers
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
extern struct timespec timeStart, timeCurrent;
extern struct timespec timePause;
extern double physicsCountdown;
extern double timeSpan;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------
//Function Prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics();
void render();
void checkShipAsteroidCollision();
void displayGameOver();
void displayYouWin();
void moveSmallAsteroidsTowardsShip();
//-----------------------------------------------------------------------------
//Global Variables
Global gl;
X11_wrapper x11(gl.xres, gl.yres);
bool gameWon = false;
bool wasdvar = false;


//-----------------------------------------------------------------------------
//Class Definitions
class Ship {
public:
	Vec pos;
	Vec dir;
	Vec vel;
	Vec acc;
	float angle;
	float color[3];
	float radius; // Add this line
    bool doubleBarrelActive;
public:
	Ship() {
		pos[0] = (Flt)(gl.xres/2);
		pos[1] = (Flt)(gl.yres/2);
		pos[2] = 0.0f;
		VecZero(dir);
		VecZero(vel);
		VecZero(acc);
		angle = 0.0;
		color[0] = color[1] = color[2] = 1.0;
		radius = 6.0f;
	}
};

class Bullet {
public:
	Vec pos;
	Vec vel;
	float color[6];
	struct timespec time;
public:
	Bullet() { }
};

class Asteroid {
public:
	Vec pos;
	Vec vel;
	int nverts;
	Flt radius;
	Vec vert[8];
	float angle;
	
	//float rotate;
	float color[3];
	struct Asteroid *prev;
	struct Asteroid *next;
public:
	Asteroid() {
		prev = NULL;
		next = NULL;
	}
};

class Game {
public:
	Ship ship;
	Asteroid *ahead;
	Bullet *barr;
	int nasteroids;
	int nbullets;
	struct timespec bulletTimer;
	struct timespec mouseThrustTimer;
	bool mouseThrustOn;

public:
	Game() {
    ahead = NULL;
    barr = new Bullet[MAX_BULLETS];
    nasteroids = 0;
    nbullets = 0;
    mouseThrustOn = false;
    for (int j = 0; j < 10; j++) {
        Asteroid *a = new Asteroid;
        a->nverts = 6;
        a->radius = rnd() * 20.0 + 40.0;
        a->angle = 0.0f;
		a->color[0] = 1.0;
		a->color[1] = 1.0;
		a->color[2] = 1.0;

        a->vel[0] = (Flt)(rnd() * 2.0 - 1.0);
        a->vel[1] = (Flt)(rnd() * 2.0 - 1.0);

        // Modify the initialization of asteroid positions
        int side = random(4);
        if (side == 0) {
            // Spawn on the left edge
            a->pos[0] = 0.0f;
            a->pos[1] = rnd() * (float)gl.yres;
        } else if (side == 1) {
            // Spawn on the top edge
            a->pos[0] = rnd() * (float)gl.xres;
            a->pos[1] = (float)gl.yres;
        } else if (side == 2) {
            // Spawn on the right edge
            a->pos[0] = (float)gl.xres;
            a->pos[1] = rnd() * (float)gl.yres;
        } else {
            // Spawn on the bottom edge
            a->pos[0] = rnd() * (float)gl.xres;
            a->pos[1] = 0.0f;
        }

        // Add to front of linked list
        a->next = ahead;
        if (ahead != NULL) {
            ahead->prev = a;
        }
        ahead = a;
        ++nasteroids;
    }
    clock_gettime(CLOCK_REALTIME, &bulletTimer);
}
	~Game() {
		delete [] barr;
	}
} g;
//X Windows variables
// ---> for fullscreen x11(0, 0);

//==========================================================================
// M A I N
//==========================================================================
int main(){
    
	logOpen();
	init_opengl();
	srand(time(NULL));
	clock_gettime(CLOCK_REALTIME, &timePause);
	clock_gettime(CLOCK_REALTIME, &timeStart);
	x11.set_mouse_position(100,100);

	bool inMenu = true;
	int done = 0;

	while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            done = check_keys(&e);
        }
		
		if(!inMenu) {
		clock_gettime(CLOCK_REALTIME, &timeCurrent);
		timeSpan = timeDiff(&timeStart, &timeCurrent);
		timeCopy(&timeStart, &timeCurrent);
		physicsCountdown += timeSpan;
		while (physicsCountdown >= physicsRate) {
			physics();
			physicsCountdown -= physicsRate;
		}
		render();
		x11.swapBuffers();
		moveSmallAsteroidsTowardsShip();
		checkShipAsteroidCollision();
	} else {
	    handleMenu();
		inMenu = !inMenu;
		}
    }
	cleanup_fonts();
	logClose();
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, gl.xres, gl.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	//
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	//
	//Clear the screen to black
	glClearColor(0.0, 0.0, 0.0, 1.0);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void normalize2d(Vec v)
{
	Flt len = v[0]*v[0] + v[1]*v[1];
	if (len == 0.0f) {
		v[0] = 1.0;
		v[1] = 0.0;
		return;
	}
	len = 1.0f / sqrt(len);
	v[0] *= len;
	v[1] *= len;
}

void check_mouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	static int ct=0;
	//std::cout << "m" << std::endl << std::flush;
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
			//a little time between each bullet
			struct timespec bt;
			clock_gettime(CLOCK_REALTIME, &bt);
			double ts = timeDiff(&g.bulletTimer, &bt);
			if (ts > 0.1) {
				timeCopy(&g.bulletTimer, &bt);
				//shoot a bullet...
				if (g.nbullets < MAX_BULLETS) {
					Bullet *b = &g.barr[g.nbullets];
					timeCopy(&b->time, &bt);
					b->pos[0] = g.ship.pos[0];
					b->pos[1] = g.ship.pos[1];
					b->vel[0] = g.ship.vel[0];
					b->vel[1] = g.ship.vel[1];
					//convert ship angle to radians
					Flt rad = ((g.ship.angle+90.0) / 360.0f) * PI * 2.0;
					//convert angle to a vector
					Flt xdir = cos(rad);
					Flt ydir = sin(rad);
					b->pos[0] += xdir*20.0f;
					b->pos[1] += ydir*20.0f;
					b->vel[0] += xdir*6.0f + rnd()*0.1;
					b->vel[1] += ydir*6.0f + rnd()*0.1;
					b->color[0] = 1.0f;
					b->color[1] = 1.0f;
					b->color[2] = 1.0f;
					++g.nbullets;
				}
			}
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	//keys[XK_Up] = 0;
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		int xdiff = savex - e->xbutton.x;
		int ydiff = savey - e->xbutton.y;
		if (++ct < 10)
			return;		
		//std::cout << "savex: " << savex << std::endl << std::flush;
		//std::cout << "e->xbutton.x: " << e->xbutton.x << std::endl <<
		//std::flush;
		if (xdiff > 0) {
			//std::cout << "xdiff: " << xdiff << std::endl << std::flush;
			g.ship.angle += 0.05f * (float)xdiff;
			if (g.ship.angle >= 360.0f)
				g.ship.angle -= 360.0f;
		}
		else if (xdiff < 0) {
			//std::cout << "xdiff: " << xdiff << std::endl << std::flush;
			g.ship.angle += 0.05f * (float)xdiff;
			if (g.ship.angle < 0.0f)
				g.ship.angle += 360.0f;
		}
		if (ydiff > 0) {
			//apply thrust
			//convert ship angle to radians
			Flt rad = ((g.ship.angle+90.0) / 360.0f) * PI * 2.0;
			//convert angle to a vector
			Flt xdir = cos(rad);
			Flt ydir = sin(rad);
			g.ship.vel[0] += xdir * (float)ydiff * 0.01f;
			g.ship.vel[1] += ydir * (float)ydiff * 0.01f;
			Flt speed = sqrt(g.ship.vel[0]*g.ship.vel[0]+
												g.ship.vel[1]*g.ship.vel[1]);
			if (speed > 10.0f) {
				speed = 10.0f;
				normalize2d(g.ship.vel);
				g.ship.vel[0] *= speed;
				g.ship.vel[1] *= speed;
			}
			g.mouseThrustOn = true;
			clock_gettime(CLOCK_REALTIME, &g.mouseThrustTimer);
		}
		x11.set_mouse_position(100,100);
		savex = 100;
		savey = 100;
	}
}

int check_keys(XEvent *e)
{
	static int shift=0;
	if (e->type != KeyRelease && e->type != KeyPress) {
		//not a keyboard event
		return 0;
	}
	int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
	//Log("key: %i\n", key);
	key = wasd(key);

	if (e->type == KeyRelease) {
		gl.keys[key] = 0;
		if (key == XK_Shift_L || key == XK_Shift_R)
			shift = 0;
		return 0;
	}
	if (e->type == KeyPress) {
		//std::cout << "press" << std::endl;
		gl.keys[key]=1;
		if (key == XK_Shift_L || key == XK_Shift_R) {
			shift = 1;
			return 0;
		}
	}
	(void)shift;
	switch (key) {
		case XK_Escape:
			return 1;

		case XK_n:
		gl.nightmodefilter = !gl.nightmodefilter;
			break;

		case XK_m:
		gl.mouse = !gl.mouse;
		if (gl.mouse)
			x11.show_mouse_cursor(1);
		else
			x11.show_mouse_cursor(0);
		break;

		case XK_r:
		wasdvar = wasdtoggle();
    	gl.show_toggle = !gl.show_toggle;
		break;

		case XK_f:
		gl.statistics = !gl.statistics;
			break;
			
		case XK_s:
			break;
		case XK_Down:
			break;
		case XK_equal:
			break;
		case XK_minus:
			break;
	}
	return 0;
}

void deleteAsteroid(Game *g, Asteroid *node)
{
	//Remove a node from doubly-linked list
	//Must look at 4 special cases below.
	if (node->prev == NULL) {
		if (node->next == NULL) {
			//only 1 item in list.
			g->ahead = NULL;
		} else {
			//at beginning of list.
			node->next->prev = NULL;
			g->ahead = node->next;
		}
	} else {
		if (node->next == NULL) {
			//at end of list.
			node->prev->next = NULL;
		} else {
			//in middle of list.
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
	}
	delete node;
	node = NULL;
}

void buildAsteroidFragment(Asteroid *ta, Asteroid *a)
{
	//build ta from a
	ta->nverts = 8;
	ta->radius = a->radius / 2.0;
	Flt r2 = ta->radius / 2.0;
	Flt angle = 0.0f;
	Flt inc = (PI * 2.0) / (Flt)ta->nverts;
	for (int i=0; i<ta->nverts; i++) {
		ta->vert[i][0] = sin(angle) * (r2 + rnd() * ta->radius);
		ta->vert[i][1] = cos(angle) * (r2 + rnd() * ta->radius);
		angle += inc;
	}
	ta->pos[0] = a->pos[0] + rnd()*2.0-5.0;
	ta->pos[1] = a->pos[1] + rnd()*2.0-5.0;
	ta->pos[2] = 0.0f;
	ta->angle = 0.0;
	//ta->rotate = a->rotate + (rnd() * 4.0 - 2.0);
	ta->vel[0] = a->vel[0] + (rnd()*1.0-0.5);
	ta->vel[1] = a->vel[1] + (rnd()*1.0-0.5);
	//std::cout << "frag" << std::endl;
}


//----------------------------------------------------------------------------------------------------//
void checkShipAsteroidCollision() {
    Asteroid *a = g.ahead;
    while (a) {
        // Calculate the distance between the ship and the asteroid
        float d0 = g.ship.pos[0] - a->pos[0];
        float d1 = g.ship.pos[1] - a->pos[1];
        float distance = sqrt(d0 * d0 + d1 * d1);

        // Check if the distance is less than the sum of the ship's and asteroid's radii
        if (distance < g.ship.radius + a->radius) {
            // Collision detected, display "GAME OVER" and exit
        	usleep(2000000); // 3 seconds
			displayGameOver();
            exit(0);
        }
        a = a->next;
    }
}
//----------------------------------------------------------------------------------------------------//
void moveSmallAsteroidsTowardsShip() {
    Asteroid *a = g.ahead;
    while (a) {
        // Calculate the distance between the asteroid and the ship
        Flt d0 = g.ship.pos[0] - a->pos[0];
        Flt d1 = g.ship.pos[1] - a->pos[1];
        Flt dist = sqrt(d0 * d0 + d1 * d1);

        // Define a minimum distance for small asteroids to start following the ship
        const Flt MIN_FOLLOW_DISTANCE = 130.0;

        // If the asteroid is small and close enough to the ship, move towards the ship
        if (a->radius < MINIMUM_ASTEROID_SIZE && dist < MIN_FOLLOW_DISTANCE) {
            // Normalize the direction vector towards the ship
            Flt dirX = d0 / dist;
            Flt dirY = d1 / dist;

            // Define the speed at which small asteroids follow the ship
            const Flt FOLLOW_SPEED = 0.3;

            // Update the asteroid's velocity to follow the ship
            a->vel[0] = dirX * FOLLOW_SPEED;
            a->vel[1] = dirY * FOLLOW_SPEED;
        }
        a = a->next;
    }
}
//----------------------------------------------------------------------------------------------------//

void physics()
{
	Flt d0,d1,dist;
	//Update ship position
	g.ship.pos[0] += g.ship.vel[0];
	g.ship.pos[1] += g.ship.vel[1];
	//Check for collision with window edges
	if (g.ship.pos[0] < 0.0) {
		g.ship.pos[0] += (float)gl.xres;
	}
	else if (g.ship.pos[0] > (float)gl.xres) {
		g.ship.pos[0] -= (float)gl.xres;
	}
	else if (g.ship.pos[1] < 0.0) {
		g.ship.pos[1] += (float)gl.yres;
	}
	else if (g.ship.pos[1] > (float)gl.yres) {
		g.ship.pos[1] -= (float)gl.yres;
	}

	if (g.nasteroids == 8) {
        // Set the gameWon flag to true
        gameWon = true;
    }
	//
	//
	//Update bullet positions
	struct timespec bt;
	clock_gettime(CLOCK_REALTIME, &bt);
	int i = 0;
	while (i < g.nbullets) {
		Bullet *b = &g.barr[i];
		//How long has bullet been alive?
		double ts = timeDiff(&b->time, &bt);
		if (ts > 2.5) {
			//time to delete the bullet.
			memcpy(&g.barr[i], &g.barr[g.nbullets-1],
				sizeof(Bullet));
			g.nbullets--;
			//do not increment i.
			continue;
		}
		//move the bullet
		b->pos[0] += b->vel[0];
		b->pos[1] += b->vel[1];
		//Check for collision with window edges
		if (b->pos[0] < 0.0) {
			b->pos[0] += (float)gl.xres;
		}
		else if (b->pos[0] > (float)gl.xres) {
			b->pos[0] -= (float)gl.xres;
		}
		else if (b->pos[1] < 0.0) {
			b->pos[1] += (float)gl.yres;
		}
		else if (b->pos[1] > (float)gl.yres) {
			b->pos[1] -= (float)gl.yres;
		}
		++i;
	}
	//
	//Update asteroid positions
	Asteroid *a = g.ahead;
	while (a) {
		a->pos[0] += a->vel[0];
		a->pos[1] += a->vel[1];
		//Check for collision with window edges
		if (a->pos[0] < -100.0) {
			a->pos[0] += (float)gl.xres+200;
		}
		else if (a->pos[0] > (float)gl.xres+100) {
			a->pos[0] -= (float)gl.xres+200;
		}
		else if (a->pos[1] < -100.0) {
			a->pos[1] += (float)gl.yres+200;
		}
		else if (a->pos[1] > (float)gl.yres+100) {
			a->pos[1] -= (float)gl.yres+200;
		}
		//a->angle += a->rotate;
		a = a->next;
	}
	//
	//Asteroid collision with bullets?
	//If collision detected:
	//     1. delete the bullet
	//     2. break the asteroid into pieces
	//        if asteroid small, delete it
	a = g.ahead;
	while (a) {
		//is there a bullet within its radius?
		int i=0;
		while (i < g.nbullets) {
			Bullet *b = &g.barr[i];
			d0 = b->pos[0] - a->pos[0];
			d1 = b->pos[1] - a->pos[1];
			dist = (d0*d0 + d1*d1);
			if (dist < (a->radius*a->radius)) {
				//std::cout << "asteroid hit." << std::endl;
				//this asteroid is hit.
				if (a->radius > MINIMUM_ASTEROID_SIZE) {
					//break it into pieces.
					Asteroid *ta = a;
					buildAsteroidFragment(ta, a);
					int r = rand()%10+5;
					for (int k=0; k<r; k++) {
						//get the next asteroid position in the array
						Asteroid *ta = new Asteroid;
						buildAsteroidFragment(ta, a);
						//add to front of asteroid linked list
						ta->next = g.ahead;
						if (g.ahead != NULL)
							g.ahead->prev = ta;
						g.ahead = ta;
						g.nasteroids++;
					}
				} else {
					a->color[0] = 1.0;
					a->color[1] = 0.1;
					a->color[2] = 0.1;
					//asteroid is too small to break up
					//delete the asteroid and bullet
					Asteroid *savea = a->next;
					deleteAsteroid(&g, a);
					a = savea;
					g.nasteroids--;
				}
				//delete the bullet...
				memcpy(&g.barr[i], &g.barr[g.nbullets-1], sizeof(Bullet));
				g.nbullets--;
				if (a == NULL)
					break;
			}
			i++;
		}
		if (a == NULL)
			break;
		a = a->next;
	}

	//---------------------------------------------------
	//check keys pressed now
	if (gl.keys[XK_Left]) {
		g.ship.angle += 8.0;
		if (g.ship.angle >= 360.0f)
			g.ship.angle -= 360.0f;
	}
	if (gl.keys[XK_Right]) {
		g.ship.angle -= 8.0;
		if (g.ship.angle < 0.0f)
			g.ship.angle += 360.0f;
	}
	if (gl.keys[XK_Up]) {
		//apply thrust
		//convert ship angle to radians
		Flt rad = ((g.ship.angle+90.0) / 360.0f) * PI * 2.0;		
		//convert angle to a vector
		Flt xdir = cos(rad);
		Flt ydir = sin(rad);
		g.ship.vel[0] += xdir*0.01f;
		g.ship.vel[1] += ydir*0.01f;
		Flt speed = sqrt(g.ship.vel[0]*g.ship.vel[0]+
						g.ship.vel[1]*g.ship.vel[1]);
		if (speed > 0.5f) {
			speed = 0.5f;
			normalize2d(g.ship.vel);
			g.ship.vel[0] *= speed;
			g.ship.vel[1] *= speed;
		}
	}
	if (gl.keys[XK_space]) {
		//a little time between each bullet
		struct timespec bt;
		clock_gettime(CLOCK_REALTIME, &bt);
		double ts = timeDiff(&g.bulletTimer, &bt);
		if (ts > 0.1) {
			timeCopy(&g.bulletTimer, &bt);
			if (g.nbullets < MAX_BULLETS) {
				//shoot a bullet...
				//Bullet *b = new Bullet;
				Bullet *b = &g.barr[g.nbullets];
				timeCopy(&b->time, &bt);
				b->pos[0] = g.ship.pos[0];
				b->pos[1] = g.ship.pos[1];
				b->vel[0] = g.ship.vel[0];
				b->vel[1] = g.ship.vel[1];
				//convert ship angle to radians
				Flt rad = ((g.ship.angle+90.0) / 360.0f) * PI * 2.0;
				//convert angle to a vector
				Flt xdir = cos(rad);
				Flt ydir = sin(rad);
				b->pos[0] += xdir*20.0f;
				b->pos[1] += ydir*20.0f;
				b->vel[0] += xdir*6.0f + rnd()*0.1;
				b->vel[1] += ydir*6.0f + rnd()*0.1;
				b->color[0] = 1.0f;
				b->color[1] = 1.0f;
				b->color[2] = 1.0f;
				g.nbullets++;
			}
		}
	}
	if (g.mouseThrustOn) {
		//should thrust be turned off
		if(gl.mouse)
		return;
	//should thrust be turned off
		struct timespec mtt;
		clock_gettime(CLOCK_REALTIME, &mtt);
		double tdif = timeDiff(&mtt, &g.mouseThrustTimer);
		//std::cout << "tdif: " << tdif << std::endl;
		if (tdif < -0.3)
			g.mouseThrustOn = false;
	}
}

void render()
{
	Rect r;
	glClear(GL_COLOR_BUFFER_BIT);
	//
	r.bot = gl.yres - 30;
	r.left = 10;
	r.center = 0;
	ggprint16(&r, 16, 0x00ff0000, "Copyright Onslaught");
	ggprint16(&r, 16, 0x00ffff00, "n bullets: %i", g.nbullets);
	ggprint16(&r, 16, 0x00ffff00, "Number of Enemies: %i", g.nasteroids);

	if (gameWon) {
        displayYouWin(); // Display "You Win" screen
        return;
    }

	if (gl.nightmodefilter){
		nightmodefilter(gl.xres, gl.yres);
	}

	if (gl.show_toggle){
		display_border(gl.xres, gl.yres);
		display_toggle(10, 100);
	}

	if (gl.statistics) {
		glColor3ub(100,100,100);
		glPushMatrix();
		glTranslatef(20.0, 20.0, 0.0);
		int w = 230;
		int h = 130;
		glBegin(GL_QUADS);
			glVertex2f(0,0);
			glVertex2f(0,h);
			glVertex2f(w,h);
			glVertex2f(w,0);
		glEnd();
		glPopMatrix();
		r.bot = 123;
		r.left = 28;
		r.center = 0;
		ggprint13(&r, 20, 0x0055ff55, "Statistics...");
		ggprint13(&r, 16, 0x0055ff55, "sec_running_time:  %i", 
									total_running_time(true));
		// ggprint13(&r. 16, 0x0055ff55, "n_physics_function_calls:  %i"), total_physics_function_calls(true);
	}
	
	
	
	//-------------------------------------------------------------------------
	//Draw the ship
	glColor3fv(g.ship.color);
	glPushMatrix();
	glTranslatef(g.ship.pos[0], g.ship.pos[1], g.ship.pos[2]);
	//float angle = atan2(ship.dir[1], ship.dir[0]);
	glRotatef(g.ship.angle, 0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLES);
	// glVertex2f(-10.0f, -10.0f);
	// glVertex2f(  0.0f, 20.0f);
	// glVertex2f( 10.0f, -10.0f);
	glVertex2f(-12.0f, -10.0f);
	glVertex2f(  0.0f,  20.0f);
	glVertex2f(  0.0f,  -6.0f);
	glVertex2f(  0.0f,  -6.0f);
	glVertex2f(  0.0f,  20.0f);
	glVertex2f( 12.0f, -10.0f);
	glEnd();
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	glVertex2f(0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	if (gl.keys[XK_Up] || g.mouseThrustOn) {
		int i;
		//draw thrust
		Flt rad = ((g.ship.angle+90.0) / 360.0f) * PI * 2.0;
		//convert angle to a vector
		Flt xdir = cos(rad);
		Flt ydir = sin(rad);
		Flt xs,ys,xe,ye,r;
		glBegin(GL_LINES);
		for (i=0; i<16; i++) {
			xs = -xdir * 11.0f + rnd() * 4.0 - 2.0;
			ys = -ydir * 11.0f + rnd() * 4.0 - 2.0;
			r = rnd()*40.0+40.0;
			xe = -xdir * r + rnd() * 18.0 - 9.0;
			ye = -ydir * r + rnd() * 18.0 - 9.0;
			glColor3f(rnd()*.3+.7, rnd()*.3+.7, 0);
			glVertex2f(g.ship.pos[0]+xs,g.ship.pos[1]+ys);
			glVertex2f(g.ship.pos[0]+xe,g.ship.pos[1]+ye);
		}
		glEnd();
		
	}
	//-------------------------------------------------------------------------
	//Draw the asteroids
	{
		Asteroid *a = g.ahead;
    while (a) {
        glPushMatrix();
        glTranslatef(a->pos[0], a->pos[1], a->pos[2]);
        glColor3f(1.0, 0.5, 0.0);  // Orange outline color
		glLineWidth(5.0f); // You can adjust this value to make the outline thicker		


        // Draw a hexagon
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < 6; j++) {
            float angle = 2.0f * 3.1415926f * j / 6.0f;
            float x = a->radius * cos(angle);
            float y = a->radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
	    glLineWidth(1.0f); // Reset line width to its default value

        // Fill the hexagon with grey color
        glColor3f(0.6, 0.1, 0.1);  // Grey fill color
        glBegin(GL_POLYGON);
        for (int j = 0; j < 6; j++) {
            float angle = 2.0f * 3.1415926f * j / 6.0f;
            float x = a->radius * cos(angle);
            float y = a->radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();

		
        glPopMatrix();
        a = a->next;
    }
	}
	//-------------------------------------------------------------------------
	//Draw the bullets
	for (int i=0; i<g.nbullets; i++) {
		Bullet *b = &g.barr[i];
		//Log("draw bullet...\n");
		glColor3f(1.0, 1.0, 0.0);
		glBegin(GL_POINTS);
		glVertex2f(b->pos[0],      b->pos[1]);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]);
		glVertex2f(b->pos[0],      b->pos[1]-1.0f);
		glVertex2f(b->pos[0],      b->pos[1]+1.0f);
		glColor3f(0.8, 0.8, 0.8);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]-1.0f);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]+1.0f);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]-1.0f);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]+1.0f);
		glEnd();
	}
}