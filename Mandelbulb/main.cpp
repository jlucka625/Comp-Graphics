//
//  main.cpp
//  Mandelbulb
//
//  Created by Jonathan Lucka on 3/14/14.
//  Copyright (c) 2014 Jonathan Lucka. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "Vector.h"

#include <iostream>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#endif

/*==========================================================
GLOBAL VARS ================================================
==========================================================*/
float s_real = -2.0, s_imag = -1.5, e_real = 2.0, e_imag = 1.5;
float dr = 0.0, di = 0.0;

float POWER = 8.0;

int MAX_ITERATIONS = 4;
const int MAX_STEP = 255;
const float BAILOUT = 2.0;
const float MIN_DIST = 0.0001;
bool hit = false;
float scaleFactor = 1.0;

//pixel array
struct RGBType
{
    float r;
    float g;
    float b;
};
RGBType *pixels = new RGBType[512 * 512];

bool topView = false;
bool shading = true;

/*==========================================================
MANDELBULB =================================================
==========================================================*/
void fillPixel(int x, int y, Vector color)
{
    int index = y * 512 + x;
    pixels[index].r = color.x;
    pixels[index].g = color.y;
    pixels[index].b = color.z;
    
}

float distanceEstimator(Vector pos)
{
    Vector z = pos;
    float dr = 1.0, r = 0.0;
    for(int i = 0; i < MAX_ITERATIONS; i++)
    {
        r = z.mag();
        if(r > BAILOUT) break;
        
        float theta = asinf(z.z / r);
        float phi = atanf(z.y / z.x);
        dr = powf(r, POWER - 1.0) * POWER * dr + 1.0;
        
        float zr = powf(r, POWER);
        theta *= POWER;
        phi *= POWER;
        
        Vector zz(cos(theta) * cos(phi), sin(phi) * cos(theta), sin(theta));
        z = zz.mult(zr);
        z = z.v_add(pos);
    }
    return 0.5 * log(r) * r / dr;
}
Vector trace(Vector from)
{
    hit = false;
    float totalDist = 0.0;
    int steps;
    Vector pos = from;
    for(steps = 0; steps < MAX_STEP; steps++)
    {
        if(topView) pos = from.v_add(Vector(0, 0, totalDist));
        else pos = from.v_add(Vector(0, totalDist, 0));
        
        float dist = distanceEstimator(pos);
        totalDist += dist;
        if(dist < MIN_DIST)
        {
            hit = true;
            break;
        }
    }
    pos.w = 1.0 - (float)steps / (float)MAX_STEP;
    return pos;
}

float max(float a, float b)
{
    if(a > b)
        return a;
    else return b;
}

Vector shade(Vector pos)
{
    Vector lightDir(0, 1, 0);
    Vector lookAt(0, 1, 0);
    
    if(topView)
    {
        lightDir.y = 0; lightDir.z = 1;
        lookAt.y = 0; lookAt.z = 1;
    }
    
    float e = 0.000001;
    float nx = distanceEstimator(pos.v_add(Vector(e, 0, 0))) - distanceEstimator(pos.v_add(Vector(-e, 0, 0)));
    float ny = distanceEstimator(pos.v_add(Vector(0, e, 0))) - distanceEstimator(pos.v_add(Vector(0, -e, 0)));
    float nz = distanceEstimator(pos.v_add(Vector(0, 0, e))) - distanceEstimator(pos.v_add(Vector(0, 0, -e)));
    
    Vector n(nx, ny, nz);
    n = n.mult(1/n.mag());

    Vector ka(0.7, 0.0, 0.4);
    Vector la = ka.mult(0.3);
    
    Vector kd(0.3, 0.0, 0.8);
    Vector ld = kd.mult(0.6).mult(max(0.0, n.dot(lightDir)));
    
    Vector ks(1.0, 1.0, 1.0);
    Vector temp1 = lookAt.v_add(lightDir);
    float mag = temp1.mag();
    Vector h = temp1.mult(1/mag);
    Vector ls = ks.mult(0.9).mult(pow(max(0.0, n.dot(h)), 64));
    
    Vector l = la.v_add(ls.v_add(ld));
    return l;
}

