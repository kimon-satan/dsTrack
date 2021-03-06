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

void dsUser::setup(int t_id, ofxUserGenerator * t_userGen, ofxDepthGenerator * t_depthGen, environment * t_env){

	id = t_id;
	userGen = t_userGen;
	depthGen = t_depthGen;
	env = t_env;

	isPointing = false;
	cloudPoints = new XnPoint3D [userGen->getWidth()* userGen->getHeight()];
    isSleeping = false;
	isIntersect = false;
	isCalibrating = false;
	sendMoveMessage = false;
	isMoving = false;
	isFakeIntersect = false;
	bufCount = 0;

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

	rot_CoM_rW.set(- CoM_rW.X,CoM_rW.Y, CoM_rW.Z);    //correct it for rotation
	rot_CoM_rW.rotate(env->fRotAngle,env->floorPoint,env->fRotAxis);

	if(rot_CoM_rW.y < env->floorPoint.y + 500){

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
			cloudPoints[numCloudPoints - 1].X = depthGen->getWidth() - (i%depthGen->getWidth());
			cloudPoints[numCloudPoints - 1].Y =i/depthGen->getWidth();
		}

	}

	Xn_depth.ConvertProjectiveToRealWorld(numCloudPoints,cloudPoints,cloudPoints);

	for(int i = 0; i < numCloudPoints; i++){ //correct rotation of points

		ofVec3f temp(cloudPoints[i].X,cloudPoints[i].Y,cloudPoints[i].Z);
		temp.rotate(env->fRotAngle, env->floorPoint, env->fRotAxis);

		rotCloudPoints.push_back(temp);

	}

	//get the height of the user

	u_height.y = 0;
	u_height.x = 0;
	u_height.z = 0;

	ofVec3f highestPoints[10];

	for(int i = 0; i < 10; i++)highestPoints[i].y = rot_CoM_rW.y + i;

	
	for(deque<ofVec3f>::iterator it = rotCloudPoints.begin(); it != rotCloudPoints.end(); it++){

		if(it->y > highestPoints[0].y &&		//only bother if the point is a contender for highest
		   it->z < rot_CoM_rW.z + env->uhZx_Thresh &&
		   it->z > rot_CoM_rW.z - env->uhZx_Thresh &&
		   it->x < rot_CoM_rW.x + env->uhZx_Thresh &&
		   it->x > rot_CoM_rW.x - env->uhZx_Thresh
		   ){

			for(int j = 9; j > -1; j --){ //go through the list highest to lowest

				if(it->y > highestPoints[j].y){

					highestPoints[j] = *it;  //slots it into correct ranking position
					break;

				}

			}

		}

	}


    if(highestPoints[0].y > rot_CoM_rW.y){ //meaning at least 10 hps found

        //find mean highest point
        ofVec3f t_uh;
        t_uh.average(&highestPoints[0], 10);
        hpFound = true;
        uh_history.push_back(t_uh);
        if(uh_history.size()  > 10){uh_history.erase(uh_history.begin());}
        for(int i = 0; i < uh_history.size(); i++)u_height += uh_history[i];
        u_height /= uh_history.size();

        //determine moving or not
        float dist = 0;
        for(int i = 0; i < uh_history.size(); i++){
            dist += u_height.distance(uh_history[i]);
        }
        dist /= uh_history.size();
        (dist < env->moveThresh) ? uhMoving = false : uhMoving = true;

    }else{

        hpFound = false;
        isPointing = false;
        return;
    }
	//work out thresholds and body parts

	pointThresh = env->pointProp * (u_height.y - env->floorPoint.y); //gives actual height of User

	float sh = (u_height.y - env->floorPoint.y) * env->sternProp;
	sternum.set(rot_CoM_rW.x, sh + env->floorPoint.y, rot_CoM_rW.z);

	//now test the cloudPoints to see if there is nearest pixel inside

	u_point.set(0,0,0);

	ofVec3f furthestPoints[10];
	float distances [10] = {0};

    for(deque<ofVec3f>::iterator it = rotCloudPoints.begin(); it != rotCloudPoints.end(); it++){

        if(it->y > sternum.y  - (pointThresh * env->allowDownPoint)){

            float dist = it->distanceSquared(sternum);
            if( dist > pow(pointThresh,2) && dist < pow(pointThresh * 2, 2)){

                for(int j = 9; j > -1; j --){ //go through the list highest to lowest

					if(dist > distances[j]){

						distances[j] = dist;  //slots it into correct ranking position
						furthestPoints[j] = *it;
						break;

					}

				}
            }
        }

    }


	if(distances[0] > 0){  //meaning there are atleast 10 points

        ofVec3f t_up;
		t_up.average(&furthestPoints[0], 10);
		isPointing = true;
        up_history.push_back(t_up);
        if(up_history.size()  > 10){up_history.erase(up_history.begin());}
        for(int i = 0; i < up_history.size(); i++)u_point += up_history[i];
        u_point /= up_history.size();

        //determine moving or not
        float dist = 0;
        for(int i = 0; i < up_history.size(); i++){
            dist += u_point.distance(up_history[i]);
        }

        dist /= up_history.size();

        (dist < env->moveThresh)? upMoving = false : upMoving = true;

		(env->isScreen) ? updateScreenIntersections() : updateSphereIntersections(); //further filtering happens in these functions

        if(isIntersect || isFakeIntersect){
            if(!upMoving && !uhMoving){

                if(isMoving){
                    sendMoveMessage = true;
                    isMoving = false;
                }else{
                    isMoving = false;
                    sendMoveMessage = false;
                }

            }else{
                if(!isMoving){
                    sendMoveMessage = true;
                    isMoving = true;
                }else{
                    isMoving =  true;
                    sendMoveMessage = false;
                }
            }
        }



	}else{

	    isPointing = false;
	    up_history.clear();
	    uh_history.clear();
	    bufCount = 0;
    }


}


