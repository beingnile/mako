/*!
 * main.c - main entry point for libsatoshi
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/libsatoshi
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <io/core.h>
#include <io/loop.h>

#include <node/addrman.h>
#include <node/chain.h>
#include <node/logger.h>
#include <node/mempool.h>
#include <node/miner.h>
#include <node/node.h>
#include <node/pool.h>
#include <node/rpc.h>
#include <node/timedata.h>

#include <satoshi/block.h>
#include <satoshi/coins.h>
#include <satoshi/consensus.h>
#include <satoshi/crypto/hash.h>
#include <satoshi/entry.h>
#include <satoshi/header.h>
#include <satoshi/net.h>
#include <satoshi/netaddr.h>
#include <satoshi/netmsg.h>
#include <satoshi/network.h>
#include <satoshi/script.h>
#include <satoshi/tx.h>
#include <satoshi/util.h>
#include <satoshi/vector.h>

#include "../internal.h"

/*
 * Node
 */

btc_node_t *
btc_node_create(const btc_network_t *network) {
  btc_node_t *node = (btc_node_t *)btc_malloc(sizeof(btc_node_t));

  memset(node, 0, sizeof(*node));

  node->network = network;
  node->loop = btc_loop_create();
  node->logger = btc_logger_create();
  node->timedata = btc_timedata_create();
  node->chain = btc_chain_create(network);
  node->mempool = btc_mempool_create(network, node->chain);
  node->miner = btc_miner_create(network, node->loop, node->chain, node->mempool);
  node->pool = btc_pool_create(network, node->loop, node->chain, node->mempool);
  node->rpc = btc_rpc_create(node);

  btc_chain_set_logger(node->chain, node->logger);
  btc_mempool_set_logger(node->mempool, node->logger);
  btc_miner_set_logger(node->miner, node->logger);
  btc_pool_set_logger(node->pool, node->logger);

  btc_chain_set_timedata(node->chain, node->timedata);
  btc_mempool_set_timedata(node->mempool, node->timedata);
  btc_miner_set_timedata(node->miner, node->timedata);
  btc_pool_set_timedata(node->pool, node->timedata);

  return node;
}

void
btc_node_destroy(btc_node_t *node) {
  btc_loop_destroy(node->loop);
  btc_logger_destroy(node->logger);
  btc_timedata_destroy(node->timedata);
  btc_chain_destroy(node->chain);
  btc_mempool_destroy(node->mempool);
  btc_miner_destroy(node->miner);
  btc_pool_destroy(node->pool);
  btc_rpc_destroy(node->rpc);
  btc_free(node);
}

static void
btc_node_log(btc_node_t *node, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  btc_logger_write(node->logger, "node", fmt, ap);
  va_end(ap);
}

int
btc_node_open(btc_node_t *node, const char *prefix, size_t map_size) {
  char path[BTC_PATH_MAX + 1];
  size_t len = btc_path_resolve(path, prefix);
  char file[BTC_PATH_MAX + 1];

  if (len < 1 || len > BTC_PATH_MAX - 20)
    return 0;

  if (!btc_fs_mkdirp(path, 0755))
    return 0;

  btc_path_join(file, path, "debug.log", 0);

  if (!btc_logger_open(node->logger, file))
    return 0;

  btc_node_log(node, "Opening node.");

  if (!btc_chain_open(node->chain, path, map_size))
    goto fail1;

  if (!btc_mempool_open(node->mempool))
    goto fail2;

  if (!btc_miner_open(node->miner))
    goto fail3;

  if (!btc_pool_open(node->pool))
    goto fail4;

  if (!btc_rpc_open(node->rpc))
    goto fail5;

  return 1;
fail5:
  btc_pool_close(node->pool);
fail4:
  btc_miner_close(node->miner);
fail3:
  btc_mempool_close(node->mempool);
fail2:
  btc_chain_close(node->chain);
fail1:
  btc_logger_close(node->logger);
  return 0;
}

void
btc_node_close(btc_node_t *node) {
  btc_rpc_close(node->rpc);
  btc_pool_close(node->pool);
  btc_miner_close(node->miner);
  btc_mempool_close(node->mempool);
  btc_chain_close(node->chain);
  btc_logger_close(node->logger);
}

void
btc_node_start(btc_node_t *node) {
  btc_loop_start(node->loop);
}

void
btc_node_stop(btc_node_t *node) {
  btc_loop_stop(node->loop);
}