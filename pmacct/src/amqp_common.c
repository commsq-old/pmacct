/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2014 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define __AMQP_COMMON_C

/* includes */
#include "pmacct.h"
#include "pmacct-data.h"
#include "amqp_common.h"

/* Functions */
void p_amqp_init_host(struct p_amqp_host *amqp_host)
{
  if (amqp_host) memset(amqp_host, 0, sizeof(struct p_amqp_host));
}

void p_amqp_set_user(struct p_amqp_host *amqp_host, char *user)
{
  if (amqp_host) amqp_host->user = user;
}

void p_amqp_set_passwd(struct p_amqp_host *amqp_host, char *passwd)
{
  if (amqp_host) amqp_host->passwd = passwd;
}

void p_amqp_set_exchange(struct p_amqp_host *amqp_host, char *exchange)
{
  if (amqp_host) amqp_host->exchange = exchange;
}

void p_amqp_set_routing_key(struct p_amqp_host *amqp_host, char *routing_key)
{
  if (amqp_host) amqp_host->routing_key = routing_key;
}

void p_amqp_set_exchange_type(struct p_amqp_host *amqp_host, char *exchange_type)
{
  if (amqp_host) amqp_host->exchange_type = exchange_type;
}

void p_amqp_set_host(struct p_amqp_host *amqp_host, char *host)
{
  if (amqp_host) amqp_host->host = host;
}

void p_amqp_set_persistent_msg(struct p_amqp_host *amqp_host, int opt)
{
  if (amqp_host) amqp_host->persistent_msg = opt;
}

int p_amqp_connect(struct p_amqp_host *amqp_host, int type)
{
  amqp_host->conn = amqp_new_connection();

  amqp_host->socket = amqp_tcp_socket_new(amqp_host->conn);
  if (!amqp_host->socket) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: no socket\n", config.name, config.type);
    return ERR;
  }

  amqp_host->status = amqp_socket_open(amqp_host->socket, amqp_host->host, 5672 /* default port */);

  if (amqp_host->status) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: unable to open socket\n", config.name, config.type);
    return ERR;
  }

  amqp_host->ret = amqp_login(amqp_host->conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp_host->user, amqp_host->passwd);
  if (amqp_host->ret.reply_type != AMQP_RESPONSE_NORMAL) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: login\n", config.name, config.type);
    return ERR;
  }

  amqp_channel_open(amqp_host->conn, 1);

  amqp_host->ret = amqp_get_rpc_reply(amqp_host->conn);
  if (amqp_host->ret.reply_type != AMQP_RESPONSE_NORMAL) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: unable to open channel\n", config.name, config.type);
    return ERR;
  }

  amqp_exchange_declare(amqp_host->conn, 1, amqp_cstring_bytes(amqp_host->exchange),
                        amqp_cstring_bytes(amqp_host->exchange_type), 0, 0, amqp_empty_table);
  amqp_host->ret = amqp_get_rpc_reply(amqp_host->conn);
  if (amqp_host->ret.reply_type != AMQP_RESPONSE_NORMAL) {
    if (type == AMQP_PUBLISH_LOG) amqp_host->routing_key = NULL;
    return ERR;
  }

  if (amqp_host->persistent_msg) {
    amqp_host->msg_props._flags = (AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG);
    amqp_host->msg_props.content_type = amqp_cstring_bytes("text/json");
    amqp_host->msg_props.delivery_mode = 2; /* persistent delivery */
  }

  return SUCCESS;
}

int p_amqp_publish(struct p_amqp_host *amqp_host, char *json_str, int type)
{
  if (type == AMQP_PUBLISH_MSG) {
    if (config.debug) Log(LOG_DEBUG, "DEBUG ( %s/%s ): publishing [E=%s RK=%s DM=%u]: %s\n", config.name,
			  config.type, amqp_host->exchange, amqp_host->routing_key,
			  amqp_host->msg_props.delivery_mode, json_str);
  }

  if (amqp_host->status == AMQP_STATUS_OK) {  
    amqp_basic_publish(amqp_host->conn, 1, amqp_cstring_bytes(amqp_host->exchange),
		       amqp_cstring_bytes(amqp_host->routing_key), 0, 0, &amqp_host->msg_props,
		       amqp_cstring_bytes(json_str));

    amqp_host->ret = amqp_get_rpc_reply(amqp_host->conn);
    if (amqp_host->ret.reply_type != AMQP_RESPONSE_NORMAL) {
      if (type == AMQP_PUBLISH_LOG) amqp_host->routing_key = NULL;
      Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: publishing\n", config.name, config.type);
      return ERR;
    }
  }
  else return ERR;

  return SUCCESS;
}

void p_amqp_close(struct p_amqp_host *amqp_host, int type)
{
  amqp_channel_close(amqp_host->conn, 1, AMQP_REPLY_SUCCESS);
  amqp_connection_close(amqp_host->conn, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(amqp_host->conn);
}
