#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "debug.h"
#include "videoconfig.h"
#include "camera.h"

//#define MESURE_TIME
//#define MESURE_TEMPE
#define RESTART_KEY

//calculate time of every circulation
#define __TIMER_START__ timer.start()
#define __TIMER_PRINT__ do { timer.end(); std::cout << (int)(timer.getTime() * 1000) << std::endl; } while (0)

static std::vector<int> getCameraList(int size);

const int32_t EXPOSURE_TIME = 300;  //time of exposure

const int32_t CAMERA_NUMBER = 1; //camera number

////trackbar globe variable and callback function
/////*exposure value*/
////const int g_eTrackbarMax = 400;
////int g_expoSlider;
///*color theshold*/
//const int g_cTrackbarMax = 255;
//int g_minHSlider;
//int g_maxHSlider;
//int g_minSSlider;
//int g_maxSSlider;
//int g_minVSlider;
//int g_maxVSlider;
//void trackbarCallback(int, void*) {}

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

	//initialize cam0
	//cam0.setExposureValue(true);
	cam0.setAutoWhiteBalance(true);
    cam0.setROIRect(cv::Rect(0, ROWS_CUTS, cam0.cols, cam0.rows - ROWS_CUTS));
	cam0.autoSet();

	//initialize wiringPi and serial
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

#ifdef RESTART_KEY

	//use GPIO0 to restart program
	pinMode(0, INPUT);
	int restartCounter = 0;
	int restartFlag = digitalRead(0);

#endif

	//use GPIO1 AND GPIO2 to control the output
	pinMode(1, INPUT);
	pinMode(2, INPUT);


#ifdef MESURE_TEMPE

	//initialize temperature detection
#define TEMPE_PATH "/sys/class/thermal/thermal_zone0/temp"
#define MAX_SIZE 32

	int tempeCounter = 0;
	int fdTempe;
	char bufTempe[MAX_SIZE];
	if ((fdTempe = open(TEMPE_PATH, O_RDONLY)) < 0)
	{
		fprintf(stderr, "failed to open temperature path\n");
	}
	else
	{
		printf("ready for detecting temperature\n");
		close(fdTempe);
	}

#endif

	//cv::namedWindow("TKB");
	/////*exposure value*/
	////g_expoSlider = 0;
	////cv::createTrackbar("expoVal", "TKB", &g_expoSlider, g_eTrackbarMax, trackbarCallback);
	///*color theshold*/
	//g_minHSlider = 0;
	//g_maxHSlider = 0;
	//g_minSSlider = 0;
	//g_maxSSlider = 0;
	//g_minVSlider = 0;
	//g_maxVSlider = 0;	
	//cv::createTrackbar("minH", "TKB", &g_minHSlider, g_cTrackbarMax, trackbarCallback);
	//cv::createTrackbar("maxH", "TKB", &g_maxHSlider, g_cTrackbarMax, trackbarCallback);
	//cv::createTrackbar("minS", "TKB", &g_minSSlider, g_cTrackbarMax, trackbarCallback);
	//cv::createTrackbar("maxS", "TKB", &g_maxSSlider, g_cTrackbarMax, trackbarCallback);
	//cv::createTrackbar("minV", "TKB", &g_minVSlider, g_cTrackbarMax, trackbarCallback);
	//cv::createTrackbar("maxV", "TKB", &g_maxVSlider, g_cTrackbarMax, trackbarCallback);

    act::Timestamp timer;
    while (1)
    {
#ifdef MESURE_TIME

		__TIMER_PRINT__;
        __TIMER_START__;

#endif

		////test exposure value
		//cam0.setExposureValue(false, g_expoSlider);

		//update frame
        cam0.update();

		//get usdful image
		cam0.getImage();

		if (digitalRead(1) == LOW && digitalRead(2) == LOW)
		{
			//sort to three area and send golf ball num in every area
			cam0.areaSort(cam0.getNoBGBallImage());
		}
		else if (digitalRead(1) == LOW && digitalRead(2) == HIGH)
		{
			//find the angle with most golf ball
			cam0.findOptimalAngle();
		}
		else if (digitalRead(1) == HIGH && digitalRead(2) == LOW)
		{
			//send out distance and angle of the nearest golf ball
			cam0.getNearestBall();
		}
		else
		{
			//send out distance and angle of every golf ball
			cam0.calcPosition();
		}

		//cam0.testTheshold(g_minHSlider, g_maxHSlider, g_minSSlider, g_maxSSlider, g_minVSlider, g_maxVSlider);

		//show all images that have been used
		cam0.showImage();

#ifdef MESURE_TEMPE

		//detecting temperature. If overheating, send out 0XC4
		if (tempeCounter <= 0)
		{
			if ((fdTempe = open(TEMPE_PATH, O_RDONLY)) < 0)
			{
				std::cout << "open path failed\n" << std::endl;
			}
			else
			{
				if (read(fdTempe, bufTempe, MAX_SIZE) < 0)
				{
					std::cout << "read temperature failed\n" << std::endl;
				}
				else
				{
					std::cout << "Temperature: " << atoi(bufTempe) / 1000.0f << std::endl;
					memset(bufTempe, NULL, MAX_SIZE * sizeof(char));
					tempeCounter = 0;
				}
				close(fdTempe);
			}
		}
		else
		{
			tempeCounter--;
		}

#endif

#ifdef RESTART_KEY

		//if GPIO1 change its voltage, restart the camera
		if (digitalRead(0) == restartFlag)
		{
			restartCounter = 0;
		}
		else
		{
			restartCounter++;
		}

		if (restartCounter >= 3)
		{
			break;
			restartCounter = 0;
		}

#endif

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