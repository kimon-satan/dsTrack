/*
 *  dsUser.cpp
 *  DS_Tracker
 *
 *  Created by Simon Katan on 15/08/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#include "dsUser.h"

dsUser::dsUser(int t_id, ofxUserGenerator * t_userGen, ofxDepthGenerator * t_depthGen){

	id = t_id;
	userGen = t_userGen;
	depthGen = t_depthGen;
	uhZx_Thresh = 100;
	pointProp = 0.25;
	eyeProp = 0.9375;
	isPointing = false;
	cloudPoints = new XnPoint3D [userGen->getWidth()* userGen->getHeight()];
    isSleeping = false;
	isIntersect = false;
	isScreen = true;
}

void dsUser::update(){

	userMask.setFromPixels(userGen->getUserPixels(id), userGen->getWidth(), userGen->getHeight(), OF_IMAGE_GRAYSCALE);
	updateFeatures();

}


void dsUser::updateFeatures(){

	const XnDepthPixel * pDepthMap = NULL;
	unsigned char * pixels = NULL;
	pixels = userGen->getUserPixels(id);
	xn::DepthGenerator Xn_depth = depthGen->getXnDepthGenerator();
	pDepthMap = Xn_depth.GetDepthMap();

	userGen->getXnUserGenerator().GetCoM( id, CoM_rW ); //get the center of mass

	//in case the UserDisappeared without a callback

	if(CoM_rW.Z == 0){

        if(!isSleeping)isSleeping = true;
	    return;

	}else{
        if(isSleeping)isSleeping = false;
	}

	rot_CoM_rW.set(CoM_rW.X,CoM_rW.Y, CoM_rW.Z);    //correct it for rotation
	rot_CoM_rW.rotate(fRotAngle,floorPoint,fRotAxis);


	int w = userGen->getWidth();
	int h = userGen->getHeight();

	numCloudPoints = 0;
	rotCloudPoints.clear();

	for(int i = 0; i < w * h; i ++){ //segment the User and get depthPixels

		if(pixels[i] > 0){
			numCloudPoints += 1;

			cloudPoints[numCloudPoints - 1].Z = pDepthMap[i];
			cloudPoints[numCloudPoints - 1].X =i%depthGen->getWidth();
			cloudPoints[numCloudPoints - 1].Y =i/depthGen->getWidth();
		}

	}

	Xn_depth.ConvertProjectiveToRealWorld(numCloudPoints,cloudPoints,cloudPoints);

	for(int i = 0; i < numCloudPoints; i++){ //correct rotation of points

		ofVec3f temp(cloudPoints[i].X,cloudPoints[i].Y,cloudPoints[i].Z);
		temp.rotate(fRotAngle, floorPoint, fRotAxis);

		rotCloudPoints.push_back(temp);

	}

	//get the height of the user

	u_height.y = 0;
	u_height.x = 0;
	u_height.z = 0;

	ofVec3f highestPoints[10];
																			//ignore people shorter than 1m !
	for(int i = 0; i < 10; i++)highestPoints[i] = floorPoint.y + 1000 + i; //a ranked list of min heights

	for(int i = 0; i < numCloudPoints; i++){

		if(rotCloudPoints[i].y > highestPoints[0].y &&		//only bother if the point is a contender for highest
		   rotCloudPoints[i].z < rot_CoM_rW.z + uhZx_Thresh &&
		   rotCloudPoints[i].z > rot_CoM_rW.z - uhZx_Thresh &&
		   rotCloudPoints[i].x < rot_CoM_rW.x + uhZx_Thresh &&
		   rotCloudPoints[i].x > rot_CoM_rW.x - uhZx_Thresh
		   ){

			for(int j = 9; j > -1; j --){ //go through the list highest to lowest

				if(rotCloudPoints[i].y > highestPoints[j].y){

					highestPoints[j] = rotCloudPoints[i];  //slots it into correct ranking position
					break;

				}

			}

		}

	}

	//find mean highest point

	u_height.average(&highestPoints[0], 10);


	//construct the testBox
	float pointThresh;

	pointThresh = pointProp * (u_height.y - floorPoint.y); //gives actual height of User

	tb_TLFront.x = rot_CoM_rW.x - (testBox.x/2);
	tb_TLFront.y = u_height.y + (testBox.y/2);
	tb_TLFront.z = rot_CoM_rW.z - pointThresh;

	tb_BRBack.x = rot_CoM_rW.x + (testBox.x/2);
	tb_BRBack.y = u_height.y - (testBox.y/2);
	tb_BRBack.z = rot_CoM_rW.z - (pointThresh + testBox.z);

	//now test the cloudPoints to see if there is nearest pixel inside

	u_point.y = 0;
	u_point.x = 0;
	u_point.z = 0;

	ofVec3f closestPoints[10];
	float lowThresh = 0;

	for(int i =0; i < 10; i++){
		closestPoints[i].y = floorPoint.y + 1000 + i;
	}

	for(int i = 0; i < numCloudPoints; i++){

		if((rotCloudPoints[i].x > tb_TLFront.x && rotCloudPoints[i].x < tb_BRBack.x)&&  //don't bother unless it's a contnder
		   (rotCloudPoints[i].y < tb_TLFront.y && rotCloudPoints[i].y > tb_BRBack.y)&&
		   (rotCloudPoints[i].z < tb_TLFront.z && rotCloudPoints[i].z > tb_BRBack.z)&&
		   rotCloudPoints[i].y > closestPoints[0].y
		   ){



				for(int j = 9; j > -1; j --){ //go through the list highest to lowest

					if(rotCloudPoints[i].y > closestPoints[j].y){

						closestPoints[j] = rotCloudPoints[i];  //slots it into correct ranking position
						break;

					}

				}


			}

	}

	if(closestPoints[0].y > floorPoint.y + 1020){

		u_point.average(&closestPoints[0], 10);
		isPointing = true;

		(isScreen) ? updateScreenIntersections() : updateSphereIntersections();

	}else{isPointing = false;}

}


void dsUser::updateScreenIntersections(){

	//calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = rot_CoM_rW.x;
	eye_Pos.y = (eyeProp * (u_height.y - floorPoint.y))+ floorPoint.y;
	eye_Pos.z = rot_CoM_rW.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

	// now calculate the intersection with the screen

	float t = screenZ + eye_Pos.z/u_dir.z;

	intersection.set(
					eye_Pos.x - u_dir.x * t,
					 eye_Pos.y - u_dir.y * t,
					 eye_Pos.z - u_dir.z * t
					);

	//test to see whether it's int the bounds of the screen;
	//this is only a 2D problem .. phew!

	if( intersection.x > screenDims.x - screenDims.width/2
	   && intersection.y > screenDims.y - screenDims.height/2 &&
	   intersection.x < screenDims.x + screenDims.width/2 &&
	   intersection.y < screenDims.y + screenDims.height/2 ){

		isIntersect = true;

	}else{

		isIntersect = false;

	}

}

void dsUser::updateSphereIntersections(){

    //calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = rot_CoM_rW.x;
	eye_Pos.y = (eyeProp * (u_height.y - floorPoint.y))+ floorPoint.y;
	eye_Pos.z = rot_CoM_rW.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

    // now calculate another point beyond the sphere

	float t = spherePos.z - sphereRad + eye_Pos.z/u_dir.z;

	ofVec3f p2(eye_Pos.x - u_dir.x * t,
              eye_Pos.y - u_dir.y * t,
              eye_Pos.z - u_dir.z * t);

    //create a quadratic with the sphere

    double a = pow((p2.x - u_point.x),2) + pow((p2.y - u_point.y),2) + pow((p2.z - u_point.z),2);
    double b = 2 *((p2.x-u_point.x) * (u_point.x - spherePos.x)
               + (p2.y-u_point.y) * (u_point.y - spherePos.y)
                + (p2.z-u_point.z) * (u_point.z - spherePos.z));

    double c = pow((u_point.x-spherePos.x),2) + pow((u_point.y-spherePos.y),2) + pow((u_point.z-spherePos.z),2) - pow(sphereRad,2);

    //now solve it

    double delta = pow(b,2) - (4 * a * c);

	if (delta > 0) { // roots are real // i.e. intersection

		double arg_sqrt = sqrt(delta);
		double root1 = (-b/(2 * a)) + (arg_sqrt/(2 * a));
		double root2 = (-b/(2 * a)) - (arg_sqrt/(2 * a));

        ofVec3f inter1, inter2;

		inter1.x = u_point.x + root1 *(p2.x - u_point.x);
		inter1.y = u_point.y + root1 *(p2.y - u_point.y);
		inter1.z = u_point.z + root1 *(p2.z - u_point.z);

		inter2.x = u_point.x + root2 *(p2.x - u_point.x);
		inter2.y = u_point.y + root2 *(p2.y - u_point.y);
		inter2.z = u_point.z + root2 *(p2.z - u_point.z);

        (inter1.z > inter2.z) ? intersection = inter1 : intersection = inter2;

        isIntersect = true;

	}else{

        intersection = p2;
		isIntersect = false; // a miss !
	}


}

void dsUser::drawMask(ofRectangle dims){

	if(!isSleeping)userMask.draw(dims.x, dims.y, dims.width, dims.height);

}


void dsUser::drawPointCloud(float mul ,bool corrected, myCol col) {

    if(!isSleeping){
	glPushMatrix();

	glBegin(GL_POINTS);


		for(int i = 0; i < numCloudPoints; i ++) {

			if(!corrected){
			glColor4ub(150, 150, 150, 255);
			glVertex3f(cloudPoints[i].X * mul, -(cloudPoints[i].Y - floorPoint.y) * mul, -cloudPoints[i].Z * mul);
			}else{

			glColor4ub(col.red, col.blue, col.green, 255);
			glVertex3f(rotCloudPoints[i].x * mul, -(rotCloudPoints[i].y - floorPoint.y) * mul, -rotCloudPoints[i].z * mul);
			}

		}


	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	glPopMatrix();
    }
}


void dsUser::drawRWFeatures(float scaling, bool pointBox){

    if(!isSleeping){
	ofNoFill();

	if(isPointing){
	glPushMatrix();
	glTranslatef(eye_Pos.x * scaling, -(eye_Pos.y - floorPoint.y) * scaling, -eye_Pos.z * scaling);
	ofSphere(0, 0, 0, 50);
	glPopMatrix();

		isIntersect ? ofSetColor(0,255,0) : ofSetColor(255,0,0);

		ofLine(u_point.x * scaling, -(u_point.y - floorPoint.y)* scaling, -u_point.z * scaling,
			   intersection.x * scaling, -(intersection.y - floorPoint.y)* scaling, -intersection.z * scaling);
	}

	glPushMatrix();
	ofFill();
	glTranslatef(u_height.x * scaling, -(u_height.y - floorPoint.y) * scaling, -u_height.z * scaling);
	ofSetColor(238, 130, 238);
	ofBox(0, 0, 0, 100);
	glPopMatrix();

	glPushMatrix();
	ofNoFill();
	glTranslatef(rot_CoM_rW.x * scaling, -(rot_CoM_rW.y - floorPoint.y) * scaling, -rot_CoM_rW.z * scaling);
	ofSetColor(0, 0, 255);
	ofSphere(0, 0, 0, 100);
	glPopMatrix();


	if(pointBox){

		ofSetColor(255, 255, 255);
		ofNoFill();
		ofPoint mid = (tb_BRBack + tb_TLFront)/2;
		glPushMatrix();
		glTranslatef(mid.x * scaling, -(mid.y - floorPoint.y) * scaling, -mid.z * scaling);
		glScalef(testBox.x, testBox.y, testBox.z);
		ofBox(scaling);
		glPopMatrix();
	}

	ofSetColor(255, 255, 255);
    }
}

void dsUser::drawIntersect(float mul){

	if(isPointing){
        ofFill();
        ofSetColor(255, 0, 0);
        ofCircle(intersection.x * mul, -(intersection.y - floorPoint.y) * mul, 150);

	}

}

void dsUser::drawSphereIntersect(float mul){

	if(isPointing){
        ofFill();
        (isIntersect) ? ofSetColor(0, 255, 0): ofSetColor(255,0,0);
        ofBox(intersection.x * mul, -(intersection.y - floorPoint.y) * mul, -intersection.z * mul, 150);
	}

}

string dsUser::getDataStr(int type){

	string dataStr;

	switch(type){

		case 0:
			dataStr = "u_point( " + ofToString(u_point.x,0) + "," + ofToString(u_point.y,0) + "," + ofToString(u_point.z,0) + ")  ";
			break;

		case 1:
			dataStr = "u_height( " + ofToString(u_height.x,0) + "," + ofToString(u_height.y,0) + "," + ofToString(u_height.z,0) + ")  ";
			break;

		case 2:
			dataStr = "rot_CoM_rW(" + 	ofToString(rot_CoM_rW.x,0) + "," + ofToString(rot_CoM_rW.y,0) + "," + ofToString(rot_CoM_rW.z,0) + ")";
			break;


	}
	return dataStr;
}


void dsUser::setFloorPlane(ofVec3f tPoint, ofVec3f tAxis, float tAngle){

	floorPoint = tPoint;
	fRotAxis = tAxis;
	fRotAngle = tAngle;

}


dsUser::~dsUser(){

	delete []cloudPoints; //for some reason dynamic memory not working on Linux ?!

};
