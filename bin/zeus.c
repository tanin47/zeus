/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <lcthw/ringbuffer.h>
#include <lcthw/list.h>
#include <rtmp.h>
#include <dbg.h>

#define BUFFER_SIZE 131072
#define SERVER_PORT 1935

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

struct fd_state {
    RingBuffer *buffer;
    List *outputs;
    Rtmp *rtmp;

    struct event *read_event;
    struct event *write_event;
};

struct fd_state *
alloc_fd_state(struct event_base *base, evutil_socket_t fd)
{
    struct fd_state *state = malloc(sizeof(struct fd_state));
    if (!state)
        return NULL;
    state->read_event = event_new(base, fd, EV_READ|EV_PERSIST, do_read, state);
    if (!state->read_event) {
        free(state);
        return NULL;
    }

    state->write_event = event_new(base, fd, EV_WRITE|EV_PERSIST, do_write, state);
    if (!state->write_event) {
        event_free(state->read_event);
        free(state);
        return NULL;
    }

    state->rtmp = rtmp_create();
    state->buffer = RingBuffer_create(BUFFER_SIZE);
    state->outputs = List_create();

    assert(state->write_event);
    return state;
}

void
free_fd_state(struct fd_state *state)
{
    event_free(state->read_event);
    event_free(state->write_event);
    RingBuffer_destroy(state->buffer);
    List_destroy(state->outputs);
    rtmp_destroy(state->rtmp);
    free(state);
}

void
do_read(evutil_socket_t fd, short events, void *arg)
{
    (void) events;

    struct fd_state *state = arg;
    ssize_t result;

    while (1) {
        assert(state->write_event);
        result = recv(fd, RingBuffer_ends_at(state->buffer), RingBuffer_available_space(state->buffer), 0);
        if (result <= 0)
            break;

        state->rtmp->total_read += result;

        RingBuffer_commit_write(state->buffer, result);
        
        while (RingBuffer_available_data(state->buffer) > 0) {
            RtmpOutputMessage *this_output = rtmp_create_output_message();
            int result_multiplex = rtmp_multiplex(state->rtmp, state->buffer, this_output);
            check(result_multiplex >= 0, "rtmp_multiplex error %d", result_multiplex);

            if (state->rtmp->end == 1) {
                event_del(state->write_event);
                rtmp_destroy_output_message(this_output);
                free_fd_state(state);
                return;
            }

            if (this_output->length > 0) {
                printf("Add block with length=%u\n", this_output->length);
                List_push(state->outputs, this_output);
                assert(state->write_event);
                event_add(state->write_event, NULL);
            } else {
                rtmp_destroy_output_message(this_output);
            }

            if (result_multiplex == 0) break;
        }

        printf("total_read=%u, last_total_read=%u\n", state->rtmp->total_read, state->rtmp->last_total_read);

        if (rtmp_should_acknowledge(state->rtmp) == 1) {
            RtmpOutputMessage *ack = rtmp_create_output_message();
            rtmp_allocate_output_message_content(ack, 16);

            int i = 0;
            unsigned char *msg = ack->message;
            msg[i++] = 0x02;

            msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;
            int_to_byte_array(4, msg, i, 3);i += 3;

            msg[i++] = 0x03;

            msg[i++] = 0x00;
            msg[i++] = 0x00;  
            msg[i++] = 0x00;
            msg[i++] = 0x00;

            // msg[i++] = 0x03;

            // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x04;
            // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;
            // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;

            int_to_byte_array(state->rtmp->total_read, msg, i, 4);i += 4;

            List_push(state->outputs, ack);
            assert(state->write_event);
            event_add(state->write_event, NULL);

            state->rtmp->last_total_read = state->rtmp->total_read;
        }
    }

    if (result == 0) {
        free_fd_state(state);
    } else if (result < 0) {
        if (errno == EAGAIN) // XXXX use evutil macro
            return;
        perror("recv");
        free_fd_state(state);
    }

error:
    free_fd_state(state);
    return;
}

void
do_write(evutil_socket_t fd, short events, void *arg)
{
    (void) events;

    struct fd_state *state = arg;
    printf("Want to write %d blocks\n", List_count(state->outputs));

    while (List_count(state->outputs) > 0) {
        RtmpOutputMessage *this_output = List_first(state->outputs);

        printf("Block write: %u\n", this_output->data_left);

        while (this_output->data_left > 0) {
            ssize_t result = send(fd, 
                                rtmp_output_message_start_at(this_output), 
                                this_output->data_left, 
                                0);
            
            if (result < 0) {
                if (errno == EAGAIN) // XXX use evutil macro
                    return;
                free_fd_state(state);
                return;
            }
            assert(result != 0);

            this_output->data_left -= result;
            printf("\tWritten: %d\n", result);

            if (this_output->data_left == 0) {
                List_unshift(state->outputs);
                rtmp_destroy_output_message(this_output);
                break;
            }
        }
    }

    event_del(state->write_event);
}

void
do_accept(evutil_socket_t listener, short event, void *arg)
{
    (void) event;

    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) { // XXXX eagain??
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd); // XXX replace all closes with EVUTIL_CLOSESOCKET */
    } else {
        struct fd_state *state;
        evutil_make_socket_nonblocking(fd);
        state = alloc_fd_state(base, fd);
        assert(state); /*XXX err*/
        assert(state->write_event);
        event_add(state->read_event, NULL);
    }
}

void
run(void)
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    base = event_base_new();
    if (!base)
        return; /*XXXerr*/

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(SERVER_PORT);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

#ifndef WIN32
    {
        int one = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
#endif

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return;
    }

    if (listen(listener, 16)<0) {
        perror("listen");
        return;
    }

    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    /*XXX check it */
    event_add(listener_event, NULL);

    event_base_dispatch(base);
}

int
main(int c, char **v)
{
    (void) c;
    (void) v;

    setvbuf(stdout, NULL, _IONBF, 0);

    run();
    return 0;
}