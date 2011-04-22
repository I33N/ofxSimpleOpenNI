#include "ofxSimpleOpenNI.h"

// Callback: New user
void XN_CALLBACK_TYPE User_NewUser(UserGenerator& generator, XnUserID nId, void* pCookie)
{
	ofxSimpleOpenNI* app = (ofxSimpleOpenNI*) pCookie;

	printf("New User %d\n", nId);

	//START POSE DETECTION OR CALIBRATION DEPENDING IF POSE IS NEEDED
	if (app->g_bNeedPose)
	{
		app->g_user.GetPoseDetectionCap().StartPoseDetection(app->g_strPose, nId);
	}
	else
	{
		app->g_user.GetSkeletonCap().RequestCalibration(nId,true);
	}
}

// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("Lost user %d\n", nId);
}

// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	ofxSimpleOpenNI* app = (ofxSimpleOpenNI*) pCookie;
	
	printf("Pose %s detected for user %d\n", strPose, nId);

	//STOP POSE DETECTION AND START CALIBRATING
	app->g_user.GetPoseDetectionCap().StopPoseDetection(nId);
	app->g_user.GetSkeletonCap().RequestCalibration(nId, true);
}

// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(SkeletonCapability& capability, XnUserID nId, void* pCookie)
{	
	printf("Calibration started for user %d\n", nId);
}

// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	ofxSimpleOpenNI* app = (ofxSimpleOpenNI*) pCookie;
	
	if (bSuccess)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		//START TO TRACK THE USER
		app->g_user.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		//START POSE DETECTION OR CALIBRATION DEPENDING IF POSE IS NEEDED
		if (app->g_bNeedPose)
		{
			app->g_user.GetPoseDetectionCap().StartPoseDetection(app->g_strPose, nId);
		}
		else
		{
			app->g_user.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

//--------------------------------------------------------------
void ofxSimpleOpenNI::setup(bool fromRecording)
{
        width = WIDTH;
        height = HEIGHT;
      
	setupOpenNI(fromRecording); 
	setupShader();	
	setupShape();
	setupTexture();
}


void ofxSimpleOpenNI::setupOpenNI(bool fromRecording)
{
	//INIT FLAGS
	g_bFromRecording = fromRecording;
	g_bNeedPose = false;

	XnStatus rc;
        EnumerationErrors errors;
      
	//OPEN XML FILE 
	if(!g_bFromRecording) 
	{
		//Case live camera -> standard xml file
		rc = g_context.InitFromXmlFile(SAMPLE_XML_PATH, &errors);
	}
	else
	{
		//Case recording -> licence xml file
		rc = g_context.InitFromXmlFile(SAMPLE_LICENSE_PATH, &errors);
		rc = g_context.RunXmlScriptFromFile(SAMPLE_LICENSE_PATH);
        }

	//CHECK XML FILE
	if (rc == XN_STATUS_NO_NODE_PRESENT)
        {
                XnChar strError[1024];
                errors.ToString(strError, 1024);
                printf("%s\n", strError);
        }
        else if (rc != XN_STATUS_OK)
        {
                printf("Open failed: %s\n", xnGetStatusString(rc));
        }

	//IF RECORDING OPEN IT
	if(g_bFromRecording) 
        	rc = g_context.OpenFileRecording(SAMPLE_RECORDING_PATH);         
        
	//FIND DEPTH/IMAGE NODES
	rc = g_context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_depth);
        rc = g_context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);

	//FIND OR CREATE USER NODE
	rc = g_context.FindExistingNode(XN_NODE_TYPE_USER, g_user);
	if (rc != XN_STATUS_OK)
	{
		rc = g_user.Create(g_context);
		CHECK_RC(rc, "Find user generator");
	}

	//REGISTER CALLBACKS
	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;

	//USER CALLBACK
	if (!g_user.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
	}
	g_user.RegisterUserCallbacks(User_NewUser,User_LostUser,this, hUserCallbacks);

	//SKELETON CALLBACK
	g_user.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart,UserCalibration_CalibrationEnd,this,hCalibrationCallbacks);
	
	//CHECK IF PI POSE IS NEEDED FOR CALIBRATION AND REGISTER CALLBACK IF SO
	if (g_user.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = true;
		if (!g_user.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
		}
		g_user.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected,NULL,this,hPoseCallbacks);
		g_user.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	//SELECT FULL BODY SKELETON
	g_user.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
