#ifndef GGADGET_QT_QT_MAIN_LOOP_H__
#define GGADGET_QT_QT_MAIN_LOOP_H__
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace qt {

class WatchNode;

/**
 * @ingroup QtLibrary
 * @{
 */

/**
 * QtMainLoop is an implementation of MainLoopInterface based on Qt.
 */
class QtMainLoop : public MainLoopInterface {
 public:
  QtMainLoop();
  virtual ~QtMainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  virtual void Run();
  virtual bool DoIteration(bool may_block);
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;
  virtual bool IsMainThread() const;
  virtual void WakeUp();

  void MarkUnusedWatchNode(WatchNode *watch_node);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(QtMainLoop);
};

/** A helper class used by QtMainLoop */
class WatchNode : public QObject {
  Q_OBJECT
 public:
  MainLoopInterface::WatchType type_;
  bool calling_, removing_;
  QtMainLoop *main_loop_;
  WatchCallbackInterface *callback_;
  QObject *object_;   // pointer to QSocketNotifier or QTimer
  int watch_id_;
  int data_;      // For IO watch, it's fd, for timeout watch, it's interval.

  WatchNode(QtMainLoop *main_loop, MainLoopInterface::WatchType type,
            WatchCallbackInterface *callback);

  virtual ~WatchNode();

 public slots:
  void OnTimeout();
  void OnIOEvent(int fd);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(WatchNode);
};

/** @} */

} // namespace qt
} // namespace ggadget
#endif  // GGADGET_QT_QT_MAIN_LOOP_H__
