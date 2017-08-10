#ifndef __CAMERA_H
#define __CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoconfig.h"

#define __rows_took_out 47

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

		void autoSet();

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
					//auto pix = originalImage.ptr<cv::Vec3b>(i)[j];

					////set BGR gain of original image
					//originalImage.ptr<cv::Vec3b>(i)[j][0] = (uchar)((float)pix[0] * gainBGR[0]);
					//originalImage.ptr<cv::Vec3b>(i)[j][1] = (uchar)((float)pix[1] * gainBGR[1]);
					//originalImage.ptr<cv::Vec3b>(i)[j][2] = (uchar)((float)pix[2] * gainBGR[2]);
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
		cv::Mat getNoBGImage() const { return noBackgroundImage; }
		cv::Mat getFieldCHImage() const { return fieldCHImage; }
        cv::Mat getNoBGBallImage() const { return noBGBallImage; }    

		void showImage() const
		{
			cv::imshow("ORG", originalImage);
			cv::imshow("ORGROI", originalImage(ROIRect));
			cv::imshow("BSC", basicImage);
			cv::imshow("AB", allBallImage);
			cv::imshow("noBG", noBackgroundImage);
			cv::imshow("FCH", fieldCHImage);
			cv::imshow("noBGB", noBGBallImage);
		}

        void getROIImage(cv::Mat &ri) const { ri = ROIImage; }

        void findConnectedComponents(const cv::Mat &binary);

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
		
		cv::Mat originalImage;
        cv::Mat basicImage;
		cv::Mat allBallImage;
        cv::Mat noBackgroundImage;
        cv::Mat fieldCHImage;
        cv::Mat noBGBallImage;
		
		std::vector<cv::Mat> BGRChannels;
		cv::Mat blueImage;
		cv::Mat greenImage;
		cv::Mat redImage;

        cv::Mat ROIImage;
        cv::Rect ROIRect;

		std::vector<int> CCSize;
		std::vector<cv::Point> CCCore;

		float gainBGR[3] = { 1.0f, 1.0f, 1.0f };
		//int brightness = 0;
		int expoTime = 0;

        int usbNumber = 0;
    };
};

#endif // !__CAMERA_H
