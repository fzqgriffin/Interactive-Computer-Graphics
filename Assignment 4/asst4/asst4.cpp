////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////
//	These skeleton codes are later altered by Ming Jin,
//	for "CS6533: Interactive Computer Graphics", 
//	taught by Prof. Andy Nealen at NYU
////////////////////////////////////////////////////////////////////////

// ============================================
// My code
// ============================================

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
// #if __GNUG__
// #   include <tr1/memory>
// #endif

#include <GL/glew.h>
#ifdef __MAC__
#   include <GLUT/glut.h>
#else
#   include <GL/glut.h>
#endif

#include "rigtform.h"
#include "arcball.h"
#include "cvec.h"
#include "matrix4.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"

#include "asstcommon.h"
#include "scenegraph.h"
#include "drawer.h"
#include "picker.h"


using namespace std;      // for string, vector, iostream, and other standard C++ stuff
// using namespace tr1; // for shared_ptr

// G L O B A L S ///////////////////////////////////////////////////

// --------- IMPORTANT --------------------------------------------------------
// Before you start working on this assignment, set the following variable
// properly to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or
// OpenGL 3.x+ with GLSL 1.3.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to
// use GLSL 1.3. Make sure that your machine supports the version of GLSL you
// are using. In particular, on Mac OS X currently there is no way of using
// OpenGL 3.x with GLSL 1.3 when GLUT is used.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
const bool g_Gl2Compatible = true;


static const float g_frustMinFov = 60.0;  // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static const float g_groundY = -2.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;
// ========================================
// TODO: you can add global variables here
// ========================================

// ============================================
// My code
static int Count_view = 0;                // used to cycle between three views
static int Count_object = 0;               // used to cycle between three objects
static int Count_sky_frame = 0;               // used to cycle between two frames for a
static int Count_arcball = 0;

static double g_arcballScreenRadius = 100.0;
static double g_arcballScale = 0.01;
// ============================================


// declare pointers to suitable nodes in the scene graph
static shared_ptr<SgRootNode> g_world;
static shared_ptr<SgRbtNode> g_skyNode, g_groundNode, g_robot1Node, g_robot2Node;
static shared_ptr<SgRbtNode> g_currentPickedRbtNode; // used later when you do picking

static int g_picking = 0;

//struct ShaderState {
//  GlProgram program;
//
//  // Handles to uniform variables
//  GLint h_uLight, h_uLight2;
//  GLint h_uProjMatrix;
//  GLint h_uModelViewMatrix;
//  GLint h_uNormalMatrix;
//  GLint h_uColor;
//
//  // Handles to vertex attributes
//  GLint h_aPosition;
//  GLint h_aNormal;
//
//  ShaderState(const char* vsfn, const char* fsfn) {
//    readAndCompileShader(program, vsfn, fsfn);
//
//    const GLuint h = program; // short hand
//
//    // Retrieve handles to uniform variables
//    h_uLight = safe_glGetUniformLocation(h, "uLight");
//    h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
//    h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
//    h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
//    h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
//    h_uColor = safe_glGetUniformLocation(h, "uColor");
//
//    // Retrieve handles to vertex attributes
//    h_aPosition = safe_glGetAttribLocation(h, "aPosition");
//    h_aNormal = safe_glGetAttribLocation(h, "aNormal");
//
//    if (!g_Gl2Compatible)
//      glBindFragDataLocation(h, 0, "fragColor");
//    checkGlErrors();
//  }
//
//};

static const int PICKING_SHADER = 2; // index of the picking shader is g_shaerFiles
static const int g_numShaders = 3;
static const char * const g_shaderFiles[g_numShaders][2] = {
  {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
  {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"},
    {"./shaders/basic-gl3.vshader", "./shaders/pick-gl3.fshader"}
};
static const char * const g_shaderFilesGl2[g_numShaders][2] = {
  {"./shaders/basic-gl2.vshader", "./shaders/diffuse-gl2.fshader"},
  {"./shaders/basic-gl2.vshader", "./shaders/solid-gl2.fshader"},
    {"./shaders/basic-gl2.vshader", "./shaders/pick-gl2.fshader"}
};
static vector<shared_ptr<ShaderState> > g_shaderStates; // our global shader states

// --------- Geometry

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

// A vertex with floating point position and normal
struct VertexPN {
  Cvec3f p, n;

  VertexPN() {}
  VertexPN(float x, float y, float z,
           float nx, float ny, float nz)
    : p(x,y,z), n(nx, ny, nz)
  {}

  // Define copy constructor and assignment operator from GenericVertex so we can
  // use make* functions from geometrymaker.h
  VertexPN(const GenericVertex& v) {
    *this = v;
  }

  VertexPN& operator = (const GenericVertex& v) {
    p = v.pos;
    n = v.normal;
    return *this;
  }
};

struct Geometry {
  GlBufferObject vbo, ibo;
  int vboLen, iboLen;

  Geometry(VertexPN *vtx, unsigned short *idx, int vboLen, int iboLen) {
    this->vboLen = vboLen;
    this->iboLen = iboLen;

    // Now create the VBO and IBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vboLen, vtx, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen, idx, GL_STATIC_DRAW);
  }

  void draw(const ShaderState& curSS) {
    // Enable the attributes used by our shader
    safe_glEnableVertexAttribArray(curSS.h_aPosition);
    safe_glEnableVertexAttribArray(curSS.h_aNormal);

    // bind vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, p));
    safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, n));

    // bind ibo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // draw!
    glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

    // Disable the attributes used by our shader
    safe_glDisableVertexAttribArray(curSS.h_aPosition);
    safe_glDisableVertexAttribArray(curSS.h_aNormal);
  }
};

