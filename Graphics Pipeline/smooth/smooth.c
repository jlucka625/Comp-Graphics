/*  
    smooth.c
    Nate Robins, 1998

    Model viewer program.  Exercises the glm library.
*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <GLUT/glut.h>
#include "gltb.h"
#include "glm.h"
#include "dirent32.h"

#pragma comment( linker, "/entry:\"mainCRTStartup\"" )  // set the entry point to be main()

#define DATA_DIR "data/"
int usingPipeline = 0;
int flatShading = 0;
int smoothShading = 0;

char*      model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		    /* display list for object */
GLMmodel*  model;			        /* glm model data structure */
GLfloat    scale;			        /* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			    /* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;

#define CLK_TCK 1000
#if defined(_WIN32)
#include <sys/timeb.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#endif

/*=======================================================================
STRUCTS =================================================================
=======================================================================*/

//for RGB colors
struct RGBType
{
    float r;
    float g;
    float b;
};
struct RGBType pixels[512 * 512];

//points taken from the model
struct projectedPoint
{
    int x;
    int y;
    double z;
    double nx;
    double ny;
    double nz;
    struct RGBType color;
};

//points on the frame
struct framePoint
{
    int populated;
    double z;
    struct RGBType color;
};
//for entire frame
struct framePoint frameBuffer[512 * 512];
//for individual triangles
struct framePoint triangle[512 * 512];

/*=======================================================================
HELPER METHODS ==========================================================
=======================================================================*/

