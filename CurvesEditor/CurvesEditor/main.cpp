//
//  main.cpp
//  CurvesEditor
//
//  Created by Kevin Donahoe on 10/5/15.
//  Copyright (c) 2015 Kevin Donahoe. All rights reserved.
//

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// Needed on MsWindows
#include <windows.h>
#endif // Win32 platform

#include <OpenGL/gl.h>
#include "float2.h"
#include <OpenGL/glu.h>
// Download glut from: http://www.opengl.org/resources/libraries/glut/
#include <GLUT/glut.h>

bool pPressed = false;
bool bPressed = false;
bool aPressed = false;
bool dPressed = false;
bool lPressed = false;
bool nonePressed = true;
float t = 0;

//int clickX = 0;
//int clickY = 0;

class Curve {
public:
    virtual float2 getPoint(float t)=0;
    
    virtual void draw(){
        glBegin(GL_LINE_STRIP);
        for (float i = 0; i < 1; i+=.01) {
            float2 point = getPoint(i);
            float x = point.x;
            float y = point.y;
            glVertex2d(x, y);
            
        }
        glEnd();
    }
    
};



class Freeform : public Curve
{
protected:
    std::vector<float2> controlPoints;
public:
    bool selected = false;
    bool isPolyLine = false;
    bool isLagrange = false;
 
    
    virtual float2 getPoint(float t)=0;
    virtual void addControlPoint(float2 p)
    {
        
        controlPoints.push_back(p);
        controlPoints.at(controlPoints.size()-1).x = p.x;
        controlPoints.at(controlPoints.size()-1).y = p.y;

    }
    
    
    void drawControlPoints(){
        for(int i=0; i<controlPoints.size(); i++){
            
            glBegin(GL_POINTS);
            float2 point = controlPoints[i];
            glVertex2d(point.x, point.y);
            glEnd();
        }
    }
    
    int numControlPoints(){
        return controlPoints.size();
    }
    
    std::vector<float2> getCPoints(){
        return controlPoints;
    }
    
    void deleteCPoint(int index){
        controlPoints.erase((controlPoints.begin()+index));
        
    }
    
};


class Polyline : public Freeform
{
public:
    float2 getPoint(float t){
        return float2(0.0, 0.0);
    }
    
    void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
    }
    
    void draw(){
        if(controlPoints.size()>1){
            for(int i = 0; i < controlPoints.size()-1; i++){
                glBegin(GL_LINE_STRIP);
                float2 point1 = controlPoints[i];
                float2 point2 = controlPoints[i+1];
                glVertex2d(point1.x, point1.y);
                glVertex2d(point2.x, point2.y);
            }
            glEnd();
        }
        
    }
    
};


class BezierCurve : public Freeform
{
    //params of bernstein: index , number of control points -1, t (the curve parameter)
    static double bernstein(int i, int n, double t) {
        if(n == 1) {
            if(i == 0) return 1-t;
            if(i == 1) return t;
            return 0;
        }
        if(i < 0 || i > n) return 0;
        return (1 - t) * bernstein(i,   n-1, t)
        +      t  * bernstein(i-1, n-1, t);
    }
    
    public :
    
    float2 getPoint(float t)
    {
        float2 r(0.0, 0.0);
        float weight;
        // for every control point
        for (float i = 0; i < controlPoints.size(); i++) {
            // compute weight using the Bernstein formula
            weight = bernstein(i, controlPoints.size()-1, t);
            r += controlPoints.at(i)*weight;
        }
        // add control point to r, weighted
        return r;
    }
};


class Largrange : public Freeform
{
    //knot weights
    std:: vector<float> knots;
    
    //n is one less than the number of controlpoints
    double lagranginate(int i, double t) {
        
        double numerator = 1;
        double denominator = 1;
        
        for(int j=0; j<controlPoints.size(); j++)
        {
            
            
            if(j != i){
                numerator *= (t - knots.at(j));
                denominator *= (knots.at(i) - knots.at(j));
          
            }
            
        }
        
        return numerator / denominator;
    }
    
    public :
        void addControlPoint(float2 p){
            controlPoints.push_back(p);
            int controlPointsSize = controlPoints.size();
            
            knots.clear();
            
            if(controlPointsSize == 1){
                knots.push_back(0);
                return;
            }
    
            
            for(int j=0; j<controlPointsSize; j++){
                knots.push_back(j / (controlPoints.size()-1.0));
            }
        }
    
    
        void eraseCP(){
        
            knots.clear();

            if(controlPoints.size() == 1){
                knots.push_back(0);
                return;
            }
        
            for(int j=0; j<controlPoints.size(); j++){
                knots.push_back(j / (controlPoints.size()-1.0));
            }
        }
    