void clearPixels()
{
    for(int i = 0; i < 512; i++)
    {
        for(int j = 0; j < 512; j++)
        {
            fillPixel(i, j, Vector(0.0, 0.0, 0.0));
        }
    }
}

float absv(float val)
{
    if(val < 0)
        return -val;
    else return val;
}

void mandelbulb()
{
    clearPixels();
    float result, xx, yy;
    float real = absv(s_real - e_real), imag = absv(s_imag - e_imag);
    float dx = real/512.0;
    float dy = imag/512.0;
    for(int y = 0; y < 512; y++)
    {
        for(float x = 0; x < 512; x++)
        {
            
            xx = real * (float)x/512.0 - e_real + dr;
            yy = imag * (float)y/512.0 - e_imag + di;
            Vector from(0.0, 0.0, 0.0);
            if(topView){ from.x = xx; from.y = yy; from.z = -4.0;}
            else{ from.x = xx; from.y = -4.0; from.z = yy;}
            
            Vector result = trace(from);
            if(hit){
                if(shading)
                    fillPixel(x, y, shade(result));
                else fillPixel(x, y, Vector(result.w, result.w, result.w));
            }
            else fillPixel(x, y, Vector(0.0, 0.0, 0.0));
        }
    }
}


/*==========================================================
DISPLAY ====================================================
===========================================================*/

void renderScene(void)
{
 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDrawPixels(512,512,GL_RGB,GL_FLOAT,pixels);
    
	glutSwapBuffers();
    
}

/*==========================================================
KEYBOARD ===================================================
===========================================================*/

void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case '1':
            if(shading) MAX_ITERATIONS = 31;
            else MAX_ITERATIONS = 1000;
            POWER = 2.0;
            mandelbulb();
            break;
        case '2':
            if(shading) MAX_ITERATIONS = 5;
            else MAX_ITERATIONS = 1000;
            POWER = 4.0;
            mandelbulb();
            break;
        case '3':
            if(shading) MAX_ITERATIONS = 5;
            else MAX_ITERATIONS = 1000;
            POWER = 6.0;
            mandelbulb();
            break;
        case '4':
            if(shading) MAX_ITERATIONS = 4;
            else MAX_ITERATIONS = 1000;
            POWER = 8.0;
            mandelbulb();
            break;
        case '5':
            if(shading) MAX_ITERATIONS = 3;
            else MAX_ITERATIONS = 1000;
            POWER = 16.0;
            mandelbulb();
            break;
        case '6':
            if(!shading)
                shading = true;
            else
            {
                shading = false;
                MAX_ITERATIONS = 1000;
            }
            mandelbulb();
            break;
        case '7':
            if(!topView)
                topView = true;
            else topView = false;
            mandelbulb();
            break;
        case 'w':
            di += 0.1/scaleFactor;
            mandelbulb();
            break;
        case 'a':
            dr -= 0.1/scaleFactor;
            mandelbulb();
            break;
        case 's':
            di -= 0.1/scaleFactor;
            mandelbulb();
            break;
        case 'd':
            dr += 0.1/scaleFactor;
            mandelbulb();
            break;
        case 'z':
            scaleFactor++;
            s_real -= s_real/2.0;
            s_imag -= s_imag/2.0;
            e_real -= e_real/2.0;
            e_imag -= e_imag/2.0;
            mandelbulb();
            break;
        case 'x':
            if(scaleFactor > 0)
                scaleFactor--;
            s_real += s_real/2.0;
            s_imag += s_imag/2.0;
            e_real += e_real/2.0;
            e_imag += e_imag/2.0;
            mandelbulb();
            break;
        case 'q':
            MAX_ITERATIONS++;
            mandelbulb();
            break;
            
            
    }
    glutPostRedisplay();
}

/*==========================================================
MAIN =======================================================
==========================================================*/

int main(int argc, char **argv) {
    
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(512,512);
	glutCreateWindow("Mandelbulb");
    
    //mandelbulb
    mandelbulb();
    
	// register callbacks
	glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboard);
    
	// enter GLUT event processing cycle
	glutMainLoop();
	
	return 1;
}
