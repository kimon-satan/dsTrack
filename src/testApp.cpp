#include "testApp.h"

enum dataStingTypes{

	MINZ_RW,
	MINY_RW,
	COM_RW,
	MINZ_PW,
	MINY_PW,
	COM_PW,

};


//--------------------------------------------------------------
void testApp::setup() {

	isCloud			= true;
	isRawCloud	    = false;
	isFloor			= true;

	TTF.loadFont("verdana.ttf", 60, true, true);

	yRot = 140;
	xRot = 0;
	zTrans = - 5000;
	yTrans = 0;
	viewScale = 2;
	pointProp = 0.25;
	eyeProp = 0.9375;
	isScreen = true;
	sternProp = 0.8;

	currentUserId = 0;
	numUsers = 0;
	currentUserId =0;

	#if defined(USE_FILE)
	fileName = "StillAndMove.oni";
	setupRecording(fileName);
	#else
	setupRecording();
	#endif


    userColors[0] = myCol(255,0,0);
    userColors[1] = myCol(0,255,0);
    userColors[2] = myCol(0,0,255);
    userColors[3] = myCol(255,255,0);
    userColors[4] = myCol(255,110,199);
    userColors[5] = myCol(86,26,139);
    userColors[6] = myCol(100,100,100);
    userColors[7] = myCol(100,100,100);
    userColors[8] = myCol(100,100,100);
    userColors[9] = myCol(100,100,100);

    screenDims.set(4000,3000);
    screenCenter.set(0,2000);
    spherePos.set(0,1500,0);
	sphereRad = 750;
    scCalibStage = 0;
	floorPlane.ptPoint.X = 0;
	floorPlane.ptPoint.Y = 0;
	floorPlane.ptPoint.Z = 0;
	floorPlane.vNormal.X = 0;
	floorPlane.vNormal.Y = 0;
	floorPlane.vNormal.Z = 0;

    outputMode = 0;

    for(int i = 0; i < 20; i++){
        dsUsers[i].setup(i, &userGen, &depthGen);
            //need to set sphere props here
    }

    screenPosManual();
    calculateScreenPlane(); //auto sets screen properties


    setupGUI();

    thisUM.dsUsers = &dsUsers[0];
    thisUM.activeUserList = &activeUserList;

}

void testApp::setupRecording(string _filename) {

#if defined (TARGET_OSX) || defined(TARGET_LINUX)// only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	if(fileName == ""){
	hardware.setup();				// libusb direct control of motor, LED and accelerometers
	hardware.setLedOption(LED_OFF); // turn off the led just for yacks (or for live installation/performances ;-)
	}
#endif


	if(fileName != ""){
	Context.setupUsingRecording(ofToDataPath(_filename));
	}else{
	Context.setup();
	}

	// all nodes created by code -> NOT using the xml config file at all
	//Context.setupUsingXMLFile();
	sceneGen.Create(Context.getXnContext());

	depthGen.setup(&Context);
	imageGen.setup(&Context);

	userGen.calibEnabled = false;
	userGen.setup(&Context);
	userGen.setSmoothing(filterFactor);				// built in openni skeleton smoothing...
	userGen.setUseMaskPixels(true);
	userGen.setUseCloudPoints(true);
	userGen.setMaxNumberOfUsers(8);					// use this to set dynamic max number of users (NB: that a hard upper limit is defined by MAX_NUMBER_USERS in ofxUserGenerator)
	userGen.addUserListener(this);

	//Context.toggleRegisterViewport();  //this fucks up realWorld proj
	Context.toggleMirror();
	//hardware.setTiltAngle(0);

}

