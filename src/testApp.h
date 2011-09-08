#ifndef _TEST_APP
#define _TEST_APP

#define USE_IR // Uncomment this to use infra red instead of RGB cam...
#define USE_FILE

#include "ofxOpenNI.h"
#include "ofMain.h"
#include "dsUser.h"
#include "ofxControlPanel.h"



class testApp : public ofBaseApp, UserListener{

public:

	void setup();
	void update();
	void draw();

	void draw3Dscene(bool drawScreen = false);

	void onLostUser(int id);
	void onNewUser(int id);

	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);

	void eventsIn(guiCallbackData & data);

	void	setupRecording(string _filename = "");
	void	setupGUI();
	string fileName;

	bool				isCloud, isRawCloud;
	bool				isFiltering, isFloor;

	ofxControlPanel gui;
    myCol userColors[10];
	ofTrueTypeFont TTF;

	ofxOpenNIContext	Context;
	ofxDepthGenerator	depthGen;

#ifdef USE_IR
	ofxIRGenerator		imageGen;
#else
	ofxImageGenerator	imageGen;
#endif

	ofxUserGenerator	userGen;
	xn::SceneAnalyzer   sceneGen;

#if defined (TARGET_OSX) || defined(TARGET_LINUX) // only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	ofxHardwareDriver	hardware;
#endif

	vector <dsUser *>		dsUsers;
	guiTypeTextDropDown * userSelector;

	ofImage				allUserMasks, depthRangeMask;

	float				filterFactor, smoothFactor, yRot, zTrans, yTrans,viewScale;
	int					selectedUser, currentUserId, numUsers;

	string				minZ_rw_str, minY_rw_str, CoM_rw_str;

	float			    correctAngle;
	ofVec3f				correctAxis;

	ofVec3f				kinectPos, testBox;

	XnPlane3D			floorPlane;

	float				pointProp, eyeProp;

    guiTypeToggle       * spTog, * scTog;

    bool                isScreen;
	float				screenZ, sphereRad;
	ofRectangle			screenDims;
	ofVec3f             spherePos;

};

#endif
