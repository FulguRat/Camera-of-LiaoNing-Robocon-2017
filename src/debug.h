#ifndef __DEBUG_H
#define __DEBUG_H

namespace act
{
    class Timestamp
    {
    public:
        void start() { start_ = clock(); }
        void end() { end_ = clock(); }
        double getTime() const { return static_cast<double>(end_ - start_) / CLOCKS_PER_SEC; }

    private:
        clockid_t start_ = 0;
        clockid_t end_ = 0;
    };
};

#endif // !__DEBUG_H
