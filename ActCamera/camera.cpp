#include <opencv2/opencv.hpp>
#include "camera.h"

act::Camera::Camera(char _id) : VCConfig(_id)
{
	if (fd != -1)
	{
		usbNumber = usb_info.back() - 0x30;
		videoCapture.open(_id);
		videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 320);
		videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
		videoCapture >> originalImage;

		cols = originalImage.cols;
		rows = originalImage.rows;

		ROIRect = {0, 0, cols, rows};
	}
}

void act::Camera::findConnectedComponents(cv::Mat &binary)
{
	auto bin = binary.clone();

	std::vector<cv::Point> stk_white;
	bool farFlag = 0;
	unsigned int counter = 0;
	unsigned long long coreX = 0;
	unsigned long long coreY = 0;
	min_max xVal(320, 0);
	min_max yVal(240, 0);

	CCCounter = 0;

	for (auto i = 0; i < bin.rows; ++i)
	{
		for (auto j = 0; j < bin.cols; ++j)
		{
			//when find a point of ball, push it into stack stk_white
			if (bin.ptr(i)[j] == 255)
			{
				bin.ptr(i)[j] = 150;
				stk_white.push_back({j, i});
			}

			//if find a white point connected, which means there are element in stk_white
			while (!stk_white.empty())
			{
				auto pix = stk_white.back();
				
				//if this connected component is too away from comera, get rid of it
				if (pix.y <= 3)
					farFlag = 1;

				stk_white.pop_back();
				counter++;
				coreX += pix.x;
				coreY += pix.y;
				xVal.min = pix.x < xVal.min ? pix.x : xVal.min;
				xVal.max = pix.x > xVal.max ? pix.x : xVal.max;
				yVal.min = pix.y < yVal.min ? pix.y : yVal.min;
				yVal.max = pix.y > yVal.max ? pix.y : yVal.max;

				auto row_0 = pix.y - 1, row_1 = pix.y, row_2 = pix.y + 1;
				auto col_0 = pix.x - 1, col_1 = pix.x, col_2 = pix.x + 1;

				//pay attention to the usage of "do {} while(0)"
#define __pass__(x, y)  do { bin.ptr(y)[x] = 150; stk_white.push_back({x, y}); } while (0)
				//row_0
				if (row_0 >= 0 && col_0 >= 0 && bin.ptr(row_0)[col_0] == 255)
					__pass__(col_0, row_0);
				if (row_0 >= 0 && bin.ptr(row_0)[col_1] == 255)
					__pass__(col_1, row_0);
				if (row_0 >= 0 && col_2 < bin.cols && bin.ptr(row_0)[col_2] == 255)
					__pass__(col_2, row_0);
				//row_1
				if (col_0 >= 0 && bin.ptr(row_1)[col_0] == 255)
					__pass__(col_0, row_1);
				if (col_2 < bin.cols && bin.ptr(row_1)[col_1] == 255)
					__pass__(col_1, row_1);
				//row_2
				if (row_2 < bin.rows && col_0 >= 0 && bin.ptr(row_2)[col_0] == 255)
					__pass__(col_0, row_2);
				if (row_2 < bin.rows && bin.ptr(row_2)[col_1] == 255)
					__pass__(col_1, row_2);
				if (row_2 < bin.rows && col_2 < bin.cols && bin.ptr(row_2)[col_2] == 255)
					__pass__(col_2, row_2);
#undef __pass__
			}
			//push_back the core of connected components and the number of white point
			if (counter > 20)
			{
				coreX /= counter;
				coreY /= counter;
				CCCore.push_back(cv::Point((int)coreX, (int)coreY));

				//if pix number size up/CC is too far/shape is wrong, push pix num into CCSize, else pop x and y out from CCCore
				if ((counter < 0.5f * STD_PIXS || counter > 3.0f * STD_PIXS) || farFlag == 1 ||
					(xVal.max - xVal.min) * (yVal.max - yVal.min) > 2.5f * (float)counter)
				{
					CCCore.pop_back();				
				}
				else
				{
					CCSize.push_back(counter);
					CCCounter++;
				}
			}
			coreX = 0;
			coreY = 0;

			xVal.min = 320;
			xVal.max = 0;
			yVal.min = 240;
			yVal.max = 0;

			farFlag = 0;
			counter = 0;
		}
	}
	////test reference pix num of specific y
	//while (!CCSize.empty() && !CCCore.empty())
	//{
	//	std::cout << CCSize.back() << "  ";
	//	std::cout << CCCore.back() << "  ";

	//	CCSize.pop_back();
	//	CCCore.pop_back();
	//}
	//std::cout << std::endl;
}