//distance between two points
double distance(struct projectedPoint p1, struct projectedPoint p2)
{
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

//finds area of a triangle
double area(struct projectedPoint p1, struct projectedPoint p2, struct projectedPoint p3)
{
    double a = distance(p1, p2);
    double b = distance(p2, p3);
    double c = distance (p3, p1);
    
    double s = (a + b + c)/2.0;
    
    return sqrt(s * (s - a) * (s - b) * (s - c));
}

//finds min between two int values
int min(int a, int b)
{
    if(a < b)
        return a;
    else return b;
}

//finds max between two int values
int max(int a, int b)
{
    if(a > b)
        return a;
    else return b;
}

//finds max between two double values
double maxd(double a, double b)
{
    if(a > b)
        return a;
    else return b;
}

//swaps two int values
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

//swaps two colors
void swapColor(struct RGBType* a, struct RGBType* b)
{
    struct RGBType temp = *a;
    *a = *b;
    *b = temp;
}

double implicitLine(double x0, double y0, double x1, double y1, double x, double y)
{
    return (y0 - y1) * x + (x1 - x0) * y + (x0 * y1) - (x1 * y0);
}

void clearTriangleBuffer(void)
{
    int i, j;
    for(i = 0; i < 512; i++)
    {
        for(j = 0; j < 512; j++)
        {
            triangle[(i * 512) + j].populated = 0;
        }
    }
}

void clearFrameBuffer(void)
{
    int i, j;
    for(i = 0; i < 512; i++)
    {
        for(j = 0; j < 512; j++)
        {
            frameBuffer[(i * 512) + j].populated = 0;
        }
    }
}

void clearPixels(void)
{
    int i, j;
    for(i = 0; i < 512; i++)
    {
        for(j = 0; j < 512; j++)
        {
            pixels[(i * 512) + j].r = 0.0;
            pixels[(i * 512) + j].g = 0.0;
            pixels[(i * 512) + j].b = 0.0;
        }
    }
}

/*=======================================================================
INTERPOLATION ===========================================================
=======================================================================*/

double interpolate1D(struct projectedPoint p1, struct projectedPoint p2, int x, int y)
{
    //intermediate point
    struct projectedPoint p3;
    p3.x = x; p3.y = y;
    
    //distance between endpoints
    double dep = distance(p1, p2);
    
    //distance from p1
    double dp1 = distance(p1, p3);
    
    dp1 /= dep;

    return (1 - dp1) * p1.z + dp1 * p2.z;
}

struct RGBType interpolate1Dcolor(struct projectedPoint p1, struct projectedPoint p2, int x, int y)
{
    //intermediate point
    struct projectedPoint p3;
    p3.x = x; p3.y = y;
    
    //distance between endpoints
    double dep = distance(p1, p2);
    
    //distance from p1
    double dp1 = distance(p1, p3);
    
    dp1 /= dep;
    
    struct RGBType color;
    color.r = (1 - dp1) * p1.color.r + dp1 * p2.color.r;
    color.g = (1 - dp1) * p1.color.g + dp1 * p2.color.g;
    color.b = (1 - dp1) * p1.color.b + dp1 * p2.color.b;
    return color;
}

double interpolate2D(struct projectedPoint p1, struct projectedPoint p2, struct projectedPoint p3, int x, int y)
{
    struct projectedPoint p4;
    p4.x = x; p4.y = y;
    
    double triArea = area(p1, p2, p3);

    double alpha1 = area(p1, p2, p4)/triArea;
    double alpha2 = area(p1, p3, p4)/triArea;
    double alpha3 = area(p2, p3, p4)/triArea;
    
    return (alpha1 * p3.z) + (alpha2 * p2.z) + (alpha3 * p1.z);
}

struct RGBType interpolate2Dcolor(struct projectedPoint p1, struct projectedPoint p2, struct projectedPoint p3, int x, int y)
{
    struct projectedPoint p4;
    p4.x = x; p4.y = y;
    
    double triArea = area(p1, p2, p3);
    
    double alpha1 = area(p1, p2, p4)/triArea;
    double alpha2 = area(p1, p3, p4)/triArea;
    double alpha3 = area(p2, p3, p4)/triArea;
    
    struct RGBType color;
    color.r = (alpha1 * p3.color.r) + (alpha2 * p2.color.r) + (alpha3 * p1.color.r);
    color.g = (alpha1 * p3.color.g) + (alpha2 * p2.color.g) + (alpha3 * p1.color.g);
    color.b = (alpha1 * p3.color.b) + (alpha2 * p2.color.b) + (alpha3 * p1.color.b);
    return color;
}

/*=======================================================================
BRASENHAM ===============================================================
=======================================================================*/
void plotPoint(struct projectedPoint p1, struct projectedPoint p2, int x, int y, struct RGBType color)
{
    double z;
    int pointIndex = (y) * 512 + x;
    if(frameBuffer[pointIndex].populated == 0)
    {
        frameBuffer[pointIndex].populated = 1;
        frameBuffer[pointIndex].z = interpolate1D(p1, p2, x, y);
        if(flatShading == 1)
        {
            frameBuffer[pointIndex].color = color;
        }
        if(smoothShading == 1)
        {
            frameBuffer[pointIndex].color = interpolate1Dcolor(p1, p2, x, y);
        }
    }
    else
    {
        z = interpolate1D(p1, p2, x, y);
        if(frameBuffer[pointIndex].z > z)
        {
            frameBuffer[pointIndex].z = z;
            if(flatShading == 1)
            {
                frameBuffer[pointIndex].color = color;
            }
            if(smoothShading == 1)
            {
                frameBuffer[pointIndex].color = interpolate1Dcolor(p1, p2, x, y);
            }
        }
    }
    triangle[pointIndex].populated = 1;
}

void brasenham(struct projectedPoint p1, struct projectedPoint p2, struct RGBType color)
{
    int pointIndex = 0;
    double z;
    if(p1.y == p2.y) //horizontal line
    {
        int x = min(p1.x, p2.x);
        int y = p1.y;
        while(x <= max(p1.x, p2.x))
        {
            plotPoint(p1, p2, x, y, color);
            x += 1;
        }
    }
    else if(p1.x == p2.x) //vertical line
    {
        int x = p1.x;
        int y = min(p1.y, p2.y);
        while(y <= max(p1.y, p2.y))
        {
            plotPoint(p1, p2, x, y, color);
            y += 1;
        }
    }
    else{
        double slope = (double)(p2.y - p1.y)/(double)(p2.x - p1.x);
        if(slope >= 0 && slope <= 1)
        {
            if(p1.x > p2.x)
            {
                swap(&p1.x, &p2.x);
                swap(&p1.y, &p2.y);
                swapColor(&p1.color, &p2.color);
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 1, p1.y + 0.5);
            int x = min(p1.x, p2.x);
            int y = min(p1.y, p2.y);
            while(x <= max(p1.x, p2.x))
            {
                plotPoint(p1, p2, x, y, color);
                if(d < 0)
                {
                    y++;
                    d += (p2.x - p1.x) + (p1.y - p2.y);
                }
                else d += p1.y - p2.y;
                x++;
            }
        }
        else if(slope >= -1 && slope <= 0)
        {
            if(p1.x < p2.x)
            {
                swap(&p1.x, &p2.x);
                swap(&p1.y, &p2.y);
                swapColor(&p1.color, &p2.color);
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 1, p1.y - 0.5);
            int x = min(p1.x, p2.x);
            int y = max(p1.y, p2.y);
            while(x <= max(p1.x, p2.x))
            {
                plotPoint(p1, p2, x, y, color);
                if(d < 0)
                {
                    y--;
                    d += (p1.y - p2.y) - (p2.x - p1.x);
                }
                else d += p1.y - p2.y;
                x++;
            }
        }
        else if(slope > 1)
        {
            if(p1.x < p2.x)
            {
                swap(&p1.x, &p2.x);
                swap(&p1.y, &p2.y);
                swapColor(&p1.color, &p2.color);
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 0.5, p1.y + 1);
            int x = min(p1.x, p2.x);
            int y = min(p1.y, p2.y);
            while(y <= max(p1.y, p2.y))
            {
                plotPoint(p1, p2, x, y, color);
                if(d < 0)
                {
                    x++;
                    d += (p2.x - p1.x) + (p1.y - p2.y);
                }
                else d += p2.x - p1.x;
                y++;
            }
        }
        else
        {
            if(p1.x > p2.x)
            {
                swap(&p1.x, &p2.x);
                swap(&p1.y, &p2.y);
                swapColor(&p1.color, &p2.color);
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 0.5, p1.y - 1);
            int x = min(p1.x, p2.x);
            int y = max(p1.y, p2.y);
            while(y >= min(p1.y, p2.y))
            {
                plotPoint(p1, p2, x, y, color);
                if(d < 0)
                {
                    x++;
                    d += (p1.y - p2.y) - (p2.x - p1.x);
                }
                else d += p1.x - p2.x;
                y--;
            }
        }
    }
}

/*=======================================================================
SCAN LINE ===============================================================
=======================================================================*/

void scanLine(struct projectedPoint p1, struct projectedPoint p2, struct projectedPoint p3, struct RGBType color)
{
    int pointIndex = 0;
    double z;
    int xmin = min(p1.x, min(p2.x, p3.x)), xmax = max(p1.x, max(p2.x, p3.x));
    int ymin = min(p1.y, min(p2.y, p3.y)), ymax = max(p1.y, max(p2.y, p3.y));
    
    int x = xmin, y = ymin, sx = 0, ex = 0, vertexIntersectCount = 0;
    
    for(y = ymin + 1; y <= ymax - 1; y++)
    {
        for(x = xmin; x <= xmax; x++)
        {
            pointIndex = y * 512 + x;
            if(triangle[pointIndex].populated == 1)
            {
                sx = x + 1;
                break;
            }
        }
        for(x = xmax; x >= xmin; x--)
        {
            pointIndex = y * 512 + x;
            if(triangle[pointIndex].populated == 1)
            {
                ex = x - 1;
                break;
            }
        }
        for(x = sx; x <= ex; x++)
        {
            pointIndex = y * 512 + x;
            if(frameBuffer[pointIndex].populated == 0)
            {
                frameBuffer[pointIndex].populated = 1;
                frameBuffer[pointIndex].z = interpolate2D(p1, p2, p3, x, y);
                if(flatShading == 1)
                {
                    frameBuffer[pointIndex].color = color;
                }
                if(smoothShading == 1)
                {
                    frameBuffer[pointIndex].color = interpolate2Dcolor(p1, p2, p3, x, y);
                }
            }
            else
            {
                z = interpolate2D(p1, p2, p3, x, y);
                if(frameBuffer[pointIndex].z >= z)
                {
                    frameBuffer[pointIndex].z = z;
                    if(flatShading == 1)
                    {
                        frameBuffer[pointIndex].color = color;
                    }
                    if(smoothShading == 1)
                    {
                        frameBuffer[pointIndex].color = interpolate2Dcolor(p1, p2, p3, x, y);
                    }
                }
            }
            triangle[pointIndex].populated = 1;
        }
    }
}

/*=======================================================================
SHADING =================================================================
=======================================================================*/

struct RGBType computeShade(double nx, double ny, double nz, GLMmaterial mat, GLdouble* modelview)
{
    //ambient shading
    float la_r = mat.ambient[0] * 0.5;
    float la_g = mat.ambient[1] * 0.5;
    float la_b = mat.ambient[2] * 0.5;
    
    //light dir
    float lx = 0;
    float ly = 0;
    float lz = 1;

    float mag;
    
    if(flatShading == 1)
    {
        mag = sqrt((nx * nx) + (ny * ny) + (nz * nz));
        nx /= mag;
        ny /= mag;
        nz /= mag;
    }
    
    //diffuse shading
    float dp = (nx * lx) + (ny * ly) + (nz * lz);
    float ld_r = mat.diffuse[0] * 1.0 * maxd(0.0, dp);
    float ld_g = mat.diffuse[1] * 1.0 * maxd(0.0, dp);
    float ld_b = mat.diffuse[2] * 1.0 * maxd(0.0, dp);
    
    //look at vector
    float ex = 0;
    float ey = 0;
    float ez = 1;
    
    //calculating h vector
    float tempx = lx + ex;
    float tempy = ly + ey;
    float tempz = lz + ez;
    
    mag = sqrt((tempx * tempx) + (tempy * tempy) + (tempz * tempz));
    float hx = tempx/mag;
    float hy = tempy/mag;
    float hz = tempz/mag;
    
    //specular shading
    dp = (nx * hx) + (ny * hy) + (nz * hz);
    float ls_r = mat.specular[0] * 0.3 * pow(maxd(0.0, dp), mat.shininess);
    float ls_g = mat.specular[1] * 0.3 * pow(maxd(0.0, dp), mat.shininess);
    float ls_b = mat.specular[2] * 0.3 * pow(maxd(0.0, dp), mat.shininess);
    
    struct RGBType color;
    color.r = la_r + ld_r + ls_r;
    color.g = la_g + ld_g + ls_g;
    color.b = la_b + ld_b + ls_b;

    return color;
}

void shade()
{
    int y, x, pi;
    for(y = 0; y < 512; y++)
    {
        for(x = 0; x < 512; x++)
        {
            pi = y * 512 + x;
            if(frameBuffer[pi].populated == 1)
            {
                pixels[pi].r = frameBuffer[pi].color.r;
                pixels[pi].g = frameBuffer[pi].color.g;
                pixels[pi].b = frameBuffer[pi].color.b;
            }
        }
    }
}

/*=======================================================================
RASTERIZE ===============================================================
=======================================================================*/

void rasterize(struct projectedPoint p1, struct projectedPoint p2, struct projectedPoint p3, GLMmaterial mat, GLdouble* modelview)
{
    int i;
    
    //find triangle normal
    struct RGBType color;
    if(flatShading == 1)
    {
        float nx = (p1.nx + p2.nx + p3.nx)/3;
        float ny = (p1.ny + p2.ny + p3.ny)/3;
        float nz = (p1.nz + p2.nz + p3.nz)/3;
        color = computeShade(nx, ny, nz, mat, modelview);
    }
    if(smoothShading == 1)
    {
        p1.color = computeShade(p1.nx, p1.ny, p1.nz, mat, modelview);
        p2.color = computeShade(p2.nx, p2.ny, p2.nz, mat, modelview);
        p3.color = computeShade(p3.nx, p3.ny, p3.nz, mat, modelview);
    }
    
    //draw line from p1 to p2
    brasenham(p1, p2, color);
        
    //draw line from p2 to p3
    brasenham(p2, p3, color);
        
    //draw line from p3 to p1
    brasenham(p3, p1, color);
        
    scanLine(p1, p2, p3, color);
    clearTriangleBuffer();
}

/*=======================================================================
PIPELINE ================================================================
=======================================================================*/

void pipeline()
{
    clearFrameBuffer();
    clearPixels();
    
    //groups and triangle variables
    GLMgroup *currentGroup = model->groups;
    GLMtriangle tri;
    int triIndex, pointIndex;
    
    //for vertices
    double a, b, c, na, nb, nc;
    int k = 0;
    struct projectedPoint pts[3];
    
    //for glProject
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    GLdouble winX, winY, winZ;
    
    GLMmaterial mat;
    
    //entire pipeline process
    while(currentGroup != NULL)
    {
        int i, j;
        mat = model->materials[currentGroup->material];
        for(i = 0; i < currentGroup->numtriangles; i++)
        {
            triIndex = currentGroup->triangles[i];
            tri = model->triangles[triIndex];
            for(j = 0; j < 3; j++)
            {
                a = model->vertices[3*tri.vindices[j]];
                b = model->vertices[3*tri.vindices[j] + 1];
                c = model->vertices[3*tri.vindices[j] + 2];
                
                pts[j].nx = model->normals[3*tri.nindices[j]];
                pts[j].ny = model->normals[3*tri.nindices[j] + 1];
                pts[j].nz = model->normals[3*tri.nindices[j] + 2];

                gluProject(a, b, c, modelview, projection, viewport, &winX, &winY, &winZ);
                pts[j].x = winX; pts[j].y = winY; pts[j].z = winZ;
                //k++;
            }
            rasterize(pts[0], pts[1], pts[2], mat, modelview);
        }
        currentGroup = currentGroup->next;
    }
    shade();
}

/*=======================================================================
=========================================================================
=======================================================================*/

float
elapsed(void)
{
    static long begin = 0;
    static long finish, difference;
    
#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*1000+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif
    
    difference = finish - begin;
    begin = finish;
    
    return (float)difference/(float)CLK_TCK;
}

void
shadowtext(int x, int y, char* s) 
{
    int lines;
    char* p;
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
        0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x+1, y-1);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x+1, y-1-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(0, 128, 255);
    glRasterPos2i(x, y);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x, y-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void
lists(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat shininess = 65.0;
    
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    
    if (model_list)
        glDeleteLists(model_list, 1);
    
    /* generate a list */
    if (material_mode == 0) { 
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT);
        else
            model_list = glmList(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT | GLM_COLOR);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
    }
}

