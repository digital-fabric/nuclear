#include "gyro.h"

struct Gyro_Async {
  GYRO_WATCHER_DECL(ev_async);
  VALUE value;
};

VALUE cGyro_Async = Qnil;

static void Gyro_Async_mark(void *ptr) {
  struct Gyro_Async *async = ptr;
  
  GYRO_WATCHER_MARK(async);
  if (async->value != Qnil) rb_gc_mark(async->value);
}

static void Gyro_Async_free(void *ptr) {
  struct Gyro_Async *async = ptr;
  
  GYRO_WATCHER_FREE(async);
}

static size_t Gyro_Async_size(const void *ptr) {
  return sizeof(struct Gyro_Async);
}

static const rb_data_type_t Gyro_Async_type = {
    "Gyro_Async",
    {Gyro_Async_mark, Gyro_Async_free, Gyro_Async_size,},
    0, 0, 0
};

static VALUE Gyro_Async_allocate(VALUE klass) {
  struct Gyro_Async *async = ALLOC(struct Gyro_Async);
  
  return TypedData_Wrap_Struct(klass, &Gyro_Async_type, async);
}

inline void async_activate(struct Gyro_Async *async) {
  if (async->active) return;

  async->active = 1;
  async->fiber = rb_fiber_current();
  async->selector = Thread_current_event_selector();
  async->ev_loop = Gyro_Selector_ev_loop(async->selector);
  Gyro_Selector_add_active_watcher(async->selector, async->self);
  ev_async_start(async->ev_loop, &async->ev_async);
}

inline void async_deactivate(struct Gyro_Async *async) {
  if (!async->active) return;

  ev_async_stop(async->ev_loop, &async->ev_async);
  Gyro_Selector_remove_active_watcher(async->selector, async->self);
  async->active = 0;
  async->ev_loop = 0;
  async->selector = Qnil;
  async->fiber = Qnil;
  async->value = Qnil;
}

void Gyro_Async_callback(struct ev_loop *ev_loop, struct ev_async *ev_async, int revents) {
  struct Gyro_Async *async = (struct Gyro_Async*)ev_async;

  Fiber_make_runnable(async->fiber, async->value);
  async_deactivate(async);
}

#define GetGyro_Async(obj, async) \
  TypedData_Get_Struct((obj), struct Gyro_Async, &Gyro_Async_type, (async))

static VALUE Gyro_Async_initialize(VALUE self) {
  struct Gyro_Async *async;
  GetGyro_Async(self, async);

  GYRO_WATCHER_INITIALIZE(async, self);
  async->value = Qnil;
  ev_async_init(&async->ev_async, Gyro_Async_callback);

  return Qnil;
}

static VALUE Gyro_Async_signal(int argc, VALUE *argv, VALUE self) {
  struct Gyro_Async *async;
  GetGyro_Async(self, async);

  if (!async->active) {
    // printf("signal called before await\n");
    return Qnil;
  }

  async->value = (argc == 1) ? argv[0] : Qnil;
  ev_async_send(async->ev_loop, &async->ev_async);

  return Qnil;
}

VALUE Gyro_Async_await(VALUE self) {
  struct Gyro_Async *async;
  VALUE ret;
  GetGyro_Async(self, async);

  async_activate(async);
  ret = Gyro_switchpoint();
  async_deactivate(async);

  TEST_RESUME_EXCEPTION(ret);
  RB_GC_GUARD(ret);
  return ret;
}

VALUE Gyro_Async_await_no_raise(VALUE self) {
  struct Gyro_Async *async;
  VALUE ret;
  GetGyro_Async(self, async);

  async_activate(async);
  ret = Gyro_switchpoint();
  async_deactivate(async);

  RB_GC_GUARD(ret);
  return ret;
}

void Init_Gyro_Async() {
  cGyro_Async = rb_define_class_under(mGyro, "Async", rb_cData);
  rb_define_alloc_func(cGyro_Async, Gyro_Async_allocate);

  rb_define_method(cGyro_Async, "initialize", Gyro_Async_initialize, 0);
  rb_define_method(cGyro_Async, "await", Gyro_Async_await, 0);
  rb_define_method(cGyro_Async, "signal", Gyro_Async_signal, -1);
}