void act::Camera::autoSet()
{
#define MAXBGR_MIN 174
#define MAXBGR_MAX 180

#define REFERAREA_MIN_X 106
#define REFERAREA_MIN_Y 80
#define REFERAREA_MAX_X 214
#define REFERAREA_MAX_Y 120

	int averageBGR[3] = { 0 };
	int refPointCounter = 0;
	int maxBGR = 0;
	int initCounter = 5;
	expoTime = 128;

	//set brightness
	do
	{
		//adjust exposure time
		if ((maxBGR < MAXBGR_MAX || maxBGR > MAXBGR_MIN) && initCounter == 0)
		{
			if (maxBGR < MAXBGR_MIN - 30) { expoTime += 2; initCounter = 4; }
			else if (maxBGR >= MAXBGR_MIN - 30 && maxBGR < MAXBGR_MIN) { expoTime++; initCounter = 4; }

			else if (maxBGR > MAXBGR_MAX + 30) { expoTime -= 2; initCounter = 4; }
			else if (maxBGR <= MAXBGR_MAX + 30 && maxBGR > MAXBGR_MAX) { expoTime--; initCounter = 4; }

			else {}
		}
		else if (initCounter > 0) { initCounter--; }
		else { break; }

		setExposureValue(false, expoTime);

		averageBGR[0] = 0;
		averageBGR[1] = 0;
		averageBGR[2] = 0;
		refPointCounter = 0;

		//update original image
		update();
		imshow("ORIGINAL", getOriginalImageROI());
		//get RGB value of white area
		for (int i = REFERAREA_MIN_Y; i < REFERAREA_MAX_Y; i++)
		{
			for (int j = REFERAREA_MIN_X; j < REFERAREA_MAX_X; j++)
			{
				auto pix = originalImage(ROIRect).ptr<cv::Vec3b>(i)[j];

				averageBGR[0] += pix[0];
				averageBGR[1] += pix[1];
				averageBGR[2] += pix[2];

				refPointCounter++;
			}
		}
		averageBGR[0] /= refPointCounter;
		averageBGR[1] /= refPointCounter;
		averageBGR[2] /= refPointCounter;

		//find the minimal value of BGR
		maxBGR = averageBGR[0] > averageBGR[1] ? averageBGR[0] : averageBGR[1];
		maxBGR = maxBGR > averageBGR[2] ? maxBGR : averageBGR[2];

		std::cout << averageBGR[0] << "   " << averageBGR[1] << "   " << averageBGR[2] << "   "  \
			<< expoTime << "   " << maxBGR << std::endl;

		//if push down Esc, kill the progress
		if (cv::waitKey(10) == 27)
		{
			break;
		}
	} while ((maxBGR < MAXBGR_MIN || maxBGR > MAXBGR_MAX) || initCounter > 0);

	std::cout << "Auto set exposure time done" << std::endl;

	////set white balance
	//gainBGR[0] = (float)maxBGR / (float)averageBGR[0];
	//gainBGR[1] = (float)maxBGR / (float)averageBGR[1];
	//gainBGR[2] = (float)maxBGR / (float)averageBGR[2];

	//std::cout << gainBGR[0] << "   " << gainBGR[1] << "   " << gainBGR[2] << std::endl;
	//std::cout << "Auto set white balance done" << std::endl;
}

