trap(:SIGINT) {
  puts "Exiting"
  exit 0
}

desc "Clean the build output"
task :clean do
  rm_rf 'build' if Dir.exists? 'build'
end

namespace :build do
  desc "Build for mac"
  task :mac64 do
    mkdir 'build' unless Dir.exists? 'build'
    chdir 'build' do
      cp '../lib/OSX_x86_64/libclang.dylib', '.'
      sh "clang++ -I ../include -L . -o clang2json ../clang2json.cpp -lclang -Wl,-rpath,@loader_path"
    end
    puts "Build complete! (Take a peek in the build/ folder)"
  end
end

namespace :rebuild do
  desc "Rebuild for Mac"
  task :mac64 => ['clean', 'build:mac']
end
