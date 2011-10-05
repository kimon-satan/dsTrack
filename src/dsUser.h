/*
 *  dsUser.h
 *  DS_Tracker
 *
 *  Created by Simon Katan on 15/08/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#ifndef _DS_USER
#define _DS_USER

#include "ofMain.h"
#include "ofxOpenNI.h"

class myCol{

public:

    myCol(){};
    myCol(int tr, int tg, int tb){

        red = tr;
        green = tg;
        blue = tb;

    }

    int red;
    int green;
    int blue;

};

class environment{

public:

    environment(){

        viewScale = 2;
        pointProp = 0.25;
        eyeProp = 0.9375;
        isScreen = true;
        sternProp = 0.8;
        uhZx_Thresh = 100;
        allowDownPoint = 0.5;
        screenBuffer.set(1.5,1.5);
        moveThresh = 15;
        sphereRad = 750;

    };

    float           viewScale;
    float			pointProp, eyeProp, sternProp, allowDownPoint;
    bool            isScreen;
	float		    screenZ, screenD, screenRot, sphereRad;
	ofVec2f			screenDims, screenBuffer;

	ofVec3f         screenP, screenQ, screenR, screenS, screenCenter, screenNormal, spherePos;

    int             moveThresh, uhZx_Thresh;

    ofVec3f         floorPoint ,fRotAxis;
	float           fRotAngle;

};

class dsUser{

	public:

	dsUser();
	~dsUser();

    void setup(int t_id, ofxUserGenerator * t_userGen, ofxDepthGenerator * t_depthGen, environment * t_env);
	void update();

	void setFloorPlane(ofVec3f tPoint, ofVec3f tAxis, float tAngle);

	void drawMask(ofRectangle dims);

	void drawRWFeatures(bool pointBox = false);
	void drawPointCloud(bool corrected = true, myCol col = myCol(100,100,100));
	void drawIntersect();
	void drawSphereIntersect();


	string getDataStr(int type);

	ofVec3f getUDir(){return u_dir;}
	ofVec3f getUPoint(){return u_point;}
	ofVec2f getScreenIntersect();
	ofVec2f getFakeScreenIntersect();

	ofVec2f getSphereIntersect();

	int id;
	bool isSleeping, isPointing, isCalibrating, isIntersect, isFakeIntersect, isMoving, sendMoveMessage;

	private:

	void updateFeatures();
	void updateScreenIntersections();
	void updateSphereIntersections();
	void calculatePointVector();

	ofImage userMask, depthMask;
	ofxUserGenerator * userGen;
	ofxDepthGenerator * depthGen;
	XnPoint3D  * cloudPoints;
	vector <ofVec3f> rotCloudPoints;
    environment * env;

    int numCloudPoints;
	bool  hpFound;

    float pointThresh;
    ofVec3f sternum;

	ofVec3f rot_CoM_rW, u_height, u_point, eye_Pos, u_dir;

	vector<ofVec3f> up_history;
	vector<ofVec3f> uh_history;

	bool upMoving, uhMoving;

	XnPoint3D CoM_rW;

    bool isBuffer;
    int bufCount;
    int moveThresh;

	float sphereRad;
	ofVec3f spherePos;

	ofVec3f intersection, rotIntersect;


};


#endif
