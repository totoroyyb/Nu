namespace nu {

namespace rpc_internal {

class RPCServerWorker {
 public:
  RPCServerWorker(std::unique_ptr<rt::TcpConn> c, nu::RPCHandler &handler,
                  Counter &counter);
  ~RPCServerWorker();

  // Sends the return results of an RPC.
  void Return(RPCReturnCode rc, RPCReturnBuffer &&buf,
              std::size_t completion_data);

  // Expose the TcpConn for an accessor on the local/remote IP addresses
  ConnAddrPair GetRPCConnAddrPair();
  netaddr GetLocalAddr();
  netaddr GetRemoteAddr();

 private:
  // Internal worker threads for sending and receiving.
  void SendWorker();
  void ReceiveWorker();

  struct completion {
    RPCReturnCode rc;
    RPCReturnBuffer buf;
    std::size_t completion_data;
  };

  rt::Spin lock_;
  std::unique_ptr<rt::TcpConn> c_;
  nu::RPCHandler &handler_;
  bool close_;
  Counter &counter_;
  rt::ThreadWaker wake_sender_;
  std::vector<completion> completions_;
  float credits_;
  unsigned int demand_;
  rt::Thread sender_;
  rt::Thread receiver_;
};

inline void RPCServerWorker::Return(RPCReturnCode rc, RPCReturnBuffer &&buf,
                                    std::size_t completion_data) {
  rt::SpinGuard guard(&lock_);
  completions_.emplace_back(rc, std::move(buf), completion_data);
  wake_sender_.Wake();
}

inline ConnAddrPair RPCServerWorker::GetRPCConnAddrPair() {
  return ConnAddrPair{GetLocalAddr(), GetRemoteAddr()};
}

inline netaddr RPCServerWorker::GetLocalAddr() { return this->c_->LocalAddr(); }

inline netaddr RPCServerWorker::GetRemoteAddr() {
  return this->c_->RemoteAddr();
}

inline void RPCFlow::Call(std::span<const std::byte> src, RPCCompletion *c) {
  rt::SpinGuard guard(&lock_);
  reqs_.emplace(req_ctx{src, c});
  if (sent_count_ - recv_count_ < credits_) wake_sender_.Wake();
}

}  // namespace rpc_internal

inline RPCReturner::RPCReturner(void *rpc_server, std::size_t completion_data)
    : completion_data_(completion_data) {
  auto server = reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server);
  this->local_addr = server->GetLocalAddr();
  this->remote_addr = server->GetRemoteAddr();
  this->rpc_server_ = rpc_server;
}

inline void RPCReturner::Return(RPCReturnCode rc,
                                std::span<const std::byte> buf,
                                std::move_only_function<void()> deleter_fn) {
  auto rpc_server =
      reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server_);
  rpc_server->Return(rc, RPCReturnBuffer(buf, std::move(deleter_fn)),
                     completion_data_);
}

inline void RPCReturner::Return(RPCReturnCode rc) {
  auto rpc_server =
      reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server_);
  rpc_server->Return(rc, RPCReturnBuffer(), completion_data_);
}

inline ConnAddrPair RPCReturner::GetRPCAddrPair() {
  auto server = reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server_);
  return server->GetRPCConnAddrPair();
}

inline netaddr RPCReturner::GetLocalAddr() {
  auto server = reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server_);
  return server->GetLocalAddr();
}

inline netaddr RPCReturner::GetRemoteAddr() {
  auto server = reinterpret_cast<rpc_internal::RPCServerWorker *>(rpc_server_);
  return server->GetRemoteAddr();
}

inline RPCReturnCode RPCClient::Call(std::span<const std::byte> args,
                                     RPCCallback &&callback) {
#ifdef DDB_SUPPORT
  DDB::DDBTraceMeta meta;
  DDB::get_trace_meta(&meta);

  auto ddb_span = to_span(meta);

  std::vector<std::byte> args_with_ddb;
  args_with_ddb.reserve(ddb_span.size() + args.size());
  args_with_ddb.insert(args_with_ddb.end(), ddb_span.begin(), ddb_span.end());
  args_with_ddb.insert(args_with_ddb.end(), args.begin(), args.end());

  std::span<const std::byte> full_args(args_with_ddb);

  auto out_meta = from_span<DDB::DDBTraceMeta>(full_args);
  BUG_ON(!out_meta.valid());
#else
  std::span<const std::byte> full_args = args;
#endif

  RPCCompletion completion(std::move(callback));
  {
    rt::Preempt p;
    if (!p.IsHeld()) {
      rt::PreemptGuardAndPark guard(&p);
      flows_[p.get_cpu()]->Call(full_args, &completion);
    } else {
      flows_[p.get_cpu()]->Call(full_args, &completion);
    }
  }
  return completion.get_return_code();
}

inline RPCReturnCode RPCClient::Call(std::span<const std::byte> args,
                                     RPCReturnBuffer *return_buf) {
#ifdef DDB_SUPPORT
  DDB::DDBTraceMeta meta;
  DDB::get_trace_meta(&meta);

  auto ddb_span = to_span(meta);

  std::vector<std::byte> args_with_ddb;
  args_with_ddb.reserve(ddb_span.size() + args.size());
  args_with_ddb.insert(args_with_ddb.end(), ddb_span.begin(), ddb_span.end());
  args_with_ddb.insert(args_with_ddb.end(), args.begin(), args.end());

  std::span<const std::byte> full_args(args_with_ddb);

  auto out_meta = from_span<DDB::DDBTraceMeta>(full_args);
  BUG_ON(!out_meta.valid());
#else
  std::span<const std::byte> full_args = args;
#endif

  RPCCompletion completion(return_buf);
  {
    rt::Preempt p;
    if (!p.IsHeld()) {
      rt::PreemptGuardAndPark guard(&p);
      flows_[p.get_cpu()]->Call(full_args, &completion);
    } else {
      flows_[p.get_cpu()]->Call(full_args, &completion);
    }
  }
  return completion.get_return_code();
}

}  // namespace nu