void dsUser::updateScreenIntersections(){

	//calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = u_height.x;
	eye_Pos.y = (env->eyeProp * (u_height.y - env->floorPoint.y))+ env->floorPoint.y;
	eye_Pos.z = u_height.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

	ofVec3f focus_dir = ofVec3f(env->screenCenter.x,env->screenCenter.y,env->screenCenter.z) - u_point;
	focus_dir.normalize();

    //test that it's in the right ball park

    if(!focus_dir.align(u_dir,70) && !isCalibrating){

        isPointing = false;
        return;
    }

	// now calculate the intersection with the screen

    float t = env->screenD - (env->screenNormal.x * eye_Pos.x + env->screenNormal.y * eye_Pos.y + env->screenNormal.z * eye_Pos.z);
		t /= (env->screenNormal.x * u_dir.x + env->screenNormal.y * u_dir.y + env->screenNormal.z * u_dir.z);

		intersection.set(
						 eye_Pos.x + u_dir.x * t,
						 eye_Pos.y + u_dir.y * t,
						 eye_Pos.z + u_dir.z * t
						 );


	if(intersection.distance(env->screenCenter) < env->screenRad){

        isIntersect = true;
        isFakeIntersect = false;
       

    }else if(intersection.distance(env->screenCenter) < env->screenRad * env->screenBufMul){

        isIntersect = false;
		isFakeIntersect = true;
		
		//fake intersection is a point on the edge of the circle
		ofVec3f dir(intersection - env->screenCenter);
		dir.normalize();
		ofVec3f fp(env->screenCenter + dir * env->screenRad); 
		intersection.set(fp);

    }else{

        isIntersect = false;
        isFakeIntersect = false;
    
    }


}

ofVec2f dsUser::getScreenIntersect(){
	
	//normalised 2d Vector with screen center as centre;
	ofVec3f rot_i(intersection);
	rot_i.rotate(-env->screenRot, env->screenCenter, ofVec3f(0,1,0));
    ofVec2f i(rot_i.x - env->screenCenter.x, rot_i.y - env->screenCenter.y);
    i /= env->screenRad;

    return i;

}



void dsUser::updateSphereIntersections(){

    //calculate the pointing vector from eyeLine to end of finger

	eye_Pos.x = rot_CoM_rW.x;
	eye_Pos.y = (env->eyeProp * (u_height.y - env->floorPoint.y))+ env->floorPoint.y;
	eye_Pos.z = rot_CoM_rW.z;

	u_dir = u_point - eye_Pos;
	u_dir.normalize();

	ofVec3f focus_dir = env->spherePos - u_point;
	focus_dir.normalize();

    //test that it's in the right ball park

    if(!focus_dir.align(u_dir,70) && !isCalibrating){

        isPointing = false;
        return;
    }

    isIntersect = solveSphereIntersection(env->sphereRad);
    if(!isIntersect){
        isFakeIntersect = solveSphereIntersection(env->sphereRad * env->sphereBufferMul);

        if(isFakeIntersect){

            //now work out the projected intersection
            ofVec3f pa;
            pa = (env->spherePos - u_point);
            ofVec3f x_point;
            x_point = u_point + pa.dot(u_dir) * u_dir;
            ofVec3f xp = x_point - env->spherePos;
            intersection = env->spherePos + xp.normalize() * env->sphereRad;


        }else{

            isFakeIntersect = false;
        }

    }else{

        isFakeIntersect = false;
    }



}

bool dsUser::solveSphereIntersection(float radius){


    // now calculate another point beyond the sphere

	float t = env->spherePos.z - radius + eye_Pos.z/u_dir.z;

	ofVec3f p2(eye_Pos.x - u_dir.x * t,
              eye_Pos.y - u_dir.y * t,
              eye_Pos.z - u_dir.z * t);

    //create a quadratic with the sphere

    double a = pow((p2.x - u_point.x),2) + pow((p2.y - u_point.y),2) + pow((p2.z - u_point.z),2);
    double b = 2 *((p2.x-u_point.x) * (u_point.x - env->spherePos.x)
               + (p2.y-u_point.y) * (u_point.y - env->spherePos.y)
                + (p2.z-u_point.z) * (u_point.z - env->spherePos.z));

    double c = pow((u_point.x-env->spherePos.x),2) + pow((u_point.y-env->spherePos.y),2) + pow((u_point.z-env->spherePos.z),2) - pow(radius,2);

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

         return true;

	}else{

        intersection = p2;
        return false;
	}


}

