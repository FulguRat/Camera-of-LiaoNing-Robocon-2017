#ifndef __CAMERA_H
#define __CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoconfig.h"

//by pixels
//#define BY_PIXELS

namespace act
{
	class Camera : public VCConfig
    {
    public:
        explicit Camera(char _id);
        Camera(const Camera &) = delete;
		Camera &operator=(const Camera &) = delete;
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
            basicImage = originalImage(ROIRect);
            noBackgroundImage = originalImage(ROIRect).clone();

			cv::cvtColor(basicImage, basicImage, CV_BGR2HSV_FULL);
			cv::cvtColor(noBackgroundImage, noBackgroundImage, CV_BGR2HSV_FULL);

            getImage();
        }

		void getImage();

        void setROIRect(const cv::Rect &r)
        {
            ROIRect = r;
            ROICols = r.width;
            ROIRows = r.height;
        }

        void setWhiteThresold(char val) { whiteThresoldValue = val; }
        void setBlackThresold(char val) { blackThresoldValue = val; }

        cv::Mat getOriginalImage() const { return originalImage; }
        cv::Mat getBasicImage() const { return basicImage; }
        cv::Mat getAllBallImage() const { return allBallImage; }
        cv::Mat getBinaryImage() const { return binaryImage; }
        cv::Mat getNoBGImage() const { return noBackgroundImage; }
        cv::Mat getFieldCHImage() const { return fieldCHImage; }

        void getROIImage(cv::Mat &ri) const { ri = ROIImage; }

        struct min_max
        {
            min_max() {}
            min_max(int _min, int _max) : min(_min), max(_max) {}
            int min = 0;
            int max = 0;
        };

        

        void getBlackBinaryImage(cv::Mat &bin) const
        {
            cv::Mat gray;

            cv::cvtColor(ROIImage, gray, cv::COLOR_BGR2GRAY);

            bin = gray < blackThresoldValue;
        }

        void getWhiteBinaryImage(cv::Mat &bin) const
        {
            cv::Mat gray;

            cv::cvtColor(ROIImage, gray, cv::COLOR_BGR2GRAY);
            bin = gray > whiteThresoldValue;
        }

        static void findConnectedComponents(const cv::Mat &binary, std::vector<int> &size, std::vector<cv::Point> &core);

        static uint32_t getWhitePixNumber(const cv::Mat &binary);

        void getContours(std::vector<std::vector<cv::Point>> &contours) const
        {
            std::vector<cv::Vec4i> hie;

            cv::Mat binary;
            getWhiteBinaryImage(binary);

            cv::findContours(binary, contours, hie, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        }

		void areaSort(cv::Mat ballImage, std::vector<int> &size, std::vector<cv::Point> &core);

        int cols = 0;
        int rows = 0;

        int ROICols = 0;
        int ROIRows = 0;

    private:
        cv::VideoCapture videoCapture;

		
		cv::Mat originalImage;
        cv::Mat basicImage;
        cv::Mat noBackgroundImage;
        cv::Mat fieldCHImage;
        cv::Mat binaryImage;
        cv::Mat allBallImage;

        cv::Mat ROIImage;
        cv::Rect ROIRect;

        char whiteThresoldValue = 0;
        char blackThresoldValue = 0;

        int usbNumber = 0;
    };
};

#endif // !__CAMERA_H
