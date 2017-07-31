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

void act::Camera::findConnectedComponents(const cv::Mat &binary, std::vector<int> &size, std::vector<cv::Point> &core)
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
				core.push_back(cv::Point((int)coreX, (int)coreY));
				coreX = 0;
				coreY = 0;

				size.push_back(counter);
				counter = 0;
			}
		}
	}
}

void act::Camera::getImage()
{
	auto col_val = new min_max[basicImage.rows];
	auto row_val = new min_max[basicImage.cols];

	//get all objects that are black and white
	allBallImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	for (auto i = 0; i < basicImage.rows; i++)
	{
		for (auto j = 0; j < basicImage.cols; j++)
		{
			auto pix = basicImage.ptr<cv::Vec3b>(i)[j];

			//white golf ball & black golf ball
			if ((pix[0] < 170 && pix[1] < 75) || (pix[0] < 180 && pix[1] < 160 && pix[2] < 30))
				*allBallImage.ptr<uchar>(i, j) = 255;
			else
				*allBallImage.ptr<uchar>(i, j) = 0;
		}
	}

	//get the contours value of the green field
	for (auto i = 0; i < noBackgroundImage.rows; ++i)
	{
		for (auto j = 0; j < noBackgroundImage.cols; ++j)
		{
			auto pix = noBackgroundImage.ptr<cv::Vec3b>(i)[j];

			//green field
			if (pix[0] > 115 && pix[0] < 145 && pix[1] > 90 && pix[2] < 145)
			{
				if (!col_val[i].min)
					col_val[i].min = j;
				col_val[i].max = j;

				if (!row_val[j].min)
					row_val[j].min = i;
				row_val[j].max = i;
			}
		}
	}
	//fill the vacancy in green field nearby the border
	for (auto i = 0; i < noBackgroundImage.rows; ++i)
	{
		for (auto j = 0; j < noBackgroundImage.cols; ++j)
		{
			if ((i < row_val[j].max && i > row_val[j].min) && (col_val[i].max - col_val[i].min > 100))
			{
				if (col_val[i].min > j)
					col_val[i].min = j;
				if (col_val[i].max < j)
					col_val[i].max = j;
			}

			if ((j > col_val[i].min && j < col_val[i].max) && (row_val[j].max - row_val[j].min > 100))
			{
				if (row_val[j].max < i)
					row_val[j].max = i;
				if (row_val[j].min > i)
					row_val[j].min = i;
			}
		}
	}

	//transform the field white and the others black, convert the image to gray image
	for (auto i = 0; i < noBackgroundImage.rows; i++)
	{
		for (auto j = 0; j < noBackgroundImage.cols; j++)
		{
			if ((i < row_val[j].max && i > row_val[j].min) || (j > col_val[i].min && j < col_val[i].max))
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {255, 255, 255};
			else
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};

			if (i == 0 || i == noBackgroundImage.rows - 1 || j == 0 || j == noBackgroundImage.cols - 1)
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};
		}
	}
	cv::cvtColor(noBackgroundImage, noBackgroundImage, CV_RGB2GRAY);

	//find contours
	std::vector<cv::Point> contours;
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = 0; j < noBackgroundImage.cols; j++)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = noBackgroundImage.cols - 1; j >= 0; j--)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = 0; i < noBackgroundImage.rows; i++)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = noBackgroundImage.rows - 1; i >= 0; i--)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
			{
				contours.push_back({j, i});
				break;
			}

	//get convex hull of the field
	fieldCHImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	if (contours.size() > 0)
	{
		std::vector<cv::Point> hull;
		convexHull(contours, hull);

		int hullCount = hull.size();
		cv::Point point0 = hull[hullCount - 1];
		for (int i = 0; i < hullCount; i++)
		{
			cv::Point point = hull[i];
			cv::line(fieldCHImage, point0, point, cv::Scalar(255));
			point0 = point;
		}
	}

	//delete the part outside the convex hull of allBallImage
	noBGBallImage = allBallImage.clone();
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = 0; j < noBackgroundImage.cols; j++)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
				break;
		}
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = noBackgroundImage.cols - 1; j >= 0; j--)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
				break;
		}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = 0; i < noBackgroundImage.rows; i++)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
				break;
		}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = noBackgroundImage.rows - 1; i >= 0; i--)
		{
			*noBGBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 255)
				break;
		}

	//image processing of allBallImage
	cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
	cv::morphologyEx(noBGBallImage, noBGBallImage, CV_MOP_CLOSE, element);

	//element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4));
	//cv::morphologyEx(noBGBallImage, noBGBallImage, CV_MOP_OPEN, element);

	delete[] col_val;
	delete[] row_val;
}

void act::Camera::areaSort(cv::Mat ballImage, std::vector<int> &size, std::vector<cv::Point> &core)
{
	cv::line(ballImage, cv::Point(257, 0), cv::Point(0  , 180), cv::Scalar(255));
	cv::line(ballImage, cv::Point(76 , 0), cv::Point(303, 175), cv::Scalar(255));

	int areaLNum = 0, areaMNum = 0, areaRNum = 0, incNum = 0;
	int targetArea = 0;

	while (!size.empty() && !core.empty())
	{
		float stdPixNum = 8.90f * (float)core.back().y - 882.9f;

		double borderXLeft = 257.14f - 1.43f * (float)core.back().y;
		double borderXRight = 75.76f + 1.3f * (float)core.back().y;

		//judge the number of golf ball in this connected component
		if ((float)size.back() >= 0.6f * stdPixNum && (float)size.back() <= 1.5f * stdPixNum)
			incNum = 1;
		else if ((float)size.back() > 1.5f * stdPixNum && (float)size.back() <= 2.25f * stdPixNum)
			incNum = 2;
		else if ((float)size.back() > 2.25f * stdPixNum)
			incNum = 3;
		else
			incNum = 0;

		if ((float)core.back().x < borderXLeft)
			areaLNum += incNum;
		else if ((float)core.back().x > borderXRight)
			areaRNum += incNum;
		else
			areaMNum += incNum;

		incNum = 0;

		size.pop_back();
		core.pop_back();
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