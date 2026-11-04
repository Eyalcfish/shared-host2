#ifndef READ_WRITE_H
#define READ_WRITE_H

#include "../shared_host.h"

error basic_send(shared_message* message); // normal send
shared_message* basic_read(shared_connection* connection); // normal read

shared_message* basic_zc_claim_for_send(shared_connection* connection, size_t size); // zero copy send claim
error basic_zc_publish_for_send(shared_message* message); // zero copy send publish

shared_message* basic_zc_claim_for_read(shared_connection* connection); // zero copy read claim
error basic_zc_drop_for_send(shared_message* message); // zero copy read free

#endif
