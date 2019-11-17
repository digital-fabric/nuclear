#ifndef RUBY_EV_H
#define RUBY_EV_H

#include "ruby.h"
#include "ruby/io.h"
#include "libev.h"

void Gyro_add_watcher_ref(VALUE obj);
void Gyro_del_watcher_ref(VALUE obj);
VALUE Gyro_snooze(VALUE self);

VALUE IO_read_watcher(VALUE io);
VALUE IO_write_watcher(VALUE io);
VALUE Gyro_IO_await(VALUE self);

int io_setstrbuf(VALUE *str, long len);
void io_set_read_length(VALUE str, long n, int shrinkable);
VALUE io_enc_str(VALUE str, rb_io_t *fptr);

#define SCHEDULE_FIBER(obj, args...) rb_funcall(obj, ID_transfer, args)
#define YIELD_TO_REACTOR() rb_funcall(Gyro_reactor_fiber, ID_transfer, 0)

#define OBJ_ID(obj) (NUM2LONG(rb_funcall(obj, rb_intern("object_id"), 0)))

extern VALUE mGyro;

extern VALUE Gyro_reactor_fiber;
extern VALUE Gyro_root_fiber;

extern ID ID_call;
extern ID ID_caller;
extern ID ID_clear;
extern ID ID_each;
extern ID ID_inspect;
extern ID ID_next_deferred;
extern ID ID_raise;
extern ID ID_read_watcher;
extern ID ID_scheduled_value;
extern ID ID_transfer;
extern ID ID_write_watcher;
extern ID ID_R;
extern ID ID_W;
extern ID ID_RW;

#endif /* RUBY_EV_H */