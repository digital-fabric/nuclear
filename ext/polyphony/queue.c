#include "polyphony.h"
#include "ring_buffer.h"

typedef struct queue {
  ring_buffer values;
  ring_buffer shift_queue;
} Queue_t;

VALUE cQueue = Qnil;

static void Queue_mark(void *ptr) {
  Queue_t *queue = ptr;
  ring_buffer_mark(&queue->values);
  ring_buffer_mark(&queue->shift_queue);
}

static void Queue_free(void *ptr) {
  Queue_t *queue = ptr;
  ring_buffer_free(&queue->values);
  ring_buffer_free(&queue->shift_queue);
  xfree(ptr);
}

static size_t Queue_size(const void *ptr) {
  return sizeof(Queue_t);
}

static const rb_data_type_t Queue_type = {
  "Queue",
  {Queue_mark, Queue_free, Queue_size,},
  0, 0, 0
};

static VALUE Queue_allocate(VALUE klass) {
  Queue_t *queue;

  queue = ALLOC(Queue_t);
  return TypedData_Wrap_Struct(klass, &Queue_type, queue);
}

#define GetQueue(obj, queue) \
  TypedData_Get_Struct((obj), Queue_t, &Queue_type, (queue))

static VALUE Queue_initialize(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  ring_buffer_init(&queue->values);
  ring_buffer_init(&queue->shift_queue);

  return self;
}

VALUE Queue_push(VALUE self, VALUE value) {
  Queue_t *queue;
  GetQueue(self, queue);
  if (queue->shift_queue.count > 0) {
    VALUE fiber = ring_buffer_shift(&queue->shift_queue);
    if (fiber != Qnil) Fiber_make_runnable(fiber, Qnil);
  }
  ring_buffer_push(&queue->values, value);
  return self;
}

VALUE Queue_unshift(VALUE self, VALUE value) {
  Queue_t *queue;
  GetQueue(self, queue);
  if (queue->shift_queue.count > 0) {
    VALUE fiber = ring_buffer_shift(&queue->shift_queue);
    if (fiber != Qnil) Fiber_make_runnable(fiber, Qnil);
  }
  ring_buffer_unshift(&queue->values, value);
  return self;
}

VALUE Queue_shift(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  if (queue->values.count == 0) {
    VALUE agent = rb_ivar_get(rb_thread_current(), ID_ivar_agent);
    VALUE fiber = rb_fiber_current();
    VALUE switchpoint_result = Qnil;
    ring_buffer_push(&queue->shift_queue, fiber);
    switchpoint_result = LibevAgent_wait_event(agent, Qnil);
    if (RTEST(rb_obj_is_kind_of(switchpoint_result, rb_eException))) {
      ring_buffer_delete(&queue->shift_queue, fiber);
      return rb_funcall(rb_mKernel, ID_raise, 1, switchpoint_result);
    }
    RB_GC_GUARD(agent);
    RB_GC_GUARD(switchpoint_result);
  }

  return ring_buffer_shift(&queue->values);
}

VALUE Queue_shift_no_wait(VALUE self) {
    Queue_t *queue;
  GetQueue(self, queue);

  return ring_buffer_shift(&queue->values);
}

VALUE Queue_delete(VALUE self, VALUE value) {
  Queue_t *queue;
  GetQueue(self, queue);

  ring_buffer_delete(&queue->values, value);
  return self;
}

VALUE Queue_clear(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  ring_buffer_clear(&queue->values);
  return self;
}

long Queue_len(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  return queue->values.count;
}

VALUE Queue_shift_each(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  ring_buffer_shift_each(&queue->values);
  return self;
}

VALUE Queue_shift_all(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  return ring_buffer_shift_all(&queue->values);
}

VALUE Queue_empty_p(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  return (queue->values.count == 0) ? Qtrue : Qfalse;
}

VALUE Queue_size_m(VALUE self) {
  Queue_t *queue;
  GetQueue(self, queue);

  return INT2NUM(queue->values.count);
}

void Init_Queue() {
  cQueue = rb_define_class_under(mPolyphony, "Queue", rb_cData);
  rb_define_alloc_func(cQueue, Queue_allocate);

  rb_define_method(cQueue, "initialize", Queue_initialize, 0);
  rb_define_method(cQueue, "push", Queue_push, 1);
  rb_define_method(cQueue, "<<", Queue_push, 1);
  rb_define_method(cQueue, "unshift", Queue_unshift, 1);

  rb_define_method(cQueue, "shift", Queue_shift, 0);
  rb_define_method(cQueue, "pop", Queue_shift, 0);
  rb_define_method(cQueue, "shift_no_wait", Queue_shift_no_wait, 0);
  rb_define_method(cQueue, "delete", Queue_delete, 1);

  rb_define_method(cQueue, "shift_each", Queue_shift_each, 0);
  rb_define_method(cQueue, "shift_all", Queue_shift_all, 0);
  rb_define_method(cQueue, "empty?", Queue_empty_p, 0);
  rb_define_method(cQueue, "size", Queue_size_m, 0);
}
