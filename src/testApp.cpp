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

	currentUserId = 0;
	numUsers = 0;
	currentUserId =0;

	#if defined(USE_FILE)
	fileName = "multiUser.oni";
	setupRecording(fileName);
	#else
	setupRecording();
	#endif


    env.screenCenter.set(0,2000,0);
    env.spherePos.set(0,700,0);

    scCalibStage = 0;

	floorPlane.ptPoint.X = 0;
	floorPlane.ptPoint.Y = 0;
	floorPlane.ptPoint.Z = 0;
	floorPlane.vNormal.X = 0;
	floorPlane.vNormal.Y = 0;
	floorPlane.vNormal.Z = 0;

    outputMode = 0;

    for(int i = 0; i < 8; i++){
        dsUsers[i].setup(i, &userGen, &depthGen, &env);
		userColors[i].setHsb(255/(i+1),240,240);
    }

    calculateScreenPlane(); //auto sets screen properties


    setupGUI();

    thisUM.dsUsers = &dsUsers[0];
    thisUM.activeUserList = &activeUserList;
    thisUM.env = &env;
    openSettings(fileTxtIn->getValueText());
}

void testApp::setupRecording(string _filename) {

#if defined (TARGET_OSX) || defined(TARGET_LINUX)// only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	if(fileName == ""){
	hardware.setup();				// libusb direct control of motor, LED and accelerometers
	hardware.setLedOption(LED_OFF); // turn off the led just for yacks (or for live installation/performances ;-)
	}
#endif


	if(fileName != ""){
	thisContext.setupUsingRecording(ofToDataPath(_filename));
	}else{
	thisContext.setup();
	}
	
	// all nodes created by code -> NOT using the xml config file at all
	//thisContext.setupUsingXMLFile();
	sceneGen.Create(thisContext.getXnContext());

	depthGen.setup(&thisContext);
	imageGen.setup(&thisContext);

	//userGen.calibEnabled = false;
	userGen.setup(&thisContext);
	userGen.setSmoothing(filterFactor);				// built in openni skeleton smoothing...
	userGen.setUseMaskPixels(true);
	userGen.setUseCloudPoints(true);
	userGen.setMaxNumberOfUsers(8);					// use this to set dynamic max number of users (NB: that a hard upper limit is defined by MAX_NUMBER_USERS in ofxUserGenerator)
	userGen.addUserListener(this);

	//thisContext.toggleRegisterViewport();  //this fucks up realWorld proj
	thisContext.toggleMirror();
	//hardware.setTiltAngle(0);


}