void act::Camera::getImage()
{
	//get basic image for later handle
	basicImage = originalImage(ROIRect).clone();
	cv::cvtColor(basicImage, basicImage, CV_BGR2HSV_FULL);

	//get the contours value of the green field
	noBackgroundImage = originalImage(ROIRect).clone();
	cv::cvtColor(noBackgroundImage, noBackgroundImage, CV_BGR2HSV_FULL);

	//get all objects that are black and white
	allBallImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	for (auto i = 0; i < basicImage.rows; i++)
	{
		for (auto j = 0; j < basicImage.cols; j++)
		{
			auto pix = basicImage.ptr<cv::Vec3b>(i)[j];

			//white golf ball & black golf ball
			if ((pix[1] < 60 && pix[2] > 185) || pix[2] < 110)
				*allBallImage.ptr<uchar>(i, j) = 255;
			else
				*allBallImage.ptr<uchar>(i, j) = 0;
		}
	}

	//get all objects that are green
	allGreenImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	for (auto i = 0; i < basicImage.rows; i++)
	{
		for (auto j = 0; j < basicImage.cols; j++)
		{
			auto pix = basicImage.ptr<cv::Vec3b>(i)[j];

			//green field, may should add orange/red/blue, fix me
			if (pix[0] > 90 && pix[0] < 140)
				*allGreenImage.ptr<uchar>(i, j) = 255;
			else
				*allGreenImage.ptr<uchar>(i, j) = 0;
		}
	}

	//findContours
	size_t maxCtsSize = 0;
	int maxCtsNumber = -1;

	fieldCtsImage = allGreenImage.clone();
	cv::findContours(fieldCtsImage, fieldContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	for (size_t i = 0; i < fieldContours.size(); i++)
	{
		if (fieldContours[i].size() > maxCtsSize)
		{
			maxCtsSize = fieldContours[i].size();
			maxCtsNumber = i;
		}
	}

	//if there are contours of field
	if (maxCtsNumber >= 0)
	{
		//draw contours
		int CtsCount = fieldContours[maxCtsNumber].size();
		cv::Point point0Cts = fieldContours[maxCtsNumber][CtsCount - 1];
		for (int i = 0; i < CtsCount; i++)
		{
			cv::Point pointCts = fieldContours[maxCtsNumber][i];
			cv::line(fieldCtsImage, point0Cts, pointCts, cv::Scalar(255));
			point0Cts = pointCts;
		}

		//get convex hull of the field
		fieldCHImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
		if (CtsCount > 0)
		{
			std::vector<cv::Point> hull;
			convexHull(fieldContours[maxCtsNumber], hull);

			int hullCount = hull.size();
			cv::Point point0CC = hull[hullCount - 1];
			for (int i = 0; i < hullCount; i++)
			{
				cv::Point pointCC = hull[i];
				cv::line(fieldCHImage, point0CC, pointCC, cv::Scalar(255));
				point0CC = pointCC;
			}
		}

		//delete the part outside the convex hull of allBallImage
		noBGBallImage = allBallImage.clone();
		for (auto i = 0; i < fieldCHImage.rows; i++)
			for (auto j = 0; j < fieldCHImage.cols; j++)
			{
				*noBGBallImage.ptr<uchar>(i, j) = 0;
				if (*fieldCHImage.ptr<uchar>(i, j) == 255)
				{
					for (auto k = fieldCHImage.cols; k > 0; k--)
					{
						*noBGBallImage.ptr<uchar>(i, k) = 0;
						if (*fieldCHImage.ptr<uchar>(i, k) == 255) { break; }
					}

					break;
				}
			}

		////image processing of allBallImage
		//cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(1, 1));
		//cv::morphologyEx(noBGBallImage, noBGBallImage, CV_MOP_CLOSE, element);

		//find connected components and get rid of oversize and undersize parts in noBGBallImage
		findConnectedComponents(noBGBallImage);
	}
	else
	{
		fieldCHImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
		noBGBallImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	}
}

void act::Camera::areaSort(cv::Mat ballImage)
{
	//if ROWS_CUTS > 80, there will be segment fault
	cv::line(ballImage, cv::Point(143, 80 - ROWS_CUTS), cv::Point(8  , 240 - ROWS_CUTS), cv::Scalar(255));
	cv::line(ballImage, cv::Point(189, 80 - ROWS_CUTS), cv::Point(316, 240 - ROWS_CUTS), cv::Scalar(255));

	int incNum = 0;

	areaLNum = 0;
	areaMNum = 0;
	areaRNum = 0;

	ballPositionImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);

	while (!CCSize.empty() || !CCCore.empty())
	{
		double borderXLeft  = 210.5f - 0.8421f * ((float)CCCore.back().y + ROWS_CUTS);
		double borderXRight = 126.2f + 0.79f   * ((float)CCCore.back().y + ROWS_CUTS);

		//judge the number of golf ball in this connected component
		if ((float)CCSize.back() >= 0.5f * STD_PIXS && (float)CCSize.back() <= 1.4f * STD_PIXS)
			incNum = 1;
		else if ((float)CCSize.back() > 1.4f * STD_PIXS && (float)CCSize.back() <= 2.25f * STD_PIXS)
			incNum = 2;
		else if ((float)CCSize.back() > 2.25f * STD_PIXS)
			incNum = 3;
		else
			incNum = 0;
		
		if ((float)CCCore.back().x < borderXLeft)
			areaLNum += incNum;
		else if ((float)CCCore.back().x > borderXRight)
			areaRNum += incNum;
		else
			areaMNum += incNum;

		incNum = 0;

		cv::circle(ballPositionImage, CCCore.back(), 5, 255, 1);

		CCSize.pop_back();
		CCCore.pop_back();
	}
	while (!CCSize.empty() && !CCCore.empty())
	{
		CCSize.pop_back();
		CCCore.pop_back();
	}

	//need better judging condition, fix me
	if (areaLNum == areaRNum && areaRNum == areaMNum && areaMNum == 0)
		targetArea = 0;
	else if (areaLNum >= areaRNum && areaLNum > areaMNum)
		targetArea = 1;
	else if (areaMNum >= areaRNum && areaMNum >= areaLNum)
		targetArea = 2;
	else if (areaRNum >= areaLNum && areaRNum > areaMNum)
		targetArea = 3;
	else
		targetArea = 2;

	//send data from serial
	serialPutchar(act::setFdSerial(), 0xDC);
	serialPutchar(act::setFdSerial(), (unsigned char)areaLNum);
	serialPutchar(act::setFdSerial(), (unsigned char)areaMNum);
	serialPutchar(act::setFdSerial(), (unsigned char)areaRNum);

	std::cout << areaLNum << "   " << areaMNum << "   " << areaRNum << "   Target Area:" << targetArea << std::endl;
}

