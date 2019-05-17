# frozen_string_literal: true

export_default :Supervisor

Coprocess   = import('./coprocess')
Exceptions  = import('./exceptions')

class Supervisor
  def initialize
    @coprocesss = []
  end

  def await(&block)
    @supervisor_fiber = Fiber.current
    block&.(self)
    suspend
  rescue Exceptions::MoveOn => e
    e.value
  ensure
    if still_running?
      stop_all_tasks
      suspend
    else
      @supervisor_fiber = nil
    end
  end

  def spawn(proc = nil, &block)
    if proc.is_a?(Coprocess)
      spawn_coprocess(proc)
    else
      spawn_proc(block || proc)
    end
  end

  def spawn_coprocess(proc)
    @coprocesss << proc
    proc.when_done { task_completed(proc) }
    proc.run unless proc.running?
    proc
  end

  def spawn_proc(proc)
    @coprocesss << Object.spawn do |coprocess|
      proc.call(coprocess)
      task_completed(coprocess)
    rescue Exception => e
      task_completed(coprocess)
    end
  end

  def still_running?
    !@coprocesss.empty?
  end

  def stop!(result = nil)
    return unless @supervisor_fiber
  
    @supervisor_fiber&.transfer Exceptions::MoveOn.new(nil, result)
  end

  def stop_all_tasks
    exception = Exceptions::Stop.new
    @coprocesss.each do |c|
      EV.next_tick { c.interrupt(exception) }
    end
  end

  def task_completed(coprocess)
    return unless @coprocesss.include?(coprocess)
    
    @coprocesss.delete(coprocess)
    @supervisor_fiber&.transfer if @coprocesss.empty?
  end
end