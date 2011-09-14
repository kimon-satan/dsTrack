#include "userManager.h"

userManager::userManager()
{
    //ctor
    for(int i = 0; i < 20; i ++)currentUserList[i] = 0;
    outputMode = 0;
    thisMess.setup(HOST, PORT);
    ofxOscMessage m;
    m.setAddress("/init");
    thisMess.sendMessage(m);

}

void userManager::sendOutputMode(int t_out){

        outputMode = t_out;
        ofxOscMessage m;
        m.setAddress("/outputMode");
        m.addIntArg(outputMode);
        thisMess.sendMessage(m);

}


void userManager::manageUsers(){


    //first find compliant users and see if they are new
    for(int  i = 0; i < activeUserList->size(); i++){

        int t_id = activeUserList->at(i);

        if(!dsUsers[t_id].isSleeping && dsUsers[t_id].isPointing && dsUsers[t_id].isIntersect){
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
                        break;}
                    } //find an empty id and assign
            }


        }else{

            for(int j = 0; j < 20; j ++)if(currentUserList[j] == t_id){

            currentUserList[j] = 0;
            sendLostUser(j);

            }; //remove the old users
        }
    }


    if(outputMode == 1){ //for test version send normalised coordinates for all users

         for(int j = 0; j < 20; j ++){
            if(currentUserList[j] > 0){
            int id = currentUserList[j];
            ofVec2f t = dsUsers[id].getScreenIntersect();
            sendPoint(j, t);
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

void userManager::sendPoint(int userId, ofVec2f point){


    ofxOscMessage m;
    m.setAddress("/point");
    m.addIntArg(userId);
    m.addFloatArg(point.x);
    m.addFloatArg(point.y);

    thisMess.sendMessage(m);

}

userManager::~userManager()
{
    //dtor
}