void testApp::setupGUI(){

	gui.loadFont("MONACO.TTF", 8);
	gui.setup("settings", 0, 0, ofGetScreenWidth(), ofGetScreenHeight());
	gui.addPanel("Camera", 4, false);
	gui.addPanel("User", 4, false);
	gui.addPanel("Screen", 4, false);
	gui.addPanel("Messaging", 4, false);

	gui.setWhichPanel(0);
	gui.setWhichColumn(0);

    fileTxtIn = gui.addTextInput("settingsFile", "mySettings.xml", 200);
    opTog = gui.addToggle("open","OPEN", false);
    svTog = gui.addToggle("save","SAVE", false);
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
	f_vars.push_back( guiVariablePointer("correctionAngle", &env.fRotAngle, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.x", &env.fRotAxis.x, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.y", &env.fRotAxis.y, GUI_VAR_FLOAT) );
	f_vars.push_back( guiVariablePointer("correctAxis.z", &env.fRotAxis.z, GUI_VAR_FLOAT) );

	gui.addVariableLister("Floor Data", f_vars);

	gui.addToggle("find floor", "FIND_FLOOR", isFloor);

	ptsl = gui.addSlider("pointTestProp", "POINT_PROP", env.pointProp, 0.01, 1.0, false);
	uhZxsl = gui.addSlider("hp_zxThresh", "ZX_THRESH", env.uhZx_Thresh, 10, 200, false);
	epsl = gui.addSlider("eyeProp", "EYE_PROP", env.eyeProp, 0.01, 1.0, false);
	stsl = gui.addSlider("sternProp", "STERN_PROP", env.sternProp, 0.01, 1.0, false);
	adpsl = gui.addSlider("allowDownPoint", "ALLOW_DOWN", env.allowDownPoint, 0.01, 1.0, false);
	mtsl = gui.addSlider("moveThresh", "MOVE_THRESH", env.moveThresh, 1, 50, false);

	gui.setWhichPanel(2);

	gui.addLabel("Screen Properties");

    scTog = gui.addToggle("ScreenOn", "SC_ON", true);
    scCalTog = gui.addToggle("Begin Calibrate", "SC_CALIB", false);

	scXsl = gui.addSlider("ScreenX", "SC_X", env.screenCenter.x , -5000, 5000, true);
	scYsl = gui.addSlider("ScreenY", "SC_Y", env.screenCenter.y , -5000, 5000, true);
	scZsl = gui.addSlider("ScreenZ", "SC_Z", env.screenCenter.z , -5000, 5000, true);
	
	scRsl = gui.addSlider("ScreenRad", "SC_RAD", env.screenRad, 100, 3500,true);
	scBMsl = gui.addSlider("ScreenBufMul", "SC_BUF", env.screenBufMul , 1, 10, true);
	scRotsl = gui.addSlider("ScreenRot", "SC_ROT", env.screenRot, -90, 90,true);



    gui.addLabel("Sphere Properties");

    spTog = gui.addToggle("SphereOn", "SP_ON", false);
    spXsl = gui.addSlider("SphereX", "SP_X", env.spherePos.x , -10000, 10000, false);
	spYsl = gui.addSlider("SphereY", "SP_Y", env.spherePos.y , -10000, 10000, false);
	spZsl = gui.addSlider("SphereZ", "SP_Z", env.spherePos.z , -10000, 5000, false);
    spRsl = gui.addSlider("SphereRad", "SP_RAD", env.sphereRad , 100, 3500, false);
    spBMsl = gui.addSlider("SphereBufferMul", "SP_BUF", env.sphereBufferMul , 1, 10, false);


    gui.setWhichPanel(3);

    vector <string> modes;

    modes.push_back("no output");
    modes.push_back("send to remote");

    gui.addMultiToggle("Output Mode", "OUTPUT", 0, modes);

	gui.setupEvents();
	gui.enableEvents();

	ofAddListener(gui.guiEvent, this, &testApp::eventsIn);


}

void testApp::eventsIn(guiCallbackData & data){


	if(data.getXmlName() == "TILT"){

	    #ifdef USE_FILE
		cout << "no device connected \n";
		#else
		hardware.setTiltAngle(data.getFloat(0));
		#endif

	}else if(data.getXmlName() == "POINT_PROP"){
		env.pointProp = data.getFloat(0);
	}else if(data.getXmlName() == "EYE_PROP"){
		env.eyeProp = data.getFloat(0);
    }else if(data.getXmlName() == "STERN_PROP"){
		env.sternProp = data.getFloat(0);
    }else if(data.getXmlName() == "ALLOW_DOWN"){
		env.allowDownPoint = data.getFloat(0);
    }else if(data.getXmlName() == "MOVE_THRESH"){
        env.moveThresh = data.getInt(0);
    }else if(data.getXmlName() == "ZX_THRESH"){
        env.uhZx_Thresh = data.getInt(0);
	}else if(data.getXmlName() == "FIND_FLOOR"){
		isFloor = data.getInt(0);
	}else if(data.getXmlName() == "SELECT_USER"){

        currentUserId = data.getInt(0) - 1;

		if(currentUserId >= 0){currentUserId = dsUsers[currentUserId].id;}else{
		currentUserId = 0;
		}

    }else if(data.getXmlName() == "SC_ON"){
		env.isScreen = data.getInt(0);
		if(env.isScreen){spTog->setValue(0,0);}else{spTog->setValue(1,0);}
    }else if(data.getXmlName() == "SP_ON"){
		env.isScreen = !data.getInt(0);
		if(!env.isScreen){scTog->setValue(0,0);}else{scTog->setValue(1,0); }
	}else if(data.getXmlName() == "SC_X"){
		env.screenCenter.x = data.getFloat(0);
		calculateScreenPlane();
		updateValues();
	}else if(data.getXmlName() == "SC_Y"){
		env.screenCenter.y = data.getFloat(0);
		calculateScreenPlane();
		updateValues();
	}else if(data.getXmlName() == "SC_Z"){
		env.screenCenter.z = data.getFloat(0);
		calculateScreenPlane();
		updateValues();
	}else if(data.getXmlName() == "SC_RAD"){
		env.screenRad = data.getFloat(0);
    }else if(data.getXmlName() == "SC_BUF"){
		env.screenBufMul = data.getFloat(0);

    }else if(data.getXmlName() == "SC_ROT"){
		env.screenRot = data.getFloat(0);
		calculateScreenPlane();
		updateValues();
	}else if(data.getXmlName() == "SP_Z"){
		env.spherePos.z = data.getFloat(0);
	}else if(data.getXmlName() == "SP_X"){
		env.spherePos.x = data.getFloat(0);
	}else if(data.getXmlName() == "SP_Y"){
		env.spherePos.y = data.getFloat(0);
	}else if(data.getXmlName() == "SP_RAD"){
		env.sphereRad = data.getFloat(0);
    }else if(data.getXmlName() == "SP_BUF"){
		env.sphereBufferMul = data.getFloat(0);
	}else if(data.getXmlName() == "SC_CALIB"){
        scCalibStage = 1;
	}else if(data.getXmlName() == "OUTPUT"){
        outputMode = data.getInt(0);
        thisUM.sendOutputMode(outputMode);
	}else if(data.getXmlName() == "OPEN"){
        openSettings(fileTxtIn->getValueText());
        opTog->setValue(0,0);
	}else if(data.getXmlName() == "SAVE"){
        saveSettings(fileTxtIn->getValueText());
        svTog->setValue(0,0);
	}


}