void testApp::setupGUI(){

	//gui.loadFont("MONACO.TTF", 8);
	gui.setup("settings", 0, 0, ofGetWidth(), ofGetHeight());
	gui.addPanel("Camera", 4, false);
	gui.addPanel("User", 4, false);
	gui.addPanel("Screen", 4, false);
	gui.addPanel("Messaging", 4, false);

	gui.setWhichPanel(0);
	gui.setWhichColumn(0);

	gui.addSlider("tilt", "TILT", 0, -35, 35, false);

	vector <guiVariablePointer> t_vars;

	t_vars.push_back( guiVariablePointer("numUsers", &numUsers, GUI_VAR_INT) );
	t_vars.push_back( guiVariablePointer("currentUserId", &currentUserId, GUI_VAR_INT) );
	t_vars.push_back( guiVariablePointer("u_point", &minZ_rw_str, GUI_VAR_STRING ));
	t_vars.push_back( guiVariablePointer("u_height", &minY_rw_str, GUI_VAR_STRING ));
	t_vars.push_back( guiVariablePointer("rot_CoM_rw", &CoM_rw_str, GUI_VAR_STRING ));

	gui.addVariableLister("User Data", t_vars);

	vector<string> t_vec;
	t_vec.push_back("none");

	userSelector = gui.addTextDropDown("SelectUser", "SELECT_USER", 0, t_vec);

	gui.setWhichPanel(1);
	gui.setWhichColumn(0);

    userSelector = gui.addTextDropDown("SelectUser", "SELECT_USER", 0, t_vec);
    gui.addVariableLister("User Data", t_vars);

	vector <guiVariablePointer> f_vars;

	f_vars.push_back( guiVariablePointer("floorPoint.y", &floorPlane.ptPoint.Y, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctionAngle", &correctAngle, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.x", &correctAxis.x, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.y", &correctAxis.y, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.z", &correctAxis.z, GUI_VAR_FLOAT) );

	gui.addVariableLister("Floor Data", f_vars);

	gui.addToggle("find floor", "FIND_FLOOR", isFloor);

	gui.addSlider("pointTestProp", "POINT_PROP", pointProp, 0.01, 1.0, false);
	gui.addSlider("eyeProp", "EYE_PROP", eyeProp, 0.01, 1.0, false);
	gui.addSlider("sternProp", "STERN_PROP", sternProp, 0.01, 1.0, false);

	gui.setWhichPanel(2);

	gui.addLabel("Screen Properties");

    scTog = gui.addToggle("ScreenOn", "SC_ON", true);
    scCalTog = gui.addToggle("Begin Calibrate", "SC_CALIB", false);
	scWsl = gui.addSlider("ScreenW", "SC_W", screenDims.x , 500, 10000, true);
	scHsl = gui.addSlider("ScreenH", "SC_H", screenDims.y , 500, 10000, true);
	scXsl = gui.addSlider("ScreenX", "SC_X", screenCenter.x , -5000, 5000, true);
	scYsl = gui.addSlider("ScreenY", "SC_Y", screenCenter.y , -5000, 5000, true);
	scZsl = gui.addSlider("ScreenZ", "SC_Z", screenCenter.z , -5000, 5000, true);
	scRotsl = gui.addSlider("ScreenRot", "SC_ROT", screenRot, -90, 90,true);

    gui.addLabel("Sphere Properties");

    spTog = gui.addToggle("SphereOn", "SP_ON", false);
    gui.addSlider("SphereX", "SP_X", spherePos.x , -10000, 10000, false);
	gui.addSlider("SphereY", "SP_Y", spherePos.y , 0, 10000, false);
	gui.addSlider("SphereZ", "SP_Z", spherePos.z , -10000, 5000, false);
    gui.addSlider("SphereRad", "SP_RAD", sphereRad , 100, 3500, false);


    gui.setWhichPanel(3);

    vector <string> modes;

    modes.push_back("no output");
    modes.push_back("testPoint");
    modes.push_back("userManaged");

    gui.addMultiToggle("Output Mode", "OUTPUT", 0, modes);

	gui.setupEvents();
	gui.enableEvents();

	ofAddListener(gui.guiEvent, this, &testApp::eventsIn);
}

