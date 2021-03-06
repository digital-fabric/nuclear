# frozen_string_literal: true

require 'bundler/setup'
require 'polyphony'
require 'http/parser'

class Http::Parser
  def setup_async
    self.on_message_complete = proc { @request_complete = true }
  end

  def parse(data)
    self << data
    return nil unless @request_complete

    @request_complete = nil
    self
  end
end

def handle_client(socket)
  parser = Http::Parser.new
  reqs = []
  parser.on_message_complete = proc do |env|
    reqs << Object.new # parser
  end
  socket.read_loop do |data|
    parser << data
    while (req = reqs.shift)
      handle_request(socket, req)
      req = nil
      # snooze
    end
  end
rescue IOError, SystemCallError => e
  # do nothing
ensure
  socket&.close
  parser.reset!
end

def handle_request(client, parser)
  status_code = 200
  data = "Hello world!\n"
  headers = "Content-Length: #{data.bytesize}\r\n"
  client.write "HTTP/1.1 #{status_code}\r\n#{headers}\r\n#{data}"
end

$incoming = Polyphony::Queue.new

$threads = (1..4).map {
  Thread.new {
    Thread.current.setup_fiber_scheduling
    loop {
      conn = $incoming.pop
      puts "#{Thread.current.inspect} pop #{conn.inspect}"
      spin { handle_client(conn) }
    }
  }
}

spin do
  server = TCPServer.open('0.0.0.0', 1234)
  puts "listening on port 1234"

  loop do
    client = server.accept
    $incoming << client
    # spin { handle_client(client) }
  end
end

puts "pid #{Process.pid}"
suspend