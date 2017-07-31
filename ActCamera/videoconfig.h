#ifndef __VIDEOCONFIG_H
#define __VIDEOCONFIG_H

#include <iostream>
#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>

#include <cstdio>

namespace act
{
    class VCConfig
    {
    public:
        VCConfig() {}
        explicit VCConfig(char _id)
        {
            sprintf(&name[10], "%d", _id);
            fd = open(name, O_RDWR | O_NONBLOCK, 0);

            if (fd != -1)
            {
                ioctl(fd, VIDIOC_QUERYCAP, &buf);
                ioctl(fd, VIDIOC_QUERYCAP, &cap);

                usb_info = std::string((char *)cap.bus_info);
            }
        }

        VCConfig(const VCConfig &) = delete;
        VCConfig &operator=(const VCConfig &) = delete;
        virtual ~VCConfig() {}

        //brightness OK
        void setBrightness(int32_t val) { __set__(V4L2_CID_BRIGHTNESS, val); }

        //contrast OK
        void setContrast(int32_t val) { __set__(V4L2_CID_CONTRAST, val); }

        //saturation OK
        void setSaturation(int32_t val) { __set__(V4L2_CID_SATURATION, val); }

        //hue OK
        void setHue(int32_t val) { __set__(V4L2_CID_HUE, val); }

        //sharpness OK
        void setSharpness(int32_t val) { __set__(V4L2_CID_SHARPNESS, val); }

        //gain NO
        void setGain(int32_t val) { __set__(V4L2_CID_GAIN, val); }

        //gamma OK
        void setGamma(int32_t val) { __set__(V4L2_CID_GAMMA, val); }

        //exposure value OK
        void setExposureValue(bool isAuto, int32_t val = 300)
        {
            if (isAuto)
            {
                __set__(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_APERTURE_PRIORITY);
            }
            else
            {
                __set__(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL);
                __set__(V4L2_CID_EXPOSURE_ABSOLUTE, val);
            }
        }

        //White Balance
        void setAutoWhiteBalance(bool val) { __set__(V4L2_CID_AUTO_WHITE_BALANCE, val ? 1 : 0); }
        void setWhiteBalanceValue(int32_t val) { __set__(V4L2_CID_WHITE_BALANCE_TEMPERATURE, val); }

        //focusing invalid

    protected:
        int32_t fd = 0;
        std::string usb_info;

    private:
        void __set__(uint32_t ty, int32_t val)
        {
            control.id = ty;
            control.value = val;
            ioctl(fd, VIDIOC_S_CTRL, &control);
        }

        char name[15] = "/dev/video";
        struct v4l2_control control = {0, 0};
        struct v4l2_buffer buf;
        struct v4l2_capability cap;
    };
};
#endif