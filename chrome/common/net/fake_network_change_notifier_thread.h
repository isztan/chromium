// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_NET_FAKE_NETWORK_CHANGE_NOTIFIER_THREAD_H_
#define CHROME_COMMON_NET_FAKE_NETWORK_CHANGE_NOTIFIER_THREAD_H_

// A fake implementation of NetworkChangeNotifierThread used for
// unit-testing.

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "chrome/common/net/network_change_notifier_thread.h"

namespace net {
class MockNetworkChangeNotifier;
}  // namespace net

namespace chrome_common_net {

class ThreadBlocker;

class FakeNetworkChangeNotifierThread : public NetworkChangeNotifierThread {
 public:
  FakeNetworkChangeNotifierThread();

  virtual ~FakeNetworkChangeNotifierThread();

  // Starts the thread in a blocked state and initializes the network
  // change notifier.
  void Start();

  // Runs the tasks that are currently blocked.  After this call,
  // thread remains in a blocked state.  A call to this function is a
  // memory barrier.
  void Pump();

  // Stops the thread.
  void Stop();

  // Trigger an IP address change notification on the owned network
  // change notifier on the owned thread.
  void NotifyIPAddressChange();

  // Implementation of NetworkChangeNotifierThread.

  virtual MessageLoop* GetMessageLoop() const;

  virtual net::NetworkChangeNotifier* GetNetworkChangeNotifier() const;

 private:
  // Used in unit tests.
  friend class FakeNetworkChangeNotifierThreadDestructionObserver;

  void NotifyIPAddressChangeOnSourceThread();

  scoped_ptr<net::MockNetworkChangeNotifier> network_change_notifier_;
  base::Thread thread_;
  scoped_ptr<ThreadBlocker> thread_blocker_;

  DISALLOW_COPY_AND_ASSIGN(FakeNetworkChangeNotifierThread);
};

}  // namespace chrome_common_net

#endif  // CHROME_COMMON_NET_FAKE_NETWORK_CHANGE_NOTIFIER_THREAD_H_
