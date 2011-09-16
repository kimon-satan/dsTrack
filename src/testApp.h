#ifndef _TEST_APP
#define _TEST_APP

#define USE_IR
//
//#define USE_FILE

#include "ofxOpenNI.h"
#include "ofMain.h"
#include "dsUser.h"
#include "ofxControlPanel.h"
#include "userManager.h"

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
    void    updateValues();

    void    calculateScreenPlane();
    void    screenPosManual();
    void    screenPosAuto();
    void    getScreenCalibrationPoints();

private:

    string fileName;

	bool				isCloud, isRawCloud;
	bool				isFiltering, isFloor;


#ifdef USE_IR
	ofxIRGenerator		imageGen;
#else
	ofxImageGenerator	imageGen;
#endif

    ofxControlPanel gui;
    myCol userColors[10];
	ofTrueTypeFont TTF;

	ofxOpenNIContext	Context;
	ofxDepthGenerator	depthGen;

	ofxUserGenerator	userGen;
	xn::SceneAnalyzer   sceneGen;

#if defined (TARGET_OSX) || defined(TARGET_LINUX) // only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	ofxHardwareDriver	hardware;
#endif

	dsUser  		        dsUsers[20];
	vector<int>             activeUserList;
	userManager             thisUM;

	guiTypeTextDropDown * userSelector;

	ofImage				allUserMasks, depthRangeMask;

	float				filterFactor, smoothFactor, yRot, xRot, zRot, zTrans, yTrans, viewScale;
	int					selectedUser, currentUserId, numUsers;

	string				minZ_rw_str, minY_rw_str, CoM_rw_str;

	float			    correctAngle;
	ofVec3f				correctAxis;

	ofVec3f				kinectPos;

	XnPlane3D			floorPlane;

	float				pointProp, eyeProp, sternProp;

    guiTypeToggle       * spTog, * scTog, * scCalTog;

    guiTypeSlider       * scXsl, * scYsl,  * scZsl, * scRotsl, * scWsl, *scHsl;

    bool                isScreen;
	float				screenZ, screenD, screenRot, sphereRad;
	ofVec2f			    screenDims;

	ofVec3f             screenP, screenQ, screenR, screenS, screenCenter, screenNormal;
	ofVec3f             spherePos;

	int scCalibStage;
	string scCalString;

    int outputMode;

    ofVec3f calVecs[3];


};

#endif
