#include "../../include/internal/read_write.h"

error basic_send(shared_message* message) {
    return 0;
} // normal send

shared_message* basic_read(shared_connection* connection) {
    return 0;
} // normal read

shared_message* basic_zc_claim_for_send(shared_connection* connection, size_t size) {
    return 0;
} // zero copy send claim

error basic_zc_publish_for_send(shared_message* message) {
    return 0;
} // zero copy send publish

shared_message* basic_zc_claim_for_read(shared_connection* connection) {
    return 0;
} // zero copy read claim

error basic_zc_drop_for_send(shared_message* message) {
    return 0;
} // zero copy read free