typedef SgGeometryShapeNode<Geometry> MyShapeNode;

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_sphere;

// --------- Scene

static const Cvec3 g_light1(2.0, 3.0, 14.0), g_light2(-2, -3.0, -5.0);  // define two lights positions in world space
// ============================================
// TODO: add a second cube's 
// 1. transformation
// 2. color
// ============================================




// ============================================
// My code
// static RigTForm g_objectRbt[2] = {RigTForm(Cvec3(-1,0,0)),RigTForm(Cvec3(1,0,0))};  //currently  2 obj are defined
// static RigTForm groundRbt = RigTForm();  // identity
 static RigTForm Init_arc = RigTForm(Cvec3(0,0,0));
 static RigTForm *g_ArcBallRbt = &Init_arc;
//==============


// my code for step3
// initate eyeRbt, use the skyRbt as the eyeRbt
static RigTForm eyeRbt = RigTForm(Cvec3(0.0, 0.25, 4.0)); //define a;

static RigTForm invEyeRbt = inv(eyeRbt);

static Cvec3f g_objectColors[2] = {Cvec3f(1, 0, 0),Cvec3f(0, 0, 1)};


//==================
// ============================================

///////////////// END OF G L O B A L S //////////////////////////////////////////////////




static void initGround() {
  // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
  VertexPN vtx[4] = {
    VertexPN(-g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
    VertexPN(-g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
  };
  unsigned short idx[] = {0, 1, 2, 0, 2, 3};
  g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}

static void initCubes() {
  int ibLen, vbLen;
  getCubeVbIbLen(vbLen, ibLen);

  // Temporary storage for cube geometry
  vector<VertexPN> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makeCube(1, vtx.begin(), idx.begin());
  g_cube.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initSphere() {
    int ibLen, vbLen;
    getSphereVbIbLen(30, 20, vbLen, ibLen);
    
    // Temporary storage for sphere geometry
    vector<VertexPN> vtx(vbLen);
    vector<unsigned short> idx(ibLen);
    makeSphere(1, 30, 20, vtx.begin(), idx.begin());   // radius, slices, stacks
    g_sphere.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState& curSS, const Matrix4& projMatrix) {
  GLfloat glmatrix[16];
  projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
  safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}

// takes MVM and its normal matrix to the shaders
// static void sendModelViewNormalMatrix(const ShaderState& curSS, const Matrix4& MVM, const Matrix4& NMVM) {
//  GLfloat glmatrix[16];
//  MVM.writeToColumnMajorMatrix(glmatrix); // send MVM    //??????????? how functions
//  safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

//  NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
//  safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
// }

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
  if (g_windowWidth >= g_windowHeight)
    g_frustFovY = g_frustMinFov;
  else {
    const double RAD_PER_DEG = 0.5 * CS175_PI/180;
    g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
  }
}

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
           g_frustFovY, g_windowWidth / static_cast <double> (g_windowHeight),
           g_frustNear, g_frustFar);
}

static int getTheWayToManipulate() {  //To determine which situation we are in
  if(g_currentPickedRbtNode) // There is a object we are manipulating
    return 1;  
  if (Count_view == 0 && Count_sky_frame == 0) // world-sky frame
    return 2;
  else
    return 3;		// ego_motion
}

static RigTForm getArcballRbt() {   //Get the Rbt of the Arcball
  switch (getTheWayToManipulate()) {
  case 1:	// on the center of the object
    return getPathAccumRbt(g_world, g_currentPickedRbtNode);;
    break;
  case 2:   //on the center of the world
    return RigTForm();
    break;
  case 3:
    return getPathAccumRbt(g_world, g_skyNode);
    break;
  default:
    throw runtime_error("Invalid ManipMode");
  }
}


