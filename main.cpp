#include<iostream>
#include<string>
#include<thread>
#include<future>
#include<mutex>
#include<windows.h>
#include"public.h"
#include"vjoyinterface.h"

using std::cout;
using std::endl;

std::mutex mtx;

volatile bool global_quit = false;
volatile long global_padButtons = 0;

void keyboardHandler()
{
    bool continueLoop = true;
    long requestedButtons = 0;
    std::string userInput;

    while(continueLoop)
    {
        requestedButtons = 0;
        std::cin>>userInput;

        if(userInput == "quit")
        {
            continueLoop = false;
        }
        else
        {
            if(userInput.find('w') != std::string::npos)
                requestedButtons |= 0b00000000000000000001000000000000;
            if(userInput.find('s') != std::string::npos)
                requestedButtons |= 0b00000000000000000010000000000000;
            if(userInput.find('a') != std::string::npos)
                requestedButtons |= 0b00000000000000000100000000000000;
            if(userInput.find('d') != std::string::npos)
                requestedButtons |= 0b00000000000000001000000000000000;
        }

        mtx.lock();
        global_quit = !continueLoop;
        global_padButtons = requestedButtons;
        mtx.unlock();
    }
}

int gamepadHandler(unsigned int DevID = 1)
{
    bool continueLoop = true;

    JOYSTICK_POSITION_V2 padPosition;
    void *ppadPosition;

//Testing if VJoy Driver is installed******************************************
    if(vJoyEnabled())
    {
        cout<<"VJD Enabled"<<endl;
        cout<<"Vendor: "<<static_cast<char *> (GetvJoyManufacturerString())<<endl;
        cout<<"Product: "<<static_cast<char *> (GetvJoyProductString())<<endl;
        cout<<"Version Number: "<<static_cast<char *> (GetvJoySerialNumberString())<<endl;
    }
    else
    {
        cout<<"VJD is bad!"<<endl;
        return 1;
    }

    cout<<"\n";

//Testing if VJoy Driver is same version with DLL******************************
    unsigned short VerDll, VerDrv;
    if (DriverMatch(&VerDll, &VerDrv))
    {
        cout<<"vJoy Driver ("<<VerDrv<<") matches vJoy DLL ("<<VerDll<<"). OK!"<<endl;
    }
    else
    {
        cout<<"FAIL!"<<endl;
        cout<<"vJoy Driver ("<<VerDrv<<") does not match vJoy DLL ("<<VerDll<<")."<<endl;
    }

//Checking virtual device status***********************************************
VjdStat status = GetVJDStatus(DevID);

	switch (status)
	{
	case VJD_STAT_OWN:
		cout<<"vJoy device "<<DevID<<" is already owned by this feeder."<<endl;
		break;
	case VJD_STAT_FREE:
		cout<<"vJoy device "<<DevID<<" is free."<<endl;
		break;
	case VJD_STAT_BUSY:
		cout<<"vJoy device "<<DevID<<" is already owned by another feeder.\nCannot continue."<<endl;
		return -3;
	case VJD_STAT_MISS:
		cout<<"vJoy device "<<DevID<<" is not installed or disabled.\nCannot continue."<<endl;
		return -4;
	default:
		cout<<"vJoy device "<<DevID<<" general error.\nCannot continue."<<endl;
		return -1;
	};

//Acquire VJoy Device**********************************************************
    if (AcquireVJD(DevID))
    {
        cout<<"Successfully acquired device number "<<DevID<<endl;
    }
    else
    {
        cout<<"Failed to acquire device number "<<DevID<<endl;
        return -1;
    }

//Main Loop********************************************************************

long buttonSetting = 0b00000000000000000000000000000000;
padPosition.lButtons = buttonSetting;//32 bit number, 32 pad buttons

    while(continueLoop)
	{
		// Set destination vJoy device
		padPosition.bDevice = (BYTE)DevID;
        padPosition.lButtons = buttonSetting;

		// Send position data to vJoy device
		ppadPosition = static_cast<void*>(&padPosition);
		if (!UpdateVJD(DevID, ppadPosition))
		{
			cout<<"Feeding vJoy device number "<<DevID<<" failed - try to enable device then press enter"<<endl;
			std::cin.get();
			AcquireVJD(DevID);
		}

		if(mtx.try_lock())//try to lock the mutex. if unable to lock, it's fine to go around 1 more loop iteration.
        {
            buttonSetting = global_padButtons;
            continueLoop = !global_quit;
            mtx.unlock();
        }

		Sleep(10);
	}

    cout<<"Relinquishing device "<<DevID;
    RelinquishVJD(DevID);
    return 0;
}

int main(char argc, char *argv[])
{
    unsigned int deviceID = 1; //default to 1 if unspecified
    if(argc > 1)
        deviceID = atoi(argv[1]);

    auto keyboard = std::thread(keyboardHandler);
    auto gamepad = std::async(gamepadHandler, deviceID);

    keyboard.detach();
    return gamepad.get();
}
