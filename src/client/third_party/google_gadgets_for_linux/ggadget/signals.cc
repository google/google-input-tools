/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "signals.h"

#include <algorithm>
#include <vector>
#include "logger.h"
#include "small_object.h"

#ifdef _DEBUG
// Uncomment the following line to enable logs.
// #define DEBUG_SIGNALS
#endif

namespace ggadget {

Connection::Connection(Signal *signal, Slot *slot)
    : signal_(signal),
      slot_(slot) {
}

Connection::~Connection() {
  delete slot_;
  slot_ = NULL;
}

void Connection::Disconnect() {
  signal_->Disconnect(this);
}

bool Connection::Reconnect(Slot *slot) {
  delete slot_;
  slot_ = NULL;
  if (slot) {
    if (!signal_->CheckCompatibility(slot)) {
      // According to our convention, no matter Reconnect succeeds or failes,
      // the slot is always owned by the Connection.
      delete slot;
      return false;
    }
    slot_ = slot;
  }
  return true;
}

class Signal::Impl : public SmallObject<> {
 public:
  Impl()
      : default_connection_(NULL),
        death_flag_ptr_(NULL) {
#ifdef DEBUG_SIGNALS
    max_connection_length_ = 0;
#endif
  }

  static void EnsureImpl(Signal *signal) {
    if (!signal->impl_)
      signal->impl_ = new Impl;
  }

  typedef std::vector<Connection *> Connections;
  Connections connections_;
  Connection *default_connection_;