void testApp::eventsIn(guiCallbackData & data){


	if(data.getXmlName() == "TILT"){
		hardware.setTiltAngle(data.getFloat(0));
	}else if(data.getXmlName() == "POINT_PROP"){
		pointProp = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setPointProp(pointProp);
	}else if(data.getXmlName() == "EYE_PROP"){
		eyeProp = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setEyeProp(eyeProp);
    }else if(data.getXmlName() == "STERN_PROP"){
		sternProp = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setSternProp(sternProp);
	}else if(data.getXmlName() == "FIND_FLOOR"){
		isFloor = data.getInt(0);
	}else if(data.getXmlName() == "SELECT_USER"){

        currentUserId = data.getInt(0) - 1;

		if(currentUserId >= 0){currentUserId = dsUsers[currentUserId].id;}else{
		currentUserId = 0;
		}

    }else if(data.getXmlName() == "SC_ON"){
		isScreen = data.getInt(0);
		for(int i = 0; i < 20; i++)dsUsers[i].isScreen = isScreen;
		if(isScreen){spTog->setValue(0,0);}else{spTog->setValue(1,0);}
    }else if(data.getXmlName() == "SP_ON"){
		isScreen = !data.getInt(0);
		for(int i = 0; i < 20; i++)dsUsers[i].isScreen = isScreen;
		if(!isScreen){scTog->setValue(0,0);}else{scTog->setValue(1,0); }
	}else if(data.getXmlName() == "SC_X"){
		screenCenter.x = data.getFloat(0);
		screenPosManual();
		updateValues();
	}else if(data.getXmlName() == "SC_Y"){
		screenCenter.y = data.getFloat(0);
		screenPosManual();
		updateValues();
	}else if(data.getXmlName() == "SC_Z"){
		screenCenter.z = data.getFloat(0);
		screenPosManual();
		updateValues();
    }else if(data.getXmlName() == "SC_W"){
		screenDims.x = data.getFloat(0);
		screenPosManual();
		updateValues();
    }else if(data.getXmlName() == "SC_H"){
		screenDims.y = data.getFloat(0);
		screenPosManual();
		updateValues();
    }else if(data.getXmlName() == "SC_ROT"){
		screenRot = data.getFloat(0);
		screenPosManual();
		updateValues();
	}else if(data.getXmlName() == "SP_Z"){
		spherePos.z = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setSphere(spherePos, sphereRad);
	}else if(data.getXmlName() == "SP_X"){
		spherePos.x = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setSphere(spherePos, sphereRad);
	}else if(data.getXmlName() == "SP_Y"){
		spherePos.y = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setSphere(spherePos, sphereRad);
	}else if(data.getXmlName() == "SP_RAD"){
		sphereRad = data.getFloat(0);
		for(int i = 0; i < 20; i++)dsUsers[i].setSphere(spherePos, sphereRad);
	}else if(data.getXmlName() == "SC_CALIB"){
        scCalibStage = 1;
	}else if(data.getXmlName() == "OUTPUT"){
        outputMode = data.getInt(0);
        thisUM.sendOutputMode(outputMode);
	}


}

void testApp::updateValues(){

    scRotsl->setValue(screenRot, 0);
    scYsl->setValue(screenCenter.y, 0);
    scXsl->setValue(screenCenter.x, 0);
    scZsl->setValue(screenCenter.z, 0);
    scWsl->setValue(screenDims.x, 0);
    scHsl->setValue(screenDims.y, 0);

}


//--------------------------------------------------------------
void testApp::update(){

	ofBackground(100, 100, 100);

#ifdef TARGET_OSX  // only working on Mac at the moment
	hardware.update();
#endif

	gui.update();

	// update all nodes
	Context.update();
	depthGen.update();
	imageGen.update();

	// update tracking nodes
	userGen.update();
	allUserMasks.setFromPixels(userGen.getUserPixels(), userGen.getWidth(), userGen.getHeight(), OF_IMAGE_GRAYSCALE);

	//update the users

	for(int  i = 0; i < 20; i++)dsUsers[i].update();

    thisUM.manageUsers();

	if(activeUserList.size() > 0 && currentUserId > 0){

		CoM_rw_str = dsUsers[currentUserId].getDataStr(COM_RW);
		minZ_rw_str = dsUsers[currentUserId].getDataStr(MINZ_RW);
		minY_rw_str = dsUsers[currentUserId].getDataStr(MINY_RW);

	}

	//try to get the floor pane

	if(isFloor){

		XnStatus st = sceneGen.GetFloor(floorPlane);

		ofVec3f vN(floorPlane.vNormal.X, floorPlane.vNormal.Y, floorPlane.vNormal.Z);
		vN.normalize();
		ofVec3f yRef(0,1,0);
		correctAngle = yRef.angle(vN);
		correctAxis = vN.getCrossed(yRef);
		kinectPos.set(0,0,0);
		kinectPos.rotate(correctAngle, ofVec3f(floorPlane.ptPoint.X, floorPlane.ptPoint.Y, floorPlane.ptPoint.Z), correctAxis);

		for(int  i = 0; i < 20; i++){
			dsUsers[i].setFloorPlane(ofVec3f(floorPlane.ptPoint.X, floorPlane.ptPoint.Y, floorPlane.ptPoint.Z),
								  correctAxis, correctAngle);
		}
	}

	if(scCalibStage > 0)getScreenCalibrationPoints();


}