ofVec2f dsUser::getSphereIntersect(){

    //first we need to make the origin 0
    ofVec2f p_i;
    ofVec3f o_i;
    o_i = intersection - env->spherePos;


    float lat,lon;

    lat =  ofDegToRad(90) -  acos(o_i.y/env->sphereRad)/2;
    lon = atan(o_i.x/o_i.z);

   p_i.x = cos(lat) * sin(lon);
   p_i.y = sin(lat) -  cos(lat) * cos(lon);

    p_i/= 2;

   return p_i;


}

void dsUser::drawMask(ofRectangle dims){

	if(!isSleeping)userMask.draw(dims.x, dims.y, dims.width, dims.height);

}


void dsUser::drawPointCloud(bool corrected, ofColor col) {

    if(!isSleeping){
	glPushMatrix();

	glBegin(GL_POINTS);


		for(deque<ofVec3f>::iterator it = rotCloudPoints.begin(); it != rotCloudPoints.end(); it++){
			
			glColor4ub(col.r,col.g,col.b, 255);
			glVertex3f(it->x * env->viewScale, -(it->y - env->floorPoint.y) * env->viewScale, -it->z * env->viewScale);
			

		}


	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	glPopMatrix();
    }
}


void dsUser::drawRWFeatures(bool pointBox){

    if(!isSleeping){
        ofNoFill();

        if(isPointing){
        glPushMatrix();
        glTranslatef(eye_Pos.x * env->viewScale, -(eye_Pos.y - env->floorPoint.y) * env->viewScale, -eye_Pos.z * env->viewScale);
        ofSphere(0, 0, 0, 50);
        glPopMatrix();

            isIntersect ? ofSetColor(0,255,0) : ofSetColor(255,0,0);

            ofLine(u_point.x * env->viewScale, -(u_point.y - env->floorPoint.y)* env->viewScale, -u_point.z * env->viewScale,
                   intersection.x * env->viewScale, -(intersection.y - env->floorPoint.y)* env->viewScale, -intersection.z * env->viewScale);
        }

        if(hpFound){

        glPushMatrix();
        ofFill();
        glTranslatef(u_height.x * env->viewScale, -(u_height.y - env->floorPoint.y) * env->viewScale, -u_height.z * env->viewScale);
        ofSetColor(238, 130, 238);
        ofBox(0, 0, 0, 100);
        glPopMatrix();

        glPushMatrix();
        ofNoFill();
        glTranslatef(sternum.x * env->viewScale, -(sternum.y - env->floorPoint.y) * env->viewScale, -sternum.z * env->viewScale);
        ofSetColor(100, 100, 100, 100);
        ofSphere(0, 0, 0, pointThresh * env->viewScale);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(sternum.x  * env->viewScale, -(sternum.y - env->floorPoint.y) * env->viewScale, -sternum.z * env->viewScale);
        glTranslatef(0, pointThresh * env->allowDownPoint * env->viewScale,0);
        glRotatef(90,1,0,0);
        ofSetColor(255, 255,255);
        ofSetRectMode(OF_RECTMODE_CENTER);
        ofRect(0,0, pointThresh * 2 * env->viewScale,pointThresh * 2 * env->viewScale);
        ofSetRectMode(OF_RECTMODE_CORNER);
        glPopMatrix();

        }


        glPushMatrix();
        ofNoFill();
        glTranslatef(rot_CoM_rW.x * env->viewScale, -(rot_CoM_rW.y - env->floorPoint.y) * env->viewScale, -rot_CoM_rW.z * env->viewScale);
        ofSetColor(0, 0, 255);
        ofSphere(0, 0, 0, 100);
        glPopMatrix();

        ofSetColor(255, 255, 255);
    }
}

void dsUser::drawIntersect(){

	if(isPointing){
        ofFill();
        if(isIntersect){
            ofSetColor(0, 255, 0);
        }else if(isFakeIntersect){
            ofSetColor(0, 0, 255);
        }else{
            ofSetColor(255,0,0);
        }

        if(isMoving){
        ofSphere(intersection.x * env->viewScale, -(intersection.y - env->floorPoint.y) * env->viewScale, -intersection.z * env->viewScale, 150);
        }else{
        ofBox(intersection.x * env->viewScale, -(intersection.y - env->floorPoint.y) * env->viewScale, -intersection.z * env->viewScale, 150);
        }
	}

}

void dsUser::drawSphereIntersect(){

	if(isPointing){
        ofFill();
        (isIntersect) ? ofSetColor(0, 255, 0): ofSetColor(255,0,0);
        if(isFakeIntersect)ofSetColor(0,0,255);
        ofBox(intersection.x * env->viewScale, -(intersection.y - env->floorPoint.y) * env->viewScale, -intersection.z * env->viewScale, 150);
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




dsUser::~dsUser(){

	delete []cloudPoints;
};
