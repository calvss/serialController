#include<iostream>
#include<string>
#include<windows.h>
#include"public.h"
#include"vjoyinterface.h"

using std::cout;
using std::endl;

int main(char argc, char *argv[])
{
    unsigned int DevID;
    JOYSTICK_POSITION_V2 padPosition;
    void *ppadPosition;

//interpreting command line arguments******************************************
    if (argc>1)
		DevID = atoi(argv[1]);

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

//Testing if VJoy Driver is correct version with DLL***************************
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
    while (1)
	{
		// Set destenition vJoy device
		padPosition.bDevice = (BYTE)DevID;

		/*
		// Set position data of 3 first axes
		if (Z>35000) Z=0;
		Z += 200;
		padPosition.wAxisZ = Z;
		padPosition.wAxisX = 32000-Z;
		padPosition.wAxisY = Z/2+7000;

		// Set position data of first 8 buttons
		Btns = 1<<(Z/4000);
		padPosition.lButtons = Btns;
		*/

		padPosition.lButtons = ~padPosition.lButtons;

		// Send position data to vJoy device
		ppadPosition = static_cast<void*>(&padPosition);
		if (!UpdateVJD(DevID, ppadPosition))
		{
			cout<<"Feeding vJoy device number "<<DevID<<" failed - try to enable device then press enter"<<endl;
			std::cin.get();
			AcquireVJD(DevID);
		}
		Sleep(500);
	}

    RelinquishVJD(DevID);
    return 0;
}