void testApp::screenPosManual(){ //this is just for manual settings of screenPosition, size and orientation


    screenP.set(screenCenter.x - (screenDims.x/2), screenCenter.y - (screenDims.y/2), screenCenter.z);
    screenQ.set(screenCenter.x + (screenDims.x/2), screenCenter.y + (screenDims.y/2), screenCenter.z);
    screenR.set(screenCenter.x - (screenDims.x/2), screenCenter.y + (screenDims.y/2), screenCenter.z);
  //  screenS.set(screenCenter.x + (screenDims.x/2), screenCenter.y - (screenDims.y/2), screenCenter.z);

    screenP.rotate(screenRot, screenCenter, ofVec3f(0,1,0));
    screenQ.rotate(screenRot, screenCenter, ofVec3f(0,1,0));
    screenR.rotate(screenRot, screenCenter, ofVec3f(0,1,0));
   // screenS.rotate(screenRot, screenCenter, ofVec3f(0,1,0)); //this is done in calculateScreenPlane

    calculateScreenPlane();
}

void testApp::screenPosAuto(){

  ofVec3f p;

   // screenRot = 999;

    calVecs[0].normalize(); //are they normalized already ?
    calVecs[1].normalize();
    calVecs[2].normalize();

    bool  pFound = false;
    float mul = 1;
    float dist = 10000;

    while(!pFound){

        ofVec3f p = (calVecs[0] * mul + dsUsers[currentUserId].getUPoint());
        ofVec3f r = p - ofVec3f(0,screenDims.y,0); //ray cast down y axis for size of screen
        ofVec3f pb = dsUsers[currentUserId].getUPoint(); //the user
        ofVec3f pc = calVecs[2] * 10000 + dsUsers[currentUserId].getUPoint(); //the ray to R

        float a = pb.distance(pc);
        float b = pc.distance(r);
        float c = pb.distance(r);

        float area = sqrt(((a+b+c)*(-a+b+c)*(a-b+c)*(a+b-c))/16);

        float t_dist = (area * 2)/a;

        if(t_dist > dist){

            pFound = true;  //the distance is staring to increase so we have already found the best point

        }else{
            dist = t_dist;
            screenR = r;
            screenP = p;
            mul += 1; //maybe we need a more efficient search ...nah
        }

    }


    // to find Q, iterate until PQ is correct length

    bool qFound = false;

    dist = 10000;
    ofVec3f q = screenR; // a point on vec UR screenWidth from R
    q.x += screenDims.x;
    ofVec3f qb = dsUsers[currentUserId].getUPoint(); //the user
    ofVec3f qc = calVecs[1] * 10000 + dsUsers[currentUserId].getUPoint(); //the ray to Q

    while(!qFound){

        float a = qb.distance(qc);
        float b = qc.distance(q);
        float c = qb.distance(q);

        float area = sqrt(((a+b+c)*(-a+b+c)*(a-b+c)*(a+b-c))/16);
        float t_dist = (area * 2)/a;

        if(t_dist > dist){

            qFound = true;

        }else{

        screenQ = q;
        dist = t_dist;
        q.rotate(1,screenP,ofVec3f(0,1,0));

        }

    }

    calculateScreenPlane();

    if(screenRot > 90){

        screenRot -= 180;
        screenPosManual();

    }else if(screenRot < 90){

        screenRot += 180;
        screenPosManual();

    }
}

