#ifndef USERMANAGER_H
#define USERMANAGER_H

#include "ofxOsc.h"
#include "ofMain.h"
#include "dsUser.h"

#define EX_HOST "172.16.1.2"
#define LOCAL_HOST "localhost"
#define PORT 12345

class userManager
{
    public:
        userManager();
        virtual ~userManager();

        void manageUsers();
        void sendOutputMode(int t_out);
        void sendCalibrationMessage(int stage, int count);
        void sendIsMoving(int id, bool isMoving);

        dsUser * dsUsers;
        vector <int> * activeUserList;

    protected:
    private:

    void sendPoint(int userId, ofVec2f point, bool isFake);
    void sendNewUser(int i);
    void sendLostUser(int i);


    ofxOscSender thisMess;
    int outputMode;

    int currentUserList[20];//this can become a 2D array when 2nd kinect is introduced ?



};

#endif // USERMANAGER_H
