#ifndef __CAMERA_H
#define __CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoconfig.h"

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

        cv::Mat getOriginalImageROI() const { return originalImage(ROIRect); }
        cv::Mat getBasicImage() const { return basicImage; }
		cv::Mat getAllBallImage() const { return allBallImage; }
		cv::Mat getNoBGImage() const { return noBackgroundImage; }
		cv::Mat getFieldCHImage() const { return fieldCHImage; }
        cv::Mat getNoBGBallImage() const { return noBGBallImage; }    

		void showImage() const
		{
			cv::imshow("ORG", originalImage(ROIRect));
			cv::imshow("BSC", basicImage);
			cv::imshow("AB", allBallImage);
			cv::imshow("noBG", noBackgroundImage);
			cv::imshow("FCH", fieldCHImage);
			cv::imshow("noBGB", noBGBallImage);
		}

        void getROIImage(cv::Mat &ri) const { ri = ROIImage; }

        static void findConnectedComponents(const cv::Mat &binary, std::vector<int> &size, std::vector<cv::Point> &core);

		void areaSort(cv::Mat ballImage, std::vector<int> &size, std::vector<cv::Point> &core);

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
        cv::Mat noBackgroundImage;
        cv::Mat fieldCHImage;
        cv::Mat noBGBallImage;

        cv::Mat ROIImage;
        cv::Rect ROIRect;

        int usbNumber = 0;
    };
};

#endif // !__CAMERA_H