    float2 getPoint(float t)
    {
        float2 r(0.0, 0.0);
        float weight;
        // for every control point
        for (float i = 0; i < controlPoints.size(); i++) {
            // compute weight using the Lagrange formula
            weight = lagranginate(i, t);
            
            r += controlPoints.at(i)*weight;

        }
        // add control point to r, weighted
        
        return r;
    }
};




std::vector<Freeform*> curves;

class CurveScene

{
public:
    void addCurve(Freeform* curve) {
        curves.push_back(curve);
    }
    //destructor --> iterates through the list, and deletes them
    ~CurveScene() {
        for(unsigned int i=0; i<curves.size(); i++)
            delete curves.at(i);
    }
    void draw() {
        for(unsigned int i=0; i<curves.size(); i++){
            if(curves.at(i)->selected == false){
                    curves.at(i)->draw();
            }
        }
    }
    
    void deleteCurve(int index){
        curves.erase(curves.begin()+index);
    }
    
    
};

CurveScene scene = *new CurveScene();



void onDisplay(){
    glClearColor(0.3f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen. color_buffer_bit needed, depth_buffer_bit not needed now but doesn't hurt
    
    
    GLfloat lineWidth;
    GLfloat widthSizes[2];
    
    widthSizes[0] = 10.0f;
    widthSizes[1] = 20.0f;
    glGetFloatv(GL_LINE_WIDTH_RANGE, widthSizes);
    
    lineWidth = widthSizes[1];
    for(int i=0; i<curves.size(); i++){
        if(curves.at(i)->selected){
            glColor3d(0.0, 0.0, 1.0);
            glPointSize(15);
            curves.at(i)->drawControlPoints();
            glColor3d(0.0, 0.0, 1.0);
            glLineWidth(lineWidth);
            curves.at(i)->draw();
            glEnd();
        }
    }
    
    glColor3d(1.0, 0.0, 0.0);
    lineWidth = widthSizes[0];
    glLineWidth(lineWidth);
    scene.draw();
    
    
    glutSwapBuffers();
    
    
}

void onReshape(int winWidth0, int winHeight0) {
    glViewport(0, 0, winWidth0, winHeight0);
}


int indexCurveSelected(){
    for(int i=0; i<curves.size(); i++){
        if(curves.at(i)->selected){
            return i;
        }
    }
    //returns initial curve in curves if none are selected
    return -1;
}


void deselectPreviouslySelected(){
    if(indexCurveSelected() != -1)
        curves.at(indexCurveSelected())->selected = false;
}


//bool isMouseClicked(int button, int state, int x, int y){
//    
//    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
//        return true;
//    }
//    return false;
//    
//}


//Add a new instance of a curve every time a key is hit
void onKeyboard(unsigned char key,int x, int y) {
    
    switch (key) {
        case 'p':{
            if(pPressed == false){
                Polyline *pLine = new Polyline;
                pLine->selected = true;
                pLine->isPolyLine = true;
                scene.addCurve(pLine);
                pPressed = true;
                nonePressed = false;
            }
            break;
        }
        case 'a':{
            aPressed = true;
            nonePressed = false;
            break;
        }
        case 'd':{
            dPressed = true;
            nonePressed = false;
            break;

        }
        case 'l':{
            if (lPressed == false) {
                Largrange *lCurve = new Largrange;
                lCurve->selected = true;
                lCurve->isLagrange = true;
                scene.addCurve(lCurve);
                lPressed = true;
                nonePressed = false;
            }
            break;
        }
        case 'b':{
            if(bPressed == false){
                BezierCurve *bCurve = new BezierCurve;
                bCurve->selected = true;
                scene.addCurve(bCurve);
                bPressed = true;
                nonePressed = false;
            }
            break;
        }
        case ' ':{
            if(curves.size() > 0){
                int indexSelected = indexCurveSelected();
                if(indexSelected == -1){
                    indexSelected++;
                }
                int nextIndex = indexSelected + 1;
                if(nextIndex != curves.size()){
                    curves.at(indexSelected)->selected = false;
                    curves.at(nextIndex)->selected = true;
                }
                else{
                    curves.at(0)->selected = true;
                    curves.at(indexSelected)->selected = false;
                }
            }
        
            break;
        }
        default:
            break;
    }
}


void checkIfEnoughPoints(){
    for(int i=0; i<curves.size(); i++){
        if (curves.at(i)->numControlPoints() < 2) {
            curves.at(i)->deleteCPoint(0);
            scene.deleteCurve(i);
        }
    }
    
}

void onKeyboardUp(unsigned char key, int x, int y) {
    if (key == 'p') {
        pPressed = false;
        nonePressed = true;
        curves.at(curves.size()-1)->selected = false;
    }
    if (key == 'a') {
        aPressed = false;
        nonePressed = true;
        
    }
    if (key == 'd') {
        dPressed = false;
        nonePressed = true;
    }
    
    if (key == 'l') {
        lPressed = false;
        nonePressed = true;
        curves.at(curves.size()-1)->selected = false;
    }
    if (key == 'b') {
        bPressed = false;
        nonePressed = true;
        curves.at(curves.size()-1)->selected = false;
    }
    checkIfEnoughPoints();
}




//distrance from the line
//sin of angle
//length of the cross product (normalize vector of the line * (r - r0) )
//if dot product is negative, then it's outside (two cases)
Freeform *closestCurveForPoly(float2 click, Freeform *curve){
    Freeform *closest = nullptr;
    for (int i=0; i<curve->getCPoints().size()-1; i++) {
        float2 cp1 = curve->getCPoints().at(i);
        float2 cp2 = curve->getCPoints().at(i+1);
        
        float2 firstLine = click - cp1;
        
        float2 secondLine = cp2 - cp1;
        
        float2 tempSecondLine = secondLine;
        
        secondLine.normalize();
        
        float crossProduct = (firstLine.x * secondLine.y - firstLine.y * secondLine.x);
        
        float dotProduct =  ((click.x - curve->getCPoints().at(i).x) *
                             (curve->getCPoints().at(i+1).x - curve->getCPoints().at(i).x))
                            + ((click.y - curve->getCPoints().at(i).y) *
                            (curve->getCPoints().at(i+1).y - curve->getCPoints().at(i).y));
        
        
        if((fabs(crossProduct) < 0.05f) && (dotProduct > 0.0f)){
            
            if(fabs(firstLine.y) > fabs(tempSecondLine.y) || (fabs(firstLine.x) > fabs(tempSecondLine.x))){
                continue;
            }

            closest = curve;
            break;
        }
    }
    return closest;
}

Freeform *closestCurveToMouse(float2 click){
    Freeform *closest = nullptr;
    for (int i=0; i<curves.size(); i++) {
        if(closest != nullptr){
            break;
        }
        if(curves.at(i)->isPolyLine){
            closest = closestCurveForPoly(click, curves.at(i));
            if(closest != nullptr){
                break;
            }
            
        }
        else{
            for(float p=0; p < 1; p+=.01 ){
                if(((fabs(click.x - curves.at(i)->getPoint(p).x)) < 0.09f) &&
                   ((fabs(click.y - curves.at(i)->getPoint(p).y)) < 0.09f))
                {
                    closest = curves.at(i);
                    break;
                }
            }

        }
    }
    
    return closest;
}



float2 *closestControlPoint(float2 click){
    float2 *closest = nullptr;
    Freeform *curve = closestCurveToMouse(click);
    if(curve != nullptr){
        for(int i=0; i<curve->numControlPoints(); i++){
            if((fabs(click.x - curve->getCPoints().at(i).x) < 0.09f) && (fabs(click.y - curve->getCPoints().at(i).y) < 0.09f)){
                closest = &curve->getCPoints().at(i);
            }
        }
        
    }
    
    return closest;
    
}



//onMove --> callback function called whenever move the mouse
//if MOUSE/key is pressed, then drag and drop
//when click mouse, you register that position --> determie the difference vector betweeen the click, and the current mouse, and add that
//difference vector to the control points of the curve
//be sure to call postdispley in move function, to ensure smooth move
//calculate difference vector --> add the difference vector to the control point

void onMouse(int button, int state, int x, int y) {
    int viewportRect[4];
    glGetIntegerv(GL_VIEWPORT, viewportRect);
    
//    int clickX = x * 2.0 / viewportRect[2] - 1.0;
//    int clickY = -y * 2.0 / viewportRect[3] + 1.0;
//    
//    
    if(!nonePressed){
        if (pPressed) {
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
                curves.at(curves.size()-1)->addControlPoint(float2(x * 2.0 / viewportRect[2] - 1.0,
                                                                   -y * 2.0 / viewportRect[3] + 1.0));
        }
        if(dPressed){
            if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && curves.size()>0){
                float2 *closestCPoint = closestControlPoint(float2(
                                                                   x * 2.0 / viewportRect[2] - 1.0,
                                                                   -y * 2.0 / viewportRect[3] + 1.0));
                
                Freeform *closestCurve = closestCurveToMouse((float2(
                                                                     x * 2.0 / viewportRect[2] - 1.0,
                                                                     -y * 2.0 / viewportRect[3] + 1.0)));
                if(closestCPoint != nullptr && closestCurve != nullptr){
                    for(int i=0; i<closestCurve->getCPoints().size(); i++){
                        if((closestCurve->getCPoints().at(i).x == closestCPoint->x) &&
                           (closestCurve->getCPoints().at(i).y == closestCPoint->y)){
                            closestCurve->deleteCPoint(i);
                            if(closestCurve->isLagrange){
                                Largrange *lcurve = (Largrange*) closestCurve;
                                lcurve->eraseCP();
                            }
                            break;
                        }
                    }
                    checkIfEnoughPoints();
                }
            }
        }
        if(lPressed){
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
                curves.at(curves.size()-1)->addControlPoint( float2(
                                                                    x * 2.0 / viewportRect[2] - 1.0,
                                                                    -y * 2.0 / viewportRect[3] + 1.0));
                
            }
            
        }
        
        if(bPressed){
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
                curves.at(curves.size()-1)->addControlPoint( float2(
                                                                    x * 2.0 / viewportRect[2] - 1.0,
                                                                    -y * 2.0 / viewportRect[3] + 1.0));
                
            }
        }
        if(aPressed){
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
                if(indexCurveSelected() != -1)
                    curves.at(indexCurveSelected())->addControlPoint( float2(
                                                                             x * 2.0 / viewportRect[2] - 1.0,
                                                                             -y * 2.0 / viewportRect[3] + 1.0));
                
            }
            

        }
        
    }else
        if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && curves.size()>0){
                Freeform *closest = closestCurveToMouse(float2(
                                                               x * 2.0 / viewportRect[2] - 1.0,
                                                               -y * 2.0 / viewportRect[3] + 1.0));
                deselectPreviouslySelected();
                if(closest != nullptr)
                    closest->selected = true;
            }
    
    glutPostRedisplay();
}

