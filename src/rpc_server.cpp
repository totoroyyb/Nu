#include "nu/rpc_server.hpp"

#include "nu/commons.hpp"
#include "nu/ctrl_server.hpp"
#include "nu/migrator.hpp"
#include "nu/proclet_server.hpp"
#include "nu/runtime.hpp"
#include "nu/utils/rpc.hpp"
#include "nu/utils/utils.hpp"
#include <iostream> 

namespace nu {

RPCServer::RPCServer()
    : listener_(kPort, [&](std::span<std::byte> args, RPCReturner *returner) {
        handler_fn(args, returner);
      }) {}

void RPCServer::handler_fn(std::span<std::byte> args, RPCReturner *returner) {
  auto &rpc_type = from_span<RPCReqType>(args);
  auto remote_addr = returner->GetRemoteAddr();
  auto local_addr = returner->GetLocalAddr();

  switch (rpc_type) {
    // Migrator
    case kReserveConns: {
      auto &req = from_span<RPCReqReserveConns>(args);
      get_runtime()->reserve_conns(req.dest_server_ip);
      returner->Return(kOk);
      break;
    }
    case kForward: {
      auto &req = from_span<RPCReqForward>(args);
      get_runtime()->migrator()->forward_to_client(req);
      returner->Return(kOk);
      break;
    }
    case kMigrateThreadAndRetVal: {
      auto &req = from_span<RPCReqMigrateThreadAndRetVal>(args);
      auto rc = req.handler(req.dest_proclet_header, req.dest_ret_val_ptr,
                            req.payload_len, req.payload);
      returner->Return(rc);
      break;
    }
    // Controller
    case kRegisterNode: {
      auto &req = from_span<RPCReqRegisterNode>(args);
      auto resp = get_runtime()->controller_server()->handle_register_node(req);
      auto span = to_span(*resp);
      returner->Return(kOk, span, [resp = std::move(resp)] {});
      break;
    }
    case kAllocateProclet: {
      auto &req = from_span<RPCReqAllocateProclet>(args);
      auto resp =
          get_runtime()->controller_server()->handle_allocate_proclet(req);
      auto span = to_span(*resp);
      returner->Return(kOk, span, [resp = std::move(resp)] {});
      break;
    }
    case kDestroyProclet: {
      auto &req = from_span<RPCReqDestroyProclet>(args);
      get_runtime()->controller_server()->handle_destroy_proclet(req);
      returner->Return(kOk);
      break;
    }
    case kResolveProclet: {
      auto &req = from_span<RPCReqResolveProclet>(args);
      auto resp =
          get_runtime()->controller_server()->handle_resolve_proclet(req);
      auto span = to_span(*resp);
      returner->Return(kOk, span, [resp = std::move(resp)] {});
      break;
    }
    case kDestroyLP: {
      auto &req = from_span<RPCReqDestroyLP>(args);
      get_runtime()->controller_server()->handle_destroy_lp(req);
      returner->Return(kOk);
      break;
    }
    // Proclet server
    case kProcletCall: {
      args = args.subspan(sizeof(RPCReqType));
      auto meta = from_span<RPCReqProcletCallDebugMeta>(args);
      uint64_t magic = meta.magic;
      assert(magic == tMetaMagic);
      std::stringstream ss;
      ss << "RIP: 0x" << std::hex << meta.rip << ", RSP: 0x" << std::hex << meta.rsp << std::endl;
      ss << "Remote Addr: " << utils::IPUtils::uint32_to_str(remote_addr.ip) << ":" << std::dec << remote_addr.port << "; value: " << remote_addr.ip << std::endl;
      ss << "Local Addr: " << utils::IPUtils::uint32_to_str(local_addr.ip) << ":" << std::dec << local_addr.port << "; value: " << local_addr.ip << std::endl;
      DEBUG_P(ss.str());
      args = args.subspan(sizeof(RPCReqProcletCallDebugMeta));
      get_runtime()->proclet_server()->parse_and_run_handler(args, returner);
      break;
    }
    case kGCStack: {
      auto &req = from_span<RPCReqGCStack>(args);
      get_runtime()->stack_manager()->put(req.stack);
      returner->Return(kOk);
      break;
    }
    case kShutdown: {
      get_runtime()->shutdown(returner);
      break;
    }
    default:
      BUG();
  }
}

void RPCServer::dec_ref_cnt() { listener_.dec_ref_cnt(); }

}  // namespace nu