void testApp::calculateScreenPlane(){

        ofVec3f pq ,pr;
		pq = screenQ - screenP; pr = screenR - screenP;

        screenCenter = screenP.getMiddle(screenQ); //in case hasn't been set
		screenNormal = pq.getCrossed(pr);
		screenNormal.normalize();
		ofVec3f dV = screenNormal.dot(screenP);
		screenD = dV.x + dV.y + dV.z;

		// now get Rotation
		ofVec3f axis(0,0,1);
		screenRot = axis.angle(screenNormal);
		if(screenNormal.x < 0)screenRot *= -1; //direction correction

        //calculate point S
        screenS = screenP.getRotated(180,screenCenter,ofVec3f(0,1,0)); //in case hasn't already been set

        for(int i = 0; i < 20; i++){

            dsUsers[i].setScreen(screenD, screenRot, screenP, screenQ, screenNormal, screenCenter, screenDims);

        }

}

void testApp::getScreenCalibrationPoints(){

    static int countdown = 5;
    static int msecs;
    static ofVec3f calVecTemps[20] = {ofVec3f(0,0,0)};
   // static bool userFound;

    if(scCalibStage == 1 && countdown == 5){

        for(int i =0; i < activeUserList.size(); i++){

            if(dsUsers[activeUserList[i]].isPointing && !dsUsers[activeUserList[i]].isSleeping){
                currentUserId = activeUserList[i];
                dsUsers[currentUserId].isCalibrating =true;
                countdown -= 1;
                break;
            }

        }

    }

    if(ofGetElapsedTimeMillis() > msecs + 1000 && scCalibStage == 10){ // if failed

        msecs = ofGetElapsedTimeMillis();
        countdown -=1;
        if(countdown == 0){

            scCalibStage = 0;
            countdown = 5;
            scCalTog->setValue(0,0);
            dsUsers[currentUserId].isCalibrating = false;
            currentUserId = 0;
            scCalString = "";
            thisUM.sendCalibrationMessage(0, 0);
        }


    }else if(ofGetElapsedTimeMillis() > msecs + 1000 && countdown > 0 && countdown < 5){ //normal routine

        msecs = ofGetElapsedTimeMillis();
        thisUM.sendCalibrationMessage(scCalibStage,countdown);

        switch(scCalibStage){

        case 1:
        scCalString = "TOP LEFT";
        break;

        case 2:
        scCalString = "BOTTOM RIGHT";
        break;

        case 3:
        scCalString = "BOTTOM LEFT";
        break;

        }

        countdown -= 1;
        scCalString = ofToString(countdown,0) + " : " + scCalString;

    }else if(countdown == -20){

        calVecs[scCalibStage -1].average(&calVecTemps[0],20);
        scCalibStage += 1;
        scCalibStage = scCalibStage%4;
        if(scCalibStage == 0){
            screenPosAuto(); //calculate screen points given it's size
            countdown = 5;
            scCalTog->setValue(0,0);
            scXsl->setValue(screenCenter.x, 0);
            scYsl->setValue(screenCenter.y, 0);
            scZsl->setValue(screenCenter.z, 0);
            scRotsl->setValue(screenRot,0);
            dsUsers[currentUserId].isCalibrating = false;
            currentUserId = 0;
            scCalString = "";
            thisUM.sendCalibrationMessage(scCalibStage, 0);

        }else{countdown = 4;}

    }else if(countdown <= 0 ){

        if(dsUsers[currentUserId].isPointing){
        calVecTemps[-countdown] = dsUsers[currentUserId].getUDir();
        countdown -= 1;
        }else{
         scCalString = "FAILED";
         scCalibStage = 10;
         thisUM.sendCalibrationMessage(scCalibStage, 0);
         countdown = 3;
        }

    }






}

