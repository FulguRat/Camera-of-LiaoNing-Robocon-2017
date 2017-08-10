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

void act::Camera::findConnectedComponents(const cv::Mat &binary)
{
	auto bin = binary.clone();

	std::vector<cv::Point> stk_white;
	unsigned int counter = 0;
	unsigned long long coreX = 0;
	unsigned long long coreY = 0;

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
				stk_white.pop_back();
				counter++;
				coreX += pix.x;
				coreY += pix.y;

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
				coreX = 0;
				coreY = 0;

				CCSize.push_back(counter);
				counter = 0;
			}
		}
	}
}

void act::Camera::autoSet()
{
	int averageBGR[3] = { 0 };
	int refPointCounter = 0;
	int minBGR = 0;
	int initCounter = 3;

	//set brightness
	do
	{
	//	//adjust brightness
	//	if (minBGR < 100 && initCounter == 0) { brightness += 10; }
	//	else if (minBGR >= 100 && minBGR < 130 && initCounter == 0) { brightness++; }

	//	else if (minBGR > 170 && initCounter == 0) { brightness -= 10; }
	//	else if (minBGR <= 170 && minBGR > 140 && initCounter == 0) { brightness--; }

	//	else if (initCounter > 0) { initCounter--; }
	//	else { break; }

	//	setBrightness(brightness);

	//	averageBGR[0] = 0;
	//	averageBGR[1] = 0;
	//	averageBGR[2] = 0;
	//	refPointCounter = 0;

	//	//update original image
	//	update();
	//	imshow("ORIGINAL", getOriginalImageROI());
	//	//get RGB value of white area
	//	for (int i = 80; i < originalImage(ROIRect).rows; i++)
	//	{
	//		for (int j = 300; j < originalImage(ROIRect).cols; j++)
	//		{
	//			auto pix = originalImage(ROIRect).ptr<cv::Vec3b>(i)[j];

	//			averageBGR[0] += pix[0];
	//			averageBGR[1] += pix[1];
	//			averageBGR[2] += pix[2];

	//			refPointCounter++;
	//		}
	//	}
	//	averageBGR[0] /= refPointCounter;
	//	averageBGR[1] /= refPointCounter;
	//	averageBGR[2] /= refPointCounter;

	//	//find the minimal value of BGR
	//	minBGR = averageBGR[0] < averageBGR[1] ? averageBGR[0] : averageBGR[1];
	//	minBGR = minBGR < averageBGR[2] ? minBGR : averageBGR[2];
	//	
	//	std::cout << averageBGR[0] << "   " << averageBGR[1] << "   " << averageBGR[2] << "   "  \
			<< brightness << "   " << minBGR << std::endl;

	//	//if push down Esc, kill the progress
	//	if (cv::waitKey(10) == 27)
	//	{
	//		break;
	//	}
	//} while ((minBGR < 130 || minBGR > 140) || initCounter > 0);

	//std::cout << "Auto set brightness done" << std::endl;

	//adjust exposure time
		if (minBGR < 100 && initCounter == 0) { expoTime += 10; }
		else if (minBGR >= 100 && minBGR < 130 && initCounter == 0) { expoTime++; }

		else if (minBGR > 170 && initCounter == 0) { expoTime -= 10; }
		else if (minBGR <= 170 && minBGR > 140 && initCounter == 0) { expoTime--; }

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
		for (int i = 80; i < originalImage(ROIRect).rows; i++)
		{
			for (int j = 300; j < originalImage(ROIRect).cols; j++)
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
		minBGR = averageBGR[0] < averageBGR[1] ? averageBGR[0] : averageBGR[1];
		minBGR = minBGR < averageBGR[2] ? minBGR : averageBGR[2];

		std::cout << averageBGR[0] << "   " << averageBGR[1] << "   " << averageBGR[2] << "   "  \
			<< expoTime << "   " << minBGR << std::endl;

		//if push down Esc, kill the progress
		if (cv::waitKey(10) == 27)
		{
			break;
		}
	} while ((minBGR < 130 || minBGR > 140) || initCounter > 0);

	std::cout << "Auto set exposure time done" << std::endl;

	//set white balance
	gainBGR[0] = (float)minBGR / (float)averageBGR[0];
	gainBGR[1] = (float)minBGR / (float)averageBGR[1];
	gainBGR[2] = (float)minBGR / (float)averageBGR[2];

	std::cout << gainBGR[0] << "   " << gainBGR[1] << "   " << gainBGR[2] << std::endl;
	std::cout << "Auto set white balance done" << std::endl;
}

void act::Camera::getImage()
{
	auto col_val = new min_max[originalImage(ROIRect).rows];
	auto row_val = new min_max[originalImage(ROIRect).cols];

	////equalize hist
	//cv::split(originalImage, BGRChannels);

	//blueImage = BGRChannels.at(0);
	//greenImage = BGRChannels.at(1);
	//redImage = BGRChannels.at(2);

	//cv::equalizeHist(blueImage , blueImage );
	//cv::equalizeHist(greenImage, greenImage);
	//cv::equalizeHist(redImage  , redImage  );

	//cv::merge(BGRChannels, originalImage);

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
			if ((/*pix[0] > 60 && pix[0] < 160 && */pix[1] < 100 && pix[2] > 100/* && pix[2] < 130*/) || 
				(/*pix[0] > 70 && */pix[0] < 140 &&/* pix[1] > 60 && pix[1] < 90 && pix[2] > 30 && */pix[2] < 25))
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

			//green field, should add orange/red/blue, fix me
			if (pix[0] > 100 && pix[0] < 130 && /*pix[1] > 60 && pix[1] < 100 && */pix[2] > 0 && pix[2] < 120)
				*allGreenImage.ptr<uchar>(i, j) = 255;
			else
				*allGreenImage.ptr<uchar>(i, j) = 0;
		}
	}

	//findContours
	size_t maxCtsSize = 0;
	int maxCtsNumber = 0;

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

	int CtsCount = fieldContours[maxCtsNumber].size();
	cv::Point point0Cts = fieldContours[maxCtsNumber][CtsCount - 1];
	for (int i = 0; i < CtsCount; i++)
	{
		cv::Point pointCts = fieldContours[maxCtsNumber][i];
		cv::line(fieldCtsImage, point0Cts, pointCts, cv::Scalar(255));
		point0Cts = pointCts;
	}

	//for (auto i = 0; i < noBackgroundImage.rows; ++i)
	//{
	//	for (auto j = 0; j < noBackgroundImage.cols; ++j)
	//	{
	//		auto pix = noBackgroundImage.ptr<cv::Vec3b>(i)[j];

	//		//green / orange / blue / redfield
	//		if ((pix[0] > 100 && pix[0] < 140 && pix[1] > 100 &&/* pix[1] < 180 && */pix[2] > 25 && pix[2] < 160) ||
	//			(pix[0] < 30  && pix[1] > 230 && pix[2] > 60 && pix[2] < 130))
	//		{
	//			if (!col_val[i].min)
	//				col_val[i].min = j;
	//			col_val[i].max = j;

	//			if (!row_val[j].min)
	//				row_val[j].min = i;
	//			row_val[j].max = i;
	//		}
	//	}
	//}
	////fill the vacancy in green field nearby the border
	//for (auto i = 0; i < noBackgroundImage.rows; ++i)
	//{
	//	for (auto j = 0; j < noBackgroundImage.cols; ++j)
	//	{
	//		if ((i < row_val[j].max && i > row_val[j].min) && (col_val[i].max - col_val[i].min > 100))
	//		{
	//			if (col_val[i].min > j)
	//				col_val[i].min = j;
	//			if (col_val[i].max < j)
	//				col_val[i].max = j;
	//		}

	//		if ((j > col_val[i].min && j < col_val[i].max) && (row_val[j].max - row_val[j].min > 100))
	//		{
	//			if (row_val[j].max < i)
	//				row_val[j].max = i;
	//			if (row_val[j].min > i)
	//				row_val[j].min = i;
	//		}
	//	}
	//}

	////transform the field white and the others black, convert the image to gray image
	//for (auto i = 0; i < noBackgroundImage.rows; i++)
	//{
	//	for (auto j = 0; j < noBackgroundImage.cols; j++)
	//	{
	//		if ((i < row_val[j].max && i > row_val[j].min) || (j > col_val[i].min && j < col_val[i].max))
	//			*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {255, 255, 255};
	//		else
	//			*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};

	//		if (i == 0 || i == noBackgroundImage.rows - 1 || j == 0 || j == noBackgroundImage.cols - 1)
	//			*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};
	//	}
	//}
	//cv::cvtColor(noBackgroundImage, noBackgroundImage, CV_RGB2GRAY);

	////find contours
	//std::vector<cv::Point> contours;
	//for (auto i = 0; i < noBackgroundImage.rows; i++)
	//	for (auto j = 0; j < noBackgroundImage.cols; j++)
	//		if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
	//		{
	//			contours.push_back({j, i});
	//			break;
	//		}
	//for (auto i = 0; i < noBackgroundImage.rows; i++)
	//	for (auto j = noBackgroundImage.cols - 1; j >= 0; j--)
	//		if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
	//		{
	//			contours.push_back({j, i});
	//			break;
	//		}
	//for (auto j = 0; j < noBackgroundImage.cols; j++)
	//	for (auto i = 0; i < noBackgroundImage.rows; i++)
	//		if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
	//		{
	//			contours.push_back({j, i});
	//			break;
	//		}
	//for (auto j = 0; j < noBackgroundImage.cols; j++)
	//	for (auto i = noBackgroundImage.rows - 1; i >= 0; i--)
	//		if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
	//		{
	//			contours.push_back({j, i});
	//			break;
	//		}

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
				break;
		}
	for (auto i = 0; i < fieldCHImage.rows; i++)
		for (auto j = fieldCHImage.cols - 1; j >= 0; j--)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*fieldCHImage.ptr<uchar>(i, j) == 255)
				break;
		}
	for (auto j = 0; j < fieldCHImage.cols; j++)
		for (auto i = 0; i < fieldCHImage.rows; i++)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*fieldCHImage.ptr<uchar>(i, j) == 255)
				break;
		}
	for (auto j = 0; j < fieldCHImage.cols; j++)
		for (auto i = fieldCHImage.rows - 1; i >= 0; i--)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*fieldCHImage.ptr<uchar>(i, j) == 255)
				break;
		}

	////insert function of deleting oversize and undersize connected components here


	////image processing of allBallImage
	//cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2));
	//cv::morphologyEx(noBGBallImage, noBGBallImage, CV_MOP_CLOSE, element);

	//element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4));
	//cv::morphologyEx(noBGBallImage, noBGBallImage, CV_MOP_OPEN, element);

	delete[] col_val;
	delete[] row_val;
}

