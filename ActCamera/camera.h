#ifndef __CAMERA_H
#define __CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoconfig.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include <errno.h>
#include <math.h>

#define E 2.718281828f
#define ROWS_CUTS 80
#define ANG_ERR 1
#define STD_PIXS (0.05521f * ((float)CCCore.back().y + ROWS_CUTS) * ((float)CCCore.back().y + ROWS_CUTS) - \
                  4.805f * ((float)CCCore.back().y + ROWS_CUTS) + 43.07f)

namespace act
{

//fd Get_Set
void getFdSerial(int fdS);
int setFdSerial(void);

//=======================================================================================
//                            definition of Camera class                                 
//=======================================================================================
class Camera : public VCConfig
{
public:
    explicit Camera(char _id);
    Camera(const Camera &) = delete;
	Camera &operator = (const Camera &) = delete;
    virtual ~Camera() {}

    bool is_open() { return fd != -1; }

    void update()
    {
        cv::Mat temp;
        videoCapture >> temp;
        originalImage = temp.clone();
		for (auto i = 0; i < temp.rows; i++)
			for (auto j = 0; j < temp.cols; j++)
			{
				originalImage.ptr<cv::Vec3b>(i)[j] = temp.ptr<cv::Vec3b>(temp.rows - i - 1)[temp.cols - j - 1];
			}
    }

	//auto set exposure time and white balance
	void autoSet();

	void getImage();

    void setROIRect(const cv::Rect &r)
    {
        ROIRect = r;
        ROICols = r.width;
        ROIRows = r.height;
    }

	cv::Mat getoriginalImage() const { return originalImage; }
    cv::Mat getOriginalImageROI() const { return originalImage(ROIRect); }
    cv::Mat getBasicImage() const { return basicImage; }
	cv::Mat getAllBallImage() const { return allBallImage; }
	cv::Mat getAllGreenImage() const { return allGreenImage; }
	cv::Mat getNoBGImage() const { return noBackgroundImage; }
	cv::Mat getFieldCHImage() const { return fieldCHImage; }
    cv::Mat getNoBGBallImage() const { return noBGBallImage; }

	void showImage() const
	{
		//cv::imshow("ORG", originalImage);
		cv::imshow("ORGROI", originalImage(ROIRect));
		//cv::imshow("BSC", basicImage);
		//cv::imshow("AB", allBallImage);
		//cv::imshow("AG", allGreenImage);
		//cv::imshow("FC", fieldCtsImage);
		//cv::imshow("FCH", fieldCHImage);
		cv::imshow("NoBGB", noBGBallImage);
		//cv::imshow("BP", ballPositionImage);
	}

    void getROIImage(cv::Mat &ri) const { ri = ROIImage; }

    void findConnectedComponents(cv::Mat &binary);

	//scheme 1:sort ball to three area and send out area number with most balls
	void areaSort(cv::Mat ballImage);
	//scheme 2:send out angle with most of balls can be get
	void findOptimalAngle(void);
	//scheme 3:send out angle and distance of nearest ball
	void getNearestBall(void);
	//scheme 4:send out angle and distance of every ball
	void calcPosition(void);

	void testTheshold(int minH, int maxH, int minS, int maxS, int minV, int maxV);

	struct min_max
	{
		min_max() {}
		min_max(int _min, int _max) : min(_min), max(_max) {}
		int min = 0;
		int max = 0;
	};

    int cols = 0;
    int rows = 0;

    int ROICols = 0;
    int ROIRows = 0;

private:
    cv::VideoCapture videoCapture;

	cv::Mat originalImage;
    cv::Mat basicImage;
	cv::Mat allBallImage;
	cv::Mat allGreenImage;
    cv::Mat noBackgroundImage;
	cv::Mat fieldCtsImage;
    cv::Mat fieldCHImage;
    cv::Mat noBGBallImage;
	cv::Mat ballPositionImage;
	cv::Mat testImage;

    cv::Mat ROIImage;
    cv::Rect ROIRect;

	std::vector<std::vector<cv::Point>> fieldContours;

	std::vector<int> CCSize;
	std::vector<cv::Point> CCCore;

	int areaLNum = 0;
	int areaMNum = 0;
	int areaRNum = 0;
	int targetArea = 0;

	unsigned int ballNumByX[320] = { 0 };
	int optimalAngle = 0;

	unsigned int CCCounter = 0;
	std::vector<unsigned char> CCAng;
	std::vector<unsigned char> CCDist;
	std::vector<unsigned int> CCBNum;
	unsigned char CCMinDist = 200;
	unsigned char CCMDAngle = 0;

	float gainBGR[3] = { 1.0f, 1.0f, 1.0f };
	//int brightness = 0;
	int expoTime = 0;

    int usbNumber = 0;
};

};

#endif // !__CAMERA_H
