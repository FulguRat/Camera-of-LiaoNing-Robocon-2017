#include <iostream>
#include <vector>
#include <unistd.h>
#include "debug.h"
#include "videoconfig.h"
#include "camera.h"

#define __TIMER_START__ timer.start()
#define __TIMER_PRINT__ do { timer.end(); std::cout << (int)(timer.getTime() * 1000) << std::endl; } while (0)

static std::vector<int> getCameraList(int size);

const int32_t EXPOSURE_TIME = 300;  // �����ع�ʱ��
const int32_t WHITE_THRESOLD = 125; // ���ð�����ֵ
const int32_t BLACK_THRESOLD = 35;  // ���ú�����ֵ

const int32_t CAMERA_NUMBER = 1; // ʹ�õ�����ͷ�ĸ���

int main(int argc, char *argv[])
{
    // ��ȡ����ͷ�豸�б�
    auto list = getCameraList(CAMERA_NUMBER);

    if (list.size() < CAMERA_NUMBER)
    {
        std::cout << "There are not " << CAMERA_NUMBER << " cameras." << std::endl;
        exit(-1);
    }

    // ������ͷ
    act::Camera cam0((char)list[0]);

    if (!cam0.is_open())
    {
        std::cout << "Open Camera failed!" << std::endl;
        exit(-2);
    }

    // ��ʼ������
    cam0.setExposureValue(false, EXPOSURE_TIME);
    cam0.setROIRect(cv::Rect(0, 50, cam0.cols, cam0.rows - 50));
    cam0.setWhiteThresold(WHITE_THRESOLD);
    cam0.setBlackThresold(BLACK_THRESOLD);

    act::Timestamp timer;
    while (1)
    {
        __TIMER_PRINT__;
        __TIMER_START__;

        cam0.update();

        ////��ȡ���ص�ֵ�����ص�����
        //      auto original_image0 = cam0.getOriginalImage();
        //      cv::setMouseCallback("ORI", [](int event, int x, int y, int flags, void* userdata)->void
        //{
        //          cv::Vec3b pix;
        //          switch (event)
        //	{
        //          case CV_EVENT_LBUTTONDOWN:
        //              pix = static_cast<cv::Mat*>(userdata)->ptr<cv::Vec3b>(y)[x];
        //              std::cout << "R: " << static_cast<int>(pix[2]) << "\tG: " << static_cast<int>(pix[1]) <<
        //			       "\tB: " << static_cast<int>(pix[0]) << std::endl;
        //              break;

        //          default:
        //              break;
        //          }
        //      }, &original_image0);

        //      cv::imshow("ORI", original_image0);
        cv::imshow("BSC", cam0.getBasicImage());
        cv::imshow("noBG", cam0.getNoBGImage());
        cv::imshow("FCH", cam0.getFieldCHImage());

        // ������
        //      std::vector<int> cc0;
        //      cam0.findConnectedComponents(cam0.getBinaryImage(), cc0);
        //std::cout << "ballNum:[" << cc0.size() << "]" << std::endl;
        cam0.areaSort(cam0.getAllBallImage());
        cv::imshow("AB", cam0.getAllBallImage());

        if (cv::waitKey(10) == 27)
        {
            break;
        }
    }

    return 0;
}

/**
 * @berif ����size������ͷ�豸
 * @ret �����ҵ����豸�����
 */
std::vector<int> getCameraList(int size)
{
    std::vector<int> list;

    // ����ǰ50���豸
    char name[15] = "/dev/video";
    int fd = 0;

    for (int i = 0; i < 50 && list.size() < 1; ++i)
    {
        sprintf(&name[10], "%d", i);

        if ((fd = open(name, O_RDWR | O_NONBLOCK, 0)) != -1)
        {
            list.push_back(i);
            close(fd);
        }
    }

    return list;
}