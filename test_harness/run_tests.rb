require "rubygems"
require "rainbow"

class TestScript
  
  def initialize(filename)
    # Save filename
    @filename = filename
    # Initialise expected return value
    load(filename)
  end
  
  # Run test case
  def run()
    @output = `./mycc -i < #{@filename}`
  end
  
  def passed?()
    @output && @output.include?(@expected_result + "\n")
  end
  
  def passed_str()
    if self.passed?
      "PASSED".foreground(:green).bright
    else
      "FAILED".foreground(:red).bright
    end
  end
  
  def to_s()
    if @output
      "#{@filename} - Expected result: \"#{@expected_result}\", #{passed_str}"
    else
      ""
    end
  end
  
  private # load() should not be publically accessible
  
  def load(filename)
    File.open(filename, 'r') do |f1|  
      # Read the first line
      @expected_result = f1.gets.gsub!("/*", "").gsub!("*/", "").strip
    end
  end
end

class TestExecutor 
  
  def initialize(directory)
    @directory = directory
    @passed = 0
    @ran = 0
    @contents = Dir.new(@directory).entries
  end
  
  def execute()
    @passed = 0
    @ran = 0
    @contents.each do |file|
      if file && file!="." && file!=".."
        @ts = TestScript.new("#{@directory}/#{file}")
        @ts.run
        puts @ts.to_s
        @passed += 1 unless !@ts.passed?
        @ran += 1
      end
    end
  end
  
  def to_s()
    if @ran == 0
      ""
    else
      @percentage = (Float.induced_from(@passed) / Float.induced_from(@ran)).to_f() * 100
      if @percentage.ceil == 100
        @color = :green
      else
        @color = :red
      end
      @percentage_str = sprintf("%.2f\%", @percentage).bright.foreground(@color)
      "Ran: #{@ran}, passed = #{@passed} ; Test results = #{@percentage_str}"
    end
  end
  
end

argc = ARGV.size
if argc != 1
  puts "--C Test Harness by Henry Thacker\nUsage: run_tests.rb [dir with example files]"
  puts "Each test file should start with a line /*Result: [EXPECTED RESULT]*/ (no spacing)"
else
  @te = TestExecutor.new(ARGV[0].to_s)
  @te.execute
  puts @te.to_s
end