void act::Camera::findOptimalAngle(void)
{
	for (int i = 0; i < 320; i++)
		ballNumByX[i] = 0;
	optimalAngle = 0;
	
	int incNum = 0;
	int minX = 0;
	int maxX = 0;

	unsigned int maxBallNum = 0;
	int minXWithMaxBall = 0;
	int xNumWithMaxBall = 0;

	bool noBallFlag = 0;

	ballPositionImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);

	if (CCSize.empty() && CCCore.empty())
		noBallFlag = 1;

	while (!CCSize.empty() || !CCCore.empty())
	{
		//judge the number of golf ball in this connected component
		if ((float)CCSize.back() >= 0.5f * STD_PIXS && (float)CCSize.back() <= 1.4f * STD_PIXS)
			incNum = 1;
		else if ((float)CCSize.back() > 1.4f * STD_PIXS && (float)CCSize.back() <= 2.25f * STD_PIXS)
			incNum = 2;
		else if ((float)CCSize.back() > 2.25f * STD_PIXS)
			incNum = 3;
		else
			incNum = 0;

		minX = CCCore.back().x - 63 > 0 ? CCCore.back().x - 63 : 0;
		maxX = CCCore.back().x + 64 < 320 ? CCCore.back().x + 64 : 320;
		for (int i = minX; i < maxX; i++)
		{
			ballNumByX[i] += incNum;
		}

		cv::circle(ballPositionImage, CCCore.back(), 5, 255, 1);

		CCSize.pop_back();
		CCCore.pop_back();
	}
	while (!CCSize.empty() && !CCCore.empty())
	{
		CCSize.pop_back();
		CCCore.pop_back();
	}

	if (noBallFlag == 0)
	{
		for (int i = 0; i < 320; i++)
		{
			if (ballNumByX[i] > maxBallNum)
			{
				maxBallNum = ballNumByX[i];
				minXWithMaxBall = i;
				xNumWithMaxBall = 0;
				while (ballNumByX[i] == maxBallNum)
				{
					xNumWithMaxBall++;
					i++;
					if (i >= 320)
						break;
				}
			}
		}

		optimalAngle = (160 - (minXWithMaxBall + xNumWithMaxBall / 2)) * 50 / 320;
	}
	else
	{
		optimalAngle = 0;
	}

	//send data from serial
	serialPutchar(act::setFdSerial(), 0xDA);
	serialPutchar(act::setFdSerial(), (unsigned char)optimalAngle);

	std::cout << "target angle:" << optimalAngle << std::endl;
}

