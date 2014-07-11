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

/*==========================================================
GLOBAL VARS ================================================
==========================================================*/

enum
{
    BLENDING,
    CASTELJAU,
    OPENGL1,
    OPENGL2,
    SUBDIV
};

//pixel array
struct RGBType
{
    float r;
    float g;
    float b;
};
RGBType *pixels = new RGBType[512 * 512];

int bezierMatrix[16] = {-1, 3, -3, 1, 3, -6, 3, 0, -3, 3, 0, 0, 1, 0, 0, 0};
float coords1[12];
float coords2[12];
float coords3[48];

struct Point2D
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

int pointCount = 0;
Point2D *pts = new Point2D[7];
int mode = BLENDING;
int subdivSteps = 1;

RGBType hullColor;
RGBType curveColor;


/*==========================================================
DISPLAY ====================================================
==========================================================*/

void clearBackground()
{
    int pointIndex;
    for(int i = 0; i < 512; i++)
    {
        for(int j = 0; j < 512; j++)
        {
            pointIndex = i * 512 + j;
            pixels[pointIndex].r = 0.0;
            pixels[pointIndex].g = 0.0;
            pixels[pointIndex].b = 0.0;
        }
    }
}

void renderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDrawPixels(512,512,GL_RGB,GL_FLOAT,pixels);

    if(mode == OPENGL1)
    {
        glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, coords1);
        glEnable(GL_MAP1_VERTEX_3);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_STRIP);
        for(int i = 0; i < 100; i++)
            glEvalCoord1f((float)i/100.0);
        glEnd();
        
        glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, coords2);
        glEnable(GL_MAP1_VERTEX_3);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_STRIP);
        for(int i = 0; i < 100; i++)
            glEvalCoord1f((float)i/100.0);
        glEnd();
    }
    else if(mode == OPENGL2)
    {
        glMap2f(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4, coords3);
        glEnable(GL_MAP2_VERTEX_3);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        for(int j = 0; j <= 8; j++)
        {
            glBegin(GL_LINE_STRIP);
            for(int i = 0; i <= 40; i++)
            {
                glEvalCoord2f((float)i/40.0, (float) j/8.0);
            }
            glEnd();
            glBegin(GL_LINE_STRIP);
            for(int i = 0; i <= 40; i++)
            {
                glEvalCoord2f((float)j/8.0, (float) i/40.0);
            }
            glEnd();
        }
    }
    glutSwapBuffers();
    
}

/*=======================================================================
BRASENHAM ===============================================================
=======================================================================*/
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

//swaps two int values
void swap(float *a, float *b)
{
    float temp = *a;
    *a = *b;
    *b = temp;
}

void plotPoint(int x, int y, RGBType color)
{
    int pointIndex = y * 512 + x;
    pixels[pointIndex].r = color.r;
    pixels[pointIndex].g = color.g;
    pixels[pointIndex].b = color.b;
}

double implicitLine(double x0, double y0, double x1, double y1, double x, double y)
{
    return (y0 - y1) * x + (x1 - x0) * y + (x0 * y1) - (x1 * y0);
}

