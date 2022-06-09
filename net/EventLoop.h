#ifndef IMITATE_MUDUO_EVENTLOOP_H
#define IMITATE_MUDUO_EVENTLOOP_H

#include "TimerId.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace lrpc {
namespace net {

using namespace util;

class Channel;
class Poller;
class TimerQueue;
class Epoller;

class EventLoop {
public:
  using TimerCallback = std::function<void()>;
  using Functor = std::function<void()>;

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 给其他线程调用
  void runInLoop(const Functor &cb);
  // void runInLoop(Functor &&cb);
  /// 将 cb 放入队列并在必要时唤醒 IO 线程
  void queueInLoop(const Functor &cb);
  // void queueInLoop(Functor &&cb);

  void cancel(TimerId timerId);

  void wakeup();

  TimerId runAt(const Timestamp &time, const TimerCallback &cb);
  TimerId runAfter(double delay, const TimerCallback &cb);
  TimerId runEvery(double interval, const TimerCallback &cb);

  // 更新 EventLoop 关心的文件描述符事件
  void updateChannel(Channel *channel);
  // 移除 channel
  void removeChannel(Channel *channel);

  // 断言
  void assertInLoopThread() {
    if (!isInLoopThread())
      abortNotInLoopThread();
  }

  // 检查当前线程是不是 EventLoop 所属线程
  bool isInLoopThread() const {
    return std::this_thread::get_id() == threadId_;
  }

  static EventLoop *getEventLoopOfCurrentThread();

private:
  bool looping_;
  bool quit_;
  bool callingPendingFunctors_;

  std::thread::id threadId_; // EventLoop 所属线程编号
  Timestamp pollReturnTime_; // 记录上一次 Poller::poll 返回的时间
  std::unique_ptr<Poller> poller_;         // 执行 IO mutilplexing
  std::unique_ptr<TimerQueue> timerQueue_; // TimerQueue
  int wakeupFd_;                           // eventfd，用来唤醒 IO 线程
  std::unique_ptr<Channel> wakeChannel_; // 处理 wakeupFd_ 上的 readable 事件
  std::vector<Channel *> activeChannels_; // 记录每一次调用 poll 的活动事件
  std::mutex mutex_; // 在多线程间保护 pendingFunctors_
  std::vector<Functor> pendingFunctors_;

  // 报错
  void abortNotInLoopThread();
  void handleRead();
  void doPendingFunctors();
};
} // namespace net
} // namespace lrpc

#endif