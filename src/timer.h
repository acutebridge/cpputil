#ifndef CPPUTIL_TIMER_H
#define CPPUTIL_TIMER_H

#include <ctime>

namespace cpputil
{

class Timer
{
  public:

    // Member types
    typedef long sec_type;
    typedef long nsec_type;

    // Modifiers
    void start();
    void stop(); 

    // State functions
    sec_type  sec() const;
    nsec_type nsec() const;

  private:
    timespec begin_;
    timespec end_;
};

inline void Timer::start()
{
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin_);
}

inline void Timer::stop()
{
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_);
}

inline Timer::sec_type Timer::sec() const
{
  return end_.tv_sec - begin_.tv_sec;
}

inline Timer::nsec_type Timer::nsec() const
{
  return 1e9 * sec() + end_.tv_nsec - begin_.tv_nsec;
}

}

#endif