  // During an Emit() call, this Signal object may be deleted in some slot.
  // Emit() should let this pointer point to a local bool variable. Once
  // *death_flag_ptr_ becomes true, Emit() should return immediately.
  bool *death_flag_ptr_;
#ifdef DEBUG_SIGNALS
  size_t max_connection_length_;
#endif
};

#ifdef DEBUG_SIGNALS
static size_t g_max_connection_length = 0;
static size_t g_signals_count = 0;
static size_t g_signals_with_impl_count = 0;
static size_t g_sum_connection_length = 0;
#endif

Signal::Signal() : impl_(NULL) {
}

Signal::~Signal() {
#ifdef DEBUG_SIGNALS
  ++g_signals_count;
  if (impl_)
    ++g_signals_with_impl_count;
  if (g_signals_count % 100 == 0) {
    DLOG("#Signals: %zu #Signals with impl: %zu %f",
         g_signals_count, g_signals_with_impl_count,
         static_cast<double>(g_signals_with_impl_count) / g_signals_count);
  }
#endif
  if (!impl_)
    return;

  for (Impl::Connections::iterator it = impl_->connections_.begin();
       it != impl_->connections_.end(); ++it) {
    delete *it;
  }

  // Set *death_flag_ to true to let Emit() know this Signal is to be deleted.
  if (impl_->death_flag_ptr_)
    *impl_->death_flag_ptr_ = true;

#ifdef DEBUG_SIGNALS
  g_sum_connection_length += impl_->max_connection_length_;
  if (impl_->max_connection_length_ > g_max_connection_length)
    g_max_connection_length = impl_->max_connection_length_;
  if (g_signals_count % 100 == 0) {
    DLOG("#Signals: %zu  MAX#CONNS: %zu  AVG#CONNS: %f",
         g_signals_count, g_max_connection_length,
         static_cast<double>(g_sum_connection_length) / g_signals_count);
  }
#endif

  delete impl_;
}

Connection *Signal::ConnectGeneral(Slot *slot) {
  if (slot && !CheckCompatibility(slot)) {
    // According to our convention, no matter Reconnect succeeds or failes,
    // the slot is always owned by the Connection.
    delete slot;
    return NULL;
  }
  return Connect(slot);
}

bool Signal::CheckCompatibility(const Slot *slot) const {
  if (!slot->HasMetadata())
    return true;

  // Check the compatibility of the arguments and the return value.
  // First, the slot's count of argument must equal to that of this signal.
  int arg_count = GetArgCount();
  if (slot->GetArgCount() != arg_count)
    return false;

  // Second, the slot's return type is compatible with that of this signal.
  // The slot can return any type if this signal returns void.
  Variant::Type return_type = GetReturnType();
  if (return_type != Variant::TYPE_VOID &&
      slot->GetReturnType() != return_type)
    return false;

  // Third, argument types must equal except that the target type is Variant.
  const Variant::Type *slot_arg_types = slot->GetArgTypes();
  const Variant::Type *signal_arg_types = GetArgTypes();
  for (int i = 0; i < arg_count; i++)
    if (slot_arg_types[i] != Variant::TYPE_VARIANT &&
        slot_arg_types[i] != signal_arg_types[i])
      return false;

  return true;
}

bool Signal::HasActiveConnections() const {
  if (!impl_)
    return false;
  if (impl_->connections_.empty())
    return false;
  for (Impl::Connections::const_iterator it = impl_->connections_.begin();
       it != impl_->connections_.end(); ++it) {
    if (*it && (*it)->slot_)
      return true;
  }
  return false;
}

ResultVariant Signal::Emit(int argc, const Variant argv[]) const {
  ResultVariant result = ResultVariant(Variant(GetReturnType()));
  if (!impl_)
    return result;

  bool death_flag = false;
  bool *death_flag_ptr = &death_flag;
  if (!impl_->death_flag_ptr_) {
    // Let the destructor inform us when this object is to be deleted.
    impl_->death_flag_ptr_ = death_flag_ptr;
  } else {
    // There must be some upper stack frame containing Emit() call of the same
    // object. We just use the outer most death_flag_.
    death_flag_ptr = impl_->death_flag_ptr_;
#ifdef DEBUG_SIGNALS
    DLOG("Signal::Emit() Re-entrance");
#endif
  }

  // Can't use iterator here, because new connection might be added during the
  // loop, which may invalidate the iterator.
  size_t n_connections = impl_->connections_.size();
  for (size_t i = 0; i < n_connections && !*death_flag_ptr; ++i) {
    Connection *connection = impl_->connections_[i];
    if (connection && connection->slot_) {
      result = connection->slot_->Call(NULL, argc, argv);
    }
  }

  if (!*death_flag_ptr && death_flag_ptr == &death_flag) {
    impl_->death_flag_ptr_ = NULL;
    // The outer most Emit() should erase all NULL slots in the connection
    // list to save memory. The NULL slots is created by Disconnect() called
    // during this Emit() call.
    Impl::Connections::iterator it = impl_->connections_.begin();
    while (it != impl_->connections_.end()) {
      if (!*it)
        it = impl_->connections_.erase(it);
      else
        ++it;
    }
  }
  return result;
}

Connection *Signal::Connect(Slot *slot) {
  Impl::EnsureImpl(this);
  Connection *connection = new Connection(this, slot);
  impl_->connections_.push_back(connection);
#ifdef DEBUG_SIGNALS
  if (impl_->connections_.size() > impl_->max_connection_length_)
    impl_->max_connection_length_ = impl_->connections_.size();
#endif
  return connection;
}

bool Signal::Disconnect(Connection *connection) {
  ASSERT(impl_);
  Impl::Connections::iterator it = std::find(impl_->connections_.begin(),
                                             impl_->connections_.end(),
                                             connection);
  if (it == impl_->connections_.end())
    return false;

  if (impl_->death_flag_ptr_) {
    // Emit() is executing, so the vector can't be changed here.
    *it = NULL;
#ifdef DEBUG_SIGNALS
    DLOG("Signal::Disconnect() called indirectly by Signal::Emit()");
#endif
  } else {
    impl_->connections_.erase(it);
  }
  delete connection;
  return true;
}

Slot *Signal::GetDefaultSlot() {
  return impl_ && impl_->default_connection_ ?
         impl_->default_connection_->slot_ : NULL;
}

Connection *Signal::SetDefaultSlot(Slot *slot) {
  Impl::EnsureImpl(this);
  if (impl_->default_connection_)
    impl_->default_connection_->Reconnect(slot);
  else
    impl_->default_connection_ = Connect(slot);
  return impl_->default_connection_;
}

size_t Signal::GetConnectionCount() const {
  if (!impl_)
    return 0;
  return impl_->connections_.size();
}

} // namespace ggadget
