# frozen_string_literal: true

count = ARGV[0] ? ARGV[0].to_i : 1000

TEST_CMD = 'ruby test/io_uring_test.rb' #'ruby test/run.rb'

def run_test(count)
  puts "#{count}: running tests..."
  system(TEST_CMD)
  return if $?.exitstatus == 0

  puts "Failure after #{count} tests"
  exit!
end

trap('INT') { exit! }
t0 = Time.now
count.times do |i|
  run_test(i + 1)
end
elapsed = Time.now - t0
puts format(
  "Successfully ran %d tests in %f seconds (%f per test)",
  count,
  elapsed,
  elapsed / count
)