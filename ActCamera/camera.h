#ifndef __CAMERA_H
#define __CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoconfig.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include <errno.h>

#define ROWS_CUTS 80

#define STD_PIXS (0.039f * ((float)CCCore.back().y + ROWS_CUTS) * ((float)CCCore.back().y + ROWS_CUTS) - \
                  2.586f * ((float)CCCore.back().y + ROWS_CUTS) - 55.34f)

namespace act
{
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
            for (auto i = 0; i < temp.rows; ++i)
            {
                for (auto j = 0; j < temp.cols; ++j)
                {
					originalImage.ptr<cv::Vec3b>(i)[j] = temp.ptr<cv::Vec3b>(temp.rows - i - 1)[temp.cols - j - 1];
                }
            }
        }

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

		//void showTrackbar();

		void showImage() const
		{
			cv::imshow("ORG", originalImage);
			cv::imshow("ORGROI", originalImage(ROIRect));
			cv::imshow("BSC", basicImage);
			cv::imshow("AB", allBallImage);
			cv::imshow("AG", allGreenImage);
			cv::imshow("FC", fieldCtsImage);
			cv::imshow("FCH", fieldCHImage);
			cv::imshow("NoBGB", noBGBallImage);
			cv::imshow("BP", ballPositionImage);
		}

        void getROIImage(cv::Mat &ri) const { ri = ROIImage; }

        void findConnectedComponents(cv::Mat &binary);

		void areaSort(cv::Mat ballImage);

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
		
		////trackbar class and its vector
		//struct trackbar
		//{
		//	trackbar() {}
		//	trackbar(char* _name, int _maxValue) : name(_name), maxValue(_maxValue) {}
		//	char* name;
		//	int maxValue = 0;
		//	int outputValue = 0;
		//	int slider = 0;
		//	void callback(int, void*)
		//	{
		//		outputValue = slider;
		//	}
		//};
		//trackbar* trackbarSet;

		cv::Mat originalImage;
        cv::Mat basicImage;
		cv::Mat allBallImage;
		cv::Mat allGreenImage;
        cv::Mat noBackgroundImage;
		cv::Mat fieldCtsImage;
        cv::Mat fieldCHImage;
        cv::Mat noBGBallImage;
		cv::Mat ballPositionImage;

        cv::Mat ROIImage;
        cv::Rect ROIRect;

		std::vector<std::vector<cv::Point>> fieldContours;

		std::vector<int> CCSize;
		std::vector<cv::Point> CCCore;

        int usbNumber = 0;
    };
};

#endif // !__CAMERA_H