void movePoint(int mouseX, int mouseY, int index, Freeform *curve){
    curve->getCPoints().at(index).x = mouseX;
    curve->getCPoints().at(index).y = mouseY;
    
    
}


void onMove(int x, int y){
    
    
    int viewportRect[4];
    glGetIntegerv(GL_VIEWPORT, viewportRect);
    
    float mouseX = x * 2.0 / viewportRect[2] - 1.0;
    float mouseY = -y * 2.0 / viewportRect[3] + 1.0;
    
    float2 click = float2(mouseX, mouseY);
    int index = -1;
    
    Freeform *curve = closestCurveToMouse(click);
    
    if(curve != nullptr){
        for(int i=0; i<curve->numControlPoints(); i++){
            if((fabs(click.x - curve->getCPoints().at(i).x) < 0.09f) && (fabs(click.y - curve->getCPoints().at(i).y) < 0.09f)){
                
                index = i;
                break;
            }
        }
    }
    
    if(index != -1){
//        movePoint(mouseX, mouseY, index, curve);
        
        curve->getCPoints().at(index).x = mouseX;
        curve->getCPoints().at(index).y = mouseX;
//        printf("%s %f %f \n", "x and y after move:", curve->getCPoints().at(index).x, curve->getCPoints().at(index).y);
//        printf("%s %f %f \n", "mouse x and y:", mouseX, mouseY);
        
    }
    
    glutPostRedisplay();

    
}


void onIdle() {
    glutPostRedisplay();
}




//--------------------------------------------------------
// The entry point of the application
//--------------------------------------------------------
int main(int argc, char *argv[]) {
    glutInit(&argc, argv);                 // GLUT initialization
    glutInitWindowSize(640, 480); // Initial resolution of the MsWindows Window is 600x600 pixels
    glutInitWindowPosition(100, 100);            // Initial location of the MsWindows window
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);    // Image = 8 bit R,G,B + double buffer + depth buffer
    
    glutCreateWindow("l01_Triangle");        // Window is born
    glutKeyboardFunc(onKeyboard);
    glutReshapeFunc(onReshape);
    glutIdleFunc(onIdle);
    glutDisplayFunc(onDisplay); // Register event handlers
    glutKeyboardUpFunc(onKeyboardUp);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMove);
    
    
    
    
    
    
    glutMainLoop();
    return 0;
}
