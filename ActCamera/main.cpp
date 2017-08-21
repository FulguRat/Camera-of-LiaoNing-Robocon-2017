#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "debug.h"
#include "videoconfig.h"
#include "camera.h"

//#define SEND_AREA
#define SEND_ANGLE
//#define SEND_COOR

//calculate time of every circulation
#define __TIMER_START__ timer.start()
#define __TIMER_PRINT__ do { timer.end(); std::cout << (int)(timer.getTime() * 1000) << std::endl; } while (0)

static std::vector<int> getCameraList(int size);

const int32_t EXPOSURE_TIME = 300;  //time of exposure

const int32_t CAMERA_NUMBER = 1; //camera number

////trackbar globe variable and callback function
//const int g_trackbarMax = 400;
//int g_trackbarSlider;
//int g_testValue = 0;
//void trackbarCallback(int, void*)
//{
//	g_testValue = g_trackbarSlider;
//}

int main(int argc, char *argv[])
{
    //get camera list
	auto list = getCameraList(CAMERA_NUMBER);

    if (list.size() < CAMERA_NUMBER)
    {
        std::cout << "There are not " << CAMERA_NUMBER << " cameras." << std::endl;
        exit(-1);
    }
	
	//instantiated Camera object
    act::Camera cam0((char)list[0]);

    if (!cam0.is_open())
    {
        std::cout << "Open Camera failed!" << std::endl;
        exit(-2);
    }
	else
	{
		std::cout << "Camera init Done" << std::endl;
	}

	//initialization cam0
	cam0.setExposureValue(true);
	cam0.setAutoWhiteBalance(true);
    cam0.setROIRect(cv::Rect(0, ROWS_CUTS, cam0.cols, cam0.rows - ROWS_CUTS));

	//initialization wiringPi and serial
	wiringPiSetup();
	int fdSerial;
	if ((fdSerial = serialOpen("/dev/ttyAMA0", 115200)) < 0)
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
	}
	else
	{
		act::getFdSerial(fdSerial);
		printf("Serial init done and fdSerial = %d\n", fdSerial);
	}

	//set GPIO1 input mode
	pinMode(0, INPUT);
	int shutdownCounter = 0;
	int shutdownFlag = digitalRead(0);

	//g_trackbarSlider = 0;
	//cv::namedWindow("TKB");
	//cv::createTrackbar("trackBar", "ORG", &g_trackbarSlider, g_trackbarMax, trackbarCallback);
	//trackbarCallback(g_trackbarSlider, 0);

    act::Timestamp timer;
    while (1)
    {
        //__TIMER_PRINT__;
        //__TIMER_START__;

		////test part
		//cam0.setExposureValue(false, g_testValue);

		//update frame
        cam0.update();

		//get usdful image
		cam0.getImage();

#ifdef SEND_AREA  

		/*sort to three area and send golf ball num in every area*/
		cam0.areaSort(cam0.getNoBGBallImage());

#elif defined SEND_ANGLE


#elif defined SEND_COOR  

		/*send out distance and angle of every golf ball*/
		cam0.calcPosition();

#else

#endif

		//show all images that have been used
		cam0.showImage();

		//if GPIO1 change its voltage, shutdown the raspberryPi
		if (digitalRead(0) == shutdownFlag)
		{
			shutdownCounter = 0;
		}
		else
		{
			shutdownCounter++;
		}

		if (shutdownCounter >= 3)
		{
			if (system("shutdown -h now") == 1)
			{
				std::cout << "ready for shutdown" << std::endl;
			}
			else
			{
				std::cout << "shutdown failed!" << std::endl;
			}
			shutdownCounter = 0;
		}

		//if push down Esc, kill the progress
        if (cv::waitKey(10) == 27)
        {
			break;
        }
    }
    return 0;
}

//find 'size' num of camera and return sequence number of them
std::vector<int> getCameraList(int size)
{
	std::vector<int> list;

	//traversal 50 devices
	char name[15] = "/dev/video";
	int fdCamera = 0;

	for (int i = 0; i < 50 && list.size() < 1; ++i)
	{
		sprintf(&name[10], "%d", i);

		if ((fdCamera = open(name, O_RDWR | O_NONBLOCK, 0)) != -1)
		{
			list.push_back(i);
			close(fdCamera);
		}
	}

	return list;
}