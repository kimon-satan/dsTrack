#include "userManager.h"

userManager::userManager()
{
    //ctor
    for(int i = 0; i < 20; i ++)currentUserList[i] = 0;
    outputMode = 0;
    thisMess.setup(LOCAL_HOST, PORT);

    ofxOscMessage m;
    m.setAddress("/init");
    thisMess.sendMessage(m);

}

void userManager::sendOutputMode(int t_out){

        if(outputMode == 0 && t_out > 0){thisMess.setup(EX_HOST, PORT);}
        if(outputMode > 0 && t_out == 0){thisMess.setup(LOCAL_HOST, PORT);}

        outputMode = t_out;
        ofxOscMessage m;
        m.setAddress("/outputMode");
        m.addIntArg(outputMode);
        thisMess.sendMessage(m);

}


void userManager::manageUsers(){

    if(outputMode == 0)return;

    //first find compliant users and see if they are new
    for(int  i = 0; i < activeUserList->size(); i++){

        int t_id = activeUserList->at(i);


        if(!dsUsers[t_id].isSleeping && dsUsers[t_id].isPointing && (dsUsers[t_id].isIntersect || dsUsers[t_id].isFakeIntersect)){
            //register newUsers
            //assign a new id as old ids will be duplicated with 2 kinects (will need a 2d array for referencing)
            //duplicate users will be filtered out at the newUser/userAwake stage (more optimal)

            bool newUser = true;

            for(int j = 0; j < 20; j ++){
                if(currentUserList[j] == t_id)newUser = false;
                } //first check user doesn't exist

            if(newUser){
                for(int j = 0; j < 20; j ++){
                    if(currentUserList[j] == 0){
                        currentUserList[j] = t_id;
                        sendNewUser(j);
                        sendIsMoving(j, dsUsers[t_id].isMoving);
                        break;}
                    } //find an empty id and assign
            }

        }else{

            for(int j = 0; j < 20; j ++){

            if(currentUserList[j] == t_id){

            currentUserList[j] = 0;
            sendLostUser(j);
            break;

                } //remove the old users
            }
        }
    }


    for(int j = 0; j < 20; j ++){
        if(currentUserList[j] > 0){
            int id = currentUserList[j];
            ofVec2f t = dsUsers[id].getScreenIntersect();
            sendPoint(j, t, dsUsers[id].isFakeIntersect);
            if(dsUsers[id].sendMoveMessage){
                sendIsMoving(j, dsUsers[id].isMoving);
            }
        }
    }

}

void userManager::sendNewUser(int id){

    ofxOscMessage m;
    m.setAddress("/newUser");
    m.addIntArg(id);
    thisMess.sendMessage(m);
}


void userManager::sendLostUser(int id){

    ofxOscMessage m;
    m.setAddress("/lostUser");
    m.addIntArg(id);
    thisMess.sendMessage(m);

}

void userManager::sendPoint(int userId, ofVec2f point, bool isFake){


    ofxOscMessage m;
    m.setAddress("/point");
    m.addIntArg(userId);
    m.addFloatArg(point.x);
    m.addFloatArg(point.y);
    m.addIntArg(isFake);

    thisMess.sendMessage(m);

}

void userManager::sendCalibrationMessage(int stage, int count){

    ofxOscMessage m;
    m.setAddress("/calib");
    m.addIntArg(stage);
    m.addIntArg(count);

    thisMess.sendMessage(m);

}

void userManager::sendIsMoving(int id, bool isMoving){

    ofxOscMessage m;
    m.setAddress("/isMoving");
    m.addIntArg(id);
    m.addIntArg((int)isMoving);
    thisMess.sendMessage(m);

}

userManager::~userManager()
{
    //dtor
}