static void drawStuff(const ShaderState& curSS, bool picking) {
    
  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeProjectionMatrix();
  sendProjectionMatrix(curSS, projmat);

  const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1)); // g_light1 position in eye coordinates
  const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
  safe_glUniform3f(curSS.h_uLight, eyeLight1[0], eyeLight1[1], eyeLight1[2]);
  safe_glUniform3f(curSS.h_uLight2, eyeLight2[0], eyeLight2[1], eyeLight2[2]);
  *g_ArcBallRbt = getArcballRbt();					// update the arballRbt everytime we draw stuff
    
//        // draw ground
//        // ===========
//        //
//        Matrix4 MVM = rigTFormToMatrix(invEyeRbt * groundRbt);
//        Matrix4 NMVM = normalMatrix(MVM);
//        sendModelViewNormalMatrix(curSS, MVM, NMVM);
//        safe_glUniform3f(curSS.h_uColor, 0.1, 0.95, 0.1); // set color
//        g_ground->draw(curSS);
    
//        // draw cubes
//        // ==========
//        MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[0]);
//        NMVM = normalMatrix(MVM);
//        sendModelViewNormalMatrix(curSS, MVM, NMVM);
//        safe_glUniform3f(curSS.h_uColor, g_objectColors[0][0], g_objectColors[0][1], g_objectColors[0][2]);
//        g_cube->draw(curSS);
//
//
//        // draw the second cube
//        MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[1]);
//        NMVM = normalMatrix(MVM);
//        sendModelViewNormalMatrix(curSS, MVM, NMVM);
//        safe_glUniform3f(curSS.h_uColor, g_objectColors[1][0], g_objectColors[1][1], g_objectColors[1][2]);
//        g_cube->draw(curSS);
//
//
//        //draw Arcball
//        if (!g_mouseMClickButton && !(g_mouseLClickButton && g_mouseRClickButton))  // only change the g_archallScale when we release the MidButton
//        {
//            g_arcballScale = getScreenToEyeScale((invEyeRbt * *g_ArcBallRbt).getTranslation()[2], g_frustFovY, g_windowHeight);
//        }
//        // manipulate the sky camera with respect to the world-sky coordinate system, or manipulate a cube and it is not with respect to itself
//        if ((((Count_sky_frame == 0 && Count_view == 0)) || (Count_view == 1 && Count_object == 2) || (Count_view == 2 && Count_object ==1)) && Count_arcball == 0){
//            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//
//            MVM = rigTFormToMatrix(invEyeRbt * *g_ArcBallRbt) * Matrix4::makeScale(Cvec3(1,1,1)* g_arcballScale * g_arcballScreenRadius);  //g_arcballScale and g_arcball control the size of the Sphere
//            NMVM = normalMatrix(MVM);
//
//            sendModelViewNormalMatrix(curSS, MVM, NMVM);
//
//            safe_glUniform3f(curSS.h_uColor, 0.37, 0.83, 0.38); // set color
//
//            g_sphere->draw(curSS);
//            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

     if (!g_mouseMClickButton && !(g_mouseLClickButton && g_mouseRClickButton))  // only change the g_archallScale when we release the MidButton
     {
     		Cvec3 arcball_center = g_ArcBallRbt->getTranslation();
     		Cvec3 arcball_center_in_eye = Cvec3( invEyeRbt * Cvec4(arcball_center,1));
            g_arcballScale = getScreenToEyeScale(arcball_center_in_eye[2], g_frustFovY, g_windowHeight);
    }

    if (!picking) {
        Drawer drawer(invEyeRbt, curSS);
        g_world->accept(drawer);

        // draw arcball as part of asst3

        // manipulate the sky camera with respect to the world-sky coordinate system, or manipulate a cube and it is not with respect to itself
        int i = getTheWayToManipulate();
        if (i != 3 && Count_arcball == 0){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            Matrix4 MVM = rigTFormToMatrix(invEyeRbt * *g_ArcBallRbt) * Matrix4::makeScale(Cvec3(1,1,1)* g_arcballScale * g_arcballScreenRadius);  //g_arcballScale and g_arcball control the size of the Sphere
            Matrix4 NMVM = normalMatrix(MVM);
            
            sendModelViewNormalMatrix(curSS, MVM, NMVM);
            
            safe_glUniform3f(curSS.h_uColor, 0.37, 0.83, 0.38); // set color
            
            g_sphere->draw(curSS);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

    }
    else {
        Picker picker(invEyeRbt, curSS);
        g_world->accept(picker);
        glFlush();
        g_currentPickedRbtNode = picker.getRbtNodeAtXY(g_mouseClickX, g_mouseClickY);
        if (g_currentPickedRbtNode == g_groundNode)
            g_currentPickedRbtNode = shared_ptr<SgRbtNode>();   // set to NULL
        else
            //reset picking flag
            g_picking = 0;
        	g_activeShader = 0;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    }
        
    
}

