trap(:SIGINT) {
  puts "Exiting"
  exit 0
}

desc "Clean the build output"
task :clean do
  rm_rf 'build' if Dir.exists? 'build'
end

desc "Builds clang2json"
task :build do
  # Requires the libclang-dev package
  mkdir 'build' unless Dir.exists?('build')
  sh "clang++ `llvm-config --cflags` ./clang2json.cpp `llvm-config --ldflags` -lclang -o build/clang2json"
end

desc "Install the executable as a symlink in /usr/local/bin"
task :symlink do
  sh "ln -s `pwd`/build/clang2json /usr/local/bin/clang2json"
end

