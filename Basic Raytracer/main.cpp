#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

#include "Vector.h"

/*===========================================================
 GLOBAL VARIABLES ===========================================
===========================================================*/

//direction vector
Vector LookAt(0, 0, -1);

//light vector
Vector LightDir (-1, 3, -5);

//ray viewpoint
Vector p (0, 0, 0);

//pixel point on screen
Vector sp (0, 0, 0);

//light constant
Vector ka (0.8, 0.4, 0.4);
Vector ks (1.0, 1.0, 1.0);

//pixel array
struct RGBType
{
    float r;
    float g;
    float b;
};
RGBType *pixels = new RGBType[256 * 256];

/*===========================================================
 SHADING ====================================================
===========================================================*/

Vector shade(Vector sc, int r, int i, int j, double discriminant, Vector kd)
{
    //compute illumination
    
    int t1 = -1 * (dotProduct(LookAt, sub(sp, sc)) - sqrt(discriminant))/dotProduct(LookAt, LookAt);
    int t2 = -1 * (dotProduct(LookAt, sub(sp, sc)) + sqrt(discriminant))/dotProduct(LookAt, LookAt);
    
    int time = std::max(t1, t2);
    
    //create surface normal vector at intersection point
    Vector n ((sp.getX() + (time * LookAt.getX()) - sc.getX())/r, (sp.getY() + (time * LookAt.getY()) - sc.getY())/r, (sp.getZ() + (time * LookAt.getZ()) - sc.getZ())/r);
    
    //ambient shading
    Vector la = mult(0.3, ka);

    //diffuse shading
    Vector ld = mult((0.5) * std::max(0.0, dotProduct(n , LightDir)), kd); //sphere color
    
    //specular shading
    Vector temp1 = add(LightDir, LookAt);
    double mag = magnitude(temp1);
    Vector vh (temp1.getX()/mag, temp1.getY()/mag, temp1.getZ()/mag);
    Vector ls = mult((0.3) * pow(std::max(0.0, dotProduct(n, vh)), 16), ks); //white

    //overall lighting
    Vector l = add(la, add(ls, ld));
    
    return l;
}

/*===========================================================
 RAYTRACE ===================================================
===========================================================*/

void raytrace(void) {

    //sphere radius
    int r1 = 48, r2 = 40;
    
    //sphere centers
    Vector sc (60, 0, -1);
    Vector sc2 (-60, 0, -1);
    
    //Diffuse constants
    Vector kd1 (0.2, 0.9, 0.2);
    Vector kd2 (0.0, 0.4, 0.8);
    
    //default black color
    for(int i = 0; i < 256 * 256; i++)
    {
        pixels[i].r= 0.0;
        pixels[i].g= 0.0;
        pixels[i].b= 0.0;
    }
    
    double discriminant = 0.0;
    
    //constructing basis vectors
    double u = 0.0, v = 0.0;
    double lx = LookAt.getX(), ly = LookAt.getY(), lz = LookAt.getZ();
    double ux = 0.0, uy = 0.0;
    if(lx == 0 && ly == 0)
    {
        ux = absv(lz);
        uy = 0.0;
    }
    else{
        ux = ly;
        uy = -lx;
    }
    Vector uvector (ux, uy, 0);
    Vector vvector = crossProduct3D(uvector, LookAt);
    
    //for each pixel
    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            //using basis vectors to find screen point
            u = -128 + j + 0.5;
            v = -128 + i + 0.5;
            
            Vector nu = mult(u, uvector);
            Vector nv = mult(v, vvector);
            sp = add(p, add(nu, nv));
            
            //check if ray hits spehere
            discriminant = pow(dotProduct(LookAt, sub(sp, sc)), 2) - dotProduct(LookAt, LookAt) * (dotProduct(sub(sp, sc), sub(sp, sc)) - pow(r1, 2));
            if(discriminant >= 0.0)
            {
                //determine pixel color
                Vector rgb_color = shade(sc, r1, i, j, discriminant, kd1);
                pixels[(256 * i) + j].r = rgb_color.getX();
                pixels[(256 * i) + j].g = rgb_color.getY();
                pixels[(256 * i) + j].b = rgb_color.getZ();
            }
            else{
                //check intersection with other spehe
                discriminant = pow(dotProduct(LookAt, sub(sp, sc2)), 2) - dotProduct(LookAt, LookAt) * (dotProduct(sub(sp, sc2), sub(sp, sc2)) - pow(r2, 2));
                if(discriminant  >= 0.0)
                {
                    //determine pixel color
                    Vector rgb_color = shade(sc2, r2, i, j, discriminant, kd2);
                    pixels[(256 * i) + j].r = rgb_color.getX();
                    pixels[(256 * i) + j].g = rgb_color.getY();
                    pixels[(256 * i) + j].b = rgb_color.getZ();
                }
            }
        }
    }
}

/*===========================================================
 DISPLAY ====================================================
===========================================================*/

void renderScene(void)
{
 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDrawPixels(256,256,GL_RGB,GL_FLOAT,pixels);
    
	glutSwapBuffers();
    
}

/*===========================================================
 MAIN =======================================================
===========================================================*/

int main(int argc, char **argv) {
    
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(256,256);
	glutCreateWindow("Lucka's Simple Raytracer");
    
    //raytracing
    raytrace();
    
	// register callbacks
	glutDisplayFunc(renderScene);
    
	// enter GLUT event processing cycle
	glutMainLoop();
	
	return 1;
}