void testApp::saveSettings(string fileName){

    ofxXmlSettings XML;
    int tagNum = XML.addTag("DSTRACK");
    if(XML.pushTag("DSTRACK", tagNum)){

        XML.setValue("POINT_PROP",env.pointProp,tagNum);
        XML.setValue("STERN_PROP",env.sternProp, tagNum);
        XML.setValue("EYE_PROP",env.eyeProp,tagNum);
        XML.setValue("UH_ZX_THRESH", env.uhZx_Thresh, tagNum);
        XML.setValue("MOVE_THRESH", env.moveThresh, tagNum);
        XML.setValue("ALLOW_DOWNPOINT", env.allowDownPoint, tagNum);
        XML.setValue("IS_SCREEN", env.isScreen, tagNum);

        XML.setValue("SCREEN_POS_X", env.screenCenter.x,tagNum);
        XML.setValue("SCREEN_POS_Y", env.screenCenter.y),tagNum;
        XML.setValue("SCREEN_POS_Z", env.screenCenter.z,tagNum);
		XML.setValue("SCREEN_RAD", env.screenRad, tagNum);
		XML.setValue("SCREEN_BUF_MUL", env.screenBufMul, tagNum);
		
        XML.setValue("SCREEN_ROT", env.screenRot,tagNum);

        XML.setValue("SPHERE_POS_X",env.spherePos.x, tagNum);
        XML.setValue("SPHERE_POS_Y",env.spherePos.y, tagNum);
        XML.setValue("SPHERE_POS_Z",env.spherePos.z, tagNum);

        XML.setValue("SPHERE_RADIUS", env.sphereRad,tagNum);
        XML.setValue("SPHERE_BUFFER_MUL", env.sphereBufferMul, tagNum);

        XML.popTag();
    }
    XML.saveFile(fileName);

}

void testApp::openSettings(string fileName){

    ofxXmlSettings XML;

    if(XML.loadFile(fileName)){
    cout << "loading \n";
    }else{
    cout << "can't find file \n";
    return;
    }

    if(XML.pushTag("DSTRACK")){

      //  val = XML.getValue("TILT", 0.0f)


        env.pointProp = XML.getValue("POINT_PROP", 0.0f);
        env.sternProp = XML.getValue("STERN_PROP", 0.0f);
        env.eyeProp = XML.getValue("EYE_PROP",0.0f);
        env.uhZx_Thresh = XML.getValue("UH_ZX_THRESH", 0.0f);
        env.moveThresh = XML.getValue("MOVE_THRESH", 0.0f);
        env.allowDownPoint = XML.getValue("ALLOW_DOWNPOINT", 0.0f);
        env.isScreen = XML.getValue("IS_SCREEN", 0);

        env.screenCenter.x = XML.getValue("SCREEN_POS_X", 0.0f);
        env.screenCenter.y = XML.getValue("SCREEN_POS_Y", 0.0f);
        env.screenCenter.z = XML.getValue("SCREEN_POS_Z", 0.0f);
        env.screenRad = XML.getValue("SCREEN_RAD", 0.0f);
        env.screenBufMul = XML.getValue("SCREEN_BUF_MUL", 0.0f);
        env.screenRot = XML.getValue("SCREEN_ROT",0.0f);


        env.spherePos.x = XML.getValue("SPHERE_POS_X",0.0f);
        env.spherePos.y = XML.getValue("SPHERE_POS_Y",0.0f);
        env.spherePos.z = XML.getValue("SPHERE_POS_Z",0.0f);

        env.sphereRad = XML.getValue("SPHERE_RADIUS", 0.0f);
        env.sphereBufferMul = XML.getValue("SPHERE_BUFFER_MUL", 0.0f);
        XML.popTag();
    }


    calculateScreenPlane();
    updateValues();

}