void act::Camera::getNearestBall(void)
{
	CCMinDist = 200;
	CCMDAngle = 0;

	ballPositionImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);

	while (!CCSize.empty() || !CCCore.empty())
	{
		CCAng.push_back((unsigned char)(((160 - (int)CCCore.back().x) * 25 / 160) + ANG_ERR));
		CCDist.push_back((unsigned char)(129.3f * pow(E, -0.05817f  * (double)CCCore.back().y)) +
			113.6f * pow(E, -0.009029f * (double)CCCore.back().y));
		//CCDist.push_back((unsigned char)(-0.00022f * pow((float)CCCore.back().y, 3) + 0.06142f * pow((float)CCCore.back().y, 2) -
		//	6.18f * (float)CCCore.back().y + 281.7f));

		cv::circle(ballPositionImage, CCCore.back(), 5, 255, 1);

		CCSize.pop_back();
		CCCore.pop_back();
	}
	while (!CCSize.empty() && !CCCore.empty())
	{
		CCSize.pop_back();
		CCCore.pop_back();
	}

	for (unsigned int i = 0; i < CCCounter; i++)
	{
		if (CCDist.back() < CCMinDist)
		{
			CCMinDist = CCDist.back();
			CCMDAngle = CCAng.back();
		}

		CCAng.pop_back();
		CCDist.pop_back();
	}

	//send data from serial
	serialPutchar(act::setFdSerial(), 0xD8);
	if (CCCounter > 0)
	{
		serialPutchar(act::setFdSerial(), (unsigned char)CCMDAngle);
		std::cout << " ang " << (unsigned int)CCMDAngle;

		serialPutchar(act::setFdSerial(), (unsigned char)CCMinDist);
		std::cout << "  min dist " << (unsigned int)CCMinDist;	
	}
	std::cout << std::endl;
}

void act::Camera::calcPosition(void)
{
	ballPositionImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);

	while (!CCSize.empty() || !CCCore.empty())
	{
		CCAng.push_back((unsigned char)(((160 - (int)CCCore.back().x) * 25 / 160) + ANG_ERR));
		CCDist.push_back((unsigned char)(129.3f * pow(E, -0.05817f  * (double)CCCore.back().y)) + 
										 113.6f * pow(E, -0.009029f * (double)CCCore.back().y));

		//judge the number of golf ball in this connected component
		if ((float)CCSize.back() >= 0.5f * STD_PIXS && (float)CCSize.back() <= 1.4f * STD_PIXS)
			CCBNum.push_back(1);
		else if ((float)CCSize.back() > 1.4f * STD_PIXS && (float)CCSize.back() <= 2.25f * STD_PIXS)
			CCBNum.push_back(2);
		else if ((float)CCSize.back() > 2.25f * STD_PIXS)
			CCBNum.push_back(3);
		else
			CCBNum.push_back(0);

		cv::circle(ballPositionImage, CCCore.back(), 5, 255, 1);

		CCSize.pop_back();
		CCCore.pop_back();
	}
	while (!CCSize.empty() && !CCCore.empty())
	{
		CCSize.pop_back();
		CCCore.pop_back();
	}

	//send data from serial
	serialPutchar(act::setFdSerial(), 0xC6);
	for (unsigned int i = 0; i < CCCounter; i++)
	{
		for (unsigned int j = 0; j < CCBNum.back(); j++)
		{
			if (CCDist.back() < 190 && (CCAng.back() >= 231 || CCAng.back() <= 25))
			{
				serialPutchar(act::setFdSerial(), (unsigned char)CCAng.back());
				std::cout << " ang " << (unsigned int)CCAng.back();

				serialPutchar(act::setFdSerial(), (unsigned char)CCDist.back());
				std::cout << "  dist " << (unsigned int)CCDist.back();
			}
		}
		CCAng.pop_back();
		CCDist.pop_back();
		CCBNum.pop_back();
	}
	serialPutchar(act::setFdSerial(), 0xC5);
	std::cout << std::endl;
}

//test threshold of white/black and green
void act::Camera::testThreshold()
{
	update();

	//get basic image for later handle
	basicImage = originalImage(ROIRect).clone();
	cv::cvtColor(basicImage, basicImage, CV_BGR2HSV_FULL);

	allBallImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);

}

//Fd Get_Set
int tmpFdSerial = 0;
void act::getFdSerial(int fdS)
{
	tmpFdSerial = fdS;
}
int act::setFdSerial(void)
{
	return tmpFdSerial;
}