void
init(void)
{
    gltbInit(GLUT_LEFT_BUTTON);
    
    /* read in the model */
    model = glmReadOBJ(model_file);
    scale = glmUnitize(model);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);
    
    if (model->nummaterials > 0)
        material_mode = 2;
    
    /* create new display lists */
    lists();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    
    glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_CULL_FACE);
}

void
reshape(int width, int height)
{
    gltbReshape(width, height);
    
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.0);
}

#define NUM_FRAMES 5
void
display(void)
{
        static char s[256], t[32];
        static char* p;
        static int frames = 0;
        
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glPushMatrix();
    
        glTranslatef(pan_x, pan_y, 0.0);
    
        gltbMatrix();

        if(usingPipeline == 0)
        {
#if 0   /* glmDraw() performance test */
            if (material_mode == 0) {
                if (facet_normal)
                    glmDraw(model, GLM_FLAT);
                else
                    glmDraw(model, GLM_SMOOTH);
            } else if (material_mode == 1) {
                if (facet_normal)
                    glmDraw(model, GLM_FLAT | GLM_COLOR);
                else
                    glmDraw(model, GLM_SMOOTH | GLM_COLOR);
            } else if (material_mode == 2) {
                if (facet_normal)
                    glmDraw(model, GLM_FLAT | GLM_MATERIAL);
                else
                    glmDraw(model, GLM_SMOOTH | GLM_MATERIAL);
            }
#else
            glCallList(model_list);
#endif
            
            glDisable(GL_LIGHTING);
            if (bounding_box) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                glEnable(GL_CULL_FACE);
                glColor4f(1.0, 0.0, 0.0, 0.25);
                glutSolidCube(2.0);
                glDisable(GL_BLEND);
            }
        }
        else{
            pipeline();
            glDrawPixels(512,512,GL_RGB,GL_FLOAT,pixels);
        }
    
        glPopMatrix();
        
        /*if (stats) {
            // XXX - this could be done a _whole lot_ faster... 
            int height = glutGet(GLUT_WINDOW_HEIGHT);
            glColor3ub(0, 0, 0);
            sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
                    "%d texcoords\n%d groups\n%d materials",
                    model->pathname, model->numvertices, model->numtriangles,
                    model->numnormals, model->numtexcoords, model->numgroups,
                    model->nummaterials);
            shadowtext(5, height-(5+18*1), s);
            }*/

        /* spit out frame rate. */
        frames++;
        if (frames > NUM_FRAMES) {
            sprintf(t, "%g fps", frames/elapsed());
            frames = 0;
        }
        if (performance) {
            shadowtext(5, 5, t);
        }
        
        glutSwapBuffers();
        glEnable(GL_LIGHTING);
}