//--------------------------------------------------------------
void testApp::draw(){

	ofSetColor(255,255,255);
	gui.draw();


	if(gui.getSelectedPanel() == 0){

		glPushMatrix();
		glTranslatef(400, 70, 0);
		glScalef(0.7, 0.7, 1);

		imageGen.draw(0, 0, 640, 480); // first draw the image
        depthGen.draw(0,480, 640, 480);

		if(activeUserList.size() > 0 && currentUserId > 0){

			ofEnableAlphaBlending();
			ofSetColor(userColors[currentUserId].red,userColors[currentUserId].blue,userColors[currentUserId].green,50);
			dsUsers[currentUserId].drawMask(ofRectangle(0,0,640,480)); //then mask over
			ofDisableAlphaBlending();


		}else{

			//draw masks and features for all users

            ofEnableAlphaBlending();
            for(int i=0; i < activeUserList.size(); i++){
            int id = activeUserList[i];
                ofSetColor(userColors[id].red,userColors[id].blue,userColors[id].green,50);
                dsUsers[id].drawMask(ofRectangle(0,0,640,480)); //then mask over
            }
			ofDisableAlphaBlending();

		}

		glPopMatrix();


	}else if(gui.getSelectedPanel() == 1){

		draw3Dscene();

	}else if(gui.getSelectedPanel() == 2){

		draw3Dscene();

	}else if(gui.getSelectedPanel() == 3){

		draw3Dscene(true);
	}

	if(scCalibStage > 0){
        ofSetColor(255,0,0);
        TTF.drawString(scCalString, 200,200);
	}

}


void testApp::draw3Dscene(bool drawScreen){

	glPushMatrix();
	glTranslatef(ofGetWidth()/2,ofGetHeight()/2, 0);
	glScalef(0.1, 0.1, 0.1);
	glTranslatef(0,yTrans,zTrans);
	glRotatef(yRot,0,1,0);
	glRotatef(xRot,1,0,0);
	ofSetColor(255);
	ofNoFill();

	glPushMatrix();
	ofBox(0, 0, 0, 10000);
	glPopMatrix();

	glTranslatef(0, 5000, 5000);

	ofBox(kinectPos.x, -(kinectPos.y - floorPlane.ptPoint.Y) * viewScale, -kinectPos.z, 150); //the origin aka(kinect)

	//drawfloor Plane and normal

	if(isRawCloud){

		ofLine(floorPlane.ptPoint.X *viewScale, 0, -5000 * viewScale,
			   floorPlane.ptPoint.X *viewScale, 0, 0
			   );

		ofLine(-5000, 0, -floorPlane.ptPoint.Z * viewScale,
			   5000, 0, -floorPlane.ptPoint.Z * viewScale
			   );

		ofLine(floorPlane.ptPoint.X * viewScale - (floorPlane.vNormal.X*1000),
			   floorPlane.vNormal.Y * 1000,
			   -(floorPlane.ptPoint.Z * viewScale - (floorPlane.vNormal.Z*1000)),

			   floorPlane.ptPoint.X * viewScale + (floorPlane.vNormal.X*1000),
			   -(floorPlane.vNormal.Y * 1000),
			   -(floorPlane.ptPoint.Z * viewScale + (floorPlane.vNormal.Z*1000))
			   );

	}else{

		ofSetColor(100);

		for(int i = 0; i < 9; i++){

			int x = 4000 - (1000 * i);
			int z = 9000 - (1000 * i);
			ofLine(x , 0, 0, x, 0, -10000);
			ofLine(-5000 , 0, -z, 5000, 0, -z);
		}

	}

    if(isScreen){

		glPushMatrix();
		ofSetColor(255);
		glScalef(viewScale,viewScale,viewScale);
		glTranslatef(0,floorPlane.ptPoint.Y,0);
        ofLine(screenP.x, -screenP.y, -screenP.z, screenQ.x, -screenQ.y, -screenQ.z);
        ofLine(screenP.x, -screenP.y, -screenP.z, screenR.x, -screenR.y, -screenR.z);
        ofLine(screenQ.x, -screenQ.y, -screenQ.z, screenR.x, -screenR.y, -screenR.z);
        ofLine(screenS.x, -screenS.y, -screenS.z, screenP.x, -screenP.y, -screenP.z);
        ofLine(screenS.x, -screenS.y, -screenS.z, screenR.x, -screenR.y, -screenR.z);
        ofLine(screenS.x, -screenS.y, -screenS.z, screenQ.x, -screenQ.y, -screenQ.z);
        glPopMatrix();


        glPushMatrix();

		for(int i =0; i < activeUserList.size(); i++){

			dsUsers[activeUserList[i]].drawIntersect(viewScale);
		}
        glPopMatrix();


	}else{


        glPushMatrix();
        glScalef(viewScale,viewScale,viewScale);
        glTranslatef(spherePos.x, -(spherePos.y - floorPlane.ptPoint.Y), -spherePos.z);

        ofNoFill();
        ofSetColor(100,100,100);
        ofSetLineWidth(1);
        ofSphere(0, 0, 0, sphereRad);

        glPopMatrix();

        for(int i =0; i < activeUserList.size(); i++){
			dsUsers[activeUserList[i]].drawSphereIntersect(viewScale);
		}


	}
	if(activeUserList.size() > 0 && currentUserId > 0){


		if(isCloud)dsUsers[currentUserId].drawPointCloud(viewScale, true, userColors[currentUserId]);
		if(isRawCloud)dsUsers[currentUserId].drawPointCloud(viewScale, false);
		dsUsers[currentUserId].drawRWFeatures(viewScale, true);

	}else{

		for(int i = 0; i < activeUserList.size(); i++){

		    int id = activeUserList[i];

		    if(isCloud)dsUsers[id].drawPointCloud(viewScale, true, userColors[id]);
		    dsUsers[id].drawRWFeatures(viewScale, true);

		}

	}

	glPopMatrix();
}


void testApp::onNewUser(int id)
{
	cout << "new dsUser added \n";
    numUsers += 1;
    dsUsers[id].isSleeping = false;
    dsUsers[id].isMoving = false;
	activeUserList.push_back(id);

	userSelector->vecDropList.push_back(ofToString(id, 0));

}

void testApp::onLostUser(int id)
{
	cout << "dsUser removed \n";
	numUsers -= 1;

	for(int i = 0; i < userSelector->vecDropList.size(); i++){ //erase the menu reference

		if(ofToInt(userSelector->vecDropList[i]) == id){

			userSelector->vecDropList.erase(userSelector->vecDropList.begin() + i);

		}
    }


    for(int i = 0; i < activeUserList.size(); i ++){
		if(activeUserList[i] == id){

			activeUserList.erase(activeUserList.begin() + i);

		}
	}
}


//--------------------------------------------------------------
void testApp::keyPressed(int key){

	switch (key) {

        case 'w':
        xRot += 1;
        if(xRot < 0 || xRot > 360)xRot = 0;
        break;

        case 'q':
        xRot -=1;
        if(xRot < 0 || xRot > 360)xRot = 0;
        break;

        case 's':
        yRot +=1;
        if(yRot < 0 || yRot > 360)yRot = 0;
        break;

        case 'a':
        yRot -=1;
        if(yRot < 0 || yRot > 360)yRot = 0;
        break;

        case 'x':
        zRot +=1;
        if(zRot < 0 || zRot > 360)zRot = 0;
        break;

        case 'z':
        zRot -=1;
        if(zRot < 0 || zRot > 360)zRot = 0;
        break;

       case '<':
       zTrans += 30;
       break;

       case '>':
       zTrans -= 30;
       break;

        case OF_KEY_UP:
        yTrans += 30;
        break;

        case OF_KEY_DOWN:
        yTrans -= 30;
        break;

        case '+':
        viewScale += 0.01;
        if(viewScale > 4)viewScale = 4;
        break;

        case '-':
        viewScale -= 0.01;
        if(viewScale < 1)viewScale = 1;
        break;


        case '[':
        screenDims.x -= 1;
        scWsl->setValue(screenDims.x, 0);
		screenPosManual();
        break;

        case ']':
        screenDims.x += 1;
        scWsl->setValue(screenDims.x, 0);
		screenPosManual();
        break;

        case '{':
        screenDims.y += 1;
        scHsl->setValue(screenDims.y, 0);
		screenPosManual();
        break;

        case '}':
        screenDims.y -= 1;
        scHsl->setValue(screenDims.y, 0);
		screenPosManual();
        break;
	}

}


//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){


}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	gui.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	gui.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	gui.mouseReleased();

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}