void brasenham(Point2D p1, Point2D p2, RGBType color)
{
    int pointIndex = 0;
    double z;
    if(p1.y == p2.y) //horizontal line
    {
        int x = min(p1.x, p2.x);
        int y = p1.y;
        while(x <= max(p1.x, p2.x))
        {
            plotPoint(x, y, color);
            x += 1;
        }
    }
    else if(p1.x == p2.x) //vertical line
    {
        int x = p1.x;
        int y = min(p1.y, p2.y);
        while(y <= max(p1.y, p2.y))
        {
            plotPoint(x, y, color);
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
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 1, p1.y + 0.5);
            int x = min(p1.x, p2.x);
            int y = min(p1.y, p2.y);
            while(x <= max(p1.x, p2.x))
            {
                plotPoint(x, y, color);
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
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 1, p1.y - 0.5);
            int x = min(p1.x, p2.x);
            int y = max(p1.y, p2.y);
            while(x <= max(p1.x, p2.x))
            {
                plotPoint(x, y, color);
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
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 0.5, p1.y + 1);
            int x = min(p1.x, p2.x);
            int y = min(p1.y, p2.y);
            while(y <= max(p1.y, p2.y))
            {
                plotPoint(x, y, color);
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
            }
            double d = implicitLine(p1.x, p1.y, p2.x, p2.y, p1.x + 0.5, p1.y - 1);
            int x = min(p1.x, p2.x);
            int y = max(p1.y, p2.y);
            while(y >= min(p1.y, p2.y))
            {
                plotPoint(x, y, color);
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


/*==========================================================
CURVE DRAWING IMPLEMENTATIONS ==============================
==========================================================*/
void matrixMult(int* geomMatrix, int* cofs)
{
    for(int i = 0; i < 4; i++)
    {
        cofs[i] = 0;
        for(int j = 0; j < 4; j++)
        {
            cofs[i] += geomMatrix[j] * bezierMatrix[i * 4 + j];
        }
    }
}

void drawBlendedCurve(Point2D p0, Point2D p1, Point2D p2, Point2D p3)
{
    int x , y, pointIndex;
    int geomMatrixX[4], geomMatrixY[4], cofsX[4], cofsY[4];
    
    geomMatrixX[0] = p0.x; geomMatrixX[1] = p1.x; geomMatrixX[2] = p2.x; geomMatrixX[3] = p3.x;
    geomMatrixY[0] = p0.y; geomMatrixY[1] = p1.y; geomMatrixY[2] = p2.y; geomMatrixY[3] = p3.y;
    matrixMult(geomMatrixX, cofsX);
    matrixMult(geomMatrixY, cofsY);
    for(float j = 0.0; j <= 1.0; j+=0.001)
    {
        x = (int)(cofsX[3] + j * (cofsX[2] + j * (cofsX[1] + j * (cofsX[0]))));
        y = (int)(cofsY[3] + j * (cofsY[2] + j * (cofsY[1] + j * (cofsY[0]))));
        pointIndex = y * 512 + x;
        pixels[pointIndex].r = 1.0;
        pixels[pointIndex].g = 1.0;
        pixels[pointIndex].b = 1.0;
    }
}

void blendedBezier()
{
    drawBlendedCurve(pts[0], pts[1], pts[2], pts[6]);
    drawBlendedCurve(pts[6], pts[3], pts[4], pts[5]);
}

void curveGL2()
{
     //x                  y                     z
     coords3[0] = -0.5; coords3[1] = 0.5; coords3[2] = 1.0;
     coords3[3] = -0.2; coords3[4] = 0.7; coords3[5] = 1.0;
     coords3[6] =  0.1; coords3[7] = 0.5; coords3[8] = 1.0;
     coords3[9] = 0.5; coords3[10] = 0.4; coords3[11] = 1.0;
     
     //x                  y                     z
     coords3[12] = -0.6; coords3[13] = 0.1; coords3[14] = 0.0;
     coords3[15] = -0.2; coords3[16] = 0.1; coords3[17] = 0.0;
     coords3[18] =  0.1; coords3[19] = 0.1; coords3[20] = 0.0;
     coords3[21] = 0.8; coords3[22] = 0.1; coords3[23] = 0.0;
     
     //x                  y                     z
     coords3[24] = -0.4; coords3[25] = -0.2; coords3[26] = 0.0;
     coords3[27] = -0.2; coords3[28] = -0.2; coords3[29] = 0.0;
     coords3[30] =  0.1; coords3[31] = -0.2; coords3[32] = 0.0;
     coords3[33] = 0.9; coords3[34] = -0.2; coords3[35] = 0.0;
     
     //x                  y                     z
     coords3[36] = -0.5; coords3[37] = -0.5; coords3[38] = 0.0;
     coords3[39] = -0.2; coords3[40] = -0.25; coords3[41] = 0.0;
     coords3[42] =  0.1; coords3[43] = -0.25; coords3[44] = 0.0;
     coords3[45] = 0.5; coords3[46] = -0.5; coords3[47] = 0.0;
}

void curveGL1()
{
    //float coords[21];
    coords1[0] = (pts[0].x - 256.0)/256.0; coords1[1] = (pts[0].y - 256.0)/256.0; coords1[2] = 0.0;
    coords1[3] = (pts[1].x - 256.0)/256.0; coords1[4] = (pts[1].y - 256.0)/256.0; coords1[5] = 0.0;
    coords1[6] = (pts[2].x - 256.0)/256.0; coords1[7] = (pts[2].y - 256.0)/256.0; coords1[8] = 0.0;
    coords1[9] = (pts[6].x - 256.0)/256.0; coords1[10] = (pts[6].y - 256.0)/256.0; coords1[11] = 0.0;
    
    coords2[0] = (pts[6].x - 256.0)/256.0; coords2[1] = (pts[6].y - 256.0)/256.0; coords2[2] = 0.0;
    coords2[3] = (pts[3].x - 256.0)/256.0; coords2[4] = (pts[3].y - 256.0)/256.0; coords2[5] = 0.0;
    coords2[6] = (pts[4].x - 256.0)/256.0; coords2[7] = (pts[4].y - 256.0)/256.0; coords2[8] = 0.0;
    coords2[9] = (pts[5].x - 256.0)/256.0; coords2[10] = (pts[5].y - 256.0)/256.0; coords2[11] = 0.0;
}

float distance(Point2D p1, Point2D p2)
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

Point2D interpolate1D(Point2D p1, Point2D p2, float t)
{
    Point2D p;
    p.x = (1.0 - t) * p1.x + t * p2.x;
    p.y = (1.0 - t) * p1.y + t * p2.y;
    return p;
}

void drawCasteljau(Point2D p0, Point2D p1, Point2D p2, Point2D p3)
{
    Point2D q1, q2, q3, r1, r2, p;
    int pointIndex = 0;
    for(float t = 0.0; t <= 1.0; t += 0.001)
    {
        q1 = interpolate1D(p0, p1, t);
        q2 = interpolate1D(p1, p2, t);
        q3 = interpolate1D(p2, p3, t);
        r1 = interpolate1D(q1, q2, t);
        r2 = interpolate1D(q2, q3, t);
        p = interpolate1D(r1, r2, t);
        pointIndex = ((int)p.y * 512 + (int)p.x);
        pixels[pointIndex].r = 1.0;
        pixels[pointIndex].g = 1.0;
        pixels[pointIndex].b = 1.0;
    }
}

void deCasteljau()
{
    drawCasteljau(pts[0], pts[1], pts[2], pts[6]);
    drawCasteljau(pts[6], pts[3], pts[4], pts[5]);
}

void drawSubdivCurve(Point2D p0, Point2D p1, Point2D p2, Point2D p3, int n)
{
    Point2D q1, q2, q3, r1, r2, p;
    int pointIndex = 0;
    if(n < subdivSteps)
    {
        q1 = interpolate1D(p0, p1, 0.5);
        q2 = interpolate1D(p1, p2, 0.5);
        q3 = interpolate1D(p2, p3, 0.5);
        r1 = interpolate1D(q1, q2, 0.5);
        r2 = interpolate1D(q2, q3, 0.5);
        p = interpolate1D(r1, r2, 0.5);
        drawSubdivCurve(p0, q1, r1, p, n + 1);
        drawSubdivCurve(p, r2, q3, p3, n + 1);
    }
    else
    {
        brasenham(p0, p1, curveColor);
        brasenham(p1, p2, curveColor);
        brasenham(p2, p3, curveColor);
    }
}

void subdiv()
{
    drawSubdivCurve(pts[0], pts[1], pts[2], pts[6], 0);
    drawSubdivCurve(pts[6], pts[3], pts[4], pts[5], 0);
}

/*==========================================================
CURVE MODE =================================================
==========================================================*/
void drawHull()
{
    brasenham(pts[0], pts[1], hullColor);
    brasenham(pts[1], pts[2], hullColor);
    brasenham(pts[2], pts[6], hullColor);
    brasenham(pts[6], pts[3], hullColor);
    brasenham(pts[3], pts[4], hullColor);
    brasenham(pts[4], pts[5], hullColor);
}

void getMiddlePoint()
{
    pts[6].x = (pts[2].x + pts[3].x)/2.0;
    pts[6].y = (pts[2].y + pts[3].y)/2.0;
}

void curveMode()
{
    if(mode == BLENDING)
        blendedBezier();
    else if(mode == CASTELJAU)
        deCasteljau();
    else if(mode == OPENGL1)
        curveGL1();
    else if(mode == OPENGL2)
        curveGL2();
    else if(mode == SUBDIV)
    {
        subdiv();
        subdivSteps++;
    }

}

/*==========================================================
KEYBOARD ===================================================
==========================================================*/

void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case '1':
            mode = BLENDING;
            break;
        case '2':
            mode = CASTELJAU;
            break;
        case '3':
            mode = OPENGL1;
            break;
        case '4':
            mode = OPENGL2;
            break;
        case '5':
            subdivSteps = 1;
            mode = SUBDIV;
            break;
        case 'c':
            pointCount = 0;
            clearBackground();
            break;
        case 'd':
            if(pointCount == 6 && mode != OPENGL2)
            {
                clearBackground();
                getMiddlePoint();
                drawHull();
                curveMode();
            }
            else if(mode == OPENGL2)
            {
                clearBackground();
                curveMode();
            }
            break;
    }
    glutPostRedisplay();
}

/*==========================================================
MOUSE ======================================================
==========================================================*/

void mouse(int button, int state, int x, int y)
{
    int pointIndex;
    if(pointCount < 6 && state == GLUT_UP)
    {
        pts[pointCount].x = x;
        pts[pointCount].y = 512 - y;
        pointCount++;
        
        pointIndex = (512 - y) * 512 + x;
        pixels[pointIndex].r = 1.0;
        pixels[pointIndex].g = 0.0;
        pixels[pointIndex].b = 0.0;
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
	glutCreateWindow("Curve Drawer");
    
    hullColor.r = 1.0; hullColor.g = 0.0; hullColor.b = 0.0;
    curveColor.r = 1.0; curveColor.g = 1.0; curveColor.b = 1.0;
    
    clearBackground();
    
	// register callbacks
	glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    
	// enter GLUT event processing cycle
	glutMainLoop();
	
	return 1;
}