void
keyboard(unsigned char key, int x, int y)
{
    GLint params[2];
    
    switch (key) {
    case 'h':
        printf("help\n\n");
        printf("y         -  Toggle graphics pipeline/flat shading");
        printf("u         -  Toggle graphics pipeline/smooth shading");
        printf("w         -  Toggle wireframe/filled\n");
        printf("c         -  Toggle culling\n");
        printf("n         -  Toggle facet/smooth normal\n");
        printf("b         -  Toggle bounding box\n");
        printf("r         -  Reverse polygon winding\n");
        printf("m         -  Toggle color/material/none mode\n");
        printf("p         -  Toggle performance indicator\n");
        printf("s/S       -  Scale model smaller/larger\n");
        printf("t         -  Show model stats\n");
        printf("o         -  Weld vertices in model\n");
        printf("+/-       -  Increase/decrease smoothing angle\n");
        printf("W         -  Write model to file (out.obj)\n");
        printf("q/escape  -  Quit\n\n");
        break;

    case 'y':
        if(usingPipeline == 0 && smoothShading == 0)
        {
            usingPipeline = 1;
            flatShading = 1;
            smoothShading = 0;
            pipeline();
        }
        else if(usingPipeline == 1 && smoothShading == 1)
        {
            smoothShading = 0;
            flatShading = 1;
            pipeline();
        }
        else
        {
            usingPipeline = 0;
            flatShading = 0;
        }
        break;
    case 'u':
        if(usingPipeline == 0 && flatShading == 0)
        {
            usingPipeline = 1;
            smoothShading = 1;
            flatShading = 0;
            pipeline();
        }
        else if(usingPipeline == 1 && flatShading == 1)
        {
            smoothShading = 1;
            flatShading = 0;
            pipeline();
        }
        else
        {
            smoothShading = 0;
            usingPipeline = 0;
        }
        break;
    case 't':
        stats = !stats;
        break;
        
    case 'p':
        performance = !performance;
        break;
        
    case 'm':
        material_mode++;
        if (material_mode > 2)
            material_mode = 0;
        printf("material_mode = %d\n", material_mode);
        lists();
        break;
        
    case 'd':
        glmDelete(model);
        init();
        lists();
        break;
        
    case 'w':
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (params[0] == GL_FILL)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
        
    case 'c':
        if (glIsEnabled(GL_CULL_FACE))
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);
        break;
        
    case 'b':
        bounding_box = !bounding_box;
        break;
        
    case 'n':
        facet_normal = !facet_normal;
        lists();
        break;
        
    case 'r':
        glmReverseWinding(model);
        lists();
        break;
        
    case 's':
        glmScale(model, 0.8);
        lists();
        break;
        
    case 'S':
        glmScale(model, 1.25);
        lists();
        break;
        
    case 'o':
        //printf("Welded %d\n", glmWeld(model, weld_distance));
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case 'O':
        weld_distance += 0.01;
        printf("Weld distance: %.2f\n", weld_distance);
        glmWeld(model, weld_distance);
        glmFacetNormals(model);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case '-':
        smoothing_angle -= 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case '+':
        smoothing_angle += 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case 'W':
        glmScale(model, 1.0/scale);
        glmWriteOBJ(model, "out.obj", GLM_SMOOTH | GLM_MATERIAL);
        break;
        
    case 'R':
        {
            GLuint i;
            GLfloat swap;
            for (i = 1; i <= model->numvertices; i++) {
                swap = model->vertices[3 * i + 1];
                model->vertices[3 * i + 1] = model->vertices[3 * i + 2];
                model->vertices[3 * i + 2] = -swap;
            }
            glmFacetNormals(model);
            lists();
            break;
        }
        
    case 27:
        exit(0);
        break;
    }
    
    glutPostRedisplay();
}

