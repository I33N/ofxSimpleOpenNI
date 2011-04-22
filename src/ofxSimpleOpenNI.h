#pragma once

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
//#include <XnOS.h>
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>

#define SAMPLE_XML_PATH "data/Config.xml"
#define SAMPLE_LICENSE_PATH "data/License.xml"
#define SAMPLE_RECORDING_PATH "data/Captured.oni"


#define WIDTH 640
#define HEIGHT 480
#define NBPOINTS WIDTH*HEIGHT

#define CHECK_RC(nRetVal, what)							\
	if (nRetVal != XN_STATUS_OK)						\
	{									\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));	\
	}

using namespace xn;

#include "ofMain.h"
#include "MSAShape3D.h"
#include "ofxShader.h"

enum ShapeType {POINTCLOUD,SPHERECLOUD,SPLATCLOUD,TRIANGLE};

class ofxSimpleOpenNI
{

	public:

		Context g_context;

		DepthGenerator g_depth;
		ImageGenerator g_image;
		UserGenerator  g_user;

		DepthMetaData g_depthMD;
		ImageMetaData g_imageMD;
		SceneMetaData g_sceneMD;

		XnBool g_bNeedPose;
		XnBool g_bFromRecording;
		
		XnChar g_strPose[20];
		XnInt nId;
 
                ofTexture	texDepth;
                ofTexture	texColor;
                ofTexture	texUser;
	
		ofxShader	shader;
	
		MSA::Shape3D	pointCloud;
		MSA::Shape3D	splatCloud;
		MSA::Shape3D	sphereCloud;
		MSA::Shape3D	mesh;

                int             width;
		int		height;

		XnDouble fXtoZ;
		XnDouble fYtoZ;

                const XnDepthPixel*	depthPixels;
                const XnUInt8*		colorPixels;
                const XnLabel*		userPixels;

	public:
	
		void setup(bool fromRecording=false);
		void setupOpenNI(bool fromRecording=false);
		void setupShape();
		void setupTexture();
		void setupShader();
	
		void init();
		void initShapePoints();
		void initShapeTriangles();
		void initShapeQuads();
		void initTexture();		

		void update();
		void updateOpenNI();
		//void updateShapePoints(bool worldSpace=false);
		void updateTexture();		

		void draw(ShapeType shapeType = POINTCLOUD);
		void drawShape(ShapeType shapeType = POINTCLOUD);
		void drawTexture();

		ofxShader* getShader(){return &shader;};
		void resetShader();
		
		inline int getWidth(){return width;};
		inline int getHeight(){return height;};

		inline DepthGenerator* getDepthGenerator(){return &g_depth;};
		inline ImageGenerator* getImageGenerator(){return &g_image;};
		inline UserGenerator* getUserGenerator(){return &g_user;};

		inline ofTexture* getTexDepth(){return &texDepth;};
		inline ofTexture* getTexColor(){return &texColor;};
		inline ofTexture* getTexUser(){return &texUser;};
};