void act::Camera::areaSort(cv::Mat ballImage)
{
	//modify with macro __rows_took_out
	cv::line(ballImage, cv::Point(128, 0), cv::Point(0  , 90), cv::Scalar(255));
	cv::line(ballImage, cv::Point(193, 0), cv::Point(320, 98), cv::Scalar(255));

	int areaLNum = 0, areaMNum = 0, areaRNum = 0, incNum = 0;
	int targetArea = 0;

	while (!CCSize.empty() && !CCCore.empty())
	{
		float stdPixNum = 10.13f * (float)CCCore.back().y - 128.30f;

		//modify with macro __rows_took_out
		double borderXLeft = 128.44f - 1.43f * (float)CCCore.back().y;
		double borderXRight = 192.76f + 1.3f * (float)CCCore.back().y;

		//judge the number of golf ball in this connected component
		if ((float)CCSize.back() >= 0.6f * stdPixNum && (float)CCSize.back() <= 1.5f * stdPixNum)
			incNum = 1;
		else if ((float)CCSize.back() > 1.5f * stdPixNum && (float)CCSize.back() <= 2.25f * stdPixNum)
			incNum = 2;
		else if ((float)CCSize.back() > 2.25f * stdPixNum)
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

		CCSize.pop_back();
		CCCore.pop_back();
	}

	//need better judging condition, fix me
	if (areaLNum >= areaRNum && areaLNum > areaMNum)
		targetArea = 1;
	else if (areaMNum >= areaRNum && areaMNum >= areaLNum)
		targetArea = 2;
	else if (areaRNum >= areaLNum && areaRNum > areaMNum)
		targetArea = 3;
	else
		targetArea = 2;

	std::cout << areaLNum << "   " << areaMNum << "   " << areaRNum << "   " << targetArea << std::endl;

	areaLNum = 0;
	areaMNum = 0;
	areaRNum = 0;
}