static void pick() {
    // We need to set the clear color to black, for pick rendering.
    // so let's save the clear color
    GLdouble clearColor[4];
    glGetDoublev(GL_COLOR_CLEAR_VALUE, clearColor);
    
    glClearColor(0, 0, 0, 0);
    
    // using PICKING_SHADER as the shader
    glUseProgram(g_shaderStates[PICKING_SHADER]->program);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawStuff(*g_shaderStates[PICKING_SHADER], true);
    
    // Uncomment below and comment out the glutPostRedisplay in mouse(...) call back
    // to see result of the pick rendering pass
    // glutSwapBuffers();
    
    //Now set back the clear color
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    
    checkGlErrors();
}



static void display() {
  glUseProgram(g_shaderStates[g_activeShader]->program);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                   // clear framebuffer color&depth
    
    
    drawStuff(*g_shaderStates[g_activeShader], g_picking==2);


  glutSwapBuffers();                                    // show the back buffer (where we rendered stuff)

  checkGlErrors();
}



static void reshape(const int w, const int h) {
  g_windowWidth = w;
  g_windowHeight = h;
  glViewport(0, 0, w, h);
  g_arcballScreenRadius = 0.25 * min(g_windowWidth, g_windowHeight);
  cerr << "Size of window is now " << w << "x" << h << endl;
  updateFrustFovY();
  glutPostRedisplay();
}

static RigTForm MoveArcball(int x, int y, double dx,double dy){
	*g_ArcBallRbt = getArcballRbt();
    Cvec2 arcball_center= getScreenSpaceCoord(((invEyeRbt * *g_ArcBallRbt).getTranslation()), makeProjectionMatrix(), g_frustNear, g_frustFovY, g_windowWidth, g_windowHeight);             //position of the arcball's center point
    Cvec2 p0 = Cvec2( g_mouseClickX, g_mouseClickY);  // the position of the point p0
    Cvec2 p1 = Cvec2( g_mouseClickX+dx, g_mouseClickY+dy);//(p0[0] + dx, p0[1] + dy);    // the position of the point p1
    Cvec3 vec0, vec1;
    double distance = norm2(p0 - arcball_center);   // The distance between p0 and center point
    if (distance <= g_arcballScreenRadius * g_arcballScreenRadius) {
        vec0 = normalize(Cvec3((p0 - arcball_center), sqrt(g_arcballScreenRadius*g_arcballScreenRadius - distance)));   // when point p0 is on the ball, vec0
    }
    if (distance > g_arcballScreenRadius * g_arcballScreenRadius) {
        vec0 = normalize(Cvec3((p0 - arcball_center), 0));                                     // when point p0 is not on the ball, vec0
    }
    distance = norm2(p1 - arcball_center);   // The distance between p1 and center point
    if (distance <= g_arcballScreenRadius * g_arcballScreenRadius) {
        vec1 = normalize(Cvec3((p1 - arcball_center), sqrt(g_arcballScreenRadius*g_arcballScreenRadius-distance)));   // when point p1 is on the ball, vec1
    }
    else
        vec1 = normalize(Cvec3((p1 - arcball_center), 0));                                     // when point p1 is not on the ball, vec1
    RigTForm result = RigTForm(Quat(0.0, vec1[0], vec1[1], vec1[2]) * Quat(0.0, -vec0[0], -vec0[1], -vec0[2]));

    return result;

}

static RigTForm getMotionRbt(const int x, const int y, const double dx, const double dy) // get the Rbt of the motion
{
	RigTForm m;
	  if (g_mouseLClickButton && !g_mouseRClickButton) { // left button down?
      m = MoveArcball( x, y, dx, dy);			
  }
  else if (g_mouseRClickButton && !g_mouseLClickButton) { // right button down?
    m = RigTForm(Cvec3(dx, dy, 0) * 0.01);                             // mouse go down, itmes go up
    *g_ArcBallRbt = getArcballRbt();
  }
  else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton)) {  // middle or (left and right) button down?
    m = RigTForm(Cvec3(0, 0, -dy) * 0.01);
  }
  switch(getTheWayToManipulate()){
  	case 1:
  		  if(Count_view == 1){ 
  			if(g_currentPickedRbtNode == g_robot1Node){
  				if (g_mouseLClickButton)
  				{
  					m = RigTForm(Quat::makeXRotation(dy) * Quat::makeYRotation(-dx));

  				}

  			}
  			}	
  		  if (Count_view == 2)
  		  {
  		  	if (g_currentPickedRbtNode == g_robot2Node)
  		  	{
  		  		if (g_mouseLClickButton)
  		  		{
  		  			m = RigTForm(Quat::makeXRotation(dy) * Quat::makeYRotation(-dx));
  		  		}

  		  	}
  		  }
  		break;
  	case 2:								//world-sky
  		  if (g_mouseLClickButton ) {   
  		  			m = MoveArcball(x,y,-dx,-dy);
          }
          if (g_mouseRClickButton ) {
                    m = RigTForm(Cvec3(-dx, -dy, 0) * 0.01);
          }
          break;
    case 3:
    	  if (g_mouseLClickButton) {    //egomotion
                    m = RigTForm(Quat::makeXRotation(dy) * Quat::makeYRotation(-dx));
          }
          break;
	}
	return m;
}