void testApp::updateValues(){

    epsl->setValue(env.eyeProp,0);
    ptsl->setValue(env.pointProp,0);
    adpsl->setValue(env.allowDownPoint,0);
    mtsl->setValue(env.moveThresh,0);
    stsl->setValue(env.sternProp,0);
    uhZxsl->setValue(env.uhZx_Thresh, 0);

    scTog->setValue(env.isScreen,0);
    spTog->setValue(!env.isScreen,0);
    scRotsl->setValue(env.screenRot, 0);
    scYsl->setValue(env.screenCenter.y, 0);
    scXsl->setValue(env.screenCenter.x, 0);
    scZsl->setValue(env.screenCenter.z, 0);
	scRsl->setValue(env.screenRad ,0);
	scBMsl->setValue(env.screenBufMul, 0);
	
	
    spXsl->setValue(env.spherePos.x,0);
    spYsl->setValue(env.spherePos.y, 0);
    spZsl->setValue(env.spherePos.z, 0);
    spRsl->setValue(env.sphereRad,0);
    spBMsl->setValue(env.sphereBufferMul,0);



}


//--------------------------------------------------------------
void testApp::update(){

	ofBackground(100, 100, 100);

#ifdef TARGET_OSX  // only working on Mac at the moment
	//hardware.update();
#endif

	gui.update();

	// update all nodes
	thisContext.update();
	depthGen.update();
	imageGen.update();

	// update tracking nodes
	
	userGen.update();
	allUserMasks.setFromPixels(userGen.getUserPixels(), userGen.getWidth(), userGen.getHeight(), OF_IMAGE_GRAYSCALE);

	//update the users

	for(int i = 0; i < 8; i++)dsUsers[i].update();

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
		env.fRotAngle = yRef.angle(vN);
		env.fRotAxis = vN.getCrossed(yRef);
		kinectPos.set(0,0,0);
		env.floorPoint = ofVec3f(floorPlane.ptPoint.X, floorPlane.ptPoint.Y, floorPlane.ptPoint.Z);

	}


}




void testApp::calculateScreenPlane(){

	ofVec3f p(env.screenCenter.x - env.screenRad, env.screenCenter.y - env.screenRad, env.screenCenter.z);
    ofVec3f q(env.screenCenter.x - env.screenRad, env.screenCenter.y + env.screenRad, env.screenCenter.z);
    ofVec3f r(env.screenCenter.x + env.screenRad, env.screenCenter.y - env.screenRad, env.screenCenter.z);
	
    p.rotate(env.screenRot, env.screenCenter, ofVec3f(0,1,0));
    q.rotate(env.screenRot, env.screenCenter, ofVec3f(0,1,0));
    r.rotate(env.screenRot, env.screenCenter, ofVec3f(0,1,0));
	
	ofVec3f pq ,pr;

	pq = q - p; pr = r - p;

	env.screenNormal = pq.getCrossed(pr);
	env.screenNormal.normalize();
	ofVec3f dV = env.screenNormal.dot(p);
	env.screenD = dV.x + dV.y + dV.z;


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
			ofSetColor(userColors[currentUserId],50);
			dsUsers[currentUserId].drawMask(ofRectangle(0,0,640,480)); //then mask over
			ofDisableAlphaBlending();


		}else{

			//draw masks and features for all users

            ofEnableAlphaBlending();
            for(int i=0; i < activeUserList.size(); i++){
            int id = activeUserList[i];
                ofSetColor(userColors[id],50);
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
       // TTF.drawString(scCalString, 200,200);
	}
	
	ofSetColor(255);
	ofDrawBitmapString(ofToString(ofGetFrameRate(),2), 400,400);


}


