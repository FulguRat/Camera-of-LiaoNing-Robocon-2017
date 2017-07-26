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

uint32_t act::Camera::getWhitePixNumber(const cv::Mat &binary)
{
	uint32_t counter = 0;
	for (int i = binary.cols * binary.rows - 1; i > 0; --i)
		if (binary.data[i] == 255)
			counter++;

	return counter;
}

void act::Camera::findConnectedComponents(const cv::Mat &binary, std::vector<int> &res)
{
	auto bin = binary.clone();

	std::vector<cv::Point> stk_white;
	unsigned long long counter = 0;

	for (auto i = 0; i < bin.rows; ++i)
	{
		for (auto j = 0; j < bin.cols; ++j)
		{
			if (bin.ptr(i)[j] == 255)
			{
				bin.ptr(i)[j] = 150;
				stk_white.push_back({j, i});
			}
			//else if(binary.ptr(i)[j] == )

			while (!stk_white.empty())
			{
				auto pix = stk_white.back();
				stk_white.pop_back();
				counter++;

				auto row_0 = pix.y - 1, row_1 = pix.y, row_2 = pix.y + 1;
				auto col_0 = pix.x - 1, col_1 = pix.x, col_2 = pix.x + 1;

#define __pass__(x, y)  do { bin.ptr(y)[x] = 150; stk_white.push_back({x, y}); } while (0)

				if (row_0 >= 0 && col_0 >= 0 && bin.ptr(row_0)[col_0] == 255)
					__pass__(col_0, row_0);
				if (row_0 >= 0 && bin.ptr(row_0)[col_1] == 255)
					__pass__(col_1, row_0);
				if (row_0 >= 0 && col_2 < bin.cols && bin.ptr(row_0)[col_2] == 255)
					__pass__(col_2, row_0);

				if (col_0 >= 0 && bin.ptr(row_1)[col_0] == 255)
					__pass__(col_0, row_1);
				if (col_2 < bin.cols && bin.ptr(row_1)[col_1] == 255)
					__pass__(col_1, row_1);

				if (row_2 < bin.rows && col_0 >= 0 && bin.ptr(row_2)[col_0] == 255)
					__pass__(col_0, row_2);
				if (row_2 < bin.rows && bin.ptr(row_2)[col_1] == 255)
					__pass__(col_1, row_2);
				if (row_2 < bin.rows && col_2 < bin.cols && bin.ptr(row_2)[col_2] == 255)
					__pass__(col_2, row_2);
#undef __pass__
			}
			if (counter)
			{
				res.push_back(counter);
				counter = 0;
			}
		}
	}
}

void act::Camera::getImage()
{
	auto col_val = new min_max[basicImage.rows];
	auto row_val = new min_max[basicImage.cols];

	//��ȡ�����������ͼ��
	allBallImage = cv::Mat::zeros(basicImage.rows, basicImage.cols, CV_8UC1);
	for (auto i = 0; i < basicImage.rows; i++)
	{
		for (auto j = 0; j < basicImage.cols; j++)
		{
			auto pix = basicImage.ptr<cv::Vec3b>(i)[j];

			if ((pix[0] < 40 && pix[1] < 35 && pix[2] < 35) || (pix[0] > 100 && pix[1] > 120 && pix[2] > 60))
				*allBallImage.ptr<uchar>(i, j) = 225;
			else
				*allBallImage.ptr<uchar>(i, j) = 0;
		}
	}

	//��ȡ����ͼ��
	for (auto i = 0; i < noBackgroundImage.rows; ++i)
	{
		for (auto j = 0; j < noBackgroundImage.cols; ++j)
		{
			auto pix = noBackgroundImage.ptr<cv::Vec3b>(i)[j];
			if (pix[0] > 45 && pix[1] > 45 && pix[2] < 35)
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

	//����Ϊ������Ϊ��
	for (auto i = 0; i < noBackgroundImage.rows; i++)
	{
		for (auto j = 0; j < noBackgroundImage.cols; j++)
		{
			if ((i < row_val[j].max && i > row_val[j].min) || (j > col_val[i].min && j < col_val[i].max))
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {225, 225, 225};
			else
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};

			if (i == 0 || i == noBackgroundImage.rows - 1 || j == 0 || j == noBackgroundImage.cols - 1)
				*noBackgroundImage.ptr<cv::Vec3b>(i, j) = {0, 0, 0};
		}
	}
	cv::cvtColor(noBackgroundImage, noBackgroundImage, CV_RGB2GRAY);

	//ȡ��Ե
	std::vector<cv::Point> contours;
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = 0; j < noBackgroundImage.cols; j++)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = noBackgroundImage.cols - 1; j >= 0; j--)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = 0; i < noBackgroundImage.rows; i++)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
			{
				contours.push_back({j, i});
				break;
			}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = noBackgroundImage.rows - 1; i >= 0; i--)
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
			{
				contours.push_back({j, i});
				break;
			}

	//��ȡ͹��,need to get the largest ConvexHull, fix me
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

	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = 0; j < noBackgroundImage.cols; j++)
		{
			*allBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
				break;
		}
	for (auto i = 0; i < noBackgroundImage.rows; i++)
		for (auto j = noBackgroundImage.cols - 1; j >= 0; j--)
		{
			*allBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
				break;
		}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = 0; i < noBackgroundImage.rows; i++)
		{
			*allBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
				break;
		}
	for (auto j = 0; j < noBackgroundImage.cols; j++)
		for (auto i = noBackgroundImage.rows - 1; i >= 0; i--)
		{
			*allBallImage.ptr<uchar>(i, j) = 0;
			if (*noBackgroundImage.ptr<uchar>(i, j) == 225)
				break;
		}

	cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
	cv::morphologyEx(allBallImage, allBallImage, CV_MOP_CLOSE, element);

	element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4));
	cv::morphologyEx(allBallImage, allBallImage, CV_MOP_OPEN, element);

	delete[] col_val;
	delete[] row_val;
}

void act::Camera::areaSort(cv::Mat ballImage)
{
	cv::line(ballImage, cv::Point(257.14, 0), cv::Point(0, 180), cv::Scalar(255));
	cv::line(ballImage, cv::Point(75.76, 0), cv::Point(303, 175), cv::Scalar(255));

	int areaLNum = 0, areaMNum = 0, areaRNum = 0;
	int targetArea = 0;

	for (auto i = 0; i < ballImage.rows; i++)
	{
		double borderXLeft = 257.14 - 1.43 * i;
		double borderXRight = 75.76 + 1.3 * i;

		for (auto j = 0; j < ballImage.cols; j++)
		{
			if (*ballImage.ptr<uchar>(i, j) == 225)
			{
				if (j < borderXLeft)
					areaLNum++;
				else if (j > borderXRight)
					areaRNum++;
				else
					areaMNum++;
			}
		}
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