static RigTForm MixFrame(const RigTForm& objRbt, const RigTForm& eyeRbt) {  // create the Mixed Frame
    return transFact(objRbt) * linFact(eyeRbt);
}

static RigTForm doMtoOwrtA(const RigTForm& M, const RigTForm& O, const RigTForm& A) {  //apply motion m to object o with rbt O refering to frame with rbt A
  return A * M * inv(A) * O;
}

static RigTForm getEyeRbt(int Count_view){
	shared_ptr<SgRbtNode> view;
	switch(Count_view){
		case 0:
		view = g_skyNode;
		eyeRbt = getPathAccumRbt(g_world,view);
		break;
		case 1:
		view = g_robot1Node;
		eyeRbt = getPathAccumRbt(g_world, view);
		break;
		case 2:
		view = g_robot2Node;
		eyeRbt =  getPathAccumRbt(g_world, view);
		break;
	}
	return eyeRbt;

}

static void motion(const int x, const int y) {
  const double dx = x - g_mouseClickX;
  const double dy = g_windowHeight - y - 1 - g_mouseClickY;

  RigTForm m =getMotionRbt(x,y,dx,dy);
  RigTForm A;
  shared_ptr<SgRbtNode> object;
  switch(getTheWayToManipulate()){
  	case 1: // manipulate a object
  		object = g_currentPickedRbtNode;
  		eyeRbt = getEyeRbt(Count_view);
  		invEyeRbt = inv(eyeRbt);
  		A = MixFrame(object->getRbt(),eyeRbt);
  		break;
  	case 2: // world-sky
  		object = g_skyNode;
  		eyeRbt = getEyeRbt(Count_view);
  		A = MixFrame(RigTForm(),eyeRbt);
  		invEyeRbt = inv(eyeRbt);
  		break;

  	case 3:	
  		if (Count_view == 0)	//sky-sky frame
  		{
  			object = g_skyNode;
  			eyeRbt = getEyeRbt(Count_view);
  			A = MixFrame(eyeRbt,eyeRbt);
  			invEyeRbt = inv(eyeRbt);
  		}
  		else
  			return;
  		break;
  }

  object->setRbt(doMtoOwrtA(m,object->getRbt(),A));


//     if (g_mouseClickDown &&  Count_view == 0 ) {                      // view is sky_camera, world-sky frame
//         if (Count_object == 0) {                                            //object is the world
//             if (Count_sky_frame == 0) {                                     //  center point is the world

// //                RigTForm relative_frame = RigTForm(g_skyRbt.getRotation());
// //                g_skyRbt = relative_frame * m * inv(relative_frame) * g_skyRbt;
// //                eyeRbt = g_skyRbt;
// //                invEyeRbt = inv(eyeRbt);
                
//                 RigTForm A = MixFrame(RigTForm(),getPathAccumRbt(g_world,g_skyNode));
//                 g_skyNode->setRbt(doMtoOwrtA(m, g_skyNode->getRbt(),A));
//                 eyeRbt = getPathAccumRbt(g_world, g_skyNode);
//                 invEyeRbt = inv(eyeRbt);

//                 glutPostRedisplay(); // we always redraw if we changed the scene
//             }
//             if (Count_sky_frame == 1) {                                     // sky-sky frame
                
// //                RigTForm relative_frame = RigTForm(g_skyRbt.getTranslation(), g_skyRbt.getRotation());
// //                g_skyRbt = relative_frame * m * inv(relative_frame) * g_skyRbt;
// //                eyeRbt = g_skyRbt;
// //                invEyeRbt = inv(eyeRbt);
                
                
//                 RigTForm A = MixFrame(getPathAccumRbt(g_world,g_skyNode),getPathAccumRbt(g_world,g_skyNode));
//                 g_skyNode->setRbt(doMtoOwrtA(m,g_skyNode->getRbt(),A));
//                  eyeRbt = getPathAccumRbt(g_world, g_skyNode);
//                  invEyeRbt = inv(eyeRbt);
                
                // glutPostRedisplay();
//             }
//         }
//         if (Count_object == 1) {                                     // object is cube 1

// //            RigTForm relative_frame = RigTForm(g_objectRbt[0].getTranslation(),g_skyRbt.getRotation());
// //            g_objectRbt[0] = relative_frame * m * inv(relative_frame) * g_objectRbt[0];
// //            eyeRbt = g_skyRbt;
// //            invEyeRbt = inv(eyeRbt);
//              RigTForm eyeRbt = getPathAccumRbt(g_world, g_skyNode,1);
//              RigTForm invEyeRbt = inv(eyeRbt);

//             glutPostRedisplay();
//         }
//         if (Count_object == 2) {                                     // object is cube 2

// //            RigTForm relative_frame = RigTForm(g_objectRbt[1].getTranslation(), g_skyRbt.getRotation());
// //            g_objectRbt[1] = relative_frame * m * inv(relative_frame) * g_objectRbt[1];
// //            eyeRbt = g_skyRbt;
// //            invEyeRbt = inv(eyeRbt);
//              RigTForm eyeRbt = getPathAccumRbt(g_world, g_skyNode,1);
//              RigTForm invEyeRbt = inv(eyeRbt);

//             glutPostRedisplay();
            
//         }
//     }
    
//     if (g_mouseClickDown && Count_view == 1 ) {                   // view is cube 1
//         if (Count_object == 0) {                                     // object is sky-camera
//             cout << "view is in cube 1 now, cannot move the camera" << endl;
//         }
//         if (Count_object == 1) {                                     // object is cube 1
//             if (g_mouseLClickButton) {
//                 m = MoveArcball( x, y, -dx, -dy);
//             }
// //            RigTForm relative_frame = RigTForm(g_objectRbt[0].getTranslation(),g_objectRbt[0].getRotation());
// //            g_objectRbt[0] = relative_frame * m * inv(relative_frame) * g_objectRbt[0];
// //            eyeRbt = g_objectRbt[0];
// //            invEyeRbt = inv(eyeRbt);

//             glutPostRedisplay();
//         }
//         if (Count_object == 2) {                                     // object is cube 2
// //            RigTForm relative_frame = RigTForm(g_objectRbt[1].getTranslation(), g_objectRbt[0].getRotation());
// //            g_objectRbt[1] = relative_frame * m * inv(relative_frame) * g_objectRbt[1];
// //            eyeRbt = g_objectRbt[0];
// //            invEyeRbt = inv(eyeRbt);

//             glutPostRedisplay();

//         }
//     }
//     if (g_mouseClickDown && Count_view == 2 ) {                   // view is cube2
//         if (Count_object == 0) {                                     // object is sky-camera
//             cout << "view is in cube 1 now, cannot move the camera" << endl;
//         }
//         if (Count_object == 1) {                                     // object is cube 1
// //            RigTForm relative_frame = RigTForm(g_objectRbt[0].getTranslation(), g_objectRbt[1].getRotation());
// //            g_objectRbt[0] = relative_frame * m * inv(relative_frame) * g_objectRbt[0];
// //            eyeRbt = g_objectRbt[1];
// //            invEyeRbt = inv(eyeRbt);


//             glutPostRedisplay();
//         }
//         if (Count_object == 2) {                                     // object is cube 2
//             if (g_mouseLClickButton) {
//                 m = MoveArcball( x, y, -dx, -dy);
//             }
// //            RigTForm relative_frame = RigTForm(g_objectRbt[1].getTranslation(), g_objectRbt[1].getRotation());
// //            g_objectRbt[1] = relative_frame * m * inv(relative_frame) * g_objectRbt[1];
// //            eyeRbt = g_objectRbt[1];
// //            invEyeRbt = inv(eyeRbt);


//             glutPostRedisplay();
            
//         }
//     }
    
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;
   glutPostRedisplay();
}

