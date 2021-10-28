/*
    Copyright (C) Mark Pupilli 2013, All rights reserved
*/
#ifndef __SIMPLE_QUEUE_H__
#define __SIMPLE_QUEUE_H__

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <assert.h>
#include "ComPacket.h"

/**
    Simple FIFO message queue for com-packets.
*/
class SimpleQueue {
 public:
  /**
        This allows you to hold a lock on a queue and have that
        lock automatically released when the LockedQueue object
        goes out of scope.

        A LockedQueue can only be constructed by a call to SimpleQueue::lock();
    */
  class LockedQueue {
    friend class SimpleQueue;

   public:
    LockedQueue(const LockedQueue&) = delete;
    LockedQueue(LockedQueue&& other)
        : m_q(other.m_q) { std::swap(m_secureLock, other.m_secureLock); }
    virtual ~LockedQueue() {}

    template <class Rep, class Period>
    void waitNotEmpty(const std::chrono::duration<Rep, Period>& timeout) {
      if (m_q.m_items.empty()) {
        m_q.m_notEmpty.wait_for(m_secureLock, timeout);
      }
    }

    void waitNotempty() {
      while (m_q.m_items.empty()) {
        m_q.m_notEmpty.wait(m_secureLock);
      }
    }

   private:
    LockedQueue(const SimpleQueue& q)
        : m_q(q), m_secureLock(q.m_lock) {}
    const SimpleQueue& m_q;
    std::unique_lock<std::mutex> m_secureLock;
  };

  SimpleQueue() {}
  SimpleQueue(const SimpleQueue&) = delete;
  virtual ~SimpleQueue() {}

  /**
        This call will block forever if you hold a LockedQueue from
        this queue in the same thread.
    */
  void emplace(const ComPacket::ConstSharedPacket& item) {
    std::lock_guard<std::mutex> guard(m_lock);
    m_items.emplace(item);
    m_notEmpty.notify_all();
  }

  /**
        This call will block forever if you already hold a LockedQueue
        from this queue in the same thread.
    */
  LockedQueue lock() { return LockedQueue(*this); }

  // You may or may not get away with calling these without holding a
  // LockedQueue object, but technically you should hold one before calling them:
  size_t size() const { return m_items.size(); }
  bool empty() const { return m_items.empty(); }
  const ComPacket::ConstSharedPacket& front() const { return m_items.front(); }
  void pop() { m_items.pop(); }

 protected:
 private:
  mutable std::mutex m_lock;
  mutable std::condition_variable m_notEmpty;
  std::queue<ComPacket::ConstSharedPacket> m_items;
};

#endif /* __SIMPLE_QUEUE_H__ */
