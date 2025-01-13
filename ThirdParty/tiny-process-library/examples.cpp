#include "process.hpp"
#include <iostream>

using namespace std;
using namespace TinyProcessLib;

int main() {
#if !defined(_WIN32) || defined(MSYS_PROCESS_USE_SH)
  // The following examples are for Unix-like systems and Windows through MSYS2


  cout << "Example 1a - the mandatory Hello World through a command" << endl;
  Process process1a("echo Hello World", "", [](const char *bytes, size_t n) {
    cout << "Output from stdout: " << string(bytes, n);
  });
  auto exit_status = process1a.get_exit_status();
  cout << "Example 1a process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 1b - Hello World using arguments" << endl;
  Process process1b(vector<string>{"/bin/echo", "Hello", "World"}, "", [](const char *bytes, size_t n) {
    cout << "Output from stdout: " << string(bytes, n);
  });
  exit_status = process1b.get_exit_status();
  cout << "Example 1b process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


#ifndef _WIN32
  cout << endl
       << "Example 1c - Hello World through a function on Unix-like systems" << endl;
  Process process1c(
      [] {
        cout << "Hello World" << endl;
        exit(0);
      },
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      });
  exit_status = process1c.get_exit_status();
  cout << "Example 1c process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));
#endif


  cout << endl
       << "Example 2 - cd into a nonexistent directory" << endl;
  Process process2(
      "cd a_nonexistent_directory", "",
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      },
      [](const char *bytes, size_t n) {
        cout << "Output from stderr: " << string(bytes, n);
        // Add a newline for prettier output on some platforms:
        if(bytes[n - 1] != '\n')
          cout << endl;
      });
  exit_status = process2.get_exit_status();
  cout << "Example 2 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 3 - async sleep process" << endl;
  thread thread3([]() {
    Process process3("sleep 2");
    auto exit_status = process3.get_exit_status();
    cout << "Example 3 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  });
  thread3.detach();
  this_thread::sleep_for(chrono::seconds(4));


  cout << endl
       << "Example 4 - killing async sleep process after 2 seconds" << endl;
  auto process4 = make_shared<Process>("sleep 4");
  thread thread4([process4]() {
    auto exit_status = process4->get_exit_status();
    cout << "Example 4 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  });
  thread4.detach();
  this_thread::sleep_for(chrono::seconds(2));
  process4->kill();
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 5 - multiple commands, stdout and stderr" << endl;
  Process process5(
      "echo Hello && ls an_incorrect_path", "",
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      },
      [](const char *bytes, size_t n) {
        cout << "Output from stderr: " << string(bytes, n);
        // Add a newline for prettier output on some platforms:
        if(bytes[n - 1] != '\n')
          cout << endl;
      });
  exit_status = process5.get_exit_status();
  cout << "Example 5 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 6 - run bash with input from stdin" << endl;
  Process process6(
      "bash", "",
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      },
      nullptr, true);
  process6.write("echo Hello from bash\n");
  process6.write("exit\n");
  exit_status = process6.get_exit_status();
  cout << "Example 6 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 7 - send data to cat through stdin" << endl;
  Process process7(
      "cat", "",
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      },
      nullptr, true);
  process7.write("Hello cat\n");
  process7.close_stdin();
  exit_status = process7.get_exit_status();
  cout << "Example 7 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 8 - demonstrates Process::try_get_exit_status" << endl;
  Process process8("sleep 3");
  while(!process8.try_get_exit_status(exit_status)) {
    cout << "Example 8 process is running" << endl;
    this_thread::sleep_for(chrono::seconds(1));
  }
  cout << "Example 8 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 9 - launch with different environment" << endl;
  Process process9("printenv", "", {{"VAR1", "value1"}, {"VAR2", "second value"}}, [](const char *bytes, size_t n) {
    std::cout << std::string{bytes, n};
  });
  exit_status = process9.get_exit_status();
  cout << "Example 9 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 10 - launch with normal environment" << endl;
  Process process10("printenv", "", [](const char *bytes, size_t n) {
    std::cout << std::string{bytes, n};
  });
  exit_status = process10.get_exit_status();
  cout << "Example 10 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;


#else
  // Examples for Windows without MSYS2


  cout << "Example 1 - the mandatory Hello World" << endl;
  Process process1("cmd /C echo Hello World", "", [](const char *bytes, size_t n) {
    cout << "Output from stdout: " << string(bytes, n);
  });
  auto exit_status = process1.get_exit_status();
  cout << "Example 1 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 2 - cd into a nonexistent directory" << endl;
  Process process2(
      "cmd /C cd a_nonexistent_directory", "",
      [](const char *bytes, size_t n) {
        cout << "Output from stdout: " << string(bytes, n);
      },
      [](const char *bytes, size_t n) {
        cout << "Output from stderr: " << string(bytes, n);
        // Add a newline for prettier output on some platforms:
        if(bytes[n - 1] != '\n')
          cout << endl;
      });
  exit_status = process2.get_exit_status();
  cout << "Example 2 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 3 - async sleep process" << endl;
  thread thread3([]() {
    Process process3("timeout 2");
    auto exit_status = process3.get_exit_status();
    cout << "Example 3 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  });
  thread3.detach();
  this_thread::sleep_for(chrono::seconds(4));


  cout << endl
       << "Example 4 - killing async sleep process after 2 seconds" << endl;
  auto process4 = make_shared<Process>("timeout 4");
  thread thread4([process4]() {
    auto exit_status = process4->get_exit_status();
    cout << "Example 4 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;
  });
  thread4.detach();
  this_thread::sleep_for(chrono::seconds(2));
  process4->kill();
  this_thread::sleep_for(chrono::seconds(2));


  cout << endl
       << "Example 5 - demonstrates Process::try_get_exit_status" << endl;
  Process process5("timeout 3");
  while(!process5.try_get_exit_status(exit_status)) {
    cout << "Example 5 process is running" << endl;
    this_thread::sleep_for(chrono::seconds(1));
  }
  cout << "Example 5 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << endl;


#endif
}