/*	
	if (!g_user.IsCapabilitySupported(XN_CAPABILITY_USER_POSITION))
	{
		printf("Supplied user generator doesn't support position\n");
	}
	else
	{
		printf("Position OK\n");
	}
*/
	//START THE GENERATION
	rc = g_context.StartGeneratingAll();

        //TEST GENERATION
	rc = g_context.WaitAndUpdateAll();
        if (rc != XN_STATUS_OK)
        {
                printf("Read failed: %s\n", xnGetStatusString(rc));
        }

	//MAKE SURE DEPTH FRAME IS MAPPED TO IMAGE FRAME
        g_depth.GetAlternativeViewPointCap().SetViewPoint(g_image);

	//GET CALIBRATION OF DEPTH CAMERA (used in shader)
	XnFieldOfView FOV;
	xnGetDepthFieldOfView(g_depth, &FOV);

	fXtoZ = tan(FOV.fHFOV/2)*2;
	fYtoZ = tan(FOV.fVFOV/2)*2;
}

void ofxSimpleOpenNI::setupShape()
{
	//TODO SWITCH on parameter
	initShapePoints();
	initShapeTriangles();
	//initShapeQuads();
}

void ofxSimpleOpenNI::setupTexture()
{
	texDepth.allocate(width,height,GL_LUMINANCE16);
        texColor.allocate(width,height,GL_RGB);
        texUser.allocate(width,height,GL_LUMINANCE16);

	texDepth.texData.pixelType = GL_UNSIGNED_SHORT;
	texDepth.texData.glType = GL_LUMINANCE;
	texDepth.texData.glTypeInternal = GL_LUMINANCE16;
	
	//texColor.texData.pixelType = GL_UNSIGNED_BYTE;
	//texColor.texData.glType = GL_RGB;
	//texColor.texData.glTypeInternal = GL_RGB;
	
	texUser.texData.pixelType = GL_UNSIGNED_SHORT;
	texUser.texData.glType = GL_LUMINANCE;
	texUser.texData.glTypeInternal = GL_LUMINANCE16;
	
	texDepth.setTextureMinMagFilter(GL_LINEAR,GL_NEAREST);
	texColor.setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
	texUser.setTextureMinMagFilter(GL_LINEAR,GL_NEAREST);

	ofDisableTextureEdgeHack();	
}

void ofxSimpleOpenNI::setupShader()
{
	shader.setup(string("myShader"),string("myShader"));
	//shader.setup(string("myShader"),string("myShader"),string("myShader"),GL_POINTS,GL_POINTS,1);

	//shader.setGeometryInputType(GL_POINTS);
	//shader.setGeometryOutputType(GL_POINTS);
	//shader.setGeometryNbOutputVertices(1);
}

//--------------------------------------------------------------
void ofxSimpleOpenNI::initShapePoints()
{
	pointCloud.enableNormal(false);
	pointCloud.enableColor(false);
	pointCloud.enableTexCoord(true);
	
	pointCloud.reserve(width*height);


	pointCloud.begin(GL_POINTS);

	unsigned int step = 1;
       
	for(int y = 0; y < height; y += step)
	{
      		for(int x = 0; x < width; x += step)
		{
			pointCloud.setTexCoord(x,y);
			pointCloud.addVertex(0,0);
		}
	}
	pointCloud.end();
}

void ofxSimpleOpenNI::initShapeTriangles()
{
	unsigned int step = 1;
	
	mesh.enableNormal(false);
	mesh.enableColor(false);
	mesh.enableTexCoord(true);
	
	mesh.reserve(floor(width/step)*floor(height/step)*2);

	mesh.begin(GL_TRIANGLE_STRIP);

       
	for(int y = 0; y < height; y += step)
	{
      		for(int x = 0; x < width; x += step)
		{
			mesh.setTexCoord(x,y);
			//mesh.addVertex(x,y);
			mesh.addVertex(0,0);
			mesh.setTexCoord(x,y+step);
			//mesh.addVertex(x,y+step);
			mesh.addVertex(0,step);
		}
	}
	mesh.end();
}

