require 'fileutils'

counter=0
people_num = 1	    
pic_num =  10
=begin
Dir.foreach(Dir.pwd) do |a|
  dir_name = "i%03d" % [counter]
  if a=~/^m/ then
  	FileUtils.mv a, dir_name
	counter += 1
  end
end
=end

FileUtils.rm 'result.txt'
FileUtils.rm 'names.txt'
File.open("result.txt", 'w'){|r|
  Dir.foreach('.') do |d|
     break if counter+1 > people_num
     if d=~/^i/ then
	pic_counter = 0
  	Dir.foreach(d) do |f|
	  if f=~/jpg/ then
	   pic_counter+=1
	    break if pic_counter>pic_num
	    r.write('db1\\'+d+'\\'+f+';'+counter.to_s+"\n")
	  end
	end
	 counter+=1
     end 
  end
}

File.open('names.txt', 'w'){|r|
	i=0
	while(i<people_num) do
		r.write((0...4).map{(65+rand(26)).chr}.join+"\n")
		i+=1
	end
}
