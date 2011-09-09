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

class dsUser{

	public:

	dsUser(int t_id, ofxUserGenerator * t_userGen, ofxDepthGenerator * t_depthGen);
	~dsUser();

	void update();

	void setFloorPlane(ofVec3f tPoint, ofVec3f tAxis, float tAngle);

	void drawMask(ofRectangle dims);

	void drawRWFeatures(float scaling, bool pointBox = false);
	void drawPointCloud(float mul, bool corrected = true, myCol col = myCol(100,100,100));
	void drawIntersect(float mul);
	void drawSphereIntersect(float mul);

	string getDataStr(int type);

	void setPointProp(float temp){pointProp = temp;}
	void setEyeProp(float temp){eyeProp = temp;}
	void setTestBox(ofVec3f temp){testBox = temp;}
	void setScreen(float tz, ofRectangle tdims){screenZ = tz; screenDims = tdims;}
	void setSphere(ofVec3f tsp, float tr){spherePos = tsp; sphereRad = tr;}

	int id;
	bool isSleeping, isScreen;

	private:

	void updateFeatures();
	void updateScreenIntersections();
	void updateSphereIntersections();
	void calculatePointVector();

	ofImage userMask, depthMask;
	ofxUserGenerator * userGen;
	ofxDepthGenerator * depthGen;
	XnPoint3D  * cloudPoints;  //[307200];
	vector <ofVec3f> rotCloudPoints;

	ofVec3f floorPoint ,fRotAxis;
	float fRotAngle;

	int uhZx_Thresh, numCloudPoints;

	bool isPointing;

	float eyeProp, pointProp;

	ofVec3f rot_CoM_rW, u_height, u_point, eye_Pos, u_dir;
	ofVec3f tb_TLFront, tb_BRBack, testBox;

	XnPoint3D CoM_rW;

	float screenZ;
	ofRectangle screenDims;

	float sphereRad;
	ofVec3f spherePos;

	ofVec3f intersection;
	bool isIntersect;


};


#endif