void ofxSimpleOpenNI::initShapeQuads()
{
	unsigned int step = 4;

	splatCloud.enableNormal(false);
	splatCloud.enableColor(false);
	splatCloud.enableTexCoord(true);
	
	splatCloud.reserve(floor(width/step)*floor(height/step)*4);

	splatCloud.begin(GL_QUADS);

	float size=1.0;

	for(int y = 0; y < height; y += step)
	{
      		for(int x = 0; x < width; x += step)
		{
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(-size,+size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(+size,+size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(+size,-size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(-size,-size);
		}
	}
	splatCloud.end();
}

/*
void ofxSimpleOpenNI::initShapeTrianglesSplat()
{
	unsigned int step = 2;

	splatCloud.enableNormal(false);
	splatCloud.enableColor(false);
	splatCloud.enableTexCoord(true);
	
	splatCloud.reserve(floor(width/step)*floor(height/step)*6);
	//splatCloud.reserve(floor(width/step)*floor(height/step)*4);

	splatCloud.begin(GL_TRIANGLES);
	//splatCloud.begin(GL_QUADS);

	float size=1.0;

	for(int y = 0; y < height; y += step)
	{
      		for(int x = 0; x < width; x += step)
		{
			splatCloud.setTexCoord(x,y);	
			splatCloud.addVertex(-size,-size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(+size,+size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(-size,+size);

			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(-size,-size);
			splatCloud.setTexCoord(x,y);	
			splatCloud.addVertex(+size,-size);
			splatCloud.setTexCoord(x,y);
			splatCloud.addVertex(+size,+size);
		}
	}
	splatCloud.end();
}
*/

//--------------------------------------------------------------
void ofxSimpleOpenNI::update()
{
	updateOpenNI();
	//updateShapePoints(false);
	updateTexture();
}

/*
void ofxSimpleOpenNI::updateShapePoints(bool worldSpace)
{
	pointCloud.begin(GL_POINTS);

	unsigned int step = 1;
       
	for(int y = 0; y < height; y += step)
	{
      		for(int x = 0; x < width; x += step)
		{
			XnPoint3D pt;
			pt.X=x;
			pt.Y=y;
			pt.Z=g_depthMD(x,y);
			
			if(worldSpace)
				g_depth.ConvertProjectiveToRealWorld(1,&pt,&pt);

			pointCloud.addVertex(pt.X,pt.Y,pt.Z);
			pointCloud.setTexCoord(x,y);
		}
	}
	pointCloud.end();
}
*/

void ofxSimpleOpenNI::updateOpenNI()
{
        XnStatus rc;
        
	//READ A NEW FRAME
        //rc = g_context.WaitAnyUpdateAll();
	rc = g_context.WaitAndUpdateAll();
        
	//CHECK IT
	if (rc != XN_STATUS_OK)
        {
                printf("Read failed: %s\n", xnGetStatusString(rc));
        }

	//GET FRAMES METADATA
        g_depth.GetMetaData(g_depthMD);
        g_image.GetMetaData(g_imageMD);
	g_user.GetUserPixels(0,g_sceneMD);
}

void ofxSimpleOpenNI::updateTexture()
{
	//GET PIXEL BUFFER REFERENCES
      	depthPixels = g_depthMD.Data();
        colorPixels = g_imageMD.Data();
	userPixels = g_sceneMD.Data();

	//LOAD THEM INTO TEXTURE
	texDepth.loadData((unsigned char*)depthPixels,width,height,GL_LUMINANCE); 
        texColor.loadData((unsigned char*)colorPixels,width,height,GL_RGB);
	texUser.loadData((unsigned char*)userPixels,width,height,GL_LUMINANCE); 
}

//--------------------------------------------------------------
/*void ofxSimpleOpenNI::updateNormalsTexture()
{
	_shaderNormals.begin();
		
	_shaderNormals.setTexture("depth",*openNI.getTexDepth(),0);

	GLfloat tx0 = 0.0f;
	GLfloat ty0 = 0.0f;
	GLfloat tx1 = openNI.getWidth();
	GLfloat ty1 = openNI.getHeight();

	GLfloat px0 = 0.0f;
	GLfloat py0 = 0.0f;
	GLfloat px1 = openNI.getWidth();
	GLfloat py1 = openNI.getHeight();	

	GLfloat tex_coords[] = {
		tx0,ty0,
		tx1,ty0,
		tx1,ty1,
		tx0,ty1
	};
	GLfloat verts[] = {
		px0,py0,
		px1,py0,
		px1,py1,
		px0,py1
	};

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer(2, GL_FLOAT, 0, tex_coords );
	glEnableClientState(GL_VERTEX_ARRAY);		
	glVertexPointer(2, GL_FLOAT, 0, verts );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	_shaderNormals.end();
}*/

//--------------------------------------------------------------
void ofxSimpleOpenNI::draw(ShapeType shapeType)
{
	drawShape(shapeType);
}

void ofxSimpleOpenNI::drawShape(ShapeType shapeType)
{
	glEnable(GL_DEPTH_TEST);

	shader.begin();

	shader.setTexture("depth",texDepth,0);
	shader.setTexture("tex",texColor,1);
	shader.setTexture("user",texUser,2);

	shader.setUniform("resolution",(float)width,(float)height);
	shader.setUniform("XYtoZ",(float)fXtoZ,(float)fYtoZ);

	switch(shapeType)
	{
		case TRIANGLE:
			mesh.draw();break;
		case POINTCLOUD:
			pointCloud.draw();break;
		case SPHERECLOUD:
			break;
		case SPLATCLOUD:
			splatCloud.draw();break;
		default:
			pointCloud.draw();break;
	}

	shader.end();

	glDisable(GL_DEPTH_TEST);
}

void ofxSimpleOpenNI::drawTexture()
{
	texDepth.draw(5,5,width/2,height/2);
	//texUser.draw(5*2+width/2,5,width/2,height/2);
        texColor.draw(5*3+width/2*2,5,width/2,height/2);
}

//--------------------------------------------------------------
void ofxSimpleOpenNI::resetShader()
{
	shader.unload();
	setupShader();
}