void testApp::draw3Dscene(bool drawScreen){
glEnable(GL_DEPTH_TEST);
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

	ofBox(kinectPos.x, -(kinectPos.y - floorPlane.ptPoint.Y) * env.viewScale, -kinectPos.z, 150); //the origin aka(kinect)
	ofLine(0,-floorPlane.ptPoint.Y,0, 0,-10000,0);

	//drawfloor Plane and normal

	if(isRawCloud){

		ofLine(floorPlane.ptPoint.X *env.viewScale, 0, -5000 * env.viewScale,
			   floorPlane.ptPoint.X *env.viewScale, 0, 0
			   );

		ofLine(-5000, 0, -floorPlane.ptPoint.Z * env.viewScale,
			   5000, 0, -floorPlane.ptPoint.Z * env.viewScale
			   );

		ofLine(floorPlane.ptPoint.X * env.viewScale - (floorPlane.vNormal.X*1000),
			   floorPlane.vNormal.Y * 1000,
			   -(floorPlane.ptPoint.Z * env.viewScale - (floorPlane.vNormal.Z*1000)),

			   floorPlane.ptPoint.X * env.viewScale + (floorPlane.vNormal.X*1000),
			   -(floorPlane.vNormal.Y * 1000),
			   -(floorPlane.ptPoint.Z * env.viewScale + (floorPlane.vNormal.Z*1000))
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

    if(env.isScreen){

		glPushMatrix();
		ofSetColor(255);
		glScalef(env.viewScale,env.viewScale,env.viewScale);
		glTranslatef(0,floorPlane.ptPoint.Y,0);
		
	

        glPushMatrix();
        
        glTranslatef(env.screenCenter.x, -env.screenCenter.y, -env.screenCenter.z);
        glRotatef(-env.screenRot, 0,1,0);
		ofSetRectMode(OF_RECTMODE_CENTER);
		ofRectangle(0,0,100,100);
		ofSetRectMode(OF_RECTMODE_CORNER);
		ofCircle(0, 0, env.screenRad);
		ofSetColor(0,0,255);
		ofCircle(0, 0, env.screenRad * env.screenBufMul);

        glPopMatrix();

        glPopMatrix();


        glPushMatrix();

		for(int i =0; i < activeUserList.size(); i++){

			dsUsers[activeUserList[i]].drawIntersect();
		}
        glPopMatrix();


	}else{


        glPushMatrix();
        glScalef(env.viewScale,env.viewScale,env.viewScale);
        glTranslatef(env.spherePos.x, -(env.spherePos.y - floorPlane.ptPoint.Y), -env.spherePos.z);

        ofNoFill();
        ofSetColor(150,150,150);
        ofSetLineWidth(1);
        ofSphere(0, 0, 0, env.sphereRad);
        ofSetColor(0,0,100);
        ofSphere(0, 0, 0, env.sphereRad * env.sphereBufferMul);

        glPopMatrix();

        for(int i =0; i < activeUserList.size(); i++){
			dsUsers[activeUserList[i]].drawSphereIntersect();
		}


	}
	if(activeUserList.size() > 0 && currentUserId > 0){


		if(isCloud)dsUsers[currentUserId].drawPointCloud(true, userColors[currentUserId]);
		if(isRawCloud)dsUsers[currentUserId].drawPointCloud( false);
		dsUsers[currentUserId].drawRWFeatures(true);

	}else{

		for(int i = 0; i < activeUserList.size(); i++){

		    int id = activeUserList[i];

		    if(isCloud)dsUsers[id].drawPointCloud(true, userColors[id]);
		    dsUsers[id].drawRWFeatures( true);

		}

	}

	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
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
        env.viewScale += 0.01;
        if(env.viewScale > 4)env.viewScale = 4;
        break;

        case '-':
        env.viewScale -= 0.01;
        if(env.viewScale < 1)env.viewScale = 1;
        break;


        case '[':
       // env.screenDims.x -= 1;
       // scWsl->setValue(env.screenDims.x, 0);
			calculateScreenPlane();
        break;

        case ']':
      //  env.screenDims.x += 1;
        //scWsl->setValue(env.screenDims.x, 0);
			calculateScreenPlane();
        break;

        case '{':
       // env.screenDims.y += 1;
       // scHsl->setValue(env.screenDims.y, 0);
			calculateScreenPlane();
        break;

        case '}':
       // env.screenDims.y -= 1;
      //  scHsl->setValue(env.screenDims.y, 0);
			calculateScreenPlane();
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
