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

		XnChar g_strPose[20];
		XnBool g_bNeedPose;
		XnInt nId;
 
                ofTexture	texDepth;
                ofTexture	texColor;
                ofTexture	texUser;
	
		ofxShader	shader;
	
		unsigned int	nbVertices;
		unsigned int	sizeVertices;

		MSA::Shape3D	pointCloud;

                int             width;
		int		height;

		XnDouble fXtoZ;
		XnDouble fYtoZ;

                const XnDepthPixel*	depthPixels;
                const XnUInt8*	colorPixels;
                const XnLabel*	userPixels;

	public:
	
		void setup();
		
		void init();
		void initShape();
		
		void update();
		void updateShape();
		
		void drawTexture();
		void drawShape();
		
		inline DepthGenerator* getDepthGenerator(){return &g_depth;};
		inline ImageGenerator* getImageGenerator(){return &g_image;};
		inline UserGenerator* getUserGenerator(){return &g_user;};

		inline ofTexture* getTedDepth(){return &texDepth;};
		inline ofTexture* getTexColor(){return &texColor;};
		inline ofTexture* getTexUser(){return &texUser;};
};