void
menu(int item)
{
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
    
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
        dirp = opendir(DATA_DIR);
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;
        name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
        strcpy(name, DATA_DIR);
        strcat(name, direntp->d_name);
        model = glmReadOBJ(name);
        scale = glmUnitize(model);
        glmFacetNormals(model);
        glmVertexNormals(model, smoothing_angle);
        
        if (model->nummaterials > 0)
            material_mode = 2;
        else
            material_mode = 0;
        
        lists();
        free(name);
        
        glutPostRedisplay();
    }
}

static GLint      mouse_state;
static GLint      mouse_button;

void
mouse(int button, int state, int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    /* fix for two-button mice -- left mouse + shift = middle mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
        button = GLUT_MIDDLE_BUTTON;
    
    gltbMouse(button, state, x, y);
    
    mouse_state = state;
    mouse_button = button;
    
    if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }

    glutPostRedisplay();
}

void
motion(int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    gltbMotion(x, y);
    if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }
    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    int buffering = GLUT_DOUBLE;
    struct dirent* direntp;
    DIR* dirp;
    int models;
    
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    
    while (--argc) {
        if (strcmp(argv[argc], "-sb") == 0)
            buffering = GLUT_SINGLE;
        else
            model_file = argv[argc];
    }
    
    if (!model_file) {
        model_file = "/data/dolphins.obj";
    }
    
    //pixels
    int p;
    for(p = 0; p < 512 * 512; p++)
    {
        pixels[p].r = 0.0;
        pixels[p].g = 0.0;
        pixels[p].b = 0.0;
    }
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
    glutCreateWindow("Smooth");
    
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    
    models = glutCreateMenu(menu);
    dirp = opendir(DATA_DIR);
    if (!dirp) {
        fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
    
    glutCreateMenu(menu);
    glutAddMenuEntry("Smooth", 0);
    glutAddMenuEntry("", 0);
    glutAddSubMenu("Models", models);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[w]   Toggle wireframe/filled", 'w');
    glutAddMenuEntry("[c]   Toggle culling on/off", 'c');
    glutAddMenuEntry("[n]   Toggle face/smooth normals", 'n');
    glutAddMenuEntry("[b]   Toggle bounding box on/off", 'b');
    glutAddMenuEntry("[p]   Toggle frame rate on/off", 'p');
    glutAddMenuEntry("[t]   Toggle model statistics", 't');
    glutAddMenuEntry("[m]   Toggle color/material/none mode", 'm');
    glutAddMenuEntry("[r]   Reverse polygon winding", 'r');
    glutAddMenuEntry("[s]   Scale model smaller", 's');
    glutAddMenuEntry("[S]   Scale model larger", 'S');
    glutAddMenuEntry("[o]   Weld redundant vertices", 'o');
    glutAddMenuEntry("[+]   Increase smoothing angle", '+');
    glutAddMenuEntry("[-]   Decrease smoothing angle", '-');
    glutAddMenuEntry("[W]   Write model to file (out.obj)", 'W');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    init();
    
    glutMainLoop();
    return 0;
}
