/*
 *  dsUser.cpp
 *  DS_Tracker
 *
 *  Created by Simon Katan on 15/08/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#include "dsUser.h"

dsUser::dsUser(){

}

void dsUser::setup(int t_id, ofxUserGenerator * t_userGen, ofxDepthGenerator * t_depthGen){

	id = t_id;
	userGen = t_userGen;
	depthGen = t_depthGen;
	uhZx_Thresh = 100;
	pointProp = 0.25;
	eyeProp = 0.9375;
	sternProp = 0.8;
	isPointing = false;
	cloudPoints = new XnPoint3D [userGen->getWidth()* userGen->getHeight()];
    isSleeping = false;
	isIntersect = false;
	isScreen = true;
	isCalibrating = false;


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

	if(rot_CoM_rW.y < floorPoint.y + 500){

        isSleeping = true; //for accidental capturing of the floor
        return;
	}


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

	for(int i = 0; i < 10; i++)highestPoints[i].y = rot_CoM_rW.y + i;

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


    if(highestPoints[0].y > rot_CoM_rW.y){ //meaning at least 10 hps found

        //find mean highest point
        u_height.average(&highestPoints[0], 10);
        hpFound = true;

    }else{

        hpFound = false;
        isPointing = false;
        return;
    }
	//work out thresholds and body parts

	pointThresh = pointProp * (u_height.y - floorPoint.y); //gives actual height of User

	float sh = (u_height.y - floorPoint.y) * sternProp;
	sternum.set(rot_CoM_rW.x, sh + floorPoint.y, rot_CoM_rW.z);

	//now test the cloudPoints to see if there is nearest pixel inside

	u_point.set(0,0,0);

	ofVec3f furthestPoints[10];
	float distances [10] = {0};

    for(int i = 0; i < numCloudPoints; i ++){

        if(rotCloudPoints[i].y > sternum.y ){

            float dist = rotCloudPoints[i].distanceSquared(sternum);
            if( dist > pow(pointThresh,2) && dist < pow(pointThresh * 2, 2)){

                for(int j = 9; j > -1; j --){ //go through the list highest to lowest

					if(dist > distances[j]){

						distances[j] = dist;  //slots it into correct ranking position
						furthestPoints[j] = rotCloudPoints[i];
						break;

					}

				}
            }
        }

    }


	if(distances[0] > 0){  //meaning there are atleast 10 points

		u_point.average(&furthestPoints[0], 10);
		isPointing = true;

		(isScreen) ? updateScreenIntersections() : updateSphereIntersections(); //further filtering happens in these functions

	}else{

	    isPointing = false;
    }


}


void dsUser::updateScreenIntersections(){

	//calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = rot_CoM_rW.x;
	eye_Pos.y = (eyeProp * (u_height.y - floorPoint.y))+ floorPoint.y;
	eye_Pos.z = rot_CoM_rW.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

	ofVec3f focus_dir = ofVec3f(screenCentre.x,screenCentre.y,screenCentre.z) - u_point;
	focus_dir.normalize();

    //test that it's in the right ball park

    if(!focus_dir.align(u_dir,70) && !isCalibrating){

        isPointing = false;
        return;
    }

	// now calculate the intersection with the screen

    float t = screenD - (screenNormal.x * eye_Pos.x + screenNormal.y * eye_Pos.y + screenNormal.z * eye_Pos.z);
		t /= (screenNormal.x * u_dir.x + screenNormal.y * u_dir.y + screenNormal.z * u_dir.z);

		intersection.set(
						 eye_Pos.x + u_dir.x * t,
						 eye_Pos.y + u_dir.y * t,
						 eye_Pos.z + u_dir.z * t
						 );

	//rotate to axis to simplify bounds problem

    ofVec3f rotP = screenP.getRotated(-screenRot, screenCentre, ofVec3f(0,1,0));
	ofVec3f	rotQ = screenQ.getRotated(-screenRot, screenCentre, ofVec3f(0,1,0));

    rotIntersect = intersection.getRotated(-screenRot, screenCentre, ofVec3f(0,1,0));


	if(rotIntersect.x > rotP.x && rotIntersect.x < rotQ.x &&
        rotIntersect.y > rotP.y && rotIntersect.y < rotQ.y){

        isIntersect = true;

    }else{
        isIntersect = false;
    }


}

ofVec2f dsUser::getScreenIntersect(){

    ofVec2f i(rotIntersect.x - screenCentre.x, rotIntersect.y - screenCentre.y);
    i /= screenDims;

    return i;

}

void dsUser::updateSphereIntersections(){

    //calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = rot_CoM_rW.x;
	eye_Pos.y = (eyeProp * (u_height.y - floorPoint.y))+ floorPoint.y;
	eye_Pos.z = rot_CoM_rW.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

	ofVec3f focus_dir = spherePos - u_point;
	focus_dir.normalize();

    //test that it's in the right ball park

    if(!focus_dir.align(u_dir,70) && !isCalibrating){

        isPointing = false;
        return;
    }


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

        if(hpFound){

        glPushMatrix();
        ofFill();
        glTranslatef(u_height.x * scaling, -(u_height.y - floorPoint.y) * scaling, -u_height.z * scaling);
        ofSetColor(238, 130, 238);
        ofBox(0, 0, 0, 100);
        glPopMatrix();

        glPushMatrix();
        ofNoFill();
        glTranslatef(sternum.x * scaling, -(sternum.y - floorPoint.y) * scaling, -sternum.z * scaling);
        ofSetColor(100, 100, 100, 100);
        ofSphere(0, 0, 0, pointThresh * scaling);
        glPopMatrix();

        }


        glPushMatrix();
        ofNoFill();
        glTranslatef(rot_CoM_rW.x * scaling, -(rot_CoM_rW.y - floorPoint.y) * scaling, -rot_CoM_rW.z * scaling);
        ofSetColor(0, 0, 255);
        ofSphere(0, 0, 0, 100);
        glPopMatrix();

        ofSetColor(255, 255, 255);
    }
}

void dsUser::drawIntersect(float mul){

	if(isPointing){
        ofFill();
        ofSetColor(255, 0, 0);
        ofSphere(intersection.x * mul, -(intersection.y - floorPoint.y) * mul, -intersection.z * mul, 150);

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

	delete []cloudPoints;
};