// insert contructRobot() and initScene() function
static void constructRobot(shared_ptr<SgTransformNode> base, const Cvec3& color) {  //define how to draw the robot. The body part and join point.

  const double ARM_LEN = 0.7,
               ARM_THICK = 0.25,
               TORSO_LEN = 1.5,
               TORSO_THICK = 0.25,
               TORSO_WIDTH = 1,
               LEG_LEN = 1.0,
               LEG_THICK = 0.25,
               HEAD_SIZE = 0.4;
  const int NUM_JOINTS = 10,
            NUM_SHAPES = 10;

  struct JointDesc {
    int parent;
    float x, y, z;
  };

  JointDesc jointDesc[NUM_JOINTS] = {   // first variable is the number of the jointDesc. [1]to[3] are coordinates of the jointDesc, which are relative.
    {-1}, // torso
    {0,  TORSO_WIDTH/2, TORSO_LEN/2, 0}, // upper right arm
    {1,  ARM_LEN, 0, 0}, // lower right arm
    {0,  -TORSO_WIDTH/2, TORSO_LEN/2, 0}, // upper left arm
    {3,  -ARM_LEN, 0, 0}, // lower left arm
    {0,  TORSO_WIDTH/4, -TORSO_LEN/2, 0}, // upper right leg 
    {5,  0, -LEG_LEN, 0}, // lower right leg 
    {0,  -TORSO_WIDTH/4, -TORSO_LEN/2, 0}, // upper left leg 
    {7,  0, -LEG_LEN, 0}, // lower left leg 
    {0, 0, TORSO_LEN/2, 0} // neck
  };

  struct ShapeDesc {
    int parentJointId;
    float x, y, z, sx, sy, sz;
    shared_ptr<Geometry> geometry;
  };

  ShapeDesc shapeDesc[NUM_SHAPES] = {
    {0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube}, // torso
    {1, ARM_LEN/2, 0, 0, ARM_LEN/2, ARM_THICK/2, ARM_THICK/2, g_sphere}, // upper right arm
    {2, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower right arm
    {3, -ARM_LEN/2, 0, 0, ARM_LEN/2, ARM_THICK/2, ARM_THICK/2, g_sphere}, // upper left arm
    {4, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower left arm
    {5, 0, -LEG_LEN/2, 0, LEG_THICK/2, LEG_LEN/2, LEG_THICK/2, g_sphere}, // upper right leg
    {6, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower right leg
    {7, 0, -LEG_LEN/2, 0, LEG_THICK/2, LEG_LEN/2, LEG_THICK/2, g_sphere}, // upper left leg 
    {8, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower left leg 
    {9, 0, 1.1*HEAD_SIZE, 0, HEAD_SIZE, HEAD_SIZE, HEAD_SIZE, g_sphere} //head
  };

  shared_ptr<SgTransformNode> jointNodes[NUM_JOINTS];    

    
    for (int i = 0; i < NUM_JOINTS; ++i) {
        if (jointDesc[i].parent == -1)
            jointNodes[i] = base;
        else {
            jointNodes[i].reset(new SgRbtNode(RigTForm(Cvec3(jointDesc[i].x, jointDesc[i].y, jointDesc[i].z))));
            jointNodes[jointDesc[i].parent]->addChild(jointNodes[i]);
        }
    }
    for (int i = 0; i < NUM_SHAPES; ++i) {
        shared_ptr<MyShapeNode> shape(
                                      new MyShapeNode(shapeDesc[i].geometry,
                                                      color,
                                                      Cvec3(shapeDesc[i].x, shapeDesc[i].y, shapeDesc[i].z),
                                                      Cvec3(0, 0, 0),
                                                      Cvec3(shapeDesc[i].sx, shapeDesc[i].sy, shapeDesc[i].sz)));
        jointNodes[shapeDesc[i].parentJointId]->addChild(shape);
    }
}

static void initScene() {				// init all nodes, and construct the robot
    g_world.reset(new SgRootNode());
    
    g_skyNode.reset(new SgRbtNode(RigTForm(Cvec3(0.0, 0.25, 4.0))));
    
    g_groundNode.reset(new SgRbtNode());
    g_groundNode->addChild(shared_ptr<MyShapeNode>(
                                                   new MyShapeNode(g_ground, Cvec3(0.1, 0.95, 0.1))));
    	
    g_robot1Node.reset(new SgRbtNode(RigTForm(Cvec3(-2, 1, 0))));
    g_robot2Node.reset(new SgRbtNode(RigTForm(Cvec3(2, 1, 0))));
    
    constructRobot(g_robot1Node, Cvec3(1, 0, 0)); // a Red robot
    constructRobot(g_robot2Node, Cvec3(0, 0, 1)); // a Blue robot
    
    g_world->addChild(g_skyNode);
    g_world->addChild(g_groundNode);
    g_world->addChild(g_robot1Node);
    g_world->addChild(g_robot2Node);
    
    
}

static void reset()
{
	// =========================================================
	// TODO:
	// - reset g_skyRbt and g_objectRbt to their default values
	// - reset the views and manipulation mode to default
	// - reset sky camera mode to use the "world-sky" frame
	// =========================================================
    
    // =========================================================
    // my code
    // g_skyRbt = RigTForm(Cvec3(0.0, 0.25, 4.0));
    // g_objectRbt[0] = RigTForm(Cvec3(-1,0,0));
    // g_objectRbt[1] = RigTForm(Cvec3(1,0,0));
    // groundRbt =  RigTForm();
    // g_ArcBallRbt = &g_wolrdRbt; //new RigTForm(Cvec3(0,0,0));

    Count_view = 0;
    Count_object = 0;
    Count_sky_frame = 0;
    g_currentPickedRbtNode = shared_ptr<SgRbtNode>();
    *g_ArcBallRbt = RigTForm();
    eyeRbt = g_skyNode->getRbt();
    invEyeRbt = inv(eyeRbt);
    initScene();
    

      glutPostRedisplay();
      glFlush();
	cout << "reset objects and modes to defaults" << endl;
}

static void mouse(const int button, const int state, const int x, const int y) {
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;  // conversion from GLUT window-coordinate-system to OpenGL window-coordinate-system

  g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
  g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
  g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

  g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
  g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
  g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

  if (g_mouseLClickButton && g_picking == 1) {
    g_picking = 2;
    g_activeShader = PICKING_SHADER;
    cerr << "Has been clicked!" << endl;
    pick();
    return;
  }

  g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;
  glutPostRedisplay();
}


static void keyboard(const unsigned char key, const int x, const int y) {
  switch (key) {
  case 27:
    exit(0);                                  // ESC
  case 'h':
    cout << " ============== H E L P ==============\n\n"
    << "h\t\thelp menu\n"
    << "s\t\tsave screenshot\n"
    << "f\t\tToggle flat shading on/off.\n"
    << "o\t\tCycle object to edit\n"
    << "v\t\tCycle view\n"
    << "drag left mouse to rotate\n" << endl;
    break;
  case 's':
    glFlush();
    writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
    break;
  case 'f':
    g_activeShader ^= 1;
    break;
  // ============================================================
  // TODO: add the following functionality for 
  //       keybaord inputs
  // - 'v': cycle through the 3 views
  // - 'o': cycle through the 3 objects being manipulated
  // - 'm': switch between "world-sky" frame and "sky-sky" frame
  // - 'r': reset the scene
  // ============================================================
  case 'v':
          Count_view=Count_view+1;
          if (Count_view==3) {
              Count_view=0;
          }
          if (Count_view==0) {
              eyeRbt = g_skyNode->getRbt();
              invEyeRbt = inv(eyeRbt);
              cout << "view frame is sky\n"<<endl;
          }
          if (Count_view==1) {
              eyeRbt = g_robot1Node->getRbt();
;
              invEyeRbt = inv(eyeRbt);
              cout << "view frame is robot1\n"<<endl;
          }
          if (Count_view==2) {
              eyeRbt = g_robot2Node->getRbt();;
              invEyeRbt = inv(eyeRbt);
              cout << "view frame is robot2\n"<<endl;          }
          break;
  // case 'o':
  //         Count_object=Count_object+1;
  //         if (Count_object==3) {
  //             Count_object=0;
  //             cout << " manipulating object sky camera\n"<<endl;
  //             g_ArcBallRbt = &g_wolrdRbt;
              
  //         }
  //         if (Count_object==1) {
  //             cout << " manipulating object cube 1\n"<<endl;
  //             g_ArcBallRbt = &g_objectRbt[0];
  //         }
  //         if (Count_object==2) {
  //             cout << " manipulating object cube 2\n"<<endl;
  //             g_ArcBallRbt = &g_objectRbt[1];
  //         }
  //         break;
  case 'p':
  		   g_picking = 1;
  		   cout << "Ready to pick\n"<<endl;
  		   break;
  case 'm':
          Count_sky_frame=Count_sky_frame+1;
          if (Count_sky_frame==2) {
              Count_sky_frame=0;
              cout << " now is world-sky frame\n"<<endl;
              *g_ArcBallRbt = RigTForm();
          }
          if (Count_sky_frame==1) {
              cout << " now is sky-sky frame\n"<<endl;
          }
          break;
  case 'a':
          Count_arcball = Count_arcball + 1;
          if (Count_arcball==2) {
              Count_arcball=0;
              cout << " draw the arcball\n"<<endl;
          }
          if (Count_arcball==1) {
              cout << " don't draw the arcball\n"<<endl;
          }
          break;
  case 'r':
          reset();
          break;

  }
  glutPostRedisplay();
}

static void initGlutState(int argc, char * argv[]) {
  glutInit(&argc, argv);                                  // initialize Glut based on cmd-line args
  glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);  //  RGBA pixel channels and double buffering
  glutInitWindowSize(g_windowWidth, g_windowHeight);      // create a window
  glutCreateWindow("Assignment 2");                       // title the window

  glutDisplayFunc(display);                               // display rendering callback
  glutReshapeFunc(reshape);                               // window reshape callback
  glutMotionFunc(motion);                                 // mouse movement callback
  glutMouseFunc(mouse);                                   // mouse click callback
  glutKeyboardFunc(keyboard);
}

static void initGLState() {
  glClearColor(128./255., 200./255., 255./255., 0.);
  glClearDepth(0.);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glReadBuffer(GL_BACK);
  if (!g_Gl2Compatible)
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShaders() {
  g_shaderStates.resize(g_numShaders);
  for (int i = 0; i < g_numShaders; ++i) {
    if (g_Gl2Compatible)
      g_shaderStates[i].reset(new ShaderState(g_shaderFilesGl2[i][0], g_shaderFilesGl2[i][1]));
    else
      g_shaderStates[i].reset(new ShaderState(g_shaderFiles[i][0], g_shaderFiles[i][1]));
  }
}

static void initGeometry() {
  initGround();
  initCubes();
    initSphere();
}




int main(int argc, char * argv[]) {
  try {
    initGlutState(argc,argv);

    glewInit(); // load the OpenGL extensions

    cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3") << endl;
    if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
    else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");

    initGLState();
    initShaders();
    initGeometry();
    initScene();
    glutMainLoop();
    return 0;
  }
  catch (const runtime_error& e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}
