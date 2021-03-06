/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#ifndef _MSG_H_
#define _MSG_H_

#include "msg_err.h"
/* EXP ops */
#include "exp_msg_zmq.h"
#include "exp_ops_codes.h"
/* SMIO THSAFE ops */
#include "smio_thsafe_zmq_server.h"
#include "smio_thsafe_zmq_client.h"

#include "dispatch_table.h"

#define MSG_OPCODE_SIZE                     (sizeof (uint32_t))
/* Arbitrary number*/
#define MSG_OPCODE_MAX                      200

/* We add more message types here */
enum _msg_type_e {
    MSG_NOT_SUPPORTED = 0xFFFF,
    MSG_EXP_ZMQ = 0,
    MSG_THSAFE_ZMQ
};

typedef enum _msg_type_e msg_type_e;

msg_type_e msg_guess_type (void *msg);
msg_err_e msg_validate (void *msg, msg_type_e expected_msg_type);
/* Handle MDP protocol (used by SMIOs, for instance) request */
msg_err_e msg_handle_mdp_request (void *owner, void *args,
        disp_table_t *disp_table);
/* Handle regular protocol (used by DEVIOs, for instance) request */
msg_err_e msg_handle_sock_request (void *owner, void *args,
        disp_table_t *disp_table);